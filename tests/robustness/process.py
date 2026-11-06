"""Bound child processes, including descendants, time and diagnostic output."""
import os
import signal
import subprocess
import tempfile
import time


def run(command, timeout=10, outputLimit=1024 * 1024, cwd=None, env=None):
    started = time.monotonic()
    with tempfile.TemporaryFile() as out, tempfile.TemporaryFile() as err:
        child = subprocess.Popen([str(item) for item in command], stdout=out, stderr=err,
                                 cwd=cwd, env=env, start_new_session=os.name == "posix")
        outcome = "EXIT"
        while True:
            if os.fstat(out.fileno()).st_size + os.fstat(err.fileno()).st_size > outputLimit:
                outcome = "OUTPUT_LIMIT"
                break
            if time.monotonic() - started >= timeout:
                outcome = "TIMEOUT"
                break
            try:
                child.wait(timeout=0.02)
                break
            except subprocess.TimeoutExpired:
                pass
        if outcome != "EXIT" and child.poll() is None:
            try:
                if os.name == "posix":
                    os.killpg(child.pid, signal.SIGKILL)
                else:
                    subprocess.run(["taskkill", "/F", "/T", "/PID", str(child.pid)],
                                   capture_output=True, timeout=5)
            except ProcessLookupError:
                pass
            except PermissionError:
                # macOS can deny a signal to a group that disappeared between
                # poll and killpg. An actual live-child denial remains a failure.
                try:
                    child.wait(timeout=0.2)
                except subprocess.TimeoutExpired:
                    child.kill()
                    child.wait(timeout=5)
                    raise RuntimeError("host denied cleanup of a live child process group")
            child.kill()
        child.wait(timeout=5)
        if outcome == "EXIT":
            if os.fstat(out.fileno()).st_size + os.fstat(err.fileno()).st_size > outputLimit:
                outcome = "OUTPUT_LIMIT"
            elif child.returncode < 0 or child.returncode >= 128:
                outcome = "CRASH"
        out.seek(0)
        err.seek(0)
        return {"outcome": outcome, "exit": child.returncode,
                "stdout": out.read(outputLimit).decode("utf-8", errors="replace"),
                "stderr": err.read(outputLimit).decode("utf-8", errors="replace")}


def require(result, description):
    if result["outcome"] != "EXIT" or result["exit"] != 0:
        raise RuntimeError(f"{description}: {result}")
