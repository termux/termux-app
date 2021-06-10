package com.termux.terminal;

import java.io.UnsupportedEncodingException;

public class TerminalTest extends TerminalTestCase {

	public void testCursorPositioning() throws Exception {
		withTerminalSized(10, 10).placeCursorAndAssert(1, 2).placeCursorAndAssert(3, 5).placeCursorAndAssert(2, 2).enterString("A")
				.assertCursorAt(2, 3);
	}

	public void testScreen() throws UnsupportedEncodingException {
		withTerminalSized(3, 3);
		assertLinesAre("   ", "   ", "   ");

		assertEquals("", mTerminal.getScreen().getTranscriptText());
		enterString("hi").assertLinesAre("hi ", "   ", "   ");
		assertEquals("hi", mTerminal.getScreen().getTranscriptText());
		enterString("\r\nu");
		assertEquals("hi\nu", mTerminal.getScreen().getTranscriptText());
		mTerminal.reset();
		assertEquals("hi\nu", mTerminal.getScreen().getTranscriptText());

		withTerminalSized(3, 3).enterString("hello");
		assertEquals("hello", mTerminal.getScreen().getTranscriptText());
		enterString("\r\nworld");
		assertEquals("hello\nworld", mTerminal.getScreen().getTranscriptText());
	}

	public void testScrollDownInAltBuffer() {
		withTerminalSized(3, 3).enterString("\033[?1049h");
		enterString("\033[38;5;111m1\r\n");
		enterString("\033[38;5;112m2\r\n");
		enterString("\033[38;5;113m3\r\n");
		enterString("\033[38;5;114m4\r\n");
		enterString("\033[38;5;115m5");
		assertLinesAre("3  ", "4  ", "5  ");
		assertForegroundColorAt(0, 0, 113);
		assertForegroundColorAt(1, 0, 114);
		assertForegroundColorAt(2, 0, 115);
	}

	public void testMouseClick() throws Exception {
		withTerminalSized(10, 10);
		assertFalse(mTerminal.isMouseTrackingActive());
		enterString("\033[?1000h");
		assertTrue(mTerminal.isMouseTrackingActive());
		enterString("\033[?1000l");
		assertFalse(mTerminal.isMouseTrackingActive());
		enterString("\033[?1000h");
		assertTrue(mTerminal.isMouseTrackingActive());

		enterString("\033[?1006h");
		mTerminal.sendMouseEvent(TerminalEmulator.MOUSE_LEFT_BUTTON, 3, 4, true);
		assertEquals("\033[<0;3;4M", mOutput.getOutputAndClear());
		mTerminal.sendMouseEvent(TerminalEmulator.MOUSE_LEFT_BUTTON, 3, 4, false);
		assertEquals("\033[<0;3;4m", mOutput.getOutputAndClear());

		// When the client says that a click is outside (which could happen when pixels are outside
		// the terminal area, see https://github.com/termux/termux-app/issues/501) the terminal
		// sends a click at the edge.
		mTerminal.sendMouseEvent(TerminalEmulator.MOUSE_LEFT_BUTTON, 0, 0, true);
		assertEquals("\033[<0;1;1M", mOutput.getOutputAndClear());
		mTerminal.sendMouseEvent(TerminalEmulator.MOUSE_LEFT_BUTTON, 11, 11, false);
		assertEquals("\033[<0;10;10m", mOutput.getOutputAndClear());
	}

	public void testNormalization() throws UnsupportedEncodingException {
		// int lowerCaseN = 0x006E;
		// int combiningTilde = 0x0303;
		// int combined = 0x00F1;
		withTerminalSized(3, 3).assertLinesAre("   ", "   ", "   ");
		enterString("\u006E\u0303");
		assertEquals(1, WcWidth.width("\u006E\u0303".toCharArray(), 0));
		// assertEquals("\u00F1  ", new String(mTerminal.getScreen().getLine(0)));
		assertLinesAre("\u006E\u0303  ", "   ", "   ");
	}

	/** On "\e[18t" xterm replies with "\e[8;${HEIGHT};${WIDTH}t" */
	public void testReportTerminalSize() throws Exception {
		withTerminalSized(5, 5);
		assertEnteringStringGivesResponse("\033[18t", "\033[8;5;5t");
		for (int width = 3; width < 12; width++) {
			for (int height = 3; height < 12; height++) {
				mTerminal.resize(width, height);
				assertEnteringStringGivesResponse("\033[18t", "\033[8;" + height + ";" + width + "t");
			}
		}
	}

	/** Device Status Report (DSR) and Report Cursor Position (CPR). */
	public void testDeviceStatusReport() throws Exception {
		withTerminalSized(5, 5);
		assertEnteringStringGivesResponse("\033[5n", "\033[0n");

		assertEnteringStringGivesResponse("\033[6n", "\033[1;1R");
		enterString("AB");
		assertEnteringStringGivesResponse("\033[6n", "\033[1;3R");
		enterString("\r\n");
		assertEnteringStringGivesResponse("\033[6n", "\033[2;1R");
	}

	/** Test the cursor shape changes using DECSCUSR. */
	public void testSetCursorStyle() throws Exception {
		withTerminalSized(5, 5);
		assertEquals(TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK, mTerminal.getCursorStyle());
		enterString("\033[3 q");
		assertEquals(TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE, mTerminal.getCursorStyle());
		enterString("\033[5 q");
		assertEquals(TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR, mTerminal.getCursorStyle());
		enterString("\033[0 q");
		assertEquals(TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK, mTerminal.getCursorStyle());
		enterString("\033[6 q");
		assertEquals(TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR, mTerminal.getCursorStyle());
		enterString("\033[4 q");
		assertEquals(TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE, mTerminal.getCursorStyle());
		enterString("\033[1 q");
		assertEquals(TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK, mTerminal.getCursorStyle());
		enterString("\033[4 q");
		assertEquals(TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE, mTerminal.getCursorStyle());
		enterString("\033[2 q");
		assertEquals(TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK, mTerminal.getCursorStyle());
	}

	public void testPaste() {
		withTerminalSized(5, 5);
		mTerminal.paste("hi");
		assertEquals("hi", mOutput.getOutputAndClear());

		enterString("\033[?2004h");
		mTerminal.paste("hi");
		assertEquals("\033[200~" + "hi" + "\033[201~", mOutput.getOutputAndClear());

		enterString("\033[?2004l");
		mTerminal.paste("hi");
		assertEquals("hi", mOutput.getOutputAndClear());
	}

	public void testSelectGraphics() {
		withTerminalSized(5, 5);
		enterString("\033[31m");
		assertEquals(mTerminal.mForeColor, 1);
		enterString("\033[32m");
		assertEquals(mTerminal.mForeColor, 2);
		enterString("\033[43m");
		assertEquals(2, mTerminal.mForeColor);
		assertEquals(3, mTerminal.mBackColor);

		// SGR 0 should reset both foreground and background color.
		enterString("\033[0m");
		assertEquals(TextStyle.COLOR_INDEX_FOREGROUND, mTerminal.mForeColor);
		assertEquals(TextStyle.COLOR_INDEX_BACKGROUND, mTerminal.mBackColor);

		// 256 colors:
		enterString("\033[38;5;119m");
		assertEquals(119, mTerminal.mForeColor);
		assertEquals(TextStyle.COLOR_INDEX_BACKGROUND, mTerminal.mBackColor);
		enterString("\033[48;5;129m");
		assertEquals(119, mTerminal.mForeColor);
		assertEquals(129, mTerminal.mBackColor);

		// Invalid parameter:
		enterString("\033[48;8;129m");
		assertEquals(119, mTerminal.mForeColor);
		assertEquals(129, mTerminal.mBackColor);

		// Multiple parameters at once:
		enterString("\033[38;5;178;48;5;179;m");
		assertEquals(178, mTerminal.mForeColor);
		assertEquals(179, mTerminal.mBackColor);

		// 24 bit colors:
		enterString(("\033[0m")); // Reset fg and bg colors.
		enterString("\033[38;2;255;127;2m");
		int expectedForeground = 0xff000000 | (255 << 16) | (127 << 8) | 2;
		assertEquals(expectedForeground, mTerminal.mForeColor);
		assertEquals(TextStyle.COLOR_INDEX_BACKGROUND, mTerminal.mBackColor);
		enterString("\033[48;2;1;2;254m");
		int expectedBackground = 0xff000000 | (1 << 16) | (2 << 8) | 254;
		assertEquals(expectedForeground, mTerminal.mForeColor);
		assertEquals(expectedBackground, mTerminal.mBackColor);

		// 24 bit colors, set fg and bg at once:
		enterString(("\033[0m")); // Reset fg and bg colors.
		assertEquals(TextStyle.COLOR_INDEX_FOREGROUND, mTerminal.mForeColor);
		assertEquals(TextStyle.COLOR_INDEX_BACKGROUND, mTerminal.mBackColor);
		enterString("\033[38;2;255;127;2;48;2;1;2;254m");
		assertEquals(expectedForeground, mTerminal.mForeColor);
		assertEquals(expectedBackground, mTerminal.mBackColor);

		// 24 bit colors, invalid input:
		enterString("\033[38;2;300;127;2;48;2;1;300;254m");
		assertEquals(expectedForeground, mTerminal.mForeColor);
		assertEquals(expectedBackground, mTerminal.mBackColor);
	}

	public void testBackgroundColorErase() {
		final int rows = 3;
		final int cols = 3;
		withTerminalSized(cols, rows);
		for (int r = 0; r < rows; r++) {
			for (int c = 0; c < cols; c++) {
				long style = getStyleAt(r, c);
				assertEquals(TextStyle.COLOR_INDEX_FOREGROUND, TextStyle.decodeForeColor(style));
				assertEquals(TextStyle.COLOR_INDEX_BACKGROUND, TextStyle.decodeBackColor(style));
			}
		}
		// Foreground color to 119:
		enterString("\033[38;5;119m");
		// Background color to 129:
		enterString("\033[48;5;129m");
		// Clear with ED, Erase in Display:
		enterString("\033[2J");
		for (int r = 0; r < rows; r++) {
			for (int c = 0; c < cols; c++) {
				long style = getStyleAt(r, c);
				assertEquals(119, TextStyle.decodeForeColor(style));
				assertEquals(129, TextStyle.decodeBackColor(style));
			}
		}
		// Background color to 139:
		enterString("\033[48;5;139m");
		// Insert two blank lines.
		enterString("\033[2L");
		for (int r = 0; r < rows; r++) {
			for (int c = 0; c < cols; c++) {
				long style = getStyleAt(r, c);
				assertEquals((r == 0 || r == 1) ? 139 : 129, TextStyle.decodeBackColor(style));
			}
		}

		withTerminalSized(cols, rows);
		// Background color to 129:
		enterString("\033[48;5;129m");
		// Erase two characters, filling them with background color:
		enterString("\033[2X");
		assertEquals(129, TextStyle.decodeBackColor(getStyleAt(0, 0)));
		assertEquals(129, TextStyle.decodeBackColor(getStyleAt(0, 1)));
		assertEquals(TextStyle.COLOR_INDEX_BACKGROUND, TextStyle.decodeBackColor(getStyleAt(0, 2)));
	}

	public void testParseColor() {
		assertEquals(0xFF0000FA, TerminalColors.parse("#0000FA"));
		assertEquals(0xFF000000, TerminalColors.parse("#000000"));
		assertEquals(0xFF000000, TerminalColors.parse("#000"));
		assertEquals(0xFF000000, TerminalColors.parse("#000000000"));
		assertEquals(0xFF53186f, TerminalColors.parse("#53186f"));

		assertEquals(0xFFFF00FF, TerminalColors.parse("rgb:F/0/F"));
		assertEquals(0xFF0000FA, TerminalColors.parse("rgb:00/00/FA"));
		assertEquals(0xFF53186f, TerminalColors.parse("rgb:53/18/6f"));

		assertEquals(0, TerminalColors.parse("invalid_0000FA"));
		assertEquals(0, TerminalColors.parse("#3456"));
	}

	/** The ncurses library still uses this. */
	public void testLineDrawing() {
		// 016 - shift out / G1. 017 - shift in / G0. "ESC ) 0" - use line drawing for G1
		withTerminalSized(4, 2).enterString("q\033)0q\016q\017q").assertLinesAre("qq─q", "    ");
		// "\0337", saving cursor should save G0, G1 and invoked charset and "ESC 8" should restore.
		withTerminalSized(4, 2).enterString("\033)0\016qqq\0337\017\0338q").assertLinesAre("────", "    ");
	}

	public void testSoftTerminalReset() {
		// See http://vt100.net/docs/vt510-rm/DECSTR and https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=650304
		// "\033[?7l" is DECRST to disable wrap-around, and DECSTR ("\033[!p") should reset it.
		withTerminalSized(3, 3).enterString("\033[?7lABCD").assertLinesAre("ABD", "   ", "   ");
		enterString("\033[!pEF").assertLinesAre("ABE", "F  ", "   ");
	}

	public void testBel() {
		withTerminalSized(3, 3);
		assertEquals(0, mOutput.bellsRung);
		enterString("\07");
		assertEquals(1, mOutput.bellsRung);
		enterString("hello\07");
		assertEquals(2, mOutput.bellsRung);
		enterString("\07hello");
		assertEquals(3, mOutput.bellsRung);
		enterString("hello\07world");
		assertEquals(4, mOutput.bellsRung);
	}

	public void testAutomargins() throws UnsupportedEncodingException {
		withTerminalSized(3, 3).enterString("abc").assertLinesAre("abc", "   ", "   ").assertCursorAt(0, 2);
		enterString("d").assertLinesAre("abc", "d  ", "   ").assertCursorAt(1, 1);

		withTerminalSized(3, 3).enterString("abc\r ").assertLinesAre(" bc", "   ", "   ").assertCursorAt(0, 1);
	}

	public void testTab() {
		withTerminalSized(11, 2).enterString("01234567890\r\tXX").assertLinesAre("01234567XX0", "           ");
		withTerminalSized(11, 2).enterString("01234567890\033[44m\r\tXX").assertLinesAre("01234567XX0", "           ");
	}

}
