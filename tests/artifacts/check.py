"""Exercise publication failures, interruption, shared writers and cache provenance."""

import argparse
import os
from pathlib import Path
import shutil
import signal
import subprocess
import time


def fingerprint(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & ((1 << 64) - 1)
    return f'{value:016x}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['ada', 'adac', 'cc', 'hook', 'work']:
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--exe-suffix', default='')
    args = parser.parse_args()
    for name in ['ada', 'adac', 'cc', 'hook', 'work']:
        setattr(args, name, getattr(args, name).resolve())
    work = args.work
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    environment = os.environ.copy()
    for key in ['ADAC', 'QBE', 'CC', 'ADA_INCLUDE_PATH', 'ADA_LIBRARY', 'ADA_RUNTIME']:
        environment.pop(key, None)
    processes = []
    hooks = set()

    def run(command, env=None, diagnostic=None):
        result = subprocess.run(list(map(str, command)), env=environment if env is None else env,
                                capture_output=True, text=True, timeout=60)
        if diagnostic is None:
            if result.returncode != 0:
                raise RuntimeError(f'{command}\n{result.returncode}\n{result.stdout}{result.stderr}')
        elif result.returncode == 0 or diagnostic not in result.stderr:
            raise RuntimeError(f'expected {diagnostic}: {command}\n{result.returncode}\n{result.stdout}{result.stderr}')
        return result

    def write(path, text):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text + '\n')

    source = work / 'app/root.adb'
    objects = work / 'objects'
    program = work / ('program' + args.exe_suffix)

    def root(value, path=source):
        write(path, f'''with Ada.Text_IO;
procedure Root is begin Ada.Text_IO.Put_Line (Integer'Image ({value})); end Root;''')

    def build(env=None, directory=objects, output=program, sources=None):
        return run([args.ada, '-v', '-D', directory, '-o', output, *(sources or [source])], env)

    def output(value, path=program):
        result = run([path])
        if result.stdout != f' {value}\n' or result.stderr:
            raise RuntimeError(f'wrong executable: {result.stdout}{result.stderr}')

    def start(command, log, env=None):
        with log.open('w') as stream:
            process = subprocess.Popen(list(map(str, command)), env=environment if env is None else env,
                                       stdout=stream, stderr=subprocess.STDOUT)
        processes.append(process)
        return process

    def finish(process, log):
        status = process.wait(timeout=60)
        if status != 0:
            raise RuntimeError(f'concurrent build failed: {status}\n{log.read_text()}')

    def hook_environment(name, selector, real, role='CC', mode='pause'):
        ready, release = work / (name + '.ready'), work / (name + '.release')
        ready.unlink(missing_ok=True)
        release.unlink(missing_ok=True)
        env = dict(environment, AIDA_TEST_REAL_TOOL=str(real), AIDA_TEST_SELECT=selector,
                   AIDA_TEST_MODE=mode, AIDA_TEST_READY=str(ready), AIDA_TEST_RELEASE=str(release))
        env[role] = str(args.hook)
        return env, ready, release

    def wait_ready(process, ready):
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            if ready.exists() and ready.read_text().splitlines():
                pid = int(ready.read_text().splitlines()[0])
                hooks.add(pid)
                return pid
            if process.poll() is not None:
                raise RuntimeError('hooked build exited before reaching the selected tool')
            time.sleep(0.01)
        raise RuntimeError('hooked build did not reach the selected tool')

    try:
        root(1)
        build()
        output(1)
        # Cache records bind every interface alias and the actual native object.
        for suffix in ['.asi', '.ali', '.o', '.build']:
            target = objects / ('root' + suffix)
            target.write_bytes(b'truncated')
            build()
            output(1)
        run([args.adac, '--bind', 'root', '-D', objects])
        # Bundle aliases are members of the same commit, including aliases
        # which the current client does not itself name.
        bundle = work / 'aliases/custom bundle.ada'
        write(bundle, 'package AA is N : constant Integer := 1; end AA;\npackage BB is end BB;')
        write(source, "with Ada.Text_IO; with AA; procedure Root is begin Ada.Text_IO.Put_Line (Integer'Image (AA.N)); end Root;")
        build(sources=[bundle, source])
        (objects / '.aida-interfaces/bb.spec.asi').unlink()
        repaired = build(sources=[bundle, source])
        if not (objects / '.aida-interfaces/bb.spec.asi').exists() or 'custom bundle needs no compiling' in repaired.stderr:
            raise RuntimeError('an incomplete interface alias set was reused')
        root(1)
        build()
        # Force a failure at the final multi-file commit record. Restoring an
        # old record cannot make the newly published members a valid group.
        record = objects / 'root.ali'
        saved_record = record.read_bytes()
        record.unlink()
        record.mkdir()
        root(2)
        run([args.ada, '-D', objects, '-o', program, source], diagnostic='cannot publish compiled unit metadata')
        output(1)
        record.rmdir()
        record.write_bytes(saved_record)
        run([args.adac, '--bind', 'root', '-D', objects], diagnostic='stale compiled unit')
        build()
        output(2)
        root(1)
        build()
        # A syntactically complete but incomplete dependency set is not a hit.
        record = objects / 'root.ali'
        data = record.read_text()
        body = data[:data.rfind('\nend ') + 1]
        removed = next(line.split()[1] for line in body.splitlines() if line.startswith('dep '))
        body = ''.join(line for line in body.splitlines(keepends=True)
                       if not line.startswith(('dep ' + removed + ' ', 'source ' + removed + ' ')))
        record.write_bytes((body + 'end ' + fingerprint(body.encode()) + '\n').encode())
        run([args.adac, '--bind', 'root', '-D', objects], diagnostic='stale compiled unit')
        build()
        output(1)
        # Prefix-truncated manifests must not become smaller valid programs.
        manifest = objects / 'units.manifest'
        original = manifest.read_bytes()
        manifest.write_bytes(original[:original.rfind(b'end-manifest')])
        run([args.adac, '--bind', 'root', '-D', objects], diagnostic='cannot read')
        build()
        # Native compiler and linker failures preserve previously published files.
        old_object = (objects / 'root.o').read_bytes()
        old_program = program.read_bytes()
        root(2)
        env, _, _ = hook_environment('compile-fail', 'root.o', args.cc, mode='fail')
        run([args.ada, '-D', objects, '-o', program, source], env, diagnostic='')
        if (objects / 'root.o').read_bytes() != old_object or program.read_bytes() != old_program:
            raise RuntimeError('a failed tool overwrote a committed object or executable')
        output(1)
        build()
        output(2)
        old_program = program.read_bytes()
        env, _, _ = hook_environment('link-fail', program.name, args.cc, mode='fail')
        run([args.ada, '-D', objects, '-o', program, source], env, diagnostic='')
        if program.read_bytes() != old_program:
            raise RuntimeError('a failed linker overwrote the executable')
        output(2)
        # Same-directory drivers and standalone frontends must wait for the
        # active build, including equivalent spellings of the directory.
        root(3)
        env, ready, release = hook_environment('concurrent', 'root.o', args.cc)
        log1, log2, log3 = work / 'first.log', work / 'second.log', work / 'frontend.log'
        first = start([args.ada, '-v', '-D', objects, '-o', program, source], log1, env)
        pid = wait_ready(first, ready)
        second_program = work / ('second' + args.exe_suffix)
        second = start([args.ada, '-v', '-D', str(objects) + '/../objects', '-o', second_program, source], log2)
        frontend = start([args.adac, '--scan', '-D', objects, source], log3)
        time.sleep(0.2)
        if second.poll() is not None or frontend.poll() is not None or log2.stat().st_size:
            raise RuntimeError('another writer entered a locked artifact directory')
        release.touch()
        finish(first, log1)
        hooks.discard(pid)
        finish(second, log2)
        finish(frontend, log3)
        output(3)
        output(3, second_program)
        # Terminating only the driver must leave exclusion with its live tool.
        root(4)
        env, ready, release = hook_environment('interrupted', 'root.o', args.cc)
        first = start([args.ada, '-D', objects, '-o', program, source], log1, env)
        pid = wait_ready(first, ready)
        first.kill()
        first.wait(timeout=10)
        second = start([args.ada, '-v', '-D', objects, '-o', second_program, source], log2)
        time.sleep(0.2)
        if second.poll() is not None or log2.stat().st_size:
            raise RuntimeError('driver termination released a live child tool lock')
        os.kill(pid, signal.SIGTERM)
        hooks.discard(pid)
        finish(second, log2)
        output(4, second_program)
        output(3)  # The interrupted build did not publish its executable.
        # A source edit between scan and target compilation cannot be stamped
        # as if all units were built from the original scan.
        root(5)
        env, ready, release = hook_environment('source-edit', 'frontend', args.adac, role='ADAC')
        first = start([args.ada, '-D', objects, '-o', program, source], log1, env)
        pid = wait_ready(first, ready)
        root(6)
        release.touch()
        status = first.wait(timeout=30)
        hooks.discard(pid)
        if status == 0 or 'stale compiled unit' not in log1.read_text():
            raise RuntimeError('a changed input was accepted under the old scan: ' + log1.read_text())
        output(3)
        build()
        output(6)
        # Back-pressure on stdout freezes emission after parsing/analysis.
        # Editing the parsed source must be diagnosed rather than stamped as
        # though the emitter had consumed the new bytes.
        large = work / 'large.adb'
        write(large, 'procedure Large is X : Integer := 0; begin\n' + 'X := X + 1;\n' * 2000 + 'end Large;')
        process = subprocess.Popen([str(args.adac), '-o', '-', str(large)], env=environment,
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        processes.append(process)
        if not process.stdout.read(64):
            raise RuntimeError('compiler did not reach emission for source snapshot control')
        with large.open('a') as stream:
            stream.write('\n-- edited while emitting\n')
        _, errors = process.communicate(timeout=30)
        if process.returncode != 1 or b'source changed while compiling' not in errors:
            raise RuntimeError('changed parsed source was not rejected: ' + errors.decode())
        # Search shadowing must invalidate provenance even for identical text.
        early, late = work / 'early', work / 'late'
        early.mkdir()
        late.mkdir()
        write(late / 'data.ads', 'package Data is N : constant Integer := 7; end Data;')
        write(source, '''with Ada.Text_IO; with Data;
procedure Root is begin Ada.Text_IO.Put_Line (Integer'Image (Data.N)); end Root;''')
        environment['ADA_INCLUDE_PATH'] = str(early) + os.pathsep + str(late)
        build()
        output(7)
        previous = (objects / 'root.ali').read_bytes()
        shutil.copy2(late / 'data.ads', early)
        rebuilt = build()
        if 'root needs no compiling' in rebuilt.stderr or previous == (objects / 'root.ali').read_bytes():
            raise RuntimeError('identical source shadowing was not tracked')
        (early / 'data.ads').unlink()
        build()
        output(7)
        (late / 'data.ads').unlink()
        run([args.ada, '-D', objects, '-o', program, source], diagnostic='cannot find the unit')
        output(7)
        root(8)
        build()
        output(8)
        if 'unit data\n' in manifest.read_text():
            raise RuntimeError('a removed dependency remains in the link manifest')
        # Target subfolders are not additional scan roots during -c replay.
        write(late / 'model.ads', 'package Model is end Model;')
        write(late / 'model/child.ads', 'package Model.Child is function Value return Integer; end Model.Child;')
        write(late / 'model/child.adb', '''with Data; package body Model.Child is
function Value return Integer is begin return Data.N; end Value; end Model.Child;''')
        write(late / 'data.ads', 'package Data is N : constant Integer := 9; end Data;')
        write(late / 'model/data.ads', 'package Data is N : constant Integer := 99; end Data;')
        write(source, '''with Ada.Text_IO; with Model.Child;
procedure Root is begin Ada.Text_IO.Put_Line (Integer'Image (Model.Child.Value)); end Root;''')
        build()
        output(9)
        # Different programs may share a cache directory; each gets its own
        # complete executable before another invocation replaces the cache.
        environment.pop('ADA_INCLUDE_PATH')
        jobs = []
        for i in range(4):
            entry = work / f'parallel-{i}/root.adb'
            root(10 + i, entry)
            executable = work / (f'parallel-program-{i}' + args.exe_suffix)
            log = work / f'parallel-{i}.log'
            process = start([args.ada, '-D', objects, '-o', executable, entry], log)
            jobs.append((process, log, executable, 10 + i))
        for process, log, executable, value in jobs:
            finish(process, log)
            output(value, executable)
        print('atomic publication, crash recovery, dependency sets, provenance and shared writers passed')
    finally:
        for pid in hooks:
            try:
                os.kill(pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
        for process in processes:
            if process.poll() is None:
                process.kill()
                process.wait(timeout=10)


if __name__ == '__main__':
    main()
