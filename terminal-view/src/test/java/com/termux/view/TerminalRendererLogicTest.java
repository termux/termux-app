package com.termux.view;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.os.Build;

import com.termux.terminal.TerminalBuffer;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalOutput;
import com.termux.terminal.TextStyle;

import org.junit.Test;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Tests {@link TerminalRenderer} scaling caps, slot alignment, space/wide-char handling,
 * unbroken cursive shaping across style/cursor/selection boundaries, Sixel flush order, and throughput.
 */
public class TerminalRendererLogicTest {

    private static final class DummyOutput extends TerminalOutput {
        @Override public void write(byte[] data, int offset, int count) {}
        @Override public void titleChanged(String oldTitle, String newTitle) {}
        @Override public void onCopyTextToClipboard(String text) {}
        @Override public void onPasteTextFromClipboard() {}
        @Override public void onBell() {}
        @Override public void onColorsChanged() {}
    }

    private static final class DrawCall {
        final String text;
        final boolean isRtl;
        final float x;
        final int color;
        final float lastScaleX;
        final boolean clipped;
        final float clipTop;
        final float clipBottom;

        DrawCall(String text, boolean isRtl, float x, int color, float lastScaleX,
                 boolean clipped, float clipTop, float clipBottom) {
            this.text = text;
            this.isRtl = isRtl;
            this.x = x;
            this.color = color;
            this.lastScaleX = lastScaleX;
            this.clipped = clipped;
            this.clipTop = clipTop;
            this.clipBottom = clipBottom;
        }
    }

    private static final class RectCall {
        final float left;
        final float top;
        final float right;
        final float bottom;
        final int color;

        RectCall(float left, float top, float right, float bottom, int color) {
            this.left = left;
            this.top = top;
            this.right = right;
            this.bottom = bottom;
            this.color = color;
        }
    }

    private static final class RecordingCanvas extends Canvas {
        final List<DrawCall> textCalls = new ArrayList<>();
        final List<RectCall> rectCalls = new ArrayList<>();
        final List<String> ops = new ArrayList<>();
        float currentScaleX = 1.0f;
        boolean hasClipInFrame = false;
        float lastClipTop = Float.NaN;
        float lastClipBottom = Float.NaN;
        private int depth = 0;
        private final float[] scaleStack = new float[16];
        private final boolean[] clipStack = new boolean[16];

        @Override
        public int save() {
            if (depth < scaleStack.length) {
                scaleStack[depth] = currentScaleX;
                clipStack[depth] = hasClipInFrame;
            }
            depth++;
            return depth;
        }

        @Override
        public void restore() {
            if (depth > 0) {
                depth--;
                if (depth < scaleStack.length) {
                    currentScaleX = scaleStack[depth];
                    hasClipInFrame = clipStack[depth];
                }
            }
        }

        @Override
        public void scale(float sx, float sy) {
            currentScaleX *= sx;
        }

        @Override
        public boolean clipRect(float left, float top, float right, float bottom) {
            hasClipInFrame = true;
            lastClipTop = top;
            lastClipBottom = bottom;
            return true;
        }

        @Override
        public void drawRect(float left, float top, float right, float bottom, Paint paint) {
            rectCalls.add(new RectCall(left, top, right, bottom, paint != null ? paint.getColor() : 0));
        }

        @Override
        public void drawBitmap(Bitmap bitmap, Rect src, RectF dst, Paint paint) {
            ops.add("bitmap");
        }

        @Override
        public void drawTextRun(char[] text, int index, int count, int contextIndex, int contextCount,
                                float x, float y, boolean isRtl, Paint paint) {
            String str = new String(text, index, count);
            ops.add("text:" + str);
            textCalls.add(new DrawCall(str, isRtl, x, paint != null ? paint.getColor() : 0,
                    currentScaleX, hasClipInFrame, lastClipTop, lastClipBottom));
        }

        @Override
        public void drawText(char[] text, int index, int count, float x, float y, Paint paint) {
            String str = new String(text, index, count);
            ops.add("text:" + str);
            textCalls.add(new DrawCall(str, false, x, paint != null ? paint.getColor() : 0,
                    currentScaleX, hasClipInFrame, lastClipTop, lastClipBottom));
        }
    }

    @Test
    public void testRtlScaleCappedAgainstDistortion() {
        // Gentle progressive caps keep Arabic comfortable next to monospace English without wide flattening.
        assertEquals(1.18f, TerminalRenderer.computeRtlRunScaleX(5.0f, 1, 10.0f), 0.0001f);
        assertEquals(1.24f, TerminalRenderer.computeRtlRunScaleX(8.0f, 2, 10.0f), 0.0001f);
        assertEquals(1.28f, TerminalRenderer.computeRtlRunScaleX(12.0f, 3, 10.0f), 0.0001f);
        assertEquals(1.32f, TerminalRenderer.computeRtlRunScaleX(20.0f, 5, 10.0f), 0.0001f);
        assertEquals(TerminalRenderer.MAX_RTL_HORIZONTAL_STRETCH,
                TerminalRenderer.computeRtlRunScaleX(40.0f, 10, 10.0f), 0.0001f);

        // Slightly wide 25px word in 20px slot -> shrinks smoothly to 0.80f to fit without overflow
        assertEquals(0.80f, TerminalRenderer.computeRtlRunScaleX(25.0f, 2, 10.0f), 0.0001f);

        // Defensive zero handling
        assertEquals(1.0f, TerminalRenderer.computeRtlRunScaleX(0f, 5, 10f), 0.0001f);
        assertEquals(1.0f, TerminalRenderer.computeRtlRunScaleX(10f, 0, 10f), 0.0001f);
    }

    @Test
    public void testRtlRunCenterPlacementWithinGridSlot() {
        // 4-col slot [2..6] with fontWidth=10 -> slot [20..60], drawnWidth = 20 * 1.15 = 23 -> centered at 28.5
        assertEquals(28.5f, TerminalRenderer.computeRtlRunLeftX(2, 4, 10.0f, 20.0f, 1.15f), 0.001f);
        // Explicit left and right alignments inside [20..60]
        assertEquals(20.0f, TerminalRenderer.computeRtlRunLeftX(2, 4, 10.0f, 20.0f, 1.15f, TerminalRenderer.ALIGN_LEFT), 0.001f);
        assertEquals(37.0f, TerminalRenderer.computeRtlRunLeftX(2, 4, 10.0f, 20.0f, 1.15f, TerminalRenderer.ALIGN_RIGHT), 0.001f);

        // Shrunk/full-width run starts directly at slot left regardless of alignment
        assertEquals(20.0f, TerminalRenderer.computeRtlRunLeftX(2, 4, 10.0f, 50.0f, 0.8f), 0.001f);

        // Neighbor-aware alignment resolution
        BidiLayout.LogicalCell[] row = new BidiLayout.LogicalCell[10];
        for (int i = 0; i < 10; i++) {
            row[i] = new BidiLayout.LogicalCell();
            row[i].codePoint = ' ';
            row[i].displayWidth = 1;
        }
        // Prompt "$ " at [0..1] + Arabic at [2..5] + blanks [6..9] -> ALIGN_LEFT (no gap after prompt)
        row[0].codePoint = '$';
        assertEquals(TerminalRenderer.ALIGN_LEFT, TerminalRenderer.resolveRtlAlignment(row, 2, 6, 10));
        // Sandwiched between "$ " and " x" -> ALIGN_CENTER
        row[7].codePoint = 'x';
        assertEquals(TerminalRenderer.ALIGN_CENTER, TerminalRenderer.resolveRtlAlignment(row, 2, 6, 10));
        // Leading spaces [0..2] before Arabic [3..6] + " x" on right -> ALIGN_RIGHT
        row[0].codePoint = ' ';
        row[8].codePoint = 'x';
        assertEquals(TerminalRenderer.ALIGN_RIGHT, TerminalRenderer.resolveRtlAlignment(row, 3, 7, 10));
        // Col-0 Arabic [0..2] before inline English 'x' at [4] -> ALIGN_CENTER so gap before English stays balanced
        row[8].codePoint = ' ';
        row[4].codePoint = 'x';
        assertEquals(TerminalRenderer.ALIGN_CENTER, TerminalRenderer.resolveRtlAlignment(row, 0, 3, 10));
    }

    @Test
    public void testSingleSpaceBridgesArabicPhraseWhileMultiSpacePreservesColumns() {
        byte[] nl = "\r\n".getBytes(StandardCharsets.US_ASCII);

        // 1. Multi-word Arabic sentence separated by single spaces renders as ONE contiguous HarfBuzz phrase
        String phrase = "\u0645\u0631\u062D\u0628\u0627 \u0628\u0643\u0645 \u0641\u064A \u0647\u0630\u0627 \u0627\u0644\u0628\u0631\u0646\u0627\u0645\u062C";
        TerminalEmulator emPhrase = new TerminalEmulator(new DummyOutput(), 40, 2, 10, 20, 4, null);
        emPhrase.append(phrase.getBytes(StandardCharsets.UTF_8), phrase.getBytes(StandardCharsets.UTF_8).length);
        emPhrase.append(nl, nl.length);

        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
        RecordingCanvas canvas1 = new RecordingCanvas();
        renderer.render(emPhrase, canvas1, 0, -1, -1, -1, -1);

        int rtlCalls = 0;
        boolean sawWholePhrase = false;
        for (DrawCall dc : canvas1.textCalls) {
            if (dc.isRtl) {
                rtlCalls++;
                if (phrase.equals(dc.text)) sawWholePhrase = true;
            }
        }
        assertEquals("Single-spaced Arabic sentence must merge into one RTL run", 1, rtlCalls);
        assertTrue("Entire bridged Arabic phrase must be passed to HarfBuzz in logical order", sawWholePhrase);

        // 2. Multi-space padding ("   ") preserves columnar TUI spacing on LTR grid
        String padded = "\u0627\u0644\u0627\u0633\u0645   \u0627\u0644\u0642\u064A\u0645\u0629";
        TerminalEmulator emPad = new TerminalEmulator(new DummyOutput(), 25, 2, 10, 20, 4, null);
        emPad.append(padded.getBytes(StandardCharsets.UTF_8), padded.getBytes(StandardCharsets.UTF_8).length);
        emPad.append(nl, nl.length);

        RecordingCanvas canvas2 = new RecordingCanvas();
        renderer.render(emPad, canvas2, 0, -1, -1, -1, -1);

        int padRtlCalls = 0;
        boolean sawFirstWord = false;
        boolean sawSecondWord = false;
        boolean sawTripleSpaceLtr = false;
        for (DrawCall dc : canvas2.textCalls) {
            if (dc.isRtl) {
                padRtlCalls++;
                if ("\u0627\u0644\u0627\u0633\u0645".equals(dc.text)) sawFirstWord = true;
                if ("\u0627\u0644\u0642\u064A\u0645\u0629".equals(dc.text)) sawSecondWord = true;
            } else if ("   ".equals(dc.text)) {
                sawTripleSpaceLtr = true;
            }
        }
        assertEquals("Multi-space padding must split into separate RTL runs", 2, padRtlCalls);
        assertTrue(sawFirstWord && sawSecondWord && sawTripleSpaceLtr);
    }

    @Test
    public void testBridgedPhrasePartialSelectionSplitsClipsCleanlyWithoutBreakingJoining() {
        byte[] nl = "\r\n".getBytes(StandardCharsets.US_ASCII);
        String phrase = "\u0645\u0631\u062D\u0628\u0627 \u0628\u0643\u0645";
        TerminalEmulator em = new TerminalEmulator(new DummyOutput(), 20, 2, 10, 20, 4, null);
        em.append(phrase.getBytes(StandardCharsets.UTF_8), phrase.getBytes(StandardCharsets.UTF_8).length);
        em.append(nl, nl.length);

        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
        RecordingCanvas canvas = new RecordingCanvas();
        // Select visual cols [4..8] ("مرحبا") while keeping "بكم" unselected
        renderer.render(em, canvas, 0, 0, 0, 4, 8);

        int clippedRtlCalls = 0;
        for (DrawCall dc : canvas.textCalls) {
            if (dc.isRtl && phrase.equals(dc.text) && dc.clipped) {
                clippedRtlCalls++;
            }
        }
        assertTrue("Selected and unselected halves of bridged phrase must both render full context",
                clippedRtlCalls >= 2);
    }

    @Test
    public void testIsShapedRtlCellExcludesSpacesBitmapsAndWideChars() {
        BidiLayout.LogicalCell c = new BidiLayout.LogicalCell();
        c.isRtl = true;
        c.codePoint = ' ';
        c.displayWidth = 1;
        c.combiningCount = 0;
        assertFalse("Plain space must stay on LTR grid", TerminalRenderer.isShapedRtlCell(c));

        c.codePoint = 0x0645; // Arabic Meem
        assertTrue(TerminalRenderer.isShapedRtlCell(c));

        c.displayWidth = 2; // Wide char inside RTL span must keep 2-col LTR geometry
        assertFalse(TerminalRenderer.isShapedRtlCell(c));

        c.displayWidth = 1;
        c.hasBitmap = true;
        assertFalse(TerminalRenderer.isShapedRtlCell(c));
    }

    @Test
    public void testRenderKeepsArabicCursiveWordUnbrokenAcrossColorCursorAndSelection() {
        TerminalEmulator em = new TerminalEmulator(new DummyOutput(), 20, 3, 10, 20, 6, null);

        // Word "تطبيق" at cols 0..4 with DIFFERENT colors on each part + partial selection!
        String word = "\u062A\u0637\u0628\u064A\u0642";
        byte[] colored = "\033[31m\u062A\u0637\033[32m\u0628\u064A\u0642\033[0m".getBytes(StandardCharsets.UTF_8);
        em.append(colored, colored.length);

        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
        RecordingCanvas canvas = new RecordingCanvas();

        // Select visual cols [1..2] inside the 5-letter colored word
        renderer.render(em, canvas, 0, 0, 0, 1, 2);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && renderer.getFontWidth() > 0f) {
            boolean sawFullShapedWord = false;
            int rtlDrawCount = 0;
            for (DrawCall dc : canvas.textCalls) {
                if (dc.isRtl && dc.text.contains(word)) {
                    sawFullShapedWord = true;
                    rtlDrawCount++;
                    assertEquals("Each clipped segment must supply the entire connected word to HarfBuzz",
                            word, dc.text);
                    assertTrue("Clipped RTL text must be horizontally clipped across segments", dc.clipped);
                    // Vertical clip must extend strictly outside the row box so diacritics aren't sliced
                    assertTrue("Vertical clip must leave headroom above row", dc.clipBottom > dc.clipTop + renderer.getFontLineSpacing());
                }
            }
            assertTrue("Expected clipped whole-word RTL shaping calls", sawFullShapedWord && rtlDrawCount >= 2);
        }
    }

    @Test
    public void testRenderMixedSentenceDrawsArabicLogicalAndEnglishLtr() {
        TerminalEmulator em = new TerminalEmulator(new DummyOutput(), 30, 3, 10, 20, 6, null);
        // "مرحبا hello عالم"
        String input = "\u0645\u0631\u062D\u0628\u0627 hello \u0639\u0627\u0644\u0645";
        byte[] bytes = input.getBytes(StandardCharsets.UTF_8);
        em.append(bytes, bytes.length);

        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
        RecordingCanvas canvas = new RecordingCanvas();
        renderer.render(em, canvas, 0, -1, -1, -1, -1);

        boolean foundHello = false;
        boolean foundMarhaba = false;
        boolean foundAlam = false;
        for (DrawCall dc : canvas.textCalls) {
            if (dc.text.contains("hello")) {
                foundHello = true;
                assertFalse("English run must render isRtl=false", dc.isRtl);
            }
            if (dc.text.contains("\u0645\u0631\u062D\u0628\u0627")) {
                foundMarhaba = true;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) assertTrue(dc.isRtl);
            }
            if (dc.text.contains("\u0639\u0627\u0644\u0645")) {
                foundAlam = true;
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) assertTrue(dc.isRtl);
            }
        }
        assertTrue(foundHello && foundMarhaba && foundAlam);
    }

    @Test
    public void testSixelBitmapFlushesPrecedingAndFollowingTextRunsInBothPaths() {
        // Move cursor to row 1 so row 0 has no cursor splitting the "abc" run.
        byte[] nl = "\r\n".getBytes(StandardCharsets.US_ASCII);

        // Test 1: Pure LTR fast-path row: "abc" + bitmap + "xyz"
        TerminalEmulator emLtr = new TerminalEmulator(new DummyOutput(), 12, 2, 10, 20, 4, null);
        emLtr.append(nl, nl.length);
        TerminalBuffer scrLtr = emLtr.getScreen();
        long normalStyle = scrLtr.allocateFullLineIfNecessary(0).getStyle(0);
        scrLtr.setChar(0, 0, 'a', normalStyle);
        scrLtr.setChar(1, 0, 'b', normalStyle);
        scrLtr.setChar(2, 0, 'c', normalStyle);
        long bmpStyle = TextStyle.encodeTerminalBitmap(1, 0, 0);
        scrLtr.setChar(3, 0, ' ', bmpStyle);
        scrLtr.setChar(4, 0, 'x', normalStyle);
        scrLtr.setChar(5, 0, 'y', normalStyle);
        scrLtr.setChar(6, 0, 'z', normalStyle);

        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
        RecordingCanvas canvasLtr = new RecordingCanvas();
        renderer.render(emLtr, canvasLtr, 0, -1, -1, -1, -1);

        int idxAbc = -1;
        int idxXyz = -1;
        for (int i = 0; i < canvasLtr.textCalls.size(); i++) {
            String t = canvasLtr.textCalls.get(i).text;
            if (t.contains("abc")) idxAbc = i;
            if (t.contains("xyz")) idxXyz = i;
        }
        assertTrue("Text before bitmap must flush cleanly in fast path", idxAbc >= 0);
        assertTrue("Text after bitmap must draw cleanly in fast path", idxXyz > idxAbc);

        // Test 2: BiDi row: "ع" + bitmap + "xy"
        TerminalEmulator emBidi = new TerminalEmulator(new DummyOutput(), 12, 2, 10, 20, 4, null);
        emBidi.append(nl, nl.length);
        TerminalBuffer scrBidi = emBidi.getScreen();
        scrBidi.setChar(0, 0, '\u0639', normalStyle);
        scrBidi.setChar(1, 0, ' ', bmpStyle);
        scrBidi.setChar(2, 0, 'x', normalStyle);
        scrBidi.setChar(3, 0, 'y', normalStyle);

        RecordingCanvas canvasBidi = new RecordingCanvas();
        renderer.render(emBidi, canvasBidi, 0, -1, -1, -1, -1);
        boolean sawArabic = false;
        boolean sawXy = false;
        for (DrawCall dc : canvasBidi.textCalls) {
            if (dc.text.contains("\u0639")) sawArabic = true;
            if (dc.text.contains("xy")) sawXy = true;
        }
        assertTrue(sawArabic && sawXy);
    }

    @Test
    public void testPerformanceAsciiFastPathAndCachedBidiFrames() {
        final int cols = 80;
        final int rows = 24;
        TerminalEmulator asciiEm = new TerminalEmulator(new DummyOutput(), cols, rows, 10, 20, rows * 2, null);
        for (int r = 0; r < rows; r++) {
            byte[] line = ("Line " + r + ": Building release target [OK] 100% completed cleanly\r\n").getBytes(StandardCharsets.US_ASCII);
            asciiEm.append(line, line.length);
        }

        TerminalEmulator bidiEm = new TerminalEmulator(new DummyOutput(), cols, rows, 10, 20, rows * 2, null);
        for (int r = 0; r < rows; r++) {
            byte[] line = ("\u0627\u0644\u0633\u0637\u0631 " + r + " hello \u0645\u0631\u062D\u0628\u0627\r\n").getBytes(StandardCharsets.UTF_8);
            bidiEm.append(line, line.length);
        }

        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
        Canvas noopCanvas = new Canvas();

        // Warmup
        for (int i = 0; i < 20; i++) {
            renderer.render(asciiEm, noopCanvas, 0, -1, -1, -1, -1);
            renderer.render(bidiEm, noopCanvas, 0, -1, -1, -1, -1);
        }

        long t0 = System.nanoTime();
        for (int i = 0; i < 300; i++) {
            renderer.render(asciiEm, noopCanvas, 0, -1, -1, -1, -1);
        }
        long asciiMs = (System.nanoTime() - t0) / 1_000_000L;

        long t1 = System.nanoTime();
        for (int i = 0; i < 300; i++) {
            renderer.render(bidiEm, noopCanvas, 0, -1, -1, -1, -1);
        }
        long bidiCachedMs = (System.nanoTime() - t1) / 1_000_000L;

        // Generous sanity ceilings for slow devices/CI containers while verifying sub-millisecond per frame
        assertTrue("300 ASCII frames took too long: " + asciiMs + "ms", asciiMs < 3000L);
        assertTrue("300 cached BiDi frames took too long: " + bidiCachedMs + "ms", bidiCachedMs < 3000L);
    }

    private static TerminalEmulator newEmulator(int cols, int rows, String content) {
        TerminalEmulator em = new TerminalEmulator(new DummyOutput(), cols, rows, 10, 20, rows * 2, null);
        if (content != null && !content.isEmpty()) {
            byte[] bytes = content.getBytes(StandardCharsets.UTF_8);
            em.append(bytes, bytes.length);
        }
        return em;
    }

    @Test
    public void testPairedAsciiBracketsRenderInMonospaceMirroredLtrRuns() {
        // Helper predicates: paired ASCII delimiters stay monospace while Arabic punctuation shapes RTL
        char[] brackets = {'(', ')', '[', ']', '{', '}', '<', '>'};
        for (char b : brackets) {
            assertTrue(TerminalRenderer.isMirroredAsciiDelimiter(b));
            BidiLayout.LogicalCell c = new BidiLayout.LogicalCell();
            c.codePoint = b;
            c.displayWidth = 1;
            c.isRtl = true;
            assertFalse("Bracket " + b + " should stay on monospace grid", TerminalRenderer.isShapedRtlCell(c));
        }
        assertEquals('(', TerminalRenderer.getMirroredAsciiChar(')'));
        assertEquals(')', TerminalRenderer.getMirroredAsciiChar('('));
        assertEquals('[', TerminalRenderer.getMirroredAsciiChar(']'));
        assertEquals(']', TerminalRenderer.getMirroredAsciiChar('['));

        BidiLayout.LogicalCell arComma = new BidiLayout.LogicalCell();
        arComma.codePoint = '\u060C';
        arComma.displayWidth = 1;
        arComma.isRtl = true;
        assertTrue(TerminalRenderer.isShapedRtlCell(arComma));

        // Rendering "مرحبا (hello)" must produce one contiguous monospace "(hello)" run and one RTL "مرحبا" run
        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
        RecordingCanvas canvas1 = new RecordingCanvas();
        TerminalEmulator em1 = newEmulator(30, 2, "\u0645\u0631\u062D\u0628\u0627 (hello)");
        renderer.render(em1, canvas1, 0, -1, -1, -1, -1);

        boolean sawMonoParenHello = false;
        boolean sawPureArabic = false;
        for (DrawCall dc : canvas1.textCalls) {
            if (!dc.isRtl && dc.text.trim().equals("(hello)") && Math.abs(dc.lastScaleX - 1.0f) < 0.001f) {
                sawMonoParenHello = true;
            }
            if (dc.isRtl && dc.text.equals("\u0645\u0631\u062D\u0628\u0627")) {
                sawPureArabic = true;
            }
            assertFalse("Opening bracket must never glue into Arabic RTL run: " + dc.text,
                    dc.isRtl && (dc.text.contains("(") || dc.text.contains(")")));
        }
        assertTrue("Expected contiguous monospace (hello) call", sawMonoParenHello);
        assertTrue("Expected un-polluted Arabic RTL run", sawPureArabic);

        // Rendering "مرحبا [1] و [2]" must emit crisp mirrored "[2]" and "[1]" monospace runs
        RecordingCanvas canvas2 = new RecordingCanvas();
        TerminalEmulator em2 = newEmulator(30, 2, "\u0645\u0631\u062D\u0628\u0627 [1] \u0648 [2]");
        renderer.render(em2, canvas2, 0, -1, -1, -1, -1);

        boolean sawBracket2 = false;
        boolean sawBracket1 = false;
        for (DrawCall dc : canvas2.textCalls) {
            if (!dc.isRtl && dc.text.trim().equals("[2]")) sawBracket2 = true;
            if (!dc.isRtl && dc.text.trim().equals("[1]")) sawBracket1 = true;
            assertFalse("Brackets must stay out of RTL shaped runs: " + dc.text,
                    dc.isRtl && (dc.text.contains("[") || dc.text.contains("]")));
        }
        assertTrue("Expected mirrored monospace [2]", sawBracket2);
        assertTrue("Expected mirrored monospace [1]", sawBracket1);
    }

    @Test
    public void testCursorShapesPreserveStandardGeometryAndRtlGlyphBounds() {
        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
        float w = renderer.mFontWidth;

        // 1. Default BLOCK cursor on trailing space after Arabic keeps standard full-cell block shape.
        TerminalEmulator blockEm = newEmulator(20, 2, "\u0645\u0631\u062D\u0628\u0627");
        int cursorColor = blockEm.mColors.mCurrentColors[TextStyle.COLOR_INDEX_CURSOR];
        RecordingCanvas cBlock = new RecordingCanvas();
        renderer.render(blockEm, cBlock, 0, -1, -1, -1, -1);
        RectCall blockTrailing = findLastCursorRect(cBlock, cursorColor);
        assertTrue("Expected standard BLOCK cursor rect on trailing cell", blockTrailing != null);
        assertEquals(w, blockTrailing.right - blockTrailing.left, 0.5f);
        assertTrue("Trailing cell cursor should sit at col 5",
                blockTrailing.left >= 4.9f * w && blockTrailing.left <= 5.1f * w);

        // 2. BAR cursor (\e[6 q) and UNDERLINE cursor (\e[4 q) inside an Arabic word keep standard shapes
        //    positioned within the Arabic word bounds without spilling over neighboring letters.
        TerminalEmulator barEm = newEmulator(20, 2, "\u001b[6 q\u0645\u0631\u062D\u0628\u0627\u001b[3G");
        RecordingCanvas cBar = new RecordingCanvas();
        renderer.render(barEm, cBar, 0, -1, -1, -1, -1);
        RectCall barRect = findLastCursorRect(cBar, cursorColor);
        assertTrue("Expected BAR cursor rect inside Arabic word", barRect != null);
        assertTrue("BAR cursor must remain thin bar, width=" + (barRect.right - barRect.left),
                (barRect.right - barRect.left) <= w * 0.35f);
        assertTrue("BAR cursor must stay inside RTL run bounds",
                barRect.left >= 0f && barRect.right <= 5.1f * w);

        TerminalEmulator ulEm = newEmulator(20, 2, "\u001b[4 q\u0645\u0631\u062D\u0628\u0627\u001b[3G");
        RecordingCanvas cUl = new RecordingCanvas();
        renderer.render(ulEm, cUl, 0, -1, -1, -1, -1);
        RectCall ulRect = findLastCursorRect(cUl, cursorColor);
        assertTrue("Expected UNDERLINE cursor rect inside Arabic word", ulRect != null);
        assertTrue("UNDERLINE cursor height must stay thin underline, h=" + (ulRect.bottom - ulRect.top),
                (ulRect.bottom - ulRect.top) <= (blockTrailing.bottom - blockTrailing.top) * 0.4f);

        // 3. BLOCK cursor inside Arabic word covers the target glyph cleanly without overshooting run slot.
        TerminalEmulator rtlBlockEm = newEmulator(20, 2, "\u0645\u0631\u062D\u0628\u0627\u001b[3G");
        RecordingCanvas cRtlBlock = new RecordingCanvas();
        renderer.render(rtlBlockEm, cRtlBlock, 0, -1, -1, -1, -1);
        RectCall rtlBlockRect = findLastCursorRect(cRtlBlock, cursorColor);
        assertTrue("Expected BLOCK cursor rect inside Arabic word", rtlBlockRect != null);
        assertTrue("RTL BLOCK cursor width must be visible", (rtlBlockRect.right - rtlBlockRect.left) > 1.5f);
        assertTrue("RTL BLOCK cursor must stay within run slot [0..5w], got ["
                        + rtlBlockRect.left + ", " + rtlBlockRect.right + "]",
                rtlBlockRect.left >= 0f && rtlBlockRect.right <= 5.1f * w);
    }

    @Test
    public void testVisualArrowStepsMoveSmoothlyAcrossArabicAndEnglish() {
        // 1. Pure LTR identity falls back to default single-step keys.
        TerminalEmulator ltrEm = newEmulator(24, 2, "hello world");
        assertEquals(TerminalView.ARROW_STEP_FALLBACK, TerminalView.resolveVisualArrowSteps(ltrEm, -1));
        assertEquals(TerminalView.ARROW_STEP_FALLBACK, TerminalView.resolveVisualArrowSteps(ltrEm, 1));

        // 2. Pure Arabic "مرحبا" (cols 0..4, cursor at col 5): walk left visual col 5->0 and back 0->5.
        TerminalEmulator arEm = newEmulator(24, 2, "\u0645\u0631\u062D\u0628\u0627");
        int col = arEm.getCursorCol(); // 5
        assertEquals(5, col);
        int[] expectedLeftCols = {0, 1, 2, 3, 4}; // visual 4='م', 3='ر', 2='ح', 1='ب', 0='ا'
        for (int targetCol : expectedLeftCols) {
            int delta = TerminalView.resolveVisualArrowSteps(arEm, -1);
            assertTrue("Expected leftward step delta != FALLBACK/0 from col=" + col,
                    delta != TerminalView.ARROW_STEP_FALLBACK && delta != 0);
            col += delta;
            assertEquals("Left arrow should land on logical col=" + targetCol, targetCol, col);
            String move = "\u001b[" + (col + 1) + "G";
            arEm.append(move.getBytes(StandardCharsets.UTF_8), move.length());
        }
        // Pressing Left at visual col 0 stays cleanly at 'ا' instead of bouncing rightward to 'ب'.
        assertEquals(0, TerminalView.resolveVisualArrowSteps(arEm, -1));

        // Now walk Right across 'ا'->'ب'->'ح'->'ر'->'م'->' ' (col 5).
        int[] expectedRightCols = {3, 2, 1, 0, 5};
        for (int targetCol : expectedRightCols) {
            int delta = TerminalView.resolveVisualArrowSteps(arEm, 1);
            assertTrue("Expected rightward step delta != FALLBACK/0 from col=" + col,
                    delta != TerminalView.ARROW_STEP_FALLBACK && delta != 0);
            col += delta;
            assertEquals("Right arrow should land on logical col=" + targetCol, targetCol, col);
            String move = "\u001b[" + (col + 1) + "G";
            arEm.append(move.getBytes(StandardCharsets.UTF_8), move.length());
        }
        // At trailing edge col 5, Right arrow falls back cleanly (autosuggestion / EOL behavior).
        assertEquals(TerminalView.ARROW_STEP_FALLBACK, TerminalView.resolveVisualArrowSteps(arEm, 1));

        // 3. Prompt + Arabic "$ مرحبا": pressing Left from visual 'ا' stops at prompt boundary without bouncing.
        TerminalEmulator promptEm = newEmulator(24, 2, "$ \u0645\u0631\u062D\u0628\u0627\u001b[7G");
        assertEquals("At leftmost Arabic char after '$ ' prompt, Left must not cross into prompt",
                0, TerminalView.resolveVisualArrowSteps(promptEm, -1));

        // 4. Mixed prompt + Arabic + English + Arabic "$ مرحبا hi بكم":
        // Every Left press from typing edge steps strictly 1 visual column left across Arabic and English.
        TerminalEmulator mixedEm = newEmulator(30, 2, "$ \u0645\u0631\u062D\u0628\u0627 hi \u0628\u0643\u0645");
        TerminalBuffer screen = mixedEm.getScreen();
        BidiLayout mixedLayout = BidiLayout.build(
                screen.allocateFullLineIfNecessary(screen.externalToInternalRow(0)),
                30, mixedEm.getCursorCol(), false, -1, -1, -1, -1, 0, false, BidiLayout.DIR_DEFAULT);
        int curCol = mixedEm.getCursorCol(); // 14 (visual 14)
        int curVis = mixedLayout.logicalToVisual[curCol];
        while (curVis > mixedLayout.promptPrefixLength) {
            int delta = TerminalView.resolveVisualArrowSteps(mixedLayout, curCol, 30, -1);
            assertTrue("Expected non-zero step at visual=" + curVis, delta != TerminalView.ARROW_STEP_FALLBACK && delta != 0);
            curCol += delta;
            int nextVis = mixedLayout.logicalToVisual[curCol];
            assertEquals("Each Left press must move strictly 1 visual col left", curVis - 1, nextVis);
            curVis = nextVis;
        }
        assertEquals("Should stop cleanly at first editable visual cell after prompt", 0,
                TerminalView.resolveVisualArrowSteps(mixedLayout, curCol, 30, -1));

        // 5. Wide emoji + Arabic: skipping width-0 continuation emits single logical character step.
        // In LTR base "hello👍hi مرحبا", 'h' is at col 7 (v7) and '👍' spans cols 5..6 (v5..v6).
        TerminalEmulator wideLtrEm = newEmulator(24, 2, "hello\uD83D\uDC4Dhi \u0645\u0631\u062D\u0628\u0627");
        TerminalBuffer wideLtrScreen = wideLtrEm.getScreen();
        BidiLayout wideLtrLayout = BidiLayout.build(
                wideLtrScreen.allocateFullLineIfNecessary(wideLtrScreen.externalToInternalRow(0)),
                24, wideLtrEm.getCursorCol(), false, -1, -1, -1, -1, 0, false, BidiLayout.DIR_DEFAULT);
        assertEquals(-1, TerminalView.resolveVisualArrowSteps(wideLtrLayout, 7, 24, -1));
        assertEquals(1, TerminalView.resolveVisualArrowSteps(wideLtrLayout, 5, 24, 1));

        // In RTL base "مرحبا👍hi", visual order is [h(7), i(8), 👍(5,6), ا(4), ..., م(0)]:
        // 'h' is at v0 (stops on Left), '👍' at v2 steps Right onto 'ا' (col 4) skipping v3 continuation.
        TerminalEmulator wideRtlEm = newEmulator(24, 2, "\u0645\u0631\u062D\u0628\u0627\uD83D\uDC4Dhi");
        TerminalBuffer wideRtlScreen = wideRtlEm.getScreen();
        BidiLayout wideRtlLayout = BidiLayout.build(
                wideRtlScreen.allocateFullLineIfNecessary(wideRtlScreen.externalToInternalRow(0)),
                24, wideRtlEm.getCursorCol(), false, -1, -1, -1, -1, 0, false, BidiLayout.DIR_DEFAULT);
        assertEquals(0, TerminalView.resolveVisualArrowSteps(wideRtlLayout, 7, 24, -1));
        assertEquals(-1, TerminalView.resolveVisualArrowSteps(wideRtlLayout, 5, 24, 1));
    }

    @Test
    public void testInvisibleBidiFormatMarksDoNotEmitTofuOrBreakRtlRuns() {
        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
        RecordingCanvas canvas = new RecordingCanvas();
        // U+200F (RLM) attached as combining mark and U+2066 (LRI) must not render format glyphs
        TerminalEmulator em = newEmulator(20, 2, "\u0645\u0631\u062D\u0628\u0627\u200F \u0628\u0643\u0645");
        renderer.render(em, canvas, 0, -1, -1, -1, -1);
        for (DrawCall dc : canvas.textCalls) {
            assertFalse("Draw call leaked RLM control char", dc.text.indexOf('\u200F') >= 0);
            assertFalse("Draw call leaked LRI control char", dc.text.indexOf('\u2066') >= 0);
        }
        assertTrue(TerminalRenderer.isInvisibleBidiFormatChar('\u200E'));
        assertTrue(TerminalRenderer.isInvisibleBidiFormatChar('\u200F'));
        assertTrue(TerminalRenderer.isInvisibleBidiFormatChar('\u2066'));
        assertTrue(TerminalRenderer.isInvisibleBidiFormatChar('\u061C'));
    }

    private static RectCall findLastCursorRect(RecordingCanvas canvas, int cursorColor) {
        RectCall found = null;
        for (RectCall rc : canvas.rectCalls) {
            if (rc.color == cursorColor) {
                found = rc;
            }
        }
        return found;
    }
}
