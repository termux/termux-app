package com.termux.display.controller.xserver;

import com.termux.display.LorieView;
import com.termux.display.controller.core.CursorLocker;
import com.termux.display.controller.winhandler.MouseEventFlags;
import com.termux.display.controller.winhandler.WinHandler;
import com.termux.display.controller.xserver.events.ButtonPress;
import com.termux.display.controller.xserver.events.ButtonRelease;
import com.termux.display.controller.xserver.events.Event;
import com.termux.display.controller.xserver.events.KeyPress;
import com.termux.display.controller.xserver.events.KeyRelease;
import com.termux.display.controller.xserver.events.MappingNotify;
import com.termux.display.controller.xserver.events.MotionNotify;
import com.termux.display.controller.xserver.events.PointerWindowEvent;

public class InputDeviceManager implements Pointer.OnPointerMotionListener, Keyboard.OnKeyboardListener,  XResourceManager.OnResourceLifecycleListener {
    private static final byte MOUSE_WHEEL_DELTA = 120;
    private Window pointWindow;
    private final LorieView xServer;

    public InputDeviceManager(LorieView xServer) {
        this.xServer = xServer;
        xServer.pointer.addOnPointerMotionListener(this);
        xServer.keyboard.addOnKeyboardListener(this);
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

    public Window getPointWindow() {
        return pointWindow;
    }

    private void sendEvent(Window window, int eventId, Event event) {
        window.sendEvent(eventId, event);
    }

    private void sendEvent(Window window, Bitmask eventMask, Event event) {
        window.sendEvent(eventMask, event);
    }
    @Override
    public void onPointerButtonPress(Pointer.Button button) {
        if (xServer.cursorLocker.getState() == CursorLocker.State.LOCKED) {
            int wheelDelta = button == Pointer.Button.BUTTON_SCROLL_UP ? MOUSE_WHEEL_DELTA : (button == Pointer.Button.BUTTON_SCROLL_DOWN ? -MOUSE_WHEEL_DELTA : 0);
            xServer.sendMouseWheelEvent(0,wheelDelta);
        }
        else {
            Bitmask eventMask = createPointerEventMask();
            eventMask.unset(button.flag());

            short x = xServer.pointer.getX();
            short y = xServer.pointer.getY();
            xServer.sendMouseEvent(x,y,button.code(),true,true);
        }
    }

    @Override
    public void onPointerButtonRelease(Pointer.Button button) {
        if (xServer.cursorLocker.getState() == CursorLocker.State.LOCKED) {
            xServer.sendMouseWheelEvent(0,0);
        }
        else {
            short x = xServer.pointer.getX();
            short y = xServer.pointer.getY();
            xServer.sendMouseEvent(x,y,button.code(),false,true);
        }
    }

    @Override
    public void onPointerMove(short x, short y) {
        xServer.sendMouseEvent(x,y,Pointer.Button.BUTTON_LEFT.code(),false,true);
    }

    @Override
    public void onKeyPress(byte keycode, int keysym) {
        Bitmask keyButMask = getKeyButMask();
        short x = xServer.pointer.getX();
        short y = xServer.pointer.getY();
        short[] localPoint = {0,0};
        if (keysym != 0 && !xServer.keyboard.hasKeysym(keycode, keysym)) {
            xServer.sendKeyEvent(keysym,keycode,true);
        }
        xServer.sendKeyEvent(0,keycode,true);
    }

    @Override
    public void onKeyRelease(byte keycode) {

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
