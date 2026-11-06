#!/usr/bin/env python3
"""Verify byte coverage and operand preservation, including unaligned copies."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from tsan_ir import instrument

# Independent inventory from the QBE memory-instruction contract.
kLoads = ["loadsb", "loadub", "loadsh", "loaduh", "loadw", "loadsw", "loaduw", "loads", "loadl", "loadd"]
kStores = ["storeb", "storeh", "storew", "stores", "storel", "stored"]

lines = ['data $message = { b "function is data" 0 }', 'function $probe(l %p, l %q) {', '@start']
for opcode in kLoads:
    resultType = "d" if opcode == "loadd" else "s" if opcode == "loads" else "l"
    lines.append(f"    %v{len(lines)} ={resultType} {opcode} %p")
for opcode in kStores:
    lines.append(f"    {opcode} 0, %q")
lines.extend(['    blit %p, %q, 19', '    ret', '}'])
source = "\n".join(lines) + "\n"
text, count = instrument(source)
assert count == len(kLoads) + len(kStores) + 2
assert "read_range_pc(l %p, l 19, l $probe)" in text
assert "write_range_pc(l %q, l 19, l $probe)" in text
assert "\n".join(line for line in text.splitlines() if not line.startswith("# aida-") and "call $__tsan_" not in line) + "\n" == source
for broken in [text, 'function $p() {\n@start\n %v =l loadunknown %p\n ret\n}\n']:
    try:
        instrument(broken)
    except ValueError:
        pass
    else:
        raise AssertionError("unsupported or double instrumentation was accepted")
print("IR instrumentation coverage passed")
