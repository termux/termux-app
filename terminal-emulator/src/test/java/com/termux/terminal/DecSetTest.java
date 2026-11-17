package com.termux.terminal;

/**
 * <pre>
 * "CSI ? Pm h", DEC Private Mode Set (DECSET)
 * </pre>
 * <p/>
 * and
 * <p/>
 * <pre>
 * "CSI ? Pm l", DEC Private Mode Reset (DECRST)
 * </pre>
 * <p/>
 * controls various aspects of the terminal
 */
public class DecSetTest extends TerminalTestCase {

	/** DECSET 25, DECTCEM, controls visibility of the cursor. */
	public void testEnableDisableCursor() {
		withTerminalSized(3, 3);
		assertTrue("Initially the cursor should be enabled", mTerminal.isCursorEnabled());
		enterString("\033[?25l"); // Disable Cursor (DECTCEM).
		assertFalse(mTerminal.isCursorEnabled());
		enterString("\033[?25h"); // Enable Cursor (DECTCEM).
		assertTrue(mTerminal.isCursorEnabled());

		enterString("\033[?25l"); // Disable Cursor (DECTCEM), again.
		assertFalse(mTerminal.isCursorEnabled());
		mTerminal.reset();
		assertTrue("Resetting the terminal should enable the cursor", mTerminal.isCursorEnabled());

		enterString("\033[?25l");
		assertFalse(mTerminal.isCursorEnabled());
		enterString("\033c"); // RIS resetting should enabled cursor.
		assertTrue(mTerminal.isCursorEnabled());
	}

	/** DECSET 2004, controls bracketed paste mode. */
	public void testBracketedPasteMode() {
		withTerminalSized(3, 3);

		mTerminal.paste("a");
		assertEquals("Pasting 'a' should output 'a' when bracketed paste mode is disabled", "a", mOutput.getOutputAndClear());

		enterString("\033[?2004h"); // Enable bracketed paste mode.
		mTerminal.paste("a");
		assertEquals("Pasting when in bracketed paste mode should be bracketed", "\033[200~a\033[201~", mOutput.getOutputAndClear());

		enterString("\033[?2004l"); // Disable bracketed paste mode.
		mTerminal.paste("a");
		assertEquals("Pasting 'a' should output 'a' when bracketed paste mode is disabled", "a", mOutput.getOutputAndClear());

		enterString("\033[?2004h"); // Enable bracketed paste mode, again.
		mTerminal.paste("a");
		assertEquals("Pasting when in bracketed paste mode again should be bracketed", "\033[200~a\033[201~", mOutput.getOutputAndClear());

		mTerminal.paste("\033ab\033cd\033");
		assertEquals("Pasting an escape character should not input it", "\033[200~abcd\033[201~", mOutput.getOutputAndClear());
		mTerminal.paste("\u0081ab\u0081cd\u009F");
		assertEquals("Pasting C1 control codes should not input it", "\033[200~abcd\033[201~", mOutput.getOutputAndClear());

		mTerminal.reset();
		mTerminal.paste("a");
		assertEquals("Terminal reset() should disable bracketed paste mode", "a", mOutput.getOutputAndClear());
	}

	/** DECSET 7, DECAWM, controls wraparound mode. */
	public void testWrapAroundMode() {
		// Default with wraparound:
		withTerminalSized(3, 3).enterString("abcd").assertLinesAre("abc", "d  ", "   ");
		// With wraparound disabled:
		withTerminalSized(3, 3).enterString("\033[?7labcd").assertLinesAre("abd", "   ", "   ");
		enterString("efg").assertLinesAre("abg", "   ", "   ");
		// Re-enabling wraparound:
		enterString("\033[?7hhij").assertLinesAre("abh", "ij ", "   ");
	}

	/** DECSET/DECRST 2026 + DECRQM (?2026$p), synchronized output mode. */
	public void testSynchronizedOutputMode2026() {
		withTerminalSized(10, 4);

		// Initially reset (DECRQM -> 2):
		enterString("\033[?2026$p");
		assertEquals("\033[?2026;2$y", mOutput.getOutputAndClear());
		assertFalse(mTerminal.isSynchronizedOutputActive());
		assertEquals(0L, mTerminal.getDeferredScreenUpdateDelayMs());

		// Enabling synchronized output updates buffer state while deferring UI redraw:
		enterString("\033[?2026hHello");
		assertTrue(mTerminal.isSynchronizedOutputActive());
		long delayMs = mTerminal.getDeferredScreenUpdateDelayMs();
		assertTrue("Expected positive bounded deferral (<= 100ms), got " + delayMs, delayMs > 0L && delayMs <= 100L);
		assertLinesAre("Hello     ", "          ", "          ", "          ");
		enterString("\033[?2026$p");
		assertEquals("\033[?2026;1$y", mOutput.getOutputAndClear());

		// Preserved across alternate screen transition inside a synchronized frame:
		enterString("\033[?1049hAlt\033[?1049l");
		assertTrue(mTerminal.isSynchronizedOutputActive());

		// Resetting (?2026l) flushes immediately (DECRQM -> 2):
		enterString("\033[?2026l");
		assertFalse(mTerminal.isSynchronizedOutputActive());
		assertEquals(0L, mTerminal.getDeferredScreenUpdateDelayMs());
		enterString("\033[?2026$p");
		assertEquals("\033[?2026;2$y", mOutput.getOutputAndClear());

		// XTSAVE (?2026s) and XTRESTORE (?2026r):
		enterString("\033[?2026h\033[?2026s\033[?2026l");
		assertFalse(mTerminal.isSynchronizedOutputActive());
		enterString("\033[?2026r");
		assertTrue(mTerminal.isSynchronizedOutputActive());

		// DECSTR (\033[!p) and RIS (\033c) both clear synchronized output:
		enterString("\033[!p");
		assertFalse(mTerminal.isSynchronizedOutputActive());
		assertEquals(0L, mTerminal.getDeferredScreenUpdateDelayMs());
		enterString("\033[?2026h\033c");
		assertFalse(mTerminal.isSynchronizedOutputActive());
		assertEquals(0L, mTerminal.getDeferredScreenUpdateDelayMs());

		// Safety timeout (100ms) auto-resets synchronized output if client stalls without ?2026l:
		enterString("\033[?2026hStalled");
		assertTrue(mTerminal.isSynchronizedOutputActive());
		mTerminal.mSyncOutputStartNs = System.nanoTime() - TerminalEmulator.SYNC_OUTPUT_MAX_HOLD_NS - 1_000_000L;
		assertFalse(mTerminal.isSynchronizedOutputActive());
		assertEquals(0L, mTerminal.getDeferredScreenUpdateDelayMs());
		enterString("\033[?2026$p");
		assertEquals("\033[?2026;2$y", mOutput.getOutputAndClear());
	}

}
