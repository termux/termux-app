# Arabic contribution changelog / سجل المساهمة العربية

## 2026-09-08 — TUI regression fix, proposed and not upstream-approved

### Fixed
- Keep exact native rendering with flow off (including the English default), and the unchanged native `drawText` path on API 21/22. With Arabic flow enabled on API 23+, plain primary-screen prose retains whole-row RTL flow.
- Shape Arabic in bounded RTL fields within the **original occupied cells** for alternate-screen, mouse-tracking, application-cursor-key, grid-like, indented and bulleted rows. Preserve headers, footers, ASCII prefixes and borders; no whole-TUI mirroring, full right-screen alignment or text-storage rewrite.
- Keep leading ASCII/numeric tokens before Arabic native, including the htop `TIME+` regression; embedded Latin/digits after Arabic remain in the field's Bidi layout.
- Omit native source glyphs and the cursor replaced by shaped fields **before rasterization**, preventing overhang duplicates and stale trails.
- Keep cursor/selection/mouse mapping inside field bounds and preserve native TUI arrows and Home/End. Fix native wide-cell mouse raw-protocol coordinates and use linear-time O(n) field lookup.
- Fix screenshot-like bulleted/indented continuation layout in a **synthetic** Claude Code fixture.

### Validation and limits
- Real nano **8.3 and 9.1** PTY editing passed: Right twice, insert, Delete, Backspace and save, with expected UTF-8 bytes verified. Real **htop 3.2.1** displayed an Arabic command; capture replay through the Termux emulator and native Android Canvas on **API 28/34** passed.
- Document portable native-harness reproduction under `tools/native-tests` with repository-relative paths. See [VALIDATION.json](VALIDATION.json) and [the final-result block](UPSTREAM_PR.md#validation) for final aggregate results; do not reuse earlier counts or APK hashes.
- No actual authenticated Claude Code/Codex end-to-end test or official compatibility approval is claimed. OEM fonts, keyboards/IMEs and selection-handle dragging on physical devices remain pending. Arbitrary widgets separated by only one space remain a field-detection heuristic limit.

### ملخص عربي
أُصلح وصل العربية داخل حقول محدودة بخلاياها المشغولة الأصلية في واجهات الطرفية، مع حفظ الترويسات والتذييلات والبادئات والحدود ومفاتيح المضيف. تبقى بادئات ASCII والأرقام السابقة للعربية أصلية، ومنها `TIME+`، ويستمر Bidi للنص المضمّن داخل الحقل. يُمنع رسم المحارف والمؤشر المستبدلين قبل تحويلهما إلى بكسلات لتجنّب التكرار والآثار؛ أُصلحت حدود المؤشر والتحديد والفأرة وإحداثيات بروتوكول الخلايا العريضة، وأصبح البحث خطياً O(n). نجح تحرير nano 8.3 و9.1 الفعلي وحفظ UTF-8 المتوقع وعرض أمر عربي في htop 3.2.1 وإعادة التمرير عبر محاكي Termux وCanvas الأصلي على API 28/34. نموذج Claude Code اصطناعي، لا جلسة موثّقة الدخول؛ تبقى فحوص الأجهزة ولوحات المفاتيح ومقابض التحديد وحدود العناصر المفصولة بمسافة واحدة بحاجة إلى تحقق أوسع. لا عكس للشاشة ولا تغيير للنص المخزّن، ولا اعتماد رسمي للمساهمة.

## Unreleased — proposed, not upstream-approved

### Added
- English/Arabic app-language picker and Arabic resources, including service notifications.
- Optional Arabic shaping/Bidi flow with shared cursor/selection mapping; bounded TUI fields supersede the earlier shaping-off grid fallback.
- Persistent flow preference and explicit termux.properties override.
- Layout, locale-migration, resource-parity and service-authorization regression coverage.
- Read-only Arabic resource validation workflow.

### Fixed
- Avoid overwriting stored Arabic during delayed AndroidX-to-framework locale migration.
- Keep service resource selection consistent with the chosen app language.
- Preserve native rendering defaults and PTY cell metrics while switching display modes.

## ملخص عربي
أُضيف مبدّل اللغة والترجمة وتدفق العربية الاختياري واختباراته. أُصلح حفظ العربية أثناء ترحيل إعدادات اللغة، وتوحيد لغة رسائل الخدمة، مع إبقاء الرسم الأصلي وإحداثيات الطرفية عند تعطيل التدفق. التفاصيل والحدود ونتائج التحقق في الدليلين ومسودة طلب الدمج. لم يُنشر إصدار رسمي.
