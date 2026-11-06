#!/usr/bin/env python3
"""Prove abnormal exits, deadlines and output overflow cannot pass the runner."""
import os
from pathlib import Path
import signal
import sys
import subprocess
import tempfile
import time

from process import run

if len(sys.argv) >= 2:
    mode = sys.argv[1]
    if mode == "sleep":
        time.sleep(20)
    elif mode == "crash":
        if os.name == "posix":
            os.kill(os.getpid(), signal.SIGKILL)
        else:
            os._exit(139)
    elif mode == "tree":
        subprocess.Popen([sys.executable, "-B", __file__, "descendant", sys.argv[2]])
        time.sleep(20)
    elif mode == "descendant":
        marker = Path(sys.argv[2])
        marker.with_suffix(".ready").write_text("ready")
        time.sleep(1.2)
        marker.write_text("survived")
    elif mode == "output":
        sys.stdout.write("x" * (1024 * 1024))
    elif mode == "reject":
        sys.stderr.write("p.adb:1:1: error: expected declaration\n")
        sys.exit(1)
    sys.exit(0)

command = [sys.executable, "-B", str(Path(__file__).resolve())]
for mode, expected in [("normal", "EXIT"), ("reject", "EXIT"), ("sleep", "TIMEOUT"),
                       ("crash", "CRASH"), ("output", "OUTPUT_LIMIT")]:
    result = run(command + [mode], timeout=0.5 if mode == "sleep" else 3, outputLimit=65536)
    assert result["outcome"] == expected, (mode, result)
    if mode == "reject":
        assert result["exit"] == 1 and ": error:" in result["stderr"]
with tempfile.TemporaryDirectory() as directory:
    marker = Path(directory) / "descendant.txt"
    result = run(command + ["tree", str(marker)], timeout=0.5)
    assert result["outcome"] == "TIMEOUT", result
    assert marker.with_suffix(".ready").exists(), "descendant control did not start"
    time.sleep(1.3)
    assert not marker.exists(), "timed-out descendant survived process-group cleanup"
print("runner failure controls passed")
