"""Check observation boundaries without treating rejected B-tests as passes."""

import importlib.util
import json
from pathlib import Path
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location("acatsBaseline", Path(__file__).with_name("run.py"))
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def commandResult(code=0, out="", err="", timeout=False):
    return {"exit": code, "stdout": out, "stderr": err, "timeout": timeout, "output_limit": False}


class ObservationTests(unittest.TestCase):
    def observe(self, results, testClass="C", gaps=None):
        with tempfile.TemporaryDirectory() as directory:
            case = {"id": "probe", "class": testClass, "sources": ["probe.ada"],
                    "support": [], "report_name": "PROBE", "harness_gaps": gaps or []}
            args = SimpleNamespace(work=Path(directory), ada=Path("ada"), adac=Path("adac"),
                                   compile_timeout=1, run_timeout=1)
            with patch.object(runner, "runCommand", side_effect=results):
                _, value = runner.observeCase(case, args)
            recorded = json.loads((Path(directory) / "probe" / "observation.json").read_text())
            self.assertEqual(value, recorded)
            return value

    def testCompleteReportPasses(self):
        value = self.observe([commandResult(), commandResult(out="TEST PROBE\nPASSED\n")])
        self.assertEqual(value["outcome"], "PASS")

    def testZeroExitWithoutResultDoesNotPass(self):
        for output in ["", "TEST PROBE\n", "TEST WRONG\nPASSED\n", "TEST PROBE\nFAILED bad\n"]:
            with self.subTest(output=output):
                value = self.observe([commandResult(), commandResult(out=output)])
                self.assertEqual(value["outcome"], "RUN_FAILED")

    def testStderrAndNonzeroExitDoNotPass(self):
        for result in [commandResult(1, "TEST PROBE\nPASSED\n"),
                       commandResult(out="TEST PROBE\nPASSED\n", err="unexpected")]:
            self.assertEqual(self.observe([commandResult(), result])["outcome"], "RUN_FAILED")

    def testBResultsRemainUngraded(self):
        accepted = self.observe([commandResult()], testClass="B")
        rejected = self.observe([commandResult(1, err="probe.ada:4:2: error: illegal")], testClass="B")
        self.assertEqual(accepted["outcome"], "B_ACCEPTED_UNGRADED")
        self.assertEqual(rejected["outcome"], "B_REJECTED_UNGRADED")

    def testSupportBlockerRemainsSeparate(self):
        result = commandResult(1, err="probe.ada:4:2: error: unknown Ident_Char")
        self.assertEqual(self.observe([result], gaps=["REPORT.Ident_Char"])["outcome"], "HARNESS_BLOCKED")
        self.assertEqual(self.observe([result])["outcome"], "COMPILE_REJECTED")

    def testSupportMetadataCannotMaskCompilerFailure(self):
        result = commandResult(1, err="probe.ada:4:2: error: expected a subtype mark")
        value = self.observe([result], gaps=["REPORT.Ident_Char"])
        self.assertEqual(value["outcome"], "COMPILE_REJECTED")

    def testQbeOrLinkFailureIsNotAnAdaRejection(self):
        result = commandResult(1, err="probe.ssa:4: error: invalid type")
        self.assertEqual(self.observe([result])["outcome"], "BUILD_FAILED")

    def testCrashAndTimeoutAreNeverOrdinaryFailures(self):
        for result, outcome in [(commandResult(-11), "CRASH"),
                                (commandResult(1, err="Assertion failed: type"), "CRASH"),
                                (commandResult(-9, timeout=True), "TIMEOUT")]:
            self.assertEqual(self.observe([result])["outcome"], outcome)
            value = self.observe([commandResult(), result])
            self.assertEqual(value["outcome"], outcome)

    def testNotApplicableCannotMaskCrash(self):
        result = commandResult(-11, out="TEST PROBE\nFAILED not applicable: reason\n")
        self.assertEqual(self.observe([commandResult(), result])["outcome"], "CRASH")

    def testActualSubprocessTimeout(self):
        result = runner.runCommand([sys.executable, "-c", "import time; time.sleep(5)"], 0.05)
        self.assertEqual(runner.abnormal(result), "TIMEOUT")

    def testSourceAndSnapshotIntegrity(self):
        manifest = runner.readJson(runner.kRoot / "manifest.json")
        expected = runner.readJson(runner.kRoot / "expected.json")
        runner.validate(manifest, expected)
        first = manifest["cases"][0]["sources"][0]
        manifest["files"][first]["sha256"] = "wrong"
        with self.assertRaisesRegex(ValueError, "modified upstream source"):
            runner.validate(manifest)

    def testAcceptedBObservationCannotBecomePass(self):
        manifest = runner.readJson(runner.kRoot / "manifest.json")
        expected = runner.readJson(runner.kRoot / "expected.json")
        test = next(case for case in manifest["cases"] if case["class"] == "B")
        expected["cases"][test["id"]]["outcome"] = "PASS"
        with self.assertRaisesRegex(ValueError, "invalid for its test class"):
            runner.validate(manifest, expected)


if __name__ == "__main__":
    unittest.main()
