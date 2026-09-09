# Proposed Arabic support

English | [العربية](README.ar.md) · [Project README](../../README.md)

This contribution branch ports an earlier Arabic prototype onto upstream Termux. **Real nano 8.3/9.1 PTY editing and htop 3.2.1 Arabic-command display checks passed**, including replay through the Termux emulator and native Android Canvas on API 28/34. These are tested results, not upstream approval or official compatibility certification. This is not an official release or an accepted upstream change. See the [draft PR and evidence](UPSTREAM_PR.md#validation); consult [VALIDATION.json](VALIDATION.json) for final aggregate Gradle/build results rather than earlier snapshot counts. Previous prototype results are not evidence for this branch.

## Scope and defaults

- **English is the initial app language.** The English/Arabic UI language selector uses AndroidX per-app locales; Arabic resource translations change the surrounding interface, not shell output or installed packages.
- **Terminal flow follows a language default unless overridden:** Arabic defaults to RTL flow on; English defaults to off. With flow off, including the English default, the exact original native rendering path is retained. The optional shaping path requires Android 6 / API 23+; API 21/22 retain the unchanged native `drawText` path. This does not extend upstream package support to older Android versions.
- Arabic resources cover app strings and an array, shared UI strings and terminal-view strings. Coverage is still evolving, including service messages: hardcoded original English messages and external help are not all localized. Selecting Arabic does not translate shell/program output or change the shell locale.
- With flow enabled on API 23+, plain primary-screen prose uses whole-row RTL flow. Alternate-screen, mouse-tracking, application-cursor-key and grid-like content instead use **bounded RTL text fields within their original occupied cells**. Indented and bulleted rows also keep their host-owned layout. This is not whole-TUI mirroring or a move to the right edge of the screen.
- Android shapes Arabic directional runs; Unicode Bidi lays out mixed text inside the applicable row or field. Terminal storage, VT cell coordinates, PTY input, commands and saved text retain logical order.
- Cursor drawing, touch selection and mouse hit testing share the bounded logical-cell-to-visual-position mapping. Input and pasted text are not reversed; the host shell/editor still interprets them.

## Controls

1. Open the drawer and tap its visible **English/العربية** control to open the language picker. An explicit change to the other language clears the saved manual flow choice: Arabic then defaults to flow on, English to off, unless a valid property overrides it. Reselecting the current language does **not** clear the manual choice.
2. In the terminal context menu, toggle **RTL text flow and shaping** to save a manual override. It survives app restarts and rotation; it is not temporary. A valid explicit property disables this menu toggle.
3. For a file-level override, add this to `~/.termux/termux.properties` and run `termux-reload-settings`:

   ```properties
   terminal-rtl-text-shaping=true
   ```

   `true` requests flow; `false` forces the original renderer. The property does not select the UI language. An absent or invalid value is ignored as a policy override; it does not force English's `false` default onto Arabic.

**Precedence:** valid explicit property → persistent manual context-menu choice → language default (Arabic `true`, English `false`). Remove the property and reload to return control to the saved manual choice or language default. The optional renderer requires API 23+; the presentation policy below selects whole-row flow or bounded fields.

## TUI regression fix — 2026-09-08

Arabic shaping now stays enabled inside detected text fields in grid/TUI presentation. Headers, footers, ASCII prefixes, borders and other host-owned cells stay in place. A field is fitted to its **original occupied cell interval**, not stretched or right-aligned across the terminal. Leading ASCII or numeric tokens before Arabic remain native, including htop's `TIME+` column; embedded Latin and digits after Arabic remain part of that field's Bidi layout.

The native source glyphs and cursor replaced by a shaped field are omitted **before rasterization**, preventing overhang duplicates and stale trails rather than trying to erase them afterward. Cursor, selection and mouse mapping respect field bounds. Native wide-cell mouse handling preserves the raw protocol coordinate, and field lookup is linear-time, O(n). Native TUI arrows and Home/End remain host-managed.

## Compatibility and limitations

| Situation | Behavior / limitation |
| --- | --- |
| Plain primary-screen prose | Whole-row RTL flow when enabled on API 23+; no terminal-storage rewrite. |
| Alternate screen, mouse tracking or application cursor-key mode | Keep the host's fixed-cell grid and native key sequences while shaping Arabic in bounded occupied-cell fields. Added shaping is no longer disabled just because the presentation is a TUI. |
| Grid-like, indented or bulleted rows | Preserve layout anchors, prefixes and borders; shape only bounded fields. Detection uses row structure, including spacing and separators. It is a heuristic, not an arbitrary-widget parser: unrelated widgets separated by only one space may not be distinguished reliably. |
| ASCII/numeric prefixes and mixed fields | Leading ASCII/numeric tokens before Arabic stay native (`htop TIME+` regression). Latin/digits embedded after Arabic inside a field still participate in Unicode Bidi. |
| nano 8.3 and 9.1 | Real PTY editing passed: Right twice, insert, Delete, Backspace and save, with the expected UTF-8 file verified. Captured output also passed Termux-emulator/native-Canvas replay on API 28/34. |
| htop 3.2.1 | Actual htop output displaying an Arabic command passed capture/replay checks, including column preservation. This establishes the tested display case, not every htop interaction. |
| Claude Code CLI and Codex CLI | A **synthetic** screenshot-like fixture reproduced the reported Claude Code layout pattern and covers the indented-continuation fix. No actual authenticated Claude Code or Codex end-to-end session was tested; no official compatibility claim is made. |
| Wrapped or mixed-direction text | Bidi layout is per visible row/field, not a whole paragraph spanning wrapped rows. Punctuation and paths may differ from the author's intended presentation without directional isolation. |
| Left/right arrows | Plain whole-flow Arabic rows retain logical remapping: unmodified left sends logical forward and right logical backward. This is **not full visual Bidi traversal** within embedded Latin text. TUI/bounded-field and Latin-only rows retain native arrows. |
| Home, End, Backspace and modified keys | Native sequences remain host-managed, including in TUIs. Actual movement and deletion of letters/diacritics depend on the shell/editor, its configuration and the keyboard/IME. |
| Ligatures, emoji and devices | Caret splitting inside lam-alef is approximate. Complex emoji follow terminal cell-width rules, not full grapheme-cluster geometry. Physical-device OEM fonts, keyboards/IMEs and selection-handle dragging remain pending. |

The host owns the TUI grid: full right-screen alignment or mirroring would break that contract. Bounded shaping preserves that contract for the covered cases; it cannot infer every arbitrary widget boundary. If an unrecognized layout misbehaves, the manual off switch remains an exact-native fallback.

## Review and validation

### Tested evidence

- **Real programs:** nano 8.3 and 9.1 ran in PTYs, completed the edit/save sequence above and produced the expected UTF-8 bytes; htop 3.2.1 displayed an actual Arabic command.
- **Renderer integration:** those captures were replayed through the Termux emulator and native Android Canvas on API 28/34. Covered regressions include bounded shaping, source-glyph/cursor suppression, preserved columns/prefixes and cursor/selection/mouse mapping.
- **Synthetic evidence:** a Claude Code screenshot-like fixture covers the reported layout pattern, bullets and indented continuation rows. It is not a recording of an authenticated product session.
- **Repository checks:** see [VALIDATION.json](VALIDATION.json) and the [final-result block](UPSTREAM_PR.md#validation) for current aggregate Gradle, native-harness, resource and APK build/signing results. Do not reuse earlier snapshot counts or hashes. The read-only resource-validator workflow was added and its script passed locally; no hosted GitHub Actions run is claimed.

### Reproduce

Run from the repository root; the portable native harness uses repository-relative test/fixture paths rather than an agent workspace path:

```sh
python3 app/src/test/python/validate_arabic_resources.py
./gradlew --rerun-tasks :terminal-emulator:testDebugUnitTest :terminal-view:testDebugUnitTest :app:testDebugUnitTest :app:assembleDebug
./tools/native-tests/run.sh
# Direct Maven alternative:
mvn -f tools/native-tests/pom.xml test
```

### Still to verify on devices / outside the covered cases

- Drawer picker both ways with live PTYs, restart/rotation, actual OS upgrade and persistent locale/property behavior on physical devices; automated policy/migration checks do not replace these.
- OEM fonts, keyboard/IME and hardware-keyboard behavior, cursor blink, touch selection-handle dragging, copy/paste and host-side deletion of diacritics on devices.
- Further TUI layouts, especially arbitrary widgets separated by one space, and actual authenticated Claude Code/Codex end-to-end use.

Report the commit, Android/API version, font, keyboard, flow setting, host program and a minimal reproduction. Remove private terminal contents and credentials from logs or screenshots. Follow the [upstream debugging guidance](../../README.md#debugging).

## Fork publication safety

**Disable GitHub Actions on a newly created fork BEFORE publishing releases.** The inherited APK-attachment workflow can delete a release and its tag on failure; the JitPack workflow targets the official repository. Preserve upstream workflows in the core patch and review fork automation separately. See [workflow details](UPSTREAM_PR.md#fork-publication-safety). A separate read-only translation-validator CI workflow is included; its script passed locally, not on GitHub yet.

## Provenance and license

Baseline: [`3b66f8799635a4dba4a206563048ff0e6792c487`](https://github.com/termux/termux-app/commit/3b66f8799635a4dba4a206563048ff0e6792c487), commit date **2026-08-24**; clone date recorded for this work: **2026-09-07**. Prototype behavior informed this port; the 2026-09-08 TUI fixes and evidence above supersede the earlier grid-shaping limitation, not upstream policy. Prototype test counts were not carried forward.

The [repository license](../../LICENSE.md) remains **GPLv3-only**, including its existing Apache 2.0 exceptions for terminal code and the [module-specific licenses and exceptions in termux-shared](../../termux-shared/LICENSE.md). Original authorship and notices remain intact. No endorsement or new copyright claim is implied.
