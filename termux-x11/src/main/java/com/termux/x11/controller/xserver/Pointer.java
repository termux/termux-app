package com.termux.x11.controller.xserver;

import android.util.Log;

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

    private static final float EPSILON = 0.001f;
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
    }

    public Pointer(LorieView xServer) {
        this.xServer = xServer;
    }

    public void setX(int x) {
        if (screenPointLiesOutsideImageBoundaryX(x)){
            return;
        }
        this.x = x;
    }

    public void setY(int y) {
        if (screenPointLiesOutsideImageBoundaryY(y)){
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
        return (short) Mathf.clamp(x, 0, xServer.screenInfo.width - 1);
    }

    public short getClampedY() {
        return (short) Mathf.clamp(y, 0, xServer.screenInfo.height - 1);
    }

    public void moveTo(int x, int y) {
        if (xServer.screenInfo.setCursorPosition(x, y)) {
            setX(x);
            setY(y);
            triggerOnPointerMove(this.x-xServer.screenInfo.offsetX, this.y-xServer.screenInfo.offsetY);
        }
    }

    private boolean screenPointLiesOutsideImageBoundaryX(float screenX) {
        float scaledX = screenX / xServer.screenInfo.scale.x;
        float imageWidth = (float) xServer.screenInfo.imageWidth + EPSILON;
        Log.d("OutsideBoundaryX","screenX: "+screenX+", scaledX:"+scaledX+", imageWidth: "+imageWidth);
        return scaledX < -EPSILON || screenX > imageWidth;
    }

    private boolean screenPointLiesOutsideImageBoundaryY(float screenY) {
        float scaledY = screenY /xServer.screenInfo.scale.y;
        float imageHeight = (float) xServer.screenInfo.imageHeight + EPSILON;
//        Log.d("OutsideBoundaryX","screenY: "+screenY+", scaledY:"+scaledY+", imageHeight: "+imageHeight);
        return scaledY < -EPSILON || screenY > imageHeight;
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
}
