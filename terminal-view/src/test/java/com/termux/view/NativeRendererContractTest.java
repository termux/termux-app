package com.termux.view;

import org.junit.Test;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;

import static org.junit.Assert.*;

/**
 * Host-JVM source contracts for the native default, without pretending Android's mock Paint can
 * prove pixel equivalence. Run alongside the pure layout/policy tests from this source checkout.
 * Actual drawTextRun shaping, fallback fonts and selection handles still need device coverage.
 */
public class NativeRendererContractTest {
    private static String source(String name) throws IOException {
        Path directory = Paths.get(System.getProperty("user.dir")).toAbsolutePath();
        while (directory != null) {
            for (String prefix : new String[]{"src/main/java/com/termux/view/",
                "terminal-view/src/main/java/com/termux/view/"}) {
                Path path = directory.resolve(prefix + name);
                if (Files.isRegularFile(path))
                    return new String(Files.readAllBytes(path), StandardCharsets.UTF_8).replace("\r\n", "\n");
            }
            directory = directory.getParent();
        }
        throw new IOException("Run source contracts from the Termux checkout: " + name);
    }

    private static String between(String text, String from, String to) {
        int start = text.indexOf(from);
        assertTrue("Missing start marker: " + from, start >= 0);
        int end = text.indexOf(to, start + from.length());
        assertTrue("Missing end marker: " + to, end > start);
        return text.substring(start, end);
    }

    @Test public void existingRendererConstructorAndViewDefaultToNative() throws Exception {
        String renderer = source("TerminalRenderer.java");
        String constructor = between(renderer,
            "public TerminalRenderer(int textSize, Typeface typeface) {",
            "public TerminalRenderer(int textSize, Typeface typeface, boolean enabled)");
        assertTrue(constructor.contains("this(textSize, typeface, false);"));
        assertFalse(constructor.contains("this(textSize, typeface, true);"));
        assertTrue(source("TerminalView.java").contains("private boolean mRtlTextShapingEnabled = false;"));
    }

    @Test public void nativeDelegationPrecedesAnyFlowPaintingOrAllocation() throws Exception {
        String renderer = source("TerminalRenderer.java");
        String firstBranch = between(renderer, "public void render(TerminalEmulator emulator, Canvas canvas, int topRow,",
            "// The opted-in frame is always cleared");
        assertTrue(firstBranch.contains("if (!isRtlRenderingEnabled(emulator)) {"));
        assertTrue(firstBranch.contains("mNativeRenderer.render(emulator, canvas, topRow, selectionY1, selectionY2, selectionX1, selectionX2);"));
        assertTrue(firstBranch.contains("return;"));
        assertFalse(firstBranch.contains("canvas.draw"));
        assertFalse(firstBranch.contains("new BidiLineLayout"));
        assertTrue(renderer.contains("// Keep upstream rendering for ALL grid anchors"));
        assertFalse(renderer.contains("drawGridRow(")); // No replacement native renderer.
        assertFalse(renderer.contains("clearRegion(emulator, canvas, left, right, y);"));
        assertTrue(renderer.contains("selectionY1, selectionY2, selectionX1, selectionX2, omittedCells);"));
        assertTrue(renderer.indexOf("java.util.Arrays.fill(omitted, field.startColumn, field.endColumn, true)") <
            renderer.indexOf("mNativeRenderer.renderRows(emulator"));
        assertTrue(renderer.contains("canvas.clipRect(left, y - mFontLineSpacing, right, y);"));
        assertTrue(renderer.contains("drawFlowRow(emulator, canvas, line, field.text, field.layout, field.startColumn,"));
    }

    @Test public void modeToggleCannotChangeCellMetricsOrResizeThePty() throws Exception {
        String renderer = source("TerminalRenderer.java");
        assertTrue(renderer.contains("mFontWidth = mNativeRenderer.mFontWidth;"));
        assertTrue(renderer.contains("mFontLineSpacing = mNativeRenderer.mFontLineSpacing;"));
        assertTrue(renderer.contains("mFontLineSpacingAndAscent = mNativeRenderer.mFontLineSpacingAndAscent;"));
        assertFalse(renderer.contains("getFontMetrics()"));
        String setter = between(source("TerminalView.java"), "public void setRtlTextShapingEnabled(boolean enabled)",
            "/** The requested shaping/flow setting");
        assertFalse(setter.contains("updateSize("));
        assertFalse(setter.contains("mTermSession.write"));
    }

    @Test public void nativeSelectionGeometryAndMouseReportingArePreservedWhenOff() throws Exception {
        String view = source("TerminalView.java");
        assertTrue(view.contains("if (!isRtlRenderingEnabled())\n            return (int) (((y - 40) / mRenderer.mFontLineSpacing) + mTopRow);"));
        assertTrue(view.contains("if (!isRtlRenderingEnabled())\n            return Math.round((cy - mTopRow) * mRenderer.mFontLineSpacing);"));
        assertTrue(view.contains("int[] columnAndRow = getColumnAndRow(e, false);"));
        String coordinates = between(view, "public int[] getColumnAndRow(MotionEvent event, boolean relativeToScroll)",
            "/** Send a single mouse event code");
        assertTrue(coordinates.contains("if (isRtlRenderingEnabled()) {"));
        assertTrue(coordinates.contains("relativeToScroll ? row : row + mTopRow"));
        assertTrue(coordinates.contains("column = relativeToScroll ? getCursorX(event.getX(), externalRow)"));
        assertTrue(coordinates.contains(": mRenderer.mouseColumnAt(mEmulator, externalRow, event.getX());"));
        String mouse = between(source("TerminalRenderer.java"), "public int mouseColumnAt(", "public float logicalBoundaryX(");
        assertTrue(mouse.contains("if (!isRtlShapedRow(emulator, row)) return (int) (x / mFontWidth);"));
        assertTrue(mouse.contains("gridLayout(emulator, row, line).mouseColumnAt(x)"));
    }

    @Test public void onlyPlainHorizontalArrowsUseTheParagraphPolicy() throws Exception {
        String view = source("TerminalView.java");
        assertTrue(view.contains("handleKeyCode(keyCode, keyMod, event.hasNoModifiers())"));
        assertTrue(view.contains("if (plainKeyEvent && keyMod == 0 &&"));
        assertTrue(view.contains("(keyCode == KeyEvent.KEYCODE_DPAD_LEFT || keyCode == KeyEvent.KEYCODE_DPAD_RIGHT) &&"));
        assertTrue(view.contains("mRenderer != null && mRenderer.isRtlCursor(term)"));
    }

    @Test public void boundedFieldsUseFullDirectionalContextRegardlessOfStyleOrCursorClips() throws Exception {
        String renderer = source("TerminalRenderer.java");
        assertTrue(renderer.contains("canvas.drawTextRun(text, run.start, run.end - run.start, run.start, run.end - run.start,"));
        assertTrue(renderer.contains("mTextPaint.getTextRunAdvances(field.text, run.start, run.end - run.start,"));
        assertTrue(renderer.contains("int column = cell.column + columnOffset;"));
        assertTrue(renderer.contains("int otherColumn = other.column + columnOffset;"));
        assertTrue(renderer.contains("gridLayout(emulator, row, line).columnAt(x)"));
        assertTrue(renderer.contains("gridLayout(emulator, row, line).boundary(column, trailing)"));
        assertFalse(source("GridBidiLayout.java").contains("getStyle("));
        assertTrue(source("textselection/TextSelectionCursorController.java").contains("terminalView.isRtlRenderingEnabled()"));
        assertTrue(source("textselection/TextSelectionHandleView.java").contains("terminalView.isRtlRenderingEnabled()"));
        String arrows = between(renderer, "public boolean isRtlCursor(", "private GridBidiLayout gridLayout(");
        assertTrue(arrows.contains("if (!isRtlFlowEnabled(emulator)) return false;"));
        assertTrue(arrows.contains("!TerminalPresentationPolicy.looksLikeGrid("));
    }

    /** Strip only an exact reviewed adapter, retaining the original immutable upstream digest. */
    private static String undoAdapter(String source, String adapter, String original, int occurrences) {
        assertEquals("Unexpected mechanical adapter: " + adapter, occurrences,
            (source.length() - source.replace(adapter, "").length()) / adapter.length());
        return source.replace(adapter, original);
    }

    @Test public void nativeLogicMatchesOfficialSnapshotExceptDocumentedAdapters() throws Exception {
        String nativeSource = source("NativeTerminalRenderer.java");
        String original = nativeSource.replace("import android.os.Build;\n", "")
            .replace("final class NativeTerminalRenderer {", "public final class TerminalRenderer {")
            .replace("public NativeTerminalRenderer(", "public TerminalRenderer(");
        String rangeAdapter = between(original,
            "        renderRows(mEmulator, canvas, topRow, mEmulator.mRows,",
            "        final boolean reverseVideo =");
        assertEquals("        renderRows(mEmulator, canvas, topRow, mEmulator.mRows,\n" +
            "            selectionY1, selectionY2, selectionX1, selectionX2, null);\n" +
            "    }\n\n" +
            "    /** Optional viewport-relative cell mask for replacement text. Masked cells retain their\n" +
            "     * original backgrounds, but never rasterize source glyphs, cursor or selection. Breaking runs\n" +
            "     * at mask edges prevents glyph overhang from leaking outside a replacement's allocation.\n" +
            "     * A null mask (the normal public path) preserves every upstream run and paint operation. */\n" +
            "    final void renderRows(TerminalEmulator mEmulator, Canvas canvas, int topRow, int rowCount,\n" +
            "                          int selectionY1, int selectionY2, int selectionX1, int selectionX2,\n" +
            "                          boolean[][] omittedCells) {\n", rangeAdapter);
        original = original.replace(rangeAdapter, "")
            .replace("topRow + rowCount;", "topRow + mEmulator.mRows;");
        original = undoAdapter(original,
            "            boolean lastRunOmitText = false;\n" +
            "            final boolean[] omitted = omittedCells == null ? null : omittedCells[row - topRow];\n", "", 1);
        original = undoAdapter(original,
            "                final boolean omitText = omitted != null && (omitted[column] ||\n" +
            "                    (codePointWcWidth == 2 && column + 1 < columns && omitted[column + 1]));\n", "", 1);
        original = undoAdapter(original, "final boolean insideCursor = !omitText && (", "final boolean insideCursor = (", 1);
        original = undoAdapter(original, "final boolean insideSelection = !omitText && column", "final boolean insideSelection = column", 1);
        original = undoAdapter(original, " || omitText != lastRunOmitText", "", 1);
        original = undoAdapter(original, "lastRunInsideSelection, lastRunOmitText);", "lastRunInsideSelection);", 2);
        original = undoAdapter(original, "                    lastRunOmitText = omitText;\n", "", 1);
        original = undoAdapter(original, "long textStyle, boolean reverseVideo, boolean omitText)", "long textStyle, boolean reverseVideo)", 1);
        String draw = "canvas.drawTextRun(text, startCharIndex, runWidthChars, startCharIndex, runWidthChars, left, y - mFontLineSpacingAndAscent, false, mTextPaint);";
        String apiAndOmissionAdapter =
            "            // Retain paint-state updates for subsequent native measurements even when omitted.\n" +
            "            if (!omitText) {\n" +
            "                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {\n" +
            "                    " + draw + "\n" +
            "                } else {\n" +
            "                    // drawTextRun was added in API 23. Older Android keeps the native LTR grid.\n" +
            "                    canvas.drawText(text, startCharIndex, runWidthChars, left, y - mFontLineSpacingAndAscent, mTextPaint);\n" +
            "                }\n" +
            "            }";
        original = undoAdapter(original, apiAndOmissionAdapter, "            " + draw, 1);
        byte[] digest = MessageDigest.getInstance("SHA-256").digest(original.getBytes(StandardCharsets.UTF_8));
        StringBuilder hex = new StringBuilder();
        for (byte value : digest) hex.append(String.format("%02x", value & 0xff));
        // Official master 3b66f8799635a4dba4a206563048ff0e6792c487 TerminalRenderer.java.
        // A future upstream renderer update must be reviewed before deliberately updating this hash.
        assertEquals("3511efd82b87db856404191d584fc2e9526e4da303edbd6a5efff111571f3b74", hex.toString());
    }
}
