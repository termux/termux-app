package com.termux.view;

import android.app.Application;
import android.os.Looper;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.ViewConfiguration;

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
import org.robolectric.annotation.LooperMode;
import org.robolectric.Shadows;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28, manifest = Config.NONE, application = Application.class)
@GraphicsMode(GraphicsMode.Mode.NATIVE)
@LooperMode(LooperMode.Mode.PAUSED)
public class TerminalViewMouseTest {
    private final RecordingOutput output = new RecordingOutput();
    private final RecordingClient client = new RecordingClient();
    private TerminalView view;

    @Before
    public void setUp() {
        view = new TerminalView(RuntimeEnvironment.getApplication(), null);
        view.setTerminalViewClient(client);
        view.setTextSize(16);
        assertTrue("Gesture coordinates require real font width", view.mRenderer.mFontWidth > 0);
        assertTrue("Gesture coordinates require real line spacing", view.mRenderer.mFontLineSpacing > 0);
        view.mEmulator = new TerminalEmulator(output, 80, 24, 8, 16, null,
            new TermuxTerminalSessionClientBase());
    }

    private void input(String text) {
        byte[] bytes = text.getBytes(StandardCharsets.UTF_8);
        view.mEmulator.append(bytes, bytes.length);
    }

    private void longPress(int source) {
        long down = SystemClock.uptimeMillis();
        float x = 2.5f * view.mRenderer.mFontWidth;
        float y = view.mRenderer.mFontLineSpacingAndAscent + 3.5f * view.mRenderer.mFontLineSpacing;
        MotionEvent event = MotionEvent.obtain(down, down, MotionEvent.ACTION_DOWN, x, y, 0);
        event.setSource(source);
        view.onTouchEvent(event);
        event.recycle();
        Shadows.shadowOf(Looper.getMainLooper()).idleFor(
            ViewConfiguration.getTapTimeout() + ViewConfiguration.getLongPressTimeout() + 1,
            TimeUnit.MILLISECONDS);
        event = MotionEvent.obtain(down, SystemClock.uptimeMillis(), MotionEvent.ACTION_UP, x, y, 0);
        event.setSource(source);
        view.onTouchEvent(event);
        event.recycle();
    }

    @Test
    public void sgrLongPressReportsRightPressAndReleaseWithoutLeftTap() {
        input("\033[?1000h\033[?1006h");
        longPress(InputDevice.SOURCE_TOUCHSCREEN);
        assertEquals("\033[<2;3;4M\033[<2;3;4m", output.text());
        assertEquals(0, client.longPresses);
        assertEquals(0, client.taps);
        assertFalse(view.isSelectingText());
    }

    @Test
    public void legacyLongPressUsesExistingReleaseEncoding() {
        input("\033[?1002h");
        longPress(InputDevice.SOURCE_TOUCHSCREEN);
        assertEquals("\033[M\"#$\033[M##$", output.text());
        assertEquals(0, client.longPresses);
    }

    @Test
    public void trackingOffKeepsClientLongPressAndDoesNotEmitMouseInput() {
        longPress(InputDevice.SOURCE_TOUCHSCREEN);
        assertEquals("", output.text());
        assertEquals(1, client.longPresses);
        assertEquals(0, client.taps);
    }

    @Test
    public void mouseDeviceLongPressKeepsClientCallbackAndLeftButtonPath() {
        input("\033[?1000h\033[?1006h");
        longPress(InputDevice.SOURCE_MOUSE);
        assertEquals("\033[<0;3;4M\033[<0;3;4m", output.text());
        assertEquals(1, client.longPresses);
    }

    private static final class RecordingClient extends TermuxTerminalViewClientBase {
        int longPresses;
        int taps;

        @Override
        public boolean onLongPress(MotionEvent event) {
            longPresses++;
            return true;
        }

        @Override
        public void onSingleTapUp(MotionEvent event) {
            taps++;
        }
    }

    private static final class RecordingOutput extends TerminalOutput {
        private final ByteArrayOutputStream bytes = new ByteArrayOutputStream();

        String text() {
            return new String(bytes.toByteArray(), StandardCharsets.UTF_8);
        }

        @Override public void write(byte[] data, int offset, int count) { bytes.write(data, offset, count); }
        @Override public void titleChanged(String oldTitle, String newTitle) { }
        @Override public void onCopyTextToClipboard(String text) { }
        @Override public void onPasteTextFromClipboard() { }
        @Override public void onBell() { }
        @Override public void onColorsChanged() { }
    }
}
