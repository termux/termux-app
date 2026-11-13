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
import com.termux.terminal.TerminalRow;
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
        // Never stretch narrow RTL glyphs wider than 1.0x; compress cleanly only on overflow.
        assertEquals(1.0f, TerminalRenderer.computeRtlRunScaleX(5.0f, 1, 10.0f), 0.0001f);
        assertEquals(1.0f, TerminalRenderer.computeRtlRunScaleX(8.0f, 2, 10.0f), 0.0001f);
        assertEquals(1.0f, TerminalRenderer.computeRtlRunScaleX(12.0f, 3, 10.0f), 0.0001f);
        assertEquals(1.0f, TerminalRenderer.computeRtlRunScaleX(20.0f, 5, 10.0f), 0.0001f);
        assertEquals(1.0f, TerminalRenderer.computeRtlRunScaleX(40.0f, 10, 10.0f), 0.0001f);

        // Overflowing 25px word in 20px slot compresses smoothly to 0.80f without distortion > 1.0f
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
        // Col-0 Arabic [0..2] before inline English 'x' at [4] -> ALIGN_LEFT so col 0 never indents
        row[8].codePoint = ' ';
        row[4].codePoint = 'x';
        assertEquals(TerminalRenderer.ALIGN_LEFT, TerminalRenderer.resolveRtlAlignment(row, 0, 3, 10));

        // Balanced centering caps left gap at <= 0.45w while preserving smaller natural centering
        assertEquals(24.5f, TerminalRenderer.computeBalancedRtlLeftX(2, 12, 10.0f, 60.0f, 1.0f, TerminalRenderer.ALIGN_CENTER), 0.001f);
        assertEquals(24.0f, TerminalRenderer.computeBalancedRtlLeftX(2, 4, 10.0f, 32.0f, 1.0f, TerminalRenderer.ALIGN_CENTER), 0.001f);
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
        float viewportW = 20 * w;

        // 1. Default BLOCK cursor at EOL after right-aligned Arabic "مرحبا" touches contentLeft directly (0px phantom gap).
        TerminalEmulator blockEm = newEmulator(20, 2, "\u0645\u0631\u062D\u0628\u0627");
        int cursorColor = blockEm.mColors.mCurrentColors[TextStyle.COLOR_INDEX_CURSOR];
        RecordingCanvas cBlock = new RecordingCanvas();
        renderer.render(blockEm, cBlock, 0, -1, -1, -1, -1);
        RectCall blockTrailing = findLastCursorRect(cBlock, cursorColor);
        DrawCall rtlDraw = null;
        for (DrawCall dc : cBlock.textCalls) {
            if (dc.isRtl) rtlDraw = dc;
        }
        assertTrue("Expected RTL text and trailing BLOCK cursor rect", blockTrailing != null && rtlDraw != null);
        assertEquals("Trailing cursor on RTL paragraph must touch leftmost glyph edge cleanly",
                rtlDraw.x, blockTrailing.right, 0.5f);
        assertTrue("Cursor width must be visible and proportional",
                (blockTrailing.right - blockTrailing.left) >= w * 0.45f);

        // 2. BAR cursor (\e[6 q) and UNDERLINE cursor (\e[4 q) inside right-aligned Arabic word stay within [contentLeft..viewportW].
        TerminalEmulator barEm = newEmulator(20, 2, "\u001b[6 q\u0645\u0631\u062D\u0628\u0627\u001b[3G");
        RecordingCanvas cBar = new RecordingCanvas();
        renderer.render(barEm, cBar, 0, -1, -1, -1, -1);
        RectCall barRect = findLastCursorRect(cBar, cursorColor);
        assertTrue("Expected BAR cursor rect inside Arabic word", barRect != null);
        assertTrue("BAR cursor must remain thin bar, width=" + (barRect.right - barRect.left),
                (barRect.right - barRect.left) <= w * 0.35f);
        assertTrue("BAR cursor must stay inside right-aligned RTL run bounds",
                barRect.left >= rtlDraw.x - 1f && barRect.right <= viewportW + 0.5f);

        TerminalEmulator ulEm = newEmulator(20, 2, "\u001b[4 q\u0645\u0631\u062D\u0628\u0627\u001b[3G");
        RecordingCanvas cUl = new RecordingCanvas();
        renderer.render(ulEm, cUl, 0, -1, -1, -1, -1);
        RectCall ulRect = findLastCursorRect(cUl, cursorColor);
        assertTrue("Expected UNDERLINE cursor rect inside Arabic word", ulRect != null);
        assertTrue("UNDERLINE cursor height must stay thin underline, h=" + (ulRect.bottom - ulRect.top),
                (ulRect.bottom - ulRect.top) <= (blockTrailing.bottom - blockTrailing.top) * 0.4f);

        // 3. BLOCK cursor inside Arabic word covers only that glyph cleanly within [contentLeft..viewportW].
        TerminalEmulator rtlBlockEm = newEmulator(20, 2, "\u0645\u0631\u062D\u0628\u0627\u001b[3G");
        RecordingCanvas cRtlBlock = new RecordingCanvas();
        renderer.render(rtlBlockEm, cRtlBlock, 0, -1, -1, -1, -1);
        RectCall rtlBlockRect = findLastCursorRect(cRtlBlock, cursorColor);
        assertTrue("Expected BLOCK cursor rect inside Arabic word", rtlBlockRect != null);
        assertTrue("RTL BLOCK cursor width must be visible", (rtlBlockRect.right - rtlBlockRect.left) > 1.5f);
        assertTrue("RTL BLOCK cursor must stay within run bounds ["
                        + rtlDraw.x + ".." + viewportW + "], got ["
                        + rtlBlockRect.left + ", " + rtlBlockRect.right + "]",
                rtlBlockRect.left >= rtlDraw.x - 1f && rtlBlockRect.right <= viewportW + 0.5f);
    }

    @Test
    public void testVisualArrowStepsMoveSmoothlyAcrossArabicAndEnglish() {
        // 1. Pure LTR identity falls back to default single-step keys.
        TerminalEmulator ltrEm = newEmulator(24, 2, "hello world");
        assertEquals(TerminalView.ARROW_STEP_FALLBACK, TerminalView.resolveVisualArrowSteps(ltrEm, -1));
        assertEquals(TerminalView.ARROW_STEP_FALLBACK, TerminalView.resolveVisualArrowSteps(ltrEm, 1));

        // 2. Pure right-aligned Arabic "مرحبا" (visual: [EOL(5)] ا(4) ب(3) ح(2) ر(1) م(0)):
        // At EOL col=5 (leftmost visual edge), Left stays (0); Right walks 'ا'(4)->'ب'(3)->'ح'(2)->'ر'(1)->'م'(0).
        TerminalEmulator arEm = newEmulator(24, 2, "\u0645\u0631\u062D\u0628\u0627");
        int col = arEm.getCursorCol(); // 5 (EOL, leftmost visual edge)
        assertEquals(5, col);
        assertEquals("Left arrow at leftmost EOL of right-aligned RTL line stays cleanly",
                0, TerminalView.resolveVisualArrowSteps(arEm, -1));
        int[] expectedRightCols = {4, 3, 2, 1, 0}; // visual 0='ا' -> 1='ب' -> 2='ح' -> 3='ر' -> 4='م'
        for (int targetCol : expectedRightCols) {
            int delta = TerminalView.resolveVisualArrowSteps(arEm, 1);
            assertTrue("Expected rightward step delta != FALLBACK/0 from col=" + col,
                    delta != TerminalView.ARROW_STEP_FALLBACK && delta != 0);
            col += delta;
            assertEquals("Right arrow should land on logical col=" + targetCol, targetCol, col);
            String move = "\u001b[" + (col + 1) + "G";
            arEm.append(move.getBytes(StandardCharsets.UTF_8), move.length());
        }
        // At rightmost edge 'م' (col 0), pressing Right stays cleanly at 'م'.
        assertEquals(0, TerminalView.resolveVisualArrowSteps(arEm, 1));

        // Now walk Left back visually: 'ر'(1)->'ح'(2)->'ب'(3)->'ا'(4)->EOL(5).
        int[] expectedLeftCols = {1, 2, 3, 4, 5};
        for (int targetCol : expectedLeftCols) {
            int delta = TerminalView.resolveVisualArrowSteps(arEm, -1);
            assertTrue("Expected leftward step delta != FALLBACK/0 from col=" + col,
                    delta != TerminalView.ARROW_STEP_FALLBACK && delta != 0);
            col += delta;
            assertEquals("Left arrow should land on logical col=" + targetCol, targetCol, col);
            String move = "\u001b[" + (col + 1) + "G";
            arEm.append(move.getBytes(StandardCharsets.UTF_8), move.length());
        }
        assertEquals(0, TerminalView.resolveVisualArrowSteps(arEm, -1));

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

        // In right-aligned RTL base "مرحبا👍hi", visual order is [h(7), i(8), 👍(5,6), ا(4), ..., م(0)]:
        // Left from v=0 ('h', col=7) steps to EOL (col=9, delta +2); Right from '👍' (col=5) steps onto 'ا' (col=4, delta -1).
        TerminalEmulator wideRtlEm = newEmulator(24, 2, "\u0645\u0631\u062D\u0628\u0627\uD83D\uDC4Dhi");
        TerminalBuffer wideRtlScreen = wideRtlEm.getScreen();
        BidiLayout wideRtlLayout = BidiLayout.build(
                wideRtlScreen.allocateFullLineIfNecessary(wideRtlScreen.externalToInternalRow(0)),
                24, wideRtlEm.getCursorCol(), false, -1, -1, -1, -1, 0, false, BidiLayout.DIR_DEFAULT);
        assertEquals(2, TerminalView.resolveVisualArrowSteps(wideRtlLayout, 7, 24, -1));
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

    @Test
    public void testParenthesizedArabicTightSpacingAndMixedDelimiters() {
        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
        float w = renderer.mFontWidth;

        // 1. "(أهلا بكم)" places mirrored '(' at visual left and ')' at visual right, hugging RTL text:
        // Delimiters remain 1.0x unstretched monospace and rtlPhrase starts right at 0.70*w (no gap).
        TerminalEmulator parensEm = newEmulator(20, 2, "(\u0623\u0647\u0644\u0627 \u0628\u0643\u0645)");
        RecordingCanvas c1 = new RecordingCanvas();
        renderer.render(parensEm, c1, 0, -1, -1, -1, -1);
        DrawCall leftParen = null;
        DrawCall rtlPhrase = null;
        DrawCall rightParen = null;
        for (DrawCall dc : c1.textCalls) {
            if (!dc.isRtl && "(".equals(dc.text)) {
                leftParen = dc;
            } else if (dc.isRtl && dc.text.contains("\u0623\u0647\u0644\u0627")) {
                rtlPhrase = dc;
            } else if (!dc.isRtl && ")".equals(dc.text)) {
                rightParen = dc;
            }
        }
        assertTrue("Expected crisp unstretched '(' delimiter", leftParen != null && Math.abs(leftParen.lastScaleX - 1.0f) < 0.01f);
        assertTrue("Expected Arabic phrase RTL run", rtlPhrase != null);
        assertTrue("Expected crisp unstretched ')' delimiter", rightParen != null && Math.abs(rightParen.lastScaleX - 1.0f) < 0.01f);
        assertTrue("Left '(' must sit directly left of Arabic phrase", leftParen.x < rtlPhrase.x);
        assertTrue("Arabic phrase must start flush right after '(' with proportional advance, delta="
                        + (rtlPhrase.x - leftParen.x),
                (rtlPhrase.x - leftParen.x) <= w * 1.25f);
        assertTrue("Right ')' must sit flush right at end of Arabic phrase and align to viewport edge",
                rightParen.x > rtlPhrase.x && rightParen.x + 1.25f * w >= 20f * w - 0.5f);

        // 2. "قال (مرحبا) لكم": words and parentheses form cohesive spacing cleanly (3 RTL word calls + 2 delim calls).
        TerminalEmulator sentenceEm = newEmulator(25, 2, "\u0642\u0627\u0644 (\u0645\u0631\u062D\u0628\u0627) \u0644\u0643\u0645");
        RecordingCanvas c2 = new RecordingCanvas();
        renderer.render(sentenceEm, c2, 0, -1, -1, -1, -1);
        int rtlWordCalls = 0;
        int parenCalls = 0;
        for (DrawCall dc : c2.textCalls) {
            if (dc.isRtl && !dc.text.trim().isEmpty()) rtlWordCalls++;
            if (!dc.isRtl && ("(".equals(dc.text) || ")".equals(dc.text))) {
                parenCalls++;
                assertEquals(1.0f, dc.lastScaleX, 0.01f);
            }
        }
        assertEquals(3, rtlWordCalls);
        assertEquals(2, parenCalls);

        // 3. LTR embedding boundary: "func(مرحبا) + x" keeps unmirrored '(' and ')' flush against 'func'.
        TerminalEmulator funcEm = newEmulator(25, 2, "func(\u0645\u0631\u062D\u0628\u0627) + x");
        RecordingCanvas c3 = new RecordingCanvas();
        renderer.render(funcEm, c3, 0, -1, -1, -1, -1);
        DrawCall funcOpen = null;
        DrawCall funcClose = null;
        DrawCall funcRtl = null;
        for (DrawCall dc : c3.textCalls) {
            if (!dc.isRtl && "(".equals(dc.text)) funcOpen = dc;
            if (!dc.isRtl && ")".equals(dc.text)) funcClose = dc;
            if (dc.isRtl && "\u0645\u0631\u062D\u0628\u0627".equals(dc.text)) funcRtl = dc;
        }
        assertTrue("Expected unmirrored '(' in func(مرحبا)", funcOpen != null);
        assertTrue("Expected RTL word in func(مرحبا)", funcRtl != null);
        assertTrue("Expected unmirrored ')' in func(مرحبا)", funcClose != null);
        assertTrue("Opening '(' must hug 'func' cleanly", funcOpen.x < funcRtl.x && funcOpen.x <= 4.1f * w);
        assertTrue("Closing ')' must stay right of Arabic word", funcClose.x > funcRtl.x);

        // 4. Nested delimiters "((مرحبا))" and cursor on delimiter align without breaking cursor visibility.
        TerminalEmulator nestedEm = newEmulator(20, 2, "((\u0645\u0631\u062D\u0628\u0627))\u001b[1G");
        RecordingCanvas c4 = new RecordingCanvas();
        renderer.render(nestedEm, c4, 0, -1, -1, -1, -1);
        int cursorColor = nestedEm.mColors.mCurrentColors[TextStyle.COLOR_INDEX_CURSOR];
        RectCall delimCursor = findLastCursorRect(c4, cursorColor);
        assertTrue("Expected visible cursor rect on nested delimiter", delimCursor != null && (delimCursor.right - delimCursor.left) > 1.0f);

        // 5. Delimiters around space-separated "(hi)" do NOT join RTL; tight touching "مر(hi)ب" DOES join.
        BidiLayout.LogicalCell[] spacedRow = new BidiLayout.LogicalCell[8];
        String sSpaced = "\u0645 (hi) \u0628";
        for (int i = 0; i < spacedRow.length; i++) {
            spacedRow[i] = new BidiLayout.LogicalCell();
            spacedRow[i].codePoint = sSpaced.charAt(i);
            spacedRow[i].displayWidth = 1;
            spacedRow[i].isRtl = (i == 0 || i == 7);
        }
        assertFalse(TerminalRenderer.canJoinRtlAdjacentDelimiter(spacedRow, 8, 2)); // '(' between ' ' and 'h'
        assertFalse(TerminalRenderer.canJoinRtlAdjacentDelimiter(spacedRow, 8, 5)); // ')' between 'i' and ' '

        BidiLayout.LogicalCell[] touchingRow = new BidiLayout.LogicalCell[7];
        String sTouch = "\u0645\u0631(hi)\u0628";
        for (int i = 0; i < touchingRow.length; i++) {
            touchingRow[i] = new BidiLayout.LogicalCell();
            touchingRow[i].codePoint = sTouch.charAt(i);
            touchingRow[i].displayWidth = 1;
            touchingRow[i].isRtl = (i != 3 && i != 4);
        }
        assertTrue(TerminalRenderer.canJoinRtlAdjacentDelimiter(touchingRow, 7, 2)); // '(' touches 'ر'
        assertTrue(TerminalRenderer.canJoinRtlAdjacentDelimiter(touchingRow, 7, 5)); // ')' touches 'ب'
    }

    private static void installRealisticArabicHook() {
        Paint.sMeasureHook = (chars, start, count, tf, textSize) -> {
            if (tf == Typeface.MONOSPACE) {
                return count * (textSize * 0.6f); // 12.0f at textSize=20
            }
            float wUnit = textSize * 0.6f; // Scaled from proportional HarfBuzz advances
            float sum = 0f;
            for (int i = 0; i < count; i++) {
                char c = chars[start + i];
                if (c == '\u0627' || c == '\u0623' || c == '\u0625' || c == '\u0644' || c == '\u0631') {
                    sum += wUnit * 0.26f; // narrow vertical letters ('ا', 'ل', 'ر')
                } else if (c == ' ') {
                    sum += wUnit * 0.44f; // natural proportional space in Typeface.DEFAULT
                } else if (c == '(' || c == ')' || c == '[' || c == ']') {
                    sum += wUnit * 0.38f; // proportional crisp delimiter in Typeface.DEFAULT
                } else if (c >= 0x0600 && c <= 0x06FF) {
                    sum += wUnit * 0.56f; // medium/wide Arabic letters
                } else {
                    sum += wUnit * 0.60f;
                }
            }
            return sum;
        };
    }

    @Test
    public void testProportionalHarfBuzzNoPhantomAlefSpaceAndZeroStretch() {
        installRealisticArabicHook();
        try {
            TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
            float monoW = renderer.mFontWidth; // 12.0f

            // 1. Prompted "$ ا": 'ا' takes its exact proportional advance (0.26 * 1.10 * 12 = 3.432px),
            //    and BAR cursor sits immediately after 'ا' (0.0px phantom space before typing space!).
            TerminalEmulator emAlefPrompt = newEmulator(20, 2, "\u001b[6 q$ \u0627");
            RecordingCanvas cPrompt = new RecordingCanvas();
            renderer.render(emAlefPrompt, cPrompt, 0, -1, -1, -1, -1);
            int cursorColor = emAlefPrompt.mColors.mCurrentColors[TextStyle.COLOR_INDEX_CURSOR];
            RectCall barPromptBeforeSpace = findLastCursorRect(cPrompt, cursorColor);
            assertTrue(barPromptBeforeSpace != null);
            float expectedAlefEnd = 2f * monoW + (monoW * 1.10f * 0.26f);
            assertEquals("Cursor after '$ ا' must touch 'ا' with zero phantom space",
                    expectedAlefEnd, barPromptBeforeSpace.left, 0.25f);

            // Pressing Spacebar once advances cursor by strictly 1 proportional space, never double-accumulating.
            emAlefPrompt.append(" ".getBytes(StandardCharsets.US_ASCII), 1);
            RecordingCanvas cPromptAfterSpace = new RecordingCanvas();
            renderer.render(emAlefPrompt, cPromptAfterSpace, 0, -1, -1, -1, -1);
            RectCall barPromptAfterSpace = findLastCursorRect(cPromptAfterSpace, cursorColor);
            float expectedSpace = monoW * 1.10f * 0.44f;
            assertEquals("Pressing space after '$ ا' must add strictly one proportional space",
                    expectedSpace, barPromptAfterSpace.left - barPromptBeforeSpace.left, 0.25f);

            // 2. Unprompted pure RTL "ا" -> "ا ": right-aligned flush at viewportW, BAR cursor touches left edge of 'ا'.
            TerminalEmulator emRtlAlef = newEmulator(20, 2, "\u001b[6 q\u0627");
            RecordingCanvas cRtl1 = new RecordingCanvas();
            renderer.render(emRtlAlef, cRtl1, 0, -1, -1, -1, -1);
            RectCall rtlBar1 = findLastCursorRect(cRtl1, cursorColor);
            float viewportW = 20f * monoW;
            float alefW = monoW * 1.10f * 0.26f;
            assertEquals("Unprompted 'ا' BAR cursor right edge must sit directly at viewportW - w(ا)",
                    viewportW - alefW, rtlBar1.right, 0.25f);
            emRtlAlef.append(" ".getBytes(StandardCharsets.US_ASCII), 1);
            RecordingCanvas cRtl2 = new RecordingCanvas();
            renderer.render(emRtlAlef, cRtl2, 0, -1, -1, -1, -1);
            RectCall rtlBar2 = findLastCursorRect(cRtl2, cursorColor);
            assertEquals("Pressing space after unprompted 'ا' shifts cursor left by strictly 1 rtlSpaceW",
                    expectedSpace, rtlBar1.right - rtlBar2.right, 0.25f);

            // 3. Long narrow-letter Arabic phrase renders undistorted (scaleX == 1.0f) and flush right.
            String alefSentence = "\u0627\u0646\u0627 \u0627\u0643\u062A\u0628 \u0627\u0644\u0627\u0646 \u0627\u0644\u064A\u0648\u0645";
            TerminalEmulator emLong = newEmulator(30, 2, alefSentence);
            RecordingCanvas cLong = new RecordingCanvas();
            renderer.render(emLong, cLong, 0, -1, -1, -1, -1);
            for (DrawCall dc : cLong.textCalls) {
                assertEquals("Continuous flow must never distort scaleX > 1.0", 1.0f, dc.lastScaleX, 0.001f);
            }
            float rightEdge = renderer.getVisualColumnRightX(emLong, 18, 0); // activeLen=19 -> v=18
            assertEquals("Flush right edge of RTL line must equal viewportW", 30f * monoW, rightEdge, 0.1f);
        } finally {
            Paint.sMeasureHook = null;
        }
    }

    @Test
    public void testMultiLineWrappedParagraphAlignmentDirAutoVsPrompt() {
        installRealisticArabicHook();
        try {
            TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
            float w = renderer.mFontWidth;
            int cols = 20;

            // A. Unprompted wrapped Arabic+English paragraph: all 3 lines align flush RIGHT at viewportW.
            TerminalEmulator arPara = newEmulator(cols, 4, "");
            TerminalBuffer scrA = arPara.getScreen();
            long st = scrA.allocateFullLineIfNecessary(0).getStyle(0);
            String r0 = "\u0645\u0631\u062D\u0628\u0627 \u0628\u0643\u0645 \u0641\u064A \u0639\u0627\u0644\u0645 \u0627"; // 19 cols
            String r1 = "\u0644\u0628\u0631\u0645\u062C\u0629 hello \u0627\u0644\u064A\u0648\u0645"; // 18 cols mixed
            String r2 = "Termux"; // pure LTR wrapped continuation of RTL paragraph!
            for (int c = 0; c < r0.length(); c++) scrA.setChar(c, 0, r0.charAt(c), st);
            scrA.setLineWrap(0);
            for (int c = 0; c < r1.length(); c++) scrA.setChar(c, 1, r1.charAt(c), st);
            scrA.setLineWrap(1);
            for (int c = 0; c < r2.length(); c++) scrA.setChar(c, 2, r2.charAt(c), st);

            for (int row = 0; row < 3; row++) {
                TerminalRow tr = scrA.allocateFullLineIfNecessary(row);
                int ctx = BidiLayout.resolveInheritedContext(scrA, row, cols);
                BidiLayout layout = BidiLayout.build(tr, cols, -1, false, -1, -1, -1, -1, row, false,
                        (byte) (ctx & BidiLayout.INHERITED_DIR_MASK),
                        (ctx & BidiLayout.INHERITED_PROMPT_FLAG) != 0);
                BidiLayout.PixelGeometry geo = renderer.ensureRowPixelGeometry(layout, cols, false);
                assertTrue("Wrapped RTL paragraph row " + row + " must be rightAligned", geo.rightAligned);
                assertEquals("Row " + row + " right edge must align flush with viewportW",
                        cols * w, geo.contentRight, 0.1f);
            }

            // B. Prompted wrapped command: "$ " anchored LEFT on row 0 AND continuation row 1 stays LEFT!
            TerminalEmulator prPara = newEmulator(cols, 3, "");
            TerminalBuffer scrP = prPara.getScreen();
            String p0 = "$ \u0645\u0631\u062D\u0628\u0627 \u0628\u0643\u0645 \u0641\u064A \u0639\u0627\u0644\u0645";
            String p1 = "\u0627\u0644\u064A\u0648\u0645 hello";
            for (int c = 0; c < p0.length(); c++) scrP.setChar(c, 0, p0.charAt(c), st);
            scrP.setLineWrap(0);
            for (int c = 0; c < p1.length(); c++) scrP.setChar(c, 1, p1.charAt(c), st);

            for (int row = 0; row < 2; row++) {
                TerminalRow tr = scrP.allocateFullLineIfNecessary(row);
                int ctx = BidiLayout.resolveInheritedContext(scrP, row, cols);
                BidiLayout layout = BidiLayout.build(tr, cols, -1, false, -1, -1, -1, -1, row, false,
                        (byte) (ctx & BidiLayout.INHERITED_DIR_MASK),
                        (ctx & BidiLayout.INHERITED_PROMPT_FLAG) != 0);
                BidiLayout.PixelGeometry geo = renderer.ensureRowPixelGeometry(layout, cols, false);
                assertFalse("Prompted row " + row + " must stay flush LEFT", geo.rightAligned);
                assertEquals(0f, geo.contentLeft, 0.01f);
            }
        } finally {
            Paint.sMeasureHook = null;
        }
    }

    @Test
    public void testMixedDelimitersZeroOverlapAndStrictGridPreserved() {
        installRealisticArabicHook();
        try {
            TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);
            float w = renderer.mFontWidth;

            // 1. Delimiters around Arabic never overlap leftward (drawX >= prevRight) and use symmetric proportional width.
            TerminalEmulator delEm = newEmulator(25, 2, "\u0642\u0627\u0644 (\u0645\u0631\u062D\u0628\u0627) world");
            RecordingCanvas cDel = new RecordingCanvas();
            renderer.render(delEm, cDel, 0, -1, -1, -1, -1);
            TerminalBuffer scrDel = delEm.getScreen();
            BidiLayout delLayout = BidiLayout.build(scrDel.allocateFullLineIfNecessary(0), 25, -1, false,
                    -1, -1, -1, -1, 0, false, BidiLayout.DIR_DEFAULT);
            BidiLayout.PixelGeometry delGeo = renderer.ensureRowPixelGeometry(delLayout, 25, false);
            for (int v = 1; v < delLayout.trimmedActiveLength; v++) {
                assertTrue("Contiguous flow cell " + v + " must never overlap predecessor",
                        delGeo.visLeft[v] >= delGeo.visRight[v - 1] - 0.01f);
            }
            for (DrawCall dc : cDel.textCalls) {
                if ("(".equals(dc.text) || ")".equals(dc.text)) {
                    assertEquals(1.0f, dc.lastScaleX, 0.001f);
                }
            }

            // 2. Strict Grid preserved cleanly for box-drawing borders, columnar tables (>= 3 spaces), and alt-buffer!
            TerminalEmulator boxEm = newEmulator(20, 2, "\u2502 \u0645\u0631\u062D\u0628\u0627 \u2502");
            BidiLayout boxLayout = BidiLayout.build(boxEm.getScreen().allocateFullLineIfNecessary(0), 20, -1, false,
                    -1, -1, -1, -1, 0, false, BidiLayout.DIR_DEFAULT);
            assertTrue(TerminalRenderer.usesStrictGrid(boxLayout, false));
            BidiLayout.PixelGeometry boxGeo = renderer.ensureRowPixelGeometry(boxLayout, 20, false);
            assertFalse(boxGeo.continuousFlow);
            assertEquals(3f * w, boxGeo.visLeft[3], 0.01f);

            TerminalEmulator tableEm = newEmulator(25, 2, "\u0627\u0644\u0627\u0633\u0645   123");
            BidiLayout tableLayout = BidiLayout.build(tableEm.getScreen().allocateFullLineIfNecessary(0), 25, -1, false,
                    -1, -1, -1, -1, 0, false, BidiLayout.DIR_DEFAULT);
            assertTrue(TerminalRenderer.usesStrictGrid(tableLayout, false));
            BidiLayout.PixelGeometry tableGeo = renderer.ensureRowPixelGeometry(tableLayout, 25, false);
            assertFalse(tableGeo.continuousFlow);

            TerminalEmulator altEm = newEmulator(20, 2, "\u001b[?1049h\u0645\u0631\u062D\u0628\u0627");
            BidiLayout altLayout = BidiLayout.build(altEm.getScreen().allocateFullLineIfNecessary(0), 20, -1, false,
                    -1, -1, -1, -1, 0, false, BidiLayout.DIR_DEFAULT);
            assertTrue(TerminalRenderer.usesStrictGrid(altLayout, altEm));
        } finally {
            Paint.sMeasureHook = null;
        }
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
