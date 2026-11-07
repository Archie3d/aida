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
import statistics
import subprocess
import time

kSources = Path(__file__).resolve().parents[1] / 'benchmarks'
kWorkloads = ['contextcalls', 'imagecalls', 'blockcalls', 'allocationcalls']


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
    args = parser.parse_args()
    if args.baseline_build and not args.baseline_ada:
        parser.error('--baseline-build requires --baseline-ada')
    if args.runs < 3:
        parser.error('at least three alternating runs are required')
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
    for workload in kWorkloads:
        programs = {}
        source = kSources / (workload + '.adb')
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
        entry = {'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                 'samples_seconds': dict(samples),
                 'median_seconds': {key: statistics.median(value) for key, value in samples.items()}}
        if 'baseline' in samples:
            entry['current_over_baseline'] = entry['median_seconds']['current'] / entry['median_seconds']['baseline']
        results[workload] = entry
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
    report = dict(schema=2, recorded_at=datetime.now(timezone.utc).isoformat(),
                  revisions=dict(current=args.revision, baseline=args.baseline_revision),
                  build_configuration=configurations, build_components_sha256=components,
                  context_probe_sha256=hashlib.sha256(args.context_probe.read_bytes()).hexdigest() if args.context_probe else None, host=dict(system=platform.system(), machine=platform.machine(),
                                    release=platform.release(), python=platform.python_version()),
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
