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

import androidx.annotation.Nullable;

import com.termux.R;
import com.termux.app.settings.properties.TermuxAppSharedProperties;
import com.termux.app.terminal.TermuxTerminalSessionClient;
import com.termux.app.utils.PluginUtils;
import com.termux.shared.data.IntentUtils;
import com.termux.shared.models.errors.Errno;
import com.termux.shared.shell.ShellUtils;
import com.termux.shared.shell.TermuxShellEnvironmentClient;
import com.termux.shared.shell.TermuxShellUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_ACTIVITY;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.shared.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.shell.TermuxSession;
import com.termux.shared.terminal.TermuxTerminalSessionClientBase;
import com.termux.shared.logger.Logger;
import com.termux.shared.notification.NotificationUtils;
import com.termux.shared.packages.PermissionUtils;
import com.termux.shared.data.DataUtils;
import com.termux.shared.models.ExecutionCommand;
import com.termux.shared.shell.TermuxTask;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSessionClient;

import java.util.ArrayList;
import java.util.List;

/**
 * A service holding a list of {@link TermuxSession} in {@link #mTermuxSessions} and background {@link TermuxTask}
 * in {@link #mTermuxTasks}, showing a foreground notification while running so that it is not terminated.
 * The user interacts with the session through {@link TermuxActivity}, but this service may outlive
 * the activity when the user or the system disposes of the activity. In that case the user may
 * restart {@link TermuxActivity} later to yet again access the sessions.
 * <p/>
 * In order to keep both terminal sessions and spawned processes (who may outlive the terminal sessions) alive as long
 * as wanted by the user this service is a foreground service, {@link Service#startForeground(int, Notification)}.
 * <p/>
 * Optionally may hold a wake and a wifi lock, in which case that is shown in the notification - see
 * {@link #buildNotification()}.
 */
public final class TermuxService extends Service implements TermuxTask.TermuxTaskClient, TermuxSession.TermuxSessionClient {

    private static int EXECUTION_ID = 1000;

    /** This service is only bound from inside the same process and never uses IPC. */
    class LocalBinder extends Binder {
        public final TermuxService service = TermuxService.this;
    }

    private final IBinder mBinder = new LocalBinder();

    private final Handler mHandler = new Handler();

    /**
     * The foreground TermuxSessions which this service manages.
     * Note that this list is observed by {@link TermuxActivity#mTermuxSessionListViewController},
     * so any changes must be made on the UI thread and followed by a call to
     * {@link ArrayAdapter#notifyDataSetChanged()} }.
     */
    final List<TermuxSession> mTermuxSessions = new ArrayList<>();

    /**
     * The background TermuxTasks which this service manages.
     */
    final List<TermuxTask> mTermuxTasks = new ArrayList<>();

    /**
     * The pending plugin ExecutionCommands that have yet to be processed by this service.
     */
    final List<ExecutionCommand> mPendingPluginExecutionCommands = new ArrayList<>();

    /** The full implementation of the {@link TerminalSessionClient} interface to be used by {@link TerminalSession}
     * that holds activity references for activity related functions.
     * Note that the service may often outlive the activity, so need to clear this reference.
     */
    TermuxTerminalSessionClient mTermuxTerminalSessionClient;

    /** The basic implementation of the {@link TerminalSessionClient} interface to be used by {@link TerminalSession}
     * that does not hold activity references.
     */
    final TermuxTerminalSessionClientBase mTermuxTerminalSessionClientBase = new TermuxTerminalSessionClientBase();

    /** The wake lock and wifi lock are always acquired and released together. */
    private PowerManager.WakeLock mWakeLock;
    private WifiManager.WifiLock mWifiLock;

    /** If the user has executed the {@link TERMUX_SERVICE#ACTION_STOP_SERVICE} intent. */
    boolean mWantsToStop = false;

    public Integer mTerminalTranscriptRows;

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

        TermuxShellUtils.clearTermuxTMPDIR(true);

        actionReleaseWakeLock(false);
        if (!mWantsToStop)
            killAllTermuxExecutionCommands();
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
        if (mTermuxTerminalSessionClient != null)
            unsetTermuxTerminalSessionClient();
        return false;
    }

    /** Make service run in foreground mode. */
    private void runStartForeground() {
        setupNotificationChannel();
        startForeground(TermuxConstants.TERMUX_APP_NOTIFICATION_ID, buildNotification());
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
        killAllTermuxExecutionCommands();
        requestStopService();
    }

    /** Kill all TermuxSessions and TermuxTasks by sending SIGKILL to their processes.
     *
     * For TermuxSessions, all sessions will be killed, whether user manually exited Termux or if
     * onDestroy() was directly called because of unintended shutdown. The processing of results
     * will only be done if user manually exited termux or if the session was started by a plugin
     * which **expects** the result back via a pending intent.
     *
     * For TermuxTasks, only tasks that were started by a plugin which **expects** the result
     * back via a pending intent will be killed, whether user manually exited Termux or if
     * onDestroy() was directly called because of unintended shutdown. The processing of results
     * will always be done for the tasks that are killed. The remaining processes will keep on
     * running until the termux app process is killed by android, like by OOM, so we let them run
     * as long as they can.
     *
     * Some plugin execution commands may not have been processed and added to mTermuxSessions and
     * mTermuxTasks lists before the service is killed, so we maintain a separate
     * mPendingPluginExecutionCommands list for those, so that we can notify the pending intent
     * creators that execution was cancelled.
     *
     * Note that if user didn't manually exit Termux and if onDestroy() was directly called because
     * of unintended shutdown, like android deciding to kill the service, then there will be no
     * guarantee that onDestroy() will be allowed to finish and termux app process may be killed before
     * it has finished. This means that in those cases some results may not be sent back to their
     * creators for plugin commands but we still try to process whatever results can be processed
     * despite the unreliable behaviour of onDestroy().
     *
     * Note that if don't kill the processes started by plugins which **expect** the result back
     * and notify their creators that they have been killed, then they may get stuck waiting for
     * the results forever like in case of commands started by Termux:Tasker or RUN_COMMAND intent,
     * since once TermuxService has been killed, no result will be sent back. They may still get
     * stuck if termux app process gets killed, so for this case reasonable timeout values should
     * be used, like in Tasker for the Termux:Tasker actions.
     *
     * We make copies of each list since items are removed inside the loop.
     */
    private synchronized void killAllTermuxExecutionCommands() {
        boolean processResult;

        Logger.logDebug(LOG_TAG, "Killing TermuxSessions=" + mTermuxSessions.size() + ", TermuxTasks=" + mTermuxTasks.size() + ", PendingPluginExecutionCommands=" + mPendingPluginExecutionCommands.size());

        List<TermuxSession> termuxSessions = new ArrayList<>(mTermuxSessions);
        for (int i = 0; i < termuxSessions.size(); i++) {
            ExecutionCommand executionCommand = termuxSessions.get(i).getExecutionCommand();
            processResult = mWantsToStop || executionCommand.isPluginExecutionCommandWithPendingResult();
            termuxSessions.get(i).killIfExecuting(this, processResult);
        }

        List<TermuxTask> termuxTasks = new ArrayList<>(mTermuxTasks);
        for (int i = 0; i < termuxTasks.size(); i++) {
            ExecutionCommand executionCommand = termuxTasks.get(i).getExecutionCommand();
            if (executionCommand.isPluginExecutionCommandWithPendingResult())
                termuxTasks.get(i).killIfExecuting(this, true);
        }

        List<ExecutionCommand> pendingPluginExecutionCommands = new ArrayList<>(mPendingPluginExecutionCommands);
        for (int i = 0; i < pendingPluginExecutionCommands.size(); i++) {
            ExecutionCommand executionCommand = pendingPluginExecutionCommands.get(i);
            if (!executionCommand.shouldNotProcessResults() && executionCommand.isPluginExecutionCommandWithPendingResult()) {
                if (executionCommand.setStateFailed(Errno.ERRNO_CANCELLED.getCode(), this.getString(com.termux.shared.R.string.error_execution_cancelled))) {
                    PluginUtils.processPluginExecutionCommandResult(this, LOG_TAG, executionCommand);
                }
            }
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
        if (mWakeLock == null && mWifiLock == null) {
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

        if (updateNotification)
            updateNotification();

        Logger.logDebug(LOG_TAG, "WakeLocks released successfully");
    }

    /** Process {@link TERMUX_SERVICE#ACTION_SERVICE_EXECUTE} intent to execute a shell command in
     * a foreground TermuxSession or in a background TermuxTask. */
    private void actionServiceExecute(Intent intent) {
        if (intent == null) {
            Logger.logError(LOG_TAG, "Ignoring null intent to actionServiceExecute");
            return;
        }

        ExecutionCommand executionCommand = new ExecutionCommand(getNextExecutionId());

        executionCommand.executableUri = intent.getData();
        executionCommand.inBackground = intent.getBooleanExtra(TERMUX_SERVICE.EXTRA_BACKGROUND, false);

        if (executionCommand.executableUri != null) {
            executionCommand.executable = executionCommand.executableUri.getPath();
            executionCommand.arguments = IntentUtils.getStringArrayExtraIfSet(intent, TERMUX_SERVICE.EXTRA_ARGUMENTS, null);
            if (executionCommand.inBackground)
                executionCommand.stdin = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_STDIN, null);
                executionCommand.backgroundCustomLogLevel = IntentUtils.getIntegerExtraIfSet(intent, TERMUX_SERVICE.EXTRA_BACKGROUND_CUSTOM_LOG_LEVEL, null);
        }

        executionCommand.workingDirectory = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_WORKDIR, null);
        executionCommand.isFailsafe = intent.getBooleanExtra(TERMUX_ACTIVITY.EXTRA_FAILSAFE_SESSION, false);
        executionCommand.sessionAction = intent.getStringExtra(TERMUX_SERVICE.EXTRA_SESSION_ACTION);
        executionCommand.commandLabel = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_COMMAND_LABEL, "Execution Intent Command");
        executionCommand.commandDescription = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_COMMAND_DESCRIPTION, null);
        executionCommand.commandHelp = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_COMMAND_HELP, null);
        executionCommand.pluginAPIHelp = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_PLUGIN_API_HELP, null);
        executionCommand.isPluginExecutionCommand = true;
        executionCommand.resultConfig.resultPendingIntent = intent.getParcelableExtra(TERMUX_SERVICE.EXTRA_PENDING_INTENT);
        executionCommand.resultConfig.resultDirectoryPath = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_RESULT_DIRECTORY, null);
        if (executionCommand.resultConfig.resultDirectoryPath != null) {
            executionCommand.resultConfig.resultSingleFile = intent.getBooleanExtra(TERMUX_SERVICE.EXTRA_RESULT_SINGLE_FILE, false);
            executionCommand.resultConfig.resultFileBasename = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_RESULT_FILE_BASENAME, null);
            executionCommand.resultConfig.resultFileOutputFormat = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_RESULT_FILE_OUTPUT_FORMAT, null);
            executionCommand.resultConfig.resultFileErrorFormat = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_RESULT_FILE_ERROR_FORMAT, null);
            executionCommand.resultConfig.resultFilesSuffix = IntentUtils.getStringExtraIfSet(intent, TERMUX_SERVICE.EXTRA_RESULT_FILES_SUFFIX, null);
        }

        // Add the execution command to pending plugin execution commands list
        mPendingPluginExecutionCommands.add(executionCommand);

        if (executionCommand.inBackground) {
            executeTermuxTaskCommand(executionCommand);
        } else {
            executeTermuxSessionCommand(executionCommand);
        }
    }





    /** Execute a shell command in background {@link TermuxTask}. */
    private void executeTermuxTaskCommand(ExecutionCommand executionCommand) {
        if (executionCommand == null) return;

        Logger.logDebug(LOG_TAG, "Executing background \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask command");

        TermuxTask newTermuxTask = createTermuxTask(executionCommand);
    }

    /** Create a {@link TermuxTask}. */
    @Nullable
    public TermuxTask createTermuxTask(String executablePath, String[] arguments, String stdin, String workingDirectory) {
        return createTermuxTask(new ExecutionCommand(getNextExecutionId(), executablePath, arguments, stdin, workingDirectory, true, false));
    }

    /** Create a {@link TermuxTask}. */
    @Nullable
    public synchronized TermuxTask createTermuxTask(ExecutionCommand executionCommand) {
        if (executionCommand == null) return null;

        Logger.logDebug(LOG_TAG, "Creating \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask");

        if (!executionCommand.inBackground) {
            Logger.logDebug(LOG_TAG, "Ignoring a foreground execution command passed to createTermuxTask()");
            return null;
        }

        if (Logger.getLogLevel() >= Logger.LOG_LEVEL_VERBOSE)
            Logger.logVerboseExtended(LOG_TAG, executionCommand.toString());

        TermuxTask newTermuxTask = TermuxTask.execute(this, executionCommand, this, new TermuxShellEnvironmentClient(), false);
        if (newTermuxTask == null) {
            Logger.logError(LOG_TAG, "Failed to execute new TermuxTask command for:\n" + executionCommand.getCommandIdAndLabelLogString());
            // If the execution command was started for a plugin, then process the error
            if (executionCommand.isPluginExecutionCommand)
                PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, false);
            else
                Logger.logErrorExtended(LOG_TAG, executionCommand.toString());
            return null;
        }

        mTermuxTasks.add(newTermuxTask);

        // Remove the execution command from the pending plugin execution commands list since it has
        // now been processed
        if (executionCommand.isPluginExecutionCommand)
            mPendingPluginExecutionCommands.remove(executionCommand);

        updateNotification();

        return newTermuxTask;
    }

    /** Callback received when a {@link TermuxTask} finishes. */
    @Override
    public void onTermuxTaskExited(final TermuxTask termuxTask) {
        mHandler.post(() -> {
            if (termuxTask != null) {
                ExecutionCommand executionCommand = termuxTask.getExecutionCommand();

                Logger.logVerbose(LOG_TAG, "The onTermuxTaskExited() callback called for \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask command");

                // If the execution command was started for a plugin, then process the results
                if (executionCommand != null && executionCommand.isPluginExecutionCommand)
                    PluginUtils.processPluginExecutionCommandResult(this, LOG_TAG, executionCommand);

                mTermuxTasks.remove(termuxTask);
            }

            updateNotification();
        });
    }





    /** Execute a shell command in a foreground {@link TermuxSession}. */
    private void executeTermuxSessionCommand(ExecutionCommand executionCommand) {
        if (executionCommand == null) return;

        Logger.logDebug(LOG_TAG, "Executing foreground \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession command");

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
     * Currently called by {@link TermuxTerminalSessionClient#addNewSession(boolean, String)} to add a new {@link TermuxSession}.
     */
    @Nullable
    public TermuxSession createTermuxSession(String executablePath, String[] arguments, String stdin, String workingDirectory, boolean isFailSafe, String sessionName) {
        return createTermuxSession(new ExecutionCommand(getNextExecutionId(), executablePath, arguments, stdin, workingDirectory, false, isFailSafe), sessionName);
    }

    /** Create a {@link TermuxSession}. */
    @Nullable
    public synchronized TermuxSession createTermuxSession(ExecutionCommand executionCommand, String sessionName) {
        if (executionCommand == null) return null;

        Logger.logDebug(LOG_TAG, "Creating \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession");

        if (executionCommand.inBackground) {
            Logger.logDebug(LOG_TAG, "Ignoring a background execution command passed to createTermuxSession()");
            return null;
        }

        if (Logger.getLogLevel() >= Logger.LOG_LEVEL_VERBOSE)
            Logger.logVerboseExtended(LOG_TAG, executionCommand.toString());

        // If the execution command was started for a plugin, only then will the stdout be set
        // Otherwise if command was manually started by the user like by adding a new terminal session,
        // then no need to set stdout
        executionCommand.terminalTranscriptRows = getTerminalTranscriptRows();
        TermuxSession newTermuxSession = TermuxSession.execute(this, executionCommand, getTermuxTerminalSessionClient(), this, new TermuxShellEnvironmentClient(), sessionName, executionCommand.isPluginExecutionCommand);
        if (newTermuxSession == null) {
            Logger.logError(LOG_TAG, "Failed to execute new TermuxSession command for:\n" + executionCommand.getCommandIdAndLabelLogString());
            // If the execution command was started for a plugin, then process the error
            if (executionCommand.isPluginExecutionCommand)
                PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, false);
            else
                Logger.logErrorExtended(LOG_TAG, executionCommand.toString());
            return null;
        }

        mTermuxSessions.add(newTermuxSession);

        // Remove the execution command from the pending plugin execution commands list since it has
        // now been processed
        if (executionCommand.isPluginExecutionCommand)
            mPendingPluginExecutionCommands.remove(executionCommand);

        // Notify {@link TermuxSessionsListViewController} that sessions list has been updated if
        // activity in is foreground
        if (mTermuxTerminalSessionClient != null)
            mTermuxTerminalSessionClient.termuxSessionListNotifyUpdated();

        updateNotification();
        TermuxActivity.updateTermuxActivityStyling(this);

        return newTermuxSession;
    }

    /** Remove a TermuxSession. */
    public synchronized int removeTermuxSession(TerminalSession sessionToRemove) {
        int index = getIndexOfSession(sessionToRemove);

        if (index >= 0)
            mTermuxSessions.get(index).finish();

        return index;
    }

    /** Callback received when a {@link TermuxSession} finishes. */
    @Override
    public void onTermuxSessionExited(final TermuxSession termuxSession) {
        if (termuxSession != null) {
            ExecutionCommand executionCommand = termuxSession.getExecutionCommand();

            Logger.logVerbose(LOG_TAG, "The onTermuxSessionExited() callback called for \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession command");

            // If the execution command was started for a plugin, then process the results
            if (executionCommand != null && executionCommand.isPluginExecutionCommand)
                PluginUtils.processPluginExecutionCommandResult(this, LOG_TAG, executionCommand);

            mTermuxSessions.remove(termuxSession);

            // Notify {@link TermuxSessionsListViewController} that sessions list has been updated if
            // activity in is foreground
            if (mTermuxTerminalSessionClient != null)
                mTermuxTerminalSessionClient.termuxSessionListNotifyUpdated();
        }

        updateNotification();
    }

    /** Get the terminal transcript rows to be used for new {@link TermuxSession}. */
    public Integer getTerminalTranscriptRows() {
        if (mTerminalTranscriptRows == null)
            setTerminalTranscriptRows();
        return mTerminalTranscriptRows;
    }

    public void setTerminalTranscriptRows() {
        // TermuxService only uses this termux property currently, so no need to load them all into
        // an internal values map like TermuxActivity does
        mTerminalTranscriptRows = TermuxAppSharedProperties.getTerminalTranscriptRows(this);
    }





    /** Process session action for new session. */
    private void handleSessionAction(int sessionAction, TerminalSession newTerminalSession) {
        Logger.logDebug(LOG_TAG, "Processing sessionAction \"" + sessionAction + "\" for session \"" + newTerminalSession.mSessionName + "\"");

        switch (sessionAction) {
            case TERMUX_SERVICE.VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_OPEN_ACTIVITY:
                setCurrentStoredTerminalSession(newTerminalSession);
                if (mTermuxTerminalSessionClient != null)
                    mTermuxTerminalSessionClient.setCurrentSession(newTerminalSession);
                startTermuxActivity();
                break;
            case TERMUX_SERVICE.VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_OPEN_ACTIVITY:
                if (getTermuxSessionsSize() == 1)
                    setCurrentStoredTerminalSession(newTerminalSession);
                startTermuxActivity();
                break;
            case TERMUX_SERVICE.VALUE_EXTRA_SESSION_ACTION_SWITCH_TO_NEW_SESSION_AND_DONT_OPEN_ACTIVITY:
                setCurrentStoredTerminalSession(newTerminalSession);
                if (mTermuxTerminalSessionClient != null)
                    mTermuxTerminalSessionClient.setCurrentSession(newTerminalSession);
                break;
            case TERMUX_SERVICE.VALUE_EXTRA_SESSION_ACTION_KEEP_CURRENT_SESSION_AND_DONT_OPEN_ACTIVITY:
                if (getTermuxSessionsSize() == 1)
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
        // from background (services). If it is not granted, then TermuxSessions that are started will
        // show in Termux notification but will not run until user manually clicks the notification.
        if (PermissionUtils.validateDisplayOverOtherAppsPermissionForPostAndroid10(this, true)) {
            TermuxActivity.startTermuxActivity(this);
        } else {
            TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(this);
            if (preferences == null) return;
            if (preferences.arePluginErrorNotificationsEnabled())
                Logger.showToast(this, this.getString(R.string.error_display_over_other_apps_permission_not_granted), true);
        }
    }





    /** If {@link TermuxActivity} has not bound to the {@link TermuxService} yet or is destroyed, then
     * interface functions requiring the activity should not be available to the terminal sessions,
     * so we just return the {@link #mTermuxTerminalSessionClientBase}. Once {@link TermuxActivity} bind
     * callback is received, it should call {@link #setTermuxTerminalSessionClient} to set the
     * {@link TermuxService#mTermuxTerminalSessionClient} so that further terminal sessions are directly
     * passed the {@link TermuxTerminalSessionClient} object which fully implements the
     * {@link TerminalSessionClient} interface.
     *
     * @return Returns the {@link TermuxTerminalSessionClient} if {@link TermuxActivity} has bound with
     * {@link TermuxService}, otherwise {@link TermuxTerminalSessionClientBase}.
     */
    public synchronized TermuxTerminalSessionClientBase getTermuxTerminalSessionClient() {
        if (mTermuxTerminalSessionClient != null)
            return mTermuxTerminalSessionClient;
        else
            return mTermuxTerminalSessionClientBase;
    }

    /** This should be called when {@link TermuxActivity#onServiceConnected} is called to set the
     * {@link TermuxService#mTermuxTerminalSessionClient} variable and update the {@link TerminalSession}
     * and {@link TerminalEmulator} clients in case they were passed {@link TermuxTerminalSessionClientBase}
     * earlier.
     *
     * @param termuxTerminalSessionClient The {@link TermuxTerminalSessionClient} object that fully
     * implements the {@link TerminalSessionClient} interface.
     */
    public synchronized void setTermuxTerminalSessionClient(TermuxTerminalSessionClient termuxTerminalSessionClient) {
        mTermuxTerminalSessionClient = termuxTerminalSessionClient;

        for (int i = 0; i < mTermuxSessions.size(); i++)
            mTermuxSessions.get(i).getTerminalSession().updateTerminalSessionClient(mTermuxTerminalSessionClient);
    }

    /** This should be called when {@link TermuxActivity} has been destroyed and in {@link #onUnbind(Intent)}
     * so that the {@link TermuxService} and {@link TerminalSession} and {@link TerminalEmulator}
     * clients do not hold an activity references.
     */
    public synchronized void unsetTermuxTerminalSessionClient() {
        for (int i = 0; i < mTermuxSessions.size(); i++)
            mTermuxSessions.get(i).getTerminalSession().updateTerminalSessionClient(mTermuxTerminalSessionClientBase);

        mTermuxTerminalSessionClient = null;
    }





    private Notification buildNotification() {
        Resources res = getResources();

        // Set pending intent to be launched when notification is clicked
        Intent notificationIntent = TermuxActivity.newInstance(this);
        PendingIntent contentIntent = PendingIntent.getActivity(this, 0, notificationIntent, 0);


        // Set notification text
        int sessionCount = getTermuxSessionsSize();
        int taskCount = mTermuxTasks.size();
        String notificationText = sessionCount + " session" + (sessionCount == 1 ? "" : "s");
        if (taskCount > 0) {
            notificationText += ", " + taskCount + " task" + (taskCount == 1 ? "" : "s");
        }

        final boolean wakeLockHeld = mWakeLock != null;
        if (wakeLockHeld) notificationText += " (wake lock held)";


        // Set notification priority
        // If holding a wake or wifi lock consider the notification of high priority since it's using power,
        // otherwise use a low priority
        int priority = (wakeLockHeld) ? Notification.PRIORITY_HIGH : Notification.PRIORITY_LOW;


        // Build the notification
        Notification.Builder builder =  NotificationUtils.geNotificationBuilder(this,
            TermuxConstants.TERMUX_APP_NOTIFICATION_CHANNEL_ID, priority,
            TermuxConstants.TERMUX_APP_NAME, notificationText, null,
            contentIntent, null, NotificationUtils.NOTIFICATION_MODE_SILENT);
        if (builder == null)  return null;

        // No need to show a timestamp:
        builder.setShowWhen(false);

        // Set notification icon
        builder.setSmallIcon(R.drawable.ic_service_notification);

        // Set background color for small notification icon
        builder.setColor(0xFF607D8B);

        // TermuxSessions are always ongoing
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

        NotificationUtils.setupNotificationChannel(this, TermuxConstants.TERMUX_APP_NOTIFICATION_CHANNEL_ID,
            TermuxConstants.TERMUX_APP_NOTIFICATION_CHANNEL_NAME, NotificationManager.IMPORTANCE_LOW);
    }

    /** Update the shown foreground service notification after making any changes that affect it. */
    private synchronized void updateNotification() {
        if (mWakeLock == null && mTermuxSessions.isEmpty() && mTermuxTasks.isEmpty()) {
            // Exit if we are updating after the user disabled all locks with no sessions or tasks running.
            requestStopService();
        } else {
            ((NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE)).notify(TermuxConstants.TERMUX_APP_NOTIFICATION_ID, buildNotification());
        }
    }





    private void setCurrentStoredTerminalSession(TerminalSession session) {
        if (session == null) return;
        // Make the newly created session the current one to be displayed
        TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(this);
        if (preferences == null) return;
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
        if (index >= 0 && index < mTermuxSessions.size())
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
