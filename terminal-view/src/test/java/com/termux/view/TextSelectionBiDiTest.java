package com.termux.view;

import android.graphics.Typeface;

import com.termux.terminal.TerminalBuffer;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalOutput;
import com.termux.terminal.TextStyle;

import org.junit.Test;

import java.nio.charset.StandardCharsets;

import static org.junit.Assert.assertEquals;

/**
 * Tests visual/logical column mapping and {@link BidiLayout#extractSelectedText} for single-line,
 * mixed Arabic/English partial selection, multi-line selection, wrapped rows, transcript rows, and wide chars.
 */
public class TextSelectionBiDiTest {

    private static final class DummyOutput extends TerminalOutput {
        @Override public void write(byte[] data, int offset, int count) {}
        @Override public void titleChanged(String oldTitle, String newTitle) {}
        @Override public void onCopyTextToClipboard(String text) {}
        @Override public void onPasteTextFromClipboard() {}
        @Override public void onBell() {}
        @Override public void onColorsChanged() {}
    }

    private static TerminalEmulator newEmulator(int cols, int rows, String content) {
        TerminalEmulator em = new TerminalEmulator(new DummyOutput(), cols, rows, 10, 20, rows * 3, null);
        if (content != null && !content.isEmpty()) {
            byte[] bytes = content.getBytes(StandardCharsets.UTF_8);
            em.append(bytes, bytes.length);
        }
        return em;
    }

    @Test
    public void testColumnTranslationAsciiFastPathAndBounds() {
        TerminalEmulator em = newEmulator(20, 5, "hello world");
        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);

        assertEquals(0, renderer.translateVisualToLogicalColumn(em, -5, 0));
        assertEquals(20, renderer.translateVisualToLogicalColumn(em, 25, 0));
        assertEquals(5, renderer.translateVisualToLogicalColumn(em, 5, 0));
        assertEquals(5, renderer.translateLogicalToVisualColumn(em, 5, 0));
        assertEquals(3, renderer.translateVisualToLogicalColumn(em, 3, 999));
    }

    @Test
    public void testSingleLineRtlCopyExactWordsAndSubstrings() {
        // Logical: "مرحبا بكم" (0..4="مرحبا", 5=' ', 6..8="بكم")
        // Visual (RTL base): 0..2="بكم" (l=8,7,6), 3=' ' (l=5), 4..8="مرحبا" (l=4,3,2,1,0)
        TerminalEmulator em = newEmulator(20, 3, "\u0645\u0631\u062D\u0628\u0627 \u0628\u0643\u0645");
        TerminalBuffer screen = em.getScreen();
        TerminalRenderer renderer = new TerminalRenderer(20, Typeface.MONOSPACE);

        assertEquals(8, renderer.translateVisualToLogicalColumn(em, 0, 0));
        assertEquals(4, renderer.translateVisualToLogicalColumn(em, 4, 0));
        assertEquals(0, renderer.translateVisualToLogicalColumn(em, 8, 0));

        // Select visual [0..2] ("بكم") -> logical "بكم"
        assertEquals("\u0628\u0643\u0645", BidiLayout.extractSelectedText(screen, 20, 3, 0, 0, 2, 0));

        // Select visual [4..8] ("مرحبا") -> logical "مرحبا"
        assertEquals("\u0645\u0631\u062D\u0628\u0627", BidiLayout.extractSelectedText(screen, 20, 3, 4, 0, 8, 0));

        // Partial word selection: visual [6..8] (logical cols 0..2 = "مرح")
        assertEquals("\u0645\u0631\u062D", BidiLayout.extractSelectedText(screen, 20, 3, 6, 0, 8, 0));

        // Reversed selX1/selX2 arguments still normalize cleanly
        assertEquals("\u0645\u0631\u062D", BidiLayout.extractSelectedText(screen, 20, 3, 8, 0, 6, 0));
    }

    @Test
    public void testSingleLineMixedArabicEnglishPartialSelectionNoLeak() {
        // Logical: "$ مرحبا hello عالم" (0..1="$ ", 2..6="مرحبا", 7=' ', 8..12="hello", 13=' ', 14..17="عالم")
        // Visual: 0..1="$ ", 2..5=rev("عالم"), 6=' ', 7..11="hello", 12=' ', 13..17=rev("مرحبا")
        String input = "$ \u0645\u0631\u062D\u0628\u0627 hello \u0639\u0627\u0644\u0645";
        TerminalEmulator em = newEmulator(30, 3, input);
        TerminalBuffer screen = em.getScreen();

        // Consistent with upstream TerminalBuffer.getSelectedText, trailing spaces on a non-wrapped slice trim cleanly
        assertEquals("$", BidiLayout.extractSelectedText(screen, 30, 3, 0, 0, 1, 0));
        // Spanning across "$ " into "عالم" keeps internal spaces intact
        assertEquals("$ \u0639\u0627\u0644\u0645", BidiLayout.extractSelectedText(screen, 30, 3, 0, 0, 5, 0));
        assertEquals("\u0639\u0627\u0644\u0645", BidiLayout.extractSelectedText(screen, 30, 3, 2, 0, 5, 0));
        assertEquals("hello", BidiLayout.extractSelectedText(screen, 30, 3, 7, 0, 11, 0));
        assertEquals("\u0645\u0631\u062D\u0628\u0627", BidiLayout.extractSelectedText(screen, 30, 3, 13, 0, 17, 0));

        // Select visually [9..17] -> "llo" + " " + "مرحبا" (logical 2..7 + 10..12).
        // Must NOT leak unselected "he" (logical 8..9)!
        assertEquals("\u0645\u0631\u062D\u0628\u0627 llo",
                BidiLayout.extractSelectedText(screen, 30, 3, 9, 0, 17, 0));
    }

    @Test
    public void testMultiLineArabicSelectionMatchesVisualHighlight() {
        // Row 0: "مرحبا بكم" -> visual: [0..2]="بكم", [3]=' ', [4..8]="مرحبا"
        // Row 1: "أهلا وسهلا" -> visual: [0..4]="وسهلا", [5]=' ', [6..9]="أهلا"
        TerminalEmulator em = newEmulator(20, 4,
                "\u0645\u0631\u062D\u0628\u0627 \u0628\u0643\u0645\r\n\u0623\u0647\u0644\u0627 \u0648\u0633\u0647\u0644\u0627");
        TerminalBuffer screen = em.getScreen();

        // Multi-line selection from Row 0 visual col 4 ("مرحبا") down to Row 1 visual col 4 ("وسهلا"):
        String selected = BidiLayout.extractSelectedText(screen, 20, 4, 4, 0, 4, 1);
        assertEquals("\u0645\u0631\u062D\u0628\u0627\n\u0648\u0633\u0647\u0644\u0627", selected);
    }

    @Test
    public void testWrappedRtlParagraphJoinsWithoutNewlineAndRespectsInheritedDirection() {
        TerminalEmulator em = newEmulator(10, 4, "");
        TerminalBuffer screen = em.getScreen();
        long normalStyle = screen.allocateFullLineIfNecessary(0).getStyle(0);

        // Row 0 fills all 10 cols and wraps to Row 1
        String r0 = "\u0645\u0631\u062D\u0628\u0627 \u0628\u0643\u0645 ";
        for (int c = 0; c < 10; c++) {
            screen.setChar(c, 0, r0.charAt(c), normalStyle);
        }
        screen.setLineWrap(0);

        String r1 = "hello \u0639\u0627\u0644";
        for (int c = 0; c < r1.length(); c++) {
            screen.setChar(c, 1, r1.charAt(c), normalStyle);
        }

        // Wrapped Row 1 inherits DIR_RTL so visual cols 0..2 are "عال" (logical 6..8) and cols 4..8 are "hello" (logical 0..4).
        // Select whole row 0 + visual [0..2] ("عال") on row 1 -> joined seamlessly with NO newline!
        String copied = BidiLayout.extractSelectedText(screen, 10, 4, 0, 0, 2, 1);
        assertEquals(r0 + "\u0639\u0627\u0644", copied);
    }

    @Test
    public void testWideEmojiSelectionHalfOrFullDoesNotDuplicateOrEmitNul() {
        // "ا 😀 ب" -> logical 0='ا', 1=' ', 2='😀', 3=cont, 4=' ', 5='ب'
        TerminalEmulator em = newEmulator(12, 3, "\u0627 \uD83D\uDE00 \u0628");
        TerminalBuffer screen = em.getScreen();
        BidiLayout layout = BidiLayout.build(screen.allocateFullLineIfNecessary(0), 12,
                -1, false, -1, -1, -1, -1, 0, false);

        int vLead = layout.logicalToVisual[2];
        int vCont = layout.logicalToVisual[3];
        assertEquals(vLead + 1, vCont);

        // Selecting both cols of emoji copies emoji once cleanly
        assertEquals("\uD83D\uDE00", BidiLayout.extractSelectedText(screen, 12, 3, vLead, 0, vCont, 0));
        // Selecting only continuation half STILL recovers the full emoji once cleanly
        assertEquals("\uD83D\uDE00", BidiLayout.extractSelectedText(screen, 12, 3, vCont, 0, vCont, 0));
    }

    @Test
    public void testScrollbackNegativeRowSelectionAndInvalidRange() {
        TerminalEmulator em = newEmulator(20, 3, "\u0633\u0637\u0631_1\r\n\u0633\u0637\u0631_2\r\n\u0633\u0637\u0631_3\r\n\u0633\u0637\u0631_4");
        TerminalBuffer screen = em.getScreen();
        // Row -1 is in transcript buffer
        String fromHistory = BidiLayout.extractSelectedText(screen, 20, 3, 0, -1, 19, -1);
        assertEquals("\u0633\u0637\u0631_1", fromHistory);

        // Inverted Y range returns empty string safely
        assertEquals("", BidiLayout.extractSelectedText(screen, 20, 3, 0, 2, 5, 0));
    }
}
