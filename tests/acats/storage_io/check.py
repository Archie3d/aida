"""Pin unchanged Storage_IO ACATS sources; executable tests grade the cases."""

import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads(Path(__file__).with_name('manifest.json').read_text())
for case in manifest['cases']:
    data = (root / case['source']).read_bytes()
    assert hashlib.sha256(data).hexdigest() == case['sha256'], case['id']
    assert hashlib.sha1(f'blob {len(data)}\0'.encode() + data).hexdigest() == case['git_blob'], case['id']
    print(f"{case['id']}: original hashes verified; execution graded by acats.{case['id']}")
