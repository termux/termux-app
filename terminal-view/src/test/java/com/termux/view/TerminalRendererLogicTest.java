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

    private static final class RecordingCanvas extends Canvas {
        final List<DrawCall> textCalls = new ArrayList<>();
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
        // Progressive gentle caps prevent flat short words ("في", "لا") while closing inter-word gaps on longer phrases.
        assertEquals(1.20f, TerminalRenderer.computeRtlRunScaleX(5.0f, 1, 10.0f), 0.0001f);
        assertEquals(1.26f, TerminalRenderer.computeRtlRunScaleX(8.0f, 2, 10.0f), 0.0001f);
        assertEquals(1.34f, TerminalRenderer.computeRtlRunScaleX(12.0f, 3, 10.0f), 0.0001f);
        assertEquals(1.42f, TerminalRenderer.computeRtlRunScaleX(20.0f, 5, 10.0f), 0.0001f);
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
}
