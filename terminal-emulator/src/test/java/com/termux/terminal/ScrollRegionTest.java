package com.termux.terminal;

/**
 * ${CSI}${top};${bottom}r" - set Scrolling Region [top;bottom] (default = full size of window) (DECSTBM).
 * <p/>
 * "DECSTBM moves the cursor to column 1, line 1 of the page" (http://www.vt100.net/docs/vt510-rm/DECSTBM).
 */
public class ScrollRegionTest extends TerminalTestCase {

	public void testScrollRegionTop() {
		withTerminalSized(3, 4).enterString("111222333444").assertLinesAre("111", "222", "333", "444");
		enterString("\033[2r").assertCursorAt(0, 0);
		enterString("\r\n\r\n\r\n\r\nCDEFGH").assertLinesAre("111", "444", "CDE", "FGH").assertHistoryStartsWith("333");
		enterString("IJK").assertLinesAre("111", "CDE", "FGH", "IJK").assertHistoryStartsWith("444");
		// Reset scroll region and enter line:
		enterString("\033[r").enterString("\r\n\r\n\r\n").enterString("LMNOPQ").assertLinesAre("CDE", "FGH", "LMN", "OPQ");
	}

	public void testScrollRegionBottom() {
		withTerminalSized(3, 4).enterString("111222333444");
		assertLinesAre("111", "222", "333", "444");
		enterString("\033[1;3r").assertCursorAt(0, 0);
		enterString("\r\n\r\nCDEFGH").assertLinesAre("222", "CDE", "FGH", "444").assertHistoryStartsWith("111");
		// Reset scroll region and enter line:
		enterString("\033[r").enterString("\r\n\r\n\r\n").enterString("IJKLMN").assertLinesAre("CDE", "FGH", "IJK", "LMN");
	}

	public void testScrollRegionResetWithOriginMode() {
		withTerminalSized(3, 4).enterString("111222333444");
		assertLinesAre("111", "222", "333", "444");
		// "\033[?6h" sets origin mode, so that the later DECSTBM resets cursor to below margin:
		enterString("\033[?6h\033[2r").assertCursorAt(1, 0);
	}

	public void testScrollRegionLeft() {
		// ${CSI}?69h for DECLRMM enabling, ${CSI}${LEFTMARGIN};${RIGHTMARGIN}s for DECSLRM margin setting.
		withTerminalSized(3, 3).enterString("\033[?69h\033[2sABCDEFG").assertLinesAre("ABC", " DE", " FG");
		enterString("HI").assertLinesAre("ADE", " FG", " HI").enterString("JK").assertLinesAre("AFG", " HI", " JK");
		enterString("\n").assertLinesAre("AHI", " JK", "   ");
	}

	public void testScrollRegionRight() {
		// ${CSI}?69h for DECLRMM enabling, ${CSI}${LEFTMARGIN};${RIGHTMARGIN}s for DECSLRM margin setting.
		withTerminalSized(3, 3).enterString("YYY\033[?69h\033[1;2sABCDEF").assertLinesAre("ABY", "CD ", "EF ");
		enterString("GH").assertLinesAre("CDY", "EF ", "GH ").enterString("IJ").assertLinesAre("EFY", "GH ", "IJ ");
		enterString("\n").assertLinesAre("GHY", "IJ ", "   ");
	}

	public void testScrollRegionOnAllSides() {
		// ${CSI}?69h for DECLRMM enabling, ${CSI}${LEFTMARGIN};${RIGHTMARGIN}s for DECSLRM margin setting.
		withTerminalSized(4, 4).enterString("ABCDEFGHIJKLMNOP").assertLinesAre("ABCD", "EFGH", "IJKL", "MNOP");
		// http://www.vt100.net/docs/vt510-rm/DECOM
		enterString("\033[?6h\033[2;3r").assertCursorAt(1, 0);
		enterString("\033[?69h\033[2;3s").assertCursorAt(1, 1);
		enterString("QRST").assertLinesAre("ABCD", "EQRH", "ISTL", "MNOP");
		enterString("UV").assertLinesAre("ABCD", "ESTH", "IUVL", "MNOP");
	}

	public void testDECCOLMResetsScrollMargin() {
		// DECCOLM — Select 80 or 132 Columns per Page (http://www.vt100.net/docs/vt510-rm/DECCOLM) has the important
		// side effect to clear scroll margins, which is useful for e.g. the "reset" utility to clear scroll margins.
		withTerminalSized(3, 4).enterString("111222333444").assertLinesAre("111", "222", "333", "444");
		enterString("\033[2r\033[?3h\r\nABCDEFGHIJKL").assertLinesAre("ABC", "DEF", "GHI", "JKL");
	}

	public void testScrollOutsideVerticalRegion() {
		withTerminalSized(3, 4).enterString("\033[0;2rhi\033[4;0Hyou").assertLinesAre("hi ", "   ", "   ", "you");
		//enterString("see").assertLinesAre("hi ", "   ", "   ", "see");
	}

	public void testNELRespectsLeftMargin() {
		// vttest "Menu 11.3.2: VT420 Cursor-Movement Test", select "10. Test other movement (CR/HT/LF/FF) within margins".
		// The NEL (ESC E) sequence moves cursor to first position on next line, where first position depends on origin mode and margin.
		withTerminalSized(3, 3).enterString("\033[?69h\033[2sABC\033ED").assertLinesAre("ABC", "D  ", "   ");
		withTerminalSized(3, 3).enterString("\033[?69h\033[2sABC\033[?6h\033ED").assertLinesAre("ABC", " D ", "   ");
	}

	public void testRiRespectsLeftMargin() {
		// Reverse Index (RI), ${ESC}M, should respect horizontal margins:
		withTerminalSized(4, 3).enterString("ABCD\033[?69h\033[2;3s\033[?6h\033M").assertLinesAre("A  D", " BC ", "    ");
	}

	public void testSdRespectsLeftMargin() {
		// Scroll Down (SD), ${CSI}${N}T, should respect horizontal margins:
		withTerminalSized(4, 3).enterString("ABCD\033[?69h\033[2;3s\033[?6h\033[2T").assertLinesAre("A  D", "    ", " BC ");
	}

	public void testBackwardIndex() {
		// vttest "Menu 11.3.2: VT420 Cursor-Movement Test", test 7.
		// Without margins:
		withTerminalSized(3, 3).enterString("ABCDEF\0336H").assertLinesAre("ABC", "DHF", "   ");
		enterString("\0336\0336I").assertLinesAre("ABC", "IHF", "   ");
		enterString("\0336\0336").assertLinesAre(" AB", " IH", "   ");
		// With left margin:
		withTerminalSized(3, 3).enterString("\033[?69h\033[2sABCDEF\0336\0336").assertLinesAre("A B", "  D", "  F");
	}

	public void testForwardIndex() {
		// vttest "Menu 11.3.2: VT420 Cursor-Movement Test", test 8.
		// Without margins:
		withTerminalSized(3, 3).enterString("ABCD\0339E").assertLinesAre("ABC", "D E", "   ");
		enterString("\0339").assertLinesAre("BC ", " E ", "   ");
		// With right margin:
		withTerminalSized(3, 3).enterString("\033[?69h\033[0;2sABCD\0339").assertLinesAre("B  ", "D  ", "   ");
	}

	public void testScrollDownWithScrollRegion() {
		withTerminalSized(2, 5).enterString("1\r\n2\r\n3\r\n4\r\n5").assertLinesAre("1 ", "2 ", "3 ", "4 ", "5 ");
		enterString("\033[3r").enterString("\033[2T").assertLinesAre("1 ", "2 ", "  ", "  ", "3 ");
	}

	public void testScrollDownBelowScrollRegion() {
		withTerminalSized(2, 5).enterString("1\r\n2\r\n3\r\n4\r\n5").assertLinesAre("1 ", "2 ", "3 ", "4 ", "5 ");
		enterString("\033[1;3r"); // DECSTBM margins.
		enterString("\033[4;1H"); // Place cursor just below bottom margin.
		enterString("QQ\r\nRR\r\n\r\n\r\nYY");
		assertLinesAre("1 ", "2 ", "3 ", "QQ", "YY");
	}

	/** See https://github.com/termux/termux-app/issues/1340 */
	public void testScrollRegionDoesNotLimitCursorMovement() {
		withTerminalSized(6, 4)
			.enterString("\033[4;7r\033[3;1Haaa\033[Axxx")
			.assertLinesAre(
				"      ",
				"   xxx",
				"aaa   ",
				"      "
			);

		withTerminalSized(6, 4)
			.enterString("\033[1;3r\033[3;1Haaa\033[Bxxx")
			.assertLinesAre(
				"      ",
				"      ",
				"aaa   ",
				"   xxx"
			);
	}

	/**
	 * See <a href="https://github.com/termux/termux-packages/issues/12556">reported issue</a>.
	 */
	public void testClearingWhenScrollingWithMargins() {
		int newForeground = 2;
		int newBackground = 3;
		int size = 3;
		TerminalTestCase terminal = withTerminalSized(size, size)
			// Enable horizontal margin and set left margin to 1:
			.enterString("\033[?69h\033[2s")
			// Set foreground and background color:
			.enterString("\033[" + (30 + newForeground) + ";" + (40 + newBackground) + "m")
			// Enter newlines to scroll down:
			.enterString("\r\n\r\n\r\n\r\n\r\n");
		for (int row = 0; row < size; row++) {
			for (int col = 0; col < size; col++) {
				// The first column (outside of the scrolling area, due to us setting a left scroll
				// margin of 1) should be unmodified, the others should use the current style:
				int expectedForeground = col == 0 ? TextStyle.COLOR_INDEX_FOREGROUND : newForeground;
				int expectedBackground = col == 0 ? TextStyle.COLOR_INDEX_BACKGROUND : newBackground;
				terminal.assertForegroundColorAt(row, col, expectedForeground);
				terminal.assertBackgroundColorAt(row, col, expectedBackground);
			}
		}
	}

}
