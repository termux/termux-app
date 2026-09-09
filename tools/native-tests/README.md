# Standalone native renderer regression / recorded PTY harness

This source-only Maven harness compiles the **current project sources in place** from
`../..`, independently of Gradle and the Android SDK. It does not build an APK or
change production sources. Use **JDK 17**, Maven 3.9.x, and the pinned **Robolectric
4.14.1**. (4.14.1 is the Robolectric version, not a Maven version.)

## Run from any working directory

```sh
# JAVA_HOME must select a JDK 17; Maven must be on PATH, or set MVN=/path/to/mvn.
./tools/native-tests/run.sh
```

The script always runs `clean test`, writes Maven output to the terminal, and
propagates failures. Direct Maven is also supported:

```sh
mvn -B -f tools/native-tests/pom.xml clean test
```

Normal verified HTTPS Maven Central and Google repositories are configured; no
custom truststore, mirror, bundled JDK, dependency cache, or offline path is
required. First use downloads dependencies, including native Robolectric/Android
runtime jars. A supported native Robolectric host is needed; validated on Linux
x86_64 with JDK 17. No nano/htop executable is needed for replay.

### Optional cache and output flags

If Maven artifacts and the **matching Robolectric 4.14.1 instrumented Android
jars** are already cached, callers may opt into offline execution:

```sh
./tools/native-tests/run.sh -o \
  -Drobolectric.offline=true \
  -Drobolectric.dependency.dir="$ROBOLECTRIC_CACHE"
```

`-Dmaven.repo.local="$MAVEN_CACHE"` optionally selects a Maven cache. These cache
locations are caller inputs, never hardcoded into this harness. To keep even
build outputs outside the checkout:

```sh
./tools/native-tests/run.sh \
  -Dnative.tests.build.dir="$OUTPUT_DIR/native-tests" \
  > "$OUTPUT_DIR/native-tests.log" 2>&1
```

Use absolute paths for output/cache flags. `-Dfixtures=/path/to/fixtures` and
`-Devidence=/path/to/evidence` are optional overrides. `-Devidence.png=true` saves
only enabled and native-disabled PNGs; PNG saving is **off by default**, without
disabling any pixel assertions. No pre-fix renderer snapshot or before-PNG oracle
is included. Do not distribute `target/`, caches, runtime jars, or generated PNGs.

## Coverage and expected count

Two JUnit classes run on **both API 28 and API 34**, using real native Android
Canvas/Skia, not legacy shadow-graphics stand-ins:

- `terminal-view/src/testNative/java/com/termux/view/ReplacementRasterizationTest.java`
  is reused directly, not copied or renamed. Four methods × two APIs = **8** test
  invocations. It asserts source glyphs are never submitted then erased; native
  ASCII anchors survive; bottom-row italic/bold-italic cursor blink and selection
  stay within mapped bounds; reused bitmaps remove old cursors; optional native
  masks preserve backgrounds; and all-English modes retain native pixels/metrics.
- `src/test/java/com/termux/view/RealPtyNativeReplayTest.java`: five methods × two
  APIs = **10** invocations. Four full fixture replays and an independent semantic
  htop TIME+ column test. **18 invocations total; zero failures/errors/skips** is
  the expected complete suite. Filtering with `-Dtest` is not a complete run.

Fixture boundaries per API: real nano **8.3: 13**, real nano **9.1: 13**, real htop
**3.2.1: 8**, explicitly synthetic Claude-style **3**. Total **74 boundary replays**
and **46 manifest cursor assertions** across both APIs. Htop's cursor is hidden;
its manifests intentionally do not invent logical cursor expectations. Nano's
write-out prompt and exit likewise have no manifest cursor expectation.

The replay tests check:

- Recorded stream SHA-256/length, byte-identical cumulative prefixes and deltas,
  action offsets, and incremental replay in 7-byte fragments against a fresh
  emulator given the complete prefix (including split UTF-8/CSI sequences).
- Every available manifest cursor, exact logical nano editing rows/combining-mark
  selection, original-versus-saved logical UTF-8 expectations and saved SHA-256.
  Both real nano captures must match their preserved expected saved bytes.
- Display-only state preservation (rows, styles, cursor, terminal modes), unchanged
  metrics, actual RTL `Canvas.drawTextRun` calls with logical text and whole-run
  context, logical/visual cell hit round trips and bounded field allocations.
- Exact outside-field pixels against a **native-anchor-only** frame: omit native
  **source glyphs** for replacement cells, retain backgrounds and native anchors.
  Do not preserve unwanted source-Arabic overhang or enlarge allowed rectangles.
  The ordinary native-disabled frame remains the actual-pixel change baseline.
- Independent semantic htop TIME+ anchoring at column 59, separate from Arabic
  command column 67; this expectation is not derived from layout allocations.
- Fresh versus reused pixels at every boundary, forced redraw, indented Arabic,
  and restored primary-buffer state at real-program exits.

Surefire XML/text is under `target/surefire-reports/`; per-API/fixture JSON and
logical rows are under `target/evidence/` (or the selected output directory).
A fixture report alone is not the final verdict: verify both Surefire classes,
18 invocations, and **zero failures, errors, or skipped tests**.

## Standalone Android wiring

Maven cannot load nested `classes.jar` inside an AAR. The POM resolves AndroidX
Test monitor 1.7.2 and Espresso idling-resource 3.6.1 over normal Maven repositories;
`extract-aar-classes.xml` extracts only their classes into disposable build output
for the test classpath. No third-party binaries are vendored.

`compile-only/com/termux/view/R.java` is a minimal stub containing exactly the five
IDs referenced by the current view/text-selection Java sources. Its sole purpose
is to compile without Android resource linking. Values are placeholders; this
harness does not inflate UI resources, validate translations, or run selection
menus/handles. The stub is **not** an Android resource implementation and must
never be used in the APK. Production Gradle files are untouched.

See [fixtures/README.md](fixtures/README.md) for recording provenance, program
licenses, unmodified-byte policy, and explicit synthetic-fixture limitations.
