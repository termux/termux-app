# Draft upstream contribution

This is a proposed PR description, not a submitted or approved PR. Start with a **draft PR or design discussion** and ask maintainers whether to separate UI localization from terminal rendering. Acceptance is their decision. Remove preparation-only notes and fill in actual validation evidence before requesting review.

هذه مسودة للنقاش مع مشرفي المشروع، وليست طلب دمج منشوراً أو مقبولاً. يُفضّل عرض التصميم أولاً، وملء نتائج التحقق الفعلية قبل طلب المراجعة. قبول المساهمة غير مضمون.

## Proposed title

```text
Added: Add optional Arabic terminal text flow and app language selection
```

## Proposed body

### Summary / الملخص

Port the Arabic prototype onto upstream Termux while retaining English as the initial app language and the unchanged original native renderer with flow off. The visible **English/العربية** drawer control opens an AndroidX app-language picker. Arabic defaults to optional terminal RTL flow on (API 23+); English defaults to off. Resources cover app strings/an array, shared UI and terminal-view strings; localization is ongoing, including service messages. Some hardcoded original English messages and external help remain untranslated. UI selection does not translate shell/program output or change the shell locale.

نقل النموذج العربي إلى أساس Termux الأصلي، مع بقاء الإنجليزية لغة البدء والرسم الأصلي دون تغيير عند تعطيل التدفق. يفتح زر **English/العربية** في درج التنقل قائمة لغة التطبيق. العربية تفعّل التدفق الاختياري افتراضياً على API 23 فأحدث، والإنجليزية تعطّله. ترجمة موارد التطبيق والواجهة المشتركة وعرض الطرفية، ومنها رسائل الخدمة، مستمرة؛ لا تشمل كل الرسائل الإنجليزية الأصلية والمساعدة الخارجية، ولا تترجم مخرجات الصدفة والبرامج.

### Design and boundaries

- Keep terminal storage, VT coordinates and PTY input in logical order. Do not reverse commands, pasted input or saved text.
- On API 23+ with flow enabled, use whole-row RTL flow for plain primary-screen prose. Alternate-screen, mouse-tracking, application-cursor-key, grid-like, indented and bulleted presentation uses **bounded RTL fields within the original occupied cells**, not whole-TUI mirroring or full right-screen alignment. The host owns the grid.
- Shape directional runs and share bounded visual mapping between rendering, cursor, touch selection and mouse hit testing. Preserve headers, footers, ASCII prefixes, borders and other host-owned cells.
- Keep the exact original native renderer on the disabled/English-default path. API 21/22 retain the unchanged native `drawText` path; they do not enter API 23+ shaping.
- Resolve flow in this order: **valid explicit `terminal-rtl-text-shaping` property → persistent manual context-menu choice → language default** (Arabic `true`, English `false`). A valid explicit property disables the menu toggle; absent/invalid values do not override this policy.
- An explicit drawer change to the other language clears the saved manual flow choice; reselecting the current language does not. Manual choices survive restart/rotation until such a language change. A property override still wins after a language change.
- Preserve leading ASCII/numeric tokens before Arabic as native cells (including htop `TIME+`); embedded Latin/digits after Arabic inside a field still use Bidi.
- Retain native TUI arrows and Home/End. Backspace and modified keys remain host-managed. Only plain whole-flow Arabic rows use logical left/right remapping, not full visual Bidi traversal.
- Document per-row/field Bidi and wrapping, approximate ligature carets and font/IME dependence. Field detection is heuristic, not a parser for arbitrary widgets: unrelated widgets separated by only one space may not be distinguished reliably.
- Distinguish proven cases (real nano PTY edit/save and htop display, emulator/native-Canvas replay), synthetic Claude Code screenshot-pattern coverage, pending physical-device/authenticated-product tests and upstream approval. Tested fixes are not an official compatibility certification.

### TUI regression fix — 2026-09-08

Bounded field shaping replaces the earlier shaping-off grid fallback without moving the host's layout. Indented continuation and bulleted rows retain their anchors. Native source glyphs and the cursor replaced by a shaped field are omitted **before rasterization**, preventing glyph-overhang duplicates and stale trails rather than erasing them afterward. Cursor/selection/mouse mapping stays within field bounds. Native wide-cell mouse handling preserves the raw protocol coordinate, and field lookup is linear-time, O(n).

Details: [English](README.en.md) · [العربية](README.ar.md).

### Baseline and provenance

- Upstream: `termux/termux-app`, commit [`3b66f8799635a4dba4a206563048ff0e6792c487`](https://github.com/termux/termux-app/commit/3b66f8799635a4dba4a206563048ff0e6792c487), dated 2026-08-24.
- Clone date recorded for this preparation: 2026-09-07.
- Earlier Arabic prototype behavior and limitations inform this port; its test results do not validate the rebased branch.
- AI assistance was used to prepare code and documentation. Human review, provenance checks and validation are still required; this note does not claim sole human authorship or official translator status.
- Preserve GPLv3-only and existing module/file license exceptions and notices. This is not a relicensing proposal.

### VALIDATION

**TUI regression evidence, 2026-09-08:** tested behavior below is distinct from upstream approval. See [machine-readable results](VALIDATION.json) for the aggregate validation record; the final source/build snapshot is recorded below; rerun on the eventual submission commit.

| Evidence class | Result and boundary |
| --- | --- |
| Real nano 8.3 and 9.1 PTY editing | **Passed:** Right twice, insert, Delete, Backspace, save; the saved file matched the expected UTF-8 bytes. These were real nano processes, not synthetic editor fixtures. |
| Real htop 3.2.1 display | **Passed:** actual htop output displaying an Arabic command; captured columns and the leading numeric `TIME+` field stayed native. This is display evidence, not a claim about every htop interaction. |
| Termux emulator + native Android Canvas | **Passed:** replay of real-program captures through the emulator and native Android graphics on API 28/34, covering bounded shaping and preserved grid positions. This is host-side native-renderer evidence, not physical-device testing. |
| Grid/rendering regressions | Covered bounded fields, leading ASCII/numeric prefixes, embedded Bidi text, native source-glyph/cursor omission before rasterization, cursor/selection/mouse bounds and native wide-cell raw-protocol mouse handling. |
| Claude Code screenshot-like fixture | **Passed synthetic reproduction:** the reported layout pattern and indented continuation/bullet regression were reproduced and fixed. This was not an actual authenticated Claude Code session. |
| Claude Code / Codex end-to-end | Not tested in actual authenticated sessions. No official product compatibility certification or upstream approval is claimed. |
| Locale / property coverage | Policy, persistent preferences, resource contexts, delayed AppCompat migration fixtures and service authorization have automated Robolectric coverage; see the final aggregate results below. |
| CI | Read-only Arabic resource and native renderer workflows added; checks passed locally. No hosted GitHub Actions run is claimed. |
| Physical devices / remaining coverage | Pending: real OS upgrade, picker with live PTY sessions on a phone, OEM fonts, keyboard/IME behavior and selection-handle dragging; arbitrary-widget layouts separated by a single space also need broader coverage. |
| Upstream status | Proposed change only; passing tests do not establish submission, maintainer acceptance or official release status. |

<!-- BEGIN FINAL_VALIDATION_RESULTS -->
**Final local verification, 2026-09-08: 304 test invocations passed, zero failures/errors/skips.** [Machine-readable results](VALIDATION.json).

- Repository Gradle tests: **286** (145 emulator, 88 terminal-view, 53 app).
- Portable native harness: **18** on API 28/34, replaying **74 boundaries**, checking **46 cursor positions**, **4,256 hit round trips** and real nano UTF-8 saves. nano 8.3 and 9.1 plus htop 3.2.1 are real process captures; the Claude-style fixture is synthetic.
- The initial API28 nano9.1 comparison included unwanted native source-glyph overhang in its expected pixels. The corrected oracle compares native UI anchors without replaced source glyphs, still requiring zero outside-field mismatches; no pixel-error tolerance was introduced.
- Native tests also passed from a relocated checkout whose path contains spaces. Tests and exact recorded fixtures are included in `tools/native-tests`.
- Resources: 67 XML files plus manifest; 113 app strings + one array, 55 shared strings and 3 terminal-view strings; parity validation passed.
- Full Gradle debug APK build passed. apksigner and zipalign passed. Artifact: `termux-arabic-tui-fixed-arm64.apk` (38482517 bytes), SHA-256 `c476e6386889b2d4af22bbed6757c8ed3c5d8335b65b24b7f34211afcdff5429`.
- Signing certificate SHA-256: `b6da01480eefd5fbf2cd3771b8d1021ec791304bdd6c4bf41d3faabad48ee5e1`. Same public untrusted test key, not a production release certificate.
- Source Java/XML/Python manifest SHA-256: `d802b73eec54b6ccd2e094120321944a3a4a8cff56a00a8e54c0eecd185d47f6`. Based on upstream `3b66f8799635a4dba4a206563048ff0e6792c487` plus this uncommitted patch.
- JDK 17.0.20.1, Gradle 9.2.1, compile SDK 36, NDK 29.0.14206865; native harness Robolectric 4.14.1.
- Both resource and native-renderer workflows have read-only permissions; their checks passed locally. Hosted GitHub Actions have not run.
- Not verified: a physical Android phone, OEM/IME/handle interactions, every TUI layout, authenticated Claude Code/Codex sessions, or official upstream approval.
<!-- END FINAL_VALIDATION_RESULTS -->

Reproduce from the repository root. The portable harness in `tools/native-tests` uses repository-relative test and fixture paths, not an agent-specific workspace:

```sh
python3 app/src/test/python/validate_arabic_resources.py
./gradlew --rerun-tasks :terminal-emulator:testDebugUnitTest :terminal-view:testDebugUnitTest :app:testDebugUnitTest :app:assembleDebug
./tools/native-tests/run.sh
# Direct Maven alternative:
mvn -f tools/native-tests/pom.xml test
```

Migration tests use real AppCompat migration with delayed-worker fixtures and compensate for Robolectric's per-context LocaleManager model. They do not prove a physical Android upgrade. Native-renderer replay does not prove OEM font, keyboard or selection-handle behavior. APK signing is not contributor identity or DCO/sign-off verification; the public untrusted debug key is not a production release key.

نجح تحرير nano 8.3 و9.1 الفعلي وحفظ UTF-8 المتوقع، وعرض أمر عربي في htop 3.2.1، وإعادة تمرير المخرجات عبر محاكي Termux وCanvas الأصلي على API 28/34. تغطية نمط Claude Code اصطناعية؛ لم تُختبر جلسات Claude/Codex فعلية موثّقة الدخول من البداية إلى النهاية. تبقى فحوص الأجهزة وخطوط المصنّعين ولوحات المفاتيح ومقابض التحديد قيد الانتظار. النتائج المجمعة النهائية موثقة في الكتلة أعلاه وVALIDATION.json. هذه أدلة اختبار، وليست موافقة المشروع الأصلي؛ لا تُعرض شاراته كدليل على نجاح هذا الفرع.

### Questions for maintainers

1. Should UI localization and optional flow be reviewed as separate changes?
2. Are whole-row prose flow and bounded occupied-cell RTL fields, with the documented single-space widget-boundary heuristic limit, suitable for an experimental optional feature?
3. Which device, font, IME and TUI coverage is required before review?

## Preparation checklist

- [ ] Review every changed file against the baseline; keep unrelated upstream files intact.
- [ ] Confirm English defaults, original-renderer equivalence, language-change reset vs same-language reselect, persistent manual choices and property precedence/disabled menu behavior.
- [x] Add the read-only translation-validator CI workflow; its script passed locally. Inherited release workflows remain unchanged.
- [ ] Review Arabic strings in context with Arabic-speaking reviewers; do not claim official translation approval.
- [x] Record real nano/htop and native-Canvas replay evidence, with synthetic Claude Code coverage separately labeled.
- [x] Finalize the current snapshot, portable harness and results. Rerun on the actual submission commit before review.
- [ ] Review AI-assisted changes, imported code provenance, notices and applicable licenses.
- [ ] Follow upstream shared-constant/util and relevant changelog guidance.
- [ ] Use the upstream commit-message convention below.
- [ ] Exclude `docs/arabic/FORK_MAINTAINER.md` from the upstream patch; it is optional fork-only personal attribution/contact, not product documentation.
- [ ] Inspect the final patch for prototype build outputs, reports, credentials and unrelated cleanup.
- [ ] Discuss the design in draft form before requesting a merge; check the contribution requirements current at submission time. Add only attestations/sign-offs the contributor can truthfully make.

## Commit-message convention

The [upstream README](../../README.md#commit-messages-guidelines) requires Conventional Commits with a **capitalized type and description**, present-tense description, and a space after the colon. Allowed types are exactly `Added`, `Changed`, `Deprecated`, `Removed`, `Fixed`, and `Security`; do not substitute lowercase `feat` or `fix`.

Examples for changes actually made:

```text
Added: Add English and Arabic app language selection
Added(terminal): Add optional Arabic text flow
Fixed(terminal): Preserve shaping context during selection
```

These are suggested messages, not a claim that commits or sign-offs have been created.

## Fork publication safety

**Disable GitHub Actions on a newly created fork BEFORE publishing any releases.** Keep the inherited upstream workflows intact in the core contribution patch; fork operational settings are separate from the proposed feature.

- [`attach_debug_apks_to_release.yml`](../../.github/workflows/attach_debug_apks_to_release.yml) runs on published releases. Its failure handler can **delete both the release and its tag**, including after version validation, build or attachment failures.
- [`trigger_library_builds_on_jitpack.yml`](../../.github/workflows/trigger_library_builds_on_jitpack.yml) targets the official `termux/termux-app` coordinates, not the new fork.
- Review release automation, permissions, signing and target repositories before enabling any fork workflows. A new read-only translation-validator CI workflow is included; it does not make inherited release automation safe for a fork.

**عطّل GitHub Actions في الفرع الجديد قبل نشر أي إصدار.** يمكن لإجراء إرفاق APK الموروث حذف الإصدار ووسمه عند الفشل، وإجراء JitPack يستهدف المشروع الرسمي. تُحفظ إجراءات المشروع الأصلي دون تغيير في الرقعة الأساسية؛ تُراجع إعدادات نشر الفرع بصورة منفصلة.

## Cleanup inventory

The official baseline has **no `.gitlab-ci.yml`**. Do not import the prototype's GitLab CI file; there is no upstream GitLab file to remove. Preserve official `docs/`, `site/`, `art/`, `fastlane/` and `.github/` content. Do not add duplicate licenses, templates or unrelated scaffolding, or carry prototype build outputs/test counts into the contribution.

## Upstream policy references

Read from the checked-out official baseline, not inferred approval:

- [Contributor and commit guidance at the baseline](https://github.com/termux/termux-app/blob/3b66f8799635a4dba4a206563048ff0e6792c487/README.md#for-maintainers-and-contributors): shared constants/utilities, relevant changelogs, external-code licenses and exact capitalized commit types.
- [Repository license at the baseline](https://github.com/termux/termux-app/blob/3b66f8799635a4dba4a206563048ff0e6792c487/LICENSE.md): GPLv3-only with stated exceptions.
- [termux-shared license at the baseline](https://github.com/termux/termux-app/blob/3b66f8799635a4dba4a206563048ff0e6792c487/termux-shared/LICENSE.md): module/file-specific MIT, GPLv3-only, GPLv2 with Classpath exception and Apache 2.0 provisions.

These references do not grant approval for this feature or personal contact material. No separate CONTRIBUTING file was found in this baseline; check upstream guidance again before submission.
