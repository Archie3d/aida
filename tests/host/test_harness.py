"""Regressions for the Windows CI harness failures, runnable on any host."""

import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
kRoot = Path(__file__).resolve().parent


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


install = module('installCheck', kRoot / 'check_install.py')
support = module('supportCheck', kRoot.parent / 'acats/grading/check_support.py')


def execution(output, code=0, stderr=''):
    return dict(exit=code, stdout=output, stderr=stderr, timeout=False, output_limit=False)


class HarnessTests(unittest.TestCase):
    def testBothNativeNewlineConventionsCompleteTheProtocol(self):
        for newline in ['\n', '\r\n']:
            self.assertTrue(support.validExecution('positive', execution(
                newline.join(['TEST SUPPORT', 'PASSED', '']))))

    def testNewlineNormalizationDoesNotAllowPartialOrFailedReports(self):
        for output in ['TEST SUPPORT\r\n', 'TEST WRONG\r\nPASSED\r\n',
                       'TEST SUPPORT\r\nFAILED probe\r\nPASSED\r\n',
                       'TEST SUPPORT\rPASSED\r', 'TEST SUPPORT\r\nPASSED\r\nextra\r\n']:
            self.assertFalse(support.validExecution('positive', execution(output)))

    def testExitStatusAndStderrStillMatter(self):
        output = 'TEST SUPPORT\r\nPASSED\r\n'
        for result in [execution(output, code=1), execution(output, stderr='unexpected'),
                       execution(output, code=-11), dict(execution(output), timeout=True)]:
            self.assertFalse(support.validExecution('positive', result))

    def testIntentionalFailureMustReportFailureAndCannotCrash(self):
        failed = 'TEST SUPPORT\r\nFAILED probe\r\n'
        self.assertTrue(support.validExecution('ordered', execution(failed, code=1)))
        for result in [execution(failed), execution(failed, code=-11), execution(failed, code=0xC0000005),
                       execution(failed, code=2),
                       execution('TEST SUPPORT\r\n', code=1),
                       execution(failed + 'PASSED\r\n', code=1)]:
            self.assertFalse(support.validExecution('ordered', result))

    def testWindowsStyleSearchUsesChildPathAndKeepsBareArgv(self):
        # Model CreateProcess: the caller must supply an executable selected
        # from the replacement PATH, while argv[0] stays bare for the driver's
        # own PATH-based installation discovery.
        with tempfile.TemporaryDirectory(prefix='host path ') as temporary:
            tool = Path(temporary) / 'ada.exe'
            tool.write_bytes(b'fixture')
            environment = {'PATH': temporary}
            def launch(command, **kwargs):
                self.assertEqual(command, ['ada', '--help'])
                self.assertEqual(kwargs['executable'], str(tool))
                self.assertEqual(kwargs['env'], environment)
                return subprocess.CompletedProcess(command, 0, 'ok\n', '')
            with patch.object(install.shutil, 'which', return_value=str(tool)) as search:
                with patch.object(install.subprocess, 'run', side_effect=launch):
                    install.run(['ada', '--help'], temporary, environment, 'ok\n')
                search.assert_called_once_with('ada', path=temporary)

    def testMissingTestExecutableCannotFallBackToCallerPath(self):
        with patch.object(install.shutil, 'which', return_value=None):
            with patch.object(install.subprocess, 'run') as launch:
                with self.assertRaises(FileNotFoundError):
                    install.run(['ada'], kRoot, {'PATH': 'missing tools'})
                launch.assert_not_called()

    def testAbsoluteExecutableDoesNotNeedPathSearch(self):
        with tempfile.TemporaryDirectory() as temporary:
            tool = Path(temporary) / 'ada.exe'
            with patch.object(install.shutil, 'which') as search:
                with patch.object(install.subprocess, 'run', return_value=
                                  subprocess.CompletedProcess([], 0, '', '')) as launch:
                    install.run([tool], temporary, {'PATH': ''})
                search.assert_not_called()
                self.assertIsNone(launch.call_args.kwargs['executable'])


if __name__ == '__main__':
    unittest.main()
