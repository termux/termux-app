package com.termux.window;

import com.termux.terminal.TerminalSession;
import com.termux.view.TerminalKeyListener;
import com.termux.view.TerminalView;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.PixelFormat;
import android.graphics.Point;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.view.ScaleGestureDetector.OnScaleGestureListener;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.LinearLayout;
import android.widget.Toast;

public class TermuxFloatView extends LinearLayout {

	public static final float ALPHA_FOCUS = 0.9f;
	public static final float ALPHA_NOT_FOCUS = 0.7f;
	public static final float ALPHA_MOVING = 0.5f;

	private int DISPLAY_WIDTH, DISPLAY_HEIGHT;

	final WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
	WindowManager mWindowManager;
	InputMethodManager imm;

	TerminalView mTerminalView;

	/** The last toast shown, used cancel current toast before showing new in {@link #showToast(String, boolean)}. */
	Toast mLastToast;

	private boolean withFocus = true;
	private int initialX;
	private int initialY;
	private float initialTouchX;
	private float initialTouchY;

	boolean isInLongPressState;

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

		mTerminalView.setOnKeyListener(new TerminalKeyListener() {
			@Override
			public float onScale(float scale) {
				if (scale < 0.9f || scale > 1.1f) {
					boolean increase = scale > 1.f;
					((TermuxFloatService) getContext()).changeFontSize(increase);
					return 1.0f;
				}
				return scale;
			}

			@Override
			public boolean onLongPress(MotionEvent event) {
				updateLongPressMode(true);
				initialX = layoutParams.x;
				initialY = layoutParams.y;
				initialTouchX = event.getRawX();
				initialTouchY = event.getRawY();
                return true;
			}

            @Override
            public void onSingleTapUp(MotionEvent e) {
                // Do nothing.
            }

			@Override
			public boolean shouldBackButtonBeMappedToEscape() {
				return true;
			}

			@Override
			public void copyModeChanged(boolean copyMode) {

			}

			@Override
			public boolean onKeyDown(int keyCode, KeyEvent e, TerminalSession session) {
				return false;
			}

			@Override
			public boolean onKeyUp(int keyCode, KeyEvent e) {
				return false;
			}

			@Override
			public boolean readControlKey() {
				return false;
			}

			@Override
			public boolean readAltKey() {
				return false;
			}

			@Override
			public boolean onCodePoint(int codePoint, boolean ctrlDown, TerminalSession session) {
				return false;
			}

		});
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

	/** Intercept touch events to obtain and loose focus on touch events. */
	@Override
	public boolean onInterceptTouchEvent(MotionEvent event) {
		if (isInLongPressState) return true;
		if (event.getAction() == MotionEvent.ACTION_DOWN) {
			if ((event.getMetaState() & (KeyEvent.META_CTRL_ON | KeyEvent.META_ALT_ON)) != 0) {
				updateLongPressMode(true);
				initialX = layoutParams.x;
				initialY = layoutParams.y;
				initialTouchX = event.getRawX();
				initialTouchY = event.getRawY();
				return true;
			}
			// FIXME: params.x and params.y are outdated when snapping to end of screen, where the movement stops but x
			// and y are wrong.
			float touchX = event.getRawX();
			float touchY = event.getRawY();
			boolean clickedInside = (touchX >= layoutParams.x) && (touchX <= (layoutParams.x + layoutParams.width)) && (touchY >= layoutParams.y)
					&& (touchY <= (layoutParams.y + layoutParams.height));
			if (withFocus != clickedInside) {
				changeFocus(clickedInside);
			} else if (clickedInside) {
				// When clicking inside, show keyboard if the user has hidden it:
				showTouchKeyboard();
			}
		}
		return false;
	}

	private void showTouchKeyboard() {
		mTerminalView.post(new Runnable() {
			@Override
			public void run() {
				imm.showSoftInput(mTerminalView, InputMethodManager.SHOW_IMPLICIT);
			}
		});
	}

	private void updateLongPressMode(boolean newValue) {
		isInLongPressState = newValue;
		setBackgroundResource(newValue ? R.drawable.floating_window_background_resize : R.drawable.floating_window_background);
		setAlpha(newValue ? ALPHA_MOVING : ALPHA_FOCUS);
		if (newValue) {
			Toast toast = Toast.makeText(getContext(), R.string.after_long_press, Toast.LENGTH_SHORT);
			toast.setGravity(Gravity.CENTER, 0, 0);
			toast.show();
		}
	}

	/** Motion events should only be dispatched here when {@link #onInterceptTouchEvent(MotionEvent)} returns true. */
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

	/** Visually indicate focus and show the soft input as needed. */
	private void changeFocus(boolean newFocus) {
		withFocus = newFocus;
		layoutParams.flags = computeLayoutFlags(withFocus);
		mWindowManager.updateViewLayout(this, layoutParams);
		setAlpha(newFocus ? ALPHA_FOCUS : ALPHA_NOT_FOCUS);
		if (newFocus) showTouchKeyboard();
	}

	/** Show a toast and dismiss the last one if still visible. */
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
