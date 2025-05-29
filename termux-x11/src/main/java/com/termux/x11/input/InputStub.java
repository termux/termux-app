// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.termux.x11.input;

/**
 * A set of functions to send client users' activities to remote host machine. This interface
 * represents low level functions without relationships with Android system. Consumers can use
 * {@link InputEventSender} to avoid conversions between Android classes and JNI types. The
 * implementations of this interface are not required to be thread-safe. All these functions should
 * be called from Android UI thread.
 */
public interface InputStub {
    // These constants must match those in the generated struct protocol::MouseEvent_MouseButton.
    int BUTTON_UNDEFINED = 0;
    int BUTTON_LEFT = 1;
    int BUTTON_MIDDLE = 2;
    int BUTTON_RIGHT = 3;
    int BUTTON_SCROLL = 4;

    /** Sends a mouse event. */
    void sendMouseEvent(float x, float y, int whichButton, boolean buttonDown, boolean relative);

    /** Sends a mouse wheel event. */
    void sendMouseWheelEvent(float deltaX, float deltaY);

    /**
     * Sends a key event, and returns false if both scanCode and keyCode are not able to be
     * converted to a known usb key code. Nothing will be sent to remote host, if this function
     * returns false.
     */
    boolean sendKeyEvent(int scanCode, int keyCode, boolean keyDown);

    /**
     * Sends a string literal. This function is useful to handle outputs from Android input
     * methods.
     */
    void sendTextEvent(byte[] utf8Bytes);

    /** Sends an event, not flushing connection. */
    void sendTouchEvent(int action, int pointerId, int x, int y);

    void sendStylusEvent(float x, float y, int pressure, int tiltX, int tiltY, int orientation, int buttons, boolean eraser, boolean mouseMode);
}
