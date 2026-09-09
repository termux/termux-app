package com.termux.view;

import com.termux.terminal.WcWidth;

import org.junit.Test;

import java.text.Bidi;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import static org.junit.Assert.*;

public class BidiLineLayoutTest {

    private static final float EPSILON = 0.001f;

    private static BidiLineLayout layout(String text, int columns, int cursor) {
        return new BidiLineLayout(text.toCharArray(), text.length(), columns, cursor);
    }

    private static float[] monoAdvances(String text, float width) {
        float[] advances = new float[text.length()];
        for (int i = 0; i < text.length(); ) {
            int cp = text.codePointAt(i);
            advances[i] = width * WcWidth.width(cp);
            i += Character.charCount(cp);
        }
        return advances;
    }

    private static List<BidiLineLayout.Cell> logicalCells(BidiLineLayout layout) {
        List<BidiLineLayout.Cell> cells = new ArrayList<>();
        for (BidiLineLayout.Run run : layout.runs) cells.addAll(run.cells);
        cells.sort(Comparator.comparingInt(cell -> cell.start));
        return cells;
    }

    private static void bounds(BidiLineLayout layout, float width) {
        for (int i = 0; i < layout.cellLeft.length; i++) {
            assertFalse(Float.isNaN(layout.leftOf(i)));
            assertTrue(layout.leftOf(i) >= 0);
            assertTrue(layout.leftOf(i) <= layout.rightOf(i));
            assertTrue(layout.rightOf(i) <= width);
        }
        float previousRight = 0;
        for (BidiLineLayout.Run run : layout.runs) {
            assertTrue(run.left + EPSILON >= previousRight);
            assertTrue(run.right >= run.left);
            assertTrue(run.right <= width);
            assertTrue(run.scale >= 0 && run.scale <= 1);
            previousRight = run.right;
        }
    }

    private static void checkHits(BidiLineLayout layout) {
        for (BidiLineLayout.Cell cell : logicalCells(layout)) {
            if (cell.width == 0) continue;
            float middle = (layout.leftOf(cell.column) + layout.rightOf(cell.column)) / 2;
            assertTrue(layout.rightOf(cell.column) > layout.leftOf(cell.column));
            assertEquals("Hit for cell at UTF-16 " + cell.start, cell.column, layout.columnAt(middle));
        }
    }

    @Test public void trimsOnlyTrailingSpacesAndNeverMutatesInput() {
        char[] text = "ab   unused".toCharArray();
        char[] original = text.clone();
        BidiLineLayout layout = new BidiLineLayout(text, 5, 5, -1);
        assertEquals(2, layout.textLength);
        assertArrayEquals(original, text);
        assertEquals(1, layout.runs.size());
        assertFalse(layout.runs.get(0).rtl);
        assertEquals(0, layout.runs.get(0).start);
        assertEquals(2, layout.runs.get(0).end);
    }

    @Test public void cursorRetainsTrailingSpacesAndClampsHighColumn() {
        assertEquals(4, layout("ab   ", 5, 3).textLength);
        assertEquals(5, layout("ab   ", 5, Integer.MAX_VALUE).textLength);
        assertEquals(2, layout("ab   ", 5, -1).textLength);
        assertEquals(0, layout("     ", 5, -1).textLength);
    }

    @Test public void combiningMarksStayWithTheirBaseIncludingSpace() {
        BidiLineLayout arabic = layout("\u0627\u064e  ", 3, -1);
        assertEquals(2, arabic.textLength);
        BidiLineLayout.Cell cell = logicalCells(arabic).get(0);
        assertEquals(0, cell.start);
        assertEquals(2, cell.end);
        assertEquals(1, cell.width);
        assertEquals(1, arabic.runs.get(0).columnCount);

        BidiLineLayout space = layout("a \u0301  ", 4, -1);
        assertEquals(3, space.textLength);
        assertEquals(2, logicalCells(space).size());
        assertEquals(1, logicalCells(space).get(1).column);
        assertEquals(3, logicalCells(space).get(1).end);
    }

    @Test public void wideSurrogateAndMarksFormOneTerminalCell() {
        String text = "A\ud83d\ude00\ufe0f\u0301B  ";
        BidiLineLayout layout = layout(text, 6, -1);
        List<BidiLineLayout.Cell> cells = logicalCells(layout);
        assertEquals(3, cells.size());
        assertEquals(1, cells.get(1).start);
        assertEquals(5, cells.get(1).end);
        assertEquals(1, cells.get(1).column);
        assertEquals(2, cells.get(1).width);
        assertEquals(3, cells.get(2).column);
        assertEquals(6, layout.textLength);
        layout.place(10, 100, monoAdvances(text, 10));
        assertEquals(layout.leftOf(1), layout.leftOf(2), 0);
        assertEquals(layout.rightOf(1), layout.rightOf(2), 0);
        assertEquals(20, layout.rightOf(1) - layout.leftOf(1), EPSILON);
        assertEquals(1, layout.columnAt(layout.rightOf(2) - 1));
        checkHits(layout);
    }

    @Test public void constructorNeverCutsWideCellOrSurrogateAtRowEnd() {
        assertEquals(1, layout("A\ud83d\ude00Z", 2, -1).textLength);
        assertEquals(3, layout("A\ud83d\ude00Z", 3, -1).textLength);
        assertEquals(1, layout("A\ud83d\ude00Z", 1, -1).textLength);
        assertEquals(0, layout("\ud83d\ude00", 1, 0).textLength);
        assertEquals(1, new BidiLineLayout(new char[]{'\ud83d'}, 1, 1, -1).textLength);
    }

    @Test public void latinRemainsLogicalAndNaturalWidthNeverStretches() {
        BidiLineLayout layout = layout("abc", 10, -1);
        layout.place(10, 100, new float[]{9, 10, 11});
        assertEquals(70, layout.runs.get(0).left, EPSILON);
        assertEquals(100, layout.runs.get(0).right, EPSILON);
        assertEquals(1, layout.runs.get(0).scale, 0);
        assertEquals(70, layout.leftOf(0), EPSILON);
        assertEquals(79, layout.leftOf(1), EPSILON);
        assertEquals(89, layout.leftOf(2), EPSILON);
        checkHits(layout);
    }

    @Test public void arabicFlowsRightToLeftWithoutReversingStoredRunCells() {
        BidiLineLayout layout = layout("\u0627\u0628\u062c", 8, -1);
        layout.place(10, 100, new float[]{8, 9, 7});
        BidiLineLayout.Run run = layout.runs.get(0);
        assertTrue(run.rtl);
        assertEquals(76, run.left, EPSILON);
        assertEquals(100, run.right, EPSILON);
        assertEquals(1, run.scale, 0);
        assertEquals(0, run.cells.get(0).start);
        assertEquals(1, run.cells.get(1).start);
        assertEquals(2, run.cells.get(2).start);
        assertEquals(100, layout.rightOf(0), EPSILON);
        assertEquals(92, layout.leftOf(0), EPSILON);
        assertEquals(83, layout.leftOf(1), EPSILON);
        assertEquals(76, layout.leftOf(2), EPSILON);
        checkHits(layout);
    }

    @Test public void mixedArabicEnglishNumbersAreInVisualRunOrder() {
        String text = "\u0627\u0628 abc 123 \u062c\u062f";
        BidiLineLayout layout = layout(text, 20, -1);
        layout.place(10, 200, monoAdvances(text, 10));
        assertTrue(layout.runs.get(0).start > 0);
        assertEquals(0, layout.runs.get(layout.runs.size() - 1).start);
        assertTrue(layout.leftOf(11) < layout.leftOf(3));
        assertTrue(layout.leftOf(3) < layout.leftOf(4));
        assertTrue(layout.leftOf(7) < layout.leftOf(8));
        assertTrue(layout.leftOf(8) < layout.leftOf(9));
        assertTrue(layout.leftOf(9) < layout.leftOf(0));
        assertTrue(layout.rtlAt(0));
        assertFalse(layout.rtlAt(3));
        assertFalse(layout.rtlAt(7));
        bounds(layout, 200);
        checkHits(layout);
    }

    @Test public void nestedBidiLevelsMatchWholeParagraphReordering() {
        String text = "\u0627 abc \u05d0\u05d1 12 xyz \u0628";
        BidiLineLayout layout = layout(text, text.length(), -1);
        Bidi bidi = new Bidi(text, Bidi.DIRECTION_RIGHT_TO_LEFT);
        Object[] expected = new Object[text.length()];
        byte[] levels = new byte[text.length()];
        for (int i = 0; i < text.length(); i++) {
            expected[i] = i;
            levels[i] = (byte) bidi.getLevelAt(i);
        }
        Bidi.reorderVisually(levels, 0, expected, 0, expected.length);
        List<Integer> actual = new ArrayList<>();
        for (BidiLineLayout.Run run : layout.runs) {
            int count = 0, priorEnd = run.start;
            for (BidiLineLayout.Cell cell : run.cells) {
                assertEquals(priorEnd, cell.start);
                priorEnd = cell.end;
                count += cell.width;
            }
            assertEquals(run.end, priorEnd);
            assertEquals(count, run.columnCount);
            if (run.rtl) {
                for (int i = run.cells.size() - 1; i >= 0; i--) actual.add(run.cells.get(i).start);
            } else {
                for (BidiLineLayout.Cell cell : run.cells) actual.add(cell.start);
            }
        }
        assertArrayEquals(expected, actual.toArray());
    }

    @Test public void lamAlefSharesAdvanceBetweenBaseCellsNotMarks() {
        BidiLineLayout layout = layout("\u0644\u0627", 4, -1);
        layout.place(10, 100, new float[]{16, 0});
        assertEquals(84, layout.runs.get(0).left, EPSILON);
        assertEquals(92, layout.leftOf(0), EPSILON);
        assertEquals(100, layout.rightOf(0), EPSILON);
        assertEquals(84, layout.leftOf(1), EPSILON);
        assertEquals(92, layout.rightOf(1), EPSILON);
        checkHits(layout);

        // Some shaping backends attribute the cluster to its other base.
        layout.place(10, 100, new float[]{0, 16});
        assertEquals(8, layout.rightOf(0) - layout.leftOf(0), EPSILON);
        assertEquals(8, layout.rightOf(1) - layout.leftOf(1), EPSILON);
        checkHits(layout);
    }

    @Test public void lamAlefWithCombiningMarkHasTwoHitRegionsOnly() {
        String text = "\u0644\u064e\u0627";
        BidiLineLayout layout = layout(text, 5, -1);
        layout.place(10, 100, new float[]{18, 0, 0});
        assertEquals(2, logicalCells(layout).size());
        assertEquals(2, logicalCells(layout).get(0).end);
        assertEquals(9, layout.rightOf(0) - layout.leftOf(0), EPSILON);
        assertEquals(9, layout.rightOf(1) - layout.leftOf(1), EPSILON);
        assertEquals(18, layout.runs.get(0).right - layout.runs.get(0).left, EPSILON);
        checkHits(layout);
    }

    @Test public void combiningAdvanceBelongsToBaseAndNeverGetsFallbackSpace() {
        BidiLineLayout layout = layout("e\u0301x", 8, -1);
        layout.place(10, 100, new float[]{8, 2, 10});
        assertEquals(2, logicalCells(layout).size());
        assertEquals(80, layout.runs.get(0).left, EPSILON);
        assertEquals(10, layout.rightOf(0) - layout.leftOf(0), EPSILON);
        layout.place(10, 100, new float[]{10, 0, 10});
        assertEquals(80, layout.runs.get(0).left, EPSILON);
        checkHits(layout);
    }

    @Test public void blankCursorRowUsesRightmostCell() {
        BidiLineLayout layout = layout("          ", 10, 0);
        assertEquals(1, layout.textLength);
        layout.place(10, 100, new float[]{10});
        assertEquals(90, layout.leftOf(0), EPSILON);
        assertEquals(100, layout.rightOf(0), EPSILON);
        assertTrue(layout.rtlAt(0));
        assertEquals(100, layout.boundary(0, false), EPSILON);
        assertEquals(90, layout.boundary(0, true), EPSILON);
        assertEquals(0, layout.columnAt(95));
        assertEquals(0, layout.columnAt(0)); // Real retained space wins over virtual blanks.
        assertEquals(80, layout.leftOf(1), EPSILON);
        assertEquals(90, layout.rightOf(1), EPSILON);
        bounds(layout, 100);
    }

    @Test public void emptySourceHasDeterministicVirtualBlanks() {
        BidiLineLayout layout = layout("", 5, 0);
        assertEquals(0, layout.textLength);
        assertTrue(layout.runs.isEmpty());
        layout.place(10, 30, new float[0]);
        assertEquals(20, layout.leftOf(0), EPSILON);
        assertEquals(30, layout.rightOf(0), EPSILON);
        assertEquals(10, layout.leftOf(1), EPSILON);
        assertEquals(0, layout.leftOf(4), EPSILON);
        assertEquals(0, layout.rightOf(4), EPSILON);
        assertEquals(0, layout.columnAt(25));
        assertEquals(1, layout.columnAt(15));
        bounds(layout, 30);
    }

    @Test public void trailingCursorIsPreservedAndIsHittable() {
        String text = "\u0627\u0628   ";
        BidiLineLayout layout = layout(text, 5, 4);
        layout.place(10, 100, monoAdvances(text, 10));
        assertEquals(5, layout.textLength);
        assertEquals(50, layout.leftOf(4), EPSILON);
        assertEquals(60, layout.rightOf(4), EPSILON);
        assertEquals(4, layout.columnAt(55));
        checkHits(layout);
    }

    @Test public void latinInsertionCursorStaysAfterTheTokenNotAtParagraphLeft() {
        String text = "abc  ";
        BidiLineLayout layout = layout(text, 5, 3);
        layout.place(10, 100, monoAdvances(text, 10));
        assertFalse(layout.rtlAt(0));
        assertFalse(layout.rtlAt(3));
        assertEquals(90, layout.leftOf(3), EPSILON);
        assertEquals(100, layout.rightOf(3), EPSILON);
        assertEquals(60, layout.leftOf(0), EPSILON);
        checkHits(layout);
    }

    @Test public void mixedRowLatinInsertionCursorFollowsLatinToken() {
        String text = "اب abc ";
        BidiLineLayout layout = layout(text, 12, 6);
        layout.place(10, 120, monoAdvances(text, 10));
        assertFalse(layout.rtlAt(6));
        assertEquals(layout.rightOf(5), layout.leftOf(6), EPSILON);
        assertTrue(layout.rightOf(6) <= layout.leftOf(2));
        checkHits(layout);
    }

    @Test public void styledBlankIsNotMistakenForInsertionCursor() {
        String text = "abc ";
        BidiLineLayout layout = new BidiLineLayout(text.toCharArray(), text.length(), 4, -1, 3);
        layout.place(10, 100, monoAdvances(text, 10));
        assertEquals(4, layout.textLength);
        assertTrue(layout.rtlAt(3)); // Authored/styled whitespace follows paragraph direction.
        assertTrue(layout.leftOf(3) < layout.leftOf(0));
    }

    @Test public void requiredBlankCannotTrimTheRealCursor() {
        String text = "abc  ";
        BidiLineLayout layout = new BidiLineLayout(text.toCharArray(), text.length(), 5, 4, 1);
        assertEquals(5, layout.textLength);
    }

    @Test public void authoredWhitespaceStillResolvesUsingParagraphDirection() {
        String text = "abc  ";
        BidiLineLayout layout = layout(text, 5, 4);
        layout.place(10, 100, monoAdvances(text, 10));
        assertTrue(layout.rtlAt(3));
        assertTrue(layout.rtlAt(4));
        assertTrue(layout.leftOf(4) < layout.leftOf(3));
        checkHits(layout);
    }

    @Test public void longLineShrinksAllRunsUniformlyAndKeepsHits() {
        StringBuilder builder = new StringBuilder();
        for (int i = 0; i < 50; i++) builder.append("\u0627 abc 12 ");
        builder.append('\u0628');
        String text = builder.toString();
        BidiLineLayout layout = layout(text, text.length(), -1);
        layout.place(10, 200, monoAdvances(text, 10));
        float expectedScale = 200f / (text.length() * 10);
        for (BidiLineLayout.Run run : layout.runs) assertEquals(expectedScale, run.scale, EPSILON);
        assertEquals(0, layout.runs.get(0).left, EPSILON);
        assertEquals(200, layout.runs.get(layout.runs.size() - 1).right, EPSILON);
        bounds(layout, 200);
        checkHits(layout);
    }

    @Test public void repeatedPlacementRecomputesGeometryWithoutAccumulatedScale() {
        BidiLineLayout layout = layout("abc", 6, -1);
        layout.place(10, 15, new float[]{10, 10, 10});
        assertEquals(0.5f, layout.runs.get(0).scale, EPSILON);
        layout.place(10, 100, new float[]{8, 8, 8});
        assertEquals(1, layout.runs.get(0).scale, 0);
        assertEquals(76, layout.runs.get(0).left, EPSILON);
        assertEquals(8, layout.rightOf(0) - layout.leftOf(0), EPSILON);
        checkHits(layout);
    }

    @Test public void leadingAndTrailingBoundariesRespectEachCellsDirection() {
        String text = "\u0627 abc";
        BidiLineLayout layout = layout(text, 10, -1);
        layout.place(10, 100, monoAdvances(text, 10));
        assertEquals(layout.rightOf(0), layout.boundary(0, false), 0);
        assertEquals(layout.leftOf(0), layout.boundary(0, true), 0);
        assertEquals(layout.leftOf(2), layout.boundary(2, false), 0);
        assertEquals(layout.rightOf(2), layout.boundary(2, true), 0);
    }

    @Test public void hitMappingClampsOutsidePixelsToNearestContentNotVirtualBlank() {
        BidiLineLayout layout = layout("abc", 10, -1);
        layout.place(10, 100, new float[]{10, 10, 10});
        assertEquals(0, layout.columnAt(-1000));
        assertEquals(2, layout.columnAt(1000));
        assertEquals(0, layout.columnAt(Float.NEGATIVE_INFINITY));
        assertEquals(2, layout.columnAt(Float.POSITIVE_INFINITY));
        assertEquals(0, layout.columnAt(Float.NaN));
        assertEquals(0, layout.columnAt(80)); // Shared-edge tie: smallest logical column.
        assertEquals(layout.leftOf(0), layout.leftOf(-100), 0);
        assertEquals(layout.rightOf(9), layout.rightOf(Integer.MAX_VALUE), 0);
        assertEquals(layout.rtlAt(9), layout.rtlAt(Integer.MAX_VALUE));
        bounds(layout, 100);
    }

    @Test public void zeroWidthViewportAndZeroColumnRowsStaySafe() {
        BidiLineLayout layout = layout("abc", 5, -1);
        layout.place(10, 0, new float[]{10, 10, 10});
        bounds(layout, 0);
        assertEquals(0, layout.runs.get(0).scale, 0);
        assertEquals(0, layout.columnAt(100));
        BidiLineLayout empty = layout("abc", 0, 0);
        empty.place(10, 100, new float[0]);
        assertEquals(0, empty.textLength);
        assertEquals(0, empty.leftOf(-1), 0);
        assertEquals(0, empty.rightOf(1), 0);
        assertEquals(0, empty.boundary(100, true), 0);
        assertEquals(-1, empty.columnAt(5));
    }

    @Test public void entirelyZeroAdvanceRunGetsHitAreaWithoutChangingNaturalWidth() {
        BidiLineLayout layout = layout("\u0644\u0627", 5, -1);
        layout.place(10, 100, new float[]{0, 0});
        assertEquals(100, layout.runs.get(0).left, EPSILON);
        assertEquals(100, layout.runs.get(0).right, EPSILON);
        assertEquals(1, layout.runs.get(0).scale, 0);
        assertEquals(90, layout.leftOf(0), EPSILON);
        assertEquals(100, layout.rightOf(0), EPSILON);
        assertEquals(layout.leftOf(0), layout.leftOf(1), 0);
        assertEquals(0, layout.columnAt(95));
        bounds(layout, 100);
    }

    @Test public void orphanMarksNeverConsumeTerminalColumns() {
        BidiLineLayout layout = layout("\u0301\u0302a", 3, -1);
        assertEquals(1, logicalCells(layout).size());
        assertEquals(1, logicalCells(layout).get(0).width);
        assertEquals(3, logicalCells(layout).get(0).end);
        layout.place(10, 100, new float[]{0, 0, 10});
        assertEquals(10, layout.rightOf(0) - layout.leftOf(0), EPSILON);
        BidiLineLayout marksOnly = layout("\u0301", 3, -1);
        assertEquals(1, marksOnly.textLength);
        assertEquals(0, marksOnly.runs.get(0).columnCount);
        marksOnly.place(10, 100, new float[]{0});
        assertEquals(90, marksOnly.leftOf(0), EPSILON);
    }

    @Test public void advancesInputIsNotMutatedAndNonFiniteAdvancesAreSafe() {
        BidiLineLayout layout = layout("abcd", 4, -1);
        float[] advances = {Float.NaN, Float.POSITIVE_INFINITY, -10, 20};
        float[] copy = advances.clone();
        layout.place(10, 100, advances);
        assertArrayEquals(copy, advances, 0);
        assertEquals(80, layout.runs.get(0).left, EPSILON);
        bounds(layout, 100);
        checkHits(layout);
    }

    @Test public void mixedEmojiArabicNumbersKeepExactUtf16Coverage() {
        String text = "\u0627\u064e \ud83d\ude00 test 12 \u0628";
        BidiLineLayout layout = layout(text, 30, -1);
        int end = 0, nextColumn = 0;
        for (BidiLineLayout.Cell cell : logicalCells(layout)) {
            assertEquals(end, cell.start);
            assertEquals(nextColumn, cell.column);
            assertFalse(Character.isLowSurrogate(text.charAt(cell.start)));
            end = cell.end;
            nextColumn += cell.width;
        }
        assertEquals(text.length(), end);
        layout.place(10, 300, monoAdvances(text, 10));
        bounds(layout, 300);
        checkHits(layout);
    }

    @Test(expected = IllegalArgumentException.class)
    public void rejectsTooShortAdvanceArray() {
        layout("abc", 3, -1).place(10, 100, new float[2]);
    }

    @Test(expected = IllegalArgumentException.class)
    public void rejectsInvalidTextLength() {
        new BidiLineLayout(new char[1], 2, 1, -1);
    }

    @Test(expected = IllegalArgumentException.class)
    public void rejectsNegativeColumns() {
        layout("", -1, -1);
    }

    @Test(expected = IllegalArgumentException.class)
    public void rejectsInvalidViewport() {
        layout("abc", 3, -1).place(10, Float.NaN, new float[3]);
    }

    @Test(expected = UnsupportedOperationException.class)
    public void logicalRunCellsCannotBeReorderedByCaller() {
        layout("abc", 3, -1).runs.get(0).cells.clear();
    }

    @Test(expected = UnsupportedOperationException.class)
    public void visualRunListCannotBeReorderedByCaller() {
        layout("abc", 3, -1).runs.clear();
    }
}
