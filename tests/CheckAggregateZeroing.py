"""Check exact initialization coverage independently of aggregate value stores."""
import argparse
import os
from pathlib import Path
import re
import subprocess


def copyControls(body, size, source):
    region = re.search(r'^@aggregatedone\.\d+\n(.*?)    call \$__ada_master_activate', body, re.M | re.S)[1]
    addresses = {source: 0}
    loads = {}
    stores = []
    instructions = []
    destination = None
    for position, line in enumerate(region.splitlines()):
        add = re.fullmatch(r'    (%[\w.]+) =l add (%[\w.]+), (\d+)', line)
        load = re.fullmatch(r'    (%[\w.]+) =[wl] load(l|w|uh|ub) (%[\w.]+)', line)
        store = re.fullmatch(r'    store(l|w|h|b) (%[\w.]+), (%[\w.]+)', line)
        if add and add[2] in addresses:
            addresses[add[1]] = addresses[add[2]] + int(add[3])
        elif load and load[3] in addresses:
            loads[load[1]] = ({'l': 8, 'w': 4, 'uh': 2, 'ub': 1}[load[2]], addresses[load[3]], position)
        elif store and store[2] in loads:
            if destination is None:
                destination = store[3]
                addresses[destination] = 0
            width = {'l': 8, 'w': 4, 'h': 2, 'b': 1}[store[1]]
            assert width == loads[store[2]][0]
            stores.append((width, addresses[store[3]], position))
        else:
            continue
        instructions.append(line)
    assert destination is not None
    assert max(value[2] for value in loads.values()) < min(value[2] for value in stores)
    for chunks in (loads.values(), stores):
        assert sorted(byte for width, offset, _ in chunks for byte in range(offset, offset + width)) == list(range(size))
    # Reuse actual emitted instructions with disjoint, equal and overlapping
    # buffers. Check every byte, including both surrounding canaries.
    controls = []
    for number, delta in enumerate((0, 8, -8, ((size + 7) // 8 + 1) * 8)):
        name = f'copy{size}_{number}'
        length = 2 * size + 48
        start, target = 24, 24 + delta
        expected = [(i * 13 + 41) % 256 for i in range(length)]
        initial = list(expected)
        expected[target:target + size] = initial[start:start + size]
        lines = [f'function w ${name}() {{', '@start', f'    %raw =l alloc8 {length}',
                 f'    {source} =l add %raw, {start}', f'    {destination} =l add %raw, {target}',
                 '    %bad0 =w copy 0']
        for offset, value in enumerate(initial):
            lines.extend([f'    %p{offset} =l add %raw, {offset}', f'    storeb {value}, %p{offset}'])
        lines.extend(instructions)
        for offset, value in enumerate(expected):
            lines.extend([f'    %v{offset} =w loadub %p{offset}', f'    %wrong{offset} =w cnew %v{offset}, {value}',
                          f'    %bad{offset + 1} =w or %bad{offset}, %wrong{offset}'])
        lines.extend([f'    ret %bad{length}', '}'])
        controls.append((name, '\n'.join(lines)))
    return controls


def check(ir):
    functions = dict(re.findall(r'^function \$aggregatezeroing__(\w+)\([^\n]*\) \{\n(.*?)^\}', ir, re.M | re.S))
    machineControl = ['export function w $main() {', '@start', '    %wrong.0 =w copy 0']
    checked = 0
    copies = []
    for size in (0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32):
        body = functions['check' + str(size)]
        assert 'call $memset' not in body and 'call $__ada_array_local' not in body, size
        # Examine initialization before any aggregate component is evaluated.
        region = re.search(r'^@aggregateboundsready\.\d+\n(.*?)    jmp @aggregatefill', body, re.M | re.S)[1]
        assert re.match(r'    storel \$[^,]+, %\.traceLocation\n', region), size
        stores = re.findall(r'^    store([bwhl]) 0, (%[\w.]+)$', region, re.M)
        # The final zero initializes the separate loop counter, not scratch.
        counter = stores.pop()[1]
        assert re.search(r'=l loadl ' + re.escape(counter) + r'\n', body), size
        base = stores[0][1]
        allocation = re.search(re.escape(base) + r' =l alloc([48]) (\d+)\n', body)
        assert int(allocation[2]) == max(1, size), size
        alignment = int(allocation[1])
        addresses = {base: 0}
        for target, origin, offset in re.findall(r'(%[\w.]+) =l add (%[\w.]+), (\d+)\n', region):
            assert origin == base, size
            addresses[target] = int(offset)
        covered = []
        for kind, address in stores:
            width = {'b': 1, 'h': 2, 'w': 4, 'l': 8}[kind]
            offset = addresses[address]
            assert alignment >= width and offset % width == 0, (size, width, offset)
            covered.extend(range(offset, offset + width))
        assert sorted(covered) == list(range(max(1, size))), (size, covered)
        if size > 0:
            assert 'call $memmove' not in body, size
            copies.extend(copyControls(body, size, base))
        # Execute the actual emitted zero instructions on dirty memory, with
        # canaries on both sides. Aggregate assignments otherwise overwrite it.
        prefix = '%control.' + str(size)
        count = max(1, size)
        machineControl.append(f'    {prefix}.raw =l alloc8 {count + 16}')
        machineControl.append(f'    {base} =l add {prefix}.raw, 8')
        for offset in range(count + 16):
            machineControl.extend([f'    {prefix}.p{offset} =l add {prefix}.raw, {offset}',
                                   f'    storeb 255, {prefix}.p{offset}'])
        initialization = region.splitlines()[1:-1] # Exclude trace publication and loop counter.
        machineControl.extend(initialization)
        for offset in range(count + 16):
            expected = 0 if 8 <= offset < 8 + count else 255
            machineControl.extend([f'    {prefix}.v{offset} =w loadub {prefix}.p{offset}',
                                   f'    {prefix}.bad{offset} =w cnew {prefix}.v{offset}, {expected}',
                                   f'    %wrong.{checked + 1} =w or %wrong.{checked}, {prefix}.bad{offset}'])
            checked += 1
    assert re.search(r'call \$memset\(l %[^,]+, w 0, l 33\)', functions['check33'])
    assert 'call $memmove' in functions['check33']
    for name in ('check65537', 'dynamic'):
        assert 'call $__ada_array_local' in functions[name], name
    for name, _ in copies:
        machineControl.extend([f'    %copy{checked} =w call ${name}()',
                               f'    %wrong.{checked + 1} =w or %wrong.{checked}, %copy{checked}'])
        checked += 1
    machineControl.extend([f'    ret %wrong.{checked}', '}'])
    machineControl.extend(source for _, source in copies)
    return '\n'.join(machineControl) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--adac', type=Path, required=True)
    parser.add_argument('--qbe', type=Path, required=True)
    parser.add_argument('--cc', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(args.adac), '-D', str(args.work), str(args.source)], check=True, timeout=25)
    control = args.work / 'zero-control.ssa'
    control.write_text(check((args.work / 'aggregatezeroing.ssa').read_text()))
    assembly = control.with_suffix('.s')
    program = args.work / ('zero-control.exe' if os.name == 'nt' else 'zero-control')
    target = ['-t', 'amd64_win'] if os.name == 'nt' else []
    subprocess.run([str(args.qbe), *target, '-o', str(assembly), str(control)], check=True, timeout=10)
    subprocess.run([str(args.cc), str(assembly), '-o', str(program)], check=True, timeout=10)
    result = subprocess.run([str(program.resolve())], capture_output=True, check=True, timeout=10)
    assert not result.stdout and not result.stderr


if __name__ == '__main__':
    main()
