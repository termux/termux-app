package com.termux.terminal;

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

    public static final int KEYMOD_ALT = 0x80000000;
    public static final int KEYMOD_CTRL = 0x40000000;
    public static final int KEYMOD_SHIFT = 0x20000000;
    public static final int KEYMOD_NUM_LOCK = 0x10000000;

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
        TERMCAP_TO_KEYCODE.put("kN", KEYCODE_PAGE_UP);
        TERMCAP_TO_KEYCODE.put("kP", KEYCODE_PAGE_DOWN);
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
