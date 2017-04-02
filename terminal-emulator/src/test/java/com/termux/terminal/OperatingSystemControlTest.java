package com.termux.terminal;

import android.util.Base64;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/** "ESC ]" is the Operating System Command. */
public class OperatingSystemControlTest extends TerminalTestCase {

	public void testSetTitle() throws Exception {
		List<ChangedTitle> expectedTitleChanges = new ArrayList<>();

		withTerminalSized(10, 10);
		enterString("\033]0;Hello, world\007");
		assertEquals("Hello, world", mTerminal.getTitle());
		expectedTitleChanges.add(new ChangedTitle(null, "Hello, world"));
		assertEquals(expectedTitleChanges, mOutput.titleChanges);

		enterString("\033]0;Goodbye, world\007");
		assertEquals("Goodbye, world", mTerminal.getTitle());
		expectedTitleChanges.add(new ChangedTitle("Hello, world", "Goodbye, world"));
		assertEquals(expectedTitleChanges, mOutput.titleChanges);

		enterString("\033]0;Goodbye, \u00F1 world\007");
		assertEquals("Goodbye, \uu00F1 world", mTerminal.getTitle());
		expectedTitleChanges.add(new ChangedTitle("Goodbye, world", "Goodbye, \uu00F1 world"));
		assertEquals(expectedTitleChanges, mOutput.titleChanges);

		// 2 should work as well (0 sets both title and icon).
		enterString("\033]2;Updated\007");
		assertEquals("Updated", mTerminal.getTitle());
		expectedTitleChanges.add(new ChangedTitle("Goodbye, \uu00F1 world", "Updated"));
		assertEquals(expectedTitleChanges, mOutput.titleChanges);

		enterString("\033[22;0t");
		enterString("\033]0;FIRST\007");
		expectedTitleChanges.add(new ChangedTitle("Updated", "FIRST"));
		assertEquals("FIRST", mTerminal.getTitle());
		assertEquals(expectedTitleChanges, mOutput.titleChanges);

		enterString("\033[22;0t");
		enterString("\033]0;SECOND\007");
		assertEquals("SECOND", mTerminal.getTitle());

		expectedTitleChanges.add(new ChangedTitle("FIRST", "SECOND"));
		assertEquals(expectedTitleChanges, mOutput.titleChanges);

		enterString("\033[23;0t");
		assertEquals("FIRST", mTerminal.getTitle());

		expectedTitleChanges.add(new ChangedTitle("SECOND", "FIRST"));
		assertEquals(expectedTitleChanges, mOutput.titleChanges);

		enterString("\033[23;0t");
		expectedTitleChanges.add(new ChangedTitle("FIRST", "Updated"));
		assertEquals(expectedTitleChanges, mOutput.titleChanges);

		enterString("\033[22;0t");
		enterString("\033[22;0t");
		enterString("\033[22;0t");
		// Popping to same title should not cause changes.
		enterString("\033[23;0t");
		enterString("\033[23;0t");
		enterString("\033[23;0t");
		assertEquals(expectedTitleChanges, mOutput.titleChanges);
	}

	public void testTitleStack() throws Exception {
		// echo -ne '\e]0;BEFORE\007' # set title
		// echo -ne '\e[22t' # push to stack
		// echo -ne '\e]0;AFTER\007' # set new title
		// echo -ne '\e[23t' # retrieve from stack

		withTerminalSized(10, 10);
		enterString("\033]0;InitialTitle\007");
		assertEquals("InitialTitle", mTerminal.getTitle());
		enterString("\033[22t");
		assertEquals("InitialTitle", mTerminal.getTitle());
		enterString("\033]0;UpdatedTitle\007");
		assertEquals("UpdatedTitle", mTerminal.getTitle());
		enterString("\033[23t");
		assertEquals("InitialTitle", mTerminal.getTitle());
		enterString("\033[23t\033[23t\033[23t");
		assertEquals("InitialTitle", mTerminal.getTitle());
	}

	public void testSetColor() throws Exception {
		// "OSC 4; $INDEX; $COLORSPEC BEL" => Change color $INDEX to the color specified by $COLORSPEC.
		withTerminalSized(4, 4).enterString("\033]4;5;#00FF00\007");
		assertEquals(Integer.toHexString(0xFF00FF00), Integer.toHexString(mTerminal.mColors.mCurrentColors[5]));
		enterString("\033]4;5;#00FFAB\007");
		assertEquals(mTerminal.mColors.mCurrentColors[5], 0xFF00FFAB);
		enterString("\033]4;255;#ABFFAB\007");
		assertEquals(mTerminal.mColors.mCurrentColors[255], 0xFFABFFAB);
		// Two indexed colors at once:
		enterString("\033]4;7;#00FF00;8;#0000FF\007");
		assertEquals(mTerminal.mColors.mCurrentColors[7], 0xFF00FF00);
		assertEquals(mTerminal.mColors.mCurrentColors[8], 0xFF0000FF);
	}

	void assertIndexColorsMatch(int[] expected) {
		for (int i = 0; i < 255; i++)
			assertEquals("index=" + i, expected[i], mTerminal.mColors.mCurrentColors[i]);
	}

	public void testResetColor() throws Exception {
		withTerminalSized(4, 4);
		int[] initialColors = new int[TextStyle.NUM_INDEXED_COLORS];
		System.arraycopy(mTerminal.mColors.mCurrentColors, 0, initialColors, 0, initialColors.length);
		int[] expectedColors = new int[initialColors.length];
		System.arraycopy(mTerminal.mColors.mCurrentColors, 0, expectedColors, 0, expectedColors.length);
		Random rand = new Random();
		for (int endType = 0; endType < 3; endType++) {
			// Both BEL (7) and ST (ESC \) can end an OSC sequence.
			String ender = (endType == 0) ? "\007" : "\033\\";
			for (int i = 0; i < 255; i++) {
				expectedColors[i] = 0xFF000000 + (rand.nextInt() & 0xFFFFFF);
				int r = (expectedColors[i] >> 16) & 0xFF;
				int g = (expectedColors[i] >> 8) & 0xFF;
				int b = expectedColors[i] & 0xFF;
				String rgbHex = String.format("%02x", r) + String.format("%02x", g) + String.format("%02x", b);
				enterString("\033]4;" + i + ";#" + rgbHex + ender);
				assertEquals(expectedColors[i], mTerminal.mColors.mCurrentColors[i]);
			}
		}

		enterString("\033]104;0\007");
		expectedColors[0] = TerminalColors.COLOR_SCHEME.mDefaultColors[0];
		assertIndexColorsMatch(expectedColors);
		enterString("\033]104;1;2\007");
		expectedColors[1] = TerminalColors.COLOR_SCHEME.mDefaultColors[1];
		expectedColors[2] = TerminalColors.COLOR_SCHEME.mDefaultColors[2];
		assertIndexColorsMatch(expectedColors);
		enterString("\033]104\007"); // Reset all colors.
		assertIndexColorsMatch(TerminalColors.COLOR_SCHEME.mDefaultColors);
	}

	public void disabledTestSetClipboard() {
		// Cannot run this as a unit test since Base64 is a android.util class.
		enterString("\033]52;c;" + Base64.encodeToString("Hello, world".getBytes(), 0) + "\007");
	}

	public void testResettingTerminalResetsColor() throws Exception {
		// "OSC 4; $INDEX; $COLORSPEC BEL" => Change color $INDEX to the color specified by $COLORSPEC.
		withTerminalSized(4, 4).enterString("\033]4;5;#00FF00\007");
		enterString("\033]4;5;#00FFAB\007").assertColor(5, 0xFF00FFAB);
		enterString("\033]4;255;#ABFFAB\007").assertColor(255, 0xFFABFFAB);
		mTerminal.reset();
		assertIndexColorsMatch(TerminalColors.COLOR_SCHEME.mDefaultColors);
	}

	public void testSettingDynamicColors() {
		// "${OSC}${DYNAMIC};${COLORSPEC}${BEL_OR_STRINGTERMINATOR}" => Change ${DYNAMIC} color to the color specified by $COLORSPEC where:
		// DYNAMIC=10: Text foreground color.
		// DYNAMIC=11: Text background color.
		// DYNAMIC=12: Text cursor color.
		withTerminalSized(3, 3).enterString("\033]10;#ABCD00\007").assertColor(TextStyle.COLOR_INDEX_FOREGROUND, 0xFFABCD00);
		enterString("\033]11;#0ABCD0\007").assertColor(TextStyle.COLOR_INDEX_BACKGROUND, 0xFF0ABCD0);
		enterString("\033]12;#00ABCD\007").assertColor(TextStyle.COLOR_INDEX_CURSOR, 0xFF00ABCD);
		// Two special colors at once
		// ("Each successive parameter changes the next color in the list. The value of P s tells the starting point in the list"):
		enterString("\033]10;#FF0000;#00FF00\007").assertColor(TextStyle.COLOR_INDEX_FOREGROUND, 0xFFFF0000);
		assertColor(TextStyle.COLOR_INDEX_BACKGROUND, 0xFF00FF00);
		// Three at once:
		enterString("\033]10;#0000FF;#00FF00;#FF0000\007").assertColor(TextStyle.COLOR_INDEX_FOREGROUND, 0xFF0000FF);
		assertColor(TextStyle.COLOR_INDEX_BACKGROUND, 0xFF00FF00).assertColor(TextStyle.COLOR_INDEX_CURSOR, 0xFFFF0000);

		// Without ending semicolon:
		enterString("\033]10;#FF0000\007").assertColor(TextStyle.COLOR_INDEX_FOREGROUND, 0xFFFF0000);
		// For background and cursor:
		enterString("\033]11;#FFFF00;\007").assertColor(TextStyle.COLOR_INDEX_BACKGROUND, 0xFFFFFF00);
		enterString("\033]12;#00FFFF;\007").assertColor(TextStyle.COLOR_INDEX_CURSOR, 0xFF00FFFF);

		// Using string terminator:
		String stringTerminator = "\033\\";
		enterString("\033]10;#FF0000" + stringTerminator).assertColor(TextStyle.COLOR_INDEX_FOREGROUND, 0xFFFF0000);
		// For background and cursor:
		enterString("\033]11;#FFFF00;" + stringTerminator).assertColor(TextStyle.COLOR_INDEX_BACKGROUND, 0xFFFFFF00);
		enterString("\033]12;#00FFFF;" + stringTerminator).assertColor(TextStyle.COLOR_INDEX_CURSOR, 0xFF00FFFF);
	}

	public void testReportSpecialColors() {
		// "${OSC}${DYNAMIC};?${BEL}" => Terminal responds with the control sequence which would set the current color.
		// Both xterm and libvte (gnome-terminal and others) use the longest color representation, which means that
		// the response is "${OSC}rgb:RRRR/GGGG/BBBB"
		withTerminalSized(3, 3).enterString("\033]10;#ABCD00\007").assertColor(TextStyle.COLOR_INDEX_FOREGROUND, 0xFFABCD00);
		assertEnteringStringGivesResponse("\033]10;?\007", "\033]10;rgb:abab/cdcd/0000\007");
		// Same as above but with string terminator. xterm uses the same string terminator in the response, which
		// e.g. script posted at http://superuser.com/questions/157563/programmatic-access-to-current-xterm-background-color
		// relies on:
		assertEnteringStringGivesResponse("\033]10;?\033\\", "\033]10;rgb:abab/cdcd/0000\033\\");
	}

}
