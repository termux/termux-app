# Keyboard validation

This runner uses the same `./gradlew test` task as the official
[unit-test workflow](https://github.com/termux/termux-app/blob/master/.github/workflows/run_tests.yml),
then builds the default `apt-android-7` Debug variant and checks the five APKs
and SHA-256 hashes, following the
[build workflow](https://github.com/termux/termux-app/blob/master/.github/workflows/debug_build.yml).
The official CI uses Temurin 17; a compatible local JDK and the SDK/NDK versions
specified in `gradle.properties` must already be installed. This runner does
not download or delete toolchains, install APKs, or publish changes.

Export `JAVA_HOME` and `ANDROID_HOME` (or `ANDROID_SDK_ROOT`). For an isolated
setup, also export `GRADLE_USER_HOME` and `ANDROID_USER_HOME` to the retained
toolchain directory. Run from the repository root:

```sh
python3 tools/kitty-keyboard/validate.py --output-dir ../termux-app-artifacts/kitty-alternates
```

Use `--offline` only with a provisioned dependency cache. Choose a new, empty
output directory for each run. It retains the source diff and newly added files,
version, Gradle log, Debug/Release test XML, five versioned APKs, checksums and
`validation.json`. This uses the default package variant; the official build CI
also tests `apt-android-5` independently. A runner failure preserves its results;
inspect `gradle.log` and the XML rather than treating old artifacts as a new pass.
Raw test directories are archived before their XML is parsed. Damaged or invalid
reports are listed in `validation.json` under `test_result_errors`; the remaining
valid reports are summarized and the runner returns failure. A nonzero Gradle
exit code is preserved even if some XML reports are unreadable.

Exercise evidence-retention regressions without a Java toolchain or device:

```sh
python3 -B tools/kitty-keyboard/test_validate.py
```

These checks use temporary fixture repositories and a simulated failing Gradle
command; they do not replace the actual app tests.

Device acceptance remains a separate step. Record the installed APK version and
hash, negotiation flags, expected and actual PTY bytes, event source and cleanup.
Direct framework delivery, OS injection, touchscreen operations and physical HID
must be labelled separately. A previous feature's device results do not validate
a newer enhancement. Physical actions remain manual; test helpers must never
claim hardware verification for synthetic events. Retain tooling and APKs for
retesting; uninstall temporary helpers and restore terminal modes after capture.

Contribution guidance comes from the app
[README](https://github.com/termux/termux-app#for-maintainers-and-contributors),
`.editorconfig`, and the
[Termux Libraries development guide](https://github.com/termux/termux-app/wiki/Termux-Libraries#forking-and-local-development).
Shared app/plugin utilities belong in the appropriate library; keyboard protocol
encoding remains in `terminal-emulator` and Android input handling in
`terminal-view`. Follow the repository's capitalized commit types and semantic
version requirements. Local Maven publishing is needed when testing dependent
plugins against modified libraries, not for building this app's own modules.
