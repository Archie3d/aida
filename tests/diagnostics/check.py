#!/usr/bin/env python3
"""Exact diagnostic contracts and rendering/driver controls."""
import argparse
import os
import pathlib
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--adac', required=True)
parser.add_argument('--ada', required=True)
parser.add_argument('--render', required=True)
parser.add_argument('--work', required=True)
args = parser.parse_args()
work = pathlib.Path(args.work).resolve()
work.mkdir(parents=True, exist_ok=True)
env = dict(os.environ, COLUMNS='60', TERM='xterm', NO_COLOR='1')


def run(command, status=1, overrides=None):
    result = subprocess.run([str(part) for part in command], cwd=work,
                            env=env | (overrides or {}), capture_output=True,
                            text=True, encoding='utf-8', timeout=30)
    assert result.returncode == status, (command, result.returncode, result.stdout, result.stderr)
    assert not result.stdout, result.stdout
    return result.stderr


def source(name, text):
    (work / name).write_text(text, encoding='utf-8')
    return name


def compile_file(name, *options, status=1):
    output = ['-D', work / 'units'] if '-c' in options else ['-o', work / 'out.ssa']
    return run([args.adac, '--no-stdlib', *output, *options, name], status)


# The same legal declarations exercise object, assignment and result contexts.
source('types.adb', '''procedure Types is
    type Count is range 0 .. 10;
    type Other is range 0 .. 10;
    X : Count := 1;
    Y : Other := 2;
    function F return Count is
    begin
        return Y;
    end F;
begin
    X := Y;
end Types;
''')
plain = compile_file('types.adb')
assert plain == """types.adb:8:16: error: the returned value has an incompatible type; expected 'Count', found 'Other'
types.adb:6:5: note: function is declared here
types.adb:11:10: error: the assigned value has an incompatible type; expected 'Count', found 'Other'
types.adb:4:5: note: assignment target is declared here
""", plain
assert compile_file('types.adb', '--diagnostic-format=plain', '--color=never') == plain
rich = compile_file('types.adb', '--diagnostic-format=rich')
assert '8 |         return Y;\n  |                ^\n' in rich, rich
assert '11 |     X := Y;\n   |          ^\n' in rich, rich
assert '\x1b' not in rich
(work / 'types.adb').write_text((work / 'types.adb').read_text().replace('return Y;', 'return X;').replace('X := Y;', 'X := X;'))
compile_file('types.adb', status=0)

source('initial.adb', '''procedure Initial is
    X : Integer := 1.5;
    X : Integer := 2;
begin
    null;
end Initial;
''')
initial = compile_file('initial.adb')
assert "initial.adb:2:20: error: initial value is not compatible with the declared subtype; expected 'Integer', found 'universal real'\n" in initial, initial
assert "initial.adb:3:5: error: 'X' has already been declared in this scope\ninitial.adb:2:5: note: previous declaration is here\n" in initial, initial

source('initiallegal.adb', (work / 'initial.adb').read_text()
       .replace('procedure Initial is', 'procedure InitialLegal is')
       .replace('end Initial;', 'end InitialLegal;').replace('1.5', '1')
       .replace('    X : Integer := 2;', '    Y : Integer := 2;'))
compile_file('initiallegal.adb', status=0)

# Preserve delimiters during recovery; independently malformed statements remain visible.
source('recover.adb', '''procedure Recover is
    X : Integer
    Y : Integer := ;
begin
    if True then
        null
    else
        null
    end if;
    null;
end Recover;
''')
recovery = compile_file('recover.adb')
assert recovery.count(': error:') == 4, recovery
assert 'recover.adb:3:5: error:' in recovery, recovery
assert 'recover.adb:3:20: error:' in recovery, recovery
assert 'recover.adb:7:5: error:' in recovery, recovery
assert 'recover.adb:9:5: error:' in recovery, recovery
source('recover.adb', '''procedure Recover is
    X : Integer;
    Y : Integer := 0;
begin
    if True then
        null;
    else
        null;
    end if;
    null;
end Recover;
''')
compile_file('recover.adb', status=0)

# Separately loaded declarations and generic replay retain original source identities.
source('values.ads', 'package Values is\n    Y : Boolean := True;\nend Values;\n')
source('client.adb', 'with Values;\nprocedure Client is\n    X : Integer;\nbegin\n    X := Values.Y;\nend Client;\n')
separate = compile_file('client.adb', '-c')
assert "client.adb:5:16: error: the assigned value has an incompatible type; expected 'Integer', found 'Boolean'" in separate, separate
assert 'client.adb:3:5: note: assignment target is declared here' in separate, separate
source('values.ads', 'package Values is\n    Y : Integer := 1;\nend Values;\n')
compile_file('client.adb', '-c', status=0)
source('values.ads', 'package Values is\n    Y : Integer := 1.5;\nend Values;\n')
unit_error = compile_file('client.adb', '-c', '--diagnostic-format=rich')
assert "values.ads:2:20: error: initial value is not compatible" in unit_error, unit_error
assert '2 |     Y : Integer := 1.5;' in unit_error, unit_error
source('values.ads', 'package Values is\n    Y : Integer := 1;\nend Values;\n')
source('generic_value.ads', 'generic\n    type T is private;\n    Initial : T;\npackage Generic_Value is\n    X : T := Initial;\nend Generic_Value;\n')
source('instance.adb', 'with Generic_Value;\nprocedure Instance is\n    package Bad is new Generic_Value (Integer, True);\nbegin\n    null;\nend Instance;\n')
instance = compile_file('instance.adb', '-c')
assert 'instance.adb:3:' in instance and ': error:' in instance, instance
assert 'generic' in instance.lower(), instance
(work / 'instance.adb').write_text((work / 'instance.adb').read_text().replace('Integer, True', 'Integer, 1'))
compile_file('instance.adb', '-c', status=0)

source('generic_action.ads', 'generic\n    with procedure Action is Too_Late;\npackage Generic_Action is\nend Generic_Action;\n')
source('replay.adb', 'with Generic_Action;\nprocedure Replay is\n    procedure Too_Late is null;\n    package Bad is new Generic_Action;\nbegin\n    null;\nend Replay;\n')
replay = compile_file('replay.adb', '-c')
# The generic's default is looked up at its declaration, rather than at the instance.
assert replay.endswith("generic_action.ads:2:30: error: no matching actual for generic formal 'Action'\nreplay.adb:4:5: note: in instantiation of 'Generic_Action' here\n"), replay
replay_rich = compile_file('replay.adb', '-c', '--diagnostic-format=rich')
assert '2 |     with procedure Action is Too_Late;' in replay_rich, replay_rich
assert '4 |     package Bad is new Generic_Action;' in replay_rich, replay_rich
run([args.adac, '--no-stdlib', '-c', '-D', work / 'interfaces', 'generic_action.ads'], status=0)
(work / 'generic_action.ads').rename(work / 'generic_action.hidden')
try:
    imported = compile_file('replay.adb', '-c', '-I', work / 'interfaces', '--diagnostic-format=rich')
    assert "generic_action.ads:2:30: error: no matching actual" in imported, imported
    assert 'with procedure Action is Too_Late;' not in imported, imported
    assert '4 |     package Bad is new Generic_Action;' in imported, imported
finally:
    (work / 'generic_action.hidden').rename(work / 'generic_action.ads')
(work / 'replay.adb').write_text((work / 'replay.adb').read_text().replace('new Generic_Action;', 'new Generic_Action (Too_Late);'))
compile_file('replay.adb', '-c', status=0)

# Rendering has an exact stable snapshot, independent of tty or terminal locale.
source('sample.adb', 'Changed on disk; this must not appear in the retained snapshot.\n')
limit = run([args.render, '--limit'], status=0)
assert limit.count(': error:') == 101 and limit.count(': note:') == 100, limit
assert limit.count('too many errors; further diagnostics suppressed') == 1, limit
render = run([args.render, '--diagnostic-format=rich', '--color=never'], status=0)
expected = pathlib.Path(__file__).with_name('render.expected').read_text(encoding='utf-8')
assert render == expected, render
colored = run([args.render, '--diagnostic-format=rich', '--color=always'], status=0)
assert '\x1b[1;31m' in colored and '\x1b[1;33m' in colored and '\x1b[1;36m' in colored
assert re.sub(r'\x1b\[[0-9;]*m', '', colored) == render
plain_render = run([args.render], status=0)
assert ' --> ' not in plain_render and '\x1b' not in plain_render
assert plain_render == run([args.render, '--diagnostic-format=plain'], status=0)
assert 'outer instance' not in plain_render.split('end of file')[1]
for width in ('40', '80', '240', 'garbage', '-20', '9999999999999999'):
    output = run([args.render, '--diagnostic-format=rich'], status=0, overrides={'COLUMNS': width})
    assert 'Offending_Token' in output and '\x1b' not in output, output
    for line in output.splitlines():
        if ' | ' in line:
            assert len(line) <= max(100, int(width) if width.isdigit() and len(width) < 4 else 100), line

# Frontend and driver validate and forward the same flags.
for tool in (args.adac, args.ada):
    for option in ('--color=bogus', '--diagnostic-format=bogus'):
        error = run([tool, option, 'initial.adb'], status=2)
        assert 'invalid diagnostic option' in error
    output = run([tool, '--diagnostic-format=rich', '--color=never', '-o', work / 'bad', 'initial.adb'])
    assert ' --> ' in output and '\x1b' not in output, output

# A real terminal defaults to snippets; TERM=dumb and NO_COLOR are independent.
if os.name != 'nt':
    import errno
    import pty
    import select
    import time

    def tty_output(overrides):
        master, slave = pty.openpty()
        terminal_env = dict(env)
        for key, value in overrides.items():
            if value is None:
                terminal_env.pop(key, None)
            else:
                terminal_env[key] = value
        process = subprocess.Popen([args.render], stdout=subprocess.DEVNULL, stderr=slave,
                                   env=terminal_env, cwd=work)
        os.close(slave)
        output = b''
        deadline = time.monotonic() + 10
        try:
            while True:
                assert time.monotonic() < deadline, 'terminal process timed out'
                ready, _, _ = select.select([master], [], [], 0.1)
                if not ready:
                    continue
                try:
                    data = os.read(master, 65536)
                except OSError as error:
                    if error.errno == errno.EIO:
                        break
                    raise
                if not data:
                    break
                output += data
            assert process.wait(timeout=5) == 0
        finally:
            os.close(master)
            if process.poll() is None:
                process.kill()
                process.wait()
        return output.decode().replace('\r\n', '\n')

    assert tty_output({}) == render
    assert tty_output({'TERM': 'dumb'}) == plain_render
    assert '\x1b[1;31m' in tty_output({'NO_COLOR': None})

print('diagnostics contracts passed')
