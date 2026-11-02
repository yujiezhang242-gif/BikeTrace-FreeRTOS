"""Verify original archive against SHA-256 manifest; no hardware access.
Working firmware is expected to differ after P1. This verifies rollback data.
"""
import hashlib
import json
import tarfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'docs/baseline_manifest.json').read_text())
archive=root/'baseline/BikeTrace_original.tar.gz'
expected=(root/'baseline/SHA256SUMS').read_text().split()[0]
assert hashlib.sha256(archive.read_bytes()).hexdigest()==expected,'Archive checksum mismatch'
with tarfile.open(archive) as t:
    for name,digest in manifest.items():
        f=t.extractfile('BikeTrace_original/'+name)
        assert f and hashlib.sha256(f.read()).hexdigest()==digest,name
print('PASS: rollback archive and %d original file hashes verified'%len(manifest))
