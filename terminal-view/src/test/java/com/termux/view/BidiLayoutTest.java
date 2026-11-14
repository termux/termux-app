package com.termux.view;

import com.termux.terminal.TerminalBuffer;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalOutput;
import com.termux.terminal.TerminalRow;
import com.termux.terminal.TextStyle;

import org.junit.Test;

import java.nio.charset.StandardCharsets;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

/**
 * Comprehensive tests for {@link BidiLayout}: mixed Arabic/English sentences, shell prompts vs
 * Markdown punctuation, TUI box borders, wrapped paragraphs, wide characters, and caching.
 */
public class BidiLayoutTest {

    private static final class DummyOutput extends TerminalOutput {
        @Override public void write(byte[] data, int offset, int count) {}
        @Override public void titleChanged(String oldTitle, String newTitle) {}
        @Override public void onCopyTextToClipboard(String text) {}
        @Override public void onPasteTextFromClipboard() {}
        @Override public void onBell() {}
        @Override public void onColorsChanged() {}
    }

    private static TerminalEmulator newEmulator(int cols, int rows, String initialText) {
        TerminalEmulator em = new TerminalEmulator(new DummyOutput(), cols, rows, 10, 20, rows * 2, null);
        if (initialText != null && !initialText.isEmpty()) {
            byte[] bytes = initialText.getBytes(StandardCharsets.UTF_8);
            em.append(bytes, bytes.length);
        }
        return em;
    }

    private static BidiLayout buildRow(TerminalBuffer screen, int row, int cols,
                                       int selY1, int selY2, int selX1, int selX2) {
        byte dir = BidiLayout.resolveInheritedDirection(screen, row, cols);
        TerminalRow tr = screen.allocateFullLineIfNecessary(screen.externalToInternalRow(row));
        return BidiLayout.build(tr, cols, -1, false, selY1, selY2, selX1, selX2, row, true, dir);
    }

    private static String visualString(BidiLayout layout, int len) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < len; i++) {
            int cp = layout.visualCells[i].codePoint;
            if (cp == 0) {
                sb.append('\0');
            } else {
                sb.appendCodePoint(cp);
            }
        }
        return sb.toString();
    }

    private static void assertBijective(BidiLayout layout, int cols) {
        boolean[] seenLog = new boolean[cols];
        for (int v = 0; v < cols; v++) {
            int l = layout.visualToLogical[v];
            assertTrue("logical index out of bounds: " + l, l >= 0 && l < cols);
            assertFalse("duplicate logical index in visualToLogical: " + l, seenLog[l]);
            seenLog[l] = true;
            assertEquals("logicalToVisual inverse mismatch at v=" + v, v, layout.logicalToVisual[l]);
        }
    }

    @Test
    public void testPureAsciiUsesIdentityFastPath() {
        TerminalEmulator em = newEmulator(40, 5, "echo hello world 123");
        BidiLayout layout = buildRow(em.getScreen(), 0, 40, -1, -1, -1, -1);
        assertTrue(layout.isIdentity);
        assertEquals(BidiLayout.DIR_LTR, layout.resolvedBaseDirection);
        assertBijective(layout, 40);
        for (int i = 0; i < 40; i++) {
            assertEquals(i, layout.visualToLogical[i]);
            assertFalse(layout.visualCells[i].isRtl);
        }
    }

    @Test
    public void testEnglishInMiddleOfArabicReordersSentenceRtl() {
        // Logical: "مرحبا hello عالم" (Arabic + English + Arabic, no prompt)
        String input = "\u0645\u0631\u062D\u0628\u0627 hello \u0639\u0627\u0644\u0645";
        TerminalEmulator em = newEmulator(40, 5, input);
        BidiLayout layout = buildRow(em.getScreen(), 0, 40, -1, -1, -1, -1);

        assertEquals(BidiLayout.DIR_RTL, layout.resolvedBaseDirection);
        assertBijective(layout, 40);

        // Visually (left to right): reversed "عالم", ' ', "hello", ' ', reversed "مرحبا"
        String vis = visualString(layout, input.length());
        String expectedVis = "\u0645\u0644\u0627\u0639 hello \u0627\u0628\u062D\u0631\u0645";
        assertEquals("English inside Arabic sentence must place 'عالم' visually left and 'مرحبا' visually right",
                expectedVis, vis);

        // Arabic segments must have isRtl=true; "hello" must stay LTR (isRtl=false)
        for (int v = 0; v < 4; v++) assertTrue("Expected RTL on عالم", layout.visualCells[v].isRtl);
        for (int v = 5; v < 10; v++) assertFalse("Expected LTR on hello", layout.visualCells[v].isRtl);
        for (int v = 11; v < 16; v++) assertTrue("Expected RTL on مرحبا", layout.visualCells[v].isRtl);
    }

    @Test
    public void testArabicInMiddleOfEnglishStaysLtrParagraph() {
        // Logical: "Run برنامج الآن ok" -> Base LTR, embedded Arabic stays between Run and ok
        String input = "Run \u0628\u0631\u0646\u0627\u0645\u062C \u0627\u0644\u0622\u0646 ok";
        TerminalEmulator em = newEmulator(40, 5, input);
        BidiLayout layout = buildRow(em.getScreen(), 0, 40, -1, -1, -1, -1);

        assertEquals(BidiLayout.DIR_LTR, layout.resolvedBaseDirection);
        assertBijective(layout, 40);

        String vis = visualString(layout, input.length());
        // Contiguous RTL phrase "برنامج الآن" reverses inside [4..15): "نآلا جمانرب"
        String expectedVis = "Run \u0646\u0622\u0644\u0627 \u062C\u0645\u0627\u0646\u0631\u0628 ok";
        assertEquals(expectedVis, vis);
        assertFalse(layout.visualCells[0].isRtl);
        assertTrue(layout.visualCells[4].isRtl);
        assertFalse(layout.visualCells[16].isRtl);
    }

    @Test
    public void testPromptAnchoredLeftWithMixedArabicEnglishCommand() {
        // "$ مرحبا hello عالم"
        String input = "$ \u0645\u0631\u062D\u0628\u0627 hello \u0639\u0627\u0644\u0645";
        TerminalEmulator em = newEmulator(40, 5, input);
        BidiLayout layout = buildRow(em.getScreen(), 0, 40, -1, -1, -1, -1);

        assertEquals(BidiLayout.DIR_RTL, layout.resolvedBaseDirection);
        String vis = visualString(layout, input.length());
        assertEquals("$ \u0645\u0644\u0627\u0639 hello \u0627\u0628\u062D\u0631\u0645", vis);
        assertFalse("Prompt '$ ' must stay LTR", layout.visualCells[0].isRtl);
        assertFalse(layout.visualCells[1].isRtl);
    }

    @Test
    public void testPathPromptAnchoredAndCurrencyNotTreatedAsPrompt() {
        // "~/termux-app $ مرحبا عالم" -> "~/termux-app $ " anchored at 0..14
        String promptCmd = "~/termux $ \u0645\u0631\u062D\u0628\u0627";
        TerminalEmulator em1 = newEmulator(40, 5, promptCmd);
        BidiLayout l1 = buildRow(em1.getScreen(), 0, 40, -1, -1, -1, -1);
        assertEquals("~/termux $ \u0627\u0628\u062D\u0631\u0645", visualString(l1, promptCmd.length()));
        assertEquals(BidiLayout.DIR_RTL, l1.resolvedBaseDirection);

        // Root hash prompt with host prefix: "[root@phone]# مرحبا"
        String rootCmd = "[root@phone]# \u0645\u0631\u062D\u0628\u0627";
        TerminalEmulator em2 = newEmulator(40, 5, rootCmd);
        BidiLayout l2 = buildRow(em2.getScreen(), 0, 40, -1, -1, -1, -1);
        assertEquals("[root@phone]# \u0627\u0628\u062D\u0631\u0645", visualString(l2, rootCmd.length()));

        // Markdown "# مرحبا عالم" at col 0 MUST NOT split as root prompt -> full RTL paragraph!
        String mdHeading = "# \u0645\u0631\u062D\u0628\u0627 \u0639\u0627\u0644\u0645";
        TerminalEmulator em3 = newEmulator(40, 5, mdHeading);
        BidiLayout l3 = buildRow(em3.getScreen(), 0, 40, -1, -1, -1, -1);
        assertEquals(BidiLayout.DIR_RTL, l3.resolvedBaseDirection);
        // '#' moves to visual right end of RTL Markdown heading!
        assertEquals("\u0645\u0644\u0627\u0639 \u0627\u0628\u062D\u0631\u0645 #", visualString(l3, mdHeading.length()));
    }

    @Test
    public void testNumbersAndListsAtStartOfArabicWithoutHackCorruption() {
        // "1. مرحبا hello" -> '1' stays level 2 LTR ('1'), '.' stays right of Arabic -> "hello مرحبا .1" visually
        String input = "1. \u0645\u0631\u062D\u0628\u0627 hello";
        TerminalEmulator em = newEmulator(40, 5, input);
        BidiLayout layout = buildRow(em.getScreen(), 0, 40, -1, -1, -1, -1);

        assertEquals(BidiLayout.DIR_RTL, layout.resolvedBaseDirection);
        assertEquals("hello \u0627\u0628\u062D\u0631\u0645 .1", visualString(layout, input.length()));
    }

    @Test
    public void testNumbersInsideArabicStayReadableLtr() {
        // "السعر 123.45 ريال" -> visual: "لاير 123.45 رعسلا"
        String input = "\u0627\u0644\u0633\u0639\u0631 123.45 \u0631\u064A\u0627\u0644";
        TerminalEmulator em = newEmulator(40, 5, input);
        BidiLayout layout = buildRow(em.getScreen(), 0, 40, -1, -1, -1, -1);

        assertEquals(BidiLayout.DIR_RTL, layout.resolvedBaseDirection);
        String vis = visualString(layout, input.length());
        assertTrue("Decimal number 123.45 must remain LTR inside Arabic", vis.contains("123.45"));
        assertTrue("Trailing word ريال must be on visual left", vis.startsWith("\u0644\u0627\u064A\u0631 "));
    }

    @Test
    public void testBoxDrawingTableBordersRemainPinned() {
        // TUI table row: "│ 1 │ مرحبا hello عالم │ OK │"
        String input = "\u2502 1 \u2502 \u0645\u0631\u062D\u0628\u0627 hello \u0639\u0627\u0644\u0645 \u2502 OK \u2502";
        TerminalEmulator em = newEmulator(50, 5, input);
        BidiLayout layout = buildRow(em.getScreen(), 0, 50, -1, -1, -1, -1);

        assertBijective(layout, 50);
        // Check exact visual column positions of every vertical border '│'
        for (int i = 0; i < input.length(); i++) {
            if (input.charAt(i) == '\u2502') {
                assertEquals("Box border at logical " + i + " moved!", '\u2502', layout.visualCells[i].codePoint);
                assertEquals(i, layout.logicalToVisual[i]);
            }
        }
        String vis = visualString(layout, input.length());
        assertEquals("\u2502 1 \u2502 \u0645\u0644\u0627\u0639 hello \u0627\u0628\u062D\u0631\u0645 \u2502 OK \u2502", vis);
    }

    @Test
    public void testWrappedParagraphInheritsRtlDirectionEvenWhenLine2StartsWithEnglish() {
        // 15-col terminal: Row 0 = "مرحبا بكم في هذا" (wrapped onto Row 1), Row 1 = "termux رائع"
        TerminalEmulator em = newEmulator(15, 4, "");
        TerminalBuffer screen = em.getScreen();
        long normalStyle = screen.allocateFullLineIfNecessary(0).getStyle(0);

        String r0Text = "\u0645\u0631\u062D\u0628\u0627 \u0628\u0643\u0645 \u0641\u064A \u0647\u0630";
        for (int c = 0; c < r0Text.length(); c++) {
            screen.setChar(c, 0, r0Text.charAt(c), normalStyle);
        }
        screen.setLineWrap(0);

        String r1Text = "termux \u0631\u0627\u0626\u0639";
        for (int c = 0; c < r1Text.length(); c++) {
            screen.setChar(c, 1, r1Text.charAt(c), normalStyle);
        }

        assertEquals(BidiLayout.DIR_RTL, BidiLayout.resolveInheritedDirection(screen, 1, 15));
        BidiLayout row1Layout = buildRow(screen, 1, 15, -1, -1, -1, -1);
        assertEquals(BidiLayout.DIR_RTL, row1Layout.resolvedBaseDirection);
        // Because row 1 inherited RTL from row 0, "رائع" goes visually LEFT and "termux" stays RIGHT!
        assertEquals("\u0639\u0626\u0627\u0631 termux", visualString(row1Layout, r1Text.length()));

        // Row 2 is NOT wrapped from Row 1 -> resets to DIR_DEFAULT
        assertEquals(BidiLayout.DIR_DEFAULT, BidiLayout.resolveInheritedDirection(screen, 2, 15));
    }

    @Test
    public void testWideEmojiInsideAndAtEndOfRtlPreservesLeadContinuationPair() {
        // Emoji U+1F600 ( 😀 , width=2) inside Arabic AND at boundary activeLength - 1!
        // "مرحبا 😀" -> logical cols: 0..4 (مرحبا), 5 (' '), 6 ('😀' lead), 7 (cont)
        TerminalEmulator em = newEmulator(30, 3, "\u0645\u0631\u062D\u0628\u0627 \uD83D\uDE00");
        BidiLayout layout = buildRow(em.getScreen(), 0, 30, -1, -1, -1, -1);
        assertBijective(layout, 30);

        int leadPos = layout.logicalToVisual[6];
        int contPos = layout.logicalToVisual[7];
        assertEquals("Emoji lead must immediately precede continuation even when reversed at end of RTL",
                leadPos + 1, contPos);
        assertEquals(0x1F600, layout.visualCells[leadPos].codePoint);
        assertEquals(2, layout.visualCells[leadPos].displayWidth);
        assertEquals(0, layout.visualCells[contPos].codePoint);
        assertEquals(0, layout.visualCells[contPos].displayWidth);
    }

    @Test
    public void testSupplementaryRtlAdlamReordersCleanly() {
        // Adlam U+1E900 U+1E901 U+1E902 + " hi"
        String input = new StringBuilder().appendCodePoint(0x1E900).appendCodePoint(0x1E901)
                .appendCodePoint(0x1E902).append(" hi").toString();
        TerminalEmulator em = newEmulator(20, 3, input);
        BidiLayout layout = buildRow(em.getScreen(), 0, 20, -1, -1, -1, -1);

        assertEquals(BidiLayout.DIR_RTL, layout.resolvedBaseDirection);
        assertBijective(layout, 20);
        // "hi" at visual 0..1, space at 2, Adlam reversed at 3..5
        assertEquals('h', layout.visualCells[0].codePoint);
        assertEquals('i', layout.visualCells[1].codePoint);
        assertEquals(0x1E902, layout.visualCells[3].codePoint);
        assertEquals(0x1E901, layout.visualCells[4].codePoint);
        assertEquals(0x1E900, layout.visualCells[5].codePoint);
    }

    @Test
    public void testCacheHitAndCacheMissHaveIdenticalVisualSelection() {
        TerminalEmulator em = newEmulator(20, 3, "\u0645\u0631\u062D\u0628\u0627 hello");
        TerminalRow row = em.getScreen().allocateFullLineIfNecessary(0);

        // Frame 1: Cache miss with visual selection [0..3]
        BidiLayout first = BidiLayout.build(row, 20, -1, false, 0, 0, 0, 3, 0, true);
        boolean[] expectedSel = new boolean[20];
        for (int v = 0; v < 20; v++) expectedSel[v] = first.visualCells[v].insideSelection;
        assertTrue(expectedSel[0] && expectedSel[3] && !expectedSel[4]);

        // Frame 2: Cache hit with SAME params -> returns exact same instance in O(1)
        BidiLayout second = BidiLayout.build(row, 20, -1, false, 0, 0, 0, 3, 0, true);
        assertSame(first, second);
        for (int v = 0; v < 20; v++) {
            assertEquals("Cache hit disagreed with cache miss at v=" + v,
                    expectedSel[v], second.visualCells[v].insideSelection);
        }

        // Frame 3: Cursor moved on same cached row -> keeps visual selection exact & updates cursor
        BidiLayout third = BidiLayout.build(row, 20, 2, true, 0, 0, 0, 3, 0, true);
        assertSame(first, third);
        assertTrue(third.visualCells[third.logicalToVisual[2]].insideCursor);
        assertFalse(third.visualCells[third.logicalToVisual[0]].insideCursor);
        assertTrue(third.visualCells[0].insideSelection && !third.visualCells[4].insideSelection);

        // Invalidate after mutation produces fresh instance
        row.setChar(0, 'X', row.getStyle(0));
        BidiLayout rebuilt = BidiLayout.build(row, 20, -1, false, -1, -1, -1, -1, 0, true);
        assertNotSame(first, rebuilt);
    }

    @Test
    public void testSixelBitmapCellsAreNotReversedInsideMixedRow() {
        TerminalEmulator em = newEmulator(10, 2, "");
        TerminalRow row = em.getScreen().allocateFullLineIfNecessary(0);
        long normalStyle = row.getStyle(0);
        row.setChar(0, '\u0645', normalStyle);
        long tile0 = TextStyle.encodeTerminalBitmap(10, 0, 0);
        long tile1 = TextStyle.encodeTerminalBitmap(10, 1, 0);
        long tile2 = TextStyle.encodeTerminalBitmap(10, 2, 0);
        row.setChar(1, ' ', tile0);
        row.setChar(2, ' ', tile1);
        row.setChar(3, ' ', tile2);
        row.setChar(4, '\u0639', normalStyle);

        BidiLayout layout = BidiLayout.build(row, 10, -1, false, -1, -1, -1, -1, 0, true);
        // Tiles 1, 2, 3 must keep ascending logical order in visual space
        assertTrue(layout.logicalToVisual[1] < layout.logicalToVisual[2]);
        assertTrue(layout.logicalToVisual[2] < layout.logicalToVisual[3]);
    }

    @Test
    public void testTrimmedActiveLengthAndTrailingCursorPreserveSelectionConsistency() {
        // Pure RTL with cursor at trailing blank col 9 keeps word columns unshifted
        TerminalEmulator rtlEm = newEmulator(20, 3, "\u0645\u0631\u062D\u0628\u0627 \u0628\u0643\u0645");
        assertEquals(9, rtlEm.getCursorCol());
        TerminalRow rtlRow = rtlEm.getScreen().allocateFullLineIfNecessary(0);
        BidiLayout rtlLayout = BidiLayout.build(rtlRow, 20, 9, true, -1, -1, -1, -1, 0, true);
        assertBijective(rtlLayout, 20);
        assertEquals(9, rtlLayout.trimmedActiveLength);
        assertEquals(0, rtlLayout.logicalToVisual[8]);
        assertEquals(8, rtlLayout.logicalToVisual[0]);

        // Prompt + Arabic leaves prompt on left and tracks trimmedActiveLength cleanly
        TerminalEmulator promptEm = newEmulator(20, 3, "$ \u0645\u0631\u062D\u0628\u0627   ");
        TerminalRow promptRow = promptEm.getScreen().allocateFullLineIfNecessary(0);
        BidiLayout promptLayout = BidiLayout.build(promptRow, 20, promptEm.getCursorCol(), true, -1, -1, -1, -1, 0, true);
        assertBijective(promptLayout, 20);
        assertEquals(7, promptLayout.trimmedActiveLength);
        assertEquals(0, promptLayout.logicalToVisual[0]);
        assertEquals(2, promptLayout.logicalToVisual[6]);

        // Embedded English at end of RTL keeps trimmedActiveLength at 'c' (col 8)
        TerminalEmulator mixedEm = newEmulator(20, 3, "\u0645\u0631\u062D\u0628\u0627 abc");
        TerminalRow mixedRow = mixedEm.getScreen().allocateFullLineIfNecessary(0);
        BidiLayout mixedLayout = BidiLayout.build(mixedRow, 20, 9, true, -1, -1, -1, -1, 0, true);
        assertBijective(mixedLayout, 20);
        assertEquals(9, mixedLayout.trimmedActiveLength);
        assertEquals(2, mixedLayout.logicalToVisual[8]);
    }

    @Test
    public void testMixedTextWithPairedBracketsPreservesBijectiveOrder() {
        String[] samples = {
                "\u0645\u0631\u062D\u0628\u0627 (hello)",
                "\u0645\u0631\u062D\u0628\u0627 [1] \u0648 [2]",
                "\u0627\u0644\u0646\u0635 {key: 123}",
                "\u0645\u0631\u062D\u0628\u0627 <tag> \u0628\u0643\u0645"
        };
        for (String sample : samples) {
            TerminalEmulator em = newEmulator(30, 2, sample);
            BidiLayout layout = buildRow(em.getScreen(), 0, 30, -1, -1, -1, -1);
            assertBijective(layout, 30);
            assertEquals(sample.length(), layout.trimmedActiveLength);
        }
    }
}
