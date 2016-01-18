package com.termux.terminal;

/** "\033[" is the Control Sequence Introducer char sequence (CSI). */
public class ControlSequenceIntroducerTest extends TerminalTestCase {

	/** CSI Ps P Scroll down Ps lines (default = 1) (SD). */
	public void testCsiT() {
		withTerminalSized(4, 6).enterString("1\r\n2\r\n3\r\nhi\033[2Tyo\r\nA\r\nB").assertLinesAre("    ", "    ", "1   ", "2 yo", "A   ",
				"Bi  ");
		// Default value (1):
		withTerminalSized(4, 6).enterString("1\r\n2\r\n3\r\nhi\033[Tyo\r\nA\r\nB").assertLinesAre("    ", "1   ", "2   ", "3 yo", "Ai  ",
				"B   ");
	}

	/** CSI Ps S Scroll up Ps lines (default = 1) (SU). */
	public void testCsiS() {
		// The behaviour here is a bit inconsistent between terminals - this is how the OS X Terminal.app does it:
		withTerminalSized(3, 4).enterString("1\r\n2\r\n3\r\nhi\033[2Sy").assertLinesAre("3  ", "hi ", "   ", "  y");
		// Default value (1):
		withTerminalSized(3, 4).enterString("1\r\n2\r\n3\r\nhi\033[Sy").assertLinesAre("2  ", "3  ", "hi ", "  y");
	}

	/** CSI Ps X  Erase Ps Character(s) (default = 1) (ECH). */
	public void testCsiX() {
		// See https://code.google.com/p/chromium/issues/detail?id=212712 where test was extraced from.
		withTerminalSized(13, 2).enterString("abcdefghijkl\b\b\b\b\b\033[X").assertLinesAre("abcdefg ijkl ", "             ");
		withTerminalSized(13, 2).enterString("abcdefghijkl\b\b\b\b\b\033[1X").assertLinesAre("abcdefg ijkl ", "             ");
		withTerminalSized(13, 2).enterString("abcdefghijkl\b\b\b\b\b\033[2X").assertLinesAre("abcdefg  jkl ", "             ");
		withTerminalSized(13, 2).enterString("abcdefghijkl\b\b\b\b\b\033[20X").assertLinesAre("abcdefg      ", "             ");
	}

}
