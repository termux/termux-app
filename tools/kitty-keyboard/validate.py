#!/usr/bin/env python3
"""Run the upstream Gradle checks and retain reproducible keyboard validation artifacts."""

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET



def archive_test_results(repo, output):
    """Copy raw evidence before parsing; one damaged report must not discard other results."""
    suites, errors = {}, []
    fields = ("tests", "failures", "errors", "skipped")
    for module in ("terminal-emulator", "terminal-view", "termux-shared", "app"):
        for directory in sorted((repo / module / "build/test-results").glob("test*UnitTest")):
            destination = output / "test-results" / module / directory.name
            try:
                shutil.copytree(directory, destination)
            except OSError as error:
                errors.append({"path": str(directory.relative_to(repo)), "error": str(error)})
                continue
            summaries = []
            for path in sorted(destination.glob("TEST-*.xml")):
                try:
                    root = ET.parse(path).getroot()
                    if root.tag != "testsuite":
                        raise ValueError("Expected a Gradle testsuite report")
                    summary = {field: int(root.get(field, 0)) for field in fields}
                    if any(value < 0 for value in summary.values()):
                        raise ValueError("Test counts must not be negative")
                    summaries.append(summary)
                except (ET.ParseError, OSError, ValueError) as error:
                    errors.append({"path": str(path.relative_to(output)), "error": str(error)})
            if summaries:
                suites[module + ":" + directory.name] = {
                    field: sum(summary[field] for summary in summaries) for field in fields
                }
    return suites, errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True, type=Path,
                        help="New artifact directory outside the repository; retained after validation")
    parser.add_argument("--offline", action="store_true", help="Use an already provisioned Gradle cache")
    args = parser.parse_args()
    repo = Path(__file__).resolve().parents[2]
    output = args.output_dir.resolve()
    if output == repo or repo in output.parents:
        parser.error("Keep artifacts outside the repository")
    if output.exists() and any(output.iterdir()):
        parser.error("Use an empty output directory to preserve earlier results")
    output.mkdir(parents=True, exist_ok=True)

    def git(*arguments):
        return subprocess.check_output(["git", *arguments], cwd=repo)

    base = git("rev-parse", "HEAD").decode().strip()
    patch = git("diff", "--binary", "HEAD")
    (output / "source.patch").write_bytes(patch)
    digest = hashlib.sha256(patch)
    # Include newly added files so the recorded version identifies the entire source snapshot.
    untracked = git("ls-files", "--others", "--exclude-standard", "-z").split(b"\0")
    for entry in sorted(filter(None, untracked)):
        source = repo / os.fsdecode(entry)
        if not source.is_file():
            continue
        data = source.read_bytes()
        digest.update(entry + b"\0" + hashlib.sha256(data).digest())
        destination = output / "untracked-source" / os.fsdecode(entry)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
    snapshot = digest.hexdigest()
    match = re.search(r'^\s*versionName "([^"]+)"\s*$', (repo / "app/build.gradle").read_text(), re.M)
    if match is None:
        raise RuntimeError("Cannot find the app versionName")
    version = match.group(1).split("+", 1)[0] + "+kitty." + base[:8] + "." + snapshot[:12]
    (output / "version.txt").write_text(version + "\n")
    env = dict(os.environ, TERMUX_APP_VERSION_NAME=version, TERMUX_PACKAGE_VARIANT="apt-android-7",
               TERMUX_SPLIT_APKS_FOR_DEBUG_BUILDS="1", TERMUX_APK_VERSION_TAG="kitty-validation")
    command = [str(repo / "gradlew"), "--no-daemon", "--max-workers=2"]
    if args.offline:
        command.append("--offline")
    command += ["test", ":app:assembleDebug"]
    print("Running upstream Gradle test and Debug APK assembly", flush=True)
    with (output / "gradle.log").open("w") as log:
        result = subprocess.run(command, cwd=repo, env=env, stdout=log, stderr=subprocess.STDOUT)

    suites, test_result_errors = archive_test_results(repo, output)
    checksums = {}
    # Copy only this invocation's APK names, not stale APKs from earlier builds.
    for abi in ("universal", "arm64-v8a", "armeabi-v7a", "x86_64", "x86"):
        apk = repo / "app/build/outputs/apk/debug" / ("termux-app_kitty-validation_" + abi + ".apk")
        if result.returncode == 0 and apk.is_file():
            shutil.copy2(apk, output / apk.name)
            checksums[apk.name] = hashlib.sha256(apk.read_bytes()).hexdigest()
    diff_check = subprocess.run(["git", "diff", "--check", "HEAD"], cwd=repo, capture_output=True)
    report = {
        "base_commit": base, "source_snapshot_sha256": snapshot, "apk_version": version,
        "command": command, "gradle_exit_code": result.returncode, "unit_tests": suites,
        "test_result_errors": test_result_errors,
        "apks": checksums, "diff_check_exit_code": diff_check.returncode,
        "device_validation": "Not run by this script; record actual device evidence separately",
        "toolchain": {key: os.environ.get(key) for key in
                      ("JAVA_HOME", "ANDROID_HOME", "ANDROID_SDK_ROOT", "GRADLE_USER_HOME", "ANDROID_USER_HOME")},
    }
    (output / "validation.json").write_text(json.dumps(report, indent=2) + "\n")
    (output / "SHA256SUMS").write_text("".join(value + "  " + name + "\n" for name, value in sorted(checksums.items())))
    print(json.dumps(report, indent=2))
    if result.returncode:
        return result.returncode
    if diff_check.returncode or len(checksums) != 5 or test_result_errors or not any(
        summary["tests"] for summary in suites.values()
    ):
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
