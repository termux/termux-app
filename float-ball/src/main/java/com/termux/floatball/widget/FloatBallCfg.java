package com.termux.floatball.widget;

import android.graphics.drawable.Drawable;

public class FloatBallCfg {
    public Drawable mIcon;
    public int mSize;
    /**
     * Mark the position of the floating ball on the screen
     *
     * @see Gravity#LEFT_TOP
     * @see Gravity#LEFT_CENTER
     * @see Gravity#LEFT_BOTTOM
     * @see Gravity#RIGHT_TOP
     * @see Gravity#RIGHT_CENTER
     * @see Gravity#RIGHT_BOTTOM
     */
    public Gravity mGravity;
    //The y coordinate offset for the first display, with the upper left corner being the origin.
    public int mOffsetY = 0;
    public boolean mHideHalfLater = true;

    public FloatBallCfg(int size, Drawable icon) {
        this(size, icon, Gravity.LEFT_TOP, 0);
    }

    public FloatBallCfg(int size, Drawable icon, Gravity gravity) {
        this(size, icon, gravity, 0);
    }

    public FloatBallCfg(int size, Drawable icon, Gravity gravity, int offsetY) {
        mSize = size;
        mIcon = icon;
        mGravity = gravity;
        mOffsetY = offsetY;
    }

    public FloatBallCfg(int size, Drawable icon, Gravity gravity, boolean hideHalfLater) {
        mSize = size;
        mIcon = icon;
        mGravity = gravity;
        mHideHalfLater = hideHalfLater;
    }

    public FloatBallCfg(int size, Drawable icon, Gravity gravity, int offsetY, boolean hideHalfLater) {
        mSize = size;
        mIcon = icon;
        mGravity = gravity;
        mOffsetY = offsetY;
        mHideHalfLater = hideHalfLater;
    }

    public void setGravity(Gravity gravity) {
        mGravity = gravity;
    }

    public void setHideHalfLater(boolean hideHalfLater) {
        mHideHalfLater = hideHalfLater;
    }

    public enum Gravity {
        LEFT_TOP(android.view.Gravity.LEFT | android.view.Gravity.TOP),
        LEFT_CENTER(android.view.Gravity.LEFT | android.view.Gravity.CENTER),
        LEFT_BOTTOM(android.view.Gravity.LEFT | android.view.Gravity.BOTTOM),
        RIGHT_TOP(android.view.Gravity.RIGHT | android.view.Gravity.TOP),
        RIGHT_CENTER(android.view.Gravity.RIGHT | android.view.Gravity.CENTER),
        RIGHT_BOTTOM(android.view.Gravity.RIGHT | android.view.Gravity.BOTTOM);

        int mValue;

        Gravity(int gravity) {
            mValue = gravity;
        }

        public int getGravity() {
            return mValue;
        }
    }
}
