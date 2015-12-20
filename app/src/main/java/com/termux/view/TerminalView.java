package com.termux.view;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Properties;

import com.termux.terminal.EmulatorDebug;
import com.termux.terminal.KeyHandler;
import com.termux.terminal.TerminalColors;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalSession;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Typeface;
import android.text.InputType;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.HapticFeedbackConstants;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.Scroller;

/** View displaying and interacting with a {@link TerminalSession}. */
public final class TerminalView extends View {

	/** Log view key and IME events. */
	private static final boolean LOG_KEY_EVENTS = false;

	/** The currently displayed terminal session, whose emulator is {@link #mEmulator}. */
	TerminalSession mTermSession;
	/** Our terminal emulator whose session is {@link #mTermSession}. */
	TerminalEmulator mEmulator;

	TerminalRenderer mRenderer;

	TerminalKeyListener mOnKeyListener;

	/** The top row of text to display. Ranges from -activeTranscriptRows to 0. */
	int mTopRow;

	/** Keeping track of the special keys acting as Ctrl and Fn for the soft keyboard and other hardware keys. */
	boolean mVirtualControlKeyDown, mVirtualFnKeyDown;

	boolean mIsSelectingText = false;
	int mSelXAnchor = -1, mSelYAnchor = -1;
	int mSelX1 = -1, mSelX2 = -1, mSelY1 = -1, mSelY2 = -1;

	float mScaleFactor = 1.f;
	final GestureAndScaleRecognizer mGestureRecognizer;

	/** Keep track of where mouse touch event started which we report as mouse scroll. */
	private int mMouseScrollStartX = -1, mMouseScrollStartY = -1;
	/** Keep track of the time when a touch event leading to sending mouse scroll events started. */
	private long mMouseStartDownTime = -1;

	final Scroller mScroller;

	/** What was left in from scrolling movement. */
	float mScrollRemainder;

	/** If non-zero, this is the last unicode code point received if that was a combining character. */
	int mCombiningAccent;

	public TerminalView(Context context, AttributeSet attributes) { // NO_UCD (unused code)
		super(context, attributes);
		mGestureRecognizer = new GestureAndScaleRecognizer(context, new GestureAndScaleRecognizer.Listener() {

			@Override
			public boolean onUp(MotionEvent e) {
				mScrollRemainder = 0.0f;
				if (mEmulator != null && mEmulator.isMouseTrackingActive()) {
					// Quick event processing when mouse tracking is active - do not wait for check of double tapping
					// for zooming.
					sendMouseEventCode(e, TerminalEmulator.MOUSE_LEFT_BUTTON, true);
					sendMouseEventCode(e, TerminalEmulator.MOUSE_LEFT_BUTTON, false);
					return true;
				}
				return false;
			}

			@Override
			public boolean onSingleTapUp(MotionEvent e) {
				if (mEmulator == null) return true;
				requestFocus();
				if (!mEmulator.isMouseTrackingActive()) {
					if (!e.isFromSource(InputDevice.SOURCE_MOUSE)) {
						mOnKeyListener.onSingleTapUp(e);
						return true;
					}
				}
				return false;
			}

			@Override
			public boolean onScroll(MotionEvent e2, float distanceX, float distanceY) {
				if (mEmulator == null) return true;
				if (mEmulator.isMouseTrackingActive() && e2.isFromSource(InputDevice.SOURCE_MOUSE)) {
					// If moving with mouse pointer while pressing button, report that instead of scroll.
					// This means that we never report moving with button press-events for touch input,
					// since we cannot just start sending these events without a starting press event,
					// which we do not do for touch input, only mouse in onTouchEvent().
					sendMouseEventCode(e2, TerminalEmulator.MOUSE_LEFT_BUTTON_MOVED, true);
				} else {
					distanceY += mScrollRemainder;
					int deltaRows = (int) (distanceY / mRenderer.mFontLineSpacing);
					mScrollRemainder = distanceY - deltaRows * mRenderer.mFontLineSpacing;
					doScroll(e2, deltaRows);
				}
				return true;
			}

			@Override
			public boolean onScale(float focusX, float focusY, float scale) {
				mScaleFactor *= scale;
				mScaleFactor = mOnKeyListener.onScale(mScaleFactor);
				return true;
			}

			@Override
			public boolean onFling(final MotionEvent e2, float velocityX, float velocityY) {
				if (mEmulator == null) return true;
				// Do not start scrolling until last fling has been taken care of:
				if (!mScroller.isFinished()) return true;

				final boolean mouseTrackingAtStartOfFling = mEmulator.isMouseTrackingActive();
				float SCALE = 0.25f;
				if (mouseTrackingAtStartOfFling) {
					mScroller.fling(0, 0, 0, -(int) (velocityY * SCALE), 0, 0, -mEmulator.mRows / 2, mEmulator.mRows / 2);
				} else {
					mScroller.fling(0, mTopRow, 0, -(int) (velocityY * SCALE), 0, 0, -mEmulator.getScreen().getActiveTranscriptRows(), 0);
				}

				post(new Runnable() {
					private int mLastY = 0;

					@Override
					public void run() {
						if (mouseTrackingAtStartOfFling != mEmulator.isMouseTrackingActive()) {
							mScroller.abortAnimation();
							return;
						}
						if (mScroller.isFinished()) return;
						boolean more = mScroller.computeScrollOffset();
						int newY = mScroller.getCurrY();
						int diff = mouseTrackingAtStartOfFling ? (newY - mLastY) : (newY - mTopRow);
						doScroll(e2, diff);
						mLastY = newY;
						if (more) post(this);
					}
				});

				return true;
			}

			@Override
			public boolean onDown(float x, float y) {
				return false;
			}

			@Override
			public boolean onDoubleTap(MotionEvent e) {
				// Do not treat is as a single confirmed tap - it may be followed by zoom.
				return false;
			}

			@Override
			public void onLongPress(MotionEvent e) {
				if (mEmulator != null && !mGestureRecognizer.isInProgress()) {
					performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
					mOnKeyListener.onLongPress(e);
				}
			}
		});
		mScroller = new Scroller(context);
	}

	/**
	 * @param onKeyListener
	 *            Listener for all kinds of key events, both hardware and IME (which makes it different from that
	 *            available with {@link View#setOnKeyListener(OnKeyListener)}.
	 */
	public void setOnKeyListener(TerminalKeyListener onKeyListener) {
		this.mOnKeyListener = onKeyListener;
	}

	/**
	 * Attach a {@link TerminalSession} to this view.
	 * 
	 * @param session
	 *            The {@link TerminalSession} this view will be displaying.
	 */
	public boolean attachSession(TerminalSession session) {
		if (session == mTermSession) return false;
		mTopRow = 0;

		mTermSession = session;
		mEmulator = null;
		mCombiningAccent = 0;

		updateSize();

		// Wait with enabling the scrollbar until we have a terminal to get scroll position from.
		setVerticalScrollBarEnabled(true);

		return true;
	}

	@Override
	public InputConnection onCreateInputConnection(EditorInfo outAttrs) {
		// Make the IME run in a limited "generate key events" mode.
		//
		// If using just "TYPE_NULL", there is a problem with the "Google Pinyin Input" being in
		// word mode when used with the "En" tab available when the "Show English keyboard" option
		// is enabled - see https://github.com/termux/termux-packages/issues/25.
		//
		// Adding TYPE_TEXT_FLAG_NO_SUGGESTIONS fixes Pinyin Input, put causes Swype to be put in
		// word mode... Using TYPE_TEXT_VARIATION_VISIBLE_PASSWORD fixes that.
		//
		// So a bit messy. If this gets too messy it's perhaps best resolved by reverting back to just
		// "TYPE_NULL" and let the Pinyin Input english keyboard be in word mode.
		outAttrs.inputType = InputType.TYPE_NULL | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS | InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD;

		// Let part of the application show behind when in landscape:
		outAttrs.imeOptions |= EditorInfo.IME_FLAG_NO_FULLSCREEN;

		return new BaseInputConnection(this, true) {

			@Override
			public boolean beginBatchEdit() {
				if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "IME: beginBatchEdit()");
				return true;
			}

			@Override
			public boolean clearMetaKeyStates(int states) {
				if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "IME: clearMetaKeyStates(" + states + ")");
				return true;
			}

			@Override
			public boolean endBatchEdit() {
				if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "IME: endBatchEdit()");
				return false;
			}

			@Override
			public boolean finishComposingText() {
				if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "IME: finishComposingText()");
				return true;
			}

			@Override
			public int getCursorCapsMode(int reqModes) {
				if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "IME: getCursorCapsMode(" + reqModes + ")");
				int mode = 0;
				if ((reqModes & TextUtils.CAP_MODE_CHARACTERS) != 0) {
					mode |= TextUtils.CAP_MODE_CHARACTERS;
				}
				return mode;
			}

			@Override
			public CharSequence getTextAfterCursor(int n, int flags) {
				return "";
			}

			@Override
			public CharSequence getTextBeforeCursor(int n, int flags) {
				return "";
			}

			@Override
			public boolean commitText(CharSequence text, int newCursorPosition) {
				if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "IME: commitText(\"" + text + "\", " + newCursorPosition + ")");
				if (mEmulator == null) return true;
				final int textLengthInChars = text.length();
				for (int i = 0; i < textLengthInChars; i++) {
					char firstChar = text.charAt(i);
					int codePoint;
					if (Character.isHighSurrogate(firstChar)) {
						if (++i < textLengthInChars) {
							codePoint = Character.toCodePoint(firstChar, text.charAt(i));
						} else {
							// At end of string, with no low surrogate following the high:
							codePoint = TerminalEmulator.UNICODE_REPLACEMENT_CHAR;
						}
					} else {
						codePoint = firstChar;
					}
					inputCodePoint(codePoint, false, false);
				}
				return true;
			}

			@Override
			public boolean deleteSurroundingText(int leftLength, int rightLength) {
				if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "IME: deleteSurroundingText(" + leftLength + ", " + rightLength + ")");

				// Swype keyboard sometimes(?) sends this on backspace:
				if (leftLength == 0 && rightLength == 0) leftLength = 1;

				for (int i = 0; i < leftLength; i++)
					sendKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DEL));
				return true;
			}

		};
	}

	@Override
	protected int computeVerticalScrollRange() {
		return mEmulator == null ? 1 : mEmulator.getScreen().getActiveRows();
	}

	@Override
	protected int computeVerticalScrollExtent() {
		return mEmulator == null ? 1 : mEmulator.mRows;
	}

	@Override
	protected int computeVerticalScrollOffset() {
		return mEmulator == null ? 1 : mEmulator.getScreen().getActiveRows() + mTopRow - mEmulator.mRows;
	}

	public void onScreenUpdated() {
		if (mEmulator == null) return;

		if (mIsSelectingText) {
			int rowShift = mEmulator.getScrollCounter();
			mSelY1 -= rowShift;
			mSelY2 -= rowShift;
			mSelYAnchor -= rowShift;
		}
		mEmulator.clearScrollCounter();

		if (mTopRow != 0) {
			// Scroll down if not already there.
			mTopRow = 0;
			scrollTo(0, 0);
		}

		invalidate();
	}

	/**
	 * Sets the text size, which in turn sets the number of rows and columns.
	 * 
	 * @param textSize
	 *            the new font size, in density-independent pixels.
	 */
	public void setTextSize(int textSize) {
		mRenderer = new TerminalRenderer(textSize, mRenderer == null ? Typeface.MONOSPACE : mRenderer.mTypeface);
		updateSize();
	}

	@Override
	public boolean onCheckIsTextEditor() {
		return true;
	}

	@Override
	public boolean isOpaque() {
		return true;
	}

	/** Send a single mouse event code to the terminal. */
	void sendMouseEventCode(MotionEvent e, int button, boolean pressed) {
		int x = (int) (e.getX() / mRenderer.mFontWidth) + 1;
		int y = (int) ((e.getY() - mRenderer.mFontLineSpacingAndAscent) / mRenderer.mFontLineSpacing) + 1;
		if (pressed && (button == TerminalEmulator.MOUSE_WHEELDOWN_BUTTON || button == TerminalEmulator.MOUSE_WHEELUP_BUTTON)) {
			if (mMouseStartDownTime == e.getDownTime()) {
				x = mMouseScrollStartX;
				y = mMouseScrollStartY;
			} else {
				mMouseStartDownTime = e.getDownTime();
				mMouseScrollStartX = x;
				mMouseScrollStartY = y;
			}
		}
		mEmulator.sendMouseEvent(button, x, y, pressed);
	}

	/** Perform a scroll, either from dragging the screen or by scrolling a mouse wheel. */
	void doScroll(MotionEvent event, int rowsDown) {
		boolean up = rowsDown < 0;
		int amount = Math.abs(rowsDown);
		for (int i = 0; i < amount; i++) {
			if (mEmulator.isMouseTrackingActive()) {
				sendMouseEventCode(event, up ? TerminalEmulator.MOUSE_WHEELUP_BUTTON : TerminalEmulator.MOUSE_WHEELDOWN_BUTTON, true);
			} else if (mEmulator.isAlternateBufferActive()) {
				// Send up and down key events for scrolling, which is what some terminals do to make scroll work in
				// e.g. less, which shifts to the alt screen without mouse handling.
				handleKeyCode(up ? KeyEvent.KEYCODE_DPAD_UP : KeyEvent.KEYCODE_DPAD_DOWN, 0);
			} else {
				mTopRow = Math.min(0, Math.max(-(mEmulator.getScreen().getActiveTranscriptRows()), mTopRow + (up ? -1 : 1)));
				if (!awakenScrollBars()) invalidate();
			}
		}
	}

	/** Overriding {@link View#onGenericMotionEvent(MotionEvent)}. */
	@Override
	public boolean onGenericMotionEvent(MotionEvent event) {
		if (mEmulator != null && event.isFromSource(InputDevice.SOURCE_MOUSE) && event.getAction() == MotionEvent.ACTION_SCROLL) {
			// Handle mouse wheel scrolling.
			boolean up = event.getAxisValue(MotionEvent.AXIS_VSCROLL) > 0.0f;
			doScroll(event, up ? -3 : 3);
			return true;
		}
		return false;
	}

	@SuppressLint("ClickableViewAccessibility")
	@Override
	public boolean onTouchEvent(MotionEvent ev) {
		if (mEmulator == null) return true;
		final boolean eventFromMouse = ev.isFromSource(InputDevice.SOURCE_MOUSE);
		final int action = ev.getAction();

		if (eventFromMouse) {
			if ((ev.getButtonState() & MotionEvent.BUTTON_SECONDARY) != 0) {
				if (action == MotionEvent.ACTION_DOWN) showContextMenu();
				return true;
			} else if (mEmulator.isMouseTrackingActive() && (ev.getAction() == MotionEvent.ACTION_DOWN || ev.getAction() == MotionEvent.ACTION_UP)) {
				sendMouseEventCode(ev, TerminalEmulator.MOUSE_LEFT_BUTTON, ev.getAction() == MotionEvent.ACTION_DOWN);
				return true;
			} else if (!mEmulator.isMouseTrackingActive() && action == MotionEvent.ACTION_DOWN) {
				// Start text selection with mouse. Note that the check against MotionEvent.ACTION_DOWN is
				// important, since we otherwise would pick up secondary mouse button up actions.
				mIsSelectingText = true;
			}
		} else if (!mIsSelectingText) {
			mGestureRecognizer.onTouchEvent(ev);
			return true;
		}

		if (mIsSelectingText) {
			int cx = (int) (ev.getX() / mRenderer.mFontWidth);
			// Offset for finger:
			final int SELECT_TEXT_OFFSET_Y = eventFromMouse ? 0 : -40;
			int cy = (int) ((ev.getY() + SELECT_TEXT_OFFSET_Y) / mRenderer.mFontLineSpacing) + mTopRow;
			switch (action) {
			case MotionEvent.ACTION_DOWN:
				mSelXAnchor = cx;
				mSelYAnchor = cy;
				mSelX1 = cx;
				mSelY1 = cy;
				mSelX2 = mSelX1;
				mSelY2 = mSelY1;
				invalidate();
				break;
			case MotionEvent.ACTION_MOVE:
			case MotionEvent.ACTION_UP:
				boolean touchBeforeAnchor = (cy < mSelYAnchor || (cy == mSelYAnchor && cx < mSelXAnchor));
				int minx = touchBeforeAnchor ? cx : mSelXAnchor;
				int maxx = !touchBeforeAnchor ? cx : mSelXAnchor;
				int miny = touchBeforeAnchor ? cy : mSelYAnchor;
				int maxy = !touchBeforeAnchor ? cy : mSelYAnchor;
				mSelX1 = minx;
				mSelY1 = miny;
				mSelX2 = maxx;
				mSelY2 = maxy;
				if (action == MotionEvent.ACTION_UP) {
					String selectedText = mEmulator.getSelectedText(mSelX1, mSelY1, mSelX2, mSelY2).trim();
					mTermSession.clipboardText(selectedText);
					toggleSelectingText();
				}
				invalidate();
				break;
			default:
				toggleSelectingText();
				invalidate();
				break;
			}
			return true;
		}

		return false;
	}

	@Override
	public boolean onKeyPreIme(int keyCode, KeyEvent event) {
		if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "onKeyPreIme(keyCode=" + keyCode + ", event=" + event + ")");
		if (keyCode == KeyEvent.KEYCODE_ESCAPE || keyCode == KeyEvent.KEYCODE_BACK) {
			// Handle the escape key ourselves to avoid the system from treating it as back key
			// and e.g. close keyboard.
			switch (event.getAction()) {
			case KeyEvent.ACTION_DOWN:
				return onKeyDown(keyCode, event);
			case KeyEvent.ACTION_UP:
				return onKeyUp(keyCode, event);
			}
		}
		return super.onKeyPreIme(keyCode, event);
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "onKeyDown(keyCode=" + keyCode + ", isSystem()=" + event.isSystem() + ", event=" + event + ")");
		if (mEmulator == null) return true;

		int metaState = event.getMetaState();
		boolean controlDownFromEvent = event.isCtrlPressed();
		boolean leftAltDownFromEvent = (metaState & KeyEvent.META_ALT_LEFT_ON) != 0;
		boolean rightAltDownFromEvent = (metaState & KeyEvent.META_ALT_RIGHT_ON) != 0;

		if (handleVirtualKeys(keyCode, event, true)) {
			invalidate();
			return true;
		} else if (event.isSystem() && keyCode != KeyEvent.KEYCODE_BACK) {
			return super.onKeyDown(keyCode, event);
		}

		int keyMod = 0;
		if (controlDownFromEvent) keyMod |= KeyHandler.KEYMOD_CTRL;
		if (event.isAltPressed()) keyMod |= KeyHandler.KEYMOD_ALT;
		if (event.isShiftPressed()) keyMod |= KeyHandler.KEYMOD_SHIFT;
		if (handleKeyCode(keyCode, keyMod)) {
			if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "handleKeyCode() took key event");
			return true;
		}

		// Clear Ctrl since we handle that ourselves:
		int bitsToClear = KeyEvent.META_CTRL_MASK;
		if (rightAltDownFromEvent) {
			// Let right Alt/Alt Gr be used to compose characters.
		} else {
			// Use left alt to send to terminal (e.g. Left Alt+B to jump back a word), so remove:
			bitsToClear |= KeyEvent.META_ALT_ON | KeyEvent.META_ALT_LEFT_ON;
		}
		int effectiveMetaState = event.getMetaState() & ~bitsToClear;

		int result = event.getUnicodeChar(effectiveMetaState);
		if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "KeyEvent#getUnicodeChar(" + effectiveMetaState + ") returned: " + result);
		if (result == 0) {
			return true;
		}

		int oldCombiningAccent = mCombiningAccent;
		if ((result & KeyCharacterMap.COMBINING_ACCENT) != 0) {
			// If entered combining accent previously, write it out:
			if (mCombiningAccent != 0) inputCodePoint(mCombiningAccent, controlDownFromEvent, leftAltDownFromEvent);
			mCombiningAccent = result & KeyCharacterMap.COMBINING_ACCENT_MASK;
		} else {
			if (mCombiningAccent != 0) {
				int combinedChar = KeyCharacterMap.getDeadChar(mCombiningAccent, result);
				if (combinedChar > 0) result = combinedChar;
				mCombiningAccent = 0;
			}
			inputCodePoint(result, controlDownFromEvent, leftAltDownFromEvent);
		}

		if (mCombiningAccent != oldCombiningAccent) invalidate();

		return true;
	}

	void inputCodePoint(int codePoint, boolean controlDownFromEvent, boolean leftAltDownFromEvent) {
		if (LOG_KEY_EVENTS) {
			Log.i(EmulatorDebug.LOG_TAG, "inputCodePoint(codePoint=" + codePoint + ", controlDownFromEvent=" + controlDownFromEvent + ", leftAltDownFromEvent="
					+ leftAltDownFromEvent + ")");
		}

		int resultingKeyCode = -1; // Set if virtual key causes this to be translated to key event.
		if (controlDownFromEvent || mVirtualControlKeyDown) {
			if (codePoint >= 'a' && codePoint <= 'z') {
				codePoint = codePoint - 'a' + 1;
			} else if (codePoint >= 'A' && codePoint <= 'Z') {
				codePoint = codePoint - 'A' + 1;
			} else if (codePoint == ' ' || codePoint == '2') {
				codePoint = 0;
			} else if (codePoint == '[' || codePoint == '3') {
				codePoint = 27; // ^[ (Esc)
			} else if (codePoint == '\\' || codePoint == '4') {
				codePoint = 28;
			} else if (codePoint == ']' || codePoint == '5') {
				codePoint = 29;
			} else if (codePoint == '^' || codePoint == '6') {
				codePoint = 30; // control-^
			} else if (codePoint == '_' || codePoint == '7') {
				codePoint = 31;
			} else if (codePoint == '8') {
				codePoint = 127; // DEL
			} else if (codePoint == '9') {
				resultingKeyCode = KeyEvent.KEYCODE_F11;
			} else if (codePoint == '0') {
				resultingKeyCode = KeyEvent.KEYCODE_F12;
			}
		} else if (mVirtualFnKeyDown) {
			if (codePoint == 'w' || codePoint == 'W') {
				resultingKeyCode = KeyEvent.KEYCODE_DPAD_UP;
			} else if (codePoint == 'a' || codePoint == 'A') {
				resultingKeyCode = KeyEvent.KEYCODE_DPAD_LEFT;
			} else if (codePoint == 's' || codePoint == 'S') {
				resultingKeyCode = KeyEvent.KEYCODE_DPAD_DOWN;
			} else if (codePoint == 'd' || codePoint == 'D') {
				resultingKeyCode = KeyEvent.KEYCODE_DPAD_RIGHT;
			} else if (codePoint == 'p' || codePoint == 'P') {
				resultingKeyCode = KeyEvent.KEYCODE_PAGE_UP;
			} else if (codePoint == 'n' || codePoint == 'N') {
				resultingKeyCode = KeyEvent.KEYCODE_PAGE_DOWN;
			} else if (codePoint == 't' || codePoint == 'T') {
				resultingKeyCode = KeyEvent.KEYCODE_TAB;
			} else if (codePoint == 'l' || codePoint == 'L') {
				codePoint = '|';
			} else if (codePoint == 'u' || codePoint == 'U') {
				codePoint = '_';
			} else if (codePoint == 'e' || codePoint == 'E') {
				codePoint = 27; // ^[ (Esc)
			} else if (codePoint == '.') {
				codePoint = 28; // ^\
			} else if (codePoint > '0' && codePoint <= '9') {
				// F1-F9
				resultingKeyCode = (codePoint - '1') + KeyEvent.KEYCODE_F1;
			} else if (codePoint == '0') {
				resultingKeyCode = KeyEvent.KEYCODE_F10;
			} else if (codePoint == 'i' || codePoint == 'I') {
				resultingKeyCode = KeyEvent.KEYCODE_INSERT;
			} else if (codePoint == 'x' || codePoint == 'X') {
				resultingKeyCode = KeyEvent.KEYCODE_FORWARD_DEL;
			} else if (codePoint == 'h' || codePoint == 'H') {
				resultingKeyCode = KeyEvent.KEYCODE_MOVE_HOME;
			} else if (codePoint == 'f' || codePoint == 'F') {
				// As left alt+f, jumping forward in readline:
				codePoint = 'f';
				leftAltDownFromEvent = true;
			} else if (codePoint == 'b' || codePoint == 'B') {
				// As left alt+b, jumping forward in readline:
				codePoint = 'b';
				leftAltDownFromEvent = true;
			}
		}

		if (codePoint > -1) {
			if (resultingKeyCode > -1) {
				handleKeyCode(resultingKeyCode, 0);
			} else {
				// The below two workarounds are needed on at least Logitech Keyboard k810 on Samsung Galaxy Tab Pro
				// (Android 4.4) with the stock Samsung Keyboard. They should be harmless when not used since the need
				// to input the original characters instead of the new ones using the keyboard should be low.
				// Rewrite U+02DC 'SMALL TILDE' to U+007E 'TILDE' for ~ to work in shells:
				if (codePoint == 0x02DC) codePoint = 0x07E;
				// Rewrite U+02CB 'MODIFIER LETTER GRAVE ACCENT' to U+0060 'GRAVE ACCENT' for ` (backticks) to work:
				if (codePoint == 0x02CB) codePoint = 0x60;

				// If left alt, send escape before the code point to make e.g. Alt+B and Alt+F work in readline:
				mTermSession.writeCodePoint(leftAltDownFromEvent, codePoint);
			}
		}
	}

	/** Input the specified keyCode if applicable and return if the input was consumed. */
	public boolean handleKeyCode(int keyCode, int keyMod) {
		TerminalEmulator term = mTermSession.getEmulator();
		String code = KeyHandler.getCode(keyCode, keyMod, term.isCursorKeysApplicationMode(), term.isKeypadApplicationMode());
		if (code == null) return false;
		mTermSession.write(code);
		return true;
	}

	/**
	 * Called when a key is released in the view.
	 * 
	 * @param keyCode
	 *            The keycode of the key which was released.
	 * @param event
	 *            A {@link KeyEvent} describing the event.
	 * @return Whether the event was handled.
	 */
	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "onKeyUp(keyCode=" + keyCode + ", event=" + event + ")");
		if (mEmulator == null) return true;

		if (handleVirtualKeys(keyCode, event, false)) {
			invalidate();
			return true;
		} else if (event.isSystem()) {
			// Let system key events through.
			return super.onKeyUp(keyCode, event);
		}

		return true;
	}

	/** Handle dedicated volume buttons as virtual keys if applicable. */
	private boolean handleVirtualKeys(int keyCode, KeyEvent event, boolean down) {
		InputDevice inputDevice = event.getDevice();
		if (inputDevice != null && inputDevice.getKeyboardType() == InputDevice.KEYBOARD_TYPE_ALPHABETIC) {
			// Do not steal dedicated buttons from a full external keyboard.
			return false;
		} else if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN) {
			if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "handleVirtualKeys(down=" + down + ") taking ctrl event");
			mVirtualControlKeyDown = down;
			return true;
		} else if (keyCode == KeyEvent.KEYCODE_VOLUME_UP) {
			if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "handleVirtualKeys(down=" + down + ") taking Fn event");
			mVirtualFnKeyDown = down;
			return true;
		}
		return false;
	}

	public void checkForTypeface() {
		new Thread() {
			@Override
			public void run() {
				try {
					File fontFile = new File("/data/data/com.termux/files/home/.termux/font.ttf");
					final Typeface newTypeface = fontFile.exists() ? Typeface.createFromFile(fontFile) : Typeface.MONOSPACE;
					if (newTypeface != mRenderer.mTypeface) {
						post(new Runnable() {
							@Override
							public void run() {
								try {
									mRenderer = new TerminalRenderer(mRenderer.mTextSize, newTypeface);
									updateSize();
									invalidate();
								} catch (Exception e) {
									Log.e(EmulatorDebug.LOG_TAG, "Error loading font", e);
								}
							}
						});
					}
				} catch (Exception e) {
					Log.e(EmulatorDebug.LOG_TAG, "Error loading font", e);
				}
			}
		}.start();
	}

	public void checkForColors() {
		new Thread() {
			@Override
			public void run() {
				try {
					File colorsFile = new File("/data/data/com.termux/files/home/.termux/colors.properties");
					final Properties props = colorsFile.isFile() ? new Properties() : null;
					if (props != null) {
						try (InputStream in = new FileInputStream(colorsFile)) {
							props.load(in);
						}
					}
					post(new Runnable() {
						@Override
						public void run() {
							try {
								if (props == null) {
									TerminalColors.COLOR_SCHEME.reset();
								} else {
									TerminalColors.COLOR_SCHEME.updateWith(props);
								}
								if (mEmulator != null) mEmulator.mColors.reset();
								invalidate();
							} catch (Exception e) {
								Log.e(EmulatorDebug.LOG_TAG, "Setting colors failed: " + e.getMessage());
							}
						}
					});
				} catch (Exception e) {
					Log.e(EmulatorDebug.LOG_TAG, "Failed colors handling", e);
				}
			}
		}.start();
	}

	/**
	 * This is called during layout when the size of this view has changed. If you were just added to the view
	 * hierarchy, you're called with the old values of 0.
	 */
	@Override
	protected void onSizeChanged(int w, int h, int oldw, int oldh) {
		updateSize();
	}

	/** Check if the terminal size in rows and columns should be updated. */
	public void updateSize() {
		int viewWidth = getWidth();
		int viewHeight = getHeight();
		if (viewWidth == 0 || viewHeight == 0 || mTermSession == null) return;

		// Set to 80 and 24 if you want to enable vttest.
		int newColumns = Math.max(8, (int) (viewWidth / mRenderer.mFontWidth));
		int newRows = Math.max(8, (viewHeight - mRenderer.mFontLineSpacingAndAscent) / mRenderer.mFontLineSpacing);

		if (mEmulator == null || (newColumns != mEmulator.mColumns || newRows != mEmulator.mRows)) {
			mTermSession.updateSize(newColumns, newRows);
			mEmulator = mTermSession.getEmulator();

			mTopRow = 0;
			scrollTo(0, 0);
			invalidate();
		}
	}

	@Override
	protected void onDraw(Canvas canvas) {
		if (mEmulator == null) {
			canvas.drawColor(0XFF000000);
		} else {
			mRenderer.render(mEmulator, canvas, mTopRow, mSelY1, mSelY2, mSelX1, mSelX2);
		}
	}

	/** Toggle text selection mode in the view. */
	public void toggleSelectingText() {
		mIsSelectingText = !mIsSelectingText;
		if (!mIsSelectingText) mSelX1 = mSelY1 = mSelX2 = mSelY2 = -1;
	}

	public TerminalSession getCurrentSession() {
		return mTermSession;
	}

}
