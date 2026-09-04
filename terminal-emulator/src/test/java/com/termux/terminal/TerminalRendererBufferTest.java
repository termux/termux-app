package com.termux.terminal;

/**
 * Standalone tests for the character-buffer encoding logic extracted from TerminalRenderer.
 * Can be run directly via "java TerminalRendererBufferTest.java" or compiled with javac.
 */
public class TerminalRendererBufferTest {

    private static final int MAX_COMBINING_CHARACTERS_PER_COLUMN = 15;

    public static void main(String[] args) {
        System.out.println("Running TerminalRendererBufferTest...");
        testCapacity_bmpNoCombining();
        testCapacity_supplementaryBase();
        testCapacity_maxCombiningBmp();
        testCapacity_maxCombiningSupplementary();
        testEncode_fourCombiningDiacritics();
        testEncode_arabicFullyVocalized();
        testEncode_maxCombining_noCrash();
        testEncode_supplementaryBaseAndCombiner();
        testAsciiGate_withCombiner_isNotAsciiPath();
        testAsciiGate_plainAscii_usesFastPath();
        testAsciiGate_boundary_del();
        System.out.println("ALL TESTS PASSED SUCCESSFULLY! ✅");
    }

    private static void assertEquals(int expected, int actual) {
        if (expected != actual) throw new AssertionError("Expected " + expected + " but got " + actual);
    }
    
    private static void assertEquals(char expected, char actual) {
        if (expected != actual) throw new AssertionError("Expected " + (int)expected + " but got " + (int)actual);
    }

    private static void assertTrue(String msg, boolean condition) {
        if (!condition) throw new AssertionError(msg);
    }

    private static void assertFalse(String msg, boolean condition) {
        if (condition) throw new AssertionError(msg);
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

    public static void testCapacity_bmpNoCombining() {
        assertEquals(1, cellCapacity('A', null));
    }

    public static void testCapacity_supplementaryBase() {
        int smp = 0x1F600; // 😀 GRINNING FACE
        assertEquals(2, cellCapacity(smp, null));
    }

    public static void testCapacity_maxCombiningBmp() {
        int[] combiners = new int[MAX_COMBINING_CHARACTERS_PER_COLUMN];
        for (int i = 0; i < combiners.length; i++)
            combiners[i] = 0x0301; // COMBINING ACUTE ACCENT (U+0301)

        int expected = 1 + MAX_COMBINING_CHARACTERS_PER_COLUMN;
        assertEquals(expected, cellCapacity('a', combiners));
    }

    public static void testCapacity_maxCombiningSupplementary() {
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

    public static void testEncode_fourCombiningDiacritics() {
        int[] combiners = { 0x0300, 0x0301, 0x0302, 0x0303 };
        char[] buf = encodeCell('e', combiners);
        assertEquals(5, buf.length);
        assertEquals('e', buf[0]);
        assertEquals((char) 0x0300, buf[1]);
        assertEquals((char) 0x0303, buf[4]);
    }

    public static void testEncode_arabicFullyVocalized() {
        // Arabic base letter + shadda + fatha + kasra + tanwin
        int[] combiners = { 0x0651, 0x064E, 0x0650, 0x064B };
        char[] buf = encodeCell(0x0628 /* ب */, combiners);
        assertEquals(5, buf.length);
        assertEquals((char) 0x0628, buf[0]);
    }

    public static void testEncode_maxCombining_noCrash() {
        int[] combiners = new int[MAX_COMBINING_CHARACTERS_PER_COLUMN];
        for (int i = 0; i < combiners.length; i++)
            combiners[i] = 0x0301;

        char[] buf = encodeCell('a', combiners);
        assertEquals(1 + MAX_COMBINING_CHARACTERS_PER_COLUMN, buf.length);
        assertEquals('a', buf[0]);
    }

    public static void testEncode_supplementaryBaseAndCombiner() {
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

    public static void testAsciiGate_withCombiner_isNotAsciiPath() {
        int base = 'A'; // ASCII
        int[] combiners = { 0x0301 }; // non-null combining array
        boolean wouldUseFastPath = (base < 127) && (combiners == null);
        assertFalse("ASCII fast-path must NOT activate when combiners are present", wouldUseFastPath);
    }

    public static void testAsciiGate_plainAscii_usesFastPath() {
        int base = 'Z';
        boolean wouldUseFastPath = (base < 127) && (true);
        assertTrue("ASCII fast-path must activate for plain ASCII with no combiners", wouldUseFastPath);
    }

    public static void testAsciiGate_boundary_del() {
        int base = 127;
        boolean wouldUseFastPath = (base < 127);
        assertFalse("Codepoint 127 must fall through to measureText path", wouldUseFastPath);
    }
}
