package com.termux.x11;

import static com.termux.x11.input.InputStub.BUTTON_UNDEFINED;

import android.graphics.PointF;

public class ScreenInfo {
    private LorieView xserver;
    public int screenWidth =1024;
    public int screenHeight =768;

    public int imageWidth;
    public int imageHeight;
    public int offsetX;
    public int offsetY;
    private final PointF mCursorPosition = new PointF();
    public PointF scale = new PointF();

    public ScreenInfo(LorieView xserver) {
       this.xserver = xserver;
    }

    public ScreenInfo(int screenWidth, int screenHeight) {
        this.screenWidth = (short) screenWidth;
        this.screenHeight = (short) screenHeight;
    }

    public short getWidthInMillimeters() {
        return (short) (screenWidth / 10);
    }

    public short getHeightInMillimeters() {
        return (short) (screenHeight / 10);
    }

    @Override
    public String toString() {
        return screenWidth + "x" + screenHeight;
    }

    private void resetTransformation() {
        float sx = (float) screenWidth / (float) imageWidth;
        float sy = (float) screenHeight / (float) imageHeight;
//        float sx = (float) imageWidth / (float) screenWidth;
//        float sy = (float) imageHeight / (float) screenHeight;
        scale.set(sx, sy);
    }

    public void handleClientSizeChanged(int w, int h) {
        screenWidth = w;
        screenHeight = h;
        moveCursorToScreenPoint((float) w / 2, (float) h / 2);

        resetTransformation();
    }

    public void handleHostSizeChanged(int w, int h) {
        imageWidth = w;
        imageHeight = h;
        moveCursorToScreenPoint((float) w / 2, (float) h / 2);
        resetTransformation();
    }

    private void moveCursorToScreenPoint(float screenX, float screenY) {
        float[] imagePoint = {(screenX - offsetX) * scale.x, (screenY - offsetY) * scale.y};
        if (setCursorPosition(imagePoint[0], imagePoint[1])) {
            xserver.sendMouseEvent(imagePoint[0], imagePoint[1], BUTTON_UNDEFINED, false, false);
        }
    }

    public boolean setCursorPosition(float newX, float newY) {
        boolean cursorMoved = false;
        if ((newX - offsetX) != mCursorPosition.x) {
            mCursorPosition.x = newX - offsetX;
            cursorMoved = true;
        }
        if ((newY - offsetY) != mCursorPosition.y) {
            mCursorPosition.y = newY - offsetY;
            cursorMoved = true;
        }
        return cursorMoved;
    }
}
