"""Adversarial checks for B-test grading, independent of frozen diagnostics."""

import importlib.util
from pathlib import Path
import sys
import unittest

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('grading', Path(__file__).with_name('grade.py'))
grading = importlib.util.module_from_spec(spec)
spec.loader.exec_module(grading)


def result(code=0, stderr='', stdout='', timeout=False):
    return dict(exit=code, stderr=stderr, stdout=stdout, timeout=timeout, output_limit=False)


class GradingTests(unittest.TestCase):
    def setUp(self):
        self.sites = [dict(source='tests/b3/probe.a', line=n, lines=[n],
                           optional=False, groups=['illegal type']) for n in [10, 20]]
        self.errors = ''.join(f'<upstream>/tests/b3/probe.a:{n}:4: error: illegal type\n'
                              for n in [10, 20])

    def checkGrade(self, errors=None, code=1, legal=None, policy=None):
        return grading.grade(self.sites, policy or {},
                             result(code, self.errors if errors is None else errors),
                             legal or result())

    def testCompleteRejectionAndLegalControlPass(self):
        self.assertEqual(self.checkGrade()['outcome'], 'PASS')

    def testRecoveryMustReachEveryRequiredSite(self):
        value = self.checkGrade(self.errors.splitlines(True)[0])
        self.assertEqual(value['outcome'], 'FAIL')
        self.assertEqual(value['missing'], ['tests/b3/probe.a:20'])

    def testUnrelatedMessageAtCorrectLocationFails(self):
        self.assertEqual(self.checkGrade(self.errors.replace('illegal type', 'unknown unit'))['outcome'], 'FAIL')

    def testWrongLineOrFileFails(self):
        for before, after in [(':10:', ':11:'), ('probe.a', 'helper.a')]:
            self.assertEqual(self.checkGrade(self.errors.replace(before, after))['outcome'], 'FAIL')

    def testExtraDiagnosticOnLegalLineFails(self):
        value = self.checkGrade(self.errors + '<upstream>/tests/b3/probe.a:25:2: error: illegal type\n')
        self.assertEqual(value['outcome'], 'FAIL')

    def testSuccessfulCompilationOrBuildFailureCannotPass(self):
        for code in [0, 2, 127]:
            self.assertEqual(self.checkGrade(code=code)['outcome'], 'FAIL')
        self.assertEqual(self.checkGrade('linker failed')['outcome'], 'FAIL')

    def testLegalControlMustCompileCleanly(self):
        for legal in [result(1), result(stderr='warning'), result(stdout='unexpected')]:
            self.assertEqual(self.checkGrade(legal=legal)['outcome'], 'FAIL')

    def testUnsupportedNeverSatisfiesEvenAnExplicitGroup(self):
        for site in self.sites:
            site['groups'] = ['.*']
        for message in ['feature not yet supported', 'unsupported attribute', 'not implemented']:
            value = self.checkGrade(self.errors.replace('illegal type', message))
            self.assertEqual(value['outcome'], 'FAIL')
            self.assertEqual(len(value['missing']), 2)

    def testCrashesTimeoutAndOutputLimitsCannotPass(self):
        for abnormal in [result(-11), result(timeout=True), dict(result(), output_limit=True)]:
            value = grading.grade(self.sites, {}, abnormal, result())
            self.assertIn(value['outcome'], grading.baseline.kCritical)
            value = grading.grade(self.sites, {}, result(1, self.errors), abnormal)
            self.assertIn(value['outcome'], grading.baseline.kCritical)

    def testOptionalSiteNeedNotBeDiagnosed(self):
        self.sites[1]['optional'] = True
        self.assertEqual(self.checkGrade(self.errors.splitlines(True)[0])['outcome'], 'PASS')
        self.assertEqual(self.checkGrade()['outcome'], 'PASS')

    def testReviewedRecoveryCannotDischargeRequiredSite(self):
        policy = dict(recovery=[dict(source='tests/b3/probe.a', lines=[20], message='cascade')])
        self.assertEqual(self.checkGrade(self.errors.replace(':20:4: error: illegal type',
                                                            ':20:4: error: cascade'), policy=policy)['outcome'], 'FAIL')
        self.assertEqual(self.checkGrade(self.errors + '<upstream>/tests/b3/probe.a:20:9: error: cascade\n',
                                        policy=policy)['outcome'], 'PASS')

    def testMarkersExcludeProseAndReadOptionalErrors(self):
        self.assertEqual(grading.markers('-- ERROR text is prose\nX; -- ERROR: {1:5;1}\nY; -- OPTIONAL ERROR:\nZ; -- OK.\n'),
                         [dict(line=2, optional=False), dict(line=3, optional=True)])

    def testAllPoliciesReferenceActualMarkers(self):
        manifest = grading.baseline.readJson(grading.baseline.kRoot / 'manifest.json')
        policies = grading.baseline.readJson(grading.kRoot / 'policies.json')
        cases = [case for case in manifest['cases'] if case['class'] == 'B']
        self.assertEqual(set(policies), {case['id'] for case in cases})
        for case in cases:
            sites = grading.expectations(case, policies[case['id']])
            keys = {f"{site['source']}:{site['line']}" for site in sites}
            self.assertTrue(set(policies[case['id']].get('sites', {})) <= keys)
            self.assertTrue((grading.kRoot / policies[case['id']]['legal']).is_file())


if __name__ == '__main__':
    unittest.main()
