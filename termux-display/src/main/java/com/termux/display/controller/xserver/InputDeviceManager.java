package com.termux.display.controller.xserver;

import static com.termux.display.controller.xserver.Keyboard.createKeycodeMap;

import android.util.Log;
import android.view.KeyEvent;

import com.termux.display.LorieView;
import com.termux.display.controller.core.CursorLocker;
import com.termux.display.controller.xserver.events.Event;

import java.util.HashMap;

public class InputDeviceManager implements Pointer.OnPointerMotionListener, Keyboard.OnKeyboardListener, XResourceManager.OnResourceLifecycleListener {
    private static final byte MOUSE_WHEEL_DELTA = 120;
    private Window pointWindow;
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
    public void onCreateResource(XResource resource) {
        updatePointWindow();
    }

    @Override
    public void onFreeResource(XResource resource) {
        updatePointWindow();
    }

    private void updatePointWindow() {
        Window pointWindow = xServer.getPointWindow();
        this.pointWindow = pointWindow;
    }

    @Override
    public void onPointerButtonPress(Pointer.Button button) {
        if (xServer.cursorLocker.getState() == CursorLocker.State.LOCKED) {
//            Log.d("onPointerButtonPress LOCKED",button.code()+","+ button.flag());
            int wheelDelta = button == Pointer.Button.BUTTON_SCROLL_UP ? MOUSE_WHEEL_DELTA : (button == Pointer.Button.BUTTON_SCROLL_DOWN ? -MOUSE_WHEEL_DELTA : 0);
            xServer.sendMouseWheelEvent(0, wheelDelta);
        } else {
            if (button == Pointer.Button.BUTTON_SCROLL_UP || button == Pointer.Button.BUTTON_SCROLL_DOWN) {
                int wheelDelta = button == Pointer.Button.BUTTON_SCROLL_UP ? MOUSE_WHEEL_DELTA : (button == Pointer.Button.BUTTON_SCROLL_DOWN ? -MOUSE_WHEEL_DELTA : 0);
                xServer.sendMouseWheelEvent(0, wheelDelta);
                return;
            }
            xServer.sendMouseEvent(0, 0, button.code(), true, true);
//            Log.d("onPointerButtonPress UnLOCKED",button.code()+","+ button.flag());
        }
    }

    @Override
    public void onPointerButtonRelease(Pointer.Button button) {
        if (xServer.cursorLocker.getState() == CursorLocker.State.LOCKED) {
            xServer.sendMouseEvent(0, 0, button.code(), false, true);
//            Log.d("onPointerButtonRelease LOCKED",button.code()+","+ button.flag());
        } else {
            xServer.sendMouseEvent(0, 0, button.code(), false, true);
//            Log.d("onPointerButtonRelease UnLOCKED",button.code()+","+ button.flag());
        }
    }

    @Override
    public void onPointerMove(short x, short y) {
//        xServer.sendMouseEvent(x,y,Pointer.Button.BUTTON_LEFT.code(),false,true);
//        Log.d("onPointerMove",x+","+ y);
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

    private Bitmask createPointerEventMask() {
        Bitmask eventMask = new Bitmask();
        eventMask.set(Event.POINTER_MOTION);

        Bitmask buttonMask = xServer.pointer.getButtonMask();
        if (!buttonMask.isEmpty()) {
            eventMask.set(Event.BUTTON_MOTION);

            if (buttonMask.isSet(Pointer.Button.BUTTON_LEFT.flag())) {
                eventMask.set(Event.BUTTON1_MOTION);
            }
            if (buttonMask.isSet(Pointer.Button.BUTTON_MIDDLE.flag())) {
                eventMask.set(Event.BUTTON2_MOTION);
            }
            if (buttonMask.isSet(Pointer.Button.BUTTON_RIGHT.flag())) {
                eventMask.set(Event.BUTTON3_MOTION);
            }
            if (buttonMask.isSet(Pointer.Button.BUTTON_SCROLL_UP.flag())) {
                eventMask.set(Event.BUTTON4_MOTION);
            }
            if (buttonMask.isSet(Pointer.Button.BUTTON_SCROLL_DOWN.flag())) {
                eventMask.set(Event.BUTTON5_MOTION);
            }
        }
        return eventMask;
    }

    public Bitmask getKeyButMask() {
        Bitmask keyButMask = new Bitmask();
        keyButMask.join(xServer.pointer.getButtonMask());
        keyButMask.join(xServer.keyboard.getModifiersMask());
        return keyButMask;
    }
}
