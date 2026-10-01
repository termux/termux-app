package com.termux.terminal;


public class HistoryTest extends TerminalTestCase {

	public void testHistory() {
		final int rows = 3;
		final int cols = 3;
		withTerminalSized(cols, rows).enterString("111222333444555666777888999");
		assertCursorAt(2, 2);
		assertLinesAre("777", "888", "999");
		assertHistoryStartsWith("666", "555");

		resize(cols, 2);
		assertHistoryStartsWith("777", "666", "555");

		resize(cols, 3);
		assertHistoryStartsWith("666", "555");
	}

	public void testHistoryWithScrollRegion() {
		// "CSI P_s ; P_s r" - set Scrolling Region [top;bottom] (default = full size of window) (DECSTBM).
		withTerminalSized(3, 4).enterString("111222333444");
		assertLinesAre("111", "222", "333", "444");
		enterString("\033[2;3r");
		// NOTE: "DECSTBM moves the cursor to column 1, line 1 of the page."
		assertCursorAt(0, 0);
		enterString("\nCDEFGH").assertLinesAre("111", "CDE", "FGH", "444");
		enterString("IJK").assertLinesAre("111", "FGH", "IJK", "444").assertHistoryStartsWith("CDE");
		enterString("LMN").assertLinesAre("111", "IJK", "LMN", "444").assertHistoryStartsWith("FGH", "CDE");
	}

	public void testScrollUpMoreLinesThanScrollRegion() {
		// "CSI P_s S" - scroll up (SU). Scrolling more lines than the scroll region holds must not
		// push blank lines into the history and evict the lines that were scrolled out.
		withTerminalSized(3, 4).enterString("111222333444");
		enterString("\033[9S").assertLinesAre("   ", "   ", "   ", "   ");
		assertEquals(4, mTerminal.getScreen().getActiveTranscriptRows());
		assertHistoryStartsWith("444", "333", "222", "111");

		withTerminalSized(3, 4).enterString("111222333444");
		enterString("\033[1;2r\033[5S").assertLinesAre("   ", "   ", "333", "444");
		assertEquals(2, mTerminal.getScreen().getActiveTranscriptRows());
		assertHistoryStartsWith("222", "111");
	}

}
