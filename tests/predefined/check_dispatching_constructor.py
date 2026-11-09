"""Reject malformed replacements before enabling predefined constructor lowering."""

import argparse
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--adac', required=True, type=Path)
parser.add_argument('--work', required=True, type=Path)
args = parser.parse_args()
args.work.mkdir(parents=True, exist_ok=True)
root = Path(__file__).resolve().parents[2]
spec = (root / 'runtime/ada/ada/tags/generic_dispatching_constructor.ads').read_text()
main = args.work / 'profile.adb'
main.write_text('''with Ada.Tags.Generic_Dispatching_Constructor;
procedure Profile is
    type Parameters is null record;
    package Model is
        type Root is abstract tagged null record;
        function Make (Params : not null access Parameters) return Root is abstract;
    end Model;
    function Create is new Ada.Tags.Generic_Dispatching_Constructor
        (Model.Root, Parameters, Model.Make);
begin
    null;
end Profile;
''')
for name, original, replacement in [
        ('tag', 'The_Tag : Tag', 'The_Tag : Integer'),
        ('constant', 'The_Tag : Tag; Params : not null access Parameters',
         'The_Tag : Tag; Params : not null access constant Parameters'),
        ('result', "return T'Class", 'return Tag')]:
    directory = args.work / name
    directory.mkdir(exist_ok=True)
    source = directory / 'ada-tags-generic_dispatching_constructor.ads'
    assert spec.count(original) == 1
    source.write_text(spec.replace(original, replacement))
    result = subprocess.run([str(args.adac.resolve()), '-o', str(directory / 'out.ssa'),
                             str(source), str(main)], capture_output=True, text=True, timeout=30)
    expected = f'{main}:8:5: error: invalid predefined Generic_Dispatching_Constructor profile\n'
    assert result.returncode == 1 and not result.stdout and result.stderr == expected, (name, result)
