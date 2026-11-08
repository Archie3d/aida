"""Protocol controls for generated long and batched host measurements."""

import contextlib
import importlib.util
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
kPath = Path(__file__).with_name('measure.py')
spec = importlib.util.spec_from_file_location('measure', kPath)
measure = importlib.util.module_from_spec(spec)
spec.loader.exec_module(measure)


class MeasurementTests(unittest.TestCase):
    def testLongWrapperKeepsContextAndKernelAndHasMatchingUnitFilename(self):
        original = 'with Ada.Text_IO;\nprocedure Probe is\nbegin\n    null;\nend Probe;\n'
        name, text = measure.longSource(original, 32)
        self.assertEqual(name, 'probelong.adb')
        self.assertTrue(text.startswith('with Ada.Text_IO;\nprocedure ProbeLong is\n'))
        self.assertIn('    procedure Probe is\n    begin\n        null;\n    end Probe;', text)
        self.assertIn('for Trial in 1 .. 32 loop\n        Probe;', text)
        self.assertTrue(text.endswith('end ProbeLong;\n'))

    def testMalformedKernelCannotSilentlyProduceAnotherWorkload(self):
        with self.assertRaises(ValueError):
            measure.longSource('procedure Probe is begin null; end Other;', 32)

    def testShortCasesRetainTheOriginalSourcesAndDoNotGenerateFiles(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            cases = measure.prepareCases(root, 'short', 32, [32], 1024, 64)
            self.assertEqual([name for name, _, _ in cases], measure.kWorkloads)
            self.assertTrue(all(source.parent == measure.kSources for _, source, _ in cases))
            self.assertEqual(list(root.iterdir()), [])

    def testBatchesHoldAllocationCountConstantAcrossLiveSetSizes(self):
        with tempfile.TemporaryDirectory(prefix='measurement sources ') as temporary:
            cases = measure.prepareCases(Path(temporary), 'extended', 2, [4, 16], 64, 64)
            self.assertEqual(len(cases), 18)
            batches = [entry for _, _, entry in cases if entry['kind'] == 'batch']
            self.assertEqual(len(batches), 9)
            self.assertEqual({entry['parameters']['order'] for entry in batches},
                             {'fifo', 'lifo', 'permuted'})
            for entry in batches:
                parameters = entry['parameters']
                self.assertEqual(parameters['allocations'], 64)
                self.assertEqual(parameters['live_objects'] * parameters['rounds'], 64)
                self.assertTrue(parameters['heap_pointer_table'])
                self.assertNotIn('@', entry['generated_source'])
            for _, source, entry in cases:
                if 'generated_source' in entry:
                    self.assertEqual(source.read_bytes(), entry['generated_source'].encode('utf-8'))

    def testUnsafeOrIncomparableConfigurationsFailBeforeCompiling(self):
        invalid = [['--repetitions', '0'], ['--repetitions', '1025'],
                   ['--batch-sizes', '0'], ['--batch-sizes', '3'], ['--batch-sizes', '16384'],
                   ['--batch-sizes', '32', '32'], ['--batch-objects', '0'],
                   ['--batch-objects', '33'], ['--batch-objects', '1048577'],
                   ['--throughput-objects', '0'], ['--throughput-objects', '33'],
                   ['--throughput-objects', '8388609'], ['--runs', '2']]
        for options in invalid:
            with self.subTest(options=options), patch.object(sys, 'argv',
                    ['measure', '--ada', 'unused', '--work', 'unused'] + options):
                with contextlib.redirect_stderr(io.StringIO()), patch.object(measure.subprocess, 'run') as run:
                    with self.assertRaises(SystemExit) as error:
                        measure.main()
                    self.assertEqual(error.exception.code, 2)
                    run.assert_not_called()

    def testFailedOrNoisyExecutionsCannotBecomeTimingSamples(self):
        for code, stderr in [(1, ''), (0, 'unexpected warning')]:
            with patch.object(measure.subprocess, 'run', return_value=
                    subprocess.CompletedProcess([], code, '', stderr)):
                with self.assertRaises(RuntimeError):
                    measure.command(['program'], '.', {})

    def testNativeExecutionRetainsItsFiniteDeadline(self):
        with patch.object(measure.subprocess, 'run', side_effect=
                subprocess.TimeoutExpired(['program'], 60)) as run:
            with self.assertRaises(subprocess.TimeoutExpired):
                measure.command(['program'], '.', {})
            self.assertEqual(run.call_args.kwargs['timeout'], 60)


if __name__ == '__main__':
    unittest.main()
