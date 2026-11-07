"""Pin random-number ACATS sources and preserve the remaining tasking blocker."""

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
for case in manifest['cases']:
    data = (root / case['source']).read_bytes()
    assert hashlib.sha256(data).hexdigest() == case['sha256'], case['id']
    assert hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest() == case['git_blob'], case['id']
    if case['outcome'] == 'PASS':
        print(f"{case['id']}: original hashes verified; execution graded by acats.{case['id']}")
        continue
    sources = [root / path for path in case['support']] + [root / case['source']]
    result = subprocess.run([str(args.adac.resolve()), '-o', str(args.work / (case['id'] + '.ssa')),
                             *map(str, sources)], capture_output=True, text=True, timeout=45)
    errors = re.sub(r'^.*' + case['id'] + r'\.a(?=:\d+:\d+:)', case['id'] + '.a', result.stderr, flags=re.M)
    assert result.returncode == 1 and not result.stdout and errors == case['expected'], (case['id'], result.returncode, errors)
    print(f"{case['id']}: select syntax rejected before coverage scenario; not a conformance pass")
