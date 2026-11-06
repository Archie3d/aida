"""Verify original pool sources and the remaining positive-Storage_Size gap."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--adac', type=Path, required=True)
parser.add_argument('--work', type=Path, required=True)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
manifest = json.loads(Path(__file__).with_name('manifest.json').read_text())
for source in manifest['files']:
    data = (root / source['source']).read_bytes()
    assert hashlib.sha256(data).hexdigest() == source['sha256'], source['source']
    assert hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest() == source['git_blob'], source['source']
args.work.mkdir(parents=True, exist_ok=True)
for case in manifest['cases']:
    if case['outcome'] == 'PASS':
        print(f"{case['id']}: original hashes verified; execution graded by acats.{case['id']}")
        continue
    sources = [root / 'support/report.ads', root / 'support/report.adb']
    sources += [root / path for path in case['support']]
    sources.append(root / 'upstream' / (case['id'] + '.a'))
    result = subprocess.run([str(args.adac.resolve()), '-o', str(args.work / (case['id'] + '.ssa')),
                             *map(str, sources)], capture_output=True, text=True, timeout=45)
    errors = re.sub(r'^.*' + case['id'] + r'\.a(?=:\d+:\d+:)', case['id'] + '.a', result.stderr, flags=re.M)
    assert result.returncode == 1 and not result.stdout and errors == case['expected'], (case['id'], result.returncode, errors)
    print(f"{case['id']}: COMPILE_REJECTED; positive Storage_Size still unsupported; not a conformance pass")
