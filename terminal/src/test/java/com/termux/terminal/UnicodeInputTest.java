package com.termux.terminal;

import java.io.UnsupportedEncodingException;

public class UnicodeInputTest extends TerminalTestCase {

	public void testIllFormedUtf8SuccessorByteNotConsumed() throws Exception {
		// The Unicode Standard Version 6.2 – Core Specification (http://www.unicode.org/versions/Unicode6.2.0/ch03.pdf):
		// "If the converter encounters an ill-formed UTF-8 code unit sequence which starts with a valid first byte, but which does not
		// continue with valid successor bytes (see Table 3-7), it must not consume the successor bytes as part of the ill-formed
		// subsequence whenever those successor bytes themselves constitute part of a well-formed UTF-8 code unit subsequence."
		withTerminalSized(5, 5);
		mTerminal.append(new byte[]{(byte) 0b11101111, (byte) 'a'}, 2);
		assertLineIs(0, ((char) TerminalEmulator.UNICODE_REPLACEMENT_CHAR) + "a   ");

		// https://code.google.com/p/chromium/issues/detail?id=212704
		byte[] input = new byte[]{
				(byte) 0x61, (byte) 0xF1,
				(byte) 0x80, (byte) 0x80,
				(byte) 0xe1, (byte) 0x80,
				(byte) 0xc2, (byte) 0x62,
				(byte) 0x80, (byte) 0x63,
				(byte) 0x80, (byte) 0xbf,
				(byte) 0x64
		};
		withTerminalSized(10, 2);
		mTerminal.append(input, input.length);
		assertLinesAre("a\uFFFD\uFFFD\uFFFDb\uFFFDc\uFFFD\uFFFDd", "          ");

		// Surrogate pairs.
		withTerminalSized(5, 2);
		input = new byte[]{
				(byte) 0xed, (byte) 0xa0,
				(byte) 0x80, (byte) 0xed,
				(byte) 0xad, (byte) 0xbf,
				(byte) 0xed, (byte) 0xae,
				(byte) 0x80, (byte) 0xed,
				(byte) 0xbf, (byte) 0xbf
		};
		mTerminal.append(input, input.length);
		assertLinesAre("\uFFFD\uFFFD\uFFFD\uFFFD ", "     ");

		// https://bugzilla.mozilla.org/show_bug.cgi?id=746900: "with this patch 0xe0 0x80 is decoded as two U+FFFDs,
		// but 0xe0 0xa0 is decoded as a single U+FFFD, and this is correct according to the "Best Practices", but IE
		// and Chrome (Version 22.0.1229.94) decode both of them as two U+FFFDs. Opera 12.11 decodes both of them as
		// one U+FFFD".
		withTerminalSized(5, 2);
		input = new byte[]{(byte) 0xe0, (byte) 0xa0, ' '};
		mTerminal.append(input, input.length);
		assertLinesAre("\uFFFD    ", "     ");

		// withTerminalSized(5, 2);
		// input = new byte[]{(byte) 0xe0, (byte) 0x80, 'a'};
		// mTerminal.append(input, input.length);
		// assertLinesAre("\uFFFD\uFFFDa  ", "     ");
	}

	public void testUnassignedCodePoint() throws UnsupportedEncodingException {
		withTerminalSized(3, 3);
		// UTF-8 for U+C2541, an unassigned code point:
		byte[] b = new byte[]{(byte) 0xf3, (byte) 0x82, (byte) 0x95, (byte) 0x81};
		mTerminal.append(b, b.length);
		enterString("Y");
		assertEquals(1, Character.charCount(TerminalEmulator.UNICODE_REPLACEMENT_CHAR));
		assertLineStartsWith(0, TerminalEmulator.UNICODE_REPLACEMENT_CHAR, (int) 'Y', ' ');
	}

	public void testStuff() {
		withTerminalSized(80, 24);
		byte[] b = new byte[]{(byte) 0xf3, (byte) 0x82, (byte) 0x95, (byte) 0x81, (byte) 0x61, (byte) 0x38, (byte) 0xe7, (byte) 0x8f,
				(byte) 0xae, (byte) 0xc2, (byte) 0x9f, (byte) 0xe8, (byte) 0xa0, (byte) 0x9f, (byte) 0xe8, (byte) 0x8c, (byte) 0xa4,
				(byte) 0xed, (byte) 0x93, (byte) 0x89, (byte) 0xef, (byte) 0xbf, (byte) 0xbd, (byte) 0x42, (byte) 0xc2, (byte) 0x9b,
				(byte) 0xe6, (byte) 0x87, (byte) 0x89, (byte) 0x5a};
		mTerminal.append(b, b.length);
	}

	public void testSimpleCombining() throws Exception {
		withTerminalSized(3, 2).enterString(" a\u0302 ").assertLinesAre(" a\u0302 ", "   ");
	}

	public void testCombiningCharacterInFirstColumn() throws Exception {
		withTerminalSized(5, 3).enterString("test\r\nhi\r\n").assertLinesAre("test ", "hi   ", "     ");

		// U+0302 is COMBINING CIRCUMFLEX ACCENT. Test case from mosh (http://mosh.mit.edu/).
		withTerminalSized(5, 5).enterString("test\r\nabc\r\n\u0302\r\ndef\r\n");
		assertLinesAre("test ", "abc  ", " \u0302    ", "def  ", "     ");
	}

	public void testCombiningCharacterInLastColumn() throws Exception {
		withTerminalSized(3, 2).enterString("  a\u0302").assertLinesAre("  a\u0302", "   ");
		withTerminalSized(3, 2).enterString("  à̲").assertLinesAre("  à̲", "   ");
		withTerminalSized(3, 2).enterString("Aà̲F").assertLinesAre("Aà̲F", "   ");
	}

	public void testWideCharacterInLastColumn() throws Exception {
		withTerminalSized(3, 2).enterString("  枝\u0302").assertLinesAre("   ", "枝\u0302 ");

		withTerminalSized(3, 2).enterString(" 枝").assertLinesAre(" 枝", "   ").assertCursorAt(0, 2);
		enterString("a").assertLinesAre(" 枝", "a  ");
	}

	public void testWideCharacterDeletion() throws Exception {
		// CSI Ps D Cursor Backward Ps Times
		withTerminalSized(3, 2).enterString("枝\033[Da").assertLinesAre(" a ", "   ");
		withTerminalSized(3, 2).enterString("枝\033[2Da").assertLinesAre("a  ", "   ");
		withTerminalSized(3, 2).enterString("枝\033[2D枝").assertLinesAre("枝 ", "   ");
		withTerminalSized(3, 2).enterString("枝\033[1D枝").assertLinesAre(" 枝", "   ");
		withTerminalSized(5, 2).enterString(" 枝 \033[Da").assertLinesAre(" 枝a ", "     ");
		withTerminalSized(5, 2).enterString("a \033[D\u0302").assertLinesAre("a\u0302    ", "     ");
		withTerminalSized(5, 2).enterString("枝 \033[D\u0302").assertLinesAre("枝\u0302   ", "     ");
		enterString("Z").assertLinesAre("枝\u0302Z  ", "     ");
		enterString("\033[D ").assertLinesAre("枝\u0302   ", "     ");
		// Go back two columns, standing at the second half of the wide character:
		enterString("\033[2DU").assertLinesAre(" U   ", "     ");
	}

	public void testWideCharOverwriting() {
		withTerminalSized(3, 2).enterString("abc\033[3D枝").assertLinesAre("枝c", "   ");
	}

	public void testOverlongUtf8Encoding() throws Exception {
		// U+0020 should be encoded as 0x20, 0xc0 0xa0 is an overlong encoding
		// so should be replaced with the replacement char U+FFFD.
		withTerminalSized(5, 5).mTerminal.append(new byte[]{(byte) 0xc0, (byte) 0xa0, 'Y'}, 3);
		assertLineIs(0, "\uFFFDY   ");
	}

	public void testWideCharacterWithoutWrapping() throws Exception {
		// With wraparound disabled. The behaviour when a wide character is output with cursor in
		// the last column when autowrap is disabled is not obvious, but we expect the wide
		// character to be ignored here.
		withTerminalSized(3, 3).enterString("\033[?7l").enterString("枝枝枝").assertLinesAre("枝 ", "   ", "   ");
		enterString("a枝").assertLinesAre("枝a", "   ", "   ");
	}

}
