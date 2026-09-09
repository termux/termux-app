package com.termux.view;

import com.termux.terminal.WcWidth;
import org.junit.Test;
import java.util.Arrays;
import static org.junit.Assert.*;

/** Synthetic advances test layout/ownership, not the quality of a device's shaping font. */
public class GridBidiLayoutTest {
    private static final float EPS = 0.001f;
    private static GridBidiLayout grid(String text, int columns, int cursor) {
        return new GridBidiLayout(text.toCharArray(), text.length(), columns, cursor);
    }
    private static float[] advances(char[] text, float unit) {
        float[] a = new float[text.length];
        for (int i = 0; i < text.length; ) {
            int cp = Character.codePointAt(text, i, text.length);
            a[i] = Math.max(0, WcWidth.width(cp)) * unit;
            i += Character.charCount(cp);
        }
        return a;
    }
    private static void place(GridBidiLayout grid, float unit) {
        float[][] a = new float[grid.fields.size()][];
        for (int i = 0; i < a.length; i++) a[i] = advances(grid.fields.get(i).text, unit);
        grid.place(10, a);
    }
    private static void field(GridBidiLayout grid, int index, int first, int end, String text) {
        GridBidiLayout.Field f = grid.fields.get(index);
        assertEquals(first, f.startColumn);
        assertEquals(end, f.endColumn);
        assertEquals(text, new String(f.text));
    }
    private static void roundTrips(GridBidiLayout grid) {
        for (GridBidiLayout.Field f : grid.fields) {
            for (BidiLineLayout.Run run : f.layout.runs) {
                for (BidiLineLayout.Cell cell : run.cells) {
                    int c = f.startColumn + cell.column;
                    float left = grid.leftOf(c), right = grid.rightOf(c);
                    assertTrue(left >= f.startColumn * 10);
                    assertTrue(right <= f.endColumn * 10);
                    assertTrue(right > left);
                    assertEquals(c, grid.columnAt((left + right) / 2));
                    assertEquals(run.rtl ? right : left, grid.boundary(c, false), EPS);
                    assertEquals(run.rtl ? left : right, grid.boundary(c, true), EPS);
                }
            }
        }
    }

    @Test public void twoSpacesDelimitFieldsAndDoNotAllocateScreenRemainder() {
        GridBidiLayout g = grid("ID  مرحبا  42  عالم        ", 60, -1);
        assertEquals(2, g.fields.size());
        field(g, 0, 4, 9, "مرحبا");
        field(g, 1, 15, 19, "عالم");
        place(g, 6);
        assertEquals(60, g.leftOf(8), EPS); // natural 30px field ends at original x=90.
        assertEquals(90, g.rightOf(4), EPS);
        for (int c : new int[]{0, 1, 2, 3, 9, 10, 11, 12, 13, 14, 19, 59}) {
            assertEquals(c, g.columnAt(c * 10 + 5));
            assertEquals(c * 10, g.leftOf(c), EPS);
        }
        roundTrips(g);
    }

    @Test public void htopTimeAndAsciiCommandPrefixKeepSemanticNativeColumns() {
        String text = String.format("%59s", "") + "0:00.12 عربي";
        GridBidiLayout g = grid(text, 80, -1);
        field(g, 0, 67, 71, "عربي");
        place(g, 6);
        for (int c = 59; c < 67; c++) {
            assertNull("TIME+ and its separator must not belong to COMMAND", g.fieldAtColumn(c));
            assertEquals(c * 10, g.boundary(c, false), EPS);
            assertEquals(c, g.mouseColumnAt(c * 10 + 5));
        }
        g = grid("  python -c عربي abc 123 بالعربية  OK", 80, -1);
        field(g, 0, 12, 33, "عربي abc 123 بالعربية");
        place(g, 6); roundTrips(g);
        for (int c = 0; c < 12; c++) assertEquals(c * 10, g.boundary(c, false), EPS);
    }

    @Test public void leadingNativeTokensDoNotSplitUnspacedMixedTokens() {
        for (String token : new String[]{"abcعربي", "0:00.12عربي", "界عربي", "😀عربي"}) {
            GridBidiLayout g = grid("ID " + token + " abc بالعربية", 80, -1);
            assertEquals(3, g.fields.get(0).startColumn);
            assertEquals(token + " abc بالعربية", new String(g.fields.get(0).text));
            place(g, 6); roundTrips(g);
        }
    }

    @Test public void mouseKeepsNativeWideSecondHalfButSelectionSnapsItsOwner() {
        for (String text : new String[]{"界 abc", "界  عربي", "界 عربي"}) {
            GridBidiLayout g = grid(text, 30, -1); place(g, 6);
            assertNull(g.fieldAtColumn(1));
            assertEquals(1, g.mouseColumnAt(15));
            assertEquals(0, g.columnAt(15));
            assertEquals(-2, g.mouseColumnAt(-25));
            assertEquals(30, g.mouseColumnAt(305));
            for (GridBidiLayout.Field f : g.fields)
                for (int c = f.startColumn; c < f.endColumn; c++)
                    assertEquals(c, g.mouseColumnAt((g.leftOf(c) + g.rightOf(c)) / 2));
        }
    }

    @Test(timeout = 3000) public void longBordersAndManyFieldLookupsStayLinear() {
        char[] border = new char[100000]; Arrays.fill(border, '-');
        GridBidiLayout g = grid(new String(border), border.length, -1);
        assertTrue(g.fields.isEmpty());
        StringBuilder fields = new StringBuilder();
        for (int i = 0; i < 20000; i++) fields.append("ب  ");
        g = grid(fields.toString(), fields.length(), -1);
        assertEquals(20000, g.fields.size());
        // Exercise every selected column repeatedly. A scan per lookup is quadratic.
        for (int repeat = 0; repeat < 10; repeat++)
            for (int c = 0; c < fields.length(); c++)
                assertEquals(c % 3 == 0, g.fieldAtColumn(c) != null);
        assertNull(g.fieldAtColumn(-1)); assertNull(g.fieldAtColumn(fields.length()));
    }

    @Test public void dashedTripletsIncludeBothEndsButShortRunsRemainText() {
        for (String border : new String[]{"---", "----", "===", "===="}) {
            GridBidiLayout g = grid("ب" + border + "ت", 30, -1);
            assertEquals(2, g.fields.size());
            for (int c = 1; c <= border.length(); c++) assertNull(g.fieldAtColumn(c));
        }
        for (String token : new String[]{"ب-ت", "ب--ت", "ب=ت", "ب==ت"}) {
            GridBidiLayout g = grid(token, 30, -1);
            assertEquals(token, new String(g.fields.get(0).text));
        }
    }

    @Test public void oneSpaceKeepsMixedDigitsAndPathInOneDirectionalParagraph() {
        String text = "  المسار /home/user/file.txt 123 بالعربية  OK";
        GridBidiLayout g = grid(text, 80, -1);
        assertEquals(1, g.fields.size());
        assertEquals("المسار /home/user/file.txt 123 بالعربية", new String(g.fields.get(0).text));
        boolean path = false;
        for (BidiLineLayout.Run r : g.fields.get(0).layout.runs) {
            String part = new String(g.fields.get(0).text, r.start, r.end - r.start);
            if (part.contains("home/user/file.txt 123")) { path = true; assertFalse(r.rtl); }
        }
        assertTrue(path);
        place(g, 6); roundTrips(g);
    }

    @Test public void leadingIndentSingleSpaceAndListPrefixesRemainNative() {
        for (String prefix : new String[]{" ", "    ", "- ", "* ", "+ ", "• ", "● ", "12. ", "2) ", "[x] ", "^G ", "M-U "}) {
            GridBidiLayout g = grid(prefix + "مرحبا", 40, -1);
            assertEquals(prefix, 1, g.fields.size());
            field(g, 0, prefix.length(), prefix.length() + 5, "مرحبا");
            place(g, 5);
            for (int c = 0; c < prefix.length(); c++) assertEquals(c, g.columnAt(c * 10 + 5));
            roundTrips(g);
        }
    }

    @Test public void asciiHeadersFootersAndAllAsciiFieldsAreNeverReordered() {
        for (String text : new String[]{"GNU nano 9.1      test.txt", "^G Help  ^O Write Out  ^X Exit", "ID  123  /tmp/file", "ASCII only"}) {
            GridBidiLayout g = grid(text, 80, -1);
            assertTrue(g.fields.isEmpty()); place(g, 5);
            for (int c = 0; c < 80; c++) assertEquals(c, g.columnAt(c * 10 + 5));
        }
    }

    @Test public void unicodeAndAsciiBordersAreNeverAllocatedToAnRtlField() {
        GridBidiLayout g = grid("│ مرحبا │ عالم | سلام --- اهلا === نص │", 60, -1);
        assertEquals(5, g.fields.size());
        place(g, 6); roundTrips(g);
        for (GridBidiLayout.Field f : g.fields) {
            String s = new String(f.text);
            assertFalse(s.contains("│")); assertFalse(s.contains("|"));
            assertFalse(s.contains("-")); assertFalse(s.contains("="));
            assertFalse(s.startsWith(" ")); assertFalse(s.endsWith(" "));
        }
    }

    @Test public void rawTabsAreNativeBoundariesWithEightCellStops() {
        GridBidiLayout g = grid("A\tمرحبا\tعالم", 40, -1);
        field(g, 0, 8, 13, "مرحبا"); field(g, 1, 16, 20, "عالم");
        place(g, 6); roundTrips(g);
        assertNull(g.fieldAtColumn(7)); assertNull(g.fieldAtColumn(15));
    }

    @Test public void cursorImmediatelyFollowingTextOwnsOneBlankNotTheWholeGap() {
        GridBidiLayout g = grid("  مرحبا     OK", 30, 7);
        field(g, 0, 2, 8, "مرحبا ");
        place(g, 6); roundTrips(g);
        assertNotNull(g.fieldAtColumn(7)); assertNull(g.fieldAtColumn(8));
        assertEquals(8, g.columnAt(85));
        GridBidiLayout distant = grid("  مرحبا     OK", 30, 9);
        field(distant, 0, 2, 7, "مرحبا");
        assertNull(distant.fieldAtColumn(9));
    }

    @Test public void insertionAfterLatinTokenStaysAtTokensTrailingEdge() {
        GridBidiLayout g = grid("  عربي abc     ", 30, 10);
        field(g, 0, 2, 11, "عربي abc ");
        place(g, 6); roundTrips(g);
        assertEquals(g.rightOf(9), g.leftOf(10), EPS);
        assertTrue(g.boundary(10, true) > g.boundary(10, false));
    }

    @Test public void cursorDoesNotBorrowBorderOrNextUiField() {
        GridBidiLayout g = grid("مرحبا|OK", 8, 5);
        field(g, 0, 0, 5, "مرحبا");
        place(g, 6); assertEquals(5, g.columnAt(55)); assertNull(g.fieldAtColumn(5));
    }

    @Test public void fullLastRowAndEveryLogicalCursorPositionStayBounded() {
        String text = "  مرحبا";
        for (int cursor = 0; cursor <= text.length() + 1; cursor++) {
            GridBidiLayout g = grid(text, 7, cursor);
            field(g, 0, 2, 7, "مرحبا");
            place(g, 6); roundTrips(g);
            for (int c = 0; c < 7; c++) {
                assertTrue(g.leftOf(c) >= 0); assertTrue(g.rightOf(c) <= 70);
            }
        }
    }

    @Test public void cursorCanUseVirtualAdjoiningBlankForUnpaddedSource() {
        GridBidiLayout g = grid("  مرحبا", 20, 7);
        field(g, 0, 2, 8, "مرحبا "); place(g, 6); roundTrips(g);
    }

    @Test public void combiningMarksAndWideCellsPreserveLogicalOwners() {
        GridBidiLayout g = grid("  بَلا😀A  OK", 30, -1);
        field(g, 0, 2, 8, "بَلا😀A");
        place(g, 6); roundTrips(g);
        assertEquals(g.leftOf(5), g.leftOf(6), EPS);
        assertEquals(g.rightOf(5), g.rightOf(6), EPS);
        assertEquals(5, g.columnAt((g.leftOf(6) + g.rightOf(6)) / 2));
    }

    @Test public void lamAlefLigatureHitSplitDoesNotSplitShapingContext() {
        GridBidiLayout g = grid("  لا  OK", 20, -1);
        assertEquals(1, g.fields.get(0).layout.runs.size());
        BidiLineLayout.Run r = g.fields.get(0).layout.runs.get(0);
        assertEquals(0, r.start); assertEquals(2, r.end);
        g.place(10, new float[][]{new float[]{9, 0}});
        assertEquals(4.5f, g.rightOf(2) - g.leftOf(2), EPS);
        assertEquals(4.5f, g.rightOf(3) - g.leftOf(3), EPS);
        roundTrips(g);
    }

    @Test public void oversizedRunsOnlyShrinkUniformlyInsideOriginalField() {
        GridBidiLayout g = grid("  مرحبا abc  OK", 40, -1);
        place(g, 20); roundTrips(g);
        for (BidiLineLayout.Run r : g.fields.get(0).layout.runs) assertEquals(0.5f, r.scale, EPS);
        place(g, 4);
        for (BidiLineLayout.Run r : g.fields.get(0).layout.runs) assertEquals(1, r.scale, EPS);
    }

    @Test public void repeatedSpacesAreLiteralNotCombiningBearingSpaces() {
        GridBidiLayout g = grid("ب \u064e ت  OK", 30, -1);
        assertEquals(1, g.fields.size());
        assertEquals("ب \u064e ت", new String(g.fields.get(0).text));
        place(g, 6); roundTrips(g);
    }

    @Test public void styleSplitsAreNotInputToBidiOrFieldSegmentation() {
        // An ANSI SGR transition changes TerminalRow styles, not text; same text = same full run.
        GridBidiLayout a = grid("  السلام  OK", 30, 4);
        GridBidiLayout b = grid("  السلام  OK", 30, 5);
        assertEquals(1, a.fields.get(0).layout.runs.size());
        assertEquals(1, b.fields.get(0).layout.runs.size());
        assertArrayEquals(a.fields.get(0).text, b.fields.get(0).text);
        place(a, 6); place(b, 6);
        assertArrayEquals(a.fields.get(0).layout.cellLeft, b.fields.get(0).layout.cellLeft, EPS);
    }

    @Test public void sourceAndUtf16AreNotRewrittenAndUsedLengthIsRespected() {
        char[] text = "  بَ😀  OK  ignored عربي".toCharArray(), copy = text.clone();
        GridBidiLayout g = new GridBidiLayout(text, 10, 40, -1);
        assertEquals(1, g.fields.size());
        GridBidiLayout.Field f = g.fields.get(0);
        assertArrayEquals(Arrays.copyOfRange(text, f.start, f.end), f.text);
        assertArrayEquals(copy, text);
        place(g, 6); roundTrips(g);
    }

    @Test public void virtualInsertionRespectsUsedLengthRatherThanBackingArrayCapacity() {
        char[] text = "  مرحباPOISON".toCharArray(), original = text.clone();
        GridBidiLayout g = new GridBidiLayout(text, 7, 20, 7);
        field(g, 0, 2, 8, "مرحبا ");
        assertArrayEquals(original, text);
        place(g, 6); roundTrips(g);
    }

    @Test public void leadingOrphanMarksStayWithFirstBase() {
        GridBidiLayout g = grid("\u064eبلا  OK", 20, -1);
        field(g, 0, 0, 3, "\u064eبلا");
        place(g, 6); roundTrips(g);
    }

    @Test public void emptyRowsAndInvalidHitsAreBounded() {
        GridBidiLayout g = grid("", 5, -1); place(g, 6);
        assertEquals(0, g.columnAt(Float.NaN)); assertEquals(0, g.columnAt(-100));
        assertEquals(4, g.columnAt(Float.POSITIVE_INFINITY));
        GridBidiLayout zero = grid("", 0, -1); place(zero, 6);
        assertEquals(-1, zero.columnAt(10)); assertEquals(0, zero.boundary(5, true), EPS);
    }
}
