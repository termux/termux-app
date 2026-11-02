package com.termux.view;

import android.os.SystemClock;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.ViewConfiguration;

import org.junit.Test;

import static org.junit.Assert.assertEquals;

/** Tests keyboard-request intent, not Android IME display or a remote phone. */
public class TerminalViewKeyboardTest extends TerminalViewTestCase {
    private void trackingTap() {
        input("\033[?1000h\033[?25l");
        tap(InputDevice.SOURCE_TOUCHSCREEN);
    }

    private void cursorResponse() {
        input("\033[?25h");
        view.onScreenUpdated();
    }

    @Test
    public void cursorResponseAfterTrackingTapRequestsKeyboardOnce() {
        trackingTap();
        assertEquals(0, client.keyboardRequests);
        cursorResponse();
        assertEquals(1, client.keyboardRequests);
        cursorResponse();
        view.onScreenUpdated();
        assertEquals(1, client.keyboardRequests);
    }

    @Test
    public void unsolicitedCursorVisibilityDoesNotRequestKeyboard() {
        input("\033[?1000h");
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void animationPhaseDoesNotSubstituteForEnabledCursor() {
        trackingTap();
        view.mEmulator.setCursorBlinkState(true);
        view.onScreenUpdated();
        assertEquals(0, client.keyboardRequests);
        cursorResponse();
        assertEquals(1, client.keyboardRequests);
        view.mEmulator.setCursorBlinkState(false);
        view.mEmulator.setCursorBlinkState(true);
        view.onScreenUpdated();
        assertEquals(1, client.keyboardRequests);
    }

    @Test
    public void responseAfterRecentTapWindowHasExpiredIsIgnored() {
        trackingTap();
        advance(TerminalView.MOUSE_TAP_KEYBOARD_TIMEOUT_MILLIS + 1);
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void newDownInvalidatesEarlierTap() {
        trackingTap();
        touch(SystemClock.uptimeMillis(), MotionEvent.ACTION_DOWN, 2.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void cancelInvalidatesEarlierTap() {
        trackingTap();
        touch(SystemClock.uptimeMillis(), MotionEvent.ACTION_CANCEL, 2.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void lostWindowFocusInvalidatesEarlierTap() {
        trackingTap();
        view.onWindowFocusChanged(false);
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void detachInvalidatesEarlierTap() {
        trackingTap();
        view.onDetachedFromWindow();
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void trackingOffClearsAuthorizationBeforeTrackingReturns() {
        trackingTap();
        input("\033[?1000l");
        view.onScreenUpdated();
        input("\033[?1000h");
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void trackingOffTapKeepsExistingClientCallback() {
        tap(InputDevice.SOURCE_TOUCHSCREEN);
        advance(ViewConfiguration.getDoubleTapTimeout() + 1);
        assertEquals(1, client.taps);
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void mouseDeviceTapDoesNotAuthorizeKeyboard() {
        input("\033[?1000h\033[?25l");
        tap(InputDevice.SOURCE_MOUSE);
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void longPressDoesNotAuthorizeKeyboardOnRelease() {
        input("\033[?1000h\033[?25l");
        longPress(InputDevice.SOURCE_TOUCHSCREEN);
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void fingerScrollDoesNotAuthorizeKeyboardOnRelease() {
        input("\033[?1000h\033[?25l");
        long down = SystemClock.uptimeMillis();
        touch(down, MotionEvent.ACTION_DOWN, 2.5f, 15.5f, InputDevice.SOURCE_TOUCHSCREEN);
        touch(down, MotionEvent.ACTION_MOVE, 2.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        touch(down, MotionEvent.ACTION_UP, 2.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        cursorResponse();
        assertEquals(0, client.keyboardRequests);
    }

    @Test
    public void anotherTapCanRequestKeyboardForAnAlreadyEnabledCursor() {
        trackingTap();
        cursorResponse();
        tap(InputDevice.SOURCE_TOUCHSCREEN);
        view.onScreenUpdated();
        assertEquals(2, client.keyboardRequests);
    }
}
