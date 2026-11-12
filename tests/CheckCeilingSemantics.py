"""Check configuration propagation and compiled locking-policy consistency."""
import argparse
import os
from pathlib import Path
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--adac', type=Path, required=True)
parser.add_argument('--work', type=Path, required=True)
parser.add_argument('--probe', type=Path, required=True)
parser.add_argument('--cmake', type=Path, required=True)
parser.add_argument('--mode-runner', type=Path, required=True)
args = parser.parse_args()
compiler = args.adac.resolve()
work = args.work.resolve()
work.mkdir(parents=True, exist_ok=True)


def run(*command, diagnostic=None):
    result = subprocess.run([str(compiler), *map(str, command)], capture_output=True, text=True, timeout=20)
    if diagnostic is None:
        assert result.returncode == 0, (command, result.returncode, result.stdout, result.stderr)
    else:
        assert result.returncode == 1 and diagnostic in result.stderr, (command, result.returncode, result.stderr)
    return result


def source(name, text):
    path = work / name
    path.write_text(text, encoding='utf-8')
    return path


cases = [
    ('trailing', 'procedure Root is begin null; end Root; pragma Locking_Policy (Ceiling_Locking);', 'must precede the first compilation unit'),
    ('unknown', 'pragma Locking_Policy (Other_Locking); procedure Root is begin null; end Root;', 'unsupported locking policy'),
    ('nested', 'procedure Root is pragma Locking_Policy (Ceiling_Locking); begin null; end Root;', 'only allowed as a configuration pragma'),
    ('misplaced', 'procedure Root is pragma Interrupt_Priority; begin null; end Root;', 'requires a task or protected definition'),
    ('duplicate', 'procedure Root is task T is pragma Priority (15); pragma Interrupt_Priority; end T; task body T is begin null; end T; begin null; end Root;', 'duplicate Interrupt_Priority pragma'),
    ('type', 'procedure Root is task T is pragma Interrupt_Priority (1.0); end T; task body T is begin null; end T; begin null; end Root;', 'integer'),
    ('argument', 'procedure Root is task T is pragma Interrupt_Priority (value => 31); end T; task body T is begin null; end T; begin null; end Root;', 'unknown pragma argument name'),
    ('policy_conflict', 'pragma Locking_Policy (Native_Locking); pragma Locking_Policy (Ceiling_Locking); procedure Root is begin null; end Root;', 'conflicting Locking_Policy'),
]
for name, text, diagnostic in cases:
    run('-o', work / f'{name}.ssa', source(f'{name}.adb', text), diagnostic=diagnostic)

root = source('root.adb', 'with PolicyModel; procedure Root is begin null; end Root;')
model_spec = source('policymodel.ads', 'pragma Locking_Policy (Ceiling_Locking); package PolicyModel is end PolicyModel;')
whole = work / 'whole.ssa'
run('-o', whole, root)
assert whole.read_text().count('call $__ada_locking_policy(w 1)') == 1
objects = work / 'objects'
run('-D', objects, root)
manifest_path = objects / 'units.manifest'
manifest = manifest_path.read_text()
assert 'locking-policy 1\n' in manifest
for path in dict.fromkeys(re.findall(r'^source (.+)$', manifest, re.M)):
    run('-c', '-D', objects, path)
assert 'locking-policy 1\n' in (objects / 'policymodel.ali').read_text()
run('--bind', 'root', '-D', objects)
assert 'call $__ada_locking_policy(w 1)' in (objects / '__ada_binder.ssa').read_text()
manifest_path.write_text(manifest.replace('locking-policy 1\n', 'locking-policy 2\n'))
run('--bind', 'root', '-D', objects, diagnostic='missing or stale compiled unit')
manifest_path.write_text(manifest)

# A source-free dependency retains the policy in its checked semantic interface.
client = source('client.adb', 'with PolicyModel; procedure Client is begin null; end Client;')
model_spec.unlink()
run('-c', '-I', objects, '-D', work / 'client-objects', client)

# Independently compiled units can conflict only when their partitions are joined.
root = source('root.adb', 'pragma Locking_Policy (Ceiling_Locking); procedure Root is begin null; end Root;')
model_spec = source('policymodel.ads', 'pragma Locking_Policy (Native_Locking); package PolicyModel is end PolicyModel;')
joined = work / 'joined'
run('--scan', '-D', joined, root)
root_manifest = (joined / 'units.manifest').read_text()
for path in dict.fromkeys(re.findall(r'^source (.+)$', root_manifest, re.M)):
    run('-c', '-D', joined, path)
run('--scan', '-D', work / 'native-scan', model_spec)
native_manifest = (work / 'native-scan' / 'units.manifest').read_text()
run('-c', '-D', joined, model_spec)
block = re.search(r'^unit policymodel\n.*?^end\n', native_manifest, re.M | re.S).group()
parts = ''.join(f'elaborate {name}\n' for name in re.findall(r'^part (.+)$', block, re.M))
(joined / 'units.manifest').write_text(root_manifest.replace('end-manifest\n', parts + block + 'end-manifest\n'))
run('--bind', 'root', '-D', joined, diagnostic='conflicting Locking_Policy')

# Policies from subunits belong to the containing compiled unit.
root = source('root.adb', 'procedure Root is procedure Child is separate; begin Child; end Root;')
child = source('root-child.adb', 'pragma Locking_Policy (Ceiling_Locking); separate (Root) procedure Child is begin null; end Child;')
run('-o', whole, root)
assert 'call $__ada_locking_policy(w 1)' in whole.read_text()
root.write_text('pragma Locking_Policy (Native_Locking); procedure Root is procedure Child is separate; begin Child; end Root;')
run('-o', whole, root, diagnostic='conflicting Locking_Policy settings in subunit')

# Native unavailability skips before any compiler invocation; malformed probe
# responses are failures even if their text resembles an unavailable result.
for mode in ('unavailable', 'fail', 'noisy'):
    environment = dict(os.environ, AIDA_TEST_PRIORITY_PROBE=mode)
    result = subprocess.run([str(args.cmake), f'-DPRIORITY_PROBE={args.probe}', '-P', str(args.mode_runner)],
                            capture_output=True, text=True, timeout=10, env=environment)
    if mode == 'unavailable':
        assert result.returncode == 0 and result.stdout.startswith('-- SKIP native interrupt priority unavailable:'), result
    else:
        assert result.returncode != 0 and 'Native interrupt priority prerequisite failed' in result.stderr, result
        assert not result.stdout.startswith('-- SKIP native interrupt priority unavailable:'), result
