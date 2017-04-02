package com.termux.window;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.ScaleGestureDetector.OnScaleGestureListener;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.LinearLayout;
import android.widget.Toast;

import com.termux.view.TerminalView;

public class TermuxFloatView extends LinearLayout {

    public static final float ALPHA_FOCUS = 0.9f;
    public static final float ALPHA_NOT_FOCUS = 0.7f;
    public static final float ALPHA_MOVING = 0.5f;

    private int DISPLAY_WIDTH, DISPLAY_HEIGHT;

    final WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
    WindowManager mWindowManager;
    InputMethodManager imm;

    TerminalView mTerminalView;

    /**
     * The last toast shown, used cancel current toast before showing new in {@link #showToast(String, boolean)}.
     */
    Toast mLastToast;

    private boolean withFocus = true;
    int initialX;
    int initialY;
    float initialTouchX;
    float initialTouchY;

    boolean isInLongPressState;

    final int[] location = new int[2];

    final ScaleGestureDetector mScaleDetector = new ScaleGestureDetector(getContext(), new OnScaleGestureListener() {
        private static final int MIN_SIZE = 50;

        @Override
        public boolean onScaleBegin(ScaleGestureDetector detector) {
            return true;
        }

        @Override
        public boolean onScale(ScaleGestureDetector detector) {
            int widthChange = (int) (detector.getCurrentSpanX() - detector.getPreviousSpanX());
            int heightChange = (int) (detector.getCurrentSpanY() - detector.getPreviousSpanY());
            layoutParams.width += widthChange;
            layoutParams.height += heightChange;
            layoutParams.width = Math.max(MIN_SIZE, layoutParams.width);
            layoutParams.height = Math.max(MIN_SIZE, layoutParams.height);
            mWindowManager.updateViewLayout(TermuxFloatView.this, layoutParams);
            TermuxFloatPrefs.saveWindowSize(getContext(), layoutParams.width, layoutParams.height);
            return true;
        }

        @Override
        public void onScaleEnd(ScaleGestureDetector detector) {
            // Do nothing.
        }
    });

    public TermuxFloatView(Context context, AttributeSet attrs) {
        super(context, attrs);
        setAlpha(ALPHA_FOCUS);
    }

    private static int computeLayoutFlags(boolean withFocus) {
        if (withFocus) {
            return 0;
        } else {
            return WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
        }
    }

    public void initializeFloatingWindow() {
        mTerminalView = (TerminalView) findViewById(R.id.terminal_view);
        mTerminalView.setOnKeyListener(new TermuxFloatKeyListener(this));
    }

    @Override
    protected void onAttachedToWindow() {
        super.onAttachedToWindow();

        Point displaySize = new Point();
        getDisplay().getSize(displaySize);
        DISPLAY_WIDTH = displaySize.x;
        DISPLAY_HEIGHT = displaySize.y;

        // mTerminalView.checkForFontAndColors();
    }

    @SuppressLint("RtlHardcoded")
    public void launchFloatingWindow() {
        int widthAndHeight = android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
        layoutParams.flags = computeLayoutFlags(true);
        layoutParams.width = widthAndHeight;
        layoutParams.height = widthAndHeight;
        layoutParams.type = WindowManager.LayoutParams.TYPE_PHONE;
        layoutParams.format = PixelFormat.RGBA_8888;

        layoutParams.gravity = Gravity.TOP | Gravity.LEFT;
        TermuxFloatPrefs.applySavedGeometry(getContext(), layoutParams);

        mWindowManager = (WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE);
        mWindowManager.addView(this, layoutParams);
        imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
        showTouchKeyboard();
    }

    /**
     * Intercept touch events to obtain and loose focus on touch events.
     */
    @Override
    public boolean onInterceptTouchEvent(MotionEvent event) {
        if (isInLongPressState) return true;

        getLocationOnScreen(location);
        int x = layoutParams.x; // location[0];
        int y = layoutParams.y; // location[1];
        float touchX = event.getRawX();
        float touchY = event.getRawY();
        boolean clickedInside = (touchX >= x) && (touchX <= (x + layoutParams.width)) && (touchY >= y) && (touchY <= (y + layoutParams.height));

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                if (!clickedInside) changeFocus(false);
                break;
            case MotionEvent.ACTION_UP:
                if (clickedInside) {
                    changeFocus(true);
                    showTouchKeyboard();
                }
                break;
        }
        return false;
    }

    void showTouchKeyboard() {
        mTerminalView.post(new Runnable() {
            @Override
            public void run() {
                imm.showSoftInput(mTerminalView, InputMethodManager.SHOW_IMPLICIT);
            }
        });
    }

    void updateLongPressMode(boolean newValue) {
        isInLongPressState = newValue;
        setBackgroundResource(newValue ? R.drawable.floating_window_background_resize : R.drawable.floating_window_background);
        setAlpha(newValue ? ALPHA_MOVING : (withFocus ? ALPHA_FOCUS : ALPHA_NOT_FOCUS));
        if (newValue) {
            Toast toast = Toast.makeText(getContext(), R.string.after_long_press, Toast.LENGTH_SHORT);
            toast.setGravity(Gravity.CENTER, 0, 0);
            toast.show();
        }
    }

    /**
     * Motion events should only be dispatched here when {@link #onInterceptTouchEvent(MotionEvent)} returns true.
     */
    @SuppressLint("ClickableViewAccessibility")
    @Override
    public boolean onTouchEvent(MotionEvent event) {
        if (isInLongPressState) {
            mScaleDetector.onTouchEvent(event);
            if (mScaleDetector.isInProgress()) return true;
            switch (event.getAction()) {
                case MotionEvent.ACTION_MOVE:
                    layoutParams.x = Math.min(DISPLAY_WIDTH - layoutParams.width, Math.max(0, initialX + (int) (event.getRawX() - initialTouchX)));
                    layoutParams.y = Math.min(DISPLAY_HEIGHT - layoutParams.height, Math.max(0, initialY + (int) (event.getRawY() - initialTouchY)));
                    mWindowManager.updateViewLayout(TermuxFloatView.this, layoutParams);
                    TermuxFloatPrefs.saveWindowPosition(getContext(), layoutParams.x, layoutParams.y);
                    break;
                case MotionEvent.ACTION_UP:
                    updateLongPressMode(false);
                    break;
            }
            return true;
        }
        return super.onTouchEvent(event);
    }

    /**
     * Visually indicate focus and show the soft input as needed.
     */
    void changeFocus(boolean newFocus) {
        if (newFocus == withFocus) {
            if (newFocus) showTouchKeyboard();
            return;
        }
        withFocus = newFocus;
        layoutParams.flags = computeLayoutFlags(withFocus);
        mWindowManager.updateViewLayout(this, layoutParams);
        setAlpha(newFocus ? ALPHA_FOCUS : ALPHA_NOT_FOCUS);
    }

    /**
     * Show a toast and dismiss the last one if still visible.
     */
    void showToast(String text, boolean longDuration) {
        if (mLastToast != null) mLastToast.cancel();
        mLastToast = Toast.makeText(getContext(), text, longDuration ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT);
        mLastToast.setGravity(Gravity.TOP, 0, 0);
        mLastToast.show();
    }

    public void closeFloatingWindow() {
        mWindowManager.removeView(this);
    }

}
