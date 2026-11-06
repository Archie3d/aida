#!/usr/bin/env python3
"""Annotate AIDA QBE loads, stores and blits using LLVM TSan range callbacks."""
import argparse
from pathlib import Path
import re

kLoads = {"loadsb": 1, "loadub": 1, "loadsh": 2, "loaduh": 2,
          "loadw": 4, "loadsw": 4, "loaduw": 4, "loads": 4, "loadl": 8, "loadd": 8}
kStores = {"storeb": 1, "storeh": 2, "storew": 4, "stores": 4, "storel": 8, "stored": 8}


def instrument(source):
    if "# aida-tsan-instrumented" in source:
        raise ValueError("IR is already instrumented")
    result = ["# aida-tsan-instrumented\n"]
    function = None
    accesses = 0
    for line in source.splitlines(keepends=True):
        code = line.split("#", 1)[0].strip()
        if re.match(r"^(?:export\s+)?function\s", code):
            match = re.search(r"\bfunction\s+(?:\w+\s+)?(\$[^\s(]+)\(", code)
            if not match:
                raise ValueError("unsupported function header: " + code)
            function = match.group(1)
        elif code == "}":
            function = None
        if function and code and not code.startswith("@"):
            instruction = re.sub(r"^%[^\s=]+\s*=\s*[wlsd]\s+", "", code)
            operation, _, operands = instruction.partition(" ")
            arguments = [argument.strip() for argument in operands.split(",")]
            events = []
            if operation in kLoads and len(arguments) == 1:
                events.append(("read", arguments[0], str(kLoads[operation])))
            elif operation in kStores and len(arguments) == 2:
                events.append(("write", arguments[1], str(kStores[operation])))
            elif operation == "blit" and len(arguments) == 3 and re.fullmatch(r"\d+", arguments[2]):
                events.extend([("read", arguments[0], arguments[2]), ("write", arguments[1], arguments[2])])
            elif operation.startswith(("load", "store", "blit", "vaarg", "vastart")):
                raise ValueError("unsupported memory instruction: " + code)
            for access, address, width in events:
                result.append(f"    call $__tsan_{access}_range_pc(l {address}, l {width}, l {function})\n")
                accesses += 1
        result.append(line)
    return "".join(result), accesses


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    output, count = instrument(args.source.read_text())
    args.output.write_text(output)
    print(f"instrumented {count} memory accesses")


if __name__ == "__main__":
    main()
