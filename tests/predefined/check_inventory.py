"""Verify declared unit availability without calling missing units implemented."""

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import re
import subprocess
import sys


def checkUnit(unit, compiler, work):
    name = unit['name']
    directory = work / name.replace('.', '-')
    directory.mkdir(parents=True, exist_ok=True)
    source = directory / 'predefined_probe.adb'
    source.write_text(f'with {name};\nprocedure Predefined_Probe is\nbegin\n    null;\nend Predefined_Probe;\n')
    result = subprocess.run([str(compiler), '-o', str(directory / 'probe.ssa'), str(source)],
                            capture_output=True, text=True, timeout=45)
    if unit['status'] == 'resolves':
        if result.returncode != 0 or result.stderr:
            return f'{name}: declared present but failed: {result.returncode}\n{result.stderr}'
    else:
        expected = re.compile(r"^.+:1:\d+: error: cannot find the unit '" + re.escape(name) + r"'\n$", re.I)
        if result.returncode <= 0 or not expected.fullmatch(result.stderr):
            return f'{name}: declared missing but observation changed: {result.returncode}\n{result.stderr}'
    return None


def checkAPI(probe, compiler, work):
    directory = work / 'api' / probe['id']
    directory.mkdir(parents=True, exist_ok=True)
    source = directory / 'api_probe.adb'
    source.write_text(probe['source'])
    result = subprocess.run([str(compiler), '-o', str(directory / 'probe.ssa'), str(source)],
                            capture_output=True, text=True, timeout=45)
    errors = re.sub(r'^.*api_probe\.adb(?=:\d+:\d+:)', 'api_probe.adb', result.stderr, flags=re.M)
    if result.returncode != 1 or errors != probe['diagnostic']:
        return f"{probe['id']}: API gap observation changed: {result.returncode}\n{errors}"
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--adac', required=True, type=Path)
    parser.add_argument('--work', required=True, type=Path)
    parser.add_argument('--jobs', type=int, default=3)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    manifest = json.loads(Path(__file__).with_name('inventory.json').read_text())
    units = manifest['units']
    if manifest['schema'] != 1 or len({u['name'] for u in units}) != len(units):
        raise ValueError('invalid or duplicate unit metadata')
    actual = {p.relative_to(root).as_posix() for p in (root / 'runtime/ada').rglob('*.ads')}
    recorded = {u['source'] for u in units if u.get('source') != 'compiler builtin' and u['status'] == 'resolves'}
    if actual != recorded:
        raise ValueError(f'update inventory for source changes: {sorted(actual ^ recorded)}')
    for unit in units:
        if unit['status'] == 'resolves' and unit.get('source') != 'compiler builtin':
            expected = 'runtime/ada/' + unit['name'].lower().replace('.', '/') + '.ads'
            if unit['source'] != expected:
                raise ValueError(f'nonhierarchical runtime source: {unit}')
        if unit['status'] not in {'resolves', 'missing'}:
            raise ValueError(f'invalid unit status: {unit}')
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        failures = list(pool.map(lambda unit: checkUnit(unit, args.adac.resolve(), args.work.resolve()), units))
        failures += list(pool.map(lambda probe: checkAPI(probe, args.adac.resolve(), args.work.resolve()), manifest['api_gap_probes']))
    errors = [failure for failure in failures if failure]
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    present = sum(u['status'] == 'resolves' for u in units)
    print(f'Verified {present} resolving units and {len(units) - present} explicit missing-unit diagnostics; API completeness is separate.')
    print(f"Verified {len(manifest['api_gap_probes'])} recorded API-gap diagnostics; these are not conformance passes.")
    return 0


if __name__ == '__main__':
    sys.exit(main())
