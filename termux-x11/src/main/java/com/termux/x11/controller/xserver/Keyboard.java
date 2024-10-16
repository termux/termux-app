package com.termux.x11.controller.xserver;

import android.view.KeyEvent;

import androidx.collection.ArraySet;

import com.termux.x11.controller.inputcontrols.ExternalController;

import java.util.ArrayList;

import com.termux.x11.LorieView;

public class Keyboard {
    public static final byte KEYSYMS_PER_KEYCODE = 2;
    public static final short KEYS_COUNT = 248;
    public final int[] keysyms = new int[KEYS_COUNT];
    private final Bitmask modifiersMask = new Bitmask();
    private final XKeycode[] keycodeMap = createKeycodeMap();
    private final ArraySet<Byte> pressedKeys = new ArraySet<>();
    private final ArrayList<OnKeyboardListener> onKeyboardListeners = new ArrayList<>();
    private final LorieView xServer;

    public interface OnKeyboardListener {
        void onKeyPress(byte keycode, int keysym);

        void onKeyRelease(byte keycode);
    }

    public Keyboard(LorieView xServer) {
        this.xServer = xServer;
    }

    public Bitmask getModifiersMask() {
        return modifiersMask;
    }

    public void setKeysyms(byte keycode, int minKeysym, int majKeysym) {
        int index = keycode - 8;
        keysyms[index * KEYSYMS_PER_KEYCODE + 0] = minKeysym;
        keysyms[index * KEYSYMS_PER_KEYCODE + 1] = majKeysym;
    }

    public boolean hasKeysym(byte keycode, int keysym) {
        int index = keycode - 8;
        return keysyms[index * KEYSYMS_PER_KEYCODE + 0] == keysym || keysyms[index * KEYSYMS_PER_KEYCODE + 1] == keysym;
    }

    public void setKeyPress(byte keycode, int keysym) {
        if (isModifierSticky(keycode)) {
            if (pressedKeys.contains(keycode)) {
                pressedKeys.remove(keycode);
                modifiersMask.unset(getModifierFlag(keycode));
                triggerOnKeyRelease(keycode);
            } else {
                pressedKeys.add(keycode);
                modifiersMask.set(getModifierFlag(keycode));
                triggerOnKeyPress(keycode, keysym);
            }
        } else if (!pressedKeys.contains(keycode)) {
            pressedKeys.add(keycode);
            if (isModifier(keycode)) modifiersMask.set(getModifierFlag(keycode));
            triggerOnKeyPress(keycode, keysym);
        }
    }

    public void setKeyRelease(byte keycode) {
        if (!isModifierSticky(keycode) && pressedKeys.contains(keycode)) {
            pressedKeys.remove(keycode);
            if (isModifier(keycode)) modifiersMask.unset(getModifierFlag(keycode));
            triggerOnKeyRelease(keycode);
        }
    }

    public void addOnKeyboardListener(OnKeyboardListener onKeyboardListener) {
        onKeyboardListeners.add(onKeyboardListener);
    }

    public void removeOnKeyboardListener(OnKeyboardListener onKeyboardListener) {
        onKeyboardListeners.remove(onKeyboardListener);
    }

    private void triggerOnKeyPress(byte keycode, int keysym) {
        for (int i = onKeyboardListeners.size() - 1; i >= 0; i--) {
            onKeyboardListeners.get(i).onKeyPress(keycode, keysym);
        }
    }

    private void triggerOnKeyRelease(byte keycode) {
        for (int i = onKeyboardListeners.size() - 1; i >= 0; i--) {
            onKeyboardListeners.get(i).onKeyRelease(keycode);
        }
    }

    public boolean onKeyEvent(KeyEvent event) {
        if (ExternalController.isGameController(event.getDevice())) return false;

        int action = event.getAction();
        if (action == KeyEvent.ACTION_DOWN || action == KeyEvent.ACTION_UP) {
            int keyCode = event.getKeyCode();
            XKeycode xKeycode = keycodeMap[keyCode];
            if (xKeycode == null) return false;

            if (action == KeyEvent.ACTION_DOWN) {
                boolean shiftPressed = event.isShiftPressed() || keyCode == KeyEvent.KEYCODE_AT || keyCode == KeyEvent.KEYCODE_STAR || keyCode == KeyEvent.KEYCODE_POUND || keyCode == KeyEvent.KEYCODE_PLUS;
                if (shiftPressed) xServer.injectKeyPress(XKeycode.KEY_SHIFT_L);
                xServer.injectKeyPress(xKeycode, event.getUnicodeChar());
            } else if (action == KeyEvent.ACTION_UP) {
                xServer.injectKeyRelease(XKeycode.KEY_SHIFT_L);
                xServer.injectKeyRelease(xKeycode);
            }
        }
        return true;
    }

    static XKeycode[] createKeycodeMap() {
        XKeycode[] keycodeMap = new XKeycode[(KeyEvent.getMaxKeyCode() + 1)];
        keycodeMap[KeyEvent.KEYCODE_ESCAPE] = XKeycode.KEY_ESC;
        keycodeMap[KeyEvent.KEYCODE_ENTER] = XKeycode.KEY_ENTER;
        keycodeMap[KeyEvent.KEYCODE_DPAD_LEFT] = XKeycode.KEY_LEFT;
        keycodeMap[KeyEvent.KEYCODE_DPAD_RIGHT] = XKeycode.KEY_RIGHT;
        keycodeMap[KeyEvent.KEYCODE_DPAD_UP] = XKeycode.KEY_UP;
        keycodeMap[KeyEvent.KEYCODE_DPAD_DOWN] = XKeycode.KEY_DOWN;
        keycodeMap[KeyEvent.KEYCODE_DEL] = XKeycode.KEY_BKSP;
        keycodeMap[KeyEvent.KEYCODE_INSERT] = XKeycode.KEY_INSERT;
        keycodeMap[KeyEvent.KEYCODE_FORWARD_DEL] = XKeycode.KEY_DEL;
        keycodeMap[KeyEvent.KEYCODE_MOVE_HOME] = XKeycode.KEY_HOME;
        keycodeMap[KeyEvent.KEYCODE_MOVE_END] = XKeycode.KEY_END;
        keycodeMap[KeyEvent.KEYCODE_PAGE_UP] = XKeycode.KEY_PRIOR;
        keycodeMap[KeyEvent.KEYCODE_PAGE_DOWN] = XKeycode.KEY_NEXT;
        keycodeMap[KeyEvent.KEYCODE_SHIFT_LEFT] = XKeycode.KEY_SHIFT_L;
        keycodeMap[KeyEvent.KEYCODE_SHIFT_RIGHT] = XKeycode.KEY_SHIFT_R;
        keycodeMap[KeyEvent.KEYCODE_CTRL_LEFT] = XKeycode.KEY_CTRL_L;
        keycodeMap[KeyEvent.KEYCODE_CTRL_RIGHT] = XKeycode.KEY_CTRL_R;
        keycodeMap[KeyEvent.KEYCODE_ALT_LEFT] = XKeycode.KEY_ALT_L;
        keycodeMap[KeyEvent.KEYCODE_ALT_RIGHT] = XKeycode.KEY_ALT_R;
        keycodeMap[KeyEvent.KEYCODE_TAB] = XKeycode.KEY_TAB;
        keycodeMap[KeyEvent.KEYCODE_SPACE] = XKeycode.KEY_SPACE;
        keycodeMap[KeyEvent.KEYCODE_A] = XKeycode.KEY_A;
        keycodeMap[KeyEvent.KEYCODE_B] = XKeycode.KEY_B;
        keycodeMap[KeyEvent.KEYCODE_C] = XKeycode.KEY_C;
        keycodeMap[KeyEvent.KEYCODE_D] = XKeycode.KEY_D;
        keycodeMap[KeyEvent.KEYCODE_E] = XKeycode.KEY_E;
        keycodeMap[KeyEvent.KEYCODE_F] = XKeycode.KEY_F;
        keycodeMap[KeyEvent.KEYCODE_G] = XKeycode.KEY_G;
        keycodeMap[KeyEvent.KEYCODE_H] = XKeycode.KEY_H;
        keycodeMap[KeyEvent.KEYCODE_I] = XKeycode.KEY_I;
        keycodeMap[KeyEvent.KEYCODE_J] = XKeycode.KEY_J;
        keycodeMap[KeyEvent.KEYCODE_K] = XKeycode.KEY_K;
        keycodeMap[KeyEvent.KEYCODE_L] = XKeycode.KEY_L;
        keycodeMap[KeyEvent.KEYCODE_M] = XKeycode.KEY_M;
        keycodeMap[KeyEvent.KEYCODE_N] = XKeycode.KEY_N;
        keycodeMap[KeyEvent.KEYCODE_O] = XKeycode.KEY_O;
        keycodeMap[KeyEvent.KEYCODE_P] = XKeycode.KEY_P;
        keycodeMap[KeyEvent.KEYCODE_Q] = XKeycode.KEY_Q;
        keycodeMap[KeyEvent.KEYCODE_R] = XKeycode.KEY_R;
        keycodeMap[KeyEvent.KEYCODE_S] = XKeycode.KEY_S;
        keycodeMap[KeyEvent.KEYCODE_T] = XKeycode.KEY_T;
        keycodeMap[KeyEvent.KEYCODE_U] = XKeycode.KEY_U;
        keycodeMap[KeyEvent.KEYCODE_V] = XKeycode.KEY_V;
        keycodeMap[KeyEvent.KEYCODE_W] = XKeycode.KEY_W;
        keycodeMap[KeyEvent.KEYCODE_X] = XKeycode.KEY_X;
        keycodeMap[KeyEvent.KEYCODE_Y] = XKeycode.KEY_Y;
        keycodeMap[KeyEvent.KEYCODE_Z] = XKeycode.KEY_Z;
        keycodeMap[KeyEvent.KEYCODE_0] = XKeycode.KEY_0;
        keycodeMap[KeyEvent.KEYCODE_1] = XKeycode.KEY_1;
        keycodeMap[KeyEvent.KEYCODE_2] = XKeycode.KEY_2;
        keycodeMap[KeyEvent.KEYCODE_3] = XKeycode.KEY_3;
        keycodeMap[KeyEvent.KEYCODE_4] = XKeycode.KEY_4;
        keycodeMap[KeyEvent.KEYCODE_5] = XKeycode.KEY_5;
        keycodeMap[KeyEvent.KEYCODE_6] = XKeycode.KEY_6;
        keycodeMap[KeyEvent.KEYCODE_7] = XKeycode.KEY_7;
        keycodeMap[KeyEvent.KEYCODE_8] = XKeycode.KEY_8;
        keycodeMap[KeyEvent.KEYCODE_9] = XKeycode.KEY_9;
        keycodeMap[KeyEvent.KEYCODE_STAR] = XKeycode.KEY_8;
        keycodeMap[KeyEvent.KEYCODE_POUND] = XKeycode.KEY_3;
        keycodeMap[KeyEvent.KEYCODE_COMMA] = XKeycode.KEY_COMMA;
        keycodeMap[KeyEvent.KEYCODE_PERIOD] = XKeycode.KEY_PERIOD;
        keycodeMap[KeyEvent.KEYCODE_SEMICOLON] = XKeycode.KEY_SEMICOLON;
        keycodeMap[KeyEvent.KEYCODE_APOSTROPHE] = XKeycode.KEY_APOSTROPHE;
        keycodeMap[KeyEvent.KEYCODE_LEFT_BRACKET] = XKeycode.KEY_BRACKET_LEFT;
        keycodeMap[KeyEvent.KEYCODE_RIGHT_BRACKET] = XKeycode.KEY_BRACKET_RIGHT;
        keycodeMap[KeyEvent.KEYCODE_GRAVE] = XKeycode.KEY_GRAVE;
        keycodeMap[KeyEvent.KEYCODE_MINUS] = XKeycode.KEY_MINUS;
        keycodeMap[KeyEvent.KEYCODE_PLUS] = XKeycode.KEY_EQUAL;
        keycodeMap[KeyEvent.KEYCODE_EQUALS] = XKeycode.KEY_EQUAL;
        keycodeMap[KeyEvent.KEYCODE_SLASH] = XKeycode.KEY_SLASH;
        keycodeMap[KeyEvent.KEYCODE_AT] = XKeycode.KEY_2;
        keycodeMap[KeyEvent.KEYCODE_BACKSLASH] = XKeycode.KEY_BACKSLASH;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_DIVIDE] = XKeycode.KEY_KP_DIVIDE;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_MULTIPLY] = XKeycode.KEY_KP_MULTIPLY;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_SUBTRACT] = XKeycode.KEY_KP_SUBTRACT;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_ADD] = XKeycode.KEY_KP_ADD;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_DOT] = XKeycode.KEY_KP_DEL;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_0] = XKeycode.KEY_KP_0;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_1] = XKeycode.KEY_KP_1;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_2] = XKeycode.KEY_KP_2;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_3] = XKeycode.KEY_KP_3;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_4] = XKeycode.KEY_KP_4;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_5] = XKeycode.KEY_KP_5;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_6] = XKeycode.KEY_KP_6;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_7] = XKeycode.KEY_KP_7;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_8] = XKeycode.KEY_KP_8;
        keycodeMap[KeyEvent.KEYCODE_NUMPAD_9] = XKeycode.KEY_KP_9;
        keycodeMap[KeyEvent.KEYCODE_F1] = XKeycode.KEY_F1;
        keycodeMap[KeyEvent.KEYCODE_F2] = XKeycode.KEY_F2;
        keycodeMap[KeyEvent.KEYCODE_F3] = XKeycode.KEY_F3;
        keycodeMap[KeyEvent.KEYCODE_F4] = XKeycode.KEY_F4;
        keycodeMap[KeyEvent.KEYCODE_F5] = XKeycode.KEY_F5;
        keycodeMap[KeyEvent.KEYCODE_F6] = XKeycode.KEY_F6;
        keycodeMap[KeyEvent.KEYCODE_F7] = XKeycode.KEY_F7;
        keycodeMap[KeyEvent.KEYCODE_F8] = XKeycode.KEY_F8;
        keycodeMap[KeyEvent.KEYCODE_F9] = XKeycode.KEY_F9;
        keycodeMap[KeyEvent.KEYCODE_F10] = XKeycode.KEY_F10;
        keycodeMap[KeyEvent.KEYCODE_F11] = XKeycode.KEY_F11;
        keycodeMap[KeyEvent.KEYCODE_F12] = XKeycode.KEY_F12;
        keycodeMap[KeyEvent.KEYCODE_NUM_LOCK] = XKeycode.KEY_NUM_LOCK;
        keycodeMap[KeyEvent.KEYCODE_CAPS_LOCK] = XKeycode.KEY_CAPS_LOCK;
        return keycodeMap;
    }


    public static Keyboard createKeyboard(LorieView xServer) {
        Keyboard keyboard = new Keyboard(xServer);
        keyboard.setKeysyms(XKeycode.KEY_ESC.id, 65307, 0);
        keyboard.setKeysyms(XKeycode.KEY_ENTER.id, 65293, 0);
        keyboard.setKeysyms(XKeycode.KEY_RIGHT.id, 65363, 0);
        keyboard.setKeysyms(XKeycode.KEY_UP.id, 65362, 0);
        keyboard.setKeysyms(XKeycode.KEY_LEFT.id, 65361, 0);
        keyboard.setKeysyms(XKeycode.KEY_DOWN.id, 65364, 0);
        keyboard.setKeysyms(XKeycode.KEY_DEL.id, 65535, 0);
        keyboard.setKeysyms(XKeycode.KEY_BKSP.id, 65288, 0);
        keyboard.setKeysyms(XKeycode.KEY_INSERT.id, 65379, 0);
        keyboard.setKeysyms(XKeycode.KEY_PRIOR.id, 65365, 0);
        keyboard.setKeysyms(XKeycode.KEY_NEXT.id, 65366, 0);
        keyboard.setKeysyms(XKeycode.KEY_HOME.id, 65360, 0);
        keyboard.setKeysyms(XKeycode.KEY_END.id, 65367, 0);
        keyboard.setKeysyms(XKeycode.KEY_SHIFT_L.id, 65505, 0);
        keyboard.setKeysyms(XKeycode.KEY_SHIFT_R.id, 65506, 0);
        keyboard.setKeysyms(XKeycode.KEY_CTRL_L.id, 65507, 0);
        keyboard.setKeysyms(XKeycode.KEY_CTRL_R.id, 65508, 0);
        keyboard.setKeysyms(XKeycode.KEY_ALT_L.id, 65511, 0);
        keyboard.setKeysyms(XKeycode.KEY_ALT_R.id, 65512, 0);
        keyboard.setKeysyms(XKeycode.KEY_TAB.id, 65289, 0);
        keyboard.setKeysyms(XKeycode.KEY_SPACE.id, 32, 32);
        keyboard.setKeysyms(XKeycode.KEY_A.id, 97, 65);
        keyboard.setKeysyms(XKeycode.KEY_B.id, 98, 66);
        keyboard.setKeysyms(XKeycode.KEY_C.id, 99, 67);
        keyboard.setKeysyms(XKeycode.KEY_D.id, 100, 68);
        keyboard.setKeysyms(XKeycode.KEY_E.id, 101, 69);
        keyboard.setKeysyms(XKeycode.KEY_F.id, 102, 70);
        keyboard.setKeysyms(XKeycode.KEY_G.id, 103, 71);
        keyboard.setKeysyms(XKeycode.KEY_H.id, 104, 72);
        keyboard.setKeysyms(XKeycode.KEY_I.id, 105, 73);
        keyboard.setKeysyms(XKeycode.KEY_J.id, 106, 74);
        keyboard.setKeysyms(XKeycode.KEY_K.id, 107, 75);
        keyboard.setKeysyms(XKeycode.KEY_L.id, 108, 76);
        keyboard.setKeysyms(XKeycode.KEY_M.id, 109, 77);
        keyboard.setKeysyms(XKeycode.KEY_N.id, 110, 78);
        keyboard.setKeysyms(XKeycode.KEY_O.id, 111, 79);
        keyboard.setKeysyms(XKeycode.KEY_P.id, 112, 80);
        keyboard.setKeysyms(XKeycode.KEY_Q.id, 113, 81);
        keyboard.setKeysyms(XKeycode.KEY_R.id, 114, 82);
        keyboard.setKeysyms(XKeycode.KEY_S.id, 115, 83);
        keyboard.setKeysyms(XKeycode.KEY_T.id, 116, 84);
        keyboard.setKeysyms(XKeycode.KEY_U.id, 117, 85);
        keyboard.setKeysyms(XKeycode.KEY_V.id, 118, 86);
        keyboard.setKeysyms(XKeycode.KEY_W.id, 119, 87);
        keyboard.setKeysyms(XKeycode.KEY_X.id, 120, 88);
        keyboard.setKeysyms(XKeycode.KEY_Y.id, 121, 89);
        keyboard.setKeysyms(XKeycode.KEY_Z.id, 122, 90);
        keyboard.setKeysyms(XKeycode.KEY_1.id, 49, 33);
        keyboard.setKeysyms(XKeycode.KEY_2.id, 50, 64);
        keyboard.setKeysyms(XKeycode.KEY_3.id, 51, 35);
        keyboard.setKeysyms(XKeycode.KEY_4.id, 52, 36);
        keyboard.setKeysyms(XKeycode.KEY_5.id, 53, 37);
        keyboard.setKeysyms(XKeycode.KEY_6.id, 54, 94);
        keyboard.setKeysyms(XKeycode.KEY_7.id, 55, 38);
        keyboard.setKeysyms(XKeycode.KEY_8.id, 56, 42);
        keyboard.setKeysyms(XKeycode.KEY_9.id, 57, 40);
        keyboard.setKeysyms(XKeycode.KEY_0.id, 48, 41);
        keyboard.setKeysyms(XKeycode.KEY_COMMA.id, 44, 60);
        keyboard.setKeysyms(XKeycode.KEY_PERIOD.id, 46, 62);
        keyboard.setKeysyms(XKeycode.KEY_SEMICOLON.id, 59, 58);
        keyboard.setKeysyms(XKeycode.KEY_APOSTROPHE.id, 39, 34);
        keyboard.setKeysyms(XKeycode.KEY_BRACKET_LEFT.id, 91, 123);
        keyboard.setKeysyms(XKeycode.KEY_BRACKET_RIGHT.id, 93, 125);
        keyboard.setKeysyms(XKeycode.KEY_GRAVE.id, 96, 126);
        keyboard.setKeysyms(XKeycode.KEY_MINUS.id, 45, 95);
        keyboard.setKeysyms(XKeycode.KEY_EQUAL.id, 61, 43);
        keyboard.setKeysyms(XKeycode.KEY_SLASH.id, 47, 63);
        keyboard.setKeysyms(XKeycode.KEY_BACKSLASH.id, 92, 124);
        keyboard.setKeysyms(XKeycode.KEY_KP_DIVIDE.id, 65455, 65455);
        keyboard.setKeysyms(XKeycode.KEY_KP_MULTIPLY.id, 65450, 65450);
        keyboard.setKeysyms(XKeycode.KEY_KP_SUBTRACT.id, 65453, 65453);
        keyboard.setKeysyms(XKeycode.KEY_KP_ADD.id, 65451, 65451);
        keyboard.setKeysyms(XKeycode.KEY_KP_0.id, 65456, 65438);
        keyboard.setKeysyms(XKeycode.KEY_KP_1.id, 65457, 65436);
        keyboard.setKeysyms(XKeycode.KEY_KP_2.id, 65458, 65433);
        keyboard.setKeysyms(XKeycode.KEY_KP_3.id, 65459, 65459);
        keyboard.setKeysyms(XKeycode.KEY_KP_4.id, 65460, 65430);
        keyboard.setKeysyms(XKeycode.KEY_KP_5.id, 65461, 65461);
        keyboard.setKeysyms(XKeycode.KEY_KP_6.id, 65462, 65432);
        keyboard.setKeysyms(XKeycode.KEY_KP_7.id, 65463, 65429);
        keyboard.setKeysyms(XKeycode.KEY_KP_8.id, 65464, 65431);
        keyboard.setKeysyms(XKeycode.KEY_KP_9.id, 65465, 65465);
        keyboard.setKeysyms(XKeycode.KEY_KP_DEL.id, 65439, 0);
        keyboard.setKeysyms(XKeycode.KEY_F1.id, 65470, 0);
        keyboard.setKeysyms(XKeycode.KEY_F2.id, 65471, 0);
        keyboard.setKeysyms(XKeycode.KEY_F3.id, 65472, 0);
        keyboard.setKeysyms(XKeycode.KEY_F4.id, 65473, 0);
        keyboard.setKeysyms(XKeycode.KEY_F5.id, 65474, 0);
        keyboard.setKeysyms(XKeycode.KEY_F6.id, 65475, 0);
        keyboard.setKeysyms(XKeycode.KEY_F7.id, 65476, 0);
        keyboard.setKeysyms(XKeycode.KEY_F8.id, 65477, 0);
        keyboard.setKeysyms(XKeycode.KEY_F9.id, 65478, 0);
        keyboard.setKeysyms(XKeycode.KEY_F10.id, 65479, 0);
        keyboard.setKeysyms(XKeycode.KEY_F11.id, 65480, 0);
        keyboard.setKeysyms(XKeycode.KEY_F12.id, 65481, 0);
        return keyboard;
    }

    public static boolean isModifier(byte keycode) {
        return
            keycode == XKeycode.KEY_SHIFT_L.id ||
                keycode == XKeycode.KEY_SHIFT_R.id ||
                keycode == XKeycode.KEY_CTRL_L.id ||
                keycode == XKeycode.KEY_CTRL_R.id ||
                keycode == XKeycode.KEY_ALT_L.id ||
                keycode == XKeycode.KEY_ALT_R.id ||
                keycode == XKeycode.KEY_CAPS_LOCK.id ||
                keycode == XKeycode.KEY_NUM_LOCK.id
            ;
    }

    public static int getModifierFlag(byte keycode) {
        if (keycode == XKeycode.KEY_SHIFT_L.id || keycode == XKeycode.KEY_SHIFT_R.id) {
            return 1;
        } else if (keycode == XKeycode.KEY_CAPS_LOCK.id) {
            return 2;
        } else if (keycode == XKeycode.KEY_CTRL_L.id || keycode == XKeycode.KEY_CTRL_R.id) {
            return 4;
        } else if (keycode == XKeycode.KEY_ALT_L.id || keycode == XKeycode.KEY_ALT_R.id) {
            return 8;
        } else if (keycode == XKeycode.KEY_NUM_LOCK.id) {
            return 16;
        }
        return 0;
    }

    public static boolean isModifierSticky(byte keycode) {
        return keycode == XKeycode.KEY_CAPS_LOCK.id || keycode == XKeycode.KEY_NUM_LOCK.id;
    }
}
