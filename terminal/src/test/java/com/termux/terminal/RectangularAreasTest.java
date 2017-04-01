package com.termux.terminal;

public class RectangularAreasTest extends TerminalTestCase {

	/** http://www.vt100.net/docs/vt510-rm/DECFRA */
	public void testFillRectangularArea() {
		withTerminalSized(3, 3).enterString("\033[88$x").assertLinesAre("XXX", "XXX", "XXX");
		withTerminalSized(3, 3).enterString("\033[88;1;1;2;10$x").assertLinesAre("XXX", "XXX", "   ");
		withTerminalSized(3, 3).enterString("\033[88;2;1;3;10$x").assertLinesAre("   ", "XXX", "XXX");
		withTerminalSized(3, 3).enterString("\033[88;1;1;100;1$x").assertLinesAre("X  ", "X  ", "X  ");
		withTerminalSized(3, 3).enterString("\033[88;1;1;100;2$x").assertLinesAre("XX ", "XX ", "XX ");
		withTerminalSized(3, 3).enterString("\033[88;100;1;100;2$x").assertLinesAre("   ", "   ", "   ");
	}

	/** http://www.vt100.net/docs/vt510-rm/DECERA */
	public void testEraseRectangularArea() {
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[$z").assertLinesAre("   ", "   ", "   ");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[1;1;2;10$z").assertLinesAre("   ", "   ", "GHI");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[2;1;3;10$z").assertLinesAre("ABC", "   ", "   ");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[1;1;100;1$z").assertLinesAre(" BC", " EF", " HI");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[1;1;100;2$z").assertLinesAre("  C", "  F", "  I");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[100;1;100;2$z").assertLinesAre("ABC", "DEF", "GHI");

		withTerminalSized(3, 3).enterString("A\033[$zBC").assertLinesAre(" BC", "   ", "   ");
	}

	/** http://www.vt100.net/docs/vt510-rm/DECSED */
	public void testSelectiveEraseInDisplay() {
		// ${CSI}1"q enables protection, ${CSI}0"q disables it.
		// ${CSI}?${0,1,2}J" erases (0=cursor to end, 1=start to cursor, 2=complete display).
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[?2J").assertLinesAre("   ", "   ", "   ");
		withTerminalSized(3, 3).enterString("ABC\033[1\"qDE\033[0\"qFGHI\033[?2J").assertLinesAre("   ", "DE ", "   ");
		withTerminalSized(3, 3).enterString("\033[1\"qABCDE\033[0\"qFGHI\033[?2J").assertLinesAre("ABC", "DE ", "   ");
	}

	/** http://vt100.net/docs/vt510-rm/DECSEL */
	public void testSelectiveEraseInLine() {
		// ${CSI}1"q enables protection, ${CSI}0"q disables it.
		// ${CSI}?${0,1,2}K" erases (0=cursor to end, 1=start to cursor, 2=complete line).
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[?2K").assertLinesAre("ABC", "DEF", "   ");
		withTerminalSized(3, 3).enterString("ABCDE\033[?0KFGHI").assertLinesAre("ABC", "DEF", "GHI");
		withTerminalSized(3, 3).enterString("ABCDE\033[?1KFGHI").assertLinesAre("ABC", "  F", "GHI");
		withTerminalSized(3, 3).enterString("ABCDE\033[?2KFGHI").assertLinesAre("ABC", "  F", "GHI");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[2;2H\033[?0K").assertLinesAre("ABC", "D  ", "GHI");
		withTerminalSized(3, 3).enterString("ABC\033[1\"qD\033[0\"qE\033[?2KFGHI").assertLinesAre("ABC", "D F", "GHI");
	}

	/** http://www.vt100.net/docs/vt510-rm/DECSERA */
	public void testSelectiveEraseInRectangle() {
		// ${CSI}1"q enables protection, ${CSI}0"q disables it.
		// ${CSI}?${TOP};${LEFT};${BOTTOM};${RIGHT}${" erases.
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[${").assertLinesAre("   ", "   ", "   ");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[1;1;2;10${").assertLinesAre("   ", "   ", "GHI");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[2;1;3;10${").assertLinesAre("ABC", "   ", "   ");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[1;1;100;1${").assertLinesAre(" BC", " EF", " HI");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[1;1;100;2${").assertLinesAre("  C", "  F", "  I");
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[100;1;100;2${").assertLinesAre("ABC", "DEF", "GHI");

		withTerminalSized(3, 3).enterString("ABCD\033[1\"qE\033[0\"qFGHI\033[${").assertLinesAre("   ", " E ", "   ");
		withTerminalSized(3, 3).enterString("ABCD\033[1\"qE\033[0\"qFGHI\033[1;1;2;10${").assertLinesAre("   ", " E ", "GHI");
	}

	/** http://vt100.net/docs/vt510-rm/DECCRA */
	public void testRectangularCopy() {
		// "${CSI}${SRC_TOP};${SRC_LEFT};${SRC_BOTTOM};${SRC_RIGHT};${SRC_PAGE};${DST_TOP};${DST_LEFT};${DST_PAGE}\$v"
		withTerminalSized(7, 3).enterString("ABC\r\nDEF\r\nGHI\033[1;1;2;2;1;2;5;1$v").assertLinesAre("ABC    ", "DEF AB ", "GHI DE ");
		withTerminalSized(7, 3).enterString("ABC\r\nDEF\r\nGHI\033[1;1;3;3;1;1;4;1$v").assertLinesAre("ABCABC ", "DEFDEF ", "GHIGHI ");
		withTerminalSized(7, 3).enterString("ABC\r\nDEF\r\nGHI\033[1;1;3;3;1;1;3;1$v").assertLinesAre("ABABC  ", "DEDEF  ", "GHGHI  ");
		withTerminalSized(7, 3).enterString("   ABC\r\n   DEF\r\n   GHI\033[1;4;3;6;1;1;1;1$v").assertLinesAre("ABCABC ", "DEFDEF ",
				"GHIGHI ");
		withTerminalSized(7, 3).enterString("   ABC\r\n   DEF\r\n   GHI\033[1;4;3;6;1;1;2;1$v").assertLinesAre(" ABCBC ", " DEFEF ",
				" GHIHI ");
		withTerminalSized(3, 3).enterString("ABC\r\nDEF\r\nGHI\033[1;1;2;2;1;2;2;1$v").assertLinesAre("ABC", "DAB", "GDE");

		// Enable ${CSI}?6h origin mode (DECOM) and ${CSI}?69h for left/right margin (DECLRMM) enabling, ${CSI}${LEFTMARGIN};${RIGHTMARGIN}s
		// for DECSLRM margin setting.
		withTerminalSized(5, 5).enterString("\033[?6h\033[?69h\033[2;4s");
		enterString("ABCDEFGHIJK").assertLinesAre(" ABC ", " DEF ", " GHI ", " JK  ", "     ");
		enterString("\033[1;1;2;2;1;2;2;1$v").assertLinesAre(" ABC ", " DAB ", " GDE ", " JK  ", "     ");
	}

	/** http://vt100.net/docs/vt510-rm/DECCARA */
	public void testChangeAttributesInRectangularArea() {
		final int b = TextStyle.CHARACTER_ATTRIBUTE_BOLD;
		// "${CSI}${TOP};${LEFT};${BOTTOM};${RIGHT};${ATTRIBUTES}\$r"
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[1;1;2;2;1$r").assertLinesAre("ABC", "DEF", "GHI");
		assertEffectAttributesSet(effectLine(b, b, b), effectLine(b, b, 0), effectLine(0, 0, 0));

		// Now with http://www.vt100.net/docs/vt510-rm/DECSACE ("${CSI}2*x") specifying rectangle:
		withTerminalSized(3, 3).enterString("\033[2*xABCDEFGHI\033[1;1;2;2;1$r").assertLinesAre("ABC", "DEF", "GHI");
		assertEffectAttributesSet(effectLine(b, b, 0), effectLine(b, b, 0), effectLine(0, 0, 0));
	}

	/** http://vt100.net/docs/vt510-rm/DECCARA */
	public void testReverseAttributesInRectangularArea() {
		final int b = TextStyle.CHARACTER_ATTRIBUTE_BOLD;
		final int u = TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE;
		final int bu = TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE;
		// "${CSI}${TOP};${LEFT};${BOTTOM};${RIGHT};${ATTRIBUTES}\$t"
		withTerminalSized(3, 3).enterString("ABCDEFGHI\033[1;1;2;2;1$t").assertLinesAre("ABC", "DEF", "GHI");
		assertEffectAttributesSet(effectLine(b, b, b), effectLine(b, b, 0), effectLine(0, 0, 0));

		// Now with http://www.vt100.net/docs/vt510-rm/DECSACE ("${CSI}2*x") specifying rectangle:
		withTerminalSized(3, 3).enterString("\033[2*xABCDEFGHI\033[1;1;2;2;1$t").assertLinesAre("ABC", "DEF", "GHI");
		assertEffectAttributesSet(effectLine(b, b, 0), effectLine(b, b, 0), effectLine(0, 0, 0));

		// Check reversal by initially bolding the B:
		withTerminalSized(3, 3).enterString("\033[2*xA\033[1mB\033[0mCDEFGHI\033[1;1;2;2;1$t").assertLinesAre("ABC", "DEF", "GHI");
		assertEffectAttributesSet(effectLine(b, 0, 0), effectLine(b, b, 0), effectLine(0, 0, 0));

		// Check reversal by initially underlining A, bolding B, then reversing both bold and underline:
		withTerminalSized(3, 3).enterString("\033[2*x\033[4mA\033[0m\033[1mB\033[0mCDEFGHI\033[1;1;2;2;1;4$t").assertLinesAre("ABC", "DEF",
				"GHI");
		assertEffectAttributesSet(effectLine(b, u, 0), effectLine(bu, bu, 0), effectLine(0, 0, 0));
	}

}
