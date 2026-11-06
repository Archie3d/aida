"""QBE command-line adapter for validation builds (the backend stays unchanged)."""
from pathlib import Path
import subprocess
import sys
import tempfile

from tsan_ir import instrument


def main(backend, arguments):
    if not arguments or arguments[-1].startswith("-"):
        raise ValueError("instrumentation requires a named input IR file")
    source = Path(arguments[-1])
    text, _ = instrument(source.read_text())
    with tempfile.TemporaryDirectory(prefix="aida-tsan-") as directory:
        annotated = Path(directory) / "annotated.ssa"
        annotated.write_text(text)
        result = subprocess.run([backend, *arguments[:-1], str(annotated)], timeout=60)
        return result.returncode
