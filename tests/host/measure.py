#!/usr/bin/env python3
"""Record alternating host timing samples; never turn timings into test grades."""

import argparse
from collections import defaultdict
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import statistics
import subprocess
import sys
import time
import textwrap

kSources = Path(__file__).resolve().parents[1] / 'benchmarks'
kWorkloads = ['contextcalls', 'imagecalls', 'blockcalls', 'allocationcalls']
kThroughputLiveObjects = 32


def longSource(source, repetitions):
    match = re.fullmatch(r'(.*?)procedure\s+(\w+)\s+is(.*)end\s+\2\s*;\s*',
                         source, flags=re.DOTALL | re.IGNORECASE)
    if match is None:
        raise ValueError('expected one benchmark procedure')
    context, name, body = match.groups()
    unit = name + 'Long'
    kernel = 'procedure ' + name + ' is' + body + 'end ' + name + ';\n'
    return unit.lower() + '.adb', (context + 'procedure ' + unit + ' is\n'
        + textwrap.indent(kernel, '    ') + 'begin\n'
        + '    for Trial in 1 .. ' + str(repetitions) + ' loop\n'
        + '        ' + name + ';\n    end loop;\nend ' + unit + ';\n')


def batchSource(template, liveObjects, rounds, order):
    positions = dict(fifo='I', lifo='kLiveObjects - I + 1',
                     permuted='((I - 1) * ' + str(liveObjects // 2 + 1) + ') mod kLiveObjects + 1')
    return (template.replace('@LIVE_OBJECTS@', str(liveObjects))
            .replace('@ROUNDS@', str(rounds)).replace('@POSITION@', positions[order]))


def prepareCases(work, suite, repetitions, batchSizes, batchObjects, throughputObjects):
    cases = []
    for workload in kWorkloads:
        source = kSources / (workload + '.adb')
        cases.append((workload, source, dict(kind='short', parameters=dict(repetitions=1))))
    if suite == 'short':
        return cases
    cases.append(('startup', kSources / 'startupprobe.adb', dict(kind='startup', parameters={})))
    for workload in kWorkloads:
        kernel = kSources / (workload + '.adb')
        filename, text = longSource(kernel.read_text(), repetitions)
        name = workload + '-long'
        directory = work / 'sources' / name
        directory.mkdir(parents=True, exist_ok=True)
        source = directory / filename
        source.write_bytes(text.encode('utf-8'))
        cases.append((name, source, dict(kind='long', parameters=dict(repetitions=repetitions),
                     kernel_sha256=hashlib.sha256(kernel.read_bytes()).hexdigest(), generated_source=text)))
    template = (kSources / 'allocationbatch.adb.in').read_text()
    batches = [(size, batchObjects, 'scaling') for size in batchSizes]
    batches.append((kThroughputLiveObjects, throughputObjects, 'throughput'))
    for liveObjects, objectCount, series in batches:
        for order in ('fifo', 'lifo', 'permuted'):
            name = 'allocationbatch-' + ('throughput-' if series == 'throughput' else '') + order + '-' + str(liveObjects)
            directory = work / 'sources' / name
            directory.mkdir(parents=True, exist_ok=True)
            source = directory / 'allocationbatch.adb'
            rounds = objectCount // liveObjects
            text = batchSource(template, liveObjects, rounds, order)
            source.write_bytes(text.encode('utf-8'))
            cases.append((name, source, dict(kind='batch', parameters=dict(
                live_objects=liveObjects, rounds=rounds, allocations=rounds * liveObjects,
                order=order, series=series, heap_pointer_table=True), generated_source=text,
                template_sha256=hashlib.sha256((kSources / 'allocationbatch.adb.in').read_bytes()).hexdigest())))
    return cases


def hostInformation():
    result = dict(system=platform.system(), machine=platform.machine(),
                  release=platform.release(), python=platform.python_version(),
                  processor=platform.processor(), logical_cpus=os.cpu_count())
    try:
        if hasattr(os, 'sched_getaffinity'):
            result['available_cpus'] = len(os.sched_getaffinity(0))
        cpuinfo = Path('/proc/cpuinfo')
        if cpuinfo.is_file():
            result['cpu_models'] = sorted({line.strip() for line in cpuinfo.read_text().splitlines()
                if line.split(':', 1)[0].strip() in ('model name', 'CPU implementer', 'CPU part')})
    except OSError:
        pass
    return result


def buildConfiguration(build):
    cache = {}
    for line in (build / 'CMakeCache.txt').read_text().splitlines():
        if not line or line.startswith(('#', '//')) or '=' not in line:
            continue
        key, value = line.split('=', 1)
        name = key.split(':', 1)[0]
        if name == 'CMAKE_BUILD_TYPE' or name.startswith(('CMAKE_C_FLAGS', 'CMAKE_CXX_FLAGS')):
            if not name.endswith('-ADVANCED'):
                cache[name] = value
        elif name in ('CMAKE_C_COMPILER', 'CMAKE_CXX_COMPILER'):
            version = subprocess.run([value, '--version'], capture_output=True,
                                     text=True, check=True, timeout=30)
            cache[name] = dict(path=value, version=version.stdout.strip())
    return cache


def command(args, work, environment):
    start = time.perf_counter()
    result = subprocess.run(list(map(str, args)), cwd=work, env=environment,
                            capture_output=True, text=True, timeout=60)
    elapsed = time.perf_counter() - start
    if result.returncode != 0 or result.stderr:
        raise RuntimeError(f'benchmark failed: {args}\n{result.returncode}\n{result.stdout}{result.stderr}')
    return result.stdout, elapsed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ada', type=Path, required=True)
    parser.add_argument('--baseline-ada', type=Path)
    parser.add_argument('--context-probe', type=Path)
    parser.add_argument('--build', type=Path)
    parser.add_argument('--baseline-build', type=Path)
    parser.add_argument('--revision', help='revision used to build the current compiler')
    parser.add_argument('--baseline-revision', help='revision used to build the baseline compiler')
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--runs', type=int, default=11)
    parser.add_argument('--suite', choices=('short', 'extended'), default='short',
                        help='original kernels, or kernels plus long/scaling/throughput controls')
    parser.add_argument('--repetitions', type=int, default=128)
    parser.add_argument('--batch-sizes', type=int, nargs='+', default=[32, 512, 4096])
    parser.add_argument('--batch-objects', type=int, default=65536)
    parser.add_argument('--throughput-objects', type=int, default=4194304)
    args = parser.parse_args()
    for name in ('revision', 'baseline_revision'):
        value = getattr(args, name)
        if value is not None and not value.strip():
            parser.error('--' + name.replace('_', '-') + ' must not be empty')
    if args.baseline_build and not args.baseline_ada:
        parser.error('--baseline-build requires --baseline-ada')
    if args.runs < 3:
        parser.error('at least three alternating runs are required')
    if not 1 <= args.repetitions <= 1024:
        parser.error('--repetitions must be between 1 and 1024')
    if len(set(args.batch_sizes)) != len(args.batch_sizes) or any(
            size < 4 or size > 8192 or size & (size - 1) for size in args.batch_sizes):
        parser.error('--batch-sizes must be distinct powers of two between 4 and 8192')
    if not 1 <= args.batch_objects <= 1048576 or any(
            args.batch_objects % size or args.batch_objects < size for size in args.batch_sizes):
        parser.error('--batch-objects must be at most 1048576 and a positive multiple of each batch size')
    if not kThroughputLiveObjects <= args.throughput_objects <= 8388608 or args.throughput_objects % kThroughputLiveObjects:
        parser.error('--throughput-objects must be a multiple of 32 between 32 and 8388608')
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    for key in ['ADAC', 'QBE', 'CC', 'ADA_LIBRARY', 'ADA_RUNTIME', 'ADA_INCLUDE_PATH']:
        environment.pop(key, None)
    compilers = {'current': args.ada.resolve(strict=True)}
    if args.baseline_ada:
        compilers['baseline'] = args.baseline_ada.resolve(strict=True)
    suffix = '.exe' if os.name == 'nt' else ''
    results = {}
    for workload, source, metadata in prepareCases(work, args.suite, args.repetitions,
                                                 args.batch_sizes, args.batch_objects, args.throughput_objects):
        programs = {}
        for label, compiler in compilers.items():
            program = work / (workload + '-' + label + suffix)
            command([compiler, '-o', program, source], work, environment)
            output, _ = command([program], work, environment)
            if output:
                raise RuntimeError('benchmark must validate silently: ' + workload)
            programs[label] = program
        samples = defaultdict(list)
        for trial in range(args.runs):
            order = list(programs)
            if trial % 2:
                order.reverse()
            for label in order:
                output, elapsed = command([programs[label]], work, environment)
                if output:
                    raise RuntimeError('unexpected benchmark output')
                samples[label].append(elapsed)
        entry = dict(metadata, source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                 source_file=str(source),
                 executable_sha256={label: hashlib.sha256(program.read_bytes()).hexdigest()
                                    for label, program in programs.items()},
                 samples_seconds=dict(samples),
                 median_seconds={key: statistics.median(value) for key, value in samples.items()})
        entry['median_absolute_deviation_seconds'] = {
            label: statistics.median(abs(value - entry['median_seconds'][label]) for value in values)
            for label, values in samples.items()}
        entry['execution_order'] = [list(programs) if trial % 2 == 0 else list(reversed(programs))
                                    for trial in range(args.runs)]
        if 'baseline' in samples:
            entry['current_over_baseline'] = entry['median_seconds']['current'] / entry['median_seconds']['baseline']
            entry['paired_sample_ratios'] = [current / baseline for current, baseline
                                             in zip(samples['current'], samples['baseline'])]
        results[workload] = entry
        print('Measured ' + workload + ': ' + ', '.join(
            label + '=' + format(value, '.6f') + 's' for label, value in entry['median_seconds'].items()),
            file=sys.stderr, flush=True)
    probes = defaultdict(list)
    if args.context_probe:
        probe = args.context_probe.resolve(strict=True)
        for trial in range(args.runs):
            order = [0, 1, 2] if trial % 2 == 0 else [2, 1, 0]
            for mode in order:
                output, _ = command([probe, mode], work, environment)
                probes[str(mode)].append(float(output))
    builds = {'current': args.build, 'baseline': args.baseline_build}
    components = {}
    configurations = {}
    for label, build in builds.items():
        if build is None:
            continue
        components[label] = {}
        configurations[label] = buildConfiguration(build.resolve())
        for component in ['adac/adac' + suffix, 'runtime/libadart.a', 'qbe/qbe' + suffix]:
            path = build.resolve() / component
            components[label][component] = hashlib.sha256(path.read_bytes()).hexdigest()
    comparison = 'current-only; no no-slowdown conclusion'
    if args.baseline_ada:
        comparison = 'paired'
        if compilers['current'] == compilers['baseline']:
            comparison = 'identical-driver harness control; not a historical baseline'
    report = dict(schema=3, recorded_at=datetime.now(timezone.utc).isoformat(),
                  suite=args.suite, process_startup_included=True, startup_control_subtracted=False,
                  warmup_runs_per_program=1,
                  collector_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  revisions=dict(current=args.revision, baseline=args.baseline_revision),
                  build_configuration=configurations, build_components_sha256=components,
                  context_probe_sha256=hashlib.sha256(args.context_probe.read_bytes()).hexdigest() if args.context_probe else None,
                  host=hostInformation(),
                  driver_sha256={label: hashlib.sha256(path.read_bytes()).hexdigest()
                                 for label, path in compilers.items()},
                  comparison=comparison,
                  runs=args.runs, workloads=results,
                  context_probe_samples_seconds=dict(probes),
                  context_probe_median_seconds={key: statistics.median(value) for key, value in probes.items()})
    (work / 'timings.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
