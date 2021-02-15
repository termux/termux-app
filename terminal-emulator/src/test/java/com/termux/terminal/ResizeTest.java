package com.termux.terminal;

public class ResizeTest extends TerminalTestCase {

	public void testResizeWhenHasHistory() {
		final int cols = 3;
		withTerminalSized(cols, 3).enterString("111222333444555666777888999").assertCursorAt(2, 2).assertLinesAre("777", "888", "999");
		resize(cols, 5).assertCursorAt(4, 2).assertLinesAre("555", "666", "777", "888", "999");
		resize(cols, 3).assertCursorAt(2, 2).assertLinesAre("777", "888", "999");
	}

	public void testResizeWhenInAltBuffer() {
		final int rows = 3, cols = 3;
		withTerminalSized(cols, rows).enterString("a\r\ndef$").assertLinesAre("a  ", "def", "$  ").assertCursorAt(2, 1);

		// Resize and back again while in main buffer:
		resize(cols, 5).assertLinesAre("a  ", "def", "$  ", "   ", "   ").assertCursorAt(2, 1);
		resize(cols, rows).assertLinesAre("a  ", "def", "$  ").assertCursorAt(2, 1);

		// Switch to alt buffer:
		enterString("\033[?1049h").assertLinesAre("   ", "   ", "   ").assertCursorAt(2, 1);
		enterString("h").assertLinesAre("   ", "   ", " h ").assertCursorAt(2, 2);

		resize(cols, 5).resize(cols, rows);

		// Switch from alt buffer:
		enterString("\033[?1049l").assertLinesAre("a  ", "def", "$  ").assertCursorAt(2, 1);
	}

	public void testShrinkingInAltBuffer() {
		final int rows = 5;
		final int cols = 3;
		withTerminalSized(cols, rows).enterString("A\r\nB\r\nC\r\nD\r\nE").assertLinesAre("A  ", "B  ", "C  ", "D  ", "E  ");
		enterString("\033[?1049h").assertLinesAre("   ", "   ", "   ", "   ", "   ");
		resize(3, 3).enterString("\033[?1049lF").assertLinesAre("C  ", "D  ", "EF ");
	}

	public void testResizeAfterNewlineWhenInAltBuffer() {
		final int rows = 3;
		final int cols = 3;
		withTerminalSized(cols, rows);
		enterString("a\r\nb\r\nc\r\nd\r\ne\r\nf\r\n").assertLinesAre("e  ", "f  ", "   ").assertCursorAt(2, 0);
		assertLineWraps(false, false, false);

		// Switch to alt buffer:
		enterString("\033[?1049h").assertLinesAre("   ", "   ", "   ").assertCursorAt(2, 0);
		enterString("h").assertLinesAre("   ", "   ", "h  ").assertCursorAt(2, 1);

		// Grow by two rows:
		resize(cols, 5).assertLinesAre("   ", "   ", "h  ", "   ", "   ").assertCursorAt(2, 1);
		resize(cols, rows).assertLinesAre("   ", "   ", "h  ").assertCursorAt(2, 1);

		// Switch from alt buffer:
		enterString("\033[?1049l").assertLinesAre("e  ", "f  ", "   ").assertCursorAt(2, 0);
	}

	public void testResizeAfterHistoryWraparound() {
		final int rows = 3;
		final int cols = 10;
		withTerminalSized(cols, rows);
		StringBuilder buffer = new StringBuilder();
		for (int i = 0; i < 1000; i++) {
			String s = Integer.toString(i);
			enterString(s);
			buffer.setLength(0);
			buffer.append(s);
			while (buffer.length() < cols)
				buffer.append(' ');
			if (i > rows) {
				assertLineIs(rows - 1, buffer.toString());
			}
			enterString("\r\n");
		}
		assertLinesAre("998       ", "999       ", "          ");
		mTerminal.resize(cols, 2);
		assertLinesAre("999       ", "          ");
		mTerminal.resize(cols, 5);
		assertLinesAre("996       ", "997       ", "998       ", "999       ", "          ");
		mTerminal.resize(cols, rows);
		assertLinesAre("998       ", "999       ", "          ");
	}

	public void testVerticalResize() {
		final int rows = 5;
		final int cols = 3;

		withTerminalSized(cols, rows);
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
		enterString("11\r\n22");
		assertLinesAre("11 ", "22 ", "   ", "   ", "   ").assertLineWraps(false, false, false, false, false);
		resize(cols, rows - 2).assertLinesAre("11 ", "22 ", "   ");

		// After resize, screen should still be same color:
		for (int r = 0; r < rows - 2; r++) {
			for (int c = 0; c < cols; c++) {
				long style = getStyleAt(r, c);
				assertEquals(119, TextStyle.decodeForeColor(style));
				assertEquals(129, TextStyle.decodeBackColor(style));
			}
		}

		// Background color to 200 and grow back size (which should be cleared to the new background color):
		enterString("\033[48;5;200m");
		resize(cols, rows);
		for (int r = 0; r < rows; r++) {
			for (int c = 0; c < cols; c++) {
                long style = getStyleAt(r, c);
				assertEquals(119, TextStyle.decodeForeColor(style));
				assertEquals("wrong at row=" + r, r >= 3 ? 200 : 129, TextStyle.decodeBackColor(style));
			}
		}
	}

	public void testHorizontalResize() {
		final int rows = 5;
		final int cols = 5;

		withTerminalSized(cols, rows);
		// Background color to 129:
		// enterString("\033[48;5;129m").assertLinesAre(" ", " ", " ", " ", " ");
		enterString("1111\r\n2222\r\n3333\r\n4444\r\n5555").assertCursorAt(4, 4);
		// assertEquals(129, TextStyle.decodeBackColor(getStyleAt(2, 2)));
		assertLinesAre("1111 ", "2222 ", "3333 ", "4444 ", "5555 ").assertLineWraps(false, false, false, false, false);
		resize(cols + 2, rows).assertLinesAre("1111   ", "2222   ", "3333   ", "4444   ", "5555   ").assertCursorAt(4, 4);
		assertLineWraps(false, false, false, false, false);
		resize(cols, rows).assertLinesAre("1111 ", "2222 ", "3333 ", "4444 ", "5555 ").assertCursorAt(4, 4);
		assertLineWraps(false, false, false, false, false);
		resize(cols - 1, rows).assertLinesAre("2222", "3333", "4444", "5555", "    ").assertCursorAt(4, 0);
		assertLineWraps(false, false, false, true, false);
		resize(cols - 2, rows).assertLinesAre("3  ", "444", "4  ", "555", "5  ").assertCursorAt(4, 1);
		assertLineWraps(false, true, false, true, false);
		// Back to original size:
		resize(cols, rows).assertLinesAre("1111 ", "2222 ", "3333 ", "4444 ", "5555 ").assertCursorAt(4, 4);
		assertLineWraps(false, false, false, false, false);
	}

	public void testLineWrap() {
		final int rows = 3, cols = 5;
		withTerminalSized(cols, rows).enterString("111111").assertLinesAre("11111", "1    ", "     ");
		assertCursorAt(1, 1).assertLineWraps(true, false, false);

		resize(7, rows).assertCursorAt(0, 6).assertLinesAre("111111 ", "       ", "       ").assertLineWraps(false, false, false);
		resize(cols, rows).assertCursorAt(1, 1).assertLinesAre("11111", "1    ", "     ").assertLineWraps(true, false, false);

		enterString("2").assertLinesAre("11111", "12   ", "     ").assertLineWraps(true, false, false);
		enterString("123").assertLinesAre("11111", "12123", "     ").assertLineWraps(true, false, false);
		enterString("W").assertLinesAre("11111", "12123", "W    ").assertLineWraps(true, true, false);

		withTerminalSized(cols, rows).enterString("1234512345");
		assertLinesAre("12345", "12345", "     ").assertLineWraps(true, false, false);
		enterString("W").assertLinesAre("12345", "12345", "W    ").assertLineWraps(true, true, false);
	}

	public void testCursorPositionWhenShrinking() {
		final int rows = 5, cols = 3;
		withTerminalSized(cols, rows).enterString("$ ").assertLinesAre("$  ", "   ", "   ", "   ", "   ").assertCursorAt(0, 2);
		resize(3, 3).assertLinesAre("$  ", "   ", "   ").assertCursorAt(0, 2);
		resize(cols, rows).assertLinesAre("$  ", "   ", "   ", "   ", "   ").assertCursorAt(0, 2);
	}

	public void testResizeWithCombiningCharInLastColumn() {
		withTerminalSized(3, 3).enterString("ABC\u0302DEF").assertLinesAre("ABC\u0302", "DEF", "   ");
		resize(4, 3).assertLinesAre("ABC\u0302D", "EF  ", "    ");

		// Same as above but with colors:
		withTerminalSized(3, 3).enterString("\033[37mA\033[35mB\033[33mC\u0302\033[32mD\033[31mE\033[34mF").assertLinesAre("ABC\u0302",
				"DEF", "   ");
		resize(4, 3).assertLinesAre("ABC\u0302D", "EF  ", "    ");
		assertForegroundIndices(effectLine(7, 5, 3, 2), effectLine(1, 4, 4, 4), effectLine(4, 4, 4, 4));
	}

	public void testResizeWithLineWrappingContinuing() {
		withTerminalSized(5, 3).enterString("\r\nAB DE").assertLinesAre("     ", "AB DE", "     ");
		resize(4, 3).assertLinesAre("AB D", "E   ", "    ");
		resize(3, 3).assertLinesAre("AB ", "DE ", "   ");
		resize(5, 3).assertLinesAre("     ", "AB DE", "     ");
	}

	public void testResizeWithWideChars() {
		final int rows = 3, cols = 4;
		String twoCharsWidthOne = new String(Character.toChars(TerminalRowTest.TWO_JAVA_CHARS_DISPLAY_WIDTH_ONE_1));
		withTerminalSized(cols, rows).enterString(twoCharsWidthOne).enterString("\r\n");
		enterString(twoCharsWidthOne).assertLinesAre(twoCharsWidthOne + "   ", twoCharsWidthOne + "   ", "    ");
		resize(3, 3).assertLinesAre(twoCharsWidthOne + "  ", twoCharsWidthOne + "  ", "   ");
		enterString(twoCharsWidthOne).assertLinesAre(twoCharsWidthOne + "  ", twoCharsWidthOne + twoCharsWidthOne + " ", "   ");
	}

	public void testResizeWithMoreWideChars() {
		final int rows = 4, cols = 5;

		withTerminalSized(cols, rows).enterString("qqrr").assertLinesAre("qqrr ", "     ", "     ", "     ");
		resize(2, rows).assertLinesAre("qq", "rr", "  ", "  ");
		resize(5, rows).assertLinesAre("qqrr ", "     ", "     ", "     ");

		withTerminalSized(cols, rows).enterString("ＱＲ").assertLinesAre("ＱＲ ", "     ", "     ", "     ");
		resize(2, rows).assertLinesAre("Ｑ", "Ｒ", "  ", "  ");
		resize(5, rows).assertLinesAre("ＱＲ ", "     ", "     ", "     ");
	}

}
