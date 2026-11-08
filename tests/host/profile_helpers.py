"""Fixed-corpus helper probes; not compiler optimizations or general Ada rules."""
from collections import Counter
import hashlib
import re

kKernelHashes = {
    'blockcalls': 'e1ed9eddd193b088e6430f65d528a89a08196dd9622610f51b7b782426c81df7',
    'allocationcalls': 'b98226f783afe91bb42e9f1ecdca3e6710825eebc7ee0fe5a806d9c8f76b81b6',
}
kVariants = {
    'blockcalls': ('control', 'control-copy', 'inline-checked-offset', 'inline-aggregate-zero', 'omit-aggregate-zero',
                   'inline-small-copy', 'inline-aggregate', 'inline-offset-and-aggregate'),
    'allocationcalls': ('control', 'control-copy', 'rebuilt-locks', 'no-storage-locks',
                       'guard-finalization', 'guard-rewind', 'guard-cleanup'),
}
kResidualVariants = {
    'blockcalls': ('control', 'control-copy', 'inline-checked-offset', 'fold-constant-offset',
                   'inline-small-copy', 'inline-offset-and-copy', 'fold-offset-and-copy'),
    'allocationcalls': ('control', 'control-copy'),
}
kOffsetCall = re.compile(r'    (%[\w.]+) =l call \$__ada_array_offset\(l ([^,]+), l (-?\d+), l (-?\d+), l (-?\d+)\)\n')


def validateKernel(name, text):
    if name not in kKernelHashes or hashlib.sha256(text.encode()).hexdigest() != kKernelHashes[name]:
        raise ValueError('helper probes require the reviewed fixed benchmark corpus')


def faultIndex(source, index):
    def replace(match):
        result, _, first, last, stride = match.groups()
        return f'    {result} =l call $__ada_array_offset(l {index}, l {first}, l {last}, l {stride})\n'
    result, count = kOffsetCall.subn(replace, source)
    if count != 1:
        raise ValueError('expected one checked offset for the fault control')
    return result


def validateResidual(source):
    buffers = set(re.findall(r'^    (%[\w.]+) =l alloc8 16$', source, re.MULTILINE))
    zeros = re.findall(r'^    storel 0, (%[\w.]+)\n    (%[\w.]+) =l add \1, 8\n    storel 0, \2$', source, re.MULTILINE)
    if len(zeros) != 1 or zeros[0][0] not in buffers or 'call $memset' in source:
        raise ValueError('residual probes require the reviewed inline-zero production shape')


def foldConstantOffset(source):
    def replace(match):
        result, index, first, last, stride = match.groups()
        if re.fullmatch(r'-?\d+', index) is None:
            # wideBound emits an explicit signed extension even for Integer
            # literals. Accept only that reviewed, exact literal definition.
            definitions = re.findall(r'^    ' + re.escape(index) + r' =l extsw (-?\d+)$',
                                     source[:match.start()], re.MULTILINE)
            if len(definitions) != 1 or not -(1 << 31) <= int(definitions[0]) < (1 << 31):
                raise ValueError('constant-offset control requires a proven literal index')
            index = definitions[0]
        index, first, last, stride = map(int, (index, first, last, stride))
        if not -(1 << 63) <= first <= index <= last < (1 << 63) or not 0 <= stride < (1 << 63):
            raise ValueError('constant-offset control requires valid nonpacked bounds/stride')
        distance = index - first
        offset = distance * stride
        if distance >= (1 << 63) or offset >= (1 << 63):
            raise ValueError('constant-offset control requires a proven signed offset')
        return f'    {result} =l copy {offset}\n'
    result, count = kOffsetCall.subn(replace, source)
    if count != 1:
        raise ValueError('expected one constant offset for the control')
    return result


def withoutStorageLocks(source):
    for name in ('__ada_mutex_lock', '__ada_mutex_unlock'):
        pattern = r'(^void ' + name + r'\(AdaMutex\* mutex\)\n\{\n)'
        source, count = re.subn(pattern, lambda m: m[0] +
            '    if (mutex == &__ada_main_context.sharedStorageMutex) { return; }\n', source, flags=re.MULTILINE)
        if count != 1:
            raise ValueError('unexpected runtime lock definition')
    return '#include "adart.h"\n' + source


def transform(source, variant):
    if variant in ('control', 'control-copy'):
        return source, {}
    if variant in ('fold-constant-offset', 'fold-offset-and-copy'):
        folded = foldConstantOffset(source)
        if variant == 'fold-constant-offset':
            return folded, {'constant-offset': 1}
        rewritten, changes = transform(folded, 'inline-small-copy')
        return rewritten, {'constant-offset': 1, **changes}
    if variant not in kVariants['blockcalls'] + kResidualVariants['blockcalls']:
        raise ValueError('unknown aggregate helper variant')
    offsets = variant in ('inline-checked-offset', 'inline-offset-and-aggregate', 'inline-offset-and-copy')
    zero = variant in ('omit-aggregate-zero', 'inline-aggregate', 'inline-offset-and-aggregate')
    copy = variant in ('inline-small-copy', 'inline-aggregate', 'inline-offset-and-aggregate', 'inline-offset-and-copy')
    buffers = set(re.findall(r'^    (%[\w.]+) =l alloc8 16$', source, re.MULTILINE))
    changes = Counter()
    lines = []
    for line in source.splitlines(keepends=True):
        match = kOffsetCall.fullmatch(line)
        if offsets and match:
            result, index, first, last, stride = match.groups()
            low, high, size = map(int, (first, last, stride))
            if not -(1 << 63) <= low <= high < (1 << 63) or not 0 <= size < (1 << 63) or (high-low)*size >= (1 << 63):
                raise ValueError('offset probe requires bounded nonpacked nonoverflowing constants')
            tag = '.helper.offset.' + str(sum(changes.values()))
            lines.extend([
                f'    %{tag}.low =w csgel {index}, {low}\n',
                f'    %{tag}.high =w cslel {index}, {high}\n',
                f'    %{tag}.valid =w and %{tag}.low, %{tag}.high\n',
                f'    jnz %{tag}.valid, @{tag}.ok, @{tag}.bad\n',
                f'@{tag}.bad\n', '    call $__ada_raise(l $__ada_exc_constraint_error)\n',
                f'    jmp @{tag}.done\n', f'@{tag}.ok\n',
                f'    %{tag}.distance =l sub {index}, {low}\n',
                f'    %{tag}.bytes =l mul %{tag}.distance, {size}\n',
                f'    jmp @{tag}.done\n', f'@{tag}.done\n',
                f'    {result} =l phi @{tag}.bad 0, @{tag}.ok %{tag}.bytes\n'])
            changes['checked-offset'] += 1
            continue
        match = re.fullmatch(r'    call \$memset\(l (%[\w.]+), w 0, l 16\)\n', line)
        if (zero or variant == 'inline-aggregate-zero') and match:
            if match[1] not in buffers:
                raise ValueError('zero probe requires the fixed 16-byte stack scratch')
            if variant == 'inline-aggregate-zero':
                tag = '.helper.zero.' + str(sum(changes.values()))
                lines.extend([f'    storel 0, {match[1]}\n',
                              f'    %{tag}.next =l add {match[1]}, 8\n',
                              f'    storel 0, %{tag}.next\n'])
                changes['inline-zero'] += 1
            else:
                # The pinned corpus writes all four scalar elements before copying.
                # This omission is not valid for arbitrary aggregates/defaults.
                changes['aggregate-zero'] += 1
            continue
        match = re.fullmatch(r'    call \$memmove\(l (%[\w.]+), l (%[\w.]+), l 16\)\n', line)
        if copy and match:
            dst, src = match.groups()
            if dst not in buffers or src not in buffers:
                raise ValueError('copy probe requires the fixed aligned stack buffers')
            tag = '.helper.copy.' + str(sum(changes.values()))
            # Read the complete source before either write, retaining overlap behavior.
            lines.extend([
                f'    %{tag}.src8 =l add {src}, 8\n',
                f'    %{tag}.low =l loadl {src}\n', f'    %{tag}.high =l loadl %{tag}.src8\n',
                f'    storel %{tag}.low, {dst}\n', f'    %{tag}.dst8 =l add {dst}, 8\n',
                f'    storel %{tag}.high, %{tag}.dst8\n'])
            changes['small-copy'] += 1
            continue
        lines.append(line)
    expected = {name: 1 for enabled, name in [(offsets, 'checked-offset'), (zero, 'aggregate-zero'), (copy, 'small-copy'), (variant == 'inline-aggregate-zero', 'inline-zero')] if enabled}
    if dict(changes) != expected:
        raise ValueError('fixed aggregate helper shape changed; review the diagnostic')
    return ''.join(lines), dict(changes)
