"""Check exact initialization coverage independently of aggregate value stores."""
import argparse
import os
from pathlib import Path
import re
import subprocess


def check(ir):
    functions = dict(re.findall(r'^function \$aggregatezeroing__(\w+)\([^\n]*\) \{\n(.*?)^\}', ir, re.M | re.S))
    machineControl = ['export function w $main() {', '@start', '    %wrong.0 =w copy 0']
    checked = 0
    for size in (0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32):
        body = functions['check' + str(size)]
        assert 'call $memset' not in body and 'call $__ada_array_local' not in body, size
        # Examine initialization before any aggregate component is evaluated.
        region = re.search(r'^@aggregateboundsready\.\d+\n(.*?)    jmp @aggregatefill', body, re.M | re.S)[1]
        assert re.match(r'    storel \$[^,]+, %\.traceLocation\n', region), size
        stores = re.findall(r'^    store([bwhl]) 0, (%[\w.]+)$', region, re.M)
        # The final zero initializes the separate loop counter, not scratch.
        counter = stores.pop()[1]
        assert re.search(r'=l loadl ' + re.escape(counter) + r'\n', body), size
        base = stores[0][1]
        allocation = re.search(re.escape(base) + r' =l alloc([48]) (\d+)\n', body)
        assert int(allocation[2]) == max(1, size), size
        alignment = int(allocation[1])
        addresses = {base: 0}
        for target, origin, offset in re.findall(r'(%[\w.]+) =l add (%[\w.]+), (\d+)\n', region):
            assert origin == base, size
            addresses[target] = int(offset)
        covered = []
        for kind, address in stores:
            width = {'b': 1, 'h': 2, 'w': 4, 'l': 8}[kind]
            offset = addresses[address]
            assert alignment >= width and offset % width == 0, (size, width, offset)
            covered.extend(range(offset, offset + width))
        assert sorted(covered) == list(range(max(1, size))), (size, covered)
        # Execute the actual emitted zero instructions on dirty memory, with
        # canaries on both sides. Aggregate assignments otherwise overwrite it.
        prefix = '%control.' + str(size)
        count = max(1, size)
        machineControl.append(f'    {prefix}.raw =l alloc8 {count + 16}')
        machineControl.append(f'    {base} =l add {prefix}.raw, 8')
        for offset in range(count + 16):
            machineControl.extend([f'    {prefix}.p{offset} =l add {prefix}.raw, {offset}',
                                   f'    storeb 255, {prefix}.p{offset}'])
        initialization = region.splitlines()[1:-1] # Exclude trace publication and loop counter.
        machineControl.extend(initialization)
        for offset in range(count + 16):
            expected = 0 if 8 <= offset < 8 + count else 255
            machineControl.extend([f'    {prefix}.v{offset} =w loadub {prefix}.p{offset}',
                                   f'    {prefix}.bad{offset} =w cnew {prefix}.v{offset}, {expected}',
                                   f'    %wrong.{checked + 1} =w or %wrong.{checked}, {prefix}.bad{offset}'])
            checked += 1
    assert re.search(r'call \$memset\(l %[^,]+, w 0, l 33\)', functions['check33'])
    for name in ('check65537', 'dynamic'):
        assert 'call $__ada_array_local' in functions[name], name
    machineControl.extend([f'    ret %wrong.{checked}', '}'])
    return '\n'.join(machineControl) + '\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--adac', type=Path, required=True)
    parser.add_argument('--qbe', type=Path, required=True)
    parser.add_argument('--cc', type=Path, required=True)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(args.adac), '-D', str(args.work), str(args.source)], check=True, timeout=25)
    control = args.work / 'zero-control.ssa'
    control.write_text(check((args.work / 'aggregatezeroing.ssa').read_text()))
    assembly = control.with_suffix('.s')
    program = args.work / ('zero-control.exe' if os.name == 'nt' else 'zero-control')
    target = ['-t', 'amd64_win'] if os.name == 'nt' else []
    subprocess.run([str(args.qbe), *target, '-o', str(assembly), str(control)], check=True, timeout=10)
    subprocess.run([str(args.cc), str(assembly), '-o', str(program)], check=True, timeout=10)
    result = subprocess.run([str(program.resolve())], capture_output=True, check=True, timeout=10)
    assert not result.stdout and not result.stderr


if __name__ == '__main__':
    main()
