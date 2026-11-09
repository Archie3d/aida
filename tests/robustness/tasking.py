#!/usr/bin/env python3
"""Build generated Ada with a sanitized C runtime; optionally annotate QBE IR."""
import argparse
import json
import os
import shutil
from pathlib import Path
import sys

from process import run, require

kRoot = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(kRoot / "tools"))
from tsan_ir import instrument


def build(args, source, directory, separate, annotations):
    directory.mkdir(parents=True, exist_ok=True)
    timeout = args.compile_timeout
    if separate:
        require(run([args.adac, "-D", directory, source], timeout), "whole-unit analysis")
        sources = [line[7:] for line in (directory / "units.manifest").read_text().splitlines()
                   if line.startswith("source ")]
        for path in dict.fromkeys(sources):
            require(run([args.adac, "-c", "-D", directory, path], timeout), "separate compilation")
        require(run([args.adac, "--bind", source.stem.lower(), "-D", directory,
                     "-o", directory / "__ada_binder.ssa"], timeout), "binding")
        units = sorted(directory.glob("*.ssa"))
    else:
        unit = directory / "program.ssa"
        require(run([args.adac, "-o", unit, source], timeout), "whole compilation")
        units = [unit]
    assemblies = []
    accesses = 0
    for unit in units:
        lowered = unit
        if annotations:
            text, count = instrument(unit.read_text())
            accesses += count
            lowered = unit.with_suffix(".tsan")
            lowered.write_text(text)
        assembly = unit.with_suffix(".s")
        require(run([args.qbe, "-o", assembly, lowered], timeout), "QBE lowering")
        assemblies.append(assembly)
    if annotations and accesses == 0:
        raise RuntimeError("no generated memory accesses were instrumented")
    program = directory / "program"
    flag = "-fsanitize=thread" if args.sanitizer == "thread" else "-fsanitize=address,undefined"
    helpers = [kRoot / "tests" / "TaskingHooks.c"] if source.stem == "taskconcurrency" else []
    require(run([args.cc, "-pthread", flag, *assemblies, *helpers, args.runtime, "-lm", "-o", program], timeout), "linking")
    return program, accesses


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ["adac", "qbe", "cc", "runtime"]:
        parser.add_argument("--" + name, required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--sanitizer", choices=["thread", "address-undefined"], required=True)
    parser.add_argument("--compile-timeout", type=float, default=60)
    parser.add_argument("--run-timeout", type=float, default=30)
    args = parser.parse_args()
    if kRoot.is_relative_to(args.work.resolve()):
        parser.error("the work directory must not contain the source checkout")
    shutil.rmtree(args.work, ignore_errors=True)
    cases = ["taskconcurrency", "tasks", "protectedconcurrency", "protectedentries", "timevalues", "sharedscalars", "taskidentity", "concurrentresults"]
    results = []
    for name in cases + (["race"] if args.sanitizer == "thread" else []):
        source = Path(__file__).with_name("race.adb") if name == "race" else kRoot / "tests" / "ada" / "tasks" / (name + ".adb")
        for separate in [False, True]:
            mode = "separate" if separate else "whole"
            directory = args.work / name / mode
            # Never let stale compiled IR or sanitizer output satisfy a check.
            shutil.rmtree(directory, ignore_errors=True)
            program, accesses = build(args, source, directory, separate, args.sanitizer == "thread")
            environment = dict(os.environ)
            if args.sanitizer == "thread":
                environment["TSAN_OPTIONS"] = "halt_on_error=1:exitcode=66:abort_on_error=0:symbolize=0"
            result = run([program], args.run_timeout, cwd=directory, env=environment)
            (directory / "run.json").write_text(json.dumps(result, indent=2) + "\n")
            if name == "race":
                if result["outcome"] != "EXIT" or result["exit"] != 66 or "ThreadSanitizer: data race" not in result["stderr"]:
                    raise RuntimeError(f"instrumented Ada race was not detected: {result}")
            else:
                require(result, f"{name} {mode}")
                expected = source.with_suffix(".expected").read_text()
                if result["stdout"] != expected or result["stderr"]:
                    raise RuntimeError(f"{name} {mode} output differs: {result}")
            results.append({"case": name, "mode": mode, "instrumented_accesses": accesses})
    (args.work / "coverage.json").write_text(json.dumps(results, indent=2) + "\n")
    print(f"{len(results)} sanitized tasking executions passed")


if __name__ == "__main__":
    main()
