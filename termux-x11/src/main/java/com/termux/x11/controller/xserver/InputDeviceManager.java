package com.termux.x11.controller.xserver;

import static com.termux.x11.controller.xserver.Keyboard.createKeycodeMap;
import static com.termux.x11.input.InputStub.BUTTON_UNDEFINED;

import com.termux.x11.LorieView;

import java.util.HashMap;

public class InputDeviceManager implements Pointer.OnPointerMotionListener, Keyboard.OnKeyboardListener {
    private static final byte MOUSE_WHEEL_DELTA = 120;
    private final LorieView xServer;
    final private HashMap<Byte, Integer> eventKeyCodeMap = createKeyMap();

    public InputDeviceManager(LorieView xServer) {
        this.xServer = xServer;
        xServer.pointer.addOnPointerMotionListener(this);
        xServer.keyboard.addOnKeyboardListener(this);
    }

    private static HashMap<Byte, Integer> createKeyMap() {
        XKeycode[] keycodeMap = createKeycodeMap();
        HashMap<Byte, Integer> keyMap = new HashMap<>();
//        Log.d("createKeyMap","xKeycode");
        for (int i = 0; i < keycodeMap.length; i++) {
            XKeycode xKeycode = keycodeMap[i];
            if (xKeycode != null) {
                if (keyMap.get(xKeycode.id) != null) {
                } else {
                    keyMap.put(xKeycode.id, i);
                }
//                Log.d("createKeyMap","xKeycode:"+xKeycode.toString()+", i:"+i);
            }
        }
        return keyMap;
    }


    @Override
    public void onPointerButtonPress(Pointer.Button button) {
        if (button == Pointer.Button.BUTTON_SCROLL_UP || button == Pointer.Button.BUTTON_SCROLL_DOWN) {
            int wheelDelta = button == Pointer.Button.BUTTON_SCROLL_UP ? MOUSE_WHEEL_DELTA : (button == Pointer.Button.BUTTON_SCROLL_DOWN ? -MOUSE_WHEEL_DELTA : 0);
            xServer.sendMouseWheelEvent(0, wheelDelta);
            return;
        }
        xServer.sendMouseEvent(0, 0, button.code(), true, true);
    }

    @Override
    public void onPointerButtonRelease(Pointer.Button button) {
        xServer.sendMouseEvent(0, 0, button.code(), false, true);
    }

    @Override
    public void onPointerMove(int x, int y) {
//        Log.d("onPointerMove", "x:" + x + ", y:" + y);
        if (xServer.pointer.getPointerButton() != null) {
//            Log.d("onPointerMove", "x:" + x + ", y:" + y);
            xServer.sendMouseEvent(x, y, xServer.pointer.getPointerButton().code(), false, false);
        } else {
            xServer.sendMouseEvent(x, y, BUTTON_UNDEFINED, false, false);
        }
    }

    @Override
    public void onPointMoveDelta(int dx, int dy) {
//        if (xServer.pointer.getPointerButton() != null) {
//            xServer.sendMouseEvent(dx, dy, xServer.pointer.getPointerButton().code(), false, true);
//        } else {
//            xServer.sendMouseEvent(dx, dy, BUTTON_UNDEFINED, false, true);
//        }
        xServer.sendMouseEvent(dx, dy, BUTTON_UNDEFINED, false, true);
    }

    @Override
    public void onKeyPress(byte keycode, int keysym) {
        int realKeyCode = (int) keycode;
        Integer mkeyCode = eventKeyCodeMap.get(keycode);
        if (mkeyCode != null) {
            realKeyCode = mkeyCode;
        }
//        Log.d("onKeyPress",realKeyCode+"");
        if (keysym != 0 && !xServer.keyboard.hasKeysym(keycode, keysym)) {
            xServer.sendKeyEvent(keysym, realKeyCode, true);
            return;
        }
        xServer.sendKeyEvent(0, realKeyCode, true);
    }

    @Override
    public void onKeyRelease(byte keycode) {
        int realKeyCode = (int) keycode;
        Integer mkeyCode = eventKeyCodeMap.get(keycode);
        if (mkeyCode != null) {
            realKeyCode = mkeyCode;
        }
//        Log.d("onKeyRelease",realKeyCode+"");
        xServer.sendKeyEvent(0, realKeyCode, false);
    }
}
