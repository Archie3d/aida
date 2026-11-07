"""Pin the unchanged Text_Streams case; the executable C-test grades its result."""

import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads(Path(__file__).with_name('manifest.json').read_text())
for case in manifest['cases']:
    data = (root / case['source']).read_bytes()
    assert hashlib.sha256(data).hexdigest() == case['sha256']
    assert hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest() == case['git_blob']
    assert case['outcome'] == 'PASS'
    assert case['expected_stdout'] == 'TEST CXAC004\nPASSED\n' and not case['expected_stderr']
    print('CXAC004: original bytes verified; complete REPORT result graded by acats.cxac004')
