// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.termux.x11.input;

import static android.view.KeyEvent.*;
import static android.view.MotionEvent.*;
import static androidx.core.math.MathUtils.clamp;
import static com.termux.x11.input.InputStub.*;
import static java.nio.charset.StandardCharsets.UTF_8;

import android.graphics.PointF;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import java.util.List;
import java.util.TreeSet;

/**
 * A set of functions to send users' activities, which are represented by Android classes, to
 * remote host machine. This class uses a {@link InputStub} to do the real injections.
 */
public final class InputEventSender {
    private static final int XI_TouchBegin = 18;
    private static final int XI_TouchUpdate = 19;
    private static final int XI_TouchEnd = 20;

    private final InputStub mInjector;

    public boolean tapToMove = false;
    public boolean preferScancodes = false;
    public boolean pointerCapture = false;
    public boolean scaleTouchpad = false;

    /**
     * Set of pressed keys for which we've sent TextEvent.
     */
    private final TreeSet<Integer> mPressedTextKeys;
    private long previousTouchTime = 0L;
    private int previousTouchX, previousTouchY;
    private int touchCounter = 0;
    private long MAX_DOUBLE_CLICK_TIMEOUT = 501;
    private int MINIMUM_DISTANCE = 15;
    private long MINIMUN_DOUBLE_CLICK_TIMEOUT = 101;

    private boolean isDoubleClick(MotionEvent event, int currentTouchX, int currentTouchY) {
        boolean doubleClick = false;
        long currentTouchTime = System.currentTimeMillis();
        if (event.getAction() == MotionEvent.ACTION_DOWN || event.getAction() == ACTION_POINTER_DOWN) {
            if (touchCounter == 0) {
                previousTouchTime = currentTouchTime;
                previousTouchX = currentTouchX;
                previousTouchY = currentTouchY;
            }
            touchCounter++;
        }

        if (touchCounter >= 2) {
            if (event.getAction() == MotionEvent.ACTION_UP || event.getAction() == ACTION_POINTER_UP) {
                doubleClick = (currentTouchTime - previousTouchTime > MINIMUN_DOUBLE_CLICK_TIMEOUT) && (currentTouchTime - previousTouchTime < MAX_DOUBLE_CLICK_TIMEOUT) && (Math.abs(currentTouchX - previousTouchX) < MINIMUM_DISTANCE) && (Math.abs(currentTouchY - previousTouchY) < MINIMUM_DISTANCE);
                if (doubleClick) {
                    touchCounter = 0;
                    return true;
                } else {
                    touchCounter = 1;
                    previousTouchTime = currentTouchTime;
                    previousTouchX = currentTouchX;
                    previousTouchY = currentTouchY;
                }
            }
        }
        return false;
    }

    public InputEventSender(InputStub injector) {
        if (injector == null)
            throw new NullPointerException();
        mInjector = injector;
        mPressedTextKeys = new TreeSet<>();
    }

    private static final List<Integer> buttons = List.of(BUTTON_UNDEFINED, BUTTON_LEFT, BUTTON_MIDDLE, BUTTON_RIGHT);

    public void sendMouseEvent(PointF pos, int button, boolean down, boolean relative) {
        if (!buttons.contains(button))
            return;
        mInjector.sendMouseEvent(pos != null ? (int) pos.x : 0, pos != null ? (int) pos.y : 0, button, down, relative);
    }

    public void sendMouseDown(int button, boolean relative) {
        if (!buttons.contains(button))
            return;
        mInjector.sendMouseEvent(0, 0, button, true, relative);
    }

    public void sendMouseUp(int button, boolean relative) {
        if (!buttons.contains(button))
            return;
        mInjector.sendMouseEvent(0, 0, button, false, relative);
    }

    public void sendMouseClick(int button, boolean relative) {
        if (!buttons.contains(button))
            return;
        mInjector.sendMouseEvent(0, 0, button, true, relative);
        mInjector.sendMouseEvent(0, 0, button, false, relative);
    }

    public void sendCursorMove(float x, float y, boolean relative) {
        mInjector.sendMouseEvent(x, y, BUTTON_UNDEFINED, false, relative);
    }

    public void sendMouseWheelEvent(float distanceX, float distanceY) {
        mInjector.sendMouseWheelEvent(distanceX, distanceY);
    }

    final boolean[] pointers = new boolean[10];

    /**
     * Extracts the touch point data from a MotionEvent, converts each point into a marshallable
     * object and passes the set of points to the JNI layer to be transmitted to the remote host.
     *
     * @param event The event to send to the remote host for injection.  NOTE: This object must be
     *              updated to represent the remote machine's coordinate system before calling this
     *              function.
     */
    public void sendTouchEvent(MotionEvent event, RenderData renderData) {
        int xx = clamp((int) ((event.getX() - renderData.offsetX) * renderData.scale.x), 0, renderData.screenWidth);
        int yy = clamp((int) ((event.getY() - renderData.offsetY) * renderData.scale.y), 0, renderData.screenHeight);
        if (isDoubleClick(event, xx, yy)) {
            sendMouseClick(BUTTON_LEFT, true);
            sendMouseClick(BUTTON_LEFT, true);
            return;
        }
        int action = event.getActionMasked();

        if (action == ACTION_MOVE || action == ACTION_HOVER_MOVE || action == ACTION_HOVER_ENTER || action == ACTION_HOVER_EXIT) {
            // In order to process all of the events associated with an ACTION_MOVE event, we need
            // to walk the list of historical events in order and add each event to our list, then
            // retrieve the current move event data.
            int pointerCount = event.getPointerCount();

            for (int p = 0; p < pointerCount; p++)
                pointers[event.getPointerId(p)] = false;

            for (int p = 0; p < pointerCount; p++) {
                int x = clamp((int) ((event.getX(p) - renderData.offsetX) * renderData.scale.x), 0, renderData.screenWidth);
                int y = clamp((int) ((event.getY(p) - renderData.offsetY) * renderData.scale.y), 0, renderData.screenHeight);
                pointers[event.getPointerId(p)] = true;
                mInjector.sendTouchEvent(XI_TouchUpdate, event.getPointerId(p), x, y);
            }

            // Sometimes Android does not send ACTION_POINTER_UP/ACTION_UP so some pointers are "stuck" in pressed state.
            for (int p = 0; p < 10; p++) {
                if (!pointers[p])
                    mInjector.sendTouchEvent(XI_TouchEnd, p, 0, 0);
            }
        } else {
            // For all other events, we only want to grab the current/active pointer.  The event
            // contains a list of every active pointer but passing all of of these to the host can
            // cause confusion on the remote OS side and result in broken touch gestures.
            int activePointerIndex = event.getActionIndex();
            int id = event.getPointerId(activePointerIndex);
            int x = clamp((int) ((event.getX(activePointerIndex) - renderData.offsetX) * renderData.scale.x), 0, renderData.screenWidth);
            int y = clamp((int) ((event.getY(activePointerIndex) - renderData.offsetY) * renderData.scale.y), 0, renderData.screenHeight);
            int a = (action == MotionEvent.ACTION_DOWN || action == ACTION_POINTER_DOWN) ? XI_TouchBegin : XI_TouchEnd;
            if (a == XI_TouchEnd) {
                mInjector.sendTouchEvent(XI_TouchUpdate, id, x, y);
            }
            mInjector.sendTouchEvent(a, id, x, y);
        }
    }


    /**
     * Converts the {@link KeyEvent} into low-level events and sends them to the host as either
     * key-events or text-events. This contains some logic for handling some special keys, and
     * avoids sending a key-up event for a key that was previously injected as a text-event.
     */
    public boolean sendKeyEvent(View v, KeyEvent e) {
        int keyCode = e.getKeyCode();
        boolean pressed = e.getAction() == KeyEvent.ACTION_DOWN;

        // Events received from software keyboards generate TextEvent in two
        // cases:
        //   1. This is an ACTION_MULTIPLE event.
        //   2. Ctrl, Alt and Meta are not pressed.
        // This ensures that on-screen keyboard always injects input that
        // correspond to what user sees on the screen, while physical keyboard
        // acts as if it is connected to the remote host.
        if (e.getAction() == ACTION_MULTIPLE) {
            if (e.getCharacters() != null)
                mInjector.sendTextEvent(e.getCharacters().getBytes(UTF_8));
            else if (e.getUnicodeChar() != 0)
                mInjector.sendTextEvent(String.valueOf((char) e.getUnicodeChar()).getBytes(UTF_8));
            return true;
        }

        boolean no_modifiers = (!e.isAltPressed() && !e.isCtrlPressed() && !e.isMetaPressed())
                || ((e.getMetaState() & META_ALT_RIGHT_ON) != 0 && (e.getCharacters() != null || e.getUnicodeChar() != 0)); // For layouts with AltGr
        // For Enter getUnicodeChar() returns 10 (line feed), but we still
        // want to send it as KeyEvent.
        char unicode = keyCode != KEYCODE_ENTER ? (char) e.getUnicodeChar() : 0;
        int scancode = (preferScancodes || !no_modifiers) ? e.getScanCode() : 0;

        if (!preferScancodes) {
            if (pressed && unicode != 0 && no_modifiers) {
                mPressedTextKeys.add(keyCode);
                if ((e.getMetaState() & META_ALT_RIGHT_ON) != 0)
                    mInjector.sendKeyEvent(0, KEYCODE_ALT_RIGHT, false); // For layouts with AltGr

                mInjector.sendTextEvent(String.valueOf(unicode).getBytes(UTF_8));

                if ((e.getMetaState() & META_ALT_RIGHT_ON) != 0)
                    mInjector.sendKeyEvent(0, KEYCODE_ALT_RIGHT, true); // For layouts with AltGr
                return true;
            }

            if (!pressed && mPressedTextKeys.contains(keyCode)) {
                mPressedTextKeys.remove(keyCode);
                return true;
            }
        }

        // KEYCODE_AT, KEYCODE_POUND, KEYCODE_STAR and KEYCODE_PLUS are
        // deprecated, but they still need to be here for older devices and
        // third-party keyboards that may still generate these events. See
        // https://source.android.com/devices/input/keyboard-devices.html#legacy-unsupported-keys
        char[][] chars = {
                {KEYCODE_AT, '@', KEYCODE_2},
                {KEYCODE_POUND, '#', KEYCODE_3},
                {KEYCODE_STAR, '*', KEYCODE_8},
                {KEYCODE_PLUS, '+', KEYCODE_EQUALS}
        };

        for (char[] i : chars) {
            if (e.getKeyCode() != i[0])
                continue;

            if ((e.getCharacters() != null && String.valueOf(i[1]).contentEquals(e.getCharacters()))
                    || e.getUnicodeChar() == i[1]) {
                mInjector.sendKeyEvent(0, KEYCODE_SHIFT_LEFT, pressed);
                mInjector.sendKeyEvent(0, i[2], pressed);
                return true;
            }
        }

        // Ignoring Android's autorepeat.
        if (e.getRepeatCount() > 0)
            return true;

        if (pointerCapture && keyCode == KEYCODE_ESCAPE && !pressed)
            v.releasePointerCapture();

        // We try to send all other key codes to the host directly.
        return mInjector.sendKeyEvent(scancode, keyCode, pressed);
    }
}
