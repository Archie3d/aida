"""Pin the unchanged text-stream case and its separately tracked equality failure."""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--ada', required=True, type=Path)
parser.add_argument('--work', required=True, type=Path)
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
manifest = json.loads(Path(__file__).with_name('manifest.json').read_text())
args.work.mkdir(parents=True, exist_ok=True)
for case in manifest['cases']:
    data = (root / case['source']).read_bytes()
    assert hashlib.sha256(data).hexdigest() == case['sha256']
    assert hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest() == case['git_blob']
    program = args.work.resolve() / case['id']
    sources = [root / path for path in case['support']] + [root / case['source']]
    command = [str(args.ada.resolve()), '-o', str(program)]
    for source in sources:
        command += ['--source', str(source)]
    built = subprocess.run(command, cwd=args.work, capture_output=True, text=True, timeout=60)
    assert built.returncode == 0 and not built.stdout and not built.stderr, built
    result = subprocess.run([str(program)], cwd=args.work, capture_output=True, text=True, timeout=20)
    assert result.returncode == 1 and result.stdout == case['expected_stdout'] and result.stderr == case['expected_stderr'], result
    print('CXAC004: original bytes verified; known composite-equality RUN_FAILED, not a conformance pass')
