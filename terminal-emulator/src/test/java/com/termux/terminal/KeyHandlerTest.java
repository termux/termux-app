package com.termux.terminal;

import android.view.KeyEvent;

import junit.framework.TestCase;

public class KeyHandlerTest extends TestCase {

	private static String stringToHex(String s) {
		if (s == null) return null;
		StringBuilder buffer = new StringBuilder();
		for (int i = 0; i < s.length(); i++) {
			if (buffer.length() > 0) {
				buffer.append(" ");
			}
			buffer.append("0x");
			buffer.append(Integer.toHexString(s.charAt(i)));
		}
		return buffer.toString();
	}

	private static void assertKeysEquals(String expected, String actual) {
		if (!expected.equals(actual)) {
			assertEquals(stringToHex(expected), stringToHex(actual));
		}
	}

	/** See http://pubs.opengroup.org/onlinepubs/7990989799/xcurses/terminfo.html */
	public void testTermCaps() {
		// Backspace.
		assertKeysEquals("\u007f", KeyHandler.getCodeFromTermcap("kb", false, false));

		// Back tab.
		assertKeysEquals("\033[Z", KeyHandler.getCodeFromTermcap("kB", false, false));

		// Arrow keys (up/down/right/left):
		assertKeysEquals("\033[A", KeyHandler.getCodeFromTermcap("ku", false, false));
		assertKeysEquals("\033[B", KeyHandler.getCodeFromTermcap("kd", false, false));
		assertKeysEquals("\033[C", KeyHandler.getCodeFromTermcap("kr", false, false));
		assertKeysEquals("\033[D", KeyHandler.getCodeFromTermcap("kl", false, false));
		// .. shifted:
		assertKeysEquals("\033[1;2A", KeyHandler.getCodeFromTermcap("kUP", false, false));
		assertKeysEquals("\033[1;2B", KeyHandler.getCodeFromTermcap("kDN", false, false));
		assertKeysEquals("\033[1;2C", KeyHandler.getCodeFromTermcap("%i", false, false));
		assertKeysEquals("\033[1;2D", KeyHandler.getCodeFromTermcap("#4", false, false));

		// Home/end keys:
		assertKeysEquals("\033[H", KeyHandler.getCodeFromTermcap("kh", false, false));
		assertKeysEquals("\033[F", KeyHandler.getCodeFromTermcap("@7", false, false));
		// ... shifted:
		assertKeysEquals("\033[1;2H", KeyHandler.getCodeFromTermcap("#2", false, false));
		assertKeysEquals("\033[1;2F", KeyHandler.getCodeFromTermcap("*7", false, false));

		// The traditional keyboard keypad:
		// [Insert] [Home] [Page Up ]
		// [Delete] [End] [Page Down]
		//
		// Termcap names (with xterm response in parenthesis):
		// K1=Upper left of keypad (xterm sends same "<ESC>[H" = Home).
		// K2=Center of keypad (xterm sends invalid response).
		// K3=Upper right of keypad (xterm sends "<ESC>[5~" = Page Up).
		// K4=Lower left of keypad (xterm sends "<ESC>[F" = End key).
		// K5=Lower right of keypad (xterm sends "<ESC>[6~" = Page Down).
		//
		// vim/neovim (runtime/doc/term.txt):
		// t_K1 <kHome> keypad home key
		// t_K3 <kPageUp> keypad page-up key
		// t_K4 <kEnd> keypad end key
		// t_K5 <kPageDown> keypad page-down key
		//
		assertKeysEquals("\033[H", KeyHandler.getCodeFromTermcap("K1", false, false));
		assertKeysEquals("\033OH", KeyHandler.getCodeFromTermcap("K1", true, false));
		assertKeysEquals("\033[5~", KeyHandler.getCodeFromTermcap("K3", false, false));
		assertKeysEquals("\033[F", KeyHandler.getCodeFromTermcap("K4", false, false));
		assertKeysEquals("\033OF", KeyHandler.getCodeFromTermcap("K4", true, false));
		assertKeysEquals("\033[6~", KeyHandler.getCodeFromTermcap("K5", false, false));

		// Function keys F1-F12:
		assertKeysEquals("\033OP", KeyHandler.getCodeFromTermcap("k1", false, false));
		assertKeysEquals("\033OQ", KeyHandler.getCodeFromTermcap("k2", false, false));
		assertKeysEquals("\033OR", KeyHandler.getCodeFromTermcap("k3", false, false));
		assertKeysEquals("\033OS", KeyHandler.getCodeFromTermcap("k4", false, false));
		assertKeysEquals("\033[15~", KeyHandler.getCodeFromTermcap("k5", false, false));
		assertKeysEquals("\033[17~", KeyHandler.getCodeFromTermcap("k6", false, false));
		assertKeysEquals("\033[18~", KeyHandler.getCodeFromTermcap("k7", false, false));
		assertKeysEquals("\033[19~", KeyHandler.getCodeFromTermcap("k8", false, false));
		assertKeysEquals("\033[20~", KeyHandler.getCodeFromTermcap("k9", false, false));
		assertKeysEquals("\033[21~", KeyHandler.getCodeFromTermcap("k;", false, false));
		assertKeysEquals("\033[23~", KeyHandler.getCodeFromTermcap("F1", false, false));
		assertKeysEquals("\033[24~", KeyHandler.getCodeFromTermcap("F2", false, false));
		// Function keys F13-F24 (same as shifted F1-F12):
		assertKeysEquals("\033[1;2P", KeyHandler.getCodeFromTermcap("F3", false, false));
		assertKeysEquals("\033[1;2Q", KeyHandler.getCodeFromTermcap("F4", false, false));
		assertKeysEquals("\033[1;2R", KeyHandler.getCodeFromTermcap("F5", false, false));
		assertKeysEquals("\033[1;2S", KeyHandler.getCodeFromTermcap("F6", false, false));
		assertKeysEquals("\033[15;2~", KeyHandler.getCodeFromTermcap("F7", false, false));
		assertKeysEquals("\033[17;2~", KeyHandler.getCodeFromTermcap("F8", false, false));
		assertKeysEquals("\033[18;2~", KeyHandler.getCodeFromTermcap("F9", false, false));
		assertKeysEquals("\033[19;2~", KeyHandler.getCodeFromTermcap("FA", false, false));
		assertKeysEquals("\033[20;2~", KeyHandler.getCodeFromTermcap("FB", false, false));
		assertKeysEquals("\033[21;2~", KeyHandler.getCodeFromTermcap("FC", false, false));
		assertKeysEquals("\033[23;2~", KeyHandler.getCodeFromTermcap("FD", false, false));
		assertKeysEquals("\033[24;2~", KeyHandler.getCodeFromTermcap("FE", false, false));
	}

	public void testKeyCodes() {
		// Return sends carriage return (\r), which normally gets translated by the device driver to newline (\n) unless the ICRNL termios
		// flag has been set.
		assertKeysEquals("\r", KeyHandler.getCode(KeyEvent.KEYCODE_ENTER, 0, false, false));

		// Backspace.
		assertKeysEquals("\u007f", KeyHandler.getCode(KeyEvent.KEYCODE_DEL, 0, false, false));

		// Space.
		assertNull(KeyHandler.getCode(KeyEvent.KEYCODE_SPACE, 0, false, false));
		assertKeysEquals("\u0000", KeyHandler.getCode(KeyEvent.KEYCODE_SPACE, KeyHandler.KEYMOD_CTRL, false, false));

		// Back tab.
		assertKeysEquals("\033[Z", KeyHandler.getCode(KeyEvent.KEYCODE_TAB, KeyHandler.KEYMOD_SHIFT, false, false));

		// Arrow keys (up/down/right/left):
		assertKeysEquals("\033[A", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_UP, 0, false, false));
		assertKeysEquals("\033[B", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_DOWN, 0, false, false));
		assertKeysEquals("\033[C", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_RIGHT, 0, false, false));
		assertKeysEquals("\033[D", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_LEFT, 0, false, false));
		// .. shifted:
		assertKeysEquals("\033[1;2A", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_UP, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[1;2B", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_DOWN, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[1;2C", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_RIGHT, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[1;2D", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_LEFT, KeyHandler.KEYMOD_SHIFT, false, false));
		// .. ctrl:ed:
		assertKeysEquals("\033[1;5A", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_UP, KeyHandler.KEYMOD_CTRL, false, false));
		assertKeysEquals("\033[1;5B", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_DOWN, KeyHandler.KEYMOD_CTRL, false, false));
		assertKeysEquals("\033[1;5C", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_RIGHT, KeyHandler.KEYMOD_CTRL, false, false));
		assertKeysEquals("\033[1;5D", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_LEFT, KeyHandler.KEYMOD_CTRL, false, false));
		// .. ctrl:ed and shifted:
		int mod = KeyHandler.KEYMOD_CTRL | KeyHandler.KEYMOD_SHIFT;
		assertKeysEquals("\033[1;6A", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_UP, mod, false, false));
		assertKeysEquals("\033[1;6B", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_DOWN, mod, false, false));
		assertKeysEquals("\033[1;6C", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_RIGHT, mod, false, false));
		assertKeysEquals("\033[1;6D", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_LEFT, mod, false, false));

		// Home/end keys:
		assertKeysEquals("\033[H", KeyHandler.getCode(KeyEvent.KEYCODE_MOVE_HOME, 0, false, false));
		assertKeysEquals("\033[F", KeyHandler.getCode(KeyEvent.KEYCODE_MOVE_END, 0, false, false));
		// ... shifted:
		assertKeysEquals("\033[1;2H", KeyHandler.getCode(KeyEvent.KEYCODE_MOVE_HOME, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[1;2F", KeyHandler.getCode(KeyEvent.KEYCODE_MOVE_END, KeyHandler.KEYMOD_SHIFT, false, false));

		// Function keys F1-F12:
		assertKeysEquals("\033OP", KeyHandler.getCode(KeyEvent.KEYCODE_F1, 0, false, false));
		assertKeysEquals("\033OQ", KeyHandler.getCode(KeyEvent.KEYCODE_F2, 0, false, false));
		assertKeysEquals("\033OR", KeyHandler.getCode(KeyEvent.KEYCODE_F3, 0, false, false));
		assertKeysEquals("\033OS", KeyHandler.getCode(KeyEvent.KEYCODE_F4, 0, false, false));
		assertKeysEquals("\033[15~", KeyHandler.getCode(KeyEvent.KEYCODE_F5, 0, false, false));
		assertKeysEquals("\033[17~", KeyHandler.getCode(KeyEvent.KEYCODE_F6, 0, false, false));
		assertKeysEquals("\033[18~", KeyHandler.getCode(KeyEvent.KEYCODE_F7, 0, false, false));
		assertKeysEquals("\033[19~", KeyHandler.getCode(KeyEvent.KEYCODE_F8, 0, false, false));
		assertKeysEquals("\033[20~", KeyHandler.getCode(KeyEvent.KEYCODE_F9, 0, false, false));
		assertKeysEquals("\033[21~", KeyHandler.getCode(KeyEvent.KEYCODE_F10, 0, false, false));
		assertKeysEquals("\033[23~", KeyHandler.getCode(KeyEvent.KEYCODE_F11, 0, false, false));
		assertKeysEquals("\033[24~", KeyHandler.getCode(KeyEvent.KEYCODE_F12, 0, false, false));
		// Function keys F13-F24 (same as shifted F1-F12):
		assertKeysEquals("\033[1;2P", KeyHandler.getCode(KeyEvent.KEYCODE_F1, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[1;2Q", KeyHandler.getCode(KeyEvent.KEYCODE_F2, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[1;2R", KeyHandler.getCode(KeyEvent.KEYCODE_F3, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[1;2S", KeyHandler.getCode(KeyEvent.KEYCODE_F4, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[15;2~", KeyHandler.getCode(KeyEvent.KEYCODE_F5, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[17;2~", KeyHandler.getCode(KeyEvent.KEYCODE_F6, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[18;2~", KeyHandler.getCode(KeyEvent.KEYCODE_F7, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[19;2~", KeyHandler.getCode(KeyEvent.KEYCODE_F8, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[20;2~", KeyHandler.getCode(KeyEvent.KEYCODE_F9, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[21;2~", KeyHandler.getCode(KeyEvent.KEYCODE_F10, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[23;2~", KeyHandler.getCode(KeyEvent.KEYCODE_F11, KeyHandler.KEYMOD_SHIFT, false, false));
		assertKeysEquals("\033[24;2~", KeyHandler.getCode(KeyEvent.KEYCODE_F12, KeyHandler.KEYMOD_SHIFT, false, false));

		assertKeysEquals("0", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_0, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals("1", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_1, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals("2", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_2, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals("3", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_3, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals("4", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_4, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals("5", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_5, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals("6", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_6, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals("7", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_7, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals("8", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_8, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals("9", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_9, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals(",", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_COMMA, KeyHandler.KEYMOD_NUM_LOCK, false, false));
		assertKeysEquals(".", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_DOT, KeyHandler.KEYMOD_NUM_LOCK, false, false));

        assertKeysEquals("\033[2~", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_0, 0, false, false));
        assertKeysEquals("\033[F", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_1, 0, false, false));
        assertKeysEquals("\033[B", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_2, 0, false, false));
        assertKeysEquals("\033[6~", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_3, 0, false, false));
        assertKeysEquals("\033[D", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_4, 0, false, false));
        assertKeysEquals("5", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_5, 0, false, false));
        assertKeysEquals("\033[C", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_6, 0, false, false));
        assertKeysEquals("\033[H", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_7, 0, false, false));
        assertKeysEquals("\033[A", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_8, 0, false, false));
        assertKeysEquals("\033[5~", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_9, 0, false, false));
        assertKeysEquals("\033[3~", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_DOT, 0, false, false));
	}

}
