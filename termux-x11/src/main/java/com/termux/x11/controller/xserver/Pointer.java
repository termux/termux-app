package com.termux.x11.controller.xserver;

import static androidx.core.math.MathUtils.clamp;

import com.termux.x11.LorieView;
import com.termux.x11.controller.math.Mathf;

import java.util.ArrayList;

public class Pointer {
    public enum Button {
        BUTTON_LEFT, BUTTON_MIDDLE, BUTTON_RIGHT, BUTTON_SCROLL_UP, BUTTON_SCROLL_DOWN, BUTTON_SCROLL_CLICK_LEFT, BUTTON_SCROLL_CLICK_RIGHT;

        public byte code() {
            return (byte) (ordinal() + 1);
        }

        public int flag() {
            return 1 << (code() + MAX_BUTTONS);
        }
    }

    public static final byte MAX_BUTTONS = 8;
    private final ArrayList<OnPointerMotionListener> onPointerMotionListeners = new ArrayList<>();
    private final Bitmask buttonMask = new Bitmask();
    private final LorieView xServer;
    private int x;
    private int y;
    private Button pointerButton;

    public Button getPointerButton() {
        return pointerButton;
    }

    public interface OnPointerMotionListener {
        default void onPointerButtonPress(Button button) {
        }

        default void onPointerButtonRelease(Button button) {
        }

        default void onPointerMove(int x, int y) {
        }

        default void onPointMoveDelta(int dx, int dy) {

        }
    }

    public Pointer(LorieView xServer) {
        this.xServer = xServer;
    }

    public void setX(int x) {
        if (screenPointLiesOutsideImageBoundaryX(x)) {
            return;
        }
        this.x = x;
    }

    public void setY(int y) {
        if (screenPointLiesOutsideImageBoundaryY(y)) {
            return;
        }
        this.y = y;
    }

    public int getX() {
        return x;
    }

    public int getY() {
        return y;
    }

    public short getClampedX() {
        return (short) Mathf.clamp(x, 0, xServer.screenInfo.screenWidth - 1);
    }

    public short getClampedY() {
        return (short) Mathf.clamp(y, 0, xServer.screenInfo.screenHeight - 1);
    }

    public void moveTo(int x, int y) {
        if (xServer.screenInfo.setCursorPosition(x, y)) {
            setX(x);
            setY(y);
            triggerOnPointerMove((int) ((this.x - xServer.screenInfo.offsetX) * xServer.screenInfo.scale.x), (int) ((this.y - xServer.screenInfo.offsetY) * xServer.screenInfo.scale.y));
        }
    }

    public void moveDelta(int dx, int dy) {
        triggerOnPointerMoveDelta(dx, dy);
    }

    private boolean screenPointLiesOutsideImageBoundaryX(float screenX) {
//        float scaledX = (screenX-xServer.screenInfo.offsetX) * xServer.screenInfo.scale.x;
//        float imageWidth = (float) xServer.screenInfo.imageWidth + EPSILON;
//        Log.d("OutsideBoundaryX", "screenX: " + screenX + ", scaledX:" + scaledX + ", imageWidth: " + imageWidth);
        return screenX < xServer.screenInfo.offsetX || screenX > xServer.screenInfo.imageWidth + xServer.screenInfo.offsetX;
    }

    private boolean screenPointLiesOutsideImageBoundaryY(float screenY) {
//        float scaledY = (screenY-xServer.screenInfo.offsetY) * xServer.screenInfo.scale.y;
//        float imageHeight = (float) xServer.screenInfo.imageHeight + EPSILON;
//        Log.d("OutsideBoundaryX","screenY: "+screenY+", scaledY:"+scaledY+", imageHeight: "+imageHeight);
        return screenY < xServer.screenInfo.offsetY || screenY > xServer.screenInfo.imageHeight + xServer.screenInfo.offsetY;
    }

    public void setButton(Button button, boolean pressed) {
        boolean oldPressed = isButtonPressed(button);
        buttonMask.set(button.flag(), pressed);
        if (oldPressed != pressed) {
            if (pressed) {
                triggerOnPointerButtonPress(button);
                this.pointerButton = button;
            } else {
                triggerOnPointerButtonRelease(button);
                this.pointerButton = null;
            }
        }
    }

    public boolean isButtonPressed(Button button) {
        return buttonMask.isSet(button.flag());
    }

    public void addOnPointerMotionListener(OnPointerMotionListener onPointerMotionListener) {
        onPointerMotionListeners.add(onPointerMotionListener);
    }

    public void removeOnPointerMotionListener(OnPointerMotionListener onPointerMotionListener) {
        onPointerMotionListeners.remove(onPointerMotionListener);
    }

    private void triggerOnPointerButtonPress(Button button) {
        for (int i = onPointerMotionListeners.size() - 1; i >= 0; i--) {
            onPointerMotionListeners.get(i).onPointerButtonPress(button);
        }
    }

    private void triggerOnPointerButtonRelease(Button button) {
        for (int i = onPointerMotionListeners.size() - 1; i >= 0; i--) {
            onPointerMotionListeners.get(i).onPointerButtonRelease(button);
        }
    }

    private void triggerOnPointerMove(int x, int y) {
        for (int i = onPointerMotionListeners.size() - 1; i >= 0; i--) {
            onPointerMotionListeners.get(i).onPointerMove(x, y);
        }
    }

    private void triggerOnPointerMoveDelta(int dx, int dy) {
        for (int i = onPointerMotionListeners.size() - 1; i >= 0; i--) {
            onPointerMotionListeners.get(i).onPointMoveDelta(dx, dy);
        }
    }
}
