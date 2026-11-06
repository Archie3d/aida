"""Check source-backed System loading, bootstrap identity and cache dependencies."""

import argparse
from pathlib import Path
import shutil
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--ada', required=True, type=Path)
parser.add_argument('--adac', required=True, type=Path)
parser.add_argument('--source', required=True, type=Path)
parser.add_argument('--work', required=True, type=Path)
parser.add_argument('--exe-suffix', default='')
args = parser.parse_args()
work = args.work.resolve()
if work.exists():
    shutil.rmtree(work)
work.mkdir(parents=True)
custom = work / 'runtime'
custom.mkdir()
spec = custom / 'system.ads'
original = args.source.read_text()
source = work / 'systemsource.adb'
source.write_text('''with Ada.Text_IO;
with System.Storage_Elements;
procedure SystemSource is
    use type System.Address;
    use type System.Storage_Elements.Integer_Address;
    function Compare_Memory (Left, Right : System.Address; Count : Long_Integer) return Integer;
    pragma Import (C, Compare_Memory, "memcmp");
    Expected : aliased Integer := System.Default_Priority;
    Object : aliased Integer := 42;
    Address : constant System.Address := Object'Address;
begin
    if System.Storage_Elements.To_Address
        (System.Storage_Elements.To_Integer (Address)) /= Address then
        raise Program_Error;
    end if;
    if Compare_Memory (Expected'Address, System.Default_Priority'Address,
                       Integer'Size / System.Storage_Unit) /= 0 then
        raise Program_Error;
    end if;
    Ada.Text_IO.Put_Line (Integer'Image (System.Default_Priority));
end SystemSource;
''')


def run(command, expected=0):
    result = subprocess.run(list(map(str, command)), cwd=work, capture_output=True,
                            text=True, timeout=60)
    assert result.returncode == expected, (command, result.returncode, result.stdout, result.stderr)
    return result


program = work / ('systemsource' + args.exe_suffix)
# Reuse the same output/cache while changing a static constant in a dependency.
for priority in (7, 9):
    spec.write_text(original.replace("(Priority'First + Priority'Last) / 2", str(priority)))
    run([args.ada.resolve(), '-o', program, spec, source])
    assert run([program]).stdout == f' {priority}\n'
    manifest = (work / 'systemsource.adaobj/units.manifest').read_text()
    assert 'system.spec' in manifest, manifest
    assert str(spec) in manifest, manifest

# A malformed intrinsic declaration must be rejected at the actual source.
spec.write_text(original.replace('type Address is private;', 'type Address is range 0 .. 10;'))
result = run([args.adac.resolve(), '-o', work / 'bad.ssa', spec, source], expected=1)
assert str(spec) in result.stderr and 'System.Address requires the compiler-completed' in result.stderr, result.stderr

spec.write_text(original.replace('    type Address is private;\n', ''))
result = run([args.adac.resolve(), '-o', work / 'missing.ssa', spec, source], expected=1)
assert str(spec) in result.stderr and 'System must declare the compiler-completed Address type' in result.stderr, result.stderr

# Ordinary semantic errors in the specification must not be hidden by bootstrap values.
spec.write_text(original.replace('Default_Priority : constant Priority :=', 'Default_Priority : constant Name :='))
result = run([args.adac.resolve(), '-o', work / 'bad-profile.ssa', spec, source], expected=1)
assert str(spec) in result.stderr and 'initial value is not compatible' in result.stderr, result.stderr

bootstrap = work / 'addressonly.adb'
bootstrap.write_text("procedure AddressOnly is X : Integer; begin if X'Address /= X'Address then raise Program_Error; end if; end AddressOnly;\n")
run([args.adac.resolve(), '--no-stdlib', '-D', work / 'bootstrap', bootstrap])
assert not (work / 'bootstrap/system.ssa').exists()
# System itself is now an ordinary source dependency, even without the stdlib path.
needs_system = work / 'needssystem.adb'
needs_system.write_text('with System; procedure NeedsSystem is begin null; end NeedsSystem;\n')
result = run([args.adac.resolve(), '--no-stdlib', '-o', work / 'missing-system.ssa', needs_system], expected=1)
assert "cannot find the unit 'System'" in result.stderr, result.stderr

print('System source, canonical Address, static dependency invalidation and source diagnostics verified')
