package com.termux.terminal;

public class ScreenBufferTest extends TerminalTestCase {

	public void testBasics() {
		TerminalBuffer screen = new TerminalBuffer(5, 3, 3);
		assertEquals("", screen.getTranscriptText());
		screen.setChar(0, 0, 'a', 0);
		assertEquals("a", screen.getTranscriptText());
		screen.setChar(0, 0, 'b', 0);
		assertEquals("b", screen.getTranscriptText());
		screen.setChar(2, 0, 'c', 0);
		assertEquals("b c", screen.getTranscriptText());
		screen.setChar(2, 2, 'f', 0);
		assertEquals("b c\n\n  f", screen.getTranscriptText());
		screen.blockSet(0, 0, 2, 2, 'X', 0);
	}

	public void testBlockSet() {
		TerminalBuffer screen = new TerminalBuffer(5, 3, 3);
		screen.blockSet(0, 0, 2, 2, 'X', 0);
		assertEquals("XX\nXX", screen.getTranscriptText());
		screen.blockSet(1, 1, 2, 2, 'Y', 0);
		assertEquals("XX\nXYY\n YY", screen.getTranscriptText());
	}

	public void testGetSelectedText() {
		withTerminalSized(5, 3).enterString("ABCDEFGHIJ").assertLinesAre("ABCDE", "FGHIJ", "     ");
		assertEquals("AB", mTerminal.getSelectedText(0, 0, 1, 0));
		assertEquals("BC", mTerminal.getSelectedText(1, 0, 2, 0));
		assertEquals("CDE", mTerminal.getSelectedText(2, 0, 4, 0));
		assertEquals("FG", mTerminal.getSelectedText(0, 1, 1, 1));
		assertEquals("GH", mTerminal.getSelectedText(1, 1, 2, 1));
		assertEquals("HIJ", mTerminal.getSelectedText(2, 1, 4, 1));

		assertEquals("ABCDEFG", mTerminal.getSelectedText(0, 0, 1, 1));
		withTerminalSized(5, 3).enterString("ABCDE\r\nFGHIJ").assertLinesAre("ABCDE", "FGHIJ", "     ");
		assertEquals("ABCDE\nFG", mTerminal.getSelectedText(0, 0, 1, 1));
	}

	public void testGetSelectedTextJoinFullLines() {
		withTerminalSized(5, 3).enterString("ABCDE\r\nFG");
		assertEquals("ABCDEFG", mTerminal.getScreen().getSelectedText(0, 0, 1, 1, true, true));

		withTerminalSized(5, 3).enterString("ABC\r\nFG");
		assertEquals("ABC\nFG", mTerminal.getScreen().getSelectedText(0, 0, 1, 1, true, true));
	}

	public void testGetWordAtLocation() {
		withTerminalSized(5, 3).enterString("ABCDEFGHIJ\r\nKLMNO");
		assertEquals("ABCDEFGHIJKLMNO", mTerminal.getScreen().getWordAtLocation(0, 0));
		assertEquals("ABCDEFGHIJKLMNO", mTerminal.getScreen().getWordAtLocation(4, 1));
		assertEquals("ABCDEFGHIJKLMNO", mTerminal.getScreen().getWordAtLocation(4, 2));

		withTerminalSized(5, 3).enterString("ABC DEF GHI ");
		assertEquals("ABC", mTerminal.getScreen().getWordAtLocation(0, 0));
		assertEquals("", mTerminal.getScreen().getWordAtLocation(3, 0));
		assertEquals("DEF", mTerminal.getScreen().getWordAtLocation(4, 0));
		assertEquals("DEF", mTerminal.getScreen().getWordAtLocation(0, 1));
		assertEquals("DEF", mTerminal.getScreen().getWordAtLocation(1, 1));
		assertEquals("GHI", mTerminal.getScreen().getWordAtLocation(0, 2));
		assertEquals("", mTerminal.getScreen().getWordAtLocation(1, 2));
		assertEquals("", mTerminal.getScreen().getWordAtLocation(2, 2));
	}
}
