package com.termux.view;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.text.InputType;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;

import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSessionClient;

import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.GraphicsMode;

/** Exercises the real Android input-connection contract; no source-text assertions. */
@RunWith(RobolectricTestRunner.class)
@Config(sdk = {28, 35})
@GraphicsMode(GraphicsMode.Mode.LEGACY)
public class TerminalViewImeTest {
    @Test
    public void optInAdvertisesNonPasswordTextForCjkIme() {
        TestClient client = new TestClient();
        client.enforceCharBasedInput = true;
        TerminalView view = new TerminalView(RuntimeEnvironment.getApplication(), null);
        view.setTerminalViewClient(client);
        EditorInfo info = new EditorInfo();

        assertTrue(view.onCheckIsTextEditor());
        assertNotNull(view.onCreateInputConnection(info));
        assertEquals("CJK IMEs need a text editor, not the TYPE_NULL/key-event contract",
            InputType.TYPE_CLASS_TEXT, info.inputType & InputType.TYPE_MASK_CLASS);
        assertEquals("Password variations can disable CJK composition",
            InputType.TYPE_TEXT_VARIATION_NORMAL, info.inputType & InputType.TYPE_MASK_VARIATION);
        assertEquals(InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS,
            info.inputType & InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS);
        assertEquals(EditorInfo.IME_FLAG_NO_FULLSCREEN, info.imeOptions);
    }

    @Test
    public void defaultKeepsLegacyKeyEventContract() {
        TerminalView view = createView(new TestClient());
        EditorInfo info = new EditorInfo();
        assertNotNull(view.onCreateInputConnection(info));
        assertEquals(InputType.TYPE_NULL, info.inputType);
        assertEquals(EditorInfo.IME_FLAG_NO_FULLSCREEN, info.imeOptions);
    }

    @Test
    public void alternateViewKeepsNormalTextContract() {
        TestClient client = new TestClient();
        client.enforceCharBasedInput = true;
        client.terminalViewSelected = false;
        TerminalView view = createView(client);
        EditorInfo info = new EditorInfo();
        assertNotNull(view.onCreateInputConnection(info));
        assertEquals(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_NORMAL, info.inputType);
    }

    @Test
    public void commitsCjkAndSupplementaryUnicodeAsUtf8ExactlyOnce() throws Exception {
        TerminalView view = createViewWithSession();
        InputConnection connection = view.onCreateInputConnection(new EditorInfo());
        String text = "\u4e2d\u6587\u65e5\u672c\u8a9e\ud55c\uae00\ud840\udc00\ud83d\ude00";
        assertTrue(connection.commitText(text, 1));
        assertTerminalInput(view, text);
        assertTrue(connection.finishComposingText());
        assertTerminalInput(view, "");
    }

    @Test
    public void composingRevisionsDoNotLeakToTerminalBeforeCommit() throws Exception {
        TerminalView view = createViewWithSession();
        InputConnection connection = view.onCreateInputConnection(new EditorInfo());
        assertTrue(connection.setComposingText("zhong", 1));
        assertTerminalInput(view, "");
        assertTrue(connection.setComposingText("\u4e2d", 1));
        assertTerminalInput(view, "");
        assertTrue(connection.commitText("\u4e2d", 1));
        assertTerminalInput(view, "\u4e2d");
        assertTrue(connection.finishComposingText());
        assertTerminalInput(view, "");
    }

    @Test
    public void finishingCompositionFlushesPendingTextExactlyOnce() throws Exception {
        TerminalView view = createViewWithSession();
        InputConnection connection = view.onCreateInputConnection(new EditorInfo());
        assertTrue(connection.setComposingText("\u4e2d\u6587", 1));
        assertTerminalInput(view, "");
        assertTrue(connection.finishComposingText());
        assertTerminalInput(view, "\u4e2d\u6587");
        assertTrue(connection.finishComposingText());
        assertTerminalInput(view, "");
    }

    @Test
    public void hardwareCtrlCSendsEtx() throws Exception {
        assertHardwareInput(KeyEvent.KEYCODE_C,
            KeyEvent.META_CTRL_ON | KeyEvent.META_CTRL_LEFT_ON, "\u0003");
    }

    @Test
    public void hardwareLeftAltBSendsEscapePrefix() throws Exception {
        assertHardwareInput(KeyEvent.KEYCODE_B,
            KeyEvent.META_ALT_ON | KeyEvent.META_ALT_LEFT_ON, "\u001bb");
    }

    @Test
    public void hardwareBackspaceSendsDel() throws Exception {
        assertHardwareInput(KeyEvent.KEYCODE_DEL, 0, "\u007f");
    }

    @Test
    public void hardwareEnterSendsCarriageReturn() throws Exception {
        assertHardwareInput(KeyEvent.KEYCODE_ENTER, 0, "\r");
    }

    @Test
    public void imeNewlineSendsCarriageReturn() throws Exception {
        TerminalView view = createViewWithSession();
        InputConnection connection = view.onCreateInputConnection(new EditorInfo());
        assertTrue(connection.commitText("\n", 1));
        assertTerminalInput(view, "\r");
    }

    private static TerminalView createView(TestClient client) {
        TerminalView view = new TerminalView(RuntimeEnvironment.getApplication(), null);
        view.setTerminalViewClient(client);
        return view;
    }

    private static TerminalView createViewWithSession() throws Exception {
        TestClient client = new TestClient();
        client.enforceCharBasedInput = true;
        TerminalView view = createView(client);
        TerminalSession session = new TerminalSession("", "", new String[0], new String[0], 100, client);
        TerminalEmulator emulator = new TerminalEmulator(session, 80, 24, 8, 16, 100, client);
        // Keep the real encoding and byte queue; only bypass native PTY/process creation.
        // No initializeEmulator(), updateSize(), finishIfRunning(), or JNI calls in this fixture.
        Field emulatorField = TerminalSession.class.getDeclaredField("mEmulator");
        emulatorField.setAccessible(true);
        emulatorField.set(session, emulator);
        Field pidField = TerminalSession.class.getDeclaredField("mShellPid");
        pidField.setAccessible(true);
        pidField.setInt(session, 1); // Enables the in-memory write queue; not an actual process.
        view.mTermSession = session;
        view.mEmulator = emulator;
        return view;
    }

    private static void assertTerminalInput(TerminalView view, String expected) throws Exception {
        Field queueField = TerminalSession.class.getDeclaredField("mTerminalToProcessIOQueue");
        queueField.setAccessible(true);
        Object queue = queueField.get(view.mTermSession);
        Method read = queue.getClass().getDeclaredMethod("read", byte[].class, boolean.class);
        read.setAccessible(true);
        byte[] bytes = new byte[4096];
        int length = (int) read.invoke(queue, bytes, false);
        assertTrue("Terminal input queue must remain open", length >= 0);
        assertArrayEquals(expected.getBytes(StandardCharsets.UTF_8), Arrays.copyOf(bytes, length));
    }

    private static void assertHardwareInput(int keyCode, int metaState, String expected) throws Exception {
        TerminalView view = createViewWithSession();
        assertNotNull(view.onCreateInputConnection(new EditorInfo()));
        KeyEvent event = new KeyEvent(0, 0, KeyEvent.ACTION_DOWN, keyCode, 0,
            metaState, 1, 0, 0, InputDevice.SOURCE_KEYBOARD);
        assertTrue(view.onKeyDown(keyCode, event));
        assertTerminalInput(view, expected);
    }

    private static final class TestClient implements TerminalViewClient, TerminalSessionClient {
        boolean enforceCharBasedInput;
        boolean terminalViewSelected = true;

        @Override public void onTextChanged(TerminalSession session) {}
        @Override public void onTitleChanged(TerminalSession session) {}
        @Override public void onSessionFinished(TerminalSession session) {}
        @Override public void onCopyTextToClipboard(TerminalSession session, String text) {}
        @Override public void onPasteTextFromClipboard(TerminalSession session) {}
        @Override public void onBell(TerminalSession session) {}
        @Override public void onColorsChanged(TerminalSession session) {}
        @Override public void onTerminalCursorStateChange(boolean state) {}
        @Override public void setTerminalShellPid(TerminalSession session, int pid) {}
        @Override public Integer getTerminalCursorStyle() { return null; }

        @Override public float onScale(float scale) { return scale; }
        @Override public void onSingleTapUp(MotionEvent e) {}
        @Override public boolean shouldBackButtonBeMappedToEscape() { return false; }
        @Override public boolean shouldEnforceCharBasedInput() { return enforceCharBasedInput; }
        @Override public boolean shouldUseCtrlSpaceWorkaround() { return false; }
        @Override public boolean isTerminalViewSelected() { return terminalViewSelected; }
        @Override public void copyModeChanged(boolean copyMode) {}
        @Override public boolean onKeyDown(int keyCode, KeyEvent e, TerminalSession session) { return false; }
        @Override public boolean onKeyUp(int keyCode, KeyEvent e) { return false; }
        @Override public boolean onLongPress(MotionEvent event) { return false; }
        @Override public boolean readControlKey() { return false; }
        @Override public boolean readAltKey() { return false; }
        @Override public boolean readShiftKey() { return false; }
        @Override public boolean readFnKey() { return false; }
        @Override public boolean onCodePoint(int codePoint, boolean ctrlDown, TerminalSession session) { return false; }
        @Override public void onEmulatorSet() {}
        @Override public void logError(String tag, String message) {}
        @Override public void logWarn(String tag, String message) {}
        @Override public void logInfo(String tag, String message) {}
        @Override public void logDebug(String tag, String message) {}
        @Override public void logVerbose(String tag, String message) {}
        @Override public void logStackTraceWithMessage(String tag, String message, Exception e) {}
        @Override public void logStackTrace(String tag, Exception e) {}
    }
}
