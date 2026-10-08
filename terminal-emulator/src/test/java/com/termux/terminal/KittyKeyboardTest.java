package com.termux.terminal;

import android.view.KeyEvent;

/** Negotiation goes through the real terminal parser; key output is checked against protocol bytes. */
public class KittyKeyboardTest extends TerminalTestCase {
    public void testNegotiationAndRestore() {
        withTerminalSized(20, 4);
        assertEnteringStringGivesResponse("\033[?u", "\033[?0u");
        enterString("\033[>1u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?1u");
        enterString("\033[>0u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?0u");
        enterString("\033[<u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?1u");
        enterString("\033[<u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?0u");
        enterString("\033[<9999u");
        assertFalse(mTerminal.isKittyKeyboardEnabled());
    }

    public void testSupportedFlagsAndSetModes() {
        withTerminalSized(20, 4);
        enterString("\033[=31u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?7u");
        enterString("\033[=1;3u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?6u");
        enterString("\033[=1;2u\033[=30;2u");
        assertTrue(mTerminal.isKittyKeyboardEnabled());
        enterString("\033[=0;1u");
        assertFalse(mTerminal.isKittyKeyboardEnabled());
        enterString("\033[=1;4u\033[=1:2u\033[>1;2u");
        assertFalse(mTerminal.isKittyKeyboardEnabled());
    }

    public void testScreenIsolationAndReset() {
        withTerminalSized(20, 4);
        enterString("\033[>1u\033[?1049h");
        assertFalse(mTerminal.isKittyKeyboardEnabled());
        enterString("\033[>1u\033[>0u\033[?1049l");
        assertTrue(mTerminal.isKittyKeyboardEnabled());
        enterString("\033[<u\033[?1049h\033[<u");
        assertTrue(mTerminal.isKittyKeyboardEnabled());
        enterString("\033c");
        assertFalse(mTerminal.isKittyKeyboardEnabled());
        enterString("\033[?1049l");
        assertFalse(mTerminal.isKittyKeyboardEnabled());
    }

    public void testFragmentedSequencesAndStackLimit() {
        withTerminalSized(20, 4);
        for (char c : "\033[>1u\033[?u".toCharArray()) enterString(String.valueOf(c));
        assertEquals("\033[?1u", mOutput.getOutputAndClear());
        for (int i = 0; i < 100; i++) enterString("\033[>1u");
        enterString("\033[<9999u");
        assertFalse(mTerminal.isKittyKeyboardEnabled());
        enterString("\033[=1 $uOK");
        assertLineIs(0, "OK                  ");
    }

    public void testShortcutBytesAndTextFallback() {
        assertEquals("\033[48;5u", KeyHandler.getKittyCodePoint('0', KeyHandler.KEYMOD_CTRL));
        assertEquals("\033[105;5u", KeyHandler.getKittyCodePoint('i', KeyHandler.KEYMOD_CTRL));
        assertEquals("\033[97;6u", KeyHandler.getKittyCodePoint('a', KeyHandler.KEYMOD_CTRL | KeyHandler.KEYMOD_SHIFT));
        assertEquals("\033[61;6u", KeyHandler.getKittyCodePoint('=', KeyHandler.KEYMOD_CTRL | KeyHandler.KEYMOD_SHIFT));
        assertEquals("\033[91;3u", KeyHandler.getKittyCodePoint('[', KeyHandler.KEYMOD_ALT));
        assertNull(KeyHandler.getKittyCodePoint('a', 0));
        assertNull(KeyHandler.getKittyCodePoint('a', KeyHandler.KEYMOD_SHIFT));
        assertNull(KeyHandler.getKittyCodePoint(0x4E2D, 0));
        assertEquals("\033[13;2u", KeyHandler.getKittyCode(KeyEvent.KEYCODE_ENTER, KeyHandler.KEYMOD_SHIFT));
        assertEquals("\033[13;5u", KeyHandler.getKittyCode(KeyEvent.KEYCODE_ENTER, KeyHandler.KEYMOD_CTRL));
        assertEquals("\033[9;6u", KeyHandler.getKittyCode(KeyEvent.KEYCODE_TAB, KeyHandler.KEYMOD_SHIFT | KeyHandler.KEYMOD_CTRL));
        assertEquals("\r", KeyHandler.getKittyCode(KeyEvent.KEYCODE_ENTER, KeyHandler.KEYMOD_NUM_LOCK));
        assertEquals("\t", KeyHandler.getKittyCode(KeyEvent.KEYCODE_TAB, 0));
        assertEquals("\u007f", KeyHandler.getKittyCode(KeyEvent.KEYCODE_DEL, 0));
        assertEquals("\033[27u", KeyHandler.getKittyCode(KeyEvent.KEYCODE_ESCAPE, 0));
        assertEquals("\033[13~", KeyHandler.getKittyCode(KeyEvent.KEYCODE_F3, 0));
        assertEquals("\033[1;2A", KeyHandler.getKittyCode(KeyEvent.KEYCODE_DPAD_UP, KeyHandler.KEYMOD_SHIFT));
        assertEquals("\033[57399;129u", KeyHandler.getKittyCode(KeyEvent.KEYCODE_NUMPAD_0, KeyHandler.KEYMOD_NUM_LOCK));
        assertEquals("\033[57425u", KeyHandler.getKittyCode(KeyEvent.KEYCODE_NUMPAD_0, 0));
        assertEquals("\033[E", KeyHandler.getKittyCode(KeyEvent.KEYCODE_NUMPAD_5, 0));
        assertEquals("\033[97;9u", KeyHandler.getKittyCodePoint('a', KeyHandler.KEYMOD_SUPER));
        assertEquals("\r", KeyHandler.getCode(KeyEvent.KEYCODE_ENTER, KeyHandler.KEYMOD_SHIFT, false, false));
        assertEquals("\033", KeyHandler.getCode(KeyEvent.KEYCODE_ESCAPE, 0, false, false));
    }
    public void testEventFlagsRestoreAndScreenIsolation() {
        withTerminalSized(20, 4);
        enterString("\033[>3u\033[>2u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?2u");
        enterString("\033[<u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?3u");
        enterString("\033[=2;3u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?1u");
        enterString("\033[=2;2u\033[?1049h\033[>2u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?2u");
        enterString("\033[?1049l");
        assertEnteringStringGivesResponse("\033[?u", "\033[?3u");
        enterString("\033c\033[?1049h");
        assertEnteringStringGivesResponse("\033[?u", "\033[?0u");
    }

    public void testEventEncoding() {
        for (int flags : new int[]{2, 3}) {
            assertEquals("\033[1;1:2A", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_UP, 0, true, false, flags, 2));
            assertEquals("\033[1;1:3A", KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_UP, 0, true, false, flags, 3));
            assertEquals("\033[13;5:2~", KeyHandler.getCode(KeyEvent.KEYCODE_F3, KeyHandler.KEYMOD_CTRL, false, false, flags, 2));
            for (int key : new int[]{KeyEvent.KEYCODE_ENTER, KeyEvent.KEYCODE_TAB, KeyEvent.KEYCODE_DEL}) {
                assertNull(KeyHandler.getCode(key, 0, false, false, flags, 3));
                assertNull(KeyHandler.getCode(key, KeyHandler.KEYMOD_CTRL, false, false, flags, 3));
            }
        }
        assertEquals("\033[105;5:2u", KeyHandler.getKittyCodePoint('i', KeyHandler.KEYMOD_CTRL, 3, 2));
        assertEquals("\033[105;1:3u", KeyHandler.getKittyCodePoint('i', 0, 3, 3));
        assertEquals("\033[105;5u", KeyHandler.getKittyCodePoint('i', KeyHandler.KEYMOD_CTRL, 1, 2));
        assertNull(KeyHandler.getKittyCodePoint('i', KeyHandler.KEYMOD_CTRL, 2, 1));
        assertNull(KeyHandler.getKittyCodePoint('a', 0, 3, 2));
        assertEquals("\033[57399;129:3u", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_0,
            KeyHandler.KEYMOD_NUM_LOCK, false, false, 3, 3));
        assertEquals("0", KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_0,
            KeyHandler.KEYMOD_NUM_LOCK, false, false, 2, 2));
        assertNull(KeyHandler.getCode(KeyEvent.KEYCODE_NUMPAD_0,
            KeyHandler.KEYMOD_NUM_LOCK, false, false, 2, 3));
        assertNull(KeyHandler.getCode(KeyEvent.KEYCODE_DPAD_UP, 0, false, false, 1, 3));
    }

    public void testAlternateFlagCombinationsAndRestoration() {
        withTerminalSized(20, 4);
        for (int flags = 0; flags <= 31; flags++) {
            enterString("\033[=" + flags + "u");
            assertEnteringStringGivesResponse("\033[?u", "\033[?" + (flags & 7) + "u");
            assertEquals((flags & 3) != 0, mTerminal.isKittyKeyboardEnabled());
        }
        enterString("\033[=4u\033[>7u\033[<u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?4u");
        enterString("\033[=1;2u\033[=2;2u\033[=4;3u");
        assertEnteringStringGivesResponse("\033[?u", "\033[?3u");
        enterString("\033[>5u\033[?1049h");
        assertEnteringStringGivesResponse("\033[?u", "\033[?0u");
        enterString("\033[>6u\033[?1049l");
        assertEnteringStringGivesResponse("\033[?u", "\033[?5u");
        enterString("\033c\033[?1049h");
        assertEnteringStringGivesResponse("\033[?u", "\033[?0u");
    }

    public void testAlternateCodePointFields() {
        KeyHandler.KittyKey equals = new KeyHandler.KittyKey('=', '+', '=');
        int modifiers = KeyHandler.KEYMOD_CTRL | KeyHandler.KEYMOD_SHIFT;
        assertEquals("\033[61:43;6u", KeyHandler.getKittyCodePoint(equals, modifiers, 5, 1));
        assertEquals("\033[61:43;6:2u", KeyHandler.getKittyCodePoint(equals, modifiers, 7, 2));
        assertEquals("\033[61;5:3u", KeyHandler.getKittyCodePoint(equals, KeyHandler.KEYMOD_CTRL, 7, 3));
        assertEquals("\033[61;6u", KeyHandler.getKittyCodePoint(equals, modifiers, 3, 1));
        assertNull(KeyHandler.getKittyCodePoint(equals, modifiers, 4, 1));
        assertNull(KeyHandler.getKittyCodePoint(equals, modifiers, 6, 1));
        assertEquals("\033[61:43;6:2u", KeyHandler.getKittyCodePoint(equals, modifiers, 6, 2));
        assertNull(KeyHandler.getKittyCodePoint(equals, modifiers, 5, 3));
        assertNull(KeyHandler.getKittyCodePoint(equals, KeyHandler.KEYMOD_SHIFT, 7, 1));
        KeyHandler.KittyKey cyrillic = new KeyHandler.KittyKey(1089, 1057, 'c');
        assertEquals("\033[1089:1057:99;6u", KeyHandler.getKittyCodePoint(cyrillic, modifiers, 5, 1));
        assertEquals("\033[1089::99;5u", KeyHandler.getKittyCodePoint(cyrillic, KeyHandler.KEYMOD_CTRL, 5, 1));
        assertEquals("\033[1089::99;1:3u", KeyHandler.getKittyCodePoint(cyrillic, 0, 7, 3));
        KeyHandler.KittyKey unknown = new KeyHandler.KittyKey('a', 0xD800, 0x80000000);
        assertEquals("\033[97;6u", KeyHandler.getKittyCodePoint(unknown, modifiers, 5, 1));
        KeyHandler.KittyKey supplementary = new KeyHandler.KittyKey(0x1F600, 0x1F601, 'a');
        assertEquals("\033[128512:128513:97;6u", KeyHandler.getKittyCodePoint(supplementary, modifiers, 5, 1));
    }

    public void testBaseLayoutMappingAndFunctionalKeyBoundaries() {
        for (int key = KeyEvent.KEYCODE_A; key <= KeyEvent.KEYCODE_Z; key++)
            assertEquals('a' + key - KeyEvent.KEYCODE_A, KeyHandler.getKittyBaseLayoutCodePoint(key));
        for (int key = KeyEvent.KEYCODE_0; key <= KeyEvent.KEYCODE_9; key++)
            assertEquals('0' + key - KeyEvent.KEYCODE_0, KeyHandler.getKittyBaseLayoutCodePoint(key));
        assertEquals('=', KeyHandler.getKittyBaseLayoutCodePoint(KeyEvent.KEYCODE_EQUALS));
        assertEquals('\\', KeyHandler.getKittyBaseLayoutCodePoint(KeyEvent.KEYCODE_BACKSLASH));
        assertEquals(0, KeyHandler.getKittyBaseLayoutCodePoint(KeyEvent.KEYCODE_UNKNOWN));
        assertEquals(0, KeyHandler.getKittyBaseLayoutCodePoint(KeyEvent.KEYCODE_PLUS));
        assertEquals(0, KeyHandler.getKittyBaseLayoutCodePoint(KeyEvent.KEYCODE_NUMPAD_0));
        assertEquals("\033[13;2:2~", KeyHandler.getCode(KeyEvent.KEYCODE_F3, KeyHandler.KEYMOD_SHIFT, false, false, 7, 2));
        assertEquals("\r", KeyHandler.getCode(KeyEvent.KEYCODE_ENTER, 0, false, false, 7, 2));
    }

}
