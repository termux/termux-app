package com.termux.floatball.utils;


import android.content.Context;
import android.view.MotionEvent;
import android.view.VelocityTracker;
import android.view.ViewConfiguration;

public class MotionVelocityUtil {
    private VelocityTracker mVelocityTracker;
    private int mMaxVelocity, mMinVelocity;

    public MotionVelocityUtil(Context context) {
        mMaxVelocity = ViewConfiguration.get(context).getScaledMaximumFlingVelocity();
        mMinVelocity = ViewConfiguration.get(context).getScaledMinimumFlingVelocity();
    }

    public int getMinVelocity() {
        return mMinVelocity < 1000 ? 1000 : mMinVelocity;
    }

    /**
     * @param event 向VelocityTracker添加MotionEvent
     * @see VelocityTracker#obtain()
     * @see VelocityTracker#addMovement(MotionEvent)
     */
    public void acquireVelocityTracker(final MotionEvent event) {
        if (null == mVelocityTracker) {
            mVelocityTracker = VelocityTracker.obtain();
        }
        mVelocityTracker.addMovement(event);
    }

    public void computeCurrentVelocity() {
        mVelocityTracker.computeCurrentVelocity(1000, mMaxVelocity);
    }

    /**
     * Retrieve the last computed X velocity.  You must first call
     * {@link #computeCurrentVelocity()} before calling this function.
     *
     * @return The previously computed X velocity.
     */
    public float getXVelocity() {
        return mVelocityTracker.getXVelocity();
    }

    /**
     * Retrieve the last computed Y velocity.  You must first call
     * {@link #computeCurrentVelocity()} before calling this function.
     *
     * @return The previously computed Y velocity.
     */
    public float getYVelocity() {
        return mVelocityTracker.getYVelocity();
    }

    /**
     * 释放VelocityTracker
     *
     * @see VelocityTracker#clear()
     * @see VelocityTracker#recycle()
     */
    public void releaseVelocityTracker() {
        if (null != mVelocityTracker) {
            mVelocityTracker.clear();
            mVelocityTracker.recycle();
            mVelocityTracker = null;
        }
    }
}
