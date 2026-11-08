#!/usr/bin/env python3
"""Benchmark-only call census and matched ablations, never a production optimization.

The census is untimed. Master omission changes general Ada semantics and is
restricted to generated copies of the two fixed, entry-free benchmark kernels.
Empty cleanup guards retain real cleanup when owner differs from checkpoint.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import statistics
import subprocess
import sys

import measure

kRoot = Path(__file__).resolve().parents[2]
kVariants = ('control', 'control-copy', 'no-lexical-masters', 'guard-finalization',
             'guard-rewind', 'guard-cleanup', 'masters-and-cleanup')
kMasterHooks = {'__ada_master_enter', '__ada_master_activate', '__ada_master_await', '__ada_master_leave'}
kEntries = {
    'adart': {
        '__ada_task_context': '0',
        '__ada_trace_enter': '0',
        '__ada_trace_leave_context': '0',
        '__ada_finalize_to': '*owner == checkpoint',
        '__ada_array_rewind': '*owner == checkpoint',
        '__ada_array_offset': '0',
        '__ada_collection_allocate_aligned': '0',
        '__ada_collection_activate': '0',
        '__ada_deallocate': '0',
    },
    'adamaster': {
        '__ada_master_enter': '0',
        '__ada_master_activate': 'master == NULL || atomic_load(&master->activities) == 0',
        '__ada_master_await': 'master == NULL || atomic_load(&master->activities) == 0',
        '__ada_master_leave': 'master == NULL || atomic_load(&master->activities) == 0',
        '__ada_task_discard': 'master == NULL || atomic_load(&master->activities) == 0',
    },
    'adalock': {'__ada_mutex_lock': '0', '__ada_mutex_unlock': '0'},
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, environment):
    result = subprocess.run(list(map(str, command)), env=environment, capture_output=True,
                            text=True, timeout=90)
    if result.returncode != 0:
        raise RuntimeError(str(command) + '\n' + result.stdout + result.stderr)
    return result


def instrument(source, entries):
    for name, condition in entries.items():
        pattern = r'(^[^;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\n\{\n)'
        source, count = re.subn(pattern, lambda m: m[0] +
            '    __ada_profile_count("' + name + '", ' + condition + ');\n', source, flags=re.MULTILINE)
        if count != 1:
            raise ValueError('expected one definition of ' + name)
    return 'extern void __ada_profile_count(const char*, int);\n' + source


def transform(source, variant):
    if variant not in kVariants:
        raise ValueError('unknown benchmark variant')
    changes = Counter()
    lines = []
    for line in source.splitlines(keepends=True):
        match = re.fullmatch(r'    call \$(__ada_\w+)\((.*)\)\n', line)
        if match:
            name, args = match.groups()
            if variant in ('no-lexical-masters', 'masters-and-cleanup') and name in kMasterHooks:
                if name == '__ada_master_enter' and not re.search(r', w [01]$', args):
                    raise ValueError('cannot omit collection/unknown master')
                changes[name] += 1
                continue
            guarded = ((name == '__ada_finalize_to' and variant in
                        ('guard-finalization', 'guard-cleanup', 'masters-and-cleanup')) or
                       (name == '__ada_array_rewind' and variant in
                        ('guard-rewind', 'guard-cleanup', 'masters-and-cleanup')))
            if guarded:
                operands = re.fullmatch(r'l ([^,]+), l ([^,]+)', args)
                if operands is None:
                    raise ValueError('unexpected cleanup signature')
                owner, checkpoint = operands.groups()
                index = sum(changes.values())
                changes[name] += 1
                tag = '.profile.' + str(index)
                lines.extend([
                    '    %' + tag + '.head =l loadl ' + owner + '\n',
                    '    %' + tag + '.empty =w ceql %' + tag + '.head, ' + checkpoint + '\n',
                    '    jnz %' + tag + '.empty, @' + tag + '.done, @' + tag + '.cleanup\n',
                    '@' + tag + '.cleanup\n', line,
                    '    jmp @' + tag + '.done\n', '@' + tag + '.done\n'])
                continue
        lines.append(line)
    return ''.join(lines), dict(changes)


def cacheValues(build):
    values = {}
    for line in (build / 'CMakeCache.txt').read_text().splitlines():
        if line and not line.startswith(('#', '//')) and '=' in line:
            key, value = line.split('=', 1)
            values[key.split(':', 1)[0]] = value
    return values


def validateWork(build, work):
    if work == build or work == kRoot:
        raise ValueError('use a separate diagnostic work directory')
    if work.is_relative_to(kRoot) and not work.is_relative_to(build):
        raise ValueError('diagnostics cannot overwrite source directories')
    if work.is_relative_to(build) and work.relative_to(build).parts[0] in ('runtime', 'tests', 'ada', 'adac', 'qbe'):
        raise ValueError('diagnostics cannot overwrite build products')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--runs', type=int, default=11)
    parser.add_argument('--repetitions', type=int, default=128)
    args = parser.parse_args()
    if args.runs < 3 or args.runs % 2 != 1 or not 1 <= args.repetitions <= 1024:
        parser.error('use an odd run count >= 3 and 1..1024 repetitions')
    if os.name == 'nt':
        parser.error('this diagnostic supports POSIX hosts; Windows assembly requires driver stack-probe preparation')
    build = args.build.resolve(strict=True)
    work = args.work.resolve()
    try:
        validateWork(build, work)
    except ValueError as error:
        parser.error(str(error))
    work.mkdir(parents=True, exist_ok=True)
    cache = cacheValues(build)
    if cache['CMAKE_BUILD_TYPE'] != 'Release':
        parser.error('a matching Release build is required')
    cc, ar = cache['CMAKE_C_COMPILER'], cache['CMAKE_AR']
    flags = shlex.split(cache.get('CMAKE_C_FLAGS', '')) + shlex.split(cache['CMAKE_C_FLAGS_RELEASE'])
    if any('sanitize' in flag for flag in flags):
        parser.error('do not mix sanitizer builds and profiling')
    suffix = '.exe' if os.name == 'nt' else ''
    ada, qbe = build / ('ada/ada' + suffix), build / ('qbe/qbe' + suffix)
    environment = os.environ.copy()
    for key in ('ADAC', 'QBE', 'CC', 'ADA_LIBRARY', 'ADA_RUNTIME', 'ADA_INCLUDE_PATH'):
        environment.pop(key, None)
    runtime = build / 'runtime/libadart.a'
    links = ['-pthread'] + (['-lm'] if sys.platform.startswith('linux') else [])
    archive = work / 'census-runtime.a'
    shutil.copyfile(runtime, archive)
    members = run([ar, 't', archive], environment).stdout.splitlines()
    instrumented = {}
    for unit, entries in kEntries.items():
        member = [name for name in members if name in (unit + '.c.o', unit + '.c.obj')]
        if len(member) != 1:
            raise ValueError('unexpected archive member for ' + unit)
        source = work / (unit + '.c')
        source.write_text(instrument((kRoot / 'runtime' / (unit + '.c')).read_text(), entries))
        obj = work / member[0]
        run([cc, *flags, '-std=c11', '-I', kRoot / 'runtime', '-c', source, '-o', obj], environment)
        run([ar, 'r', archive, obj], environment)
        instrumented[unit] = dict(source_sha256=sha(source), object_sha256=sha(obj))
    counters = work / 'counters.o'
    run([cc, *flags, '-std=c11', '-c', Path(__file__).with_name('ProfileCounters.c'), '-o', counters], environment)
    census = {}
    censusExecutables = {}
    workloads = {}
    for kernel in ('blockcalls', 'allocationcalls'):
        directory = work / kernel
        directory.mkdir(exist_ok=True)
        filename, text = measure.longSource((measure.kSources / (kernel + '.adb')).read_text(), args.repetitions)
        source = directory / filename
        source.write_bytes(text.encode())
        control = directory / ('control' + suffix)
        run([ada, '-o', control, source], environment)
        objects = Path(str(control) + '.adaobj')
        mainObject = objects / (source.stem + '.o')
        original = mainObject.with_suffix('.ssa').read_text()
        otherObjects = sorted(p for p in objects.glob('*.o') if p != mainObject)
        programs = {'control': control}
        metadata = {'control': dict(changes={}, ir_sha256=sha(mainObject.with_suffix('.ssa')),
                                    object_sha256=sha(mainObject))}
        for variant in kVariants[1:]:
            rewritten, changes = transform(original, variant)
            ir = directory / (variant + '.ssa')
            ir.write_bytes(rewritten.encode())
            assembly, obj = ir.with_suffix('.s'), ir.with_suffix('.o')
            run([qbe, '-o', assembly, ir], environment)
            run([cc, '-c', assembly, '-o', obj], environment)
            program = directory / (variant + suffix)
            run([cc, '-o', program, *otherObjects, obj, runtime, *links], environment)
            programs[variant] = program
            metadata[variant] = dict(changes=changes, ir_sha256=sha(ir), object_sha256=sha(obj))
        # Identical QBE/object controls catch unintended transformations before timing.
        assert metadata['control']['object_sha256'] == metadata['control-copy']['object_sha256']
        for program in programs.values():
            result = run([program], environment)
            if result.stdout or result.stderr:
                raise RuntimeError('benchmark validation must be silent')
        countsProgram = directory / ('census' + suffix)
        run([cc, '-o', countsProgram, *otherObjects, mainObject, counters, archive, *links], environment)
        result = run([countsProgram], environment)
        if result.stderr:
            raise RuntimeError('noisy census: ' + result.stderr)
        census[kernel] = json.loads(result.stdout)
        censusExecutables[kernel] = sha(countsProgram)
        samples = {name: [] for name in programs}
        orders = []
        for trial in range(args.runs):
            # Rotate and reverse to distribute position and temporal drift.
            names = list(programs)
            offset = trial % len(names)
            order = names[offset:] + names[:offset]
            if trial % 2: order.reverse()
            orders.append(order)
            for variant in order:
                output, elapsed = measure.command([programs[variant]], work, environment)
                if output: raise RuntimeError('noisy benchmark')
                samples[variant].append(elapsed)
        medians = {name: statistics.median(values) for name, values in samples.items()}
        workloads[kernel] = dict(generated_source=text, source_sha256=sha(source),
            kernel_sha256=sha(measure.kSources / (kernel + '.adb')), repetitions=args.repetitions,
            variants=metadata, executable_sha256={name: sha(path) for name, path in programs.items()},
            samples_seconds=samples, median_seconds=medians, execution_order=orders,
            median_absolute_deviation_seconds={name: statistics.median(abs(x-medians[name]) for x in values)
                                               for name, values in samples.items()},
            current_over_control={name: median/medians['control'] for name, median in medians.items()},
            paired_sample_ratios={name: [x/y for x,y in zip(values,samples['control'])]
                                  for name,values in samples.items()})
        print(kernel + ': ' + str(workloads[kernel]['current_over_control']), file=sys.stderr, flush=True)
    report = dict(schema=1, recorded_at=datetime.now(timezone.utc).isoformat(),
        description='Diagnostic-only generated QBE ablations; not production builds or correctness/performance claims for arbitrary Ada.',
        limits=['POSIX hosts only; Windows uses the existing unmodified host suite.',
                'No sampling percentages: census counters are untimed, single-thread and perturb execution.',
                'Master omission is benchmark-only, not a generally valid compiler optimization.',
                'Cleanup guards retain nonempty calls and exception checks; no exception branches are removed.',
                'Variant differences include code-generation/layout effects, not isolated helper timings.'],
        source_base_revision=run(['git', '-C', kRoot, 'rev-parse', 'HEAD'], environment).stdout.strip(),
        census_predicates=kEntries, census_executable_sha256=censusExecutables, census_runtime_sha256=sha(archive),
        census_untimed=True, census_scope='Selected runtime function entries, including runtime-internal calls; empty means the recorded owner/checkpoint or zero-activity predicate.',
        build_configuration=measure.buildConfiguration(build), host=measure.hostInformation(), runs=args.runs,
        process_startup_included=True, startup_subtracted=False,
        source_sha256={str(p.relative_to(kRoot)):sha(p) for p in [Path(__file__),Path(__file__).with_name('ProfileCounters.c'),Path(__file__).with_name('measure.py'),*[kRoot/'runtime'/f'{u}.c' for u in kEntries]]},
        build_components_sha256={str(p.relative_to(build)):sha(p) for p in [ada,build/('adac/adac'+suffix),qbe,runtime]},
        census_instrumentation=instrumented, census=census, workloads=workloads)
    (work/'profile.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__ == '__main__':
    main()
