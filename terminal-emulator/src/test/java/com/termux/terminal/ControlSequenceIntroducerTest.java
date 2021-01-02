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

	/** CSI Pm m  Set SGR parameter(s) from semicolon-separated list Pm. */
	public void testCsiSGRParameters() {
		// Set more parameters (19) than supported (16).  Additional parameters should be silently consumed.
		withTerminalSized(3, 2).enterString("\033[0;38;2;255;255;255;48;2;0;0;0;1;2;3;4;5;7;8;9mabc").assertLinesAre("abc", "   ");
	}

	/** CSI Ps b  Repeat the preceding graphic character Ps times (REP). */
	public void testRepeat() {
		withTerminalSized(3, 2).enterString("a\033[b").assertLinesAre("aa ", "   ");
		withTerminalSized(3, 2).enterString("a\033[2b").assertLinesAre("aaa", "   ");
		// When no char has been output we ignore REP:
		withTerminalSized(3, 2).enterString("\033[b").assertLinesAre("   ", "   ");
		// This shows that REP outputs the last emitted code point and not the one relative to the
		// current cursor position:
		withTerminalSized(5, 2).enterString("abcde\033[2G\033[2b\n").assertLinesAre("aeede", "     ");
	}

	/** CSI 3 J  Clear scrollback (xterm, libvte; non-standard). */
	public void testCsi3J() {
		withTerminalSized(3, 2).enterString("a\r\nb\r\nc\r\nd");
		assertEquals("a\nb\nc\nd", mTerminal.getScreen().getTranscriptText());
		enterString("\033[3J");
		assertEquals("c\nd", mTerminal.getScreen().getTranscriptText());

		withTerminalSized(3, 2).enterString("Lorem_ipsum");
		assertEquals("Lorem_ipsum", mTerminal.getScreen().getTranscriptText());
		enterString("\033[3J");
		assertEquals("ipsum", mTerminal.getScreen().getTranscriptText());

		withTerminalSized(3, 2).enterString("w\r\nx\r\ny\r\nz\033[?1049h\033[3J\033[?1049l");
		assertEquals("y\nz", mTerminal.getScreen().getTranscriptText());
	}

}
