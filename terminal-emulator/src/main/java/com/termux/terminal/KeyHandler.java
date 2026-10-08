package com.termux.terminal;

import android.view.KeyEvent;
import java.util.HashMap;
import java.util.Map;

import static android.view.KeyEvent.KEYCODE_BACK;
import static android.view.KeyEvent.KEYCODE_BREAK;
import static android.view.KeyEvent.KEYCODE_DEL;
import static android.view.KeyEvent.KEYCODE_DPAD_CENTER;
import static android.view.KeyEvent.KEYCODE_DPAD_DOWN;
import static android.view.KeyEvent.KEYCODE_DPAD_LEFT;
import static android.view.KeyEvent.KEYCODE_DPAD_RIGHT;
import static android.view.KeyEvent.KEYCODE_DPAD_UP;
import static android.view.KeyEvent.KEYCODE_ENTER;
import static android.view.KeyEvent.KEYCODE_ESCAPE;
import static android.view.KeyEvent.KEYCODE_F1;
import static android.view.KeyEvent.KEYCODE_F10;
import static android.view.KeyEvent.KEYCODE_F11;
import static android.view.KeyEvent.KEYCODE_F12;
import static android.view.KeyEvent.KEYCODE_F2;
import static android.view.KeyEvent.KEYCODE_F3;
import static android.view.KeyEvent.KEYCODE_F4;
import static android.view.KeyEvent.KEYCODE_F5;
import static android.view.KeyEvent.KEYCODE_F6;
import static android.view.KeyEvent.KEYCODE_F7;
import static android.view.KeyEvent.KEYCODE_F8;
import static android.view.KeyEvent.KEYCODE_F9;
import static android.view.KeyEvent.KEYCODE_FORWARD_DEL;
import static android.view.KeyEvent.KEYCODE_INSERT;
import static android.view.KeyEvent.KEYCODE_MOVE_END;
import static android.view.KeyEvent.KEYCODE_MOVE_HOME;
import static android.view.KeyEvent.KEYCODE_NUMPAD_0;
import static android.view.KeyEvent.KEYCODE_NUMPAD_1;
import static android.view.KeyEvent.KEYCODE_NUMPAD_2;
import static android.view.KeyEvent.KEYCODE_NUMPAD_3;
import static android.view.KeyEvent.KEYCODE_NUMPAD_4;
import static android.view.KeyEvent.KEYCODE_NUMPAD_5;
import static android.view.KeyEvent.KEYCODE_NUMPAD_6;
import static android.view.KeyEvent.KEYCODE_NUMPAD_7;
import static android.view.KeyEvent.KEYCODE_NUMPAD_8;
import static android.view.KeyEvent.KEYCODE_NUMPAD_9;
import static android.view.KeyEvent.KEYCODE_NUMPAD_ADD;
import static android.view.KeyEvent.KEYCODE_NUMPAD_COMMA;
import static android.view.KeyEvent.KEYCODE_NUMPAD_DIVIDE;
import static android.view.KeyEvent.KEYCODE_NUMPAD_DOT;
import static android.view.KeyEvent.KEYCODE_NUMPAD_ENTER;
import static android.view.KeyEvent.KEYCODE_NUMPAD_EQUALS;
import static android.view.KeyEvent.KEYCODE_NUMPAD_MULTIPLY;
import static android.view.KeyEvent.KEYCODE_NUMPAD_SUBTRACT;
import static android.view.KeyEvent.KEYCODE_NUM_LOCK;
import static android.view.KeyEvent.KEYCODE_PAGE_DOWN;
import static android.view.KeyEvent.KEYCODE_PAGE_UP;
import static android.view.KeyEvent.KEYCODE_SPACE;
import static android.view.KeyEvent.KEYCODE_SYSRQ;
import static android.view.KeyEvent.KEYCODE_TAB;

public final class KeyHandler {

    public static final int KITTY_DISAMBIGUATE = 1;
    public static final int KITTY_REPORT_EVENTS = 2;
    public static final int KITTY_REPORT_ALTERNATE_KEYS = 4;
    public static final int KEY_EVENT_PRESS = 1;
    public static final int KEY_EVENT_REPEAT = 2;
    public static final int KEY_EVENT_RELEASE = 3;

    public static final int KEYMOD_ALT = 0x80000000;
    public static final int KEYMOD_CTRL = 0x40000000;
    public static final int KEYMOD_SHIFT = 0x20000000;
    public static final int KEYMOD_NUM_LOCK = 0x10000000;
    public static final int KEYMOD_SUPER = 0x08000000;
    public static final int KEYMOD_CAPS_LOCK = 0x04000000;
    private static final int[] KEYPAD_NAVIGATION_KEYS = {
        KEYCODE_INSERT, KEYCODE_MOVE_END, KEYCODE_DPAD_DOWN, KEYCODE_PAGE_DOWN, KEYCODE_DPAD_LEFT,
        KEYCODE_NUMPAD_5, KEYCODE_DPAD_RIGHT, KEYCODE_MOVE_HOME, KEYCODE_DPAD_UP, KEYCODE_PAGE_UP
    };
    private static final int[] KITTY_KEYPAD_NAVIGATION = {
        57425, 57424, 57420, 57422, 57417, 57427, 57418, 57423, 57419, 57421
    };

    /** Encode a non-text key in negotiated kitty disambiguation mode, or return null for text. */
    public static String getKittyCode(int keyCode, int keyMode) {
        return getKittyCode(keyCode, keyMode, KEY_EVENT_PRESS);
    }

    private static String getKittyCode(int keyCode, int keyMode, int eventType) {
        if (eventType == KEY_EVENT_RELEASE && isKittyRecoveryKey(keyCode)) return null;
        int codePoint;
        char suffix = 'u';
        boolean numLock = (keyMode & KEYMOD_NUM_LOCK) != 0;
        if (keyCode >= KEYCODE_NUMPAD_0 && keyCode <= KEYCODE_NUMPAD_9) {
            if (numLock) {
                codePoint = 57399 + keyCode - KEYCODE_NUMPAD_0;
            } else {
                // Insert, End, Down, Page Down, Left, Begin, Right, Home, Up, Page Up.
                codePoint = KITTY_KEYPAD_NAVIGATION[keyCode - KEYCODE_NUMPAD_0];
            }
            return codePoint == 57427 ? kittySequence(1, keyMode, 'E', eventType) : kittySequence(codePoint, keyMode, 'u', eventType);
        }
        switch (keyCode) {
            case KEYCODE_ESCAPE:
            case KEYCODE_BACK: codePoint = 27; break;
            case KEYCODE_ENTER:
            case KEYCODE_DPAD_CENTER: codePoint = 13; break;
            case KEYCODE_TAB: codePoint = 9; break;
            case KEYCODE_DEL: codePoint = 127; break;
            // Printable keys, including Space, must first be resolved through the Android layout.
            // AltGr+Space may produce a different character instead of an Alt shortcut.
            case KEYCODE_SPACE: return null;
            case KEYCODE_DPAD_UP: suffix = 'A'; codePoint = 1; break;
            case KEYCODE_DPAD_DOWN: suffix = 'B'; codePoint = 1; break;
            case KEYCODE_DPAD_RIGHT: suffix = 'C'; codePoint = 1; break;
            case KEYCODE_DPAD_LEFT: suffix = 'D'; codePoint = 1; break;
            case KEYCODE_MOVE_HOME: suffix = 'H'; codePoint = 1; break;
            case KEYCODE_MOVE_END: suffix = 'F'; codePoint = 1; break;
            case KEYCODE_INSERT: suffix = '~'; codePoint = 2; break;
            case KEYCODE_FORWARD_DEL: suffix = '~'; codePoint = 3; break;
            case KEYCODE_PAGE_UP: suffix = '~'; codePoint = 5; break;
            case KEYCODE_PAGE_DOWN: suffix = '~'; codePoint = 6; break;
            case KEYCODE_F1: suffix = 'P'; codePoint = 1; break;
            case KEYCODE_F2: suffix = 'Q'; codePoint = 1; break;
            // CSI R overlaps with cursor position reports; kitty uses CSI 13 ~ for F3.
            case KEYCODE_F3: suffix = '~'; codePoint = 13; break;
            case KEYCODE_F4: suffix = 'S'; codePoint = 1; break;
            case KEYCODE_F5: suffix = '~'; codePoint = 15; break;
            case KEYCODE_F6: suffix = '~'; codePoint = 17; break;
            case KEYCODE_F7: suffix = '~'; codePoint = 18; break;
            case KEYCODE_F8: suffix = '~'; codePoint = 19; break;
            case KEYCODE_F9: suffix = '~'; codePoint = 20; break;
            case KEYCODE_F10: suffix = '~'; codePoint = 21; break;
            case KEYCODE_F11: suffix = '~'; codePoint = 23; break;
            case KEYCODE_F12: suffix = '~'; codePoint = 24; break;
            case KEYCODE_NUMPAD_DOT: codePoint = numLock ? 57409 : 57426; break;
            case KEYCODE_NUMPAD_DIVIDE: codePoint = 57410; break;
            case KEYCODE_NUMPAD_MULTIPLY: codePoint = 57411; break;
            case KEYCODE_NUMPAD_SUBTRACT: codePoint = 57412; break;
            case KEYCODE_NUMPAD_ADD: codePoint = 57413; break;
            case KEYCODE_NUMPAD_ENTER: codePoint = 57414; break;
            case KEYCODE_NUMPAD_EQUALS: codePoint = 57415; break;
            case KEYCODE_NUMPAD_COMMA: codePoint = 57416; break;
            case KeyEvent.KEYCODE_CAPS_LOCK: codePoint = 57358; break;
            case KeyEvent.KEYCODE_SCROLL_LOCK: codePoint = 57359; break;
            case KEYCODE_NUM_LOCK: codePoint = 57360; break;
            case KEYCODE_SYSRQ: codePoint = 57361; break;
            case KEYCODE_BREAK: codePoint = 57362; break;
            default: return null;
        }
        int ordinaryModifiers = keyMode & (KEYMOD_SHIFT | KEYMOD_ALT | KEYMOD_CTRL | KEYMOD_SUPER);
        if (ordinaryModifiers == 0 && suffix == 'u') {
            // Preserve the recovery keys even when Caps Lock or Num Lock is on.
            if (codePoint == 13) return "\r";
            if (codePoint == 9) return "\t";
            if (codePoint == 127) return "\u007f";
        }
        return kittySequence(codePoint, keyMode, suffix, eventType);
    }

    /** Immutable layout identity, also retained across repeats for a matching release. */
    public static final class KittyKey {
        public final int codePoint;
        public final int shiftedCodePoint;
        public final int baseLayoutCodePoint;

        public KittyKey(int codePoint, int shiftedCodePoint, int baseLayoutCodePoint) {
            this.codePoint = codePoint;
            this.shiftedCodePoint = validAlternate(shiftedCodePoint, codePoint);
            this.baseLayoutCodePoint = validAlternate(baseLayoutCodePoint, codePoint);
        }

        private static int validAlternate(int alternate, int primary) {
            return alternate >= 32 && Character.isValidCodePoint(alternate) &&
                !(alternate >= 0xD800 && alternate <= 0xDFFF) && alternate != primary ? alternate : 0;
        }
    }

    /** Known standard PC-101 positions from Android key codes; unknown or extended keys are omitted. */
    public static int getKittyBaseLayoutCodePoint(int keyCode) {
        if (keyCode >= KeyEvent.KEYCODE_A && keyCode <= KeyEvent.KEYCODE_Z)
            return 'a' + keyCode - KeyEvent.KEYCODE_A;
        if (keyCode >= KeyEvent.KEYCODE_0 && keyCode <= KeyEvent.KEYCODE_9)
            return '0' + keyCode - KeyEvent.KEYCODE_0;
        switch (keyCode) {
            case KEYCODE_SPACE: return ' ';
            case KeyEvent.KEYCODE_GRAVE: return '`';
            case KeyEvent.KEYCODE_MINUS: return '-';
            case KeyEvent.KEYCODE_EQUALS: return '=';
            case KeyEvent.KEYCODE_LEFT_BRACKET: return '[';
            case KeyEvent.KEYCODE_RIGHT_BRACKET: return ']';
            case KeyEvent.KEYCODE_BACKSLASH: return '\\';
            case KeyEvent.KEYCODE_SEMICOLON: return ';';
            case KeyEvent.KEYCODE_APOSTROPHE: return '\'';
            case KeyEvent.KEYCODE_COMMA: return ',';
            case KeyEvent.KEYCODE_PERIOD: return '.';
            case KeyEvent.KEYCODE_SLASH: return '/';
            default: return 0;
        }
    }

    /** Plain and Shift-only text stays UTF-8; shortcut keys use the unshifted code point. */
    public static String getKittyCodePoint(int unshiftedCodePoint, int keyMode) {
        if (unshiftedCodePoint < 32 || !Character.isValidCodePoint(unshiftedCodePoint)) return null;
        if ((keyMode & (KEYMOD_ALT | KEYMOD_CTRL | KEYMOD_SUPER)) == 0) return null;
        return kittySequence(unshiftedCodePoint, keyMode & ~(KEYMOD_NUM_LOCK | KEYMOD_CAPS_LOCK), 'u');
    }

    /** Recovery control keys deliberately have no release events without report-all-keys support. */
    public static boolean isKittyRecoveryKey(int keyCode) {
        return keyCode == KEYCODE_ENTER || keyCode == KEYCODE_DPAD_CENTER ||
            keyCode == KEYCODE_TAB || keyCode == KEYCODE_DEL;
    }

    public static String getKittyCodePoint(int unshiftedCodePoint, int keyMode, int flags, int eventType) {
        return getKittyCodePoint(new KittyKey(unshiftedCodePoint, 0, 0), keyMode, flags, eventType);
    }

    /** Alternate reporting enhances an encoded event without turning layout text into an escape code. */
    public static String getKittyCodePoint(KittyKey key, int keyMode, int flags, int eventType) {
        if ((flags & KITTY_REPORT_EVENTS) == 0) {
            if (eventType == KEY_EVENT_RELEASE) return null;
            eventType = KEY_EVENT_PRESS;
        }
        if ((flags & KITTY_DISAMBIGUATE) == 0 && eventType == KEY_EVENT_PRESS) return null;
        if (key.codePoint < 32 || !Character.isValidCodePoint(key.codePoint)) return null;
        if (eventType != KEY_EVENT_RELEASE && getKittyCodePoint(key.codePoint, keyMode) == null) return null;
        String number = Integer.toString(key.codePoint);
        if ((flags & KITTY_REPORT_ALTERNATE_KEYS) != 0) {
            int shifted = (keyMode & KEYMOD_SHIFT) != 0 ? key.shiftedCodePoint : 0;
            if (shifted != 0 || key.baseLayoutCodePoint != 0)
                number += ":" + (shifted == 0 ? "" : shifted) +
                    (key.baseLayoutCodePoint == 0 ? "" : ":" + key.baseLayoutCodePoint);
        }
        return kittySequence(number, keyMode & ~(KEYMOD_NUM_LOCK | KEYMOD_CAPS_LOCK), 'u', eventType);
    }

    /** Reuse a previously encoded functional identity with the release event's current modifiers. */
    public static String getKittyReleaseCode(String releaseCode, int keyMode) {
        int separator = releaseCode.indexOf(';');
        int number = Integer.parseInt(releaseCode.substring(2, separator));
        return kittySequence(number, keyMode, releaseCode.charAt(releaseCode.length() - 1), KEY_EVENT_RELEASE);
    }

    private static String kittySequence(int number, int keyMode, char suffix) {
        return kittySequence(number, keyMode, suffix, KEY_EVENT_PRESS);
    }

    private static String kittySequence(int number, int keyMode, char suffix, int eventType) {
        return kittySequence(Integer.toString(number), keyMode, suffix, eventType);
    }

    private static String kittySequence(String number, int keyMode, char suffix, int eventType) {
        int modifiers = 1;
        if ((keyMode & KEYMOD_SHIFT) != 0) modifiers += 1;
        if ((keyMode & KEYMOD_ALT) != 0) modifiers += 2;
        if ((keyMode & KEYMOD_CTRL) != 0) modifiers += 4;
        if ((keyMode & KEYMOD_SUPER) != 0) modifiers += 8;
        if ((keyMode & KEYMOD_CAPS_LOCK) != 0) modifiers += 64;
        if ((keyMode & KEYMOD_NUM_LOCK) != 0) modifiers += 128;
        String prefix = number.equals("1") && suffix != 'u' && suffix != '~' &&
            modifiers == 1 && eventType == KEY_EVENT_PRESS ? "" : number;
        return "\033[" + prefix + (modifiers == 1 && eventType == KEY_EVENT_PRESS ? "" : ";" + modifiers) +
            (eventType == KEY_EVENT_PRESS ? "" : ":" + eventType) + suffix;
    }

    /** Select negotiated or legacy functional-key encoding. Null leaves layout text to the caller. */
    public static String getCode(int keyCode, int keyMode, boolean cursorApp, boolean keypadApplication,
                                 boolean kittyKeyboard) {
        return kittyKeyboard ? getKittyCode(keyCode, keyMode) :
            getCode(keyCode, keyMode, cursorApp, keypadApplication);
    }

    /** Event reporting is independent of disambiguation; text-producing legacy keypad keys stay text. */
    public static String getCode(int keyCode, int keyMode, boolean cursorApp, boolean keypadApplication,
                                 int flags, int eventType) {
        if ((flags & KITTY_REPORT_EVENTS) == 0) {
            if (eventType == KEY_EVENT_RELEASE) return null;
            return getCode(keyCode, keyMode, cursorApp, keypadApplication,
                (flags & KITTY_DISAMBIGUATE) != 0);
        }
        if ((flags & KITTY_DISAMBIGUATE) == 0) {
            String legacy = getCode(keyCode, keyMode, cursorApp, keypadApplication);
            if (isKittyRecoveryKey(keyCode)) {
                if (eventType == KEY_EVENT_RELEASE) return null;
                // Keep legacy presses and plain recovery repeats, but report modified repeats.
                return eventType == KEY_EVENT_REPEAT ? getKittyCode(keyCode, keyMode, eventType) : legacy;
            }
            if ((keyCode == KEYCODE_ESCAPE || keyCode == KEYCODE_BACK) && eventType == KEY_EVENT_PRESS)
                return legacy;
            if (keyCode >= KEYCODE_NUMPAD_0 && keyCode <= KEYCODE_NUMPAD_9) {
                if ((keyMode & KEYMOD_NUM_LOCK) != 0 || keyCode == KEYCODE_NUMPAD_5)
                    return eventType == KEY_EVENT_RELEASE ? null : legacy;
                keyCode = KEYPAD_NAVIGATION_KEYS[keyCode - KEYCODE_NUMPAD_0];
            } else if (keyCode == KEYCODE_NUMPAD_DOT && (keyMode & KEYMOD_NUM_LOCK) == 0) {
                keyCode = KEYCODE_FORWARD_DEL;
            } else if (keyCode >= KEYCODE_NUMPAD_DIVIDE && keyCode <= KeyEvent.KEYCODE_NUMPAD_RIGHT_PAREN) {
                return eventType == KEY_EVENT_RELEASE ? null : legacy;
            }
        }
        return getKittyCode(keyCode, keyMode, eventType);
    }

    private static final Map<String, Integer> TERMCAP_TO_KEYCODE = new HashMap<>();

    static {
        // terminfo: http://pubs.opengroup.org/onlinepubs/7990989799/xcurses/terminfo.html
        // termcap: http://man7.org/linux/man-pages/man5/termcap.5.html
        TERMCAP_TO_KEYCODE.put("%i", KEYMOD_SHIFT | KEYCODE_DPAD_RIGHT);
        TERMCAP_TO_KEYCODE.put("#2", KEYMOD_SHIFT | KEYCODE_MOVE_HOME); // Shifted home
        TERMCAP_TO_KEYCODE.put("#4", KEYMOD_SHIFT | KEYCODE_DPAD_LEFT);
        TERMCAP_TO_KEYCODE.put("*7", KEYMOD_SHIFT | KEYCODE_MOVE_END); // Shifted end key

        TERMCAP_TO_KEYCODE.put("k1", KEYCODE_F1);
        TERMCAP_TO_KEYCODE.put("k2", KEYCODE_F2);
        TERMCAP_TO_KEYCODE.put("k3", KEYCODE_F3);
        TERMCAP_TO_KEYCODE.put("k4", KEYCODE_F4);
        TERMCAP_TO_KEYCODE.put("k5", KEYCODE_F5);
        TERMCAP_TO_KEYCODE.put("k6", KEYCODE_F6);
        TERMCAP_TO_KEYCODE.put("k7", KEYCODE_F7);
        TERMCAP_TO_KEYCODE.put("k8", KEYCODE_F8);
        TERMCAP_TO_KEYCODE.put("k9", KEYCODE_F9);
        TERMCAP_TO_KEYCODE.put("k;", KEYCODE_F10);
        TERMCAP_TO_KEYCODE.put("F1", KEYCODE_F11);
        TERMCAP_TO_KEYCODE.put("F2", KEYCODE_F12);
        TERMCAP_TO_KEYCODE.put("F3", KEYMOD_SHIFT | KEYCODE_F1);
        TERMCAP_TO_KEYCODE.put("F4", KEYMOD_SHIFT | KEYCODE_F2);
        TERMCAP_TO_KEYCODE.put("F5", KEYMOD_SHIFT | KEYCODE_F3);
        TERMCAP_TO_KEYCODE.put("F6", KEYMOD_SHIFT | KEYCODE_F4);
        TERMCAP_TO_KEYCODE.put("F7", KEYMOD_SHIFT | KEYCODE_F5);
        TERMCAP_TO_KEYCODE.put("F8", KEYMOD_SHIFT | KEYCODE_F6);
        TERMCAP_TO_KEYCODE.put("F9", KEYMOD_SHIFT | KEYCODE_F7);
        TERMCAP_TO_KEYCODE.put("FA", KEYMOD_SHIFT | KEYCODE_F8);
        TERMCAP_TO_KEYCODE.put("FB", KEYMOD_SHIFT | KEYCODE_F9);
        TERMCAP_TO_KEYCODE.put("FC", KEYMOD_SHIFT | KEYCODE_F10);
        TERMCAP_TO_KEYCODE.put("FD", KEYMOD_SHIFT | KEYCODE_F11);
        TERMCAP_TO_KEYCODE.put("FE", KEYMOD_SHIFT | KEYCODE_F12);

        TERMCAP_TO_KEYCODE.put("kb", KEYCODE_DEL); // backspace key

        TERMCAP_TO_KEYCODE.put("kd", KEYCODE_DPAD_DOWN); // terminfo=kcud1, down-arrow key
        TERMCAP_TO_KEYCODE.put("kh", KEYCODE_MOVE_HOME);
        TERMCAP_TO_KEYCODE.put("kl", KEYCODE_DPAD_LEFT);
        TERMCAP_TO_KEYCODE.put("kr", KEYCODE_DPAD_RIGHT);

        // K1=Upper left of keypad:
        // t_K1 <kHome> keypad home key
        // t_K3 <kPageUp> keypad page-up key
        // t_K4 <kEnd> keypad end key
        // t_K5 <kPageDown> keypad page-down key
        TERMCAP_TO_KEYCODE.put("K1", KEYCODE_MOVE_HOME);
        TERMCAP_TO_KEYCODE.put("K3", KEYCODE_PAGE_UP);
        TERMCAP_TO_KEYCODE.put("K4", KEYCODE_MOVE_END);
        TERMCAP_TO_KEYCODE.put("K5", KEYCODE_PAGE_DOWN);

        TERMCAP_TO_KEYCODE.put("ku", KEYCODE_DPAD_UP);

        TERMCAP_TO_KEYCODE.put("kB", KEYMOD_SHIFT | KEYCODE_TAB); // termcap=kB, terminfo=kcbt: Back-tab
        TERMCAP_TO_KEYCODE.put("kD", KEYCODE_FORWARD_DEL); // terminfo=kdch1, delete-character key
        TERMCAP_TO_KEYCODE.put("kDN", KEYMOD_SHIFT | KEYCODE_DPAD_DOWN); // non-standard shifted arrow down
        TERMCAP_TO_KEYCODE.put("kF", KEYMOD_SHIFT | KEYCODE_DPAD_DOWN); // terminfo=kind, scroll-forward key
        TERMCAP_TO_KEYCODE.put("kI", KEYCODE_INSERT);
        TERMCAP_TO_KEYCODE.put("kP", KEYCODE_PAGE_UP);
        TERMCAP_TO_KEYCODE.put("kN", KEYCODE_PAGE_DOWN);
        TERMCAP_TO_KEYCODE.put("kR", KEYMOD_SHIFT | KEYCODE_DPAD_UP); // terminfo=kri, scroll-backward key
        TERMCAP_TO_KEYCODE.put("kUP", KEYMOD_SHIFT | KEYCODE_DPAD_UP); // non-standard shifted up

        TERMCAP_TO_KEYCODE.put("@7", KEYCODE_MOVE_END);
        TERMCAP_TO_KEYCODE.put("@8", KEYCODE_NUMPAD_ENTER);
    }

    static String getCodeFromTermcap(String termcap, boolean cursorKeysApplication, boolean keypadApplication) {
        Integer keyCodeAndMod = TERMCAP_TO_KEYCODE.get(termcap);
        if (keyCodeAndMod == null) return null;
        int keyCode = keyCodeAndMod;
        int keyMod = 0;
        if ((keyCode & KEYMOD_SHIFT) != 0) {
            keyMod |= KEYMOD_SHIFT;
            keyCode &= ~KEYMOD_SHIFT;
        }
        if ((keyCode & KEYMOD_CTRL) != 0) {
            keyMod |= KEYMOD_CTRL;
            keyCode &= ~KEYMOD_CTRL;
        }
        if ((keyCode & KEYMOD_ALT) != 0) {
            keyMod |= KEYMOD_ALT;
            keyCode &= ~KEYMOD_ALT;
        }
        if ((keyCode & KEYMOD_NUM_LOCK) != 0) {
            keyMod |= KEYMOD_NUM_LOCK;
            keyCode &= ~KEYMOD_NUM_LOCK;
        }
        return getCode(keyCode, keyMod, cursorKeysApplication, keypadApplication);
    }

    public static String getCode(int keyCode, int keyMode, boolean cursorApp, boolean keypadApplication) {
        boolean numLockOn = (keyMode & KEYMOD_NUM_LOCK) != 0;
        keyMode &= ~KEYMOD_NUM_LOCK;
        switch (keyCode) {
            case KEYCODE_DPAD_CENTER:
                return "\015";

            case KEYCODE_DPAD_UP:
                return (keyMode == 0) ? (cursorApp ? "\033OA" : "\033[A") : transformForModifiers("\033[1", keyMode, 'A');
            case KEYCODE_DPAD_DOWN:
                return (keyMode == 0) ? (cursorApp ? "\033OB" : "\033[B") : transformForModifiers("\033[1", keyMode, 'B');
            case KEYCODE_DPAD_RIGHT:
                return (keyMode == 0) ? (cursorApp ? "\033OC" : "\033[C") : transformForModifiers("\033[1", keyMode, 'C');
            case KEYCODE_DPAD_LEFT:
                return (keyMode == 0) ? (cursorApp ? "\033OD" : "\033[D") : transformForModifiers("\033[1", keyMode, 'D');

            case KEYCODE_MOVE_HOME:
                // Note that KEYCODE_HOME is handled by the system and never delivered to applications.
                // On a Logitech k810 keyboard KEYCODE_MOVE_HOME is sent by FN+LeftArrow.
                return (keyMode == 0) ? (cursorApp ? "\033OH" : "\033[H") : transformForModifiers("\033[1", keyMode, 'H');
            case KEYCODE_MOVE_END:
                return (keyMode == 0) ? (cursorApp ? "\033OF" : "\033[F") : transformForModifiers("\033[1", keyMode, 'F');

            // An xterm can send function keys F1 to F4 in two modes: vt100 compatible or
            // not. Because Vim may not know what the xterm is sending, both types of keys
            // are recognized. The same happens for the <Home> and <End> keys.
            // normal vt100 ~
            // <F1> t_k1 <Esc>[11~ <xF1> <Esc>OP *<xF1>-xterm*
            // <F2> t_k2 <Esc>[12~ <xF2> <Esc>OQ *<xF2>-xterm*
            // <F3> t_k3 <Esc>[13~ <xF3> <Esc>OR *<xF3>-xterm*
            // <F4> t_k4 <Esc>[14~ <xF4> <Esc>OS *<xF4>-xterm*
            // <Home> t_kh <Esc>[7~ <xHome> <Esc>OH *<xHome>-xterm*
            // <End> t_@7 <Esc>[4~ <xEnd> <Esc>OF *<xEnd>-xterm*
            case KEYCODE_F1:
                return (keyMode == 0) ? "\033OP" : transformForModifiers("\033[1", keyMode, 'P');
            case KEYCODE_F2:
                return (keyMode == 0) ? "\033OQ" : transformForModifiers("\033[1", keyMode, 'Q');
            case KEYCODE_F3:
                return (keyMode == 0) ? "\033OR" : transformForModifiers("\033[1", keyMode, 'R');
            case KEYCODE_F4:
                return (keyMode == 0) ? "\033OS" : transformForModifiers("\033[1", keyMode, 'S');
            case KEYCODE_F5:
                return transformForModifiers("\033[15", keyMode, '~');
            case KEYCODE_F6:
                return transformForModifiers("\033[17", keyMode, '~');
            case KEYCODE_F7:
                return transformForModifiers("\033[18", keyMode, '~');
            case KEYCODE_F8:
                return transformForModifiers("\033[19", keyMode, '~');
            case KEYCODE_F9:
                return transformForModifiers("\033[20", keyMode, '~');
            case KEYCODE_F10:
                return transformForModifiers("\033[21", keyMode, '~');
            case KEYCODE_F11:
                return transformForModifiers("\033[23", keyMode, '~');
            case KEYCODE_F12:
                return transformForModifiers("\033[24", keyMode, '~');

            case KEYCODE_SYSRQ:
                return "\033[32~"; // Sys Request / Print
            // Is this Scroll lock? case Cancel: return "\033[33~";
            case KEYCODE_BREAK:
                return "\033[34~"; // Pause/Break

            case KEYCODE_ESCAPE:
            case KEYCODE_BACK:
                return "\033";

            case KEYCODE_INSERT:
                return transformForModifiers("\033[2", keyMode, '~');
            case KEYCODE_FORWARD_DEL:
                return transformForModifiers("\033[3", keyMode, '~');

            case KEYCODE_PAGE_UP:
                return transformForModifiers("\033[5", keyMode, '~');
            case KEYCODE_PAGE_DOWN:
                return transformForModifiers("\033[6", keyMode, '~');
            case KEYCODE_DEL:
                String prefix = ((keyMode & KEYMOD_ALT) == 0) ? "" : "\033";
                // Just do what xterm and gnome-terminal does:
                return prefix + (((keyMode & KEYMOD_CTRL) == 0) ? "\u007F" : "\u0008");
            case KEYCODE_NUM_LOCK:
                if (keypadApplication) {
                    return "\033OP";
                } else {
                    return null;
                }
            case KEYCODE_SPACE:
                // If ctrl is not down, return null so that it goes through normal input processing (which may e.g. cause a
                // combining accent to be written):
                return ((keyMode & KEYMOD_CTRL) == 0) ? null : "\0";
            case KEYCODE_TAB:
                // This is back-tab when shifted:
                return (keyMode & KEYMOD_SHIFT) == 0 ? "\011" : "\033[Z";
            case KEYCODE_ENTER:
                return ((keyMode & KEYMOD_ALT) == 0) ? "\r" : "\033\r";

            case KEYCODE_NUMPAD_ENTER:
                return keypadApplication ? transformForModifiers("\033O", keyMode, 'M') : "\n";
            case KEYCODE_NUMPAD_MULTIPLY:
                return keypadApplication ? transformForModifiers("\033O", keyMode, 'j') : "*";
            case KEYCODE_NUMPAD_ADD:
                return keypadApplication ? transformForModifiers("\033O", keyMode, 'k') : "+";
            case KEYCODE_NUMPAD_COMMA:
                return ",";
            case KEYCODE_NUMPAD_DOT:
                if (numLockOn) {
                    return keypadApplication ? "\033On" : ".";
                } else {
                    // DELETE
                    return transformForModifiers("\033[3", keyMode, '~');
                }
            case KEYCODE_NUMPAD_SUBTRACT:
                return keypadApplication ? transformForModifiers("\033O", keyMode, 'm') : "-";
            case KEYCODE_NUMPAD_DIVIDE:
                return keypadApplication ? transformForModifiers("\033O", keyMode, 'o') : "/";
            case KEYCODE_NUMPAD_0:
                if (numLockOn) {
                    return keypadApplication ? transformForModifiers("\033O", keyMode, 'p') : "0";
                } else {
                    // INSERT
                    return transformForModifiers("\033[2", keyMode, '~');
                }
            case KEYCODE_NUMPAD_1:
                if (numLockOn) {
                    return keypadApplication ? transformForModifiers("\033O", keyMode, 'q') : "1";
                } else {
                    // END
                    return (keyMode == 0) ? (cursorApp ? "\033OF" : "\033[F") : transformForModifiers("\033[1", keyMode, 'F');
                }
            case KEYCODE_NUMPAD_2:
                if (numLockOn) {
                    return keypadApplication ? transformForModifiers("\033O", keyMode, 'r') : "2";
                } else {
                    // DOWN
                    return (keyMode == 0) ? (cursorApp ? "\033OB" : "\033[B") : transformForModifiers("\033[1", keyMode, 'B');
                }
            case KEYCODE_NUMPAD_3:
                if (numLockOn) {
                    return keypadApplication ? transformForModifiers("\033O", keyMode, 's') : "3";
                } else {
                    // PGDN
                    return "\033[6~";
                }
            case KEYCODE_NUMPAD_4:
                if (numLockOn) {
                    return keypadApplication ? transformForModifiers("\033O", keyMode, 't') : "4";
                } else {
                    // LEFT
                    return (keyMode == 0) ? (cursorApp ? "\033OD" : "\033[D") : transformForModifiers("\033[1", keyMode, 'D');
                }
            case KEYCODE_NUMPAD_5:
                return keypadApplication ? transformForModifiers("\033O", keyMode, 'u') : "5";
            case KEYCODE_NUMPAD_6:
                if (numLockOn) {
                    return keypadApplication ? transformForModifiers("\033O", keyMode, 'v') : "6";
                } else {
                    // RIGHT
                    return (keyMode == 0) ? (cursorApp ? "\033OC" : "\033[C") : transformForModifiers("\033[1", keyMode, 'C');
                }
            case KEYCODE_NUMPAD_7:
                if (numLockOn) {
                    return keypadApplication ? transformForModifiers("\033O", keyMode, 'w') : "7";
                } else {
                    // HOME
                    return (keyMode == 0) ? (cursorApp ? "\033OH" : "\033[H") : transformForModifiers("\033[1", keyMode, 'H');
                }
            case KEYCODE_NUMPAD_8:
                if (numLockOn) {
                    return keypadApplication ? transformForModifiers("\033O", keyMode, 'x') : "8";
                } else {
                    // UP
                    return (keyMode == 0) ? (cursorApp ? "\033OA" : "\033[A") : transformForModifiers("\033[1", keyMode, 'A');
                }
            case KEYCODE_NUMPAD_9:
                if (numLockOn) {
                    return keypadApplication ? transformForModifiers("\033O", keyMode, 'y') : "9";
                } else {
                    // PGUP
                    return "\033[5~";
                }
            case KEYCODE_NUMPAD_EQUALS:
                return keypadApplication ? transformForModifiers("\033O", keyMode, 'X') : "=";
        }

        return null;
    }

    private static String transformForModifiers(String start, int keymod, char lastChar) {
        int modifier;
        switch (keymod) {
            case KEYMOD_SHIFT:
                modifier = 2;
                break;
            case KEYMOD_ALT:
                modifier = 3;
                break;
            case (KEYMOD_SHIFT | KEYMOD_ALT):
                modifier = 4;
                break;
            case KEYMOD_CTRL:
                modifier = 5;
                break;
            case KEYMOD_SHIFT | KEYMOD_CTRL:
                modifier = 6;
                break;
            case KEYMOD_ALT | KEYMOD_CTRL:
                modifier = 7;
                break;
            case KEYMOD_SHIFT | KEYMOD_ALT | KEYMOD_CTRL:
                modifier = 8;
                break;
            default:
                return start + lastChar;
        }
        return start + (";" + modifier) + lastChar;
    }
}
