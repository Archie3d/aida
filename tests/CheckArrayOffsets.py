"""Check successful constant folding and preserved runtime failure paths."""
import argparse
from pathlib import Path
import re
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--adac', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(args.adac), '-D', str(args.work), str(args.source)], check=True, timeout=25)
    ir = (args.work / 'constantarrayoffsets.ssa').read_text()
    functions = dict(re.findall(r'^function \w* ?\$constantarrayoffsets__(\w+)\([^\n]*\) \{\n(.*?)^\}', ir, re.M | re.S))
    for name, offset in [('known', 8), ('extreme', 4)]:
        body = functions[name]
        assert 'call $__ada_array_offset' not in body, name
        # The folded result must still publish its location and inspect pending status.
        assert re.search(r'storel \$[^,]+, %\.traceLocation\n    %[^ ]+ =l copy ' + str(offset)
                         + r'\n    %[^ ]+ =l loadl %\.taskContext\n    %[^ ]+ =w cnel ', body), name
    for name in ('dynamic', 'nullrange', 'packed'):
        assert 'call $__ada_array_offset' in functions[name], name


if __name__ == '__main__':
    main()
