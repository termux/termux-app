# Kitty keyboard protocol support

The terminal supports disambiguate escape codes (flag `1`), report event
types (flag `2`) and report alternate keys (flag `4`). It does not implement
reporting all keys or associated text. Unsupported flags are ignored and are never reported
as enabled by a query.

Applications negotiate support using `CSI ? u`, enable it using `CSI > 1 u`, and
restore the previous mode using `CSI < u`. Direct flag updates (`CSI = flags ;
mode u`) and popping multiple entries are supported. The main and alternate
screens have independent stacks, limited to 32 entries including the current mode. Terminal reset
clears both stacks. Without negotiation the existing keyboard encoding is used.

Examples after enabling flag `1`:

| Input | Bytes sent |
| --- | --- |
| Ctrl+0 | `ESC [ 48 ; 5 u` |
| Ctrl+I | `ESC [ 105 ; 5 u` |
| Ctrl+Shift+A | `ESC [ 97 ; 6 u` |
| Shift+Enter | `ESC [ 13 ; 2 u` |
| Ctrl+Enter | `ESC [ 13 ; 5 u` |
| Escape | `ESC [ 27 u` |
| Plain Tab / Enter / Backspace | Existing recovery control bytes |

Hardware shortcuts use the active Android layout's unshifted Unicode character.
Shift, Alt, Ctrl and Android Meta (mapped to Super) are encoded. Num Lock and
Caps Lock are included for functional keys; keypad keys use dedicated codes.
Plain text, IME commits and paste retain their existing paths. Soft keyboard or
extra-key shortcuts encode the available character and modifiers; an IME text
commit cannot recover a physical key identity or its original modifier state.
Termux application shortcuts continue to take precedence over terminal input.
This includes the existing Shift+PageUp/PageDown scroll bindings. Main-screen
scrollback, alternate-screen arrow scrolling and mouse reporting retain their
existing scroll behavior. Space is resolved through the Android layout before
shortcut encoding, so right Alt/AltGr text is preserved; left Alt+Space and
Ctrl+Space still use negotiated keyboard encoding.

The program receiving the input must support the protocol. Intermediaries such
as terminal multiplexers also need to pass or translate the sequences correctly.

Specification: https://sw.kovidgoyal.net/kitty/keyboard-protocol/

## Repeat and release events

Enable both enhancements with `CSI > 3 u`, or add event reporting to an existing
mode with `CSI = 2 ; 2 u`. A query returns the enabled supported flags, including
`2` or `3`. Flag `2` can also be enabled independently with `CSI > 2 u`.

Android `ACTION_DOWN` with a nonzero repeat count is encoded as event type `2`;
`ACTION_UP` is encoded as type `3`. Press events omit the optional event type.
Examples with flags `3`:

| Input | Press | Repeat | Release |
| --- | --- | --- | --- |
| Up | `ESC [ A` | `ESC [ 1 ; 1 : 2 A` | `ESC [ 1 ; 1 : 3 A` |
| Ctrl+I | `ESC [ 105 ; 5 u` | `ESC [ 105 ; 5 : 2 u` | `ESC [ 105 ; 5 : 3 u` |
| Escape | `ESC [ 27 u` | `ESC [ 27 ; 1 : 2 u` | `ESC [ 27 ; 1 : 3 u` |
| Plain Enter | `CR` | `CR` | None |
| Plain A | `a` | `a` | None |

With flag `2` alone, shortcut press bytes retain legacy encoding, while their
repeat and release events use CSI event encoding. Escape likewise retains its
legacy press byte. Modified Enter, Tab and Backspace repeats also use CSI
with event type `2`; their presses keep legacy bytes and releases stay suppressed.
Plain recovery-key repeats remain control bytes even with lock modifiers enabled.
Functional keys use CSI event encoding. Legacy keypad text
and application-keypad encodings retain their existing input paths; keypad
navigation events use the equivalent normal navigation key identity. Flag `1`
is required for dedicated keypad identities.

Enter, Tab and Backspace do not emit release events, including modified variants.
Plain and Shift-only text, AltGr text, IME commits and paste retain their text
paths. Standalone modifier key events require the unsupported report-all-keys
flag `8`. Soft input and virtual Fn mappings do not manufacture repeat or release
events from text commits or virtual shortcuts.

Only keys delivered to the terminal have a matching release. Application
shortcuts and Shift+PageUp/PageDown remain prior to terminal encoding. Pending
releases are discarded on canceled input, focus loss, view detachment, session
changes, screen switches, terminal reset, or keyboard mode updates. Device IDs
and press timestamps prevent an unrelated key-up from matching a held key.
Release events preserve the layout/keypad identity resolved on key-down and use
the current physical modifiers, including when Ctrl, Shift or Num Lock changes
before the key is released. Virtual modifiers used for the original shortcut
are retained for its release.

## Alternate key codes

Flag `4` enhances an existing encoded key event with optional shifted and
standard PC-101 layout code points. Enable it with disambiguation using
`CSI > 5 u`, or with disambiguation and events using `CSI > 7 u`.
It can be added using `CSI = 4 ; 2 u` and removed using `CSI = 4 ; 3 u`.
Queries report supported combinations `0` through `7`; flags `8` and `16`
remain unsupported. Flag `4` alone is queryable but keeps legacy input.

The key field is `primary:shifted:base`. The primary stays the active layout's
unshifted character. The shifted value is resolved through Android's current
layout, not by assuming uppercase, and is included only when Shift is active.
Physical left/right Shift states are retained for side-specific layout mappings,
including right Shift with AltGr. Virtual Shift retains its left-Shift default.
The base value uses known Android letter, digit, Space and punctuation key codes
to identify their standard PC-101 equivalents. Unknown and extended positions
are omitted. Values identical to the primary are redundant and omitted.
A base-only alternate keeps an empty shifted subfield: `primary::base`.

Examples on layouts with the indicated characters:

| Input and flags | Bytes |
| --- | --- |
| Ctrl+Shift+A, flags `5` | `ESC [ 97 : 65 ; 6 u` |
| Ctrl+Shift+= producing `+`, flags `5` | `ESC [ 61 : 43 ; 6 u` |
| Same key repeated, flags `7` | `ESC [ 61 : 43 ; 6 : 2 u` |
| Cyrillic `с` on the C position, Ctrl, flags `5` | `ESC [ 1089 : : 99 ; 5 u` |
| Cyrillic `с` on C, Ctrl+Shift, flags `5` | `ESC [ 1089 : 1057 : 99 ; 6 u` |
| Plain Shift+A, flags `7` | UTF-8 `A` |

Presses retain an immutable layout identity for matching releases. A release
uses the initial primary/base identity and the current modifier state; if Shift
has been released, its shifted subfield is omitted. Functional-key sequences
retain their dedicated identities and never acquire printable alternates.

IME commits and virtual shortcuts supply no physical identity and do not invent
base or shifted fields. Soft-keyboard flagged KeyEvents can report a known
layout-shifted character, but omit the physical base. Dead-key composition
fallbacks also omit unavailable alternates. Right Alt/AltGr remains part of
layout text resolution, and plain text keeps its UTF-8 path without flag `8`.
The base mapping assumes standard Android key-code assignments; custom firmware
or key-layout remapping needs separate validation.

## Validation status

### Alternate-key review fixes (2026-10-09)

- Preserve actual left/right Shift states when resolving the shifted alternate;
  a physical right Shift no longer gains an artificial left-Shift bit. Four
  new Android regressions cover generic/left/right/both Shift, repeats and
  releases, right Shift with AltGr, ordinary text and virtual Shift's existing
  default. These side-specific layouts are test fixtures, not hardware layouts.
- Archive raw Gradle test directories before parsing their XML. Damaged reports
  are retained and recorded in `test_result_errors`, other valid suites are
  still summarized, and the runner reports failure. Three isolated tool checks
  include an end-to-end simulated Gradle failure with truncated XML.
- All 388 Debug/Release Gradle test executions passed (155 emulator and 39 app
  tests per variant), along with the three validation-tool checks; five Debug
  APKs were rebuilt and retained. Version: `0.118.0+kitty.817dfb43.edcdab58162c`.
- All 138 native PTY checks passed on Android 16 PTP-AN10: the previous 125
  cases plus 13 new Shift-side fixtures. The 13 new cases are synthetic layout
  fixtures delivered as framework events, not physical keyboard layouts. See
  [the review-fix report](kitty-keyboard-alternate-review-validation.json).

### Alternate keys (2026-10-09)

- The complete upstream `./gradlew test` task passed with 155 terminal-emulator
  and 35 app tests per Debug/Release variant: 380 executions, zero failures,
  errors or skips. Three terminal and seven Android input tests were added.
- Negotiation covers every supported flag combination, unsupported masks,
  set/add/remove, stack restoration, separate screens and reset. Encoding
  covers shifted punctuation, base-only/non-Latin fields, supplementary Unicode,
  omitted invalid/duplicate alternatives, functional and recovery boundaries.
- Robolectric fixtures exercise Cyrillic and changed Greek layouts, a shifted
  character that is not uppercase, release identity after layout/modifier
  changes, AltGr, soft input, and alternate-only legacy isolation. These are
  synthetic layout fixtures, not physical non-Latin keyboard coverage. The
  Robolectric 4.10 map lacks Shift+=; that US entry is supplied explicitly.
- Debug APK assembly succeeded for the four ABIs and universal package.
  Toolchain retained: Temurin 21.0.12.1, Gradle 9.2.1, Android API 36,
  Build Tools 36.0.0, NDK 29.0.14206865. The upstream CI currently uses Temurin
  17; this local run used the retained compatible JDK 21.
- Reproduce and retain the source snapshot, all unit-test XML, log, versioned
  APKs and checksums with the [validation runner](../tools/kitty-keyboard/README.md).
  This entry point was run successfully, retaining five uniquely versioned APKs
  and all 380 Debug/Release test results.
- On the Android 16 arm64 PTP-AN10, all 125 new native PTY checks passed against
  installed version `0.118.0+kitty.59c2f1ea.10429391625c`. See
  [the alternate-key device report](kitty-keyboard-alternate-device-validation.json).
  It includes all flags `0` through `7`, shifted A and punctuation, dropping the
  shifted subfield after Shift release, and earlier event/text/recovery cases.
- Of these, 122 checks use Android's real virtual keyboard mapping and three
  override Unicode values in a synthetic KeyEvent layout fixture. All input was
  delivered directly to production `TerminalView`, not through OS dispatch or
  physical HID. The three fixture checks cover Cyrillic primary/shifted/base,
  base-only fields and soft-keyboard base suppression across a real native PTY.
  No new flag `4` physical layout, real IME, SSH or multiplexer verification is
  claimed; the historical reports below still validate their original flags.


### Event reporting (2026-10-08)

- All 152 terminal-emulator and 28 app tests passed with zero failures, errors
  or skips. This adds two terminal tests and eighteen Android input tests.
- Negotiation covers both flag `2` alone and flags `3`, set/add/remove,
  push/pop, independent screens, and reset. Byte comparisons cover repeated
  and released navigation, shortcuts, Escape, F3 and keypad keys.
- Android events pass through the production `TerminalView` into the session
  input queue under Robolectric API 28. Coverage includes current release
  modifiers, Num Lock identity changes, AltGr layout text, recovery keys,
  consumed shortcuts, canceled/orphaned releases, separate device IDs,
  screen round trips, mode changes, reset and focus loss. The real extra-key
  bridge and virtual Fn press paths are covered; the deferred virtual Fn Ctrl
  limitation remains unchanged. Two P1 regression tests additionally cover
  Num Lock changes during repeats and prevent orphaned repeats or mode changes
  from creating or reviving a pending release. Only the initial press records
  the release identity. A P2 regression additionally checks flag `2` modified
  Enter/Tab/Backspace repeats and ordinary recovery repeats with lock modifiers.
  These tests do not
  start a native PTY or exercise physical keyboard hardware.
- Debug APK assembly passed for arm64-v8a, armeabi-v7a, x86, x86_64 and universal
  using the default apt-android-7 package variant. Toolchain: Temurin
  21.0.12.1, Gradle 9.2.1, API 36, Build Tools 36.0.0, NDK 29.0.14206865.
- `git diff --check` passed. The initial local build had no attached device.
  Subsequent P1/P2 device verification on the Android 16 arm64 PTP-AN10 passed
  all 59 raw byte checks against the installed production `TerminalView`, real
  session input queue and native PTY. See
  [the event-reporting device report](kitty-keyboard-events-device-validation.json).
- Device coverage includes flags `0`/`1`/`2`/`3`, ordinary and modified repeats,
  matching releases, recovery/text boundaries, modifier-first release, F3,
  orphaned repeats, the three P1 Num Lock transitions, and P2 modified recovery
  repeats under both flags `2` and `3`. Android framework `KeyEvent` objects
  were delivered directly to the production view through temporary
  instrumentation. This does not exercise physical HID or OS input dispatch.
- APK version: `0.118.0+kitty.events.p2.60a581a1.2439221fb851`. The source patch
  and APK checksums are recorded with the report. A first attempt had one
  infrastructure timeout due to background freezing; the final foreground
  rerun passed all cases. Capture used foreground `dd` with a start handshake.
- New physical-keyboard, real IME UI, SSH, multiplexer, lifecycle and session
  switching checks remain outside this recorded device coverage.

For device acceptance, negotiate `3` in a raw PTY capture, hold and release Up,
Ctrl+I, Escape, F3 and a Num Lock keypad key, and compare press/repeat/release
bytes against this table. Repeat with flag `2` alone, restore with `CSI < u`,
and confirm that plain text and recovery keys emit no releases. Check consumed
application/scroll shortcuts, modifier-first release, screen/session changes,
background/foreground, IME commits and extra keys. Record injection separately
from physical HID events and record SSH/multiplexer versions with raw captures.
The historical device reports below validate flag `1`, not this new flag `2`.

### Bluetooth keyboard and real IME validation (2026-10-08/09)

- The user connected a RAPOO Keyboard-1 Keyboard over Bluetooth. Android
  recognized an external alphabetic keyboard (device ID 9, keyboard type 2).
  Test keys passed through actual Android input dispatch and the installed
  Termux activity/client/view into the native PTY; no synthetic key events
  were used. See [the real-input report](kitty-keyboard-real-input-validation.json).
- With flags `3`, recorded physical input confirms Up and Ctrl+I
  press/repeat/release, Ctrl+Shift+A and Escape press/release, modified
  Shift+Tab repeats, ordinary text, and ordinary recovery bytes without
  releases. The first group also contained incomplete or different actions:
  Ctrl+Enter had no repeat, the requested Alt+Backspace was Alt+Space, and
  the modifier-first I release still carried Ctrl. These were not passed.
- A simpler physical flags `2` supplement passed all three recovery-key
  cases. Raw Linux events confirm actual Ctrl+Enter, Shift+Tab and left
  Alt+Backspace held for about two seconds. PTY captures contain their legacy
  press bytes followed by 34, 37 and 39 correct repeat sequences, respectively,
  with no recovery-key releases. This validates the P2 fix on real hardware.
- All 12 real IME checks passed. ADB touchscreen taps operated Fcitx's real
  English keyboard, Rime engine and Termux CTRL/ALT extra keys. They cover
  Ctrl+0, Alt+1, clearing one-shot modifiers before plain digits, Enter and
  Ctrl+Enter, flags `2` legacy soft-input presses, and Home/foreground return.
  During `zhongwen` composition the PTY received no bytes; tapping the visible
  `中文` candidate committed exactly its UTF-8 bytes. No direct text or key
  injection was used for these IME checks.
- This keyboard has no numeric keypad, so the P1 Num Lock transitions still
  rely on the earlier 59 framework/native-PTY checks and local regressions.
  Modifier-first release, additional physical layouts/keys, character-producing
  AltGr, other IMEs, SSH and multiplexers remain outside the new coverage.

### Review fixes (2026-10-07)

- Resolve printable Space through the keyboard layout before encoding; a
  synthetic AltGr layout now checks NBSP and Euro text, including Ctrl+AltGr
  modifier handling. Ctrl+Space and left Alt+Space keep their negotiated codes.
- Run the existing scroll actions before key encoding, so Shift+PageUp/PageDown
  retain precedence in both legacy and kitty mode. Other Page keys still reach
  the terminal application.
- Select functional-key encoding through one `KeyHandler.getCode` entry point
  used by the view and virtual Fn mappings. Original commits were reworded to
  follow the repository's commit-message requirements without changing their
  source trees; the original history is retained in the local
  `backup/kitty-pre-review-20261007` branch.
- All 150 terminal-emulator and 10 app tests passed locally (zero failures,
  errors or skips). The app suite includes four new regression tests for the
  above layout and shortcut cases. The AltGr mapping is a test fixture, not a
  claim of coverage for every Android keyboard layout.
- The revised APK (`0.118.0+kitty.65c625b2`, source `65c625b2`) was installed as
  an update on the Android 16 arm64 PTP-AN10 device, preserving application data.
  All 96 recorded checks passed; see
  [the review-fix device report](kitty-keyboard-review-device-validation.json).
- Of these, 83 checks delivered Android framework `KeyEvent` objects directly
  to the installed production `TerminalView` through a temporary instrumentation
  component, then checked the real session queue, native PTY and shell capture.
  They include Ctrl/left-Alt+Space, page-key delivery, primary-screen scrollback
  offsets, and alternate-screen arrow scrolling. These are not physical HID or
  OS input-dispatch tests.
- Two more checks verified right Alt+Space against the current virtual keyboard
  mapping in kitty and legacy modes. That mapping returns no character; both
  modes correctly sent no bytes. This does not exercise an NBSP-producing layout.
- Eleven checks used real touches on the Rime keyboard and Termux CTRL/ALT extra
  keys, including Space combinations, one-shot modifier clearing, Enter,
  background/foreground and keyboard close/open. Capture setup used temporary
  instrumentation to avoid Chinese composition of the startup command.
- The user reports completing manual combination retests on this revision across
  on-device/external physical keyboards, local Termux/SSH, and Zellij/tmux.
  This is user-reported verification; tool versions and raw captures were not
  supplied. The earlier manual report below remains historical evidence.
- The external-keyboard AltGr/NBSP case remains uncovered; the combination
  retest report does not specifically confirm a character-producing AltGr layout.

Remaining manual verification is an AltGr+Space layout that produces NBSP or
another layout character.

### Historical validation before review fixes

Validated on 2026-10-06 with an isolated temporary toolchain:

- Eclipse Temurin JDK 21.0.12.1, Gradle 9.2.1, Android API 36, Build Tools 36.0.0,
  and NDK 29.0.14206865.
- All 150 terminal-emulator unit tests passed, including the five negotiation,
  state restoration, screen isolation, parsing and encoding tests in
  `KittyKeyboardTest`.
- All six app unit tests passed, including four Robolectric API 28 tests in
  `KittyKeyboardInputTest`. Those tests exercise Android keyboard events through
  `TerminalView` into the session input queue for Ctrl+0, Ctrl+I, Ctrl+Shift+A,
  shifted punctuation, modified Enter/Tab, mode restoration, soft input, and
  application shortcut handling. They do not start a native PTY or run a shell.
- Debug APK assembly succeeded for arm64-v8a, armeabi-v7a, x86, x86_64 and the
  universal APK using the default apt-android-7 package variant.
- The initial test run caught F3 being treated as the Enter recovery key because
  both use parameter 13. Recovery handling now also checks the sequence suffix.
- On a physical PTP-AN10 device running Android 16 (arm64-v8a), the installed
  Termux APK passed 52 raw input byte comparisons: 26 combinations in kitty
  mode and the same 26 in legacy mode. Android's `input` command injected the
  key events; they passed through the production `TerminalView`, native PTY and
  a foreground shell capture. Mode queries also verified push/pop restoration
  and isolation between the main and alternate screens. The source code tested
  was commit `91ae4885`, with APK version `0.118.0+kitty.91ae4885`.
- See [the real-device report](kitty-keyboard-device-validation.json) for each
  expected and actual byte sequence. This is device execution evidence, not
  Bluetooth/USB keyboard hardware coverage.
- Seven additional checks passed using real touches on the device's Rime numeric
  keyboard and Termux CTRL extra key: Ctrl+0/1/2, plain 0 after one-shot CTRL
  clears, IME Enter, and modifier encoding after background/foreground and soft
  keyboard close/open. See [the IME report](kitty-keyboard-ime-validation.json).
  A temporary instrumentation component started the ASCII capture command to
  avoid Rime composing the setup command as Chinese; the checked input itself
  came from the real keyboard UI and crossed the native PTY. The component is
  removed after validation.
- The user additionally reports successful manual verification combining
  on-device and external physical keyboards, local Termux and SSH, and
  Zellij/tmux. Kitty-encoded input reached the receiver in the tested
  combinations. This is user-reported verification; tool versions and raw
  captures were not supplied.
- Broader keyboard layouts, IME composition and additional application or
  multiplexer versions are outside the recorded coverage. Text-only IME commits cannot expose
  physical Shift/Alt key identity that Android does not deliver to the terminal.
- `git diff --check` passed.

With the toolchain paths exported as `JAVA_HOME` and `ANDROID_HOME`, reproduce
the checks from the repository root:

```sh
./gradlew --no-daemon --max-workers=2 :terminal-emulator:testDebugUnitTest
./gradlew --no-daemon --max-workers=2 :app:testDebugUnitTest :app:assembleDebug
```

For temporary setups, keep `GRADLE_USER_HOME` and `ANDROID_USER_HOME` inside the
same temporary directory as the JDK and SDK. Retain the toolchain, APKs,
checksums and validation reports while device verification is pending. The
current event-reporting validation keeps its temporary toolchain for reuse,
following the user's explicit retention requirement.
