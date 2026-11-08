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


class RuntimeProfileTests(unittest.TestCase):
    def testControlCopiesAreByteIdentical(self):
        import profile_runtime as profile
        source = 'export function $probe() {\n@start\n    ret\n}\n'
        for variant in ('control', 'control-copy'):
            self.assertEqual(profile.transform(source, variant), (source, {}))

    def testMasterExperimentRetainsRealCleanupAndExceptionBranches(self):
        import profile_runtime as profile
        source = ('    call $__ada_master_enter(l %context, l %master, w 1)\n'
                  '    call $__ada_master_activate(l %context, l %master)\n'
                  '    jnz %exception, @failed, @ok\n'
                  '    call $__ada_finalize_to(l %owner, l %mark)\n'
                  '    call $__ada_master_await(l %context, l %master)\n'
                  '    call $__ada_master_leave(l %context, l %master)\n')
        result, changes = profile.transform(source, 'no-lexical-masters')
        self.assertEqual(sum(changes.values()), 4)
        self.assertIn('call $__ada_finalize_to(l %owner, l %mark)', result)
        self.assertIn('jnz %exception, @failed, @ok', result)
        with self.assertRaises(ValueError):
            profile.transform(source.replace('w 1)', 'w 2)'), 'no-lexical-masters')

    def testGuardKeepsTheNonemptyCallAndUsesDistinctLabels(self):
        import profile_runtime as profile
        source = ('    call $__ada_finalize_to(l %owner, l %mark)\n'
                  '    call $__ada_array_rewind(l %arena, l 0)\n'
                  '    jnz %exception, @failed, @ok\n')
        result, changes = profile.transform(source, 'guard-cleanup')
        self.assertEqual(sum(changes.values()), 2)
        self.assertIn('call $__ada_finalize_to(l %owner, l %mark)', result)
        self.assertIn('call $__ada_array_rewind(l %arena, l 0)', result)
        self.assertIn('ceql %.profile.0.head, %mark', result)
        self.assertIn('ceql %.profile.1.head, 0', result)
        self.assertIn('jnz %exception, @failed, @ok', result)
        with self.assertRaises(ValueError):
            profile.transform(source.replace('l %owner, l %mark', 'w %owner, l %mark'), 'guard-cleanup')

    def testCensusFailsIfAnEntryIsMissingOrAmbiguous(self):
        import profile_runtime as profile
        source = 'void probe(int value)\n{\n    (void)value;\n}\n'
        with self.assertRaises(ValueError):
            profile.instrument(source, {'missing': '0'})
        with self.assertRaises(ValueError):
            profile.instrument(source + source, {'probe': '0'})
        self.assertIn('__ada_profile_count("probe", value == 0);',
                      profile.instrument(source, {'probe': 'value == 0'}))

    def testDiagnosticWorkCannotOverwriteSourcesOrBuildProducts(self):
        import profile_runtime as profile
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / 'repo'
            build = root / 'build'
            with patch.object(profile, 'kRoot', root):
                for work in (root, root / 'runtime', root / 'qbe', root / 'tests/host',
                             build, build / 'runtime', build / 'runtime/nested', build / 'tests/nested'):
                    with self.subTest(work=work), self.assertRaises(ValueError):
                        profile.validateWork(build, work)
                profile.validateWork(build, build / 'host performance/profiling')
                profile.validateWork(build, Path(temporary) / 'external diagnostics')

    def testInvalidProfileSettingsDoNotCompileOrWrite(self):
        import profile_runtime as profile
        for options in (['--runs', '2'], ['--runs', '4'], ['--repetitions', '0'], ['--repetitions', '1025']):
            with patch.object(sys, 'argv', ['profile', '--build', 'unused', '--work', 'unused'] + options):
                with contextlib.redirect_stderr(io.StringIO()), patch.object(profile.subprocess, 'run') as run:
                    with self.assertRaises(SystemExit):
                        profile.main()
                    run.assert_not_called()


class HelperProfileTests(unittest.TestCase):
    def testConstantOffsetControlRetainsExceptionChecksAndProvesOnlyValidCases(self):
        import profile_helpers as helpers
        source = ('    %result =l call $__ada_array_offset(l -2, l -4, l 4, l 8)\n'
                  '    jnz %exception, @failed, @ready\n')
        result = helpers.foldConstantOffset(source)
        self.assertIn('%result =l copy 16', result)
        self.assertIn('jnz %exception, @failed, @ready', result)
        widened = '    %index =l extsw -2\n' + source.replace('l -2,', 'l %index,')
        self.assertIn('%result =l copy 16', helpers.foldConstantOffset(widened))
        with self.assertRaises(ValueError):
            helpers.foldConstantOffset(widened.replace('extsw -2', 'extsw 4294967294'))
        for args in ('l 0, l 1, l 4, l 4', 'l 5, l 1, l 4, l 4',
                     'l 2, l 4, l 1, l 4', 'l 2, l 1, l 4, l -1',
                     'l 2, l 0, l 4, l 9223372036854775807',
                     'l 9223372036854775807, l -9223372036854775808, l 9223372036854775807, l 0',
                     'l %index, l 1, l 4, l 4'):
            with self.subTest(args=args), self.assertRaises(ValueError):
                helpers.foldConstantOffset(f'    %result =l call $__ada_array_offset({args})\n')

    def testResidualSuiteRequiresCompleteProductionInlineZeroing(self):
        import profile_helpers as helpers
        source = ('    %scratch =l alloc8 16\n    storel 0, %scratch\n'
                  '    %tail =l add %scratch, 8\n    storel 0, %tail\n')
        helpers.validateResidual(source)
        for invalid in (source.replace('alloc8 16', 'alloc4 16'),
                        source.replace('storel 0, %tail', 'storew 0, %tail'),
                        source + source,
                        source + '    call $memset(l %scratch, w 0, l 16)\n'):
            with self.assertRaises(ValueError):
                helpers.validateResidual(invalid)

    def testResidualCombinedProbeRetainsZeroingAndExceptionBarrier(self):
        import profile_helpers as helpers
        zero = ('    %scratch =l alloc8 16\n    storel 0, %scratch\n'
                '    %tail =l add %scratch, 8\n    storel 0, %tail\n')
        source = (zero + '    %dest =l alloc8 16\n'
                  '    %result =l call $__ada_array_offset(l %index, l 1, l 4, l 4)\n'
                  '    jnz %exception, @failed, @ready\n'
                  '    call $memmove(l %dest, l %scratch, l 16)\n')
        for variant, offset in (('inline-offset-and-copy', 'checked-offset'),
                                ('fold-offset-and-copy', 'constant-offset')):
            proven = '    %index =l extsw 2\n' + source
            result, changes = helpers.transform(proven, variant)
            self.assertEqual(changes, {offset: 1, 'small-copy': 1})
            self.assertIn(zero, result)
            self.assertIn('jnz %exception, @failed, @ready', result)
            if offset == 'checked-offset':
                self.assertIn('call $__ada_raise(l $__ada_exc_constraint_error)', result)
            self.assertLess(result.rindex(' =l loadl '), result.index('    storel %.helper.copy.'))
            helpers.validateResidual(result)

    def testFixedCorpusIsRequiredForUnsafeDiagnosticOmissions(self):
        import profile_helpers as helpers
        for name in helpers.kKernelHashes:
            text = (Path(__file__).parents[1] / 'benchmarks' / (name + '.adb')).read_text()
            helpers.validateKernel(name, text)
            with self.assertRaises(ValueError):
                helpers.validateKernel(name, text + '\n')
        with self.assertRaises(ValueError):
            helpers.validateKernel('other', '')

    def testCheckedOffsetRetainsBothBoundsAndOriginalExceptionCheck(self):
        import profile_helpers as helpers
        source = ('    %result =l call $__ada_array_offset(l %index, l 1, l 4, l 4)\n'
                  '    jnz %exception, @failed, @ready\n')
        result, changes = helpers.transform(source, 'inline-checked-offset')
        self.assertEqual(changes, {'checked-offset': 1})
        self.assertIn('csgel %index, 1', result)
        self.assertIn('cslel %index, 4', result)
        self.assertIn('call $__ada_raise(l $__ada_exc_constraint_error)', result)
        self.assertIn('%result =l phi ', result)
        self.assertIn('jnz %exception, @failed, @ready', result)
        self.assertIn('l 0, l 1, l 4, l 4', helpers.faultIndex(source, 0))

    def testUnsupportedOffsetShapesCannotSilentlyBecomeUncheckedOffsets(self):
        import profile_helpers as helpers
        args = [('1','4','-1'), ('1','4','9223372036854775807'), ('4','1','4'),
                ('1','1','9223372036854775808'), ('%first','4','4')]
        for first,last,stride in args:
            with self.subTest(args=(first,last,stride)), self.assertRaises(ValueError):
                helpers.transform(f'    %r =l call $__ada_array_offset(l %i, l {first}, l {last}, l {stride})\n', 'inline-checked-offset')

    def testSmallCopyLoadsAllSourceWordsBeforeWriting(self):
        import profile_helpers as helpers
        source = ('    %src =l alloc8 16\n    %dst =l alloc8 16\n'
                  '    call $memmove(l %dst, l %src, l 16)\n')
        result, changes = helpers.transform(source, 'inline-small-copy')
        self.assertEqual(changes, {'small-copy': 1})
        self.assertLess(result.rindex(' =l loadl '), result.index('    storel '))
        with self.assertRaises(ValueError):
            helpers.transform(source.replace('alloc8 16', 'alloc4 16'), 'inline-small-copy')

    def testZeroOmissionRequiresTheReviewedScratchShape(self):
        import profile_helpers as helpers
        source = '    %scratch =l alloc8 16\n    call $memset(l %scratch, w 0, l 16)\n'
        result, changes = helpers.transform(source, 'omit-aggregate-zero')
        self.assertEqual(changes, {'aggregate-zero': 1})
        self.assertNotIn('memset', result)
        with self.assertRaises(ValueError):
            helpers.transform(source.replace('l 16)', 'l 8)'), 'omit-aggregate-zero')

    def testInlineZeroKeepsAllBytesAndOriginalSourceLocation(self):
        import profile_helpers as helpers
        source = ('    %scratch =l alloc8 16\n'
                  '    storel $location, %.traceLocation\n'
                  '    call $memset(l %scratch, w 0, l 16)\n')
        result, changes = helpers.transform(source, 'inline-aggregate-zero')
        self.assertEqual(changes, {'inline-zero': 1})
        self.assertIn('storel $location, %.traceLocation', result)
        self.assertEqual(result.count('storel 0,'), 2)
        self.assertIn('add %scratch, 8', result)
        self.assertIn('storel 0, %.helper.zero.0.next', result)
        with self.assertRaises(ValueError):
            helpers.transform(source.replace('alloc8 16', 'alloc8 8'), 'inline-aggregate-zero')

    def testStorageLockDiagnosticKeepsAllOtherMutexOperations(self):
        import profile_helpers as helpers
        source = ('void __ada_mutex_lock(AdaMutex* mutex)\n{\n    pthread_mutex_lock(mutex);\n}\n'
                  'void __ada_mutex_unlock(AdaMutex* mutex)\n{\n    pthread_mutex_unlock(mutex);\n}\n')
        result = helpers.withoutStorageLocks(source)
        self.assertEqual(result.count('mutex == &__ada_main_context.sharedStorageMutex'), 2)
        self.assertIn('pthread_mutex_lock(mutex);', result)
        self.assertIn('pthread_mutex_unlock(mutex);', result)
        with self.assertRaises(ValueError):
            helpers.withoutStorageLocks(source.replace('__ada_mutex_unlock', 'missing'))


if __name__ == '__main__':
    unittest.main()
