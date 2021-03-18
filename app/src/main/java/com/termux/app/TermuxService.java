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
import com.termux.app.settings.preferences.TermuxAppSharedPreferences;
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
     * Note that this list is observed by {@link TermuxActivity#mTermuxSessionListViewController},
     * so any changes must be made on the UI thread and followed by a call to
     * {@link ArrayAdapter#notifyDataSetChanged()} }.
     */
    final List<TerminalSession> mTerminalSessions = new ArrayList<>();

    /**
     * The background jobs which this service manages.
     */
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

    /** If the user has executed the {@link TERMUX_SERVICE#ACTION_STOP_SERVICE} intent. */
    boolean mWantsToStop = false;

    private static final String LOG_TAG = "TermuxService";

    @Override
    public void onCreate() {
        Logger.logVerbose(LOG_TAG, "onCreate");
        runStartForeground();
    }

    @SuppressLint("Wakelock")
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Logger.logDebug(LOG_TAG, "onStartCommand");

        // Run again in case service is already started and onCreate() is not called
        runStartForeground();

        String action = intent.getAction();

        if (action != null) {
            switch (action) {
                case TERMUX_SERVICE.ACTION_STOP_SERVICE:
                    Logger.logDebug(LOG_TAG, "ACTION_STOP_SERVICE intent received");
                    actionStopService();
                    break;
                case TERMUX_SERVICE.ACTION_WAKE_LOCK:
                    Logger.logDebug(LOG_TAG, "ACTION_WAKE_LOCK intent received");
                    actionAcquireWakeLock();
                    break;
                case TERMUX_SERVICE.ACTION_WAKE_UNLOCK:
                    Logger.logDebug(LOG_TAG, "ACTION_WAKE_UNLOCK intent received");
                    actionReleaseWakeLock(true);
                    break;
                case TERMUX_SERVICE.ACTION_SERVICE_EXECUTE:
                    Logger.logDebug(LOG_TAG, "ACTION_SERVICE_EXECUTE intent received");
                    actionServiceExecute(intent);
                    break;
                default:
                    Logger.logError(LOG_TAG, "Invalid action: \"" + action + "\"");
                    break;
            }
        }

        // If this service really do get killed, there is no point restarting it automatically - let the user do on next
        // start of {@link Term):
        return Service.START_NOT_STICKY;
    }

    @Override
    public void onDestroy() {
        Logger.logVerbose(LOG_TAG, "onDestroy");
        File termuxTmpDir = TermuxConstants.TERMUX_TMP_DIR;

        if (termuxTmpDir.exists()) {
            try {
                TermuxInstaller.deleteFolder(termuxTmpDir.getCanonicalFile());
            } catch (Exception e) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Error while removing file at " + termuxTmpDir.getAbsolutePath(), e);
            }

            termuxTmpDir.mkdirs();
        }

        actionReleaseWakeLock(false);
        finishAllTerminalSessions();
        runStopForeground();
    }

    @Override
    public IBinder onBind(Intent intent) {
        Logger.logVerbose(LOG_TAG, "onBind");
        return mBinder;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        Logger.logVerbose(LOG_TAG, "onUnbind");

        // Since we cannot rely on {@link TermuxActivity.onDestroy()} to always complete,
        // we unset clients here as well if it failed, so that we do not leave service and session
        // clients with references to the activity.
        if(mTermuxSessionClient != null)
            unsetTermuxSessionClient();
        return false;
    }

    /** Make service run in foreground mode. */
    private void runStartForeground() {
        setupNotificationChannel();
        startForeground(NOTIFICATION_ID, buildNotification());
    }

    /** Make service leave foreground mode. */
    private void runStopForeground() {
        stopForeground(true);
    }

    /** Request to stop service. */
    private void requestStopService() {
        Logger.logDebug(LOG_TAG, "Requesting to stop service");
        runStopForeground();
        stopSelf();
    }



    /** Process action to stop service. */
    private void actionStopService() {
        mWantsToStop = true;
        finishAllTerminalSessions();
        requestStopService();
    }

    /** Process action to acquire Power and Wi-Fi WakeLocks. */
    @SuppressLint({"WakelockTimeout", "BatteryLife"})
    private void actionAcquireWakeLock() {
        if (mWakeLock == null) {
            Logger.logDebug(LOG_TAG, "Acquiring WakeLocks");

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

            Logger.logDebug(LOG_TAG, "WakeLocks acquired successfully");
        } else {
            Logger.logDebug(LOG_TAG, "Ignoring acquiring WakeLocks since they are already held");
        }
    }

    /** Process action to release Power and Wi-Fi WakeLocks. */
    private void actionReleaseWakeLock(boolean updateNotification) {
        if (mWakeLock == null && mWifiLock == null){
            Logger.logDebug(LOG_TAG, "Ignoring releasing WakeLocks since none are already held");
            return;
        }

        Logger.logDebug(LOG_TAG, "Releasing WakeLocks");

        if (mWakeLock != null) {
            mWakeLock.release();
            mWakeLock = null;
        }

        if (mWifiLock != null) {
            mWifiLock.release();
            mWifiLock = null;
        }

        if(updateNotification)
            updateNotification();

        Logger.logDebug(LOG_TAG, "WakeLocks released successfully");
    }

    /** Process action to execute a shell command in a foreground session or in background. */
    private void actionServiceExecute(Intent intent) {
        Uri executableUri = intent.getData();
        String executablePath = (executableUri == null ? null : executableUri.getPath());

        String[] arguments = (executableUri == null ? null : intent.getStringArrayExtra(TERMUX_SERVICE.EXTRA_ARGUMENTS));
        String cwd = intent.getStringExtra(TERMUX_SERVICE.EXTRA_WORKDIR);

        PendingIntent pendingIntent = intent.getParcelableExtra(TERMUX_SERVICE.EXTRA_PENDING_INTENT);

        if (intent.getBooleanExtra(TERMUX_SERVICE.EXTRA_BACKGROUND, false)) {
            executeBackgroundCommand(executablePath, arguments, cwd, pendingIntent);
        } else {
            executeForegroundCommand(intent, executablePath, arguments, cwd);
        }
    }

    /** Execute a shell command in background with {@link BackgroundJob}. */
    private void executeBackgroundCommand(String executablePath, String[] arguments, String cwd, PendingIntent pendingIntent) {
        Logger.logDebug(LOG_TAG, "Starting background command");

        BackgroundJob task = new BackgroundJob(cwd, executablePath, arguments, this, pendingIntent);
        mBackgroundTasks.add(task);
        updateNotification();
    }

    /** Callback received when a {@link BackgroundJob} finishes. */
    public void onBackgroundJobExited(final BackgroundJob task) {
        mHandler.post(() -> {
            mBackgroundTasks.remove(task);
            updateNotification();
        });
    }

    /** Execute a shell command in a foreground terminal session. */
    private void executeForegroundCommand(Intent intent, String executablePath, String[] arguments, String cwd) {
        Logger.logDebug(LOG_TAG, "Starting foreground command");

        boolean failsafe = intent.getBooleanExtra(TERMUX_ACTIVITY.ACTION_FAILSAFE_SESSION, false);
        TerminalSession newSession = createTerminalSession(executablePath, arguments, cwd, failsafe);

        // Transform executable path to session name, e.g. "/bin/do-something.sh" => "do something.sh".
        if (executablePath != null) {
            int lastSlash = executablePath.lastIndexOf('/');
            String name = (lastSlash == -1) ? executablePath : executablePath.substring(lastSlash + 1);
            name = name.replace('-', ' ');
            newSession.mSessionName = name;
        }

        // Make the newly created session the current one to be displayed:
        TermuxAppSharedPreferences preferences = new TermuxAppSharedPreferences(this);
        preferences.setCurrentSession(newSession.mHandle);

        // Notify {@link TermuxSessionsListViewController} that sessions list has been updated if
        // activity in is foreground
        if(mTermuxSessionClient != null)
            mTermuxSessionClient.terminalSessionListNotifyUpdated();

        // Launch the main Termux app, which will now show the current session:
        startActivity(new Intent(this, TermuxActivity.class).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
    }



    /** Create a terminal session. */
    public TerminalSession createTerminalSession(String executablePath, String[] arguments, String cwd, boolean failSafe) {
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

    /** Remove a terminal session. */
    public int removeTerminalSession(TerminalSession sessionToRemove) {
        int indexOfRemoved = mTerminalSessions.indexOf(sessionToRemove);
        mTerminalSessions.remove(indexOfRemoved);

        if (mTerminalSessions.isEmpty() && mWakeLock == null) {
            // Finish if there are no sessions left and the wake lock is not held, otherwise keep the service alive if
            // holding wake lock since there may be daemon processes (e.g. sshd) running.
            requestStopService();
        } else {
            updateNotification();
        }
        return indexOfRemoved;
    }

    /** Finish all terminal sessions by sending SIGKILL to their shells. */
    private void finishAllTerminalSessions() {
        for (int i = 0; i < mTerminalSessions.size(); i++)
            mTerminalSessions.get(i).finishIfRunning();
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

    /** This should be called when {@link TermuxActivity} has been destroyed and in {@link #onUnbind(Intent)}
     * so that the {@link TermuxService} and {@link TerminalSession} and {@link TerminalEmulator}
     * clients do not hold an activity references.
     */
    public void unsetTermuxSessionClient() {
        for (int i = 0; i < mTerminalSessions.size(); i++)
            mTerminalSessions.get(i).updateTerminalSessionClient(mTermuxSessionClientBase);

        mTermuxSessionClient = null;
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

    /** Update the shown foreground service notification after making any changes that affect it. */
    void updateNotification() {
        if (mWakeLock == null && mTerminalSessions.isEmpty() && mBackgroundTasks.isEmpty()) {
            // Exit if we are updating after the user disabled all locks with no sessions or tasks running.
            requestStopService();
        } else {
            ((NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE)).notify(NOTIFICATION_ID, buildNotification());
        }
    }



    public boolean wantsToStop() {
        return mWantsToStop;
    }

    public List<TerminalSession> getSessions() {
        return mTerminalSessions;
    }

}
