#!/usr/bin/env python3
"""Exercise failure-evidence retention without a Java toolchain or Android device."""

import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


SOURCE = Path(__file__).with_name("validate.py")
spec = importlib.util.spec_from_file_location("kitty_validation", SOURCE)
validation = importlib.util.module_from_spec(spec)
spec.loader.exec_module(validation)


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)


class ValidationEvidenceTest(unittest.TestCase):
    def test_damaged_xml_is_archived_and_other_modules_are_summarized(self):
        with tempfile.TemporaryDirectory(prefix="kitty-validation-evidence-") as temporary:
            root = Path(temporary)
            repo, output = root / "repo", root / "output"
            damaged = repo / "terminal-emulator/build/test-results/testDebugUnitTest/TEST-damaged.xml"
            valid = repo / "app/build/test-results/testDebugUnitTest/TEST-valid.xml"
            write(damaged, '<testsuite tests="2"><testcase')
            write(valid, '<testsuite tests="3" failures="0" errors="0" skipped="0"/>')
            summaries, errors = validation.archive_test_results(repo, output)
            self.assertEqual((output / "test-results/terminal-emulator/testDebugUnitTest/TEST-damaged.xml").read_bytes(),
                             damaged.read_bytes())
            self.assertEqual((output / "test-results/app/testDebugUnitTest/TEST-valid.xml").read_bytes(),
                             valid.read_bytes())
            self.assertEqual(summaries["app:testDebugUnitTest"]["tests"], 3)
            self.assertEqual(len(errors), 1)
            self.assertIn("TEST-damaged.xml", errors[0]["path"])

    def test_invalid_counts_preserve_raw_evidence_and_valid_suites(self):
        with tempfile.TemporaryDirectory(prefix="kitty-validation-counts-") as temporary:
            root = Path(temporary)
            directory = root / "repo/app/build/test-results/testReleaseUnitTest"
            write(directory / "TEST-invalid.xml", '<testsuite tests="invalid"/>')
            write(directory / "TEST-negative.xml", '<testsuite tests="-1"/>')
            write(directory / "TEST-valid.xml", '<testsuite tests="2" failures="1"/>')
            summaries, errors = validation.archive_test_results(root / "repo", root / "output")
            self.assertEqual(summaries["app:testReleaseUnitTest"]["tests"], 2)
            self.assertEqual(summaries["app:testReleaseUnitTest"]["failures"], 1)
            self.assertEqual(len(errors), 2)
            self.assertEqual(len(list((root / "output/test-results/app/testReleaseUnitTest").glob("*.xml"))), 3)

    def test_cli_failure_still_writes_summary_and_archives_damaged_xml(self):
        with tempfile.TemporaryDirectory(prefix="kitty-validation-cli-") as temporary:
            root = Path(temporary)
            repo, output = root / "repo", root / "output"
            script = repo / "tools/kitty-keyboard/validate.py"
            script.parent.mkdir(parents=True)
            shutil.copy2(SOURCE, script)
            write(repo / "app/build.gradle", '    versionName "0.118.0"\n')
            gradle = repo / "gradlew"
            write(gradle, """#!/bin/sh
mkdir -p terminal-emulator/build/test-results/testDebugUnitTest app/build/test-results/testDebugUnitTest
printf '<testsuite tests="2"><testcase' > terminal-emulator/build/test-results/testDebugUnitTest/TEST-damaged.xml
printf '<testsuite tests="3" failures="0" errors="0" skipped="0"/>' > app/build/test-results/testDebugUnitTest/TEST-valid.xml
printf 'Simulated Gradle failure for evidence-retention regression\\n'
exit 19
""")
            gradle.chmod(0o755)
            subprocess.run(["git", "init", "--quiet", str(repo)], check=True)
            subprocess.run(["git", "add", "."], cwd=repo, check=True)
            subprocess.run(["git", "-c", "user.name=Kitty Validation Tests", "-c", "user.email=kitty-tests@example.invalid",
                            "-c", "commit.gpgsign=false", "-c", "core.hooksPath=/dev/null",
                            "commit", "--quiet", "-m", "Fixture"], cwd=repo, check=True)
            result = subprocess.run([sys.executable, "-B", str(script), "--offline", "--output-dir", str(output)],
                                    cwd=repo, capture_output=True, text=True)
            self.assertEqual(result.returncode, 19, result.stderr)
            report = json.loads((output / "validation.json").read_text())
            self.assertEqual(report["gradle_exit_code"], 19)
            self.assertEqual(len(report["test_result_errors"]), 1)
            self.assertEqual(report["unit_tests"]["app:testDebugUnitTest"]["tests"], 3)
            self.assertEqual(report["apks"], {})
            self.assertEqual((output / "test-results/terminal-emulator/testDebugUnitTest/TEST-damaged.xml").read_text(),
                             '<testsuite tests="2"><testcase')
            self.assertIn("Simulated Gradle failure", (output / "gradle.log").read_text())


if __name__ == "__main__":
    unittest.main()
