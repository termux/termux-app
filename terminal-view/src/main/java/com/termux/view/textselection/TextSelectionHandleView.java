package com.termux.view.textselection;

import android.annotation.SuppressLint;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.SystemClock;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import android.view.WindowManager;
import android.widget.PopupWindow;

import com.termux.view.R;
import com.termux.view.TerminalView;
import com.termux.view.support.PopupWindowCompatGingerbread;

@SuppressLint("ViewConstructor")
public class TextSelectionHandleView extends View {
    private final TerminalView terminalView;
    private PopupWindow mHandle;
    private final CursorController mCursorController;

    private final Drawable mHandleLeftDrawable;
    private final Drawable mHandleRightDrawable;
    private Drawable mHandleDrawable;

    private boolean mIsDragging;

    final int[] mTempCoords = new int[2];
    Rect mTempRect;

    private int mPointX;
    private int mPointY;
    private float mTouchToWindowOffsetX;
    private float mTouchToWindowOffsetY;
    private float mHotspotX;
    private float mHotspotY;
    private float mTouchOffsetY;
    private int mLastParentX;
    private int mLastParentY;

    private int mHandleHeight;
    private int mHandleWidth;

    private final int mInitialOrientation;
    private int mOrientation;

    public static final int LEFT = 0;
    public static final int RIGHT = 2;

    private long mLastTime;

    public TextSelectionHandleView(TerminalView terminalView, CursorController cursorController, int initialOrientation) {
        super(terminalView.getContext());
        this.terminalView = terminalView;
        mCursorController = cursorController;
        mInitialOrientation = initialOrientation;

        mHandleLeftDrawable = getContext().getDrawable(R.drawable.text_select_handle_left_material);
        mHandleRightDrawable = getContext().getDrawable(R.drawable.text_select_handle_right_material);

        setOrientation(mInitialOrientation);
    }

    private void initHandle() {
        mHandle = new PopupWindow(terminalView.getContext(), null,
            android.R.attr.textSelectHandleWindowStyle);
        mHandle.setSplitTouchEnabled(true);
        mHandle.setClippingEnabled(false);
        mHandle.setWidth(ViewGroup.LayoutParams.WRAP_CONTENT);
        mHandle.setHeight(ViewGroup.LayoutParams.WRAP_CONTENT);
        mHandle.setBackgroundDrawable(null);
        mHandle.setAnimationStyle(0);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            mHandle.setWindowLayoutType(WindowManager.LayoutParams.TYPE_APPLICATION_SUB_PANEL);
            mHandle.setEnterTransition(null);
            mHandle.setExitTransition(null);
        } else {
            PopupWindowCompatGingerbread.setWindowLayoutType(mHandle, WindowManager.LayoutParams.TYPE_APPLICATION_SUB_PANEL);
        }
        mHandle.setContentView(this);
    }

    public void setOrientation(int orientation) {
        mOrientation = orientation;
        int handleWidth = 0;
        switch (orientation) {
            case LEFT: {
                mHandleDrawable = mHandleLeftDrawable;
                handleWidth = mHandleDrawable.getIntrinsicWidth();
                mHotspotX = (handleWidth * 3) / (float) 4;
                break;
            }

            case RIGHT: {
                mHandleDrawable = mHandleRightDrawable;
                handleWidth = mHandleDrawable.getIntrinsicWidth();
                mHotspotX = handleWidth / (float) 4;
                break;
            }
        }

        mHandleHeight = mHandleDrawable.getIntrinsicHeight();

        mHandleWidth = handleWidth;
        mTouchOffsetY = -mHandleHeight * 0.3f;
        mHotspotY = 0;
        invalidate();
    }

    public void show() {
        if (!isPositionVisible()) {
            hide();
            return;
        }

        // We remove handle from its parent first otherwise the following exception may be thrown
        // java.lang.IllegalStateException: The specified child already has a parent. You must call removeView() on the child's parent first.
        removeFromParent();

        initHandle(); // init the handle
        invalidate(); // invalidate to make sure onDraw is called

        final int[] coords = mTempCoords;
        terminalView.getLocationInWindow(coords);
        coords[0] += mPointX;
        coords[1] += mPointY;

        if (mHandle != null)
            mHandle.showAtLocation(terminalView, 0, coords[0], coords[1]);
    }

    public void hide() {
        mIsDragging = false;

        if (mHandle != null) {
            mHandle.dismiss();

            // We remove handle from its parent, otherwise it may still be shown in some cases even after the dismiss call
            removeFromParent();
            mHandle = null;  // garbage collect the handle
        }
        invalidate();
    }

    public void removeFromParent() {
        if (!isParentNull()) {
            ((ViewGroup)this.getParent()).removeView(this);
        }
    }

    public void positionAtCursor(final int cx, final int cy, boolean forceOrientationCheck) {
        int x = terminalView.getPointX(cx);
        int y = terminalView.getPointY(cy + 1);
        moveTo(x, y, forceOrientationCheck);
    }

    private void moveTo(int x, int y, boolean forceOrientationCheck) {
        float oldHotspotX = mHotspotX;
        checkChangedOrientation(x, forceOrientationCheck);
        mPointX = (int) (x - (isShowing() ? oldHotspotX : mHotspotX));
        mPointY = y;

        if (isPositionVisible()) {
            int[] coords = null;

            if (isShowing()) {
                coords = mTempCoords;
                terminalView.getLocationInWindow(coords);
                int x1 = coords[0] + mPointX;
                int y1 = coords[1] + mPointY;
                if (mHandle != null)
                    mHandle.update(x1, y1, getWidth(), getHeight());
            } else {
                show();
            }

            if (mIsDragging) {
                if (coords == null) {
                    coords = mTempCoords;
                    terminalView.getLocationInWindow(coords);
                }
                if (coords[0] != mLastParentX || coords[1] != mLastParentY) {
                    mTouchToWindowOffsetX += coords[0] - mLastParentX;
                    mTouchToWindowOffsetY += coords[1] - mLastParentY;
                    mLastParentX = coords[0];
                    mLastParentY = coords[1];
                }
            }
        } else {
            hide();
        }
    }

    public void changeOrientation(int orientation) {
        if (mOrientation != orientation) {
            setOrientation(orientation);
        }
    }

    private void checkChangedOrientation(int posX, boolean force) {
        if (!mIsDragging && !force) {
            return;
        }
        long millis = SystemClock.currentThreadTimeMillis();
        if (millis - mLastTime < 50 && !force) {
            return;
        }
        mLastTime = millis;

        final TerminalView hostView = terminalView;
        final int left = hostView.getLeft();
        final int right = hostView.getWidth();
        final int top = hostView.getTop();
        final int bottom = hostView.getHeight();

        if (mTempRect == null) {
            mTempRect = new Rect();
        }
        final Rect clip = mTempRect;
        clip.left = left + terminalView.getPaddingLeft();
        clip.top = top + terminalView.getPaddingTop();
        clip.right = right - terminalView.getPaddingRight();
        clip.bottom = bottom - terminalView.getPaddingBottom();

        final ViewParent parent = hostView.getParent();
        if (parent == null || !parent.getChildVisibleRect(hostView, clip, null)) {
            return;
        }

        if (posX - mHandleWidth < clip.left) {
            changeOrientation(RIGHT);
        } else if (posX + mHandleWidth > clip.right) {
            changeOrientation(LEFT);
        } else {
            changeOrientation(mInitialOrientation);
        }
    }

    private boolean isPositionVisible() {
        // Always show a dragging handle.
        if (mIsDragging) {
            return true;
        }

        final TerminalView hostView = terminalView;
        final int left = 0;
        final int right = hostView.getWidth();
        final int top = 0;
        final int bottom = hostView.getHeight();

        if (mTempRect == null) {
            mTempRect = new Rect();
        }
        final Rect clip = mTempRect;
        clip.left = left + terminalView.getPaddingLeft();
        clip.top = top + terminalView.getPaddingTop();
        clip.right = right - terminalView.getPaddingRight();
        clip.bottom = bottom - terminalView.getPaddingBottom();

        final ViewParent parent = hostView.getParent();
        if (parent == null || !parent.getChildVisibleRect(hostView, clip, null)) {
            return false;
        }

        final int[] coords = mTempCoords;
        hostView.getLocationInWindow(coords);
        final int posX = coords[0] + mPointX + (int) mHotspotX;
        final int posY = coords[1] + mPointY + (int) mHotspotY;

        return posX >= clip.left && posX <= clip.right &&
            posY >= clip.top && posY <= clip.bottom;
    }

    @Override
    public void onDraw(Canvas c) {
        final int width = mHandleDrawable.getIntrinsicWidth();
        int height = mHandleDrawable.getIntrinsicHeight();
        mHandleDrawable.setBounds(0, 0, width, height);
        mHandleDrawable.draw(c);
    }

    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        terminalView.updateFloatingToolbarVisibility(event);
        switch (event.getActionMasked()) {
            case MotionEvent.ACTION_DOWN: {
                final float rawX = event.getRawX();
                final float rawY = event.getRawY();
                mTouchToWindowOffsetX = rawX - mPointX;
                mTouchToWindowOffsetY = rawY - mPointY;
                final int[] coords = mTempCoords;
                terminalView.getLocationInWindow(coords);
                mLastParentX = coords[0];
                mLastParentY = coords[1];
                mIsDragging = true;
                break;
            }

            case MotionEvent.ACTION_MOVE: {
                final float rawX = event.getRawX();
                final float rawY = event.getRawY();

                final float newPosX = rawX - mTouchToWindowOffsetX + mHotspotX;
                final float newPosY = rawY - mTouchToWindowOffsetY + mHotspotY + mTouchOffsetY;

                mCursorController.updatePosition(this, Math.round(newPosX), Math.round(newPosY));
                break;
            }

            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
                mIsDragging = false;
        }
        return true;
    }

    @Override
    public void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        setMeasuredDimension(mHandleDrawable.getIntrinsicWidth(),
            mHandleDrawable.getIntrinsicHeight());
    }

    public int getHandleHeight() {
        return mHandleHeight;
    }

    public int getHandleWidth() {
        return mHandleWidth;
    }

    public boolean isShowing() {
        if (mHandle != null)
            return mHandle.isShowing();
        else
            return false;
    }

    public boolean isParentNull() {
        return this.getParent() == null;
    }

    public boolean isDragging() {
        return mIsDragging;
    }

}
