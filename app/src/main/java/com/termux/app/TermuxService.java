package com.termux.app;

import android.annotation.SuppressLint;
import android.app.Notification;
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
import com.termux.app.terminal.TermuxSession;
import com.termux.app.terminal.TermuxSessionClient;
import com.termux.app.terminal.TermuxSessionClientBase;
import com.termux.app.utils.Logger;
import com.termux.app.utils.NotificationUtils;
import com.termux.app.utils.PermissionUtils;
import com.termux.app.shell.ShellUtils;
import com.termux.app.utils.DataUtils;
import com.termux.app.models.ExecutionCommand;
import com.termux.app.models.ExecutionCommand.ExecutionState;
import com.termux.app.terminal.TermuxTask;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSessionClient;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import javax.annotation.Nullable;

/**
 * A service holding a list of termux sessions, {@link #mTermuxSessions}, showing a foreground notification while
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
    private static final String NOTIFICATION_CHANNEL_NAME = TermuxConstants.TERMUX_APP_NAME + " App";
    public static final int NOTIFICATION_ID = 1337;

    private static int EXECUTION_ID = 1000;

    /** This service is only bound from inside the same process and never uses IPC. */
    class LocalBinder extends Binder {
        public final TermuxService service = TermuxService.this;
    }

    private final IBinder mBinder = new LocalBinder();

    private final Handler mHandler = new Handler();

    /**
     * The foreground termux sessions which this service manages.
     * Note that this list is observed by {@link TermuxActivity#mTermuxSessionListViewController},
     * so any changes must be made on the UI thread and followed by a call to
     * {@link ArrayAdapter#notifyDataSetChanged()} }.
     */
    final List<TermuxSession> mTermuxSessions = new ArrayList<>();

    /**
     * The background termux tasks which this service manages.
     */
    final List<TermuxTask> mTermuxTasks = new ArrayList<>();

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
                TermuxInstaller.deleteDirectory(termuxTmpDir.getCanonicalFile());
            } catch (Exception e) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Error while removing file at " + termuxTmpDir.getAbsolutePath(), e);
            }

            termuxTmpDir.mkdirs();
        }

        actionReleaseWakeLock(false);
        finishAllTermuxSessions();
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
        finishAllTermuxSessions();
        requestStopService();
    }

    /** Finish all termux sessions by sending SIGKILL to their shells. */
    private synchronized void finishAllTermuxSessions() {
        ExecutionCommand executionCommand;

        // TODO: Should SIGKILL also be send to background processes maintained by mTermuxTasks?
        for (int i = 0; i < mTermuxSessions.size(); i++) {
            TermuxSession termuxSession = mTermuxSessions.get(i);
            executionCommand = termuxSession.getExecutionCommand();

            // If the execution command was started for a plugin and is currently executing, then notify the callers
            if(executionCommand.isPluginExecutionCommand && executionCommand.isExecuting()) {
                if (executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, this.getString(R.string.error_sending_sigkill_to_process), null)) {
                    TermuxSession.processTermuxSessionResult(this, termuxSession, null);
                }
            }

            termuxSession.getTerminalSession().finishIfRunning();
        }
    }





    /** Process action to acquire Power and Wi-Fi WakeLocks. */
    @SuppressLint({"WakelockTimeout", "BatteryLife"})
    private void actionAcquireWakeLock() {
        if (mWakeLock != null) {
            Logger.logDebug(LOG_TAG, "Ignoring acquiring WakeLocks since they are already held");
            return;
        }

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

    /** Process {@link TERMUX_SERVICE#ACTION_SERVICE_EXECUTE} intent to execute a shell command in
     * a foreground termux session or in background. */
    private void actionServiceExecute(Intent intent) {
        if (intent == null){
            Logger.logError(LOG_TAG, "Ignoring null intent to actionServiceExecute");
            return;
        }

        ExecutionCommand executionCommand = new ExecutionCommand(getNextExecutionId());

        executionCommand.executableUri = intent.getData();

        if(executionCommand.executableUri != null) {
            executionCommand.executable = executionCommand.executableUri.getPath();
            executionCommand.arguments = intent.getStringArrayExtra(TERMUX_SERVICE.EXTRA_ARGUMENTS);
        }

        executionCommand.workingDirectory = intent.getStringExtra(TERMUX_SERVICE.EXTRA_WORKDIR);
        executionCommand.inBackground = intent.getBooleanExtra(TERMUX_SERVICE.EXTRA_BACKGROUND, false);
        executionCommand.isFailsafe = intent.getBooleanExtra(TERMUX_ACTIVITY.ACTION_FAILSAFE_SESSION, false);
        executionCommand.sessionAction = intent.getStringExtra(TERMUX_SERVICE.EXTRA_SESSION_ACTION);
        executionCommand.commandLabel = DataUtils.getDefaultIfNull(intent.getStringExtra(TERMUX_SERVICE.EXTRA_COMMAND_LABEL), "Execution Intent Command");
        executionCommand.commandDescription = intent.getStringExtra(TERMUX_SERVICE.EXTRA_COMMAND_DESCRIPTION);
        executionCommand.commandHelp = intent.getStringExtra(TERMUX_SERVICE.EXTRA_COMMAND_HELP);
        executionCommand.pluginAPIHelp = intent.getStringExtra(TERMUX_SERVICE.EXTRA_PLUGIN_API_HELP);
        executionCommand.isPluginExecutionCommand = true;
        executionCommand.pluginPendingIntent = intent.getParcelableExtra(TERMUX_SERVICE.EXTRA_PENDING_INTENT);

        if (executionCommand.inBackground) {
            executeTermuxTaskCommand(executionCommand);
        } else {
            executeTermuxSessionCommand(executionCommand);
        }
    }





    /** Execute a shell command in background {@link TermuxTask}. */
    private void executeTermuxTaskCommand(ExecutionCommand executionCommand) {
        if (executionCommand == null) return;

        Logger.logDebug(LOG_TAG, "Starting background termux task command");

        TermuxTask newTermuxTask = createTermuxTask(executionCommand);
    }

    /** Create a {@link TermuxTask}. */
    @Nullable
    public TermuxTask createTermuxTask(String executablePath, String[] arguments, String workingDirectory) {
        return createTermuxTask(new ExecutionCommand(getNextExecutionId(), executablePath, arguments, workingDirectory, true, false));
    }

    /** Create a {@link TermuxTask}. */
    @Nullable
    public synchronized TermuxTask createTermuxTask(ExecutionCommand executionCommand) {
        if (executionCommand == null) return null;

        Logger.logDebug(LOG_TAG, "Creating termux task");

        if (!executionCommand.inBackground) {
            Logger.logDebug(LOG_TAG, "Ignoring a foreground execution command passed to createTermuxTask()");
            return null;
        }

        if(Logger.getLogLevel() >= Logger.LOG_LEVEL_VERBOSE)
            Logger.logVerbose(LOG_TAG, executionCommand.toString());

        TermuxTask newTermuxTask = TermuxTask.create(this, executionCommand);
        if (newTermuxTask == null) {
            Logger.logError(LOG_TAG, "Failed to execute new termux task command for:\n" + executionCommand.getCommandIdAndLabelLogString());
            return null;
        };

        mTermuxTasks.add(newTermuxTask);

        updateNotification();

        return newTermuxTask;
    }

    /** Callback received when a {@link TermuxTask} finishes. */
    public synchronized void onTermuxTaskExited(final TermuxTask task) {
        mHandler.post(() -> {
            mTermuxTasks.remove(task);
            updateNotification();
        });
    }





    /** Execute a shell command in a foreground {@link TermuxSession}. */
    private void executeTermuxSessionCommand(ExecutionCommand executionCommand) {
        if (executionCommand == null) return;
        
        Logger.logDebug(LOG_TAG, "Starting foreground termux session command");

        String sessionName = null;

        // Transform executable path to session name, e.g. "/bin/do-something.sh" => "do something.sh".
        if (executionCommand.executable != null) {
            sessionName = ShellUtils.getExecutableBasename(executionCommand.executable).replace('-', ' ');
        }

        TermuxSession newTermuxSession = createTermuxSession(executionCommand, sessionName);
        if (newTermuxSession == null) return;

        handleSessionAction(DataUtils.getIntFromString(executionCommand.sessionAction,
            TERMUX_SERVICE.VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_OPEN_ACTIVITY),
            newTermuxSession.getTerminalSession());
    }

    /**
     * Create a {@link TermuxSession}.
     * Currently called by {@link TermuxSessionClient#addNewSession(boolean, String)} to add a new termux session.
     */
    @Nullable
    public TermuxSession createTermuxSession(String executablePath, String[] arguments, String workingDirectory, boolean isFailSafe, String sessionName) {
        return createTermuxSession(new ExecutionCommand(getNextExecutionId(), executablePath, arguments, workingDirectory, false, isFailSafe), sessionName);
    }

    /** Create a {@link TermuxSession}. */
    @Nullable
    public synchronized TermuxSession createTermuxSession(ExecutionCommand executionCommand, String sessionName) {
        if (executionCommand == null) return null;

        Logger.logDebug(LOG_TAG, "Creating termux session");

        if (executionCommand.inBackground) {
            Logger.logDebug(LOG_TAG, "Ignoring a background execution command passed to createTermuxSession()");
            return null;
        }

        if(Logger.getLogLevel() >= Logger.LOG_LEVEL_VERBOSE)
            Logger.logVerbose(LOG_TAG, executionCommand.toString());
        
        TermuxSession newTermuxSession = TermuxSession.create(this, executionCommand, getTermuxSessionClient(), sessionName);
        if (newTermuxSession == null) {
            Logger.logError(LOG_TAG, "Failed to execute new termux session command for:\n" + executionCommand.getCommandIdAndLabelLogString());
            return null;
        };

        mTermuxSessions.add(newTermuxSession);

        // Notify {@link TermuxSessionsListViewController} that sessions list has been updated if
        // activity in is foreground
        if(mTermuxSessionClient != null)
            mTermuxSessionClient.termuxSessionListNotifyUpdated();

        updateNotification();
        TermuxActivity.updateTermuxActivityStyling(this);

        return newTermuxSession;
    }

    /** Remove a termux session. */
    public synchronized int removeTermuxSession(TerminalSession sessionToRemove) {
        int index = getIndexOfSession(sessionToRemove);

        if(index >= 0) {
            TermuxSession termuxSession = mTermuxSessions.get(index);

            if (termuxSession.getExecutionCommand().setState(ExecutionState.EXECUTED)) {
                // If the execution command was started for a plugin and is currently executing, then process the result
                if(termuxSession.getExecutionCommand().isPluginExecutionCommand)
                    TermuxSession.processTermuxSessionResult(this, termuxSession, null);
                else
                    termuxSession.getExecutionCommand().setState(ExecutionState.SUCCESS);
            }

            mTermuxSessions.remove(termuxSession);

            // Notify {@link TermuxSessionsListViewController} that sessions list has been updated if
            // activity in is foreground
            if(mTermuxSessionClient != null)
                mTermuxSessionClient.termuxSessionListNotifyUpdated();
        }

        if (mTermuxSessions.isEmpty() && mWakeLock == null) {
            // Finish if there are no sessions left and the wake lock is not held, otherwise keep the service alive if
            // holding wake lock since there may be daemon processes (e.g. sshd) running.
            requestStopService();
        } else {
            updateNotification();
        }
        return index;
    }





    /** Process session action for new session. */
    private void handleSessionAction(int sessionAction, TerminalSession newTerminalSession) {
        Logger.logDebug(LOG_TAG, "Processing sessionAction \"" + sessionAction + "\" for session \"" + newTerminalSession.mSessionName + "\"");

        switch (sessionAction) {
            case TERMUX_SERVICE.VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_OPEN_ACTIVITY:
                setCurrentStoredTerminalSession(newTerminalSession);
                if(mTermuxSessionClient != null)
                    mTermuxSessionClient.setCurrentSession(newTerminalSession);
                startTermuxActivity();
                break;
            case TERMUX_SERVICE.VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_OPEN_ACTIVITY:
                if(getTermuxSessionsSize() == 1)
                    setCurrentStoredTerminalSession(newTerminalSession);
                startTermuxActivity();
                break;
            case TERMUX_SERVICE.VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_DONT_OPEN_ACTIVITY:
                setCurrentStoredTerminalSession(newTerminalSession);
                if(mTermuxSessionClient != null)
                    mTermuxSessionClient.setCurrentSession(newTerminalSession);
                break;
            case TERMUX_SERVICE.VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_DONT_OPEN_ACTIVITY:
                if(getTermuxSessionsSize() == 1)
                    setCurrentStoredTerminalSession(newTerminalSession);
                break;
            default:
                Logger.logError(LOG_TAG, "Invalid sessionAction: \"" + sessionAction + "\". Force using default sessionAction.");
                handleSessionAction(TERMUX_SERVICE.VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_OPEN_ACTIVITY, newTerminalSession);
                break;
        }
    }

    /** Launch the {@link }TermuxActivity} to bring it to foreground. */
    private void startTermuxActivity() {
        // For android >= 10, apps require Display over other apps permission to start foreground activities
        // from background (services). If it is not granted, then termux sessions that are started will
        // show in Termux notification but will not run until user manually clicks the notification.
        if(PermissionUtils.validateDisplayOverOtherAppsPermissionForPostAndroid10(this)) {
            TermuxActivity.startTermuxActivity(this);
        }
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
    public synchronized TermuxSessionClientBase getTermuxSessionClient() {
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
    public synchronized void setTermuxSessionClient(TermuxSessionClient termuxSessionClient) {
        mTermuxSessionClient = termuxSessionClient;

        for (int i = 0; i < mTermuxSessions.size(); i++)
            mTermuxSessions.get(i).getTerminalSession().updateTerminalSessionClient(mTermuxSessionClient);
    }

    /** This should be called when {@link TermuxActivity} has been destroyed and in {@link #onUnbind(Intent)}
     * so that the {@link TermuxService} and {@link TerminalSession} and {@link TerminalEmulator}
     * clients do not hold an activity references.
     */
    public synchronized void unsetTermuxSessionClient() {
        for (int i = 0; i < mTermuxSessions.size(); i++)
            mTermuxSessions.get(i).getTerminalSession().updateTerminalSessionClient(mTermuxSessionClientBase);

        mTermuxSessionClient = null;
    }





    private Notification buildNotification() {
        Resources res = getResources();

        // Set pending intent to be launched when notification is clicked
        Intent notificationIntent = TermuxActivity.newInstance(this);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);


        // Set notification text
        int sessionCount = getTermuxSessionsSize();
        int taskCount = mTermuxTasks.size();
        String notifiationText = sessionCount + " session" + (sessionCount == 1 ? "" : "s");
        if (taskCount > 0) {
            notifiationText += ", " + taskCount + " task" + (taskCount == 1 ? "" : "s");
        }

        final boolean wakeLockHeld = mWakeLock != null;
        if (wakeLockHeld) notifiationText += " (wake lock held)";


        // Set notification priority
        // If holding a wake or wifi lock consider the notification of high priority since it's using power,
        // otherwise use a low priority
        int priority = (wakeLockHeld) ? Notification.PRIORITY_HIGH : Notification.PRIORITY_LOW;


        // Build the notification
        Notification.Builder builder =  NotificationUtils.geNotificationBuilder(this,
            NOTIFICATION_CHANNEL_ID, priority,
            getText(R.string.application_name), notifiationText, null,
            pendingIntent, NotificationUtils.NOTIFICATION_MODE_SILENT);
        if(builder == null)  return null;

        // No need to show a timestamp:
        builder.setShowWhen(false);

        // Set notification icon
        builder.setSmallIcon(R.drawable.ic_service_notification);

        // Set background color for small notification icon
        builder.setColor(0xFF607D8B);

        // Termux sessions are always ongoing
        builder.setOngoing(true);


        // Set Exit button action
        Intent exitIntent = new Intent(this, TermuxService.class).setAction(TERMUX_SERVICE.ACTION_STOP_SERVICE);
        builder.addAction(android.R.drawable.ic_delete, res.getString(R.string.notification_action_exit), PendingIntent.getService(this, 0, exitIntent, 0));


        // Set Wakelock button actions
        String newWakeAction = wakeLockHeld ? TERMUX_SERVICE.ACTION_WAKE_UNLOCK : TERMUX_SERVICE.ACTION_WAKE_LOCK;
        Intent toggleWakeLockIntent = new Intent(this, TermuxService.class).setAction(newWakeAction);
        String actionTitle = res.getString(wakeLockHeld ? R.string.notification_action_wake_unlock : R.string.notification_action_wake_lock);
        int actionIcon = wakeLockHeld ? android.R.drawable.ic_lock_idle_lock : android.R.drawable.ic_lock_lock;
        builder.addAction(actionIcon, actionTitle, PendingIntent.getService(this, 0, toggleWakeLockIntent, 0));


        return builder.build();
    }

    private void setupNotificationChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;

        NotificationUtils.setupNotificationChannel(this, NOTIFICATION_CHANNEL_ID,
            NOTIFICATION_CHANNEL_NAME, NotificationManager.IMPORTANCE_LOW);
    }

    /** Update the shown foreground service notification after making any changes that affect it. */
    void updateNotification() {
        if (mWakeLock == null && mTermuxSessions.isEmpty() && mTermuxTasks.isEmpty()) {
            // Exit if we are updating after the user disabled all locks with no sessions or tasks running.
            requestStopService();
        } else {
            ((NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE)).notify(NOTIFICATION_ID, buildNotification());
        }
    }





    private void setCurrentStoredTerminalSession(TerminalSession session) {
        if(session == null) return;
        // Make the newly created session the current one to be displayed:
        TermuxAppSharedPreferences preferences = new TermuxAppSharedPreferences(this);
        preferences.setCurrentSession(session.mHandle);
    }

    public synchronized boolean isTermuxSessionsEmpty() {
        return mTermuxSessions.isEmpty();
    }

    public synchronized int getTermuxSessionsSize() {
        return mTermuxSessions.size();
    }

    public synchronized List<TermuxSession> getTermuxSessions() {
        return mTermuxSessions;
    }

    @Nullable
    public synchronized TermuxSession getTermuxSession(int index) {
        if(index >= 0 && index < mTermuxSessions.size())
            return mTermuxSessions.get(index);
        else
            return null;
    }

    public synchronized TermuxSession getLastTermuxSession() {
        return mTermuxSessions.isEmpty() ? null : mTermuxSessions.get(mTermuxSessions.size() - 1);
    }

    public synchronized int getIndexOfSession(TerminalSession terminalSession) {
        for (int i = 0; i < mTermuxSessions.size(); i++) {
            if (mTermuxSessions.get(i).getTerminalSession().equals(terminalSession))
                return i;
        }
        return -1;
    }

    public synchronized TerminalSession getTerminalSessionForHandle(String sessionHandle) {
        TerminalSession terminalSession;
        for (int i = 0, len = mTermuxSessions.size(); i < len; i++) {
            terminalSession = mTermuxSessions.get(i).getTerminalSession();
            if (terminalSession.mHandle.equals(sessionHandle))
                return terminalSession;
        }
        return null;
    }



    public static synchronized int getNextExecutionId() {
        return EXECUTION_ID++;
    }

    public boolean wantsToStop() {
        return mWantsToStop;
    }

}
