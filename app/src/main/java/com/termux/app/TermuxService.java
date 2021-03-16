package com.termux.app;

import android.annotation.SuppressLint;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.net.Uri;
import android.net.wifi.WifiManager;
import android.os.Binder;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.provider.Settings;
import android.widget.ArrayAdapter;

import com.termux.R;
import com.termux.app.TermuxConstants.TERMUX_APP.TERMUX_ACTIVITY;
import com.termux.app.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.app.settings.preferences.TermuxSharedPreferences;
import com.termux.app.terminal.TermuxSessionClient;
import com.termux.app.terminal.TermuxSessionClientBase;
import com.termux.app.utils.Logger;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSessionClient;

import java.io.File;
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
public final class TermuxService extends Service {

    private static final String NOTIFICATION_CHANNEL_ID = "termux_notification_channel";
    private static final int NOTIFICATION_ID = 1337;

    /** This service is only bound from inside the same process and never uses IPC. */
    class LocalBinder extends Binder {
        public final TermuxService service = TermuxService.this;
    }

    private final IBinder mBinder = new LocalBinder();

    private final Handler mHandler = new Handler();

    /**
     * The terminal sessions which this service manages.
     * <p/>
     * Note that this list is observed by {@link TermuxActivity#mTermuxSessionListViewController}, so any changes must be made on the UI
     * thread and followed by a call to {@link ArrayAdapter#notifyDataSetChanged()} }.
     */
    final List<TerminalSession> mTerminalSessions = new ArrayList<>();

    final List<BackgroundJob> mBackgroundTasks = new ArrayList<>();

    /** The full implementation of the {@link TerminalSessionClient} interface to be used by {@link TerminalSession}
     * that holds activity references for activity related functions.
     * Note that the service may often outlive the activity, so need to clear this reference.
     */
    TermuxSessionClient mTermuxSessionClient;

    /** The basic implementation of the {@link TerminalSessionClient} interface to be used by {@link TerminalSession}
     * that does not hold activity references.
     */
    final TermuxSessionClientBase mTermuxSessionClientBase = new TermuxSessionClientBase();;

    /** The wake lock and wifi lock are always acquired and released together. */
    private PowerManager.WakeLock mWakeLock;
    private WifiManager.WifiLock mWifiLock;

    /** If the user has executed the {@link TermuxConstants.TERMUX_APP.TERMUX_SERVICE#ACTION_STOP_SERVICE} intent. */
    boolean mWantsToStop = false;

    private static final String LOG_TAG = "TermuxService";

    @SuppressLint("Wakelock")
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String action = intent.getAction();
        if (TERMUX_SERVICE.ACTION_STOP_SERVICE.equals(action)) {
            mWantsToStop = true;
            for (int i = 0; i < mTerminalSessions.size(); i++)
                mTerminalSessions.get(i).finishIfRunning();
            stopSelf();
        } else if (TERMUX_SERVICE.ACTION_WAKE_LOCK.equals(action)) {
            if (mWakeLock == null) {
                PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
                mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TermuxConstants.TERMUX_APP_NAME.toLowerCase() + ":service-wakelock");
                mWakeLock.acquire();

                // http://tools.android.com/tech-docs/lint-in-studio-2-3#TOC-WifiManager-Leak
                WifiManager wm = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
                mWifiLock = wm.createWifiLock(WifiManager.WIFI_MODE_FULL_HIGH_PERF, TermuxConstants.TERMUX_APP_NAME.toLowerCase());
                mWifiLock.acquire();

                String packageName = getPackageName();
                if (!pm.isIgnoringBatteryOptimizations(packageName)) {
                    Intent whitelist = new Intent();
                    whitelist.setAction(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
                    whitelist.setData(Uri.parse("package:" + packageName));
                    whitelist.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);

                    try {
                        startActivity(whitelist);
                    } catch (ActivityNotFoundException e) {
                        Logger.logStackTraceWithMessage(LOG_TAG, "Failed to call ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS", e);
                    }
                }

                updateNotification();
            }
        } else if (TERMUX_SERVICE.ACTION_WAKE_UNLOCK.equals(action)) {
            if (mWakeLock != null) {
                mWakeLock.release();
                mWakeLock = null;

                mWifiLock.release();
                mWifiLock = null;

                updateNotification();
            }
        } else if (TERMUX_SERVICE.ACTION_SERVICE_EXECUTE.equals(action)) {
            Uri executableUri = intent.getData();
            String executablePath = (executableUri == null ? null : executableUri.getPath());

            String[] arguments = (executableUri == null ? null : intent.getStringArrayExtra(TERMUX_SERVICE.EXTRA_ARGUMENTS));
            String cwd = intent.getStringExtra(TERMUX_SERVICE.EXTRA_WORKDIR);

            if (intent.getBooleanExtra(TERMUX_SERVICE.EXTRA_BACKGROUND, false)) {
                BackgroundJob task = new BackgroundJob(cwd, executablePath, arguments, this, intent.getParcelableExtra("pendingIntent"));
                mBackgroundTasks.add(task);
                updateNotification();
            } else {
                boolean failsafe = intent.getBooleanExtra(TERMUX_ACTIVITY.ACTION_FAILSAFE_SESSION, false);
                TerminalSession newSession = createTermSession(executablePath, arguments, cwd, failsafe);

                // Transform executable path to session name, e.g. "/bin/do-something.sh" => "do something.sh".
                if (executablePath != null) {
                    int lastSlash = executablePath.lastIndexOf('/');
                    String name = (lastSlash == -1) ? executablePath : executablePath.substring(lastSlash + 1);
                    name = name.replace('-', ' ');
                    newSession.mSessionName = name;
                }

                // Make the newly created session the current one to be displayed:
                TermuxSharedPreferences preferences = new TermuxSharedPreferences(this);
                preferences.setCurrentSession(newSession.mHandle);

                // Launch the main Termux app, which will now show the current session:
                startActivity(new Intent(this, TermuxActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
            }
        } else if (action != null) {
            Logger.logError(LOG_TAG, "Unknown TermuxService action: '" + action + "'");
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
        setupNotificationChannel();
        startForeground(NOTIFICATION_ID, buildNotification());
    }

    /** Update the shown foreground service notification after making any changes that affect it. */
    void updateNotification() {
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
        // otherwise use a low priority
        builder.setPriority((wakeLockHeld) ? Notification.PRIORITY_HIGH : Notification.PRIORITY_LOW);

        // No need to show a timestamp:
        builder.setShowWhen(false);

        // Background color for small notification icon:
        builder.setColor(0xFF607D8B);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            builder.setChannelId(NOTIFICATION_CHANNEL_ID);
        }

        Resources res = getResources();
        Intent exitIntent = new Intent(this, TermuxService.class).setAction(TERMUX_SERVICE.ACTION_STOP_SERVICE);
        builder.addAction(android.R.drawable.ic_delete, res.getString(R.string.notification_action_exit), PendingIntent.getService(this, 0, exitIntent, 0));

        String newWakeAction = wakeLockHeld ? TERMUX_SERVICE.ACTION_WAKE_UNLOCK : TERMUX_SERVICE.ACTION_WAKE_LOCK;
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
        File termuxTmpDir = TermuxConstants.TERMUX_TMP_DIR;

        if (termuxTmpDir.exists()) {
            try {
                TermuxInstaller.deleteFolder(termuxTmpDir.getCanonicalFile());
            } catch (Exception e) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Error while removing file at " + termuxTmpDir.getAbsolutePath(), e);
            }

            termuxTmpDir.mkdirs();
        }

        if (mWakeLock != null) mWakeLock.release();
        if (mWifiLock != null) mWifiLock.release();

        stopForeground(true);

        for (int i = 0; i < mTerminalSessions.size(); i++)
            mTerminalSessions.get(i).finishIfRunning();
    }

    public List<TerminalSession> getSessions() {
        return mTerminalSessions;
    }

    public TerminalSession createTermSession(String executablePath, String[] arguments, String cwd, boolean failSafe) {
        TermuxConstants.TERMUX_HOME_DIR.mkdirs();

        if (cwd == null || cwd.isEmpty()) cwd = TermuxConstants.TERMUX_HOME_DIR_PATH;

        String[] env = BackgroundJob.buildEnvironment(failSafe, cwd);
        boolean isLoginShell = false;

        if (executablePath == null) {
            if (!failSafe) {
                for (String shellBinary : new String[]{"login", "bash", "zsh"}) {
                    File shellFile = new File(TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH, shellBinary);
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

        TerminalSession session = new TerminalSession(executablePath, cwd, args, env, getTermuxSessionClient());
        mTerminalSessions.add(session);
        updateNotification();

        // Make sure that terminal styling is always applied.
        Intent stylingIntent = new Intent(TERMUX_ACTIVITY.ACTION_RELOAD_STYLE);
        stylingIntent.putExtra(TERMUX_ACTIVITY.EXTRA_RELOAD_STYLE, "styling");
        sendBroadcast(stylingIntent);

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

    /** If {@link TermuxActivity} has not bound to the {@link TermuxService} yet or is destroyed, then
     * interface functions requiring the activity should not be available to the terminal sessions,
     * so we just return the {@link #mTermuxSessionClientBase}. Once {@link TermuxActivity} bind
     * callback is received, it should call {@link #setTermuxSessionClient} to set the
     * {@link TermuxService#mTermuxSessionClient} so that further terminal sessions are directly
     * passed the {@link TermuxSessionClient} object which fully implements the
     * {@link TerminalSessionClient} interface.
     *
     * @return Returns the {@link TermuxSessionClient} if {@link TermuxActivity} has bound with
     * {@link TermuxService}, otherwise {@link TermuxSessionClientBase}.
     */
    public TermuxSessionClientBase getTermuxSessionClient() {
        if (mTermuxSessionClient != null)
            return mTermuxSessionClient;
        else
            return mTermuxSessionClientBase;
    }

    /** This should be called when {@link TermuxActivity#onServiceConnected} is called to set the
     * {@link TermuxService#mTermuxSessionClient} variable and update the {@link TerminalSession}
     * and {@link TerminalEmulator} clients in case they were passed {@link TermuxSessionClientBase}
     * earlier.
     *
     * @param termuxSessionClient The {@link TermuxSessionClient} object that fully
     * implements the {@link TerminalSessionClient} interface.
     */
    public void setTermuxSessionClient(TermuxSessionClient termuxSessionClient) {
        mTermuxSessionClient = termuxSessionClient;

        for (int i = 0; i < mTerminalSessions.size(); i++)
            mTerminalSessions.get(i).updateTerminalSessionClient(mTermuxSessionClient);
    }

    /** This should be called when {@link TermuxActivity} has been destroyed so that the
     * {@link TermuxService} and {@link TerminalSession} and {@link TerminalEmulator} clients do not
     * hold an activity references.
     */
    public void unsetTermuxSessionClient() {
        mTermuxSessionClient = null;

        for (int i = 0; i < mTerminalSessions.size(); i++)
            mTerminalSessions.get(i).updateTerminalSessionClient(mTermuxSessionClientBase);
    }

    public void onBackgroundJobExited(final BackgroundJob task) {
        mHandler.post(() -> {
            mBackgroundTasks.remove(task);
            updateNotification();
        });
    }

    private void setupNotificationChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;

        String channelName = TermuxConstants.TERMUX_APP_NAME;
        String channelDescription = "Notifications from " + TermuxConstants.TERMUX_APP_NAME;
        int importance = NotificationManager.IMPORTANCE_LOW;

        NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID, channelName,importance);
        channel.setDescription(channelDescription);
        NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        manager.createNotificationChannel(channel);
    }

    public boolean wantsToStop() {
        return mWantsToStop;
    }
}
