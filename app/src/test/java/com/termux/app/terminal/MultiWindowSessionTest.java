package com.termux.app.terminal;

import android.app.Application;

import com.termux.app.TermuxActivity;
import com.termux.app.TermuxService;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.termux.shell.TermuxShellManager;
import com.termux.shared.termux.shell.command.runner.terminal.TermuxSession;
import com.termux.terminal.TerminalSession;
import com.termux.view.TerminalView;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.util.ReflectionHelpers;

import java.lang.reflect.Constructor;

import static org.junit.Assert.*;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 28, application = Application.class)
public class MultiWindowSessionTest {
    private TermuxService service;
    private TermuxShellManager shells;

    @Before
    public void setUp() {
        service = Robolectric.buildService(TermuxService.class).get();
        shells = new TermuxShellManager(RuntimeEnvironment.getApplication());
        ReflectionHelpers.setField(service, "mShellManager", shells);
    }

    private TestClient window() {
        TermuxActivity activity = Robolectric.buildActivity(TermuxActivity.class).get();
        ReflectionHelpers.setField(activity, "mTermuxService", service);
        ReflectionHelpers.setField(activity, "mTerminalView", new TerminalView(activity, null));
        TestClient client = new TestClient(activity);
        ReflectionHelpers.setField(activity, "mTermuxTerminalSessionActivityClient", client);
        service.setTermuxTerminalSessionClient(client);
        return client;
    }

    private TerminalSession session() throws Exception {
        TerminalSession session = new TerminalSession("/bin/sh", "/", new String[0],
            new String[0], 100, service.getTermuxTerminalSessionClient());
        // Register sessions without launching an Android native shell in the JVM test.
        Constructor<TermuxSession> constructor = TermuxSession.class.getDeclaredConstructor(
            TerminalSession.class, ExecutionCommand.class, TermuxSession.TermuxSessionClient.class, boolean.class);
        constructor.setAccessible(true);
        shells.mTermuxSessions.add(constructor.newInstance(session, new ExecutionCommand(), service, false));
        return session;
    }

    @Test
    public void rejectedSwitchPreservesBothWindowsAndCallbackOwners() throws Exception {
        TestClient first = window();
        TestClient second = window();
        TerminalSession a = session();
        TerminalSession b = session();
        first.setCurrentSession(a);
        second.setCurrentSession(b);

        first.setCurrentSession(b);

        assertSame(a, first.getActivity().getCurrentSession());
        assertSame(b, second.getActivity().getCurrentSession());
        assertSame(first, ReflectionHelpers.getField(a, "mClient"));
        assertSame(second, ReflectionHelpers.getField(b, "mClient"));
        assertTrue(service.isSessionAttachedToOther(a, second.getActivity().getActivityId()));
    }

    @Test
    public void selectingCurrentSessionDoesNotAnnounceASwitch() throws Exception {
        TestClient first = window();
        TerminalSession a = session();
        first.setCurrentSession(a);
        first.changes = 0;

        first.setCurrentSession(a);

        assertEquals(0, first.changes);
    }

    @Test
    public void restartingWindowAfterExitDoesNotStealOtherWindowsSession() throws Exception {
        TestClient first = window();
        TestClient second = window();
        TestClient third = window();
        TerminalSession a = session();
        TerminalSession b = session();
        TerminalSession c = session();
        first.setCurrentSession(a);
        second.setCurrentSession(b);
        third.setCurrentSession(c);
        shells.mTermuxSessions.remove(service.getTermuxSessionForTerminalSession(b));
        second.stored = c;

        second.onStart();

        assertTrue(second.getActivity().isFinishing());
        assertSame(c, third.getActivity().getCurrentSession());
        assertSame(third, ReflectionHelpers.getField(c, "mClient"));
        assertSame(a, first.getActivity().getCurrentSession());
    }

    @Test
    public void backgroundSessionExitKeepsCurrentWindowOpen() throws Exception {
        TestClient first = window();
        TerminalSession background = session();
        TerminalSession current = session();
        first.setCurrentSession(background);
        first.setCurrentSession(current);
        // Model a session already removed by the service's exit callback.
        shells.mTermuxSessions.remove(service.getTermuxSessionForTerminalSession(background));

        first.removeFinishedSession(background);

        assertFalse(first.getActivity().isFinishing());
        assertSame(current, first.getActivity().getCurrentSession());
    }

    @Test
    public void closingWindowRehomesBackgroundCallbacksAndPreservesOtherWindow() throws Exception {
        TestClient first = window();
        TestClient second = window();
        TerminalSession background = session();
        TerminalSession a = session();
        TerminalSession b = session();
        first.setCurrentSession(background);
        first.setCurrentSession(a);
        second.setCurrentSession(b);

        service.detachAllSessionsForActivity(first.getActivity().getActivityId());
        service.removeTermuxTerminalSessionClient(first);

        assertSame(second, ReflectionHelpers.getField(background, "mClient"));
        assertSame(second, ReflectionHelpers.getField(b, "mClient"));
        assertSame(b, second.getActivity().getCurrentSession());
        assertFalse(service.isSessionAttached(a));
    }

    @Test
    public void switchingToUnattachedSessionReleasesPreviousOwnership() throws Exception {
        TestClient first = window();
        TestClient second = window();
        TerminalSession a = session();
        TerminalSession b = session();
        first.setCurrentSession(a);

        first.setCurrentSession(b);
        second.setCurrentSession(a);

        assertSame(b, first.getActivity().getCurrentSession());
        assertSame(a, second.getActivity().getCurrentSession());
        assertSame(first, ReflectionHelpers.getField(b, "mClient"));
        assertSame(second, ReflectionHelpers.getField(a, "mClient"));
    }

    @Test
    public void unbindingAllWindowsReleasesOwnership() throws Exception {
        TestClient first = window();
        TerminalSession a = session();
        first.setCurrentSession(a);

        service.unsetTermuxTerminalSessionClient();

        assertFalse(service.isSessionAttached(a));
    }

    private static class TestClient extends TermuxTerminalSessionActivityClient {
        int changes;
        TerminalSession stored;

        TestClient(TermuxActivity activity) { super(activity); }

        @Override void notifyOfSessionChange() { changes++; }
        @Override public void termuxSessionListNotifyUpdated() {}
        @Override public TerminalSession getCurrentStoredSessionOrLast() { return stored; }
    }
}
