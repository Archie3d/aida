#!/usr/bin/env python3
"""Compile and execute success/failure controls for the local ACATS adapters."""

import argparse
import importlib.util
from pathlib import Path
import sys
import tempfile

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('grading', Path(__file__).with_name('grade.py'))
grading = importlib.util.module_from_spec(spec)
spec.loader.exec_module(grading)

kPositive = '''
    for Code in 0 .. 255 loop
        if Report.Ident_Char (Character'Val (Code)) /= Character'Val (Code) then
            Report.Failed ("character identity");
        end if;
    end loop;
    TCTouch.Touch ('X');
    TCTouch.Flush;
    TCTouch.Validate ("", "flush");
    TCTouch.Touch ('A');
    TCTouch.Touch ('B');
    TCTouch.Touch ('A');
    TCTouch.Validate ("BAA", "unordered multiplicity", False);
    TCTouch.Validate ("", "reset after validation");
    declare
        Shifted : String (5 .. 7) := "BAA";
    begin
        TCTouch.Touch ('A');
        TCTouch.Touch ('B');
        TCTouch.Touch ('A');
        TCTouch.Validate (Shifted, "shifted expected bounds", False);
    end;
    TCTouch.Touch ('A');
    TCTouch.Validate_One_Of ("A", "B", Message => "default alternatives");
    TCTouch.Touch ('F');
    TCTouch.Validate_One_Of ("A", "B", "C", "D", "E", "F", "sixth alternative");
    TCTouch.Validate_One_Of ("", "X", Message => "explicit empty alternative");
    TCTouch.Assert (True, "true assertion");
    TCTouch.Assert_Not (False, "false assertion");
'''
kFailures = {
    'ordered': 'TCTouch.Touch (\'A\'); TCTouch.Touch (\'B\'); TCTouch.Validate ("BA", "probe");',
    'multiplicity': 'TCTouch.Touch (\'A\'); TCTouch.Touch (\'A\'); TCTouch.Touch (\'B\'); TCTouch.Validate ("ABB", "probe", False);',
    'length': 'TCTouch.Touch (\'A\'); TCTouch.Validate ("AA", "probe", False);',
    'empty_defaults': 'TCTouch.Validate_One_Of ("X", "Y", Message => "probe");',
    'assert': 'TCTouch.Assert (False, "probe"); TCTouch.Flush;',
    'assert_not': 'TCTouch.Assert_Not (True, "probe");',
    'overflow': "for Index in 1 .. 1025 loop TCTouch.Touch ('X'); end loop; TCTouch.Flush;",
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ada', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    args.ada = args.ada.resolve(strict=True)
    args.work.mkdir(parents=True, exist_ok=True)
    support = grading.kRoot.parent / 'support'
    helpers = [support / (unit + '.' + ext) for unit in ['report', 'tctouch'] for ext in ['ads', 'adb']]
    for name, statements in dict(positive=kPositive, **kFailures).items():
        with tempfile.TemporaryDirectory(prefix=name + '-', dir=args.work) as temporary:
            work = Path(temporary).resolve()
            source, program = work / 'probe.adb', work / 'probe'
            source.write_text('with Report; with TCTouch;\nprocedure Probe is\nbegin\n'
                              '    Report.Test ("SUPPORT", "adapter controls");\n'
                              + statements + '\n    Report.Result;\nend Probe;\n')
            compiled = grading.baseline.runCommand([str(args.ada), '-o', str(program),
                                                    *map(str, helpers), str(source)], 30)
            if grading.baseline.abnormal(compiled) or compiled['exit'] != 0:
                raise ValueError(f"{name}: build failed: {compiled}")
            executed = grading.baseline.runCommand([str(program)], 5)
            if grading.baseline.abnormal(executed):
                raise ValueError(f"{name}: abnormal execution: {executed}")
            if name == 'positive':
                valid = executed['exit'] == 0 and not executed['stderr'] and executed['stdout'] == 'TEST SUPPORT\nPASSED\n'
            else:
                valid = executed['exit'] > 0 and 'FAILED ' in executed['stdout'] and 'PASSED' not in executed['stdout']
            if not valid:
                raise ValueError(f"{name}: incorrect grading: {executed}")
            print(name + ': verified')
    return 0


if __name__ == '__main__':
    sys.exit(main())
