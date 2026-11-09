"""Verify no-op process elision, invalidation, all file modes and source-free clean."""

import argparse
import os
from pathlib import Path
import shutil
import time
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ['ada', 'adac', 'cc', 'ar', 'ranlib', 'qbe', 'runtime', 'hook', 'work']:
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--exe-suffix', default='')
    args = parser.parse_args()
    for name in ['ada', 'adac', 'cc', 'ar', 'ranlib', 'qbe', 'runtime', 'hook', 'work']:
        setattr(args, name, getattr(args, name).resolve())
    work = args.work
    if work.exists():
        shutil.rmtree(work)
    work.mkdir(parents=True)
    env = os.environ.copy()
    for key in ['ADAC', 'QBE', 'CC', 'ADA_INCLUDE_PATH', 'ADA_LIBRARY', 'ADA_RUNTIME']:
        env.pop(key, None)
    tools = work / 'tools'
    tools.mkdir()
    proxy = tools / ('frontend' + args.exe_suffix)
    shutil.copy2(args.hook, proxy)
    trace = work / 'frontend.trace'
    env.update(ADAC=str(proxy), AIDA_TEST_REAL_TOOL=str(args.adac), AIDA_TEST_TRACE=str(trace))
    source = work / 'app/root.adb'
    source.parent.mkdir()
    early, late = work / 'early', work / 'late'
    early.mkdir()
    late.mkdir()
    env['ADA_INCLUDE_PATH'] = str(early) + os.pathsep + str(late)
    dependency = late / 'data.ads'
    dependency.write_text('package Data is N : constant Integer := 1; end Data;\n')
    source.write_text('''with Ada.Text_IO; with Data;
procedure Root is begin Ada.Text_IO.Put_Line (Integer'Image (Data.N)); end Root;
''')
    objects = work / 'objects'
    executable = work / ('program' + args.exe_suffix)
    command = [args.ada, '-v', '-D', objects, '-o', executable, source]

    def run(arguments, environment=None, diagnostic=None, cwd=work):
        result = subprocess.run(list(map(str, arguments)), env=env if environment is None else environment,
                                cwd=cwd, capture_output=True, text=True, timeout=60)
        if diagnostic is None:
            if result.returncode != 0:
                raise RuntimeError(f'{arguments}\n{result.returncode}\n{result.stdout}{result.stderr}')
        elif result.returncode == 0 or diagnostic not in result.stderr:
            raise RuntimeError(f'expected {diagnostic}: {arguments}\n{result.returncode}\n{result.stdout}{result.stderr}')
        return result

    def output(value):
        result = run([executable])
        if result.stdout != f' {value}\n' or result.stderr:
            raise RuntimeError('stale executable: ' + result.stdout + result.stderr)

    def noop(arguments=command, environment=None):
        before = trace.read_bytes() if trace.exists() else b''
        stamps = {path: (path.stat().st_mtime_ns, path.stat().st_ino)
                  for path in objects.rglob('*') if path.is_file()}
        if executable.exists():
            stamps[executable] = (executable.stat().st_mtime_ns, executable.stat().st_ino)
        result = run(arguments, environment)
        if 'no-op build' not in result.stderr or '--scan' in result.stderr or '--bind' in result.stderr:
            raise RuntimeError('unchanged invocation did not bypass all tools: ' + result.stderr)
        after = trace.read_bytes() if trace.exists() else b''
        if before != after:
            raise RuntimeError('frontend ran during a no-op build')
        for path, stamp in stamps.items():
            if not path.exists() or (path.stat().st_mtime_ns, path.stat().st_ino) != stamp:
                raise RuntimeError('no-op rewrote a product: ' + str(path))

    def rebuild(arguments=command, environment=None):
        result = run(arguments, environment)
        if 'no-op build' in result.stderr:
            raise RuntimeError('changed build was incorrectly skipped')
        return result

    rebuild()
    output(1)
    noop()
    # Content identity, not mtime, decides whether source input changed.
    os.utime(source, None)
    os.utime(dependency, None)
    noop()
    rebuild([*command, '--force'])
    noop()
    # A higher-priority candidate with identical text changes lookup provenance.
    shutil.copy2(dependency, early)
    rebuild()
    output(1)
    noop()
    (early / 'data.ads').write_text('package Data is N : constant Integer := 2; end Data;\n')
    rebuild()
    output(2)
    (early / 'data.ads').unlink()
    rebuild()
    output(1)
    dependency.unlink()
    run(command, diagnostic='cannot find the unit')
    output(1)
    dependency.write_text('package Data is N : constant Integer := 3; end Data;\n')
    rebuild()
    output(3)
    # An unrelated directory entry is not a unit lookup dependency.
    (early / 'unrelated.txt').write_text('not a source candidate\n')
    noop()
    # Changing flags, environment or explicit argument order invalidates the receipt.
    changed_env = dict(env, AIDA_NOOP_TEST_SETTING='changed')
    rebuild(environment=changed_env)
    noop(environment=changed_env)
    rebuild()
    noop()
    # Complete frontend/native/binder artifacts and output bytes are all covered.
    for name in ['root.ali', 'root.asi', '.aida-interfaces/root.body.asi', 'root.o',
                 'root.build', '__ada_binder.ssa', '__ada_binder.o', 'units.manifest', 'build.receipt']:
        path = objects / name
        path.write_bytes(b'corrupt')
        rebuild()
        output(3)
        noop()
    executable.unlink()
    rebuild()
    output(3)
    executable.write_bytes(b'corrupt executable')
    rebuild()
    output(3)
    noop()
    # An artifact changed during successful linking must not become the baseline
    # of a valid receipt. The next build repairs it before permitting a no-op.
    link_ready, link_release = work / 'link.ready', work / 'link.release'
    link_env = dict(env, ADAC=str(args.adac), CC=str(args.hook), AIDA_TEST_REAL_TOOL=str(args.cc),
                    AIDA_TEST_MODE='pause', AIDA_TEST_SELECT=executable.name,
                    AIDA_TEST_READY=str(link_ready), AIDA_TEST_RELEASE=str(link_release))
    with (work / 'held-link.log').open('w') as log:
        linking = subprocess.Popen(list(map(str, command)), env=link_env, cwd=work,
                                   stdout=log, stderr=subprocess.STDOUT)
    try:
        deadline = time.monotonic() + 30
        while not link_ready.exists() and time.monotonic() < deadline:
            if linking.poll() is not None:
                raise RuntimeError('held link stopped before artifact corruption control')
            time.sleep(0.01)
        if not link_ready.exists():
            raise RuntimeError('held build did not reach linking')
        (objects / '.aida-interfaces/root.body.asi').write_bytes(b'corrupted while linking')
        link_release.touch()
        if linking.wait(timeout=60) != 0:
            raise RuntimeError('held link failed: ' + (work / 'held-link.log').read_text())
        if 'artifacts changed before receipt publication' not in (work / 'held-link.log').read_text():
            raise RuntimeError('successful link accepted a corrupt frontend product')
        rebuild(environment=link_env)
        output(3)
        noop(environment=link_env)
    finally:
        link_release.touch()
        if linking.poll() is None:
            linking.kill()
            linking.wait(timeout=10)
    # Runtime archives and ordered external object/archive inputs are link inputs.
    c_source = work / 'extra.c'
    external = work / 'extra.o'
    c_source.write_text('int extra(void) { return 1; }\n')
    run([args.cc, '-c', c_source, '-o', external])
    linked = [*command, external]
    rebuild(linked)
    noop(linked)
    c_source.write_text('int extra(void) { return 2; }\n')
    run([args.cc, '-c', c_source, '-o', external])
    rebuild(linked)
    noop(linked)
    other_source, other = work / 'other.c', work / 'other.o'
    other_source.write_text('int other(void) { return 4; }\n')
    run([args.cc, '-c', other_source, '-o', other])
    ordered = [*command, external, other]
    rebuild(ordered)
    noop(ordered)
    reordered = [*command, other, external]
    rebuild(reordered)
    noop(reordered)
    runtime = work / 'libadart-copy.a'
    shutil.copy2(args.runtime, runtime)
    runtime_env = dict(env, ADA_RUNTIME=str(runtime))
    rebuild(environment=runtime_env)
    noop(environment=runtime_env)
    runtime_source, runtime_object = work / 'runtime-extra.c', work / 'runtime-extra.o'
    runtime_source.write_text('int unused_runtime_extension(void) { return 5; }\n')
    run([args.cc, '-c', runtime_source, '-o', runtime_object])
    run([args.ar, 'r', runtime, runtime_object])
    run([args.ranlib, runtime])
    rebuild(environment=runtime_env)
    noop(environment=runtime_env)
    # Replacing a selected tool at the same path is visible even without an
    # environment/configuration change. Copies retain executable signing bytes.
    replacement = tools / ('replacement' + args.exe_suffix)
    shutil.copy2(args.adac, replacement)
    os.replace(replacement, proxy)
    rebuild(linked)
    noop(linked)
    shutil.copy2(args.hook, replacement)
    os.replace(replacement, proxy)
    rebuild(command)
    noop()
    # A wrapper's actual frontend entry symlink is part of the saved snapshot.
    real1, real2 = tools / ('real1' + args.exe_suffix), tools / ('real2' + args.exe_suffix)
    shutil.copy2(args.adac, real1)
    shutil.copy2(args.adac, real2)
    entry = tools / ('entry' + args.exe_suffix)
    try:
        entry.symlink_to(real1)
    except OSError:
        pass
    else:
        aliased_env = dict(env, AIDA_TEST_REAL_TOOL=str(entry))
        rebuild(environment=aliased_env)
        noop(environment=aliased_env)
        entry.unlink()
        entry.symlink_to(real2)
        rebuild(environment=aliased_env)
        noop(environment=aliased_env)
        rebuild()
    # File-only IR and assembly modes have their own input snapshots.
    for flags, suffix in [(['--emit-ir'], '.ssa'), (['-S'], '.s'), (['-S', '-k'], '.kept.s')]:
        target = work / ('whole' + suffix)
        invocation = [args.ada, '-v', *flags, '-D', objects, '-o', target, source]
        rebuild(invocation)
        noop(invocation)
        target.unlink()
        rebuild(invocation)
        noop(invocation)
    same_directory = [args.ada, '-v', '-S', '-D', objects, '-o', objects / 'whole.s', source]
    rebuild(same_directory)
    noop(same_directory)
    # Stdout remains a stream and is emitted on every invocation.
    streamed = [args.ada, '--emit-ir', '-o', '-', source]
    first, second = run(streamed), run(streamed)
    if not first.stdout or first.stdout != second.stdout:
        raise RuntimeError('stdout IR was cached or changed')
    # Cleanup does not locate or execute tools, accepts explicit selectors, and
    # removes only the selected artifact directory (not source/executable files).
    rebuild()
    unavailable = dict(env, ADAC=str(work / 'missing-adac'), QBE=str(work / 'missing-qbe'),
                       CC=str(work / 'missing-cc'), ADA_RUNTIME=str(work / 'missing-runtime'))
    # Source-free cleanup waits for the same lock as an active build.
    assembly_target = work / 'held-assembly.s'
    for file_mode in [False, True]:
        ready, release = work / f'clean-{file_mode}.ready', work / f'clean-{file_mode}.release'
        held = dict(env, ADAC=str(args.adac), CC=str(args.hook), AIDA_TEST_REAL_TOOL=str(args.cc),
                    AIDA_TEST_MODE='pause', AIDA_TEST_SELECT='root.o',
                    AIDA_TEST_READY=str(ready), AIDA_TEST_RELEASE=str(release))
        invocation = command
        if file_mode:
            rebuild()
            invocation = [args.ada, '-v', '-S', '-D', objects, '-o', assembly_target, source]
            held.update(CC=str(args.cc), QBE=str(args.hook), AIDA_TEST_REAL_TOOL=str(args.qbe),
                        AIDA_TEST_SELECT=assembly_target.name)
        with (work / 'held-build.log').open('w') as log:
            building = subprocess.Popen(list(map(str, invocation)), env=held, cwd=work,
                                        stdout=log, stderr=subprocess.STDOUT)
        cleaning = None
        hook_pid = None
        try:
            deadline = time.monotonic() + 30
            while time.monotonic() < deadline:
                if ready.exists() and ready.read_text().splitlines():
                    hook_pid = int(ready.read_text().splitlines()[0])
                    break
                if building.poll() is not None:
                    raise RuntimeError('held build stopped before cleanup coordination control')
                time.sleep(0.01)
            if hook_pid is None:
                raise RuntimeError('held build did not reach native compilation')
            cleaning = subprocess.Popen([str(args.ada), '--clean', '-D', str(objects)], env=unavailable,
                                        cwd=work, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            time.sleep(0.2)
            if cleaning.poll() is not None:
                raise RuntimeError('cleanup entered a live build directory')
            release.touch()
            if building.wait(timeout=60) != 0:
                raise RuntimeError('held build failed: ' + (work / 'held-build.log').read_text())
            hook_pid = None
            _, errors = cleaning.communicate(timeout=30)
            if cleaning.returncode != 0:
                raise RuntimeError('concurrent cleanup failed: ' + errors.decode())
        finally:
            if hook_pid is not None:
                import signal
                try:
                    os.kill(hook_pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
            for process in [building, cleaning]:
                if process is not None and process.poll() is None:
                    process.kill()
                    process.wait(timeout=10)
        if objects.exists() or not executable.exists() or not source.exists():
            raise RuntimeError('cleanup deleted the wrong products or retained its directory')
    if not assembly_target.exists():
        raise RuntimeError('cleanup removed assembly outside the selected directory')
    run([args.ada, '--clean', '-D', objects], unavailable)
    missing = work / 'never-created/deep/objects'
    run([args.ada, '--clean', '-D', missing], unavailable)
    if (work / 'never-created').exists():
        raise RuntimeError('cleaning a missing target created directories')
    run([args.ada, '--clean'], unavailable, diagnostic='--clean needs')
    run([args.ada, '--clean', '-D', source], unavailable, diagnostic='not a directory')
    run([args.ada, '--clean', '-D', work], unavailable, diagnostic='refusing to clean')
    run([args.ada, '--clean', '-D', work.parent], unavailable, diagnostic='refusing to clean')
    run([args.ada, '--clean', '-D', Path(work.anchor)], unavailable, diagnostic='refusing to clean')
    default = [args.ada, '-v', source]
    rebuild(default)
    run([args.ada, '--clean', '-o', 'root'], unavailable)
    if (work / 'root.adaobj').exists():
        raise RuntimeError('-o cleanup did not infer the default artifact directory')
    rebuild(default)
    run([args.ada, '--clean', source], unavailable)
    if (work / 'root.adaobj').exists():
        raise RuntimeError('source-based cleanup failed')
    real = work / 'real cache'
    real.mkdir()
    (real / 'sentinel').write_text('keep')
    link = work / 'linked cache'
    try:
        link.symlink_to(real, target_is_directory=True)
    except OSError:
        pass
    else:
        run([args.ada, '--clean', '-D', link], unavailable)
        if link.exists() or not (real / 'sentinel').exists():
            raise RuntimeError('cleanup followed a directory symlink')
        dangling = work / 'dangling cache'
        dangling.symlink_to(work / 'missing link target', target_is_directory=True)
        run([args.ada, '--clean', '-D', dangling], unavailable)
        if dangling.is_symlink():
            raise RuntimeError('cleanup retained a dangling directory symlink')
    nested = work / 'nested links cache'
    nested.mkdir()
    try:
        (nested / 'external').symlink_to(real, target_is_directory=True)
        (nested / 'dangling').symlink_to(work / 'missing link target', target_is_directory=True)
    except OSError:
        pass
    else:
        run([args.ada, '--clean', '-D', nested], unavailable)
        if nested.exists() or not (real / 'sentinel').exists():
            raise RuntimeError('cleanup followed a nested directory symlink')
    if os.name == 'nt':
        # Junctions are directory reparse points and do not require symlink
        # privileges. Exercise both a selected junction and a nested one.
        junction = work / 'junction cache'
        run(['cmd', '/c', 'mklink', '/J', junction, real], unavailable)
        run([args.ada, '--clean', '-D', junction], unavailable)
        if junction.exists() or not (real / 'sentinel').exists():
            raise RuntimeError('cleanup followed a selected directory junction')
        nested.mkdir(exist_ok=True)
        run(['cmd', '/c', 'mklink', '/J', nested / 'junction', real], unavailable)
        run([args.ada, '--clean', '-D', nested], unavailable)
        if nested.exists() or not (real / 'sentinel').exists():
            raise RuntimeError('cleanup followed a nested directory junction')
    print('no-op execution, dependency/tool/artifact invalidation and source-free cleanup passed')


if __name__ == '__main__':
    main()
