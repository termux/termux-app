package com.termux.view;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Rect;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;
import android.os.Build;
import android.text.Editable;
import android.text.InputType;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.view.accessibility.AccessibilityManager;
import android.view.ActionMode;
import android.view.HapticFeedbackConstants;
import android.view.InputDevice;
import android.view.KeyCharacterMap;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.Scroller;

import com.termux.terminal.EmulatorDebug;
import com.termux.terminal.KeyHandler;
import com.termux.terminal.TerminalBuffer;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalSession;

/** View displaying and interacting with a {@link TerminalSession}. */
public final class TerminalView extends View {

    /** Log view key and IME events. */
    private static final boolean LOG_KEY_EVENTS = false;

    /** The currently displayed terminal session, whose emulator is {@link #mEmulator}. */
    TerminalSession mTermSession;
    /** Our terminal emulator whose session is {@link #mTermSession}. */
    TerminalEmulator mEmulator;

    TerminalRenderer mRenderer;

    TerminalViewClient mClient;

    /** The top row of text to display. Ranges from -activeTranscriptRows to 0. */
    int mTopRow;

    boolean mIsSelectingText = false, mIsDraggingLeftSelection, mInitialTextSelection;
    int mSelX1 = -1, mSelX2 = -1, mSelY1 = -1, mSelY2 = -1;
    float mSelectionDownX, mSelectionDownY;
    private ActionMode mActionMode;
    private BitmapDrawable mLeftSelectionHandle, mRightSelectionHandle;

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

    private boolean mAccessibilityEnabled;

    public TerminalView(Context context, AttributeSet attributes) { // NO_UCD (unused code)
        super(context, attributes);
        mGestureRecognizer = new GestureAndScaleRecognizer(context, new GestureAndScaleRecognizer.Listener() {

            boolean scrolledWithFinger;

            @Override
            public boolean onUp(MotionEvent e) {
                mScrollRemainder = 0.0f;
                if (mEmulator != null && mEmulator.isMouseTrackingActive() && !mIsSelectingText && !scrolledWithFinger) {
                    // Quick event processing when mouse tracking is active - do not wait for check of double tapping
                    // for zooming.
                    sendMouseEventCode(e, TerminalEmulator.MOUSE_LEFT_BUTTON, true);
                    sendMouseEventCode(e, TerminalEmulator.MOUSE_LEFT_BUTTON, false);
                    return true;
                }
                scrolledWithFinger = false;
                return false;
            }

            @Override
            public boolean onSingleTapUp(MotionEvent e) {
                if (mEmulator == null) return true;
                if (mIsSelectingText) {
                    toggleSelectingText(null);
                    return true;
                }
                requestFocus();
                if (!mEmulator.isMouseTrackingActive()) {
                    if (!e.isFromSource(InputDevice.SOURCE_MOUSE)) {
                        mClient.onSingleTapUp(e);
                        return true;
                    }
                }
                return false;
            }

            @Override
            public boolean onScroll(MotionEvent e, float distanceX, float distanceY) {
                if (mEmulator == null || mIsSelectingText) return true;
                if (mEmulator.isMouseTrackingActive() && e.isFromSource(InputDevice.SOURCE_MOUSE)) {
                    // If moving with mouse pointer while pressing button, report that instead of scroll.
                    // This means that we never report moving with button press-events for touch input,
                    // since we cannot just start sending these events without a starting press event,
                    // which we do not do for touch input, only mouse in onTouchEvent().
                    sendMouseEventCode(e, TerminalEmulator.MOUSE_LEFT_BUTTON_MOVED, true);
                } else {
                    scrolledWithFinger = true;
                    distanceY += mScrollRemainder;
                    int deltaRows = (int) (distanceY / mRenderer.mFontLineSpacing);
                    mScrollRemainder = distanceY - deltaRows * mRenderer.mFontLineSpacing;
                    doScroll(e, deltaRows);
                }
                return true;
            }

            @Override
            public boolean onScale(float focusX, float focusY, float scale) {
                if (mEmulator == null || mIsSelectingText) return true;
                mScaleFactor *= scale;
                mScaleFactor = mClient.onScale(mScaleFactor);
                return true;
            }

            @Override
            public boolean onFling(final MotionEvent e2, float velocityX, float velocityY) {
                if (mEmulator == null || mIsSelectingText) return true;
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
                if (mGestureRecognizer.isInProgress()) return;
                if (mClient.onLongPress(e)) return;
                if (!mIsSelectingText) {
                    performHapticFeedback(HapticFeedbackConstants.LONG_PRESS);
                    toggleSelectingText(e);
                }
            }
        });
        mScroller = new Scroller(context);
        AccessibilityManager am = (AccessibilityManager) context.getSystemService(Context.ACCESSIBILITY_SERVICE);
        mAccessibilityEnabled = am.isEnabled();
    }

    /**
     * @param onKeyListener Listener for all kinds of key events, both hardware and IME (which makes it different from that
     *                      available with {@link View#setOnKeyListener(OnKeyListener)}.
     */
    public void setOnKeyListener(TerminalViewClient onKeyListener) {
        this.mClient = onKeyListener;
    }

    /**
     * Attach a {@link TerminalSession} to this view.
     *
     * @param session The {@link TerminalSession} this view will be displaying.
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
        // Using InputType.NULL is the most correct input type and avoids issues with other hacks.
        //
        // Previous keyboard issues:
        // https://github.com/termux/termux-packages/issues/25
        // https://github.com/termux/termux-app/issues/87.
        // https://github.com/termux/termux-app/issues/126.
        // https://github.com/termux/termux-app/issues/137 (japanese chars and TYPE_NULL).
        outAttrs.inputType = InputType.TYPE_NULL;

        // Note that IME_ACTION_NONE cannot be used as that makes it impossible to input newlines using the on-screen
        // keyboard on Android TV (see https://github.com/termux/termux-app/issues/221).
        outAttrs.imeOptions = EditorInfo.IME_FLAG_NO_FULLSCREEN;

        return new BaseInputConnection(this, true) {

            @Override
            public boolean finishComposingText() {
                if (LOG_KEY_EVENTS) Log.i(EmulatorDebug.LOG_TAG, "IME: finishComposingText()");
                super.finishComposingText();

                sendTextToTerminal(getEditable());
                getEditable().clear();
                return true;
            }

            @Override
            public boolean commitText(CharSequence text, int newCursorPosition) {
                if (LOG_KEY_EVENTS) {
                    Log.i(EmulatorDebug.LOG_TAG, "IME: commitText(\"" + text + "\", " + newCursorPosition + ")");
                }
                super.commitText(text, newCursorPosition);

                if (mEmulator == null) return true;

                Editable content = getEditable();
                sendTextToTerminal(content);
                content.clear();
                return true;
            }

            @Override
            public boolean deleteSurroundingText(int leftLength, int rightLength) {
                if (LOG_KEY_EVENTS) {
                    Log.i(EmulatorDebug.LOG_TAG, "IME: deleteSurroundingText(" + leftLength + ", " + rightLength + ")");
                }
                // The stock Samsung keyboard with 'Auto check spelling' enabled sends leftLength > 1.
                KeyEvent deleteKey = new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_DEL);
                for (int i = 0; i < leftLength; i++) sendKeyEvent(deleteKey);
                return super.deleteSurroundingText(leftLength, rightLength);
            }

            void sendTextToTerminal(CharSequence text) {
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

                    boolean ctrlHeld = false;
                    if (codePoint <= 31 && codePoint != 27) {
                        if (codePoint == '\n') {
                            // The AOSP keyboard and descendants seems to send \n as text when the enter key is pressed,
                            // instead of a key event like most other keyboard apps. A terminal expects \r for the enter
                            // key (although when icrnl is enabled this doesn't make a difference - run 'stty -icrnl' to
                            // check the behaviour).
                            codePoint = '\r';
                        }

                        // E.g. penti keyboard for ctrl input.
                        ctrlHeld = true;
                        switch (codePoint) {
                            case 31:
                                codePoint = '_';
                                break;
                            case 30:
                                codePoint = '^';
                                break;
                            case 29:
                                codePoint = ']';
                                break;
                            case 28:
                                codePoint = '\\';
                                break;
                            default:
                                codePoint += 96;
                                break;
                        }
                    }

                    inputCodePoint(codePoint, ctrlHeld, false);
                }
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

        int rowsInHistory = mEmulator.getScreen().getActiveTranscriptRows();
        if (mTopRow < -rowsInHistory) mTopRow = -rowsInHistory;

        boolean skipScrolling = false;
        if (mIsSelectingText) {
            // Do not scroll when selecting text.
            int rowShift = mEmulator.getScrollCounter();
            if (-mTopRow + rowShift > rowsInHistory) {
                // .. unless we're hitting the end of history transcript, in which
                // case we abort text selection and scroll to end.
                toggleSelectingText(null);
            } else {
                skipScrolling = true;
                mTopRow -= rowShift;
                mSelY1 -= rowShift;
                mSelY2 -= rowShift;
            }
        }

        if (!skipScrolling && mTopRow != 0) {
            // Scroll down if not already there.
            if (mTopRow < -3) {
                // Awaken scroll bars only if scrolling a noticeable amount
                // - we do not want visible scroll bars during normal typing
                // of one row at a time.
                awakenScrollBars();
            }
            mTopRow = 0;
        }

        mEmulator.clearScrollCounter();

        invalidate();
        if (mAccessibilityEnabled) setContentDescription(getText());
    }

    /**
     * Sets the text size, which in turn sets the number of rows and columns.
     *
     * @param textSize the new font size, in density-independent pixels.
     */
    public void setTextSize(int textSize) {
        mRenderer = new TerminalRenderer(textSize, mRenderer == null ? Typeface.MONOSPACE : mRenderer.mTypeface);
        updateSize();
    }

    public void setTypeface(Typeface newTypeface) {
        mRenderer = new TerminalRenderer(mRenderer.mTextSize, newTypeface);
        updateSize();
        invalidate();
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
    @TargetApi(23)
    public boolean onTouchEvent(MotionEvent ev) {
        if (mEmulator == null) return true;
        final int action = ev.getAction();

        if (mIsSelectingText) {
            int cy = (int) (ev.getY() / mRenderer.mFontLineSpacing) + mTopRow;
            int cx = (int) (ev.getX() / mRenderer.mFontWidth);

            switch (action) {
                case MotionEvent.ACTION_UP:
                    mInitialTextSelection = false;
                    break;
                case MotionEvent.ACTION_DOWN:
                    int distanceFromSel1 = Math.abs(cx - mSelX1) + Math.abs(cy - mSelY1);
                    int distanceFromSel2 = Math.abs(cx - mSelX2) + Math.abs(cy - mSelY2);
                    mIsDraggingLeftSelection = distanceFromSel1 <= distanceFromSel2;
                    mSelectionDownX = ev.getX();
                    mSelectionDownY = ev.getY();
                    break;
                case MotionEvent.ACTION_MOVE:
                    if (mInitialTextSelection) break;
                    float deltaX = ev.getX() - mSelectionDownX;
                    float deltaY = ev.getY() - mSelectionDownY;
                    int deltaCols = (int) Math.ceil(deltaX / mRenderer.mFontWidth);
                    int deltaRows = (int) Math.ceil(deltaY / mRenderer.mFontLineSpacing);
                    mSelectionDownX += deltaCols * mRenderer.mFontWidth;
                    mSelectionDownY += deltaRows * mRenderer.mFontLineSpacing;
                    if (mIsDraggingLeftSelection) {
                        mSelX1 += deltaCols;
                        mSelY1 += deltaRows;
                    } else {
                        mSelX2 += deltaCols;
                        mSelY2 += deltaRows;
                    }

                    mSelX1 = Math.min(mEmulator.mColumns, Math.max(0, mSelX1));
                    mSelX2 = Math.min(mEmulator.mColumns, Math.max(0, mSelX2));

                    if (mSelY1 == mSelY2 && mSelX1 > mSelX2 || mSelY1 > mSelY2) {
                        // Switch handles.
                        mIsDraggingLeftSelection = !mIsDraggingLeftSelection;
                        int tmpX1 = mSelX1, tmpY1 = mSelY1;
                        mSelX1 = mSelX2;
                        mSelY1 = mSelY2;
                        mSelX2 = tmpX1;
                        mSelY2 = tmpY1;
                    }

                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
                        mActionMode.invalidateContentRect();
                    invalidate();
                    break;
                default:
                    break;
            }
            mGestureRecognizer.onTouchEvent(ev);
            return true;
        } else if (ev.isFromSource(InputDevice.SOURCE_MOUSE)) {
            if (ev.isButtonPressed(MotionEvent.BUTTON_SECONDARY)) {
                if (action == MotionEvent.ACTION_DOWN) showContextMenu();
                return true;
            } else if (ev.isButtonPressed(MotionEvent.BUTTON_TERTIARY)) {
                ClipboardManager clipboard = (ClipboardManager) getContext().getSystemService(Context.CLIPBOARD_SERVICE);
                ClipData clipData = clipboard.getPrimaryClip();
                if (clipData != null) {
                    CharSequence paste = clipData.getItemAt(0).coerceToText(getContext());
                    if (!TextUtils.isEmpty(paste)) mEmulator.paste(paste.toString());
                }
            } else if (mEmulator.isMouseTrackingActive()) { // BUTTON_PRIMARY.
                switch (ev.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                    case MotionEvent.ACTION_UP:
                        sendMouseEventCode(ev, TerminalEmulator.MOUSE_LEFT_BUTTON, ev.getAction() == MotionEvent.ACTION_DOWN);
                        break;
                    case MotionEvent.ACTION_MOVE:
                        sendMouseEventCode(ev, TerminalEmulator.MOUSE_LEFT_BUTTON_MOVED, true);
                        break;
                }
                return true;
            }
        }

        mGestureRecognizer.onTouchEvent(ev);
        return true;
    }

    @Override
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        if (LOG_KEY_EVENTS)
            Log.i(EmulatorDebug.LOG_TAG, "onKeyPreIme(keyCode=" + keyCode + ", event=" + event + ")");
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (mIsSelectingText) {
                toggleSelectingText(null);
                return true;
            } else if (mClient.shouldBackButtonBeMappedToEscape()) {
                // Intercept back button to treat it as escape:
                switch (event.getAction()) {
                    case KeyEvent.ACTION_DOWN:
                        return onKeyDown(keyCode, event);
                    case KeyEvent.ACTION_UP:
                        return onKeyUp(keyCode, event);
                }
            }
        }
        return super.onKeyPreIme(keyCode, event);
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (LOG_KEY_EVENTS)
            Log.i(EmulatorDebug.LOG_TAG, "onKeyDown(keyCode=" + keyCode + ", isSystem()=" + event.isSystem() + ", event=" + event + ")");
        if (mEmulator == null) return true;

        if (mClient.onKeyDown(keyCode, event, mTermSession)) {
            invalidate();
            return true;
        } else if (event.isSystem() && (!mClient.shouldBackButtonBeMappedToEscape() || keyCode != KeyEvent.KEYCODE_BACK)) {
            return super.onKeyDown(keyCode, event);
        } else if (event.getAction() == KeyEvent.ACTION_MULTIPLE && keyCode == KeyEvent.KEYCODE_UNKNOWN) {
            mTermSession.write(event.getCharacters());
            return true;
        }

        final int metaState = event.getMetaState();
        final boolean controlDownFromEvent = event.isCtrlPressed();
        final boolean leftAltDownFromEvent = (metaState & KeyEvent.META_ALT_LEFT_ON) != 0;
        final boolean rightAltDownFromEvent = (metaState & KeyEvent.META_ALT_RIGHT_ON) != 0;

        int keyMod = 0;
        if (controlDownFromEvent) keyMod |= KeyHandler.KEYMOD_CTRL;
        if (event.isAltPressed()) keyMod |= KeyHandler.KEYMOD_ALT;
        if (event.isShiftPressed()) keyMod |= KeyHandler.KEYMOD_SHIFT;
        if (!event.isFunctionPressed() && handleKeyCode(keyCode, keyMod)) {
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
        if (LOG_KEY_EVENTS)
            Log.i(EmulatorDebug.LOG_TAG, "KeyEvent#getUnicodeChar(" + effectiveMetaState + ") returned: " + result);
        if (result == 0) {
            return false;
        }

        int oldCombiningAccent = mCombiningAccent;
        if ((result & KeyCharacterMap.COMBINING_ACCENT) != 0) {
            // If entered combining accent previously, write it out:
            if (mCombiningAccent != 0)
                inputCodePoint(mCombiningAccent, controlDownFromEvent, leftAltDownFromEvent);
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

        if (mTermSession == null) return;

        final boolean controlDown = controlDownFromEvent || mClient.readControlKey();
        final boolean altDown = leftAltDownFromEvent || mClient.readAltKey();

        if (mClient.onCodePoint(codePoint, controlDown, mTermSession)) return;

        if (controlDown) {
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
            } else if (codePoint == '_' || codePoint == '7' || codePoint == '/') {
                // "Ctrl-/ sends 0x1f which is equivalent of Ctrl-_ since the days of VT102"
                // - http://apple.stackexchange.com/questions/24261/how-do-i-send-c-that-is-control-slash-to-the-terminal
                codePoint = 31;
            } else if (codePoint == '8') {
                codePoint = 127; // DEL
            }
        }

        if (codePoint > -1) {
            // Work around bluetooth keyboards sending funny unicode characters instead
            // of the more normal ones from ASCII that terminal programs expect - the
            // desire to input the original characters should be low.
            switch (codePoint) {
                case 0x02DC: // SMALL TILDE.
                    codePoint = 0x007E; // TILDE (~).
                    break;
                case 0x02CB: // MODIFIER LETTER GRAVE ACCENT.
                    codePoint = 0x0060; // GRAVE ACCENT (`).
                    break;
                case 0x02C6: // MODIFIER LETTER CIRCUMFLEX ACCENT.
                    codePoint = 0x005E; // CIRCUMFLEX ACCENT (^).
                    break;
            }

            // If left alt, send escape before the code point to make e.g. Alt+B and Alt+F work in readline:
            mTermSession.writeCodePoint(altDown, codePoint);
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
     * @param keyCode The keycode of the key which was released.
     * @param event   A {@link KeyEvent} describing the event.
     * @return Whether the event was handled.
     */
    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (LOG_KEY_EVENTS)
            Log.i(EmulatorDebug.LOG_TAG, "onKeyUp(keyCode=" + keyCode + ", event=" + event + ")");
        if (mEmulator == null) return true;

        if (mClient.onKeyUp(keyCode, event)) {
            invalidate();
            return true;
        } else if (event.isSystem()) {
            // Let system key events through.
            return super.onKeyUp(keyCode, event);
        }

        return true;
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
        int newColumns = Math.max(4, (int) (viewWidth / mRenderer.mFontWidth));
        int newRows = Math.max(4, (viewHeight - mRenderer.mFontLineSpacingAndAscent) / mRenderer.mFontLineSpacing);

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

            if (mIsSelectingText) {
                final int gripHandleWidth = mLeftSelectionHandle.getIntrinsicWidth();
                final int gripHandleMargin = gripHandleWidth / 4; // See the png.

                int right = Math.round((mSelX1) * mRenderer.mFontWidth) + gripHandleMargin;
                int top = (mSelY1 + 1 - mTopRow) * mRenderer.mFontLineSpacing + mRenderer.mFontLineSpacingAndAscent;
                mLeftSelectionHandle.setBounds(right - gripHandleWidth, top, right, top + mLeftSelectionHandle.getIntrinsicHeight());
                mLeftSelectionHandle.draw(canvas);

                int left = Math.round((mSelX2 + 1) * mRenderer.mFontWidth) - gripHandleMargin;
                top = (mSelY2 + 1 - mTopRow) * mRenderer.mFontLineSpacing + mRenderer.mFontLineSpacingAndAscent;
                mRightSelectionHandle.setBounds(left, top, left + gripHandleWidth, top + mRightSelectionHandle.getIntrinsicHeight());
                mRightSelectionHandle.draw(canvas);
            }
        }
    }

    /** Toggle text selection mode in the view. */
    @TargetApi(23)
    public void toggleSelectingText(MotionEvent ev) {
        mIsSelectingText = !mIsSelectingText;
        mClient.copyModeChanged(mIsSelectingText);

        if (mIsSelectingText) {
            if (mLeftSelectionHandle == null) {
                mLeftSelectionHandle = (BitmapDrawable) getContext().getDrawable(R.drawable.text_select_handle_left_material);
                mRightSelectionHandle = (BitmapDrawable) getContext().getDrawable(R.drawable.text_select_handle_right_material);
            }

            int cx = (int) (ev.getX() / mRenderer.mFontWidth);
            final boolean eventFromMouse = ev.isFromSource(InputDevice.SOURCE_MOUSE);
            // Offset for finger:
            final int SELECT_TEXT_OFFSET_Y = eventFromMouse ? 0 : -40;
            int cy = (int) ((ev.getY() + SELECT_TEXT_OFFSET_Y) / mRenderer.mFontLineSpacing) + mTopRow;

            mSelX1 = mSelX2 = cx;
            mSelY1 = mSelY2 = cy;

            TerminalBuffer screen = mEmulator.getScreen();
            if (!" ".equals(screen.getSelectedText(mSelX1, mSelY1, mSelX1, mSelY1))) {
                // Selecting something other than whitespace. Expand to word.
                while (mSelX1 > 0 && !"".equals(screen.getSelectedText(mSelX1 - 1, mSelY1, mSelX1 - 1, mSelY1))) {
                    mSelX1--;
                }
                while (mSelX2 < mEmulator.mColumns - 1 && !"".equals(screen.getSelectedText(mSelX2 + 1, mSelY1, mSelX2 + 1, mSelY1))) {
                    mSelX2++;
                }
            }

            mInitialTextSelection = true;
            mIsDraggingLeftSelection = true;
            mSelectionDownX = ev.getX();
            mSelectionDownY = ev.getY();

            final ActionMode.Callback callback = new ActionMode.Callback() {
                @Override
                public boolean onCreateActionMode(ActionMode mode, Menu menu) {
                    int show = MenuItem.SHOW_AS_ACTION_ALWAYS | MenuItem.SHOW_AS_ACTION_WITH_TEXT;

                    ClipboardManager clipboard = (ClipboardManager) getContext().getSystemService(Context.CLIPBOARD_SERVICE);
                    menu.add(Menu.NONE, 1, Menu.NONE, R.string.copy_text).setShowAsAction(show);
                    menu.add(Menu.NONE, 2, Menu.NONE, R.string.paste_text).setEnabled(clipboard.hasPrimaryClip()).setShowAsAction(show);
                    menu.add(Menu.NONE, 3, Menu.NONE, R.string.text_selection_more);
                    return true;
                }

                @Override
                public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
                    return false;
                }

                @Override
                public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
                    if (!mIsSelectingText) {
                        // Fix issue where the dialog is pressed while being dismissed.
                        return true;
                    }
                    switch (item.getItemId()) {
                        case 1:
                            String selectedText = mEmulator.getSelectedText(mSelX1, mSelY1, mSelX2, mSelY2).trim();
                            mTermSession.clipboardText(selectedText);
                            break;
                        case 2:
                            ClipboardManager clipboard = (ClipboardManager) getContext().getSystemService(Context.CLIPBOARD_SERVICE);
                            ClipData clipData = clipboard.getPrimaryClip();
                            if (clipData != null) {
                                CharSequence paste = clipData.getItemAt(0).coerceToText(getContext());
                                if (!TextUtils.isEmpty(paste)) mEmulator.paste(paste.toString());
                            }
                            break;
                        case 3:
                            showContextMenu();
                            break;
                    }
                    toggleSelectingText(null);
                    return true;
                }

                @Override
                public void onDestroyActionMode(ActionMode mode) {
                }

            };

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                mActionMode = startActionMode(new ActionMode.Callback2() {
                    @Override
                    public boolean onCreateActionMode(ActionMode mode, Menu menu) {
                        return callback.onCreateActionMode(mode, menu);
                    }

                    @Override
                    public boolean onPrepareActionMode(ActionMode mode, Menu menu) {
                        return false;
                    }

                    @Override
                    public boolean onActionItemClicked(ActionMode mode, MenuItem item) {
                        return callback.onActionItemClicked(mode, item);
                    }

                    @Override
                    public void onDestroyActionMode(ActionMode mode) {
                        // Ignore.
                    }

                    @Override
                    public void onGetContentRect(ActionMode mode, View view, Rect outRect) {
                        int x1 = Math.round(mSelX1 * mRenderer.mFontWidth);
                        int x2 = Math.round(mSelX2 * mRenderer.mFontWidth);
                        int y1 = Math.round((mSelY1 - mTopRow) * mRenderer.mFontLineSpacing);
                        int y2 = Math.round((mSelY2 + 1 - mTopRow) * mRenderer.mFontLineSpacing);
                        outRect.set(Math.min(x1, x2), y1, Math.max(x1, x2), y2);
                    }
                }, ActionMode.TYPE_FLOATING);
            } else {
                mActionMode = startActionMode(callback);
            }


            invalidate();
        } else {
            mActionMode.finish();
            mSelX1 = mSelY1 = mSelX2 = mSelY2 = -1;
            invalidate();
        }
    }

    public TerminalSession getCurrentSession() {
        return mTermSession;
    }

    private CharSequence getText() {
        return mEmulator.getScreen().getSelectedText(0, mTopRow, mEmulator.mColumns, mTopRow +mEmulator.mRows);
    }

}
