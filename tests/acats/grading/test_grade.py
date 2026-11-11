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

    def testRelatedNotesDoNotSatisfyMarkers(self):
        notes = self.errors.replace(': error:', ': note:')
        self.assertEqual(self.checkGrade(notes)['outcome'], 'FAIL')
        self.assertEqual(len(self.checkGrade(notes)['missing']), 2)
        self.assertEqual(self.checkGrade(self.errors + notes)['outcome'], 'PASS')
        self.assertEqual(self.checkGrade(self.errors + 'malformed note')['outcome'], 'FAIL')
        self.assertEqual(self.checkGrade(legal=result(stderr=notes))['outcome'], 'FAIL')

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

    def reviewedFixture(self, caseId):
        manifest = grading.baseline.readJson(grading.baseline.kRoot / 'manifest.json')
        case = next(case for case in manifest['cases'] if case['id'] == caseId)
        policy = grading.baseline.readJson(grading.kRoot / 'policies.json')[caseId]
        sites = grading.expectations(case, policy)
        snapshot = grading.baseline.readJson(grading.baseline.kRoot / 'expected.json')
        errors = snapshot['cases'][caseId]['compile_stderr']
        return sites, policy, errors

    def testIndefiniteDeclarationSpansStayWithinTheirDeclarations(self):
        sites, policy, errors = self.reviewedFixture('b330001')
        self.assertEqual(len(sites), 19)
        self.assertEqual(grading.grade(sites, policy, result(1, errors), result())['outcome'], 'PASS')
        for declaration, marker in [(129, 130), (152, 153), (162, 163), (222, 223)]:
            # Either token in this two-line declaration is valid; the line
            # before the declaration cannot discharge its required error.
            moved = errors.replace(f':{declaration}:4:', f':{marker}:4:')
            self.assertNotEqual(moved, errors)
            self.assertEqual(grading.grade(sites, policy, result(1, moved), result())['outcome'], 'PASS')
            outside = errors.replace(f':{declaration}:4:', f':{declaration - 1}:4:')
            value = grading.grade(sites, policy, result(1, outside), result())
            self.assertEqual(value['outcome'], 'FAIL')
            self.assertIn(f'tests/b3/b330001.a:{marker}', value['missing'])

    def testIndefiniteObjectCannotBeGradedAsAnArrayComponent(self):
        sites, policy, errors = self.reviewedFixture('b330001')
        wrong = errors.replace('an object of an unknown-discriminant view requires an initializer',
                               'an array component requires a definite subtype view', 1)
        self.assertNotEqual(wrong, errors)
        value = grading.grade(sites, policy, result(1, wrong), result())
        self.assertEqual(value['outcome'], 'FAIL')
        self.assertIn('tests/b3/b330001.a:149', value['missing'])
        self.assertEqual(len(value['unexpected']), 1)

    def omitSiteDiagnostics(self, errors, sites):
        def included(line):
            diagnostic = grading.kDiagnostic.fullmatch(line)
            return diagnostic and any(
                diagnostic[1].endswith('/' + site['source']) and int(diagnostic[2]) in site['lines']
                for site in sites)
        return ''.join(line for line in errors.splitlines(True) if not included(line.rstrip('\n')))

    def testAncestryCompletionsAreOptionalAndCannotReplaceRequiredErrors(self):
        for caseId, required in [('b394a02', [78]), ('b394a03', [88, 133])]:
            sites, policy, errors = self.reviewedFixture(caseId)
            optional = [site for site in sites if site['optional']]
            self.assertEqual(len(optional), 4 if caseId == 'b394a02' else 2)
            self.assertEqual(grading.grade(sites, policy, result(1, errors), result())['outcome'], 'PASS')
            without = self.omitSiteDiagnostics(errors, optional)
            self.assertEqual(grading.grade(sites, policy, result(1, without), result())['outcome'], 'PASS')
            message = ('a descendant of a nonlimited interface must be nonlimited'
                       if caseId == 'b394a02' else 'incompatible synchronized interface categories')
            completions = without + ''.join(
                f"<upstream>/{site['source']}:{min(site['lines'])}:5: error: {message}\n"
                for site in optional)
            self.assertEqual(grading.grade(sites, policy, result(1, completions), result())['outcome'], 'PASS')
            for marker in required:
                # Keep the related completion error, but remove the error at
                # the original declaration. The required marker must fail.
                missing = self.omitSiteDiagnostics(completions, [site for site in sites if site['line'] == marker])
                value = grading.grade(sites, policy, result(1, missing), result())
                self.assertEqual(value['outcome'], 'FAIL')
                self.assertIn(f'tests/b3/{caseId}.a:{marker}', value['missing'])

    def testAncestryErrorsCannotEscapeTheirDeclarationsOrUseAnotherRule(self):
        for caseId in ['b394a02', 'b394a03', 'b394a04', 'b394a05']:
            sites, policy, errors = self.reviewedFixture(caseId)
            self.assertEqual(grading.grade(sites, policy, result(1, errors), result())['outcome'], 'PASS')
            first = sites[0]
            start = min(first['lines'])
            outside = errors.replace(f':{start}:5:', f':{start - 1}:5:')
            self.assertNotEqual(outside, errors)
            value = grading.grade(sites, policy, result(1, outside), result())
            self.assertEqual(value['outcome'], 'FAIL')
            self.assertIn(f"{first['source']}:{first['line']}", value['missing'])
            original = grading.kDiagnostic.fullmatch(errors.splitlines()[0])[4]
            wrong = errors.replace(original,
                'a record type cannot implement a synchronized, task or protected interface', 1)
            value = grading.grade(sites, policy, result(1, wrong), result())
            self.assertEqual(value['outcome'], 'FAIL')
            self.assertIn(f"{first['source']}:{first['line']}", value['missing'])


if __name__ == '__main__':
    unittest.main()
