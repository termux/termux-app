package com.termux.view;

import android.app.Application;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.MotionEvent;

import com.termux.shared.termux.terminal.TermuxTerminalSessionClientBase;
import com.termux.shared.termux.terminal.TermuxTerminalViewClientBase;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalOutput;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;

import static org.junit.Assert.*;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28, manifest = Config.NONE, application = Application.class)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
public class TerminalViewMouseTest {
    private final RecordingOutput output = new RecordingOutput();
    private TerminalView view;

    @Before
    public void setUp() {
        view = new TerminalView(RuntimeEnvironment.getApplication(), null);
        view.setTerminalViewClient(new TermuxTerminalViewClientBase());
        view.setTextSize(16);
        assertTrue("Gesture coordinates require real font width", view.mRenderer.mFontWidth > 0);
        assertTrue("Gesture coordinates require real line spacing", view.mRenderer.mFontLineSpacing > 0);
        view.mEmulator = new TerminalEmulator(output, 80, 24, 8, 16, null,
            new TermuxTerminalSessionClientBase());
        view.mGestureRecognizer.mListener.onDown(0, 0);
    }

    private void input(String text) {
        byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
        view.mEmulator.append(bytes, bytes.length);
    }

    private MotionEvent event(long down, int action, float column, float row, int source) {
        MotionEvent event = MotionEvent.obtain(down, SystemClock.uptimeMillis(), action,
            column * view.mRenderer.mFontWidth,
            view.mRenderer.mFontLineSpacingAndAscent + row * view.mRenderer.mFontLineSpacing, 0);
        event.setSource(source);
        return event;
    }

    private void scroll(float columns, float rows) {
        MotionEvent event = event(1, MotionEvent.ACTION_MOVE, 2.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        view.mGestureRecognizer.mListener.onScroll(event,
            columns * view.mRenderer.mFontWidth, rows * view.mRenderer.mFontLineSpacing);
        event.recycle();
    }

    @Test
    public void horizontalFractionalMotionUsesRightWheelAndLocksAxis() {
        input("\033[?1000h\033[?1006h");
        scroll(0.4f, 0);
        assertEquals("", output.text());
        scroll(0.7f, 0);
        assertEquals("\033[<67;3;4M", output.text());
        scroll(1, 4);
        assertEquals("\033[<67;3;4M\033[<67;3;4M", output.text());
    }

    @Test
    public void negativeMotionUsesLeftWheelThroughLegacyEncoder() {
        input("\033[?1002h");
        scroll(-2.25f, 0);
        assertEquals("\033[Mb#$\033[Mb#$", output.text());
    }

    @Test
    public void verticalGestureKeepsUpWheelOriginAndAxis() {
        input("\033[?1000h\033[?1006h");
        scroll(0, -1);
        MotionEvent event = event(1, MotionEvent.ACTION_MOVE, 9.5f, 8.5f, InputDevice.SOURCE_TOUCHSCREEN);
        view.mGestureRecognizer.mListener.onScroll(event, 100, -view.mRenderer.mFontLineSpacing);
        event.recycle();
        assertEquals("\033[<64;3;4M\033[<64;3;4M", output.text());
    }

    @Test
    public void trackingOffDoesNotSendHorizontalWheel() {
        scroll(4, 0);
        assertEquals("", output.text());
        assertEquals(0, view.getTopRow());
    }

    @Test
    public void mouseDragStillSendsMotionRatherThanWheel() {
        input("\033[?1002h\033[?1006h");
        MotionEvent event = event(1, MotionEvent.ACTION_MOVE, 2.5f, 3.5f, InputDevice.SOURCE_MOUSE);
        view.mGestureRecognizer.mListener.onScroll(event, 100, 0);
        event.recycle();
        assertEquals("\033[<32;3;4M", output.text());
    }

    @Test
    public void horizontalGestureDoesNotStartVerticalFling() {
        input("\033[?1000h\033[?1006h");
        scroll(2, 0);
        MotionEvent event = event(1, MotionEvent.ACTION_UP, 2.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        view.mGestureRecognizer.mListener.onFling(event, -1000, 2000);
        event.recycle();
        assertTrue(view.mScroller.isFinished());
        assertEquals("\033[<67;3;4M\033[<67;3;4M", output.text());
    }

    @Test
    public void nextDownClearsCancelledGestureFractionAndAxis() {
        input("\033[?1000h\033[?1006h");
        scroll(0.8f, 0);
        MotionEvent cancel = event(1, MotionEvent.ACTION_CANCEL, 2.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        view.onTouchEvent(cancel);
        cancel.recycle();
        MotionEvent down = event(2, MotionEvent.ACTION_DOWN, 2.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        view.onTouchEvent(down);
        down.recycle();
        scroll(0.3f, 0);
        assertEquals("", output.text());
    }

    @Test
    public void actualTouchSwipeDoesNotAlsoProduceVerticalWheelOrLeftTap() {
        input("\033[?1000h\033[?1006h");
        long time = SystemClock.uptimeMillis();
        MotionEvent down = event(time, MotionEvent.ACTION_DOWN, 30.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        view.onTouchEvent(down);
        down.recycle();
        MotionEvent move = event(time, MotionEvent.ACTION_MOVE, 10.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        view.onTouchEvent(move);
        move.recycle();
        MotionEvent up = event(time, MotionEvent.ACTION_UP, 10.5f, 3.5f, InputDevice.SOURCE_TOUCHSCREEN);
        view.onTouchEvent(up);
        up.recycle();
        assertFalse(output.text().isEmpty());
        assertFalse(output.text().contains("\033[<0;"));
        assertFalse(output.text().contains("\033[<64;"));
        assertFalse(output.text().contains("\033[<65;"));
        assertTrue(output.text().contains("\033[<67;"));
    }

    private static final class RecordingOutput extends TerminalOutput {
        private final ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        String text() { return new String(bytes.toByteArray(), StandardCharsets.UTF_8); }
        @Override public void write(byte[] data, int offset, int count) { bytes.write(data, offset, count); }
        @Override public void titleChanged(String oldTitle, String newTitle) { }
        @Override public void onCopyTextToClipboard(String text) { }
        @Override public void onPasteTextFromClipboard() { }
        @Override public void onBell() { }
        @Override public void onColorsChanged() { }
    }
}
