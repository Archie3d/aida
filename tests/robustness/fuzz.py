#!/usr/bin/env python3
"""Reproducible black-box mutations through loading, parsing, sema and emission."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import sys

from process import run

kSeeds = [
    b"procedure P is begin null; end P;",
    b"procedure P is X : Integer := 2; begin X := X + 1; end P;",
    b"procedure P is type R is record X : Integer; end record; V : R := (X => 1); begin null; end P;",
    b"procedure P is type A is array (1 .. 3) of Integer; X : A := (others => 0); begin X (2) := 1; end P;",
    b"procedure P is function F (X : Integer) return Integer is begin return X; end F; begin null; end P;",
    b"procedure P is package Q is type T is range 0 .. 10; end Q; begin null; end P;",
    b"procedure P is generic type T is private; package G is end G; package I is new G (Integer); begin null; end P;",
    b"procedure P is X : Integer := " + b"1 + " * 63 + b"1; begin null; end P;",
    b"procedure P is X : Integer := " + b"(" * 16 + b"1" + b")" * 16 + b"; begin null; end P;",
    b"procedure P is begin " + b"declare begin " * 12 + b"null; " + b"end; " * 12 + b"end P;",
]


def corpus(seed, iterations):
    rng = random.Random(seed)
    for index, source in enumerate(kSeeds):
        yield f"control-{index}", source, True
    bad = [b"\xff\xc0\x80", b'procedure P is X : String := "unterminated;',
           b"procedure P is type T is record X : ; end record; begin null; end P;",
           b"procedure P is X : Integer := 2#9#; begin null; end P;",
           b"procedure P is begin if then end if; end P;",
           b"procedure P is begin " + b"declare begin " * 1500 + b"null; " + b"end; " * 1500 + b"end P;",
           b"procedure P is X : Integer := " + b"(" * 2000 + b"1" + b")" * 2000 + b"; begin null; end P;",
           b"procedure P is X : Integer := " + b"1 + " * 4000 + b"1; begin null; end P;",
           b"procedure P is begin X" + b".X" * 4000 + b" := 1; end P;",
           b"procedure P is X : Missing := 2; begin X := X + 1; end P;",
           b"@" * 300]
    for index, source in enumerate(bad):
        yield f"malformed-{index}", source, False
    for index in range(iterations):
        source = bytearray(rng.choice(kSeeds))
        for _ in range(rng.randrange(1, 5)):
            position = rng.randrange(len(source) + 1)
            operation = rng.randrange(4)
            if operation == 0:
                del source[position:position + rng.randrange(1, 16)]
            elif operation == 1:
                source[position:position] = rng.choice([b";", b"(", b"end ", b"\0", b"\xff", b"'", b"record "])
            elif operation == 2:
                source[position:position + 1] = bytes([rng.randrange(256)])
            else:
                source = source[:position]
        yield f"mutation-{index:05}", bytes(source), None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--adac", required=True)
    parser.add_argument("--work", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=819)
    parser.add_argument("--iterations", type=int, default=192)
    parser.add_argument("--timeout", type=float, default=3)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    observations = []
    failures = []
    for name, source, legal in corpus(args.seed, args.iterations):
        path = args.work / "p.adb"
        path.write_bytes(source)
        ir = args.work / "p.ssa"
        ir.unlink(missing_ok=True)
        result = run([args.adac, "--no-stdlib", "-o", ir, path], timeout=args.timeout)
        normal = result["outcome"] == "EXIT"
        accepted = normal and result["exit"] == 0 and ir.exists() and ir.stat().st_size > 0
        rejected = normal and result["exit"] == 1 and ": error:" in result["stderr"]
        expectedCapacity = {"malformed-5": "syntax nesting", "malformed-6": "syntax nesting",
                            "malformed-7": "expression depth", "malformed-8": "name suffix depth"}
        valid = (accepted or rejected) and not (legal is True and not accepted) and not (legal is False and not rejected)
        if name in expectedCapacity:
            valid = valid and (expectedCapacity[name] + " exceeds compiler limit of 128") in result["stderr"]
        if name == "malformed-9":
            valid = valid and "'missing' is not declared" in result["stderr"]
        if name == "malformed-10":
            valid = valid and "too many errors; further diagnostics suppressed" in result["stderr"]
            valid = valid and result["stderr"].count(": error:") == 101
        # Sanitizer failures must never masquerade as an ordinary rejection.
        valid = valid and not any(word in result["stderr"] for word in
                                  ["Sanitizer", "runtime error:", "Assertion", "terminate called"])
        digest = hashlib.sha256(source).hexdigest()
        observations.append({"case": name, "source_sha256": digest,
                             "outcome": result["outcome"], "exit": result["exit"]})
        if not valid:
            (args.work / (name + ".adb")).write_bytes(source)
            (args.work / (name + ".json")).write_text(json.dumps(result, indent=2) + "\n")
            failures.append(name)
    (args.work / "observations.json").write_text(json.dumps({"seed": args.seed, "cases": observations}, indent=2) + "\n")
    if failures:
        raise RuntimeError(f"robustness failures (sources and diagnostics in {args.work}): {failures}")
    print(f"{len(observations)} cases passed; seed {args.seed}")


if __name__ == "__main__":
    main()
