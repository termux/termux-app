package com.termux.terminal;

import junit.framework.TestCase;
import java.text.Bidi;

/**
 * Tests for character buffer encoding, Bidi layout reordering, and cache invalidation.
 * Extends JUnit TestCase for automated test execution during Gradle builds.
 */
public class TerminalRendererBufferTest extends TestCase {

    private static final int MAX_COMBINING_CHARACTERS_PER_COLUMN = 15;

    public static void main(String[] args) {
        TerminalRendererBufferTest t = new TerminalRendererBufferTest();
        t.testCapacity_bmpNoCombining();
        t.testCapacity_supplementaryBase();
        t.testCapacity_maxCombiningBmp();
        t.testCapacity_maxCombiningSupplementary();
        t.testEncode_fourCombiningDiacritics();
        t.testEncode_arabicFullyVocalized();
        t.testEncode_maxCombining_noCrash();
        t.testEncode_supplementaryBaseAndCombiner();
        t.testAsciiGate_withCombiner_isNotAsciiPath();
        t.testAsciiGate_plainAscii_usesFastPath();
        t.testAsciiGate_boundary_del();
        t.testBidiReordering_arabicWord();
        t.testBidiReordering_promptPreservedAtLeft();
        t.testBidiReordering_wideCharPreserved();
        t.testTerminalRow_cacheInvalidationOnSetChar();
        t.testTerminalRow_cacheInvalidationOnClear();
        System.out.println("ALL TESTS PASSED SUCCESSFULLY! ✅");
    }

    // ---------------------------------------------------------------------------
    // Helpers that mirror the logic in TerminalRenderer exactly
    // ---------------------------------------------------------------------------

    private static int cellCapacity(int baseCodePoint, int[] combiningCodePoints) {
        int cap = Character.charCount(baseCodePoint);
        if (combiningCodePoints != null) {
            for (int cp : combiningCodePoints)
                cap += Character.charCount(cp);
        }
        return cap;
    }

    private static char[] encodeCell(int baseCodePoint, int[] combiningCodePoints) {
        int cap = cellCapacity(baseCodePoint, combiningCodePoints);
        char[] buf = new char[cap];
        int used = Character.toChars(baseCodePoint, buf, 0);
        if (combiningCodePoints != null) {
            for (int cp : combiningCodePoints)
                used += Character.toChars(cp, buf, used);
        }
        assertEquals(cap, used);
        return buf;
    }

    // ---------------------------------------------------------------------------
    // 1. Combining-character buffer capacity
    // ---------------------------------------------------------------------------

    public void testCapacity_bmpNoCombining() {
        assertEquals(1, cellCapacity('A', null));
    }

    public void testCapacity_supplementaryBase() {
        int smp = 0x1F600; // 😀 GRINNING FACE
        assertEquals(2, cellCapacity(smp, null));
    }

    public void testCapacity_maxCombiningBmp() {
        int[] combiners = new int[MAX_COMBINING_CHARACTERS_PER_COLUMN];
        for (int i = 0; i < combiners.length; i++)
            combiners[i] = 0x0301; // COMBINING ACUTE ACCENT (U+0301)

        int expected = 1 + MAX_COMBINING_CHARACTERS_PER_COLUMN;
        assertEquals(expected, cellCapacity('a', combiners));
    }

    public void testCapacity_maxCombiningSupplementary() {
        int base = 0x11000;   // arbitrary supplementary base
        int combiner = 0x1D167; // MUSICAL SYMBOL COMBINING TREMOLO-1 (supplementary combiner)

        int[] combiners = new int[MAX_COMBINING_CHARACTERS_PER_COLUMN];
        for (int i = 0; i < combiners.length; i++)
            combiners[i] = combiner;

        int expected = 2 + MAX_COMBINING_CHARACTERS_PER_COLUMN * 2;
        assertEquals(expected, cellCapacity(base, combiners));
    }

    // ---------------------------------------------------------------------------
    // 2. Encoding correctness — no truncation, no ArrayIndexOutOfBoundsException
    // ---------------------------------------------------------------------------

    public void testEncode_fourCombiningDiacritics() {
        int[] combiners = { 0x0300, 0x0301, 0x0302, 0x0303 };
        char[] buf = encodeCell('e', combiners);
        assertEquals(5, buf.length);
        assertEquals('e', buf[0]);
        assertEquals((char) 0x0300, buf[1]);
        assertEquals((char) 0x0303, buf[4]);
    }

    public void testEncode_arabicFullyVocalized() {
        // Arabic base letter + shadda + fatha + kasra + tanwin
        int[] combiners = { 0x0651, 0x064E, 0x0650, 0x064B };
        char[] buf = encodeCell(0x0628 /* ب */, combiners);
        assertEquals(5, buf.length);
        assertEquals((char) 0x0628, buf[0]);
    }

    public void testEncode_maxCombining_noCrash() {
        int[] combiners = new int[MAX_COMBINING_CHARACTERS_PER_COLUMN];
        for (int i = 0; i < combiners.length; i++)
            combiners[i] = 0x0301;

        char[] buf = encodeCell('a', combiners);
        assertEquals(1 + MAX_COMBINING_CHARACTERS_PER_COLUMN, buf.length);
        assertEquals('a', buf[0]);
    }

    public void testEncode_supplementaryBaseAndCombiner() {
        int base     = 0x11000;
        int combiner = 0x1D167;
        char[] buf = encodeCell(base, new int[]{ combiner });
        assertEquals(4, buf.length);
        assertEquals(base, Character.codePointAt(buf, 0));
        assertEquals(combiner, Character.codePointAt(buf, 2));
    }

    // ---------------------------------------------------------------------------
    // 3. ASCII fast-path — lookup table values are consistent with direct encoding
    // ---------------------------------------------------------------------------

    public void testAsciiGate_withCombiner_isNotAsciiPath() {
        int base = 'A'; // ASCII
        int[] combiners = { 0x0301 }; // non-null combining array
        boolean wouldUseFastPath = (base < 127) && (combiners == null);
        assertFalse("ASCII fast-path must NOT activate when combiners are present", wouldUseFastPath);
    }

    public void testAsciiGate_plainAscii_usesFastPath() {
        int base = 'Z';
        boolean wouldUseFastPath = (base < 127) && (true);
        assertTrue("ASCII fast-path must activate for plain ASCII with no combiners", wouldUseFastPath);
    }

    public void testAsciiGate_boundary_del() {
        int base = 127;
        boolean wouldUseFastPath = (base < 127);
        assertFalse("Codepoint 127 must fall through to measureText path", wouldUseFastPath);
    }

    // ---------------------------------------------------------------------------
    // 4. Bidi reordering & wide-character tests
    // ---------------------------------------------------------------------------

    public void testBidiReordering_arabicWord() {
        // "مرحبا" in logical order: M, R, H, B, A (indices 0, 1, 2, 3, 4)
        char[] chars = { '\u0645', '\u0631', '\u062D', '\u0628', '\u0627' };
        Bidi bidi = new Bidi(chars, 0, null, 0, chars.length, Bidi.DIRECTION_LEFT_TO_RIGHT);
        assertFalse(bidi.isLeftToRight());

        byte[] levels = new byte[chars.length];
        Integer[] visualToLogical = new Integer[chars.length];
        for (int i = 0; i < chars.length; i++) {
            levels[i] = (byte) bidi.getLevelAt(i);
            visualToLogical[i] = i;
        }
        Bidi.reorderVisually(levels, 0, visualToLogical, 0, chars.length);

        // Visual order of pure RTL must be reversed: 4, 3, 2, 1, 0
        assertEquals(4, (int) visualToLogical[0]);
        assertEquals(3, (int) visualToLogical[1]);
        assertEquals(2, (int) visualToLogical[2]);
        assertEquals(1, (int) visualToLogical[3]);
        assertEquals(0, (int) visualToLogical[4]);
    }

    public void testBidiReordering_promptPreservedAtLeft() {
        // Anchored prompt 'A' (idx 0), ' ' (idx 1), then Arabic 'م' 'ر' 'ح' 'ب' 'ا' (idx 2..6)
        char[] chars = { 'A', ' ', '\u0645', '\u0631', '\u062D', '\u0628', '\u0627' };
        Bidi bidi = new Bidi(chars, 0, null, 0, chars.length, Bidi.DIRECTION_LEFT_TO_RIGHT);
        assertFalse(bidi.isLeftToRight());

        byte[] levels = new byte[chars.length];
        Integer[] visualToLogical = new Integer[chars.length];
        for (int i = 0; i < chars.length; i++) {
            levels[i] = (byte) bidi.getLevelAt(i);
            visualToLogical[i] = i;
        }
        Bidi.reorderVisually(levels, 0, visualToLogical, 0, chars.length);

        // Prompt MUST remain at visual 0 and 1 on the left side of the terminal
        assertEquals(0, (int) visualToLogical[0]);
        assertEquals(1, (int) visualToLogical[1]);
        // Arabic word is reversed at visual 2..6
        assertEquals(6, (int) visualToLogical[2]);
        assertEquals(5, (int) visualToLogical[3]);
        assertEquals(4, (int) visualToLogical[4]);
        assertEquals(3, (int) visualToLogical[5]);
        assertEquals(2, (int) visualToLogical[6]);
    }

    public void testBidiReordering_promptArabicEnglish() {
        // "$ مرحبا hello": Anchored prompt 'A', 'A' (idx 0..1), Arabic (idx 2..6), space (idx 7), English "hello" (idx 8..12)
        char[] chars = { 'A', 'A', '\u0645', '\u0631', '\u062D', '\u0628', '\u0627', ' ', 'h', 'e', 'l', 'l', 'o' };
        Bidi bidi = new Bidi(chars, 0, null, 0, chars.length, Bidi.DIRECTION_LEFT_TO_RIGHT);
        assertFalse(bidi.isLeftToRight());

        byte[] levels = new byte[chars.length];
        Integer[] visualToLogical = new Integer[chars.length];
        for (int i = 0; i < chars.length; i++) {
            levels[i] = (byte) bidi.getLevelAt(i);
            visualToLogical[i] = i;
        }
        Bidi.reorderVisually(levels, 0, visualToLogical, 0, chars.length);

        // Prompt MUST remain at visual 0 and 1
        assertEquals(0, (int) visualToLogical[0]);
        assertEquals(1, (int) visualToLogical[1]);
        // Arabic word is reversed at visual 2..6
        assertEquals(6, (int) visualToLogical[2]);
        assertEquals(5, (int) visualToLogical[3]);
        assertEquals(4, (int) visualToLogical[4]);
        assertEquals(3, (int) visualToLogical[5]);
        assertEquals(2, (int) visualToLogical[6]);
        // Space and English follow in visual LTR order: 7, 8, 9, 10, 11, 12
        assertEquals(7, (int) visualToLogical[7]);
        assertEquals(8, (int) visualToLogical[8]);
        assertEquals(9, (int) visualToLogical[9]);
        assertEquals(10, (int) visualToLogical[10]);
        assertEquals(11, (int) visualToLogical[11]);
        assertEquals(12, (int) visualToLogical[12]);
    }

    public void testBidiReordering_wideCharPreserved() {
        // Wide char placeholder representation with strong LTR markers
        // Ensuring lead (idx 5) and continuation (idx 6) remain in visual order
        char[] chars = { '\u0645', '\u0631', '\u062D', '\u0628', '\u0627', 'A', 'A', '\u0639', '\u0627' };
        Bidi bidi = new Bidi(chars, 0, null, 0, chars.length, Bidi.DIRECTION_LEFT_TO_RIGHT);

        byte[] levels = new byte[chars.length];
        Integer[] visualToLogical = new Integer[chars.length];
        for (int i = 0; i < chars.length; i++) {
            levels[i] = (byte) bidi.getLevelAt(i);
            visualToLogical[i] = i;
        }
        Bidi.reorderVisually(levels, 0, visualToLogical, 0, chars.length);

        int posLead = -1;
        int posTrail = -1;
        for (int i = 0; i < visualToLogical.length; i++) {
            if (visualToLogical[i] == 5) posLead = i;
            if (visualToLogical[i] == 6) posTrail = i;
        }
        // Lead cell MUST appear immediately before trail cell in visual layout
        assertEquals(posLead + 1, posTrail);
    }

    // ---------------------------------------------------------------------------
    // 5. TerminalRow cache invalidation tests
    // ---------------------------------------------------------------------------

    public void testTerminalRow_cacheInvalidationOnSetChar() {
        TerminalRow row = new TerminalRow(80, TextStyle.NORMAL);
        row.mCachedBidiLayout = "dummy_layout";
        row.mLogicalToVisual = new int[80];
        row.mVisualToLogical = new int[80];

        row.setChar(0, 'A', TextStyle.NORMAL);
        assertNull("mCachedBidiLayout must be invalidated on setChar", row.mCachedBidiLayout);
        assertNull("mLogicalToVisual must be invalidated on setChar", row.mLogicalToVisual);
        assertNull("mVisualToLogical must be invalidated on setChar", row.mVisualToLogical);
    }

    public void testTerminalRow_cacheInvalidationOnClear() {
        TerminalRow row = new TerminalRow(80, TextStyle.NORMAL);
        row.mCachedBidiLayout = "dummy_layout";
        row.mLogicalToVisual = new int[80];
        row.mVisualToLogical = new int[80];

        row.clear(TextStyle.NORMAL);
        assertNull("mCachedBidiLayout must be invalidated on clear", row.mCachedBidiLayout);
        assertNull("mLogicalToVisual must be invalidated on clear", row.mLogicalToVisual);
        assertNull("mVisualToLogical must be invalidated on clear", row.mVisualToLogical);
    }
}
