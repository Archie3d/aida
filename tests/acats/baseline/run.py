#!/usr/bin/env python3
"""Observe the explicit ACATS selection; frozen failures are not conformance passes."""

import argparse
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import signal
import subprocess
import sys
import tempfile

kRoot = Path(__file__).resolve().parent
kCritical = {"CRASH", "TIMEOUT", "OUTPUT_LIMIT"}
kCOutcomes = {"PASS", "COMPILE_REJECTED", "HARNESS_BLOCKED", "BUILD_FAILED", "RUN_FAILED", "NOT_APPLICABLE"} | kCritical
kBOutcomes = {"B_ACCEPTED_UNGRADED", "B_REJECTED_UNGRADED"} | kCritical


def readJson(path):
    return json.loads(path.read_text(encoding="utf-8"))


def validate(manifest, expected=None):
    if manifest["schema"] != 1:
        raise ValueError("unknown manifest schema")
    ids = [case["id"] for case in manifest["cases"]]
    if len(ids) != len(set(ids)):
        raise ValueError("duplicate case identity")
    used = set()
    for case in manifest["cases"]:
        if case["class"] not in {"B", "C"} or not case["objective"]:
            raise ValueError(f"invalid case metadata: {case['id']}")
        for source in case["sources"]:
            path = (kRoot / "upstream" / source).resolve()
            if not path.is_relative_to(kRoot / "upstream"):
                raise ValueError("source escapes the baseline source directory")
            data = path.read_bytes()
            digest = hashlib.sha256(data).hexdigest()
            blob = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
            if manifest["files"][source] != {"sha256": digest, "git_blob": blob}:
                raise ValueError(f"modified upstream source: {source}")
            used.add(source)
    if used != set(manifest["files"]):
        raise ValueError("unreferenced or unregistered source")
    if expected is not None:
        if set(ids) != set(expected["cases"]):
            raise ValueError("snapshot and selection differ")
        if expected["source_revision"] != manifest["revision"]:
            raise ValueError("snapshot source revision differs")
        for case in manifest["cases"]:
            outcome = expected["cases"][case["id"]]["outcome"]
            allowed = kBOutcomes if case["class"] == "B" else kCOutcomes
            if outcome not in allowed:
                raise ValueError("observation is invalid for its test class")
            result = expected["cases"][case["id"]]
            if outcome == "PASS" and (result["compile_exit"] != 0 or result["run_exit"] != 0
                or result["run_stderr"] or result["run_stdout"] != f"TEST {case['report_name']}\nPASSED\n"):
                raise ValueError("PASS requires the complete successful REPORT protocol")
        if expected["counts"] != dict(sorted(Counter(v["outcome"] for v in expected["cases"].values()).items())):
            raise ValueError("snapshot counts do not match its cases")


def runCommand(command, timeout):
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               start_new_session=(os.name == "posix"))
    expired = False
    try:
        out, err = process.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        expired = True
        if os.name == "posix":
            os.killpg(process.pid, signal.SIGKILL)
        else:
            process.kill()
        out, err = process.communicate()
    limited = len(out) + len(err) > 1024 * 1024
    return {"exit": process.returncode, "timeout": expired, "output_limit": limited,
            "stdout": out[:1024 * 1024].decode("utf-8", errors="replace"),
            "stderr": err[:1024 * 1024].decode("utf-8", errors="replace")}


def normalize(value, directory):
    # Match the most specific paths first, including Darwin's /private aliases.
    substitutions = [(str(directory), "<work>"), (str(kRoot / "upstream"), "<upstream>"),
                     (str(kRoot.parent / "support"), "<support>"),
                     (str(kRoot.parents[2]), "<repo>")]
    for source, target in substitutions:
        value = value.replace(source, target)
    return value.replace("\r\n", "\n")


def abnormal(result):
    if result["timeout"]:
        return "TIMEOUT"
    if result["output_limit"]:
        return "OUTPUT_LIMIT"
    if result["exit"] < 0 or re.search(
            r"Assertion.*failed|Segmentation fault|AddressSanitizer|UndefinedBehaviorSanitizer|"
            r"libc\+\+abi:|terminate called|Stack dump:|Abort trap", result["stderr"], re.I):
        return "CRASH"
    return None


def supportBlocked(case, errors):
    # A known prerequisite is not proof that it caused this compilation to
    # fail. Keep earlier parser/semantic failures visible as compiler gaps.
    for gap in case["harness_gaps"]:
        if gap.startswith("REPORT."):
            routine = re.escape(gap.split(".")[1])
            if re.search(rf"(?:'{routine}'.*not declared|unknown.*\b{routine}\b)", errors, re.I):
                return True
        elif gap.startswith("ImpDef") and "cannot find the unit 'impdef'" in errors.lower():
            return True
        elif gap.startswith("TCTouch."):
            routine = re.escape(gap.split(".")[1].split()[0])
            if re.search(rf"'{routine}'.*not declared in 'tctouch'", errors, re.I):
                return True
    return False


def observeCase(case, args):
    work = args.work / case["id"]
    work.mkdir(parents=True, exist_ok=True)
    support = kRoot.parent / "support"
    with tempfile.TemporaryDirectory(prefix="attempt-", dir=work) as temporary:
        directory = Path(temporary)
        sources = [str(kRoot / "upstream" / name) for name in case["sources"]]
        helpers = [str(support / "report.ads"), str(support / "report.adb")]
        if "tctouch" in case["support"]:
            helpers += [str(support / "tctouch.ads"), str(support / "tctouch.adb")]
        program = directory / "program"
        if case["class"] == "B":
            command = [str(args.adac), "-o", str(directory / "test.ssa"), *helpers, *sources]
        else:
            # Upstream ACATS also uses .a for Ada sources; explicitly override
            # the driver's static-archive interpretation without renaming files.
            source_inputs = []
            for source in sources:
                source_inputs.extend(["--source", source])
            command = [str(args.ada), "-o", str(program), *helpers, *source_inputs]
        compiled = runCommand(command, args.compile_timeout)
        observed = {"compile_exit": compiled["exit"],
                    "compile_stdout": normalize(compiled["stdout"], directory),
                    "compile_stderr": normalize(compiled["stderr"], directory)}
        outcome = abnormal(compiled)
        if outcome:
            observed["outcome"] = outcome
        elif case["class"] == "B":
            observed["outcome"] = "B_ACCEPTED_UNGRADED" if compiled["exit"] == 0 else "B_REJECTED_UNGRADED"
        elif compiled["exit"] != 0:
            observed["outcome"] = "HARNESS_BLOCKED" if supportBlocked(case, compiled["stderr"]) else (
                "COMPILE_REJECTED" if re.search(r"\.(?:a|ada|am|ads|adb):\d+:\d+: error:", compiled["stderr"], re.I)
                else "BUILD_FAILED")
        else:
            executed = runCommand([str(program)], args.run_timeout)
            observed.update(run_exit=executed["exit"],
                            run_stdout=normalize(executed["stdout"], directory),
                            run_stderr=normalize(executed["stderr"], directory))
            expected = f"TEST {case['report_name']}\nPASSED\n"
            observed["outcome"] = abnormal(executed) or (
                "PASS" if executed["exit"] == 0 and not executed["stderr"]
                and executed["stdout"].replace("\r\n", "\n") == expected else "RUN_FAILED")
            if observed["outcome"] not in kCritical and "FAILED not applicable:" in executed["stdout"]:
                observed["outcome"] = "NOT_APPLICABLE"
        (work / "observation.json").write_text(json.dumps(observed, indent=2) + "\n", encoding="utf-8")
        return case["id"], observed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ada", type=Path)
    parser.add_argument("--adac", type=Path)
    parser.add_argument("--work", type=Path)
    parser.add_argument("--jobs", type=int, default=2)
    parser.add_argument("--compile-timeout", type=float, default=30)
    parser.add_argument("--run-timeout", type=float, default=5)
    parser.add_argument("--case", action="append", default=[])
    modes = parser.add_mutually_exclusive_group(required=True)
    modes.add_argument("--record", action="store_true", help="replace the reviewed observation snapshot")
    modes.add_argument("--check", action="store_true", help="check snapshot stability, not language conformance")
    modes.add_argument("--observe", action="store_true", help="write observations without changing the snapshot")
    modes.add_argument("--validate", action="store_true", help="check source provenance and snapshot metadata only")
    args = parser.parse_args()
    manifest = readJson(kRoot / "manifest.json")
    expected = readJson(kRoot / "expected.json") if (kRoot / "expected.json").exists() else None
    validate(manifest, expected if args.check or args.validate else None)
    if args.validate and expected is None:
        parser.error("no snapshot to validate")
    if args.validate:
        print(f"Validated {len(manifest['cases'])} explicitly selected cases and upstream source hashes")
        return 0
    if not args.ada or not args.adac or not args.work or args.jobs < 1:
        parser.error("execution requires --ada, --adac, --work and positive --jobs")
    args.ada = args.ada.resolve(strict=True)
    args.adac = args.adac.resolve(strict=True)
    args.work = args.work.resolve()
    selected = [case for case in manifest["cases"] if not args.case or case["id"] in args.case]
    if args.case and set(args.case) != {case["id"] for case in selected}:
        parser.error("unknown case")
    if args.record and args.case:
        parser.error("recording requires the complete selection")
    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        results = dict(pool.map(lambda case: observeCase(case, args), selected))
    counts = Counter(value["outcome"] for value in results.values())
    snapshot = {"schema": 1, "source_revision": manifest["revision"],
                "host": {"system": platform.system(), "machine": platform.machine()},
                "compiler_sha256": hashlib.sha256(args.adac.read_bytes()).hexdigest(),
                "counts": dict(sorted(counts.items())), "cases": results}
    args.work.mkdir(parents=True, exist_ok=True)
    (args.work / "results.json").write_text(json.dumps(snapshot, indent=2) + "\n", encoding="utf-8")
    if args.record:
        (kRoot / "expected.json").write_text(json.dumps(snapshot, indent=2) + "\n", encoding="utf-8")
    print("Observed ACATS baseline (B observations are ungraded):")
    for outcome, count in sorted(counts.items()):
        print(f"  {outcome}: {count}")
    drift = []
    if args.check:
        if expected is None:
            parser.error("no snapshot; use --record first")
        drift = [name for name, result in results.items() if result != expected["cases"][name]]
        for name in drift:
            print(f"DRIFT {name}: recorded {expected['cases'][name]['outcome']}, observed {results[name]['outcome']}")
        print("Snapshot " + ("changed" if drift else "unchanged") + "; unchanged failures are not ACATS passes.")
    critical = [name for name, result in results.items() if result["outcome"] in kCritical]
    if critical:
        print("Critical observations require triage: " + ", ".join(critical))
    return 1 if critical or drift else 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError, KeyError) as error:
        print(f"baseline error: {error}", file=sys.stderr)
        sys.exit(2)
