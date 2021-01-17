package com.termux.terminal;

import org.junit.Assert;

public class CursorAndScreenTest extends TerminalTestCase {

	public void testDeleteLinesKeepsStyles() {
		int cols = 5, rows = 5;
		withTerminalSized(cols, rows);
		for (int row = 0; row < 5; row++) {
			for (int col = 0; col < 5; col++) {
				// Foreground color to col, background to row:
				enterString("\033[38;5;" + col + "m");
				enterString("\033[48;5;" + row + "m");
				enterString(Character.toString((char) ('A' + col + row * 5)));
			}
		}
		assertLinesAre("ABCDE", "FGHIJ", "KLMNO", "PQRST", "UVWXY");
		for (int row = 0; row < 5; row++) {
			for (int col = 0; col < 5; col++) {
				long s = getStyleAt(row, col);
				Assert.assertEquals(col, TextStyle.decodeForeColor(s));
				Assert.assertEquals(row, TextStyle.decodeBackColor(s));
			}
		}
		// "${CSI}H" - place cursor at 1,1, then "${CSI}2M" to delete two lines.
		enterString("\033[H\033[2M");
		assertLinesAre("KLMNO", "PQRST", "UVWXY", "     ", "     ");
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 5; col++) {
				long s = getStyleAt(row, col);
				Assert.assertEquals(col, TextStyle.decodeForeColor(s));
				Assert.assertEquals(row + 2, TextStyle.decodeBackColor(s));
			}
		}
		// Set default fg and background for the new blank lines:
		enterString("\033[38;5;98m");
		enterString("\033[48;5;99m");
		// "${CSI}B" to go down one line, then "${CSI}2L" to insert two lines:
		enterString("\033[B\033[2L");
		assertLinesAre("KLMNO", "     ", "     ", "PQRST", "UVWXY");
		for (int row = 0; row < 5; row++) {
			for (int col = 0; col < 5; col++) {
				int wantedForeground = (row == 1 || row == 2) ? 98 : col;
				int wantedBackground = (row == 1 || row == 2) ? 99 : (row == 0 ? 2 : row);
				long s = getStyleAt(row, col);
				Assert.assertEquals(wantedForeground, TextStyle.decodeForeColor(s));
				Assert.assertEquals(wantedBackground, TextStyle.decodeBackColor(s));
			}
		}
	}

	public void testDeleteCharacters() {
		withTerminalSized(5, 2).enterString("枝ce").assertLinesAre("枝ce ", "     ");
		withTerminalSized(5, 2).enterString("a枝ce").assertLinesAre("a枝ce", "     ");
		withTerminalSized(5, 2).enterString("nice").enterString("\033[G\033[P").assertLinesAre("ice  ", "     ");
		withTerminalSized(5, 2).enterString("nice").enterString("\033[G\033[2P").assertLinesAre("ce   ", "     ");
		withTerminalSized(5, 2).enterString("nice").enterString("\033[2G\033[2P").assertLinesAre("ne   ", "     ");
		// "${CSI}${n}P, the delete characters (DCH) sequence should cap characters to delete.
		withTerminalSized(5, 2).enterString("nice").enterString("\033[G\033[99P").assertLinesAre("     ", "     ");
		// With combining char U+0302.
		withTerminalSized(5, 2).enterString("n\u0302ice").enterString("\033[G\033[2P").assertLinesAre("ce   ", "     ");
		withTerminalSized(5, 2).enterString("n\u0302ice").enterString("\033[G\033[P").assertLinesAre("ice  ", "     ");
		withTerminalSized(5, 2).enterString("n\u0302ice").enterString("\033[2G\033[2P").assertLinesAre("n\u0302e   ", "     ");
		// With wide 枝 char, checking that putting char at part replaces other with whitespace:
		withTerminalSized(5, 2).enterString("枝ce").enterString("\033[Ga").assertLinesAre("a ce ", "     ");
		withTerminalSized(5, 2).enterString("枝ce").enterString("\033[2Ga").assertLinesAre(" ace ", "     ");
		// With wide 枝 char, deleting either part replaces other with whitespace:
		withTerminalSized(5, 2).enterString("枝ce").enterString("\033[G\033[P").assertLinesAre(" ce  ", "     ");
		withTerminalSized(5, 2).enterString("枝ce").enterString("\033[2G\033[P").assertLinesAre(" ce  ", "     ");
		withTerminalSized(5, 2).enterString("枝ce").enterString("\033[2G\033[2P").assertLinesAre(" e   ", "     ");
		withTerminalSized(5, 2).enterString("枝ce").enterString("\033[G\033[2P").assertLinesAre("ce   ", "     ");
		withTerminalSized(5, 2).enterString("a枝ce").enterString("\033[G\033[P").assertLinesAre("枝ce ", "     ");
	}

	public void testInsertMode() {
		// "${CSI}4h" enables insert mode.
		withTerminalSized(5, 2).enterString("nice").enterString("\033[G\033[4hA").assertLinesAre("Anice", "     ");
		withTerminalSized(5, 2).enterString("nice").enterString("\033[2G\033[4hA").assertLinesAre("nAice", "     ");
		withTerminalSized(5, 2).enterString("nice").enterString("\033[G\033[4hABC").assertLinesAre("ABCni", "     ");
		// With combining char U+0302.
		withTerminalSized(5, 2).enterString("n\u0302ice").enterString("\033[G\033[4hA").assertLinesAre("An\u0302ice", "     ");
		withTerminalSized(5, 2).enterString("n\u0302ice").enterString("\033[G\033[4hAB").assertLinesAre("ABn\u0302ic", "     ");
		withTerminalSized(5, 2).enterString("n\u0302ic\u0302e").enterString("\033[2G\033[4hA").assertLinesAre("n\u0302Aic\u0302e", "     ");
		// ... but without insert mode, combining char should be overwritten:
		withTerminalSized(5, 2).enterString("n\u0302ice").enterString("\033[GA").assertLinesAre("Aice ", "     ");
		// ... also with two combining:
		withTerminalSized(5, 2).enterString("n\u0302\u0302i\u0302ce").enterString("\033[GA").assertLinesAre("Ai\u0302ce ", "     ");
		// ... and in last column:
		withTerminalSized(5, 2).enterString("n\u0302\u0302ice!\u0302").enterString("\033[5GA").assertLinesAre("n\u0302\u0302iceA", "     ");
		withTerminalSized(5, 2).enterString("nic\u0302e!\u0302").enterString("\033[4G枝").assertLinesAre("nic\u0302枝", "     ");
		withTerminalSized(5, 2).enterString("nic枝\u0302").enterString("\033[3GA").assertLinesAre("niA枝\u0302", "     ");
		withTerminalSized(5, 2).enterString("nic枝\u0302").enterString("\033[3GA").assertLinesAre("niA枝\u0302", "     ");
		// With wide 枝 char.
		withTerminalSized(5, 2).enterString("nice").enterString("\033[G\033[4h枝").assertLinesAre("枝nic", "     ");
		withTerminalSized(5, 2).enterString("nice").enterString("\033[2G\033[4h枝").assertLinesAre("n枝ic", "     ");
		withTerminalSized(5, 2).enterString("n枝ce").enterString("\033[G\033[4ha").assertLinesAre("an枝c", "     ");
	}

	/** HPA—Horizontal Position Absolute (http://www.vt100.net/docs/vt510-rm/HPA) */
	public void testCursorHorizontalPositionAbsolute() {
		withTerminalSized(4, 4).enterString("ABC\033[`").assertCursorAt(0, 0);
		enterString("\033[1`").assertCursorAt(0, 0).enterString("\033[2`").assertCursorAt(0, 1);
		enterString("\r\n\033[3`").assertCursorAt(1, 2).enterString("\033[22`").assertCursorAt(1, 3);
		// Enable and configure right and left margins, first without origin mode:
		enterString("\033[?69h\033[2;3s\033[`").assertCursorAt(0, 0).enterString("\033[22`").assertCursorAt(0, 3);
		// .. now with origin mode:
		enterString("\033[?6h\033[`").assertCursorAt(0, 1).enterString("\033[22`").assertCursorAt(0, 2);
	}

	public void testCursorForward() {
		// "${CSI}${N:=1}C" moves cursor forward N columns:
		withTerminalSized(6, 2).enterString("A\033[CB\033[2CC").assertLinesAre("A B  C", "      ");
		// If an attempt is made to move the cursor to the right of the right margin, the cursor stops at the right margin:
		withTerminalSized(6, 2).enterString("A\033[44CB").assertLinesAre("A    B", "      ");
		// Enable right margin and verify that CUF ends at the set right margin:
		withTerminalSized(6, 2).enterString("\033[?69h\033[1;3s\033[44CAB").assertLinesAre("  A   ", "B     ");
	}

	public void testCursorBack() {
		// "${CSI}${N:=1}D" moves cursor back N columns:
		withTerminalSized(3, 2).enterString("A\033[DB").assertLinesAre("B  ", "   ");
		withTerminalSized(3, 2).enterString("AB\033[2DC").assertLinesAre("CB ", "   ");
		// If an attempt is made to move the cursor to the left of the left margin, the cursor stops at the left margin:
		withTerminalSized(3, 2).enterString("AB\033[44DC").assertLinesAre("CB ", "   ");
		// Enable left margin and verify that CUB ends at the set left margin:
		withTerminalSized(6, 2).enterString("ABCD\033[?69h\033[2;6s\033[44DE").assertLinesAre("AECD  ", "      ");
	}

	public void testCursorUp() {
		// "${CSI}${N:=1}A" moves cursor up N rows:
		withTerminalSized(3, 3).enterString("ABCDEFG\033[AH").assertLinesAre("ABC", "DHF", "G  ");
		withTerminalSized(3, 3).enterString("ABCDEFG\033[2AH").assertLinesAre("AHC", "DEF", "G  ");
		// If an attempt is made to move the cursor above the top margin, the cursor stops at the top margin:
		withTerminalSized(3, 3).enterString("ABCDEFG\033[44AH").assertLinesAre("AHC", "DEF", "G  ");
	}

	public void testCursorDown() {
		// "${CSI}${N:=1}B" moves cursor down N rows:
		withTerminalSized(3, 3).enterString("AB\033[BC").assertLinesAre("AB ", "  C", "   ");
		withTerminalSized(3, 3).enterString("AB\033[2BC").assertLinesAre("AB ", "   ", "  C");
		// If an attempt is made to move the cursor below the bottom margin, the cursor stops at the bottom margin:
		withTerminalSized(3, 3).enterString("AB\033[44BC").assertLinesAre("AB ", "   ", "  C");
	}

	public void testReportCursorPosition() {
		withTerminalSized(10, 10);
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 10; j++) {
				enterString("\033[" + (i + 1) + ";" + (j + 1) + "H"); // CUP cursor position.
				assertCursorAt(i, j);
				// Device Status Report (DSR):
				assertEnteringStringGivesResponse("\033[6n", "\033[" + (i + 1) + ";" + (j + 1) + "R");
				// DECXCPR — Extended Cursor Position. Note that http://www.vt100.net/docs/vt510-rm/DECXCPR says
				// the response is "${CSI}${LINE};${COLUMN};${PAGE}R" while xterm (http://invisible-island.net/xterm/ctlseqs/ctlseqs.html)
				// drops the question mark. Expect xterm behaviour here.
				assertEnteringStringGivesResponse("\033[?6n", "\033[?" + (i + 1) + ";" + (j + 1) + ";1R");
			}
		}
	}

	/**
	 * See comments on horizontal tab handling in TerminalEmulator.java.
	 * <p/>
	 * We do not want to color already written cells when tabbing over them.
	 */
	public void DISABLED_testHorizontalTabColorsBackground() {
		withTerminalSized(10, 3).enterString("\033[48;5;15m").enterString("\t");
		assertCursorAt(0, 8);
		for (int i = 0; i < 10; i++) {
			int expectedColor = i < 8 ? 15 : TextStyle.COLOR_INDEX_BACKGROUND;
			assertEquals(expectedColor, TextStyle.decodeBackColor(getStyleAt(0, i)));
		}
	}

	/**
	 * Test interactions between the cursor overflow bit and various escape sequences.
	 * <p/>
	 * Adapted from hterm:
	 * https://chromium.googlesource.com/chromiumos/platform/assets/+/2337afa5c063127d5ce40ec7fec9b602d096df86%5E%21/#F2
	 */
	public void testClearingOfAutowrap() {
		// Fill a row with the last hyphen wrong, then run a command that
		// modifies the screen, then add a hyphen. The wrap bit should be
		// cleared, so the extra hyphen can fix the row.
		withTerminalSized(15, 6);

		enterString("-----  1  ----X");
		enterString("\033[K-");  // EL

		enterString("-----  2  ----X");
		enterString("\033[J-");  // ED

		enterString("-----  3  ----X");
		enterString("\033[@-");  // ICH

		enterString("-----  4  ----X");
		enterString("\033[P-");  // DCH

		enterString("-----  5  ----X");
		enterString("\033[X-");  // ECH

		// DL will delete the entire line but clear the wrap bit, so we
		// expect a hyphen at the end and nothing else.
		enterString("XXXXXXXXXXXXXXX");
		enterString("\033[M-");  // DL

		assertLinesAre(
				"-----  1  -----",
				"-----  2  -----",
				"-----  3  -----",
				"-----  4  -----",
				"-----  5  -----",
				"              -");
	}

	public void testBackspaceAcrossWrappedLines() {
		// Backspace should not go to previous line if not auto-wrapped:
		withTerminalSized(3, 3).enterString("hi\r\n\b\byou").assertLinesAre("hi ", "you", "   ");
		// Backspace should go to previous line if auto-wrapped:
		withTerminalSized(3, 3).enterString("hi y").assertLinesAre("hi ", "y  ", "   ").enterString("\b\b#").assertLinesAre("hi#", "y  ", "   ");
		// Initial backspace should do nothing:
		withTerminalSized(3, 3).enterString("\b\b\b\bhi").assertLinesAre("hi ", "   ", "   ");
	}

	public void testCursorSaveRestoreLocation() {
		// DEC save/restore
		withTerminalSized(4, 2).enterString("t\0337est\r\nme\0338ry ").assertLinesAre("try ", "me  ");
		// ANSI.SYS save/restore
		withTerminalSized(4, 2).enterString("t\033[sest\r\nme\033[ury ").assertLinesAre("try ", "me  ");
		// Alternate screen enter/exit
		withTerminalSized(4, 2).enterString("t\033[?1049h\033[Hest\r\nme").assertLinesAre("est ", "me  ").enterString("\033[?1049lry").assertLinesAre("try ", "    ");
	}

	public void testCursorSaveRestoreTextStyle() {
		long s;

		// DEC save/restore
		withTerminalSized(4, 2).enterString("\033[31;42;4m..\0337\033[36;47;24m\0338..");
		s = getStyleAt(0, 3);
		Assert.assertEquals(1, TextStyle.decodeForeColor(s));
		Assert.assertEquals(2, TextStyle.decodeBackColor(s));
		Assert.assertEquals(TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE, TextStyle.decodeEffect(s));

		// ANSI.SYS save/restore
		withTerminalSized(4, 2).enterString("\033[31;42;4m..\033[s\033[36;47;24m\033[u..");
		s = getStyleAt(0, 3);
		Assert.assertEquals(1, TextStyle.decodeForeColor(s));
		Assert.assertEquals(2, TextStyle.decodeBackColor(s));
		Assert.assertEquals(TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE, TextStyle.decodeEffect(s));

		// Alternate screen enter/exit
		withTerminalSized(4, 2);
		enterString("\033[31;42;4m..\033[?1049h\033[H\033[36;47;24m.");
		s = getStyleAt(0, 0);
		Assert.assertEquals(6, TextStyle.decodeForeColor(s));
		Assert.assertEquals(7, TextStyle.decodeBackColor(s));
		Assert.assertEquals(0, TextStyle.decodeEffect(s));
		enterString("\033[?1049l..");
		s = getStyleAt(0, 3);
		Assert.assertEquals(1, TextStyle.decodeForeColor(s));
		Assert.assertEquals(2, TextStyle.decodeBackColor(s));
		Assert.assertEquals(TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE, TextStyle.decodeEffect(s));
	}

}
