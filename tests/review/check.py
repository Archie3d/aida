"""Phase 11 compiler and driver review regressions."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import time

parser = argparse.ArgumentParser(description=__doc__)
for name in ['ada', 'adac', 'cc', 'qbe', 'runtime', 'hook', 'source', 'work']:
    parser.add_argument('--' + name, type=Path, required=True)
parser.add_argument('--exe-suffix', default='')
parser.add_argument('--separate-libm', action='store_true')
parser.add_argument('--sanitizer-flag', default='')
args = parser.parse_args()
for name in ['ada', 'adac', 'cc', 'qbe', 'runtime', 'hook', 'source', 'work']:
    setattr(args, name, getattr(args, name).resolve())
work = args.work
if work.exists():
    shutil.rmtree(work)
work.mkdir(parents=True)
env = os.environ.copy()
for key in ['ADAC', 'QBE', 'CC', 'ADA_INCLUDE_PATH', 'ADA_LIBRARY', 'ADA_RUNTIME']:
    env.pop(key, None)
env.update(ADAC=str(args.adac), QBE=str(args.qbe), CC=str(args.cc))


def run(command, environment=env, status=0):
    result = subprocess.run(list(map(str, command)), env=environment, cwd=work,
                            capture_output=True, text=True, timeout=45)
    assert result.returncode == status, (command, result.returncode, result.stdout, result.stderr)
    return result


# Execute identical checks through per-unit objects and whole-program assembly.
program = work / ('review' + args.exe_suffix)
run([args.ada, '-o', program, args.source])
run([program])
assembly = work / 'review.s'
run([args.ada, '-S', '-o', assembly, args.source])
run([args.cc, '-pthread', *([args.sanitizer_flag] if args.sanitizer_flag else []), assembly, args.runtime,
     *(['-lm'] if args.separate_libm else []), '-o', program])
run([program])

# The grammar permits repetitions of one logical operator at each level.
operators = ['and', 'or', 'xor', 'and then', 'or else']
source = work / 'logic.adb'
prefix = 'procedure Logic is\n    X : Boolean;\nbegin\n    X := '
for left in operators:
    for right in operators:
        expression = f'True {left} False {right} False'
        source.write_text(prefix + expression + ';\nend Logic;\n')
        result = run([args.adac, '--no-stdlib', '-o', work / 'logic.ssa', source],
                     status=0 if left == right else 1)
        if left != right:
            column = len('    X := ' + f'True {left} False ') + 1
            diagnostic = f'{source}:4:{column}: error: different logical operators require parentheses\n'
            assert result.stderr == diagnostic, result.stderr
        for expression in [f'(True {left} False) {right} False',
                           f'True {left} (False {right} False)']:
            source.write_text(prefix + expression + ';\nend Logic;\n')
            run([args.adac, '--no-stdlib', '-o', work / 'logic.ssa', source])
# Parentheses do not excuse mixing inside a nested expression.
source.write_text(prefix + '(True or False and False);\nend Logic;\n')
result = run([args.adac, '--no-stdlib', '-o', work / 'logic.ssa', source], status=1)
assert ':4:25: error: different logical operators require parentheses' in result.stderr, result.stderr

# Identically named native tools selected from different PATH directories must
# invalidate every native object, even when the proxy executable bytes match.
source = work / 'root.adb'
source.write_text('procedure Root is begin null; end Root;\n')
for tool, real in [('CC', args.cc), ('QBE', args.qbe)]:
    directory = work / tool.lower()
    directory.mkdir()
    paths = [directory / 'first', directory / 'second']
    name = 'native-proxy' + args.exe_suffix
    for path in paths:
        path.mkdir()
        shutil.copy2(args.hook, path / name)
    trace = directory / 'trace'
    selected = dict(env, **{tool: name}, AIDA_TEST_REAL_TOOL=str(real), AIDA_TEST_TRACE=str(trace))
    objects = directory / 'objects'
    command = [args.ada, '--force', '-D', objects, '-o', directory / ('root' + args.exe_suffix), source]

    def stamps():
        return {path.name: (path.stat().st_ino, path.stat().st_mtime_ns)
                for path in objects.glob('*.o')}

    selected['PATH'] = str(paths[0]) + os.pathsep + env.get('PATH', '')
    run(command, selected)
    before = stamps()
    assert len(before) >= 2, before  # main and binder
    calls = trace.read_text().splitlines()
    run(command, selected)
    assert stamps() == before, 'unchanged toolchain rebuilt native objects'
    # CC still links on --force; QBE must do no work at all.
    extra = trace.read_text().splitlines()[len(calls):]
    if tool == 'QBE':
        assert not extra, extra
    else:
        assert not any(' -c ' in line for line in extra), extra
    selected['PATH'] = str(paths[1]) + os.pathsep + env.get('PATH', '')
    run(command, selected)
    after = stamps()
    assert before.keys() == after.keys() and all(before[key] != after[key] for key in before), (before, after)
    run([directory / ('root' + args.exe_suffix)], selected)
    run(command, selected)
    assert stamps() == after, 'selected replacement toolchain did not reuse objects'

# File outputs must need write access only in the output directory. Keep the
# guard there for both driver and direct front-end calls. Unix permission bits
# are not meaningful on Windows (or for root).
parent = work / 'readonly'
output = parent / 'writable'
output.mkdir(parents=True)
if os.name != 'nt':
    parent.chmod(0o555)
try:
    for mode, suffix in [('-S', '.s'), ('--emit-ir', '.ssa')]:
        run([args.ada, mode, '-o', output / ('result' + suffix), source])
    run([args.adac, '-o', output / 'direct.ssa', source])
    assert not (work / '.readonly.aida-lock').exists()
    assert not (parent / '.writable.aida-lock').exists()

    # Hold the first writer inside its tool, then verify that the second cannot
    # invoke even the front end until publication and receipt updates finish.
    for mode, suffix, tool, real in [('-S', '.s', 'QBE', args.qbe),
                                     ('--emit-ir', '.ssa', 'ADAC', args.adac)]:
        proxy = work / ('blocking-proxy' + args.exe_suffix)
        shutil.copy2(args.hook, proxy)
        ready = work / ('ready' + suffix)
        release = work / ('release' + suffix)
        trace = work / ('trace' + suffix)
        target = output / ('concurrent' + suffix)
        selected = dict(env, **{tool: str(proxy)}, AIDA_TEST_REAL_TOOL=str(real),
                        AIDA_TEST_TRACE=str(trace), AIDA_TEST_SELECT=target.name,
                        AIDA_TEST_READY=str(ready), AIDA_TEST_RELEASE=str(release))
        command = list(map(str, [args.ada, '--force', mode, '-o', target, source]))
        processes = []
        try:
            processes.append(subprocess.Popen(command, env=selected, cwd=work,
                                               stdout=subprocess.PIPE, stderr=subprocess.PIPE))
            deadline = time.monotonic() + 15
            while not ready.exists():
                assert processes[0].poll() is None, processes[0].communicate()
                assert time.monotonic() < deadline, 'first writer did not reach the tool'
                time.sleep(0.02)
            before = trace.read_bytes()
            # Use another artifact directory so only the output guard can
            # serialize these writers.
            second_command = [*command, '-D', str(work / ('second-objects' + suffix))]
            processes.append(subprocess.Popen(second_command, env=selected, cwd=work,
                                               stdout=subprocess.PIPE, stderr=subprocess.PIPE))
            time.sleep(0.3)
            assert processes[1].poll() is None, processes[1].communicate()
            assert trace.read_bytes() == before, 'concurrent writer entered tool before lock release'
            release.touch()
            for process in processes:
                stdout, stderr = process.communicate(timeout=30)
                assert process.returncode == 0, (stdout, stderr)
            assert target.stat().st_size > 0
        finally:
            release.touch()
            for process in processes:
                if process.poll() is None:
                    process.kill()
                    process.communicate()
finally:
    if os.name != 'nt':
        parent.chmod(0o755)
