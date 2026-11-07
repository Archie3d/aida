#!/usr/bin/env python3
"""Exercise a relocated install using independent sources and spaced paths."""

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile


def run(command, directory, environment, expected=None):
    result = subprocess.run(list(map(str, command)), cwd=directory, env=environment,
                            capture_output=True, text=True, timeout=60)
    if result.returncode != 0 or (expected is not None and (result.stdout != expected or result.stderr)):
        raise RuntimeError(f"command failed: {command}\n{result.returncode}\n{result.stdout}{result.stderr}")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cmake', required=True)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--libdir', required=True)
    args = parser.parse_args()
    if Path(args.libdir).anchor or '..' in Path(args.libdir).parts:
        parser.error('relocation validation requires a relative install libdir within the prefix')
    cache = (args.build / 'CMakeCache.txt').read_text()
    for line in cache.splitlines():
        if line.startswith(('CMAKE_INSTALL_BINDIR:', 'CMAKE_INSTALL_LIBDIR:')):
            destination = Path(line.split('=', 1)[1])
            if destination.anchor or '..' in destination.parts:
                parser.error('installation destinations must stay within the temporary prefix')
    args.work.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    for key in ['ADAC', 'QBE', 'CC', 'ADA_LIBRARY', 'ADA_RUNTIME', 'ADA_INCLUDE_PATH']:
        environment.pop(key, None)
    with tempfile.TemporaryDirectory(prefix='installed toolchain ', dir=args.work.resolve()) as temporary:
        root = Path(temporary)
        original, prefix = root / 'original install', root / 'relocated install'
        run([args.cmake, '--install', args.build.resolve(), '--prefix', original], root, environment)
        original.rename(prefix)
        assert not original.exists()
        library = prefix / args.libdir / 'ada/adainclude'
        system = library / 'system.ads'
        system.write_text(system.read_text().replace('end System;',
                          '    Relocation_Probe : constant := 73;\nend System;'))
        source = root / 'source with spaces'
        source.mkdir()
        dependencies = root / 'dependency library with spaces'
        dependencies.mkdir()
        (dependencies / 'helper.ads').write_text('package Helper is\n    Value : constant Integer := 9;\nend Helper;\n')
        main_source = source / 'probe.adb'
        main_source.write_text('''with Ada.Text_IO;
with System;
with Helper;
procedure Probe is
begin
    if Helper.Value /= 9 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line ("installed" & Integer'Image (System.Relocation_Probe));
end Probe;
''')
        environment['PATH'] = str(prefix / 'bin') + os.pathsep + environment.get('PATH', '')
        environment['ADA_INCLUDE_PATH'] = str(root / 'missing directory') + os.pathsep + str(dependencies)
        suffix = '.exe' if os.name == 'nt' else ''
        driver, frontend = prefix / ('bin/ada' + suffix), prefix / ('bin/adac' + suffix)
        logs = []
        for invocation, name in [(driver, 'absolute output'), ('ada', 'path output')]:
            program = root / (name + suffix)
            built = run([invocation, '-v', '-o', program, main_source], root, environment)
            # The copied System spec makes fallback to build-tree sources fail.
            # Verbose commands also prove both companion programs and archive
            # were selected from this install while the build tree still exists.
            log = built.stderr.replace('\\', '/').casefold()
            for selected in [str(prefix / 'bin/adac'), str(prefix / 'bin/qbe'),
                             str(prefix / ('bin/../' + args.libdir + '/ada/adalib/libadart.a'))]:
                if selected.replace('\\', '/').casefold() not in log:
                    raise RuntimeError('relocated toolchain component not used: ' + selected + '\n' + built.stderr)
            logs.append(built.stderr)
            run([program], root, environment, 'installed 73\n')
        for invocation, name in [(frontend, 'absolute frontend'), ('adac', 'path frontend')]:
            output = root / (name + '.ssa')
            run([invocation, '-o', output, main_source], root, environment)
            if not output.is_file() or output.stat().st_size == 0:
                raise RuntimeError('standalone frontend did not emit IR')
        # Environment overrides must retain native list syntax, drive letters
        # and spaces. Reusing the emitted main also checks source discovery.
        override = root / 'override library'
        shutil.copytree(library, override)
        system = override / 'system.ads'
        system.write_text(system.read_text().replace(':= 73;', ':= 97;'))
        archive = root / 'override runtime' / 'libadart.a'
        archive.parent.mkdir()
        shutil.copy2(prefix / args.libdir / 'ada/adalib/libadart.a', archive)
        overridden = dict(environment, ADA_LIBRARY=str(override), ADA_RUNTIME=str(archive))
        program = root / ('override output' + suffix)
        built = run([driver, '-v', '-o', program, main_source], root, overridden)
        if str(archive).replace('\\', '/').casefold() not in built.stderr.replace('\\', '/').casefold():
            raise RuntimeError('runtime override was not used: ' + built.stderr)
        logs.append(built.stderr)
        run([program], root, overridden, 'installed 97\n')
        if os.name == 'posix':
            link = root / 'linked tools'
            link.mkdir()
            for name in ['ada', 'adac']:
                (link / name).symlink_to(prefix / 'bin' / name)
            linked = dict(environment, PATH=str(link) + os.pathsep + environment['PATH'])
            program = root / 'symlink output'
            run(['ada', '-o', program, main_source], root, linked)
            run([program], root, linked, 'installed 73\n')
            run(['adac', '-o', root / 'symlink.ssa', main_source], root, linked)
        (args.work / 'install.log').write_text('\n'.join(logs))
    (args.work / 'result.json').write_text(json.dumps(dict(
        outcome='PASS', native_paths=os.name, libdir=args.libdir,
        checks=['relocation', 'absolute and PATH invocation', 'standalone frontend',
                'spaced source/output/install paths', 'ADA_INCLUDE_PATH', 'library and runtime overrides',
                *(['symlink invocation'] if os.name == 'posix' else [])]), indent=2) + '\n')
    print('installed toolchain validation passed')


if __name__ == '__main__':
    main()
