"""Validate source hashes and remaining prerequisite observations."""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--adac', required=True, type=Path)
parser.add_argument('--work', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
manifest = json.loads(Path(__file__).with_name('manifest.json').read_text())
args.work.mkdir(parents=True, exist_ok=True)
for support in manifest.get('support_files', []):
    data = (root / support['source']).read_bytes()
    assert hashlib.sha256(data).hexdigest() == support['sha256'], support['source']
    assert hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest() == support['git_blob'], support['source']
for case in manifest['cases']:
    source = root / case['source']
    data = source.read_bytes()
    assert hashlib.sha256(data).hexdigest() == case['sha256'], source
    assert hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest() == case['git_blob'], source
    if case['outcome'] == 'PASS':
        print(f"{case['id']}: hashes verified; executable result checked by acats.{case['id']}")
        continue
    sources = [root / 'support/report.ads', root / 'support/report.adb', source]
    sources[2:2] = [root / path for path in case.get('support', [])]
    if case['id'] == 'cd30002':
        sources.insert(0, Path(__file__).with_name('impdef.ads'))
    result = subprocess.run([str(args.adac.resolve()), '-o', str(args.work / (case['id'] + '.ssa')),
                             *map(str, sources)], capture_output=True, text=True, timeout=45)
    errors = re.sub(r'^.*' + re.escape(source.name) + r'(?=:\d+:\d+:)', source.name, result.stderr, flags=re.M)
    assert result.returncode == 1 and not result.stdout and errors == case['expected'], (
        case['id'], result.returncode, result.stdout, errors)
    print(f"{case['id']}: {case['outcome']} unchanged; not a conformance pass")
