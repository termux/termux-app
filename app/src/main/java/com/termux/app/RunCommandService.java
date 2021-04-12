package com.termux.app;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.Build;
import android.os.IBinder;

import com.termux.R;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.RUN_COMMAND_SERVICE;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.notification.NotificationUtils;
import com.termux.app.utils.PluginUtils;
import com.termux.shared.data.DataUtils;
import com.termux.shared.models.ExecutionCommand;

/**
 * A service that receives {@link RUN_COMMAND_SERVICE#ACTION_RUN_COMMAND} intent from third party apps and
 * plugins that contains info on command execution and forwards the extras to {@link TermuxService}
 * for the actual execution.
 *
 * Check https://github.com/termux/termux-app/wiki/RUN_COMMAND-Intent for more info.
 */
public class RunCommandService extends Service {

    private static final String LOG_TAG = "RunCommandService";

    class LocalBinder extends Binder {
        public final RunCommandService service = RunCommandService.this;
    }

    private final IBinder mBinder = new RunCommandService.LocalBinder();

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    @Override
    public void onCreate() {
        Logger.logVerbose(LOG_TAG, "onCreate");
        runStartForeground();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Logger.logDebug(LOG_TAG, "onStartCommand");

        if (intent == null) return Service.START_NOT_STICKY;

        // Run again in case service is already started and onCreate() is not called
        runStartForeground();

        ExecutionCommand executionCommand = new ExecutionCommand();
        executionCommand.pluginAPIHelp = this.getString(R.string.error_run_command_service_api_help, RUN_COMMAND_SERVICE.RUN_COMMAND_API_HELP_URL);

        String errmsg;

        // If invalid action passed, then just return
        if (!RUN_COMMAND_SERVICE.ACTION_RUN_COMMAND.equals(intent.getAction())) {
            errmsg = this.getString(R.string.error_run_command_service_invalid_intent_action, intent.getAction());
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, errmsg, null);
            PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, false);
            return Service.START_NOT_STICKY;
        }

        executionCommand.executable = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_COMMAND_PATH);
        executionCommand.arguments = intent.getStringArrayExtra(RUN_COMMAND_SERVICE.EXTRA_ARGUMENTS);
        executionCommand.stdin = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_STDIN);
        executionCommand.workingDirectory = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_WORKDIR);
        executionCommand.inBackground = intent.getBooleanExtra(RUN_COMMAND_SERVICE.EXTRA_BACKGROUND, false);
        executionCommand.sessionAction = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_SESSION_ACTION);
        executionCommand.commandLabel = DataUtils.getDefaultIfNull(intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_COMMAND_LABEL), "RUN_COMMAND Execution Intent Command");
        executionCommand.commandDescription = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_COMMAND_DESCRIPTION);
        executionCommand.commandHelp = intent.getStringExtra(RUN_COMMAND_SERVICE.EXTRA_COMMAND_HELP);
        executionCommand.isPluginExecutionCommand = true;
        executionCommand.pluginPendingIntent = intent.getParcelableExtra(RUN_COMMAND_SERVICE.EXTRA_PENDING_INTENT);



        // If "allow-external-apps" property to not set to "true", then just return
        // We enable force notifications if "allow-external-apps" policy is violated so that the
        // user knows someone tried to run a command in termux context, since it may be malicious
        // app or imported (tasker) plugin project and not the user himself. If a pending intent is
        // also sent, then its creator is also logged and shown.
        errmsg = PluginUtils.checkIfRunCommandServiceAllowExternalAppsPolicyIsViolated(this);
        if (errmsg != null) {
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, errmsg, null);
            PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, true);
            return Service.START_NOT_STICKY;
        }



        // If executable is null or empty, then exit here instead of getting canonical path which would expand to "/"
        if (executionCommand.executable == null || executionCommand.executable.isEmpty()) {
            errmsg  = this.getString(R.string.error_run_command_service_mandatory_extra_missing, RUN_COMMAND_SERVICE.EXTRA_COMMAND_PATH);
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, errmsg, null);
            PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, false);
            return Service.START_NOT_STICKY;
        }

        // Get canonical path of executable
        executionCommand.executable = FileUtils.getCanonicalPath(executionCommand.executable, null, true);

        // If executable is not a regular file, or is not readable or executable, then just return
        // Setting of missing read and execute permissions is not done
        errmsg = FileUtils.validateRegularFileExistenceAndPermissions(this, "executable", executionCommand.executable, null,
            PluginUtils.PLUGIN_EXECUTABLE_FILE_PERMISSIONS, true, true,
            false);
        if (errmsg != null) {
            errmsg  += "\n" + this.getString(R.string.msg_executable_absolute_path, executionCommand.executable);
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, errmsg, null);
            PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, false);
            return Service.START_NOT_STICKY;
        }



        // If workingDirectory is not null or empty
        if (executionCommand.workingDirectory != null && !executionCommand.workingDirectory.isEmpty()) {
            // Get canonical path of workingDirectory
            executionCommand.workingDirectory = FileUtils.getCanonicalPath(executionCommand.workingDirectory, null, true);

            // If workingDirectory is not a directory, or is not readable or writable, then just return
            // Creation of missing directory and setting of read, write and execute permissions are only done if workingDirectory is
            // under {@link TermuxConstants#TERMUX_FILES_DIR_PATH}
            // We try to set execute permissions, but ignore if they are missing, since only read and write permissions are required
            // for working directories.
            errmsg = FileUtils.validateDirectoryFileExistenceAndPermissions(this, "working", executionCommand.workingDirectory, TermuxConstants.TERMUX_FILES_DIR_PATH, true,
                PluginUtils.PLUGIN_WORKING_DIRECTORY_PERMISSIONS, true, true,
                true, true);
            if (errmsg != null) {
                errmsg  += "\n" + this.getString(R.string.msg_working_directory_absolute_path, executionCommand.workingDirectory);
                executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, errmsg, null);
                PluginUtils.processPluginExecutionCommandError(this, LOG_TAG, executionCommand, false);
                return Service.START_NOT_STICKY;
            }
        }



        executionCommand.executableUri = new Uri.Builder().scheme(TERMUX_SERVICE.URI_SCHEME_SERVICE_EXECUTE).path(FileUtils.getExpandedTermuxPath(executionCommand.executable)).build();

        Logger.logVerbose(LOG_TAG, executionCommand.toString());

        // Create execution intent with the action TERMUX_SERVICE#ACTION_SERVICE_EXECUTE to be sent to the TERMUX_SERVICE
        Intent execIntent = new Intent(TERMUX_SERVICE.ACTION_SERVICE_EXECUTE, executionCommand.executableUri);
        execIntent.setClass(this, TermuxService.class);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_ARGUMENTS, executionCommand.arguments);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_STDIN, executionCommand.stdin);
        if (executionCommand.workingDirectory != null && !executionCommand.workingDirectory.isEmpty()) execIntent.putExtra(TERMUX_SERVICE.EXTRA_WORKDIR, executionCommand.workingDirectory);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_BACKGROUND, executionCommand.inBackground);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_SESSION_ACTION, executionCommand.sessionAction);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_COMMAND_LABEL, executionCommand.commandLabel);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_COMMAND_DESCRIPTION, executionCommand.commandDescription);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_COMMAND_HELP, executionCommand.commandHelp);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_PLUGIN_API_HELP, executionCommand.pluginAPIHelp);
        execIntent.putExtra(TERMUX_SERVICE.EXTRA_PENDING_INTENT, executionCommand.pluginPendingIntent);

        // Start TERMUX_SERVICE and pass it execution intent
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            this.startForegroundService(execIntent);
        } else {
            this.startService(execIntent);
        }

        runStopForeground();

        return Service.START_NOT_STICKY;
    }

    private void runStartForeground() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            setupNotificationChannel();
            startForeground(TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_ID, buildNotification());
        }
    }

    private void runStopForeground() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            stopForeground(true);
        }
    }

    private Notification buildNotification() {
        // Build the notification
        Notification.Builder builder =  NotificationUtils.geNotificationBuilder(this,
            TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_ID, Notification.PRIORITY_LOW,
            TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_NAME, null, null,
            null, NotificationUtils.NOTIFICATION_MODE_SILENT);
        if (builder == null)  return null;

        // No need to show a timestamp:
        builder.setShowWhen(false);

        // Set notification icon
        builder.setSmallIcon(R.drawable.ic_service_notification);

        // Set background color for small notification icon
        builder.setColor(0xFF607D8B);

        return builder.build();
    }

    private void setupNotificationChannel() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;

        NotificationUtils.setupNotificationChannel(this, TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_ID,
            TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_CHANNEL_NAME, NotificationManager.IMPORTANCE_LOW);
    }

}
