package com.termux.terminal;

/** OSC 8 metadata follows terminal cells without opening or validating URLs. */
public class HyperlinkTest extends TerminalTestCase {
    private static final String CLOSE = "\033]8;;\033\\";

    private String open(String params, String uri) {
        return "\033]8;" + params + ";" + uri + "\033\\";
    }

    private int link(int row, int column) {
        TerminalBuffer screen = mTerminal.getScreen();
        return screen.mLines[screen.externalToInternalRow(row)].getHyperlink(column);
    }

    private String uri(int row, int column) {
        return mTerminal.getHyperlinkUri(link(row, column));
    }

    public void testOpenCloseAndTerminators() {
        withTerminalSized(20, 3);
        enterString(open("", "https://example.com/a;b") + "label" + CLOSE + "plain");
        assertEquals("https://example.com/a;b", uri(0, 0));
        assertEquals(link(0, 0), link(0, 4));
        assertNull(uri(0, 5));
        enterString("\033]8;;https://example.org\007X\033]8;;\007Y");
        assertEquals("https://example.org", uri(0, 10));
        assertNull(uri(0, 11));
        assertNull(mTerminal.getHyperlinkUri(-1));
        assertNull(mTerminal.getHyperlinkUri(TerminalEmulator.MAX_HYPERLINKS + 1));
    }

    public void testExplicitIdsAndSeparateImplicitLinks() {
        withTerminalSized(10, 3);
        enterString(open("id=part", "https://example.com") + "A" + CLOSE);
        enterString(open("ignored=value:id=part", "https://example.com") + "B" + CLOSE);
        assertEquals(link(0, 0), link(0, 1));
        enterString(open("id=part", "https://example.org") + "C" + CLOSE);
        assertTrue(link(0, 0) != link(0, 2));
        enterString(open("", "https://example.com") + "D" + CLOSE);
        enterString(open("", "https://example.com") + "E" + CLOSE);
        assertTrue(link(0, 3) != link(0, 4));
    }

    public void testWideCharactersOverwriteAndCombining() {
        withTerminalSized(10, 3);
        enterString(open("", "https://example.com") + "\u754c\u0301" + CLOSE);
        assertEquals(uri(0, 0), uri(0, 1));
        assertEquals("https://example.com", uri(0, 0));
        enterString("\033[1;2HX");
        assertNull(uri(0, 0));
        assertNull(uri(0, 1));
        enterString("\033[1;1H" + open("", "https://example.org") + "\u754c" + CLOSE);
        enterString("\033[1;1HY");
        assertNull(uri(0, 0));
        assertNull(uri(0, 1));
    }

    public void testCombiningMarksKeepTheBaseCharactersLink() {
        withTerminalSized(6, 3);
        enterString(open("", "https://base.example") + "\u754c" + CLOSE + "\u0301");
        assertEquals("https://base.example", uri(0, 0));
        assertEquals(uri(0, 0), uri(0, 1));
        enterString(open("", "https://other.example") + "\u0302" + CLOSE + "x");
        assertEquals("https://base.example", uri(0, 0));
        assertEquals(uri(0, 0), uri(0, 1));
        assertNull(uri(0, 2));
        resize(3, 4);
        assertEquals("https://base.example", uri(0, 0));
        assertEquals(uri(0, 0), uri(0, 1));
    }

    public void testLinkedSpacesSurviveReflowAfterUnicodeText() {
        String[] prefixes = {"", "x", "\ud83d\ude00\u0301"};
        int[] columns = {0, 1, 2};
        for (int i = 0; i < prefixes.length; i++) {
            withTerminalSized(8, 3);
            enterString(prefixes[i] + open("", "https://spaces.example") + "   " + CLOSE + "\r\nX");
            resize(6, 3);
            for (int col = columns[i]; col < columns[i] + 3; col++) {
                assertEquals("https://spaces.example", uri(0, col));
            }
            assertNull(uri(0, columns[i] + 3));
        }
    }

    public void testCopyWithinRowKeepsOriginalLinks() {
        TerminalRow row = new TerminalRow(6, TextStyle.NORMAL);
        for (int col = 0; col < 6; col++) row.setChar(col, 'a' + col, TextStyle.NORMAL, col + 1);
        row.copyInterval(row, 0, 4, 2);
        assertEquals(1, row.getHyperlink(2));
        assertEquals(2, row.getHyperlink(3));
        assertEquals(3, row.getHyperlink(4));
        assertEquals(4, row.getHyperlink(5));
        row.copyInterval(row, 2, 6, 0);
        for (int col = 0; col < 4; col++) assertEquals(col + 1, row.getHyperlink(col));
        row.clear(TextStyle.NORMAL);
        for (int col = 0; col < 6; col++) assertEquals(0, row.getHyperlink(col));
    }

    public void testLinksSurviveReflowAndHistory() {
        withTerminalSized(6, 3);
        enterString(open("", "https://example.com") + "abcdef" + CLOSE);
        resize(3, 4);
        assertEquals("https://example.com", uri(0, 0));
        assertEquals("https://example.com", uri(1, 2));
        enterString("\r\n1\r\n2\r\n3\r\n4");
        assertEquals("https://example.com", uri(-2, 0));
        assertEquals("https://example.com", uri(-1, 2));
    }

    public void testEraseAndResetCloseLinks() {
        withTerminalSized(10, 3);
        enterString(open("", "https://example.com") + "abc" + CLOSE);
        enterString("\033[2K");
        for (int col = 0; col < 10; col++) assertNull(uri(0, col));
        enterString(open("", "https://example.org"));
        mTerminal.reset();
        enterString("X");
        assertNull(uri(mTerminal.getCursorRow(), mTerminal.getCursorCol() - 1));
    }

    public void testFullTableDoesNotRetargetLiveCells() {
        withTerminalSized(TerminalEmulator.MAX_HYPERLINKS + 2, 2);
        for (int i = 0; i < TerminalEmulator.MAX_HYPERLINKS; i++) {
            enterString(open("", "https://example.com/" + i) + "x" + CLOSE);
        }
        enterString(open("", "https://overflow.example") + "z" + CLOSE);
        assertNull(uri(0, TerminalEmulator.MAX_HYPERLINKS));
        assertEquals("https://example.com/0", uri(0, 0));
        assertEquals("https://example.com/1023", uri(0, 1023));
        enterString("\033[1;1H ");
        for (int i = 0; i < TerminalEmulator.MAX_HYPERLINKS / 4; i++) {
            enterString(open("id=replacement", "https://replacement.example") + CLOSE);
        }
        enterString("\033[1;1H" + open("id=replacement", "https://replacement.example") + "r" + CLOSE);
        assertEquals("https://replacement.example", uri(0, 0));
        assertEquals("https://example.com/1", uri(0, 1));
        assertEquals("https://example.com/1023", uri(0, 1023));
    }

    public void testAlternateBufferDoesNotReplaceMainBufferLinks() {
        withTerminalSized(10, 3);
        enterString(open("id=main", "https://main.example") + "M" + CLOSE);
        enterString("\033[?1049h\033[H" + open("id=alt", "https://alt.example") + "A" + CLOSE);
        assertEquals("https://alt.example", uri(0, 0));
        enterString("\033[?1049l");
        assertEquals("https://main.example", uri(0, 0));
    }
}
