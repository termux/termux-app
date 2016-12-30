package com.termux.app;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.support.v4.content.WakefulBroadcastReceiver;
import android.util.Log;
import android.widget.ArrayAdapter;

import com.termux.R;
import com.termux.terminal.EmulatorDebug;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSession.SessionChangedCallback;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

/**
 * A service holding a list of terminal sessions, {@link #mTerminalSessions}, showing a foreground notification while
 * running so that it is not terminated. The user interacts with the session through {@link TermuxActivity}, but this
 * service may outlive the activity when the user or the system disposes of the activity. In that case the user may
 * restart {@link TermuxActivity} later to yet again access the sessions.
 * <p/>
 * In order to keep both terminal sessions and spawned processes (who may outlive the terminal sessions) alive as long
 * as wanted by the user this service is a foreground service, {@link Service#startForeground(int, Notification)}.
 * <p/>
 * Optionally may hold a wake and a wifi lock, in which case that is shown in the notification - see
 * {@link #buildNotification()}.
 */
public final class TermuxService extends Service implements SessionChangedCallback {

    /** Note that this is a symlink on the Android M preview. */
    @SuppressLint("SdCardPath")
    public static final String FILES_PATH = "/data/data/com.termux/files";
    public static final String PREFIX_PATH = FILES_PATH + "/usr";
    public static final String HOME_PATH = FILES_PATH + "/home";

    private static final int NOTIFICATION_ID = 1337;

    private static final String ACTION_STOP_SERVICE = "com.termux.service_stop";
    private static final String ACTION_LOCK_WAKE = "com.termux.service_wake_lock";
    private static final String ACTION_UNLOCK_WAKE = "com.termux.service_wake_unlock";
    /** Intent action to launch a new terminal session. Executed from TermuxWidgetProvider. */
    public static final String ACTION_EXECUTE = "com.termux.service_execute";

    public static final String EXTRA_ARGUMENTS = "com.termux.execute.arguments";

    public static final String EXTRA_CURRENT_WORKING_DIRECTORY = "com.termux.execute.cwd";
    private static final String EXTRA_EXECUTE_IN_BACKGROUND = "com.termux.execute.background";

    /** This service is only bound from inside the same process and never uses IPC. */
    class LocalBinder extends Binder {
        public final TermuxService service = TermuxService.this;
    }

    private final IBinder mBinder = new LocalBinder();

    private final Handler mHandler = new Handler();

    /**
     * The terminal sessions which this service manages.
     * <p/>
     * Note that this list is observed by {@link TermuxActivity#mListViewAdapter}, so any changes must be made on the UI
     * thread and followed by a call to {@link ArrayAdapter#notifyDataSetChanged()} }.
     */
    final List<TerminalSession> mTerminalSessions = new ArrayList<>();

    final List<BackgroundJob> mBackgroundTasks = new ArrayList<>();

    /** Note that the service may often outlive the activity, so need to clear this reference. */
    SessionChangedCallback mSessionChangeCallback;

    /** The wake lock and wifi lock are always acquired and released together. */
    private PowerManager.WakeLock mWakeLock;
    private WifiManager.WifiLock mWifiLock;

    /** If the user has executed the {@link #ACTION_STOP_SERVICE} intent. */
    boolean mWantsToStop = false;

    @SuppressLint("Wakelock")
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String action = intent.getAction();
        if (ACTION_STOP_SERVICE.equals(action)) {
            mWantsToStop = true;
            for (int i = 0; i < mTerminalSessions.size(); i++)
                mTerminalSessions.get(i).finishIfRunning();
            stopSelf();
        } else if (ACTION_LOCK_WAKE.equals(action)) {
            if (mWakeLock == null) {
                PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
                mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, EmulatorDebug.LOG_TAG);
                mWakeLock.acquire();

                WifiManager wm = (WifiManager) getSystemService(Context.WIFI_SERVICE);
                mWifiLock = wm.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, EmulatorDebug.LOG_TAG);
                mWifiLock.acquire();

                updateNotification();
            }
        } else if (ACTION_UNLOCK_WAKE.equals(action)) {
            if (mWakeLock != null) {
                mWakeLock.release();
                mWakeLock = null;

                mWifiLock.release();
                mWifiLock = null;

                updateNotification();
            }
        } else if (ACTION_EXECUTE.equals(action)) {
            Uri executableUri = intent.getData();
            String executablePath = (executableUri == null ? null : executableUri.getPath());

            String[] arguments = (executableUri == null ? null : intent.getStringArrayExtra(EXTRA_ARGUMENTS));
            String cwd = intent.getStringExtra(EXTRA_CURRENT_WORKING_DIRECTORY);

            if (intent.getBooleanExtra(EXTRA_EXECUTE_IN_BACKGROUND, false)) {
                BackgroundJob task = new BackgroundJob(cwd, executablePath, arguments, this);
                mBackgroundTasks.add(task);
                updateNotification();
            } else {
                TerminalSession newSession = createTermSession(executablePath, arguments, cwd, false);

                // Transform executable path to session name, e.g. "/bin/do-something.sh" => "do something.sh".
                if (executablePath != null) {
                    int lastSlash = executablePath.lastIndexOf('/');
                    String name = (lastSlash == -1) ? executablePath : executablePath.substring(lastSlash + 1);
                    name = name.replace('-', ' ');
                    newSession.mSessionName = name;
                }

                // Make the newly created session the current one to be displayed:
                TermuxPreferences.storeCurrentSession(this, newSession);

                // Launch the main Termux app, which will now show the current session:
                startActivity(new Intent(this, TermuxActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
            }
        } else if (action != null) {
            Log.e(EmulatorDebug.LOG_TAG, "Unknown TermuxService action: '" + action + "'");
        }

        if ((flags & START_FLAG_REDELIVERY) == 0) {
            // Service is started by WBR, not restarted by system, so release the WakeLock from WBR.
            WakefulBroadcastReceiver.completeWakefulIntent(intent);
        }

        // If this service really do get killed, there is no point restarting it automatically - let the user do on next
        // start of {@link Term):
        return Service.START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        startForeground(NOTIFICATION_ID, buildNotification());
    }

    /** Update the shown foreground service notification after making any changes that affect it. */
    private void updateNotification() {
        if (mWakeLock == null && mTerminalSessions.isEmpty() && mBackgroundTasks.isEmpty()) {
            // Exit if we are updating after the user disabled all locks with no sessions or tasks running.
            stopSelf();
        } else {
            ((NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE)).notify(NOTIFICATION_ID, buildNotification());
        }
    }

    private Notification buildNotification() {
        Intent notifyIntent = new Intent(this, TermuxActivity.class);
        // PendingIntent#getActivity(): "Note that the activity will be started outside of the context of an existing
        // activity, so you must use the Intent.FLAG_ACTIVITY_NEW_TASK launch flag in the Intent":
        notifyIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notifyIntent, 0);

        int sessionCount = mTerminalSessions.size();
        int taskCount = mBackgroundTasks.size();
        String contentText = sessionCount + " session" + (sessionCount == 1 ? "" : "s");
        if (taskCount > 0) {
            contentText += ", " + taskCount + " task" + (taskCount == 1 ? "" : "s");
        }

        final boolean wakeLockHeld = mWakeLock != null;
        if (wakeLockHeld) contentText += " (wake lock held)";

        Notification.Builder builder = new Notification.Builder(this);
        builder.setContentTitle(getText(R.string.application_name));
        builder.setContentText(contentText);
        builder.setSmallIcon(R.drawable.ic_service_notification);
        builder.setContentIntent(pendingIntent);
        builder.setOngoing(true);

        // If holding a wake or wifi lock consider the notification of high priority since it's using power,
        // otherwise use a minimal priority since this is just a background service notification:
        builder.setPriority((wakeLockHeld) ? Notification.PRIORITY_HIGH : Notification.PRIORITY_MIN);

        // No need to show a timestamp:
        builder.setShowWhen(false);

        // Background color for small notification icon:
        builder.setColor(0xFF000000);

        Resources res = getResources();
        Intent exitIntent = new Intent(this, TermuxService.class).setAction(ACTION_STOP_SERVICE);
        builder.addAction(android.R.drawable.ic_delete, res.getString(R.string.notification_action_exit), PendingIntent.getService(this, 0, exitIntent, 0));

        String newWakeAction = wakeLockHeld ? ACTION_UNLOCK_WAKE : ACTION_LOCK_WAKE;
        Intent toggleWakeLockIntent = new Intent(this, TermuxService.class).setAction(newWakeAction);
        String actionTitle = res.getString(wakeLockHeld ?
            R.string.notification_action_wake_unlock :
            R.string.notification_action_wake_lock);
        int actionIcon = wakeLockHeld ? android.R.drawable.ic_lock_idle_lock : android.R.drawable.ic_lock_lock;
        builder.addAction(actionIcon, actionTitle, PendingIntent.getService(this, 0, toggleWakeLockIntent, 0));

        return builder.build();
    }

    @Override
    public void onDestroy() {
        if (mWakeLock != null) mWakeLock.release();
        if (mWifiLock != null) mWifiLock.release();

        stopForeground(true);

        for (int i = 0; i < mTerminalSessions.size(); i++)
            mTerminalSessions.get(i).finishIfRunning();
        mTerminalSessions.clear();
    }

    public List<TerminalSession> getSessions() {
        return mTerminalSessions;
    }

    TerminalSession createTermSession(String executablePath, String[] arguments, String cwd, boolean failSafe) {
        new File(HOME_PATH).mkdirs();

        if (cwd == null) cwd = HOME_PATH;

        String[] env = BackgroundJob.buildEnvironment(failSafe, cwd);
        boolean isLoginShell = false;

        if (executablePath == null) {
            File shell = new File(HOME_PATH, ".termux/shell");
            if (shell.exists()) {
                try {
                    File canonicalFile = shell.getCanonicalFile();
                    if (canonicalFile.isFile() && canonicalFile.canExecute()) {
                        executablePath = canonicalFile.getName().equals("busybox") ? (PREFIX_PATH + "/bin/ash") : canonicalFile.getAbsolutePath();
                    } else {
                        Log.w(EmulatorDebug.LOG_TAG, "$HOME/.termux/shell points to non-executable shell: " + canonicalFile.getAbsolutePath());
                    }
                } catch (IOException e) {
                    Log.e(EmulatorDebug.LOG_TAG, "Error checking $HOME/.termux/shell", e);
                }
            }

            if (executablePath == null) {
                // Try bash, zsh and ash in that order:
                for (String shellBinary : new String[]{"bash", "zsh", "ash"}) {
                    File shellFile = new File(PREFIX_PATH + "/bin/" + shellBinary);
                    if (shellFile.canExecute()) {
                        executablePath = shellFile.getAbsolutePath();
                        break;
                    }
                }
            }

            if (executablePath == null) {
                // Fall back to system shell as last resort:
                executablePath = "/system/bin/sh";
            }
            isLoginShell = true;
        }

        String[] processArgs = BackgroundJob.setupProcessArgs(executablePath, arguments);
        executablePath = processArgs[0];
        int lastSlashIndex = executablePath.lastIndexOf('/');
        String processName = (isLoginShell ? "-" : "") +
            (lastSlashIndex == -1 ? executablePath : executablePath.substring(lastSlashIndex + 1));

        String[] args = new String[processArgs.length];
        args[0] = processName;
        if (processArgs.length > 1) System.arraycopy(processArgs, 1, args, 1, processArgs.length - 1);

        TerminalSession session = new TerminalSession(executablePath, cwd, args, env, this);
        mTerminalSessions.add(session);
        updateNotification();
        return session;
    }

    public int removeTermSession(TerminalSession sessionToRemove) {
        int indexOfRemoved = mTerminalSessions.indexOf(sessionToRemove);
        mTerminalSessions.remove(indexOfRemoved);
        if (mTerminalSessions.isEmpty() && mWakeLock == null) {
            // Finish if there are no sessions left and the wake lock is not held, otherwise keep the service alive if
            // holding wake lock since there may be daemon processes (e.g. sshd) running.
            stopSelf();
        } else {
            updateNotification();
        }
        return indexOfRemoved;
    }

    @Override
    public void onTitleChanged(TerminalSession changedSession) {
        if (mSessionChangeCallback != null) mSessionChangeCallback.onTitleChanged(changedSession);
    }

    @Override
    public void onSessionFinished(final TerminalSession finishedSession) {
        if (mSessionChangeCallback != null)
            mSessionChangeCallback.onSessionFinished(finishedSession);
    }

    @Override
    public void onTextChanged(TerminalSession changedSession) {
        if (mSessionChangeCallback != null) mSessionChangeCallback.onTextChanged(changedSession);
    }

    @Override
    public void onClipboardText(TerminalSession session, String text) {
        if (mSessionChangeCallback != null) mSessionChangeCallback.onClipboardText(session, text);
    }

    @Override
    public void onBell(TerminalSession session) {
        if (mSessionChangeCallback != null) mSessionChangeCallback.onBell(session);
    }

    @Override
    public void onColorsChanged(TerminalSession session) {
        if (mSessionChangeCallback != null) mSessionChangeCallback.onColorsChanged(session);
    }

    public void onBackgroundJobExited(final BackgroundJob task) {
        mHandler.post(new Runnable() {
            @Override
            public void run() {
                mBackgroundTasks.remove(task);
                updateNotification();
            }
        });
    }
}
