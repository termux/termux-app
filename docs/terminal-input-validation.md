# Terminal input queue validation

The input worker preserves FIFO order for accepted keyboard and paste jobs. Paste admission is atomic: if the queue cannot retain the whole paste, none of that submission is enqueued. A 64 KiB reserve is available to non-paste input. This reduces, but cannot eliminate, input rejection under sustained pressure. The current visible session shows a rejection toast; other clients receive a main-thread callback with a log/bell default. There is no automatic retry.

The 4 MiB queue budget accounts for retained payloads plus per-job overhead. It is not a bound on all heap allocations: sanitizing and UTF-8 encoding an active paste also allocate temporary buffers, bounded by the accepted paste size.

## Host checks (2026-11-10)

- All terminal-emulator production and test Java sources compiled with Java 8 language compatibility, using Eclipse ECJ 3.42.0 against Android 15 API classes (`org.robolectric:android-all:15-robolectric-12650502`). Temporary NonNull/Nullable annotation stubs were used for compilation.
- 10 input queue tests passed in ByblackFinalIntegrationTest: 1 MiB paste plus a later key, full control-character/newline sanitization, Unicode, bracketed paste, reserved typing capacity, rejection without a partial paste, copying byte ranges, tiny-job saturation, blocked-writer shutdown, downstream closure, empty and invalid input.
- 42 selected JUnit 4.13.2 tests passed in total: ByteQueueTest, ByblackFinalIntegrationTest, TerminalTest and UnicodeInputTest. Host execution used a temporary SystemClock.uptimeMillis shim based on System.nanoTime; the Android native method is unavailable on the host. The shim and dependencies are not part of the source changes.
- strings.xml parses successfully; git diff --check passes.

These host checks do not establish Android UI responsiveness, and are not a full Android Gradle build.

## Still required before leaving draft

- Run the Android Gradle tests and build the APK with the supported SDK/NDK.
- On a device, paste a large payload into a controlled input-consuming program; confirm the UI remains responsive and the received bytes and bracket delimiters match.
- Pause the consumer, fill the input queue, and confirm rejected submissions show the toast without sending a partial paste. Resume and verify accepted input order.
- Exercise keyboard input after a large paste, UTF-8 text, empty/control-only pastes, and repeated session creation/exit while input is pending.

Do not retry rejected shell commands blindly: earlier accepted submissions may already have executed.
