package com.termux.app.utils;

import android.app.Activity;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.Nullable;

import com.termux.R;
import com.termux.app.TermuxConstants;
import com.termux.app.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.app.activities.ReportActivity;
import com.termux.app.settings.preferences.TermuxAppSharedPreferences;
import com.termux.app.settings.preferences.TermuxPreferenceConstants;
import com.termux.app.settings.properties.SharedProperties;
import com.termux.app.settings.properties.TermuxPropertyConstants;
import com.termux.models.ReportInfo;
import com.termux.models.ExecutionCommand;
import com.termux.models.UserAction;

public class PluginUtils {

    private static final String NOTIFICATION_CHANNEL_ID_PLUGIN_COMMAND_ERRORS = "termux_plugin_command_errors_notification_channel";
    private static final String NOTIFICATION_CHANNEL_NAME_PLUGIN_COMMAND_ERRORS = TermuxConstants.TERMUX_APP_NAME + " Plugin Commands Errors";

    /** Required file permissions for the executable file of execute intent. Executable file must have read and execute permissions */
    public static final String PLUGIN_EXECUTABLE_FILE_PERMISSIONS = "r-x"; // Default: "r-x"
    /** Required file permissions for the working directory of execute intent. Working directory must have read and write permissions.
     * Execute permissions should be attempted to be set, but ignored if they are missing */
    public static final String PLUGIN_WORKING_DIRECTORY_PERMISSIONS = "rwx"; // Default: "rwx"

    private static final String LOG_TAG = "PluginUtils";

    /**
     * Process {@link ExecutionCommand} result.
     *
     * The ExecutionCommand currentState must be greater or equal to {@link ExecutionCommand.ExecutionState#EXECUTED}.
     * If the {@link ExecutionCommand#isPluginExecutionCommand} is {@code true} and {@link ExecutionCommand#pluginPendingIntent}
     * is not {@code null}, then the result of commands is sent back to the {@link PendingIntent} creator.
     *
     * @param context The {@link Context} that will be used to send result intent to the {@link PendingIntent} creator.
     * @param logTag The log tag to use for logging.
     * @param executionCommand The {@link ExecutionCommand} to process.
     */
    public static void processPluginExecutionCommandResult(final Context context, String logTag, final ExecutionCommand executionCommand) {
        if (executionCommand == null) return;

        if(!executionCommand.hasExecuted()) {
            Logger.logWarn(LOG_TAG, "Ignoring call to processPluginExecutionCommandResult() since the execution command has not been ExecutionState.EXECUTED");
            return;
        }

        // Must be a normal command like foreground terminal session started by user
        if(!executionCommand.isPluginExecutionCommand)
            return;

        logTag = TextDataUtils.getDefaultIfNull(logTag, LOG_TAG);

        Logger.logDebug(LOG_TAG, executionCommand.toString());


        // If pluginPendingIntent is null, then just return
        if(executionCommand.pluginPendingIntent == null) return;



        // Send pluginPendingIntent to its creator
        final Bundle resultBundle = new Bundle();

        Logger.logDebug(LOG_TAG,  "Sending execution result for Execution Command \"" + executionCommand.getCommandIdAndLabelLogString() +  "\" to " + executionCommand.pluginPendingIntent.getCreatorPackage());

        String truncatedStdout = null;
        String truncatedStderr = null;
        String truncatedErrmsg = null;

        String stdoutOriginalLength = (executionCommand.stdout == null) ? null: String.valueOf(executionCommand.stdout.length());
        String stderrOriginalLength = (executionCommand.stderr == null) ? null: String.valueOf(executionCommand.stderr.length());

        if(executionCommand.stderr == null || executionCommand.stderr.isEmpty()) {
            truncatedStdout = TextDataUtils.getTruncatedCommandOutput(executionCommand.stdout, TextDataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, false, false, false);
        } else if (executionCommand.stdout == null || executionCommand.stdout.isEmpty()) {
            truncatedStderr = TextDataUtils.getTruncatedCommandOutput(executionCommand.stderr, TextDataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, false, false, false);
        } else {
            truncatedStdout = TextDataUtils.getTruncatedCommandOutput(executionCommand.stdout, TextDataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES / 2, false, false, false);
            truncatedStderr = TextDataUtils.getTruncatedCommandOutput(executionCommand.stderr, TextDataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES / 2, false, false, false);
        }

        if(truncatedStdout != null && executionCommand.stdout != null && truncatedStdout.length() < executionCommand.stdout.length()){
            Logger.logWarn(logTag, "Execution Result for Execution Command \"" + executionCommand.getCommandIdAndLabelLogString() +  "\" stdout length truncated from " + stdoutOriginalLength + " to " + truncatedStdout.length());
            executionCommand.stdout = truncatedStdout;
        }

        if(truncatedStderr != null && executionCommand.stderr != null && truncatedStderr.length() < executionCommand.stderr.length()){
            Logger.logWarn(logTag, "Execution Result for Execution Command \"" + executionCommand.getCommandIdAndLabelLogString() +  "\" stderr length truncated from " + stderrOriginalLength + " to " + truncatedStderr.length());
            executionCommand.stderr = truncatedStderr;
        }


        //Combine errmsg and stacktraces
        if(executionCommand.isStateFailed()) {
            executionCommand.errmsg = Logger.getMessageAndStackTracesString(executionCommand.errmsg, executionCommand.throwableList);
        }

        String errmsgOriginalLength = (executionCommand.errmsg == null) ? null: String.valueOf(executionCommand.errmsg.length());

        // trim from end to preseve start of stacktraces
        truncatedErrmsg = TextDataUtils.getTruncatedCommandOutput(executionCommand.errmsg, TextDataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES / 4, true, false, false);
        if(truncatedErrmsg != null && executionCommand.errmsg != null && truncatedErrmsg.length() < executionCommand.errmsg.length()){
            Logger.logWarn(logTag, "Execution Result for Execution Command \"" + executionCommand.getCommandIdAndLabelLogString() +  "\" errmsg length truncated from " + errmsgOriginalLength + " to " + truncatedErrmsg.length());
            executionCommand.errmsg = truncatedErrmsg;
        }


        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT, executionCommand.stdout);
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT_ORIGINAL_LENGTH, stdoutOriginalLength);
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDERR, executionCommand.stderr);
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDERR_ORIGINAL_LENGTH, stderrOriginalLength);
        if (executionCommand.exitCode != null) resultBundle.putInt(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE, executionCommand.exitCode);
        if (executionCommand.errCode != null) resultBundle.putInt(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERR, executionCommand.errCode);
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERRMSG, executionCommand.errmsg);

        Intent resultIntent = new Intent();
        resultIntent.putExtra(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE, resultBundle);

        if(context != null) {
            try {
                executionCommand.pluginPendingIntent.send(context, Activity.RESULT_OK, resultIntent);
            } catch (PendingIntent.CanceledException e) {
                // The caller doesn't want the result? That's fine, just ignore
            }
        }

        if(!executionCommand.isStateFailed())
            executionCommand.setState(ExecutionCommand.ExecutionState.SUCCESS);

    }



    /**
     * Proceses {@link ExecutionCommand} error.
     *
     * The ExecutionCommand currentState must be equal to {@link ExecutionCommand.ExecutionState#FAILED}.
     * The {@link ExecutionCommand#errCode} must have been set to a value greater than {@link ExecutionCommand#RESULT_CODE_OK}.
     * The {@link ExecutionCommand#errmsg} and any {@link ExecutionCommand#throwableList} must also
     * be set with appropriate error info.
     * If the {@link TermuxPreferenceConstants.TERMUX_APP#KEY_PLUGIN_ERROR_NOTIFICATIONS_ENABLED} is
     * enabled, then a flash and a notification will be shown for the error as well
     * on the {@link #NOTIFICATION_CHANNEL_NAME_PLUGIN_COMMAND_ERRORS} channel.
     *
     * @param context The {@link Context} for operations.
     * @param logTag The log tag to use for logging.
     * @param executionCommand The {@link ExecutionCommand} that failed.
     */
    public static void processPluginExecutionCommandError(final Context context, String logTag, final ExecutionCommand executionCommand) {
        if(context == null || executionCommand == null) return;

        if(!executionCommand.isStateFailed()) {
            Logger.logWarn(LOG_TAG, "Ignoring call to processPluginExecutionCommandError() since the execution command does not have ExecutionState.FAILED state");
            return;
        }

        // Log the error and any exception
        logTag = TextDataUtils.getDefaultIfNull(logTag, LOG_TAG);
        Logger.logStackTracesWithMessage(logTag, "(" + executionCommand.errCode + ") " + executionCommand.errmsg, executionCommand.throwableList);

        TermuxAppSharedPreferences preferences = new TermuxAppSharedPreferences(context);
        // If user has disabled notifications for plugin, then just return
        if (!preferences.getPluginErrorNotificationsEnabled())
            return;

        // Flash the errmsg
        Logger.showToast(context, executionCommand.errmsg, true);

        // Send a notification to show the errmsg which when clicked will open the {@link ReportActivity}
        // to show the details of the error
        String title = TermuxConstants.TERMUX_APP_NAME + " Plugin Execution Command Error";

        Intent notificationIntent = ReportActivity.newInstance(context, new ReportInfo(UserAction.PLUGIN_EXECUTION_COMMAND, logTag, title, ExecutionCommand.getDetailedMarkdownString(executionCommand), true));
        PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        // Setup the notification channel if not already set up
        setupPluginCommandErrorsNotificationChannel(context);

        // Use markdown in notification
        CharSequence notifiationText = MarkdownUtils.getSpannedMarkdownText(context, executionCommand.errmsg);
        //CharSequence notifiationText = executionCommand.errmsg;

        // Build the notification
        Notification.Builder builder = getPluginCommandErrorsNotificationBuilder(context, title, notifiationText, notifiationText, pendingIntent, NotificationUtils.NOTIFICATION_MODE_VIBRATE);
        if(builder == null)  return;

        // Send the notification
        int nextNotificationId = NotificationUtils.getNextNotificationId(context);
        NotificationManager notificationManager = NotificationUtils.getNotificationManager(context);
        if(notificationManager != null)
            notificationManager.notify(nextNotificationId, builder.build());
    }



    /**
     * Get {@link Notification.Builder} for {@link #NOTIFICATION_CHANNEL_ID_PLUGIN_COMMAND_ERRORS}
     * and {@link #NOTIFICATION_CHANNEL_NAME_PLUGIN_COMMAND_ERRORS}.
     *
     * @param context The {@link Context} for operations.
     * @param title The title for the notification.
     * @param notifiationText The second line text of the notification.
     * @param notificationBigText The full text of the notification that may optionally be styled.
     * @param pendingIntent The {@link PendingIntent} which should be sent when notification is clicked.
     * @param notificationMode The notification mode. It must be one of {@code NotificationUtils.NOTIFICATION_MODE_*}.
     * @return Returns the {@link Notification.Builder}.
     */
    @Nullable
    public static Notification.Builder getPluginCommandErrorsNotificationBuilder(final Context context, final CharSequence title, final CharSequence notifiationText, final CharSequence notificationBigText, final PendingIntent pendingIntent, final int notificationMode) {

        Notification.Builder builder =  NotificationUtils.geNotificationBuilder(context,
            NOTIFICATION_CHANNEL_ID_PLUGIN_COMMAND_ERRORS, Notification.PRIORITY_HIGH,
            title, notifiationText, notificationBigText, pendingIntent, notificationMode);

        if(builder == null)  return null;

        // Enable timestamp
        builder.setShowWhen(true);

        // Set notification icon
        builder.setSmallIcon(R.drawable.ic_error_notification);

        // Set background color for small notification icon
        builder.setColor(0xFF607D8B);

        // Dismiss on click
        builder.setAutoCancel(true);

        return builder;
    }

    /**
     * Setup the notification channel for {@link #NOTIFICATION_CHANNEL_ID_PLUGIN_COMMAND_ERRORS} and
     * {@link #NOTIFICATION_CHANNEL_NAME_PLUGIN_COMMAND_ERRORS}.
     *
     * @param context The {@link Context} for operations.
     */
    public static void setupPluginCommandErrorsNotificationChannel(final Context context) {
        NotificationUtils.setupNotificationChannel(context, NOTIFICATION_CHANNEL_ID_PLUGIN_COMMAND_ERRORS,
            NOTIFICATION_CHANNEL_NAME_PLUGIN_COMMAND_ERRORS, NotificationManager.IMPORTANCE_HIGH);
    }



    /**
     * Check if {@link TermuxConstants#PROP_ALLOW_EXTERNAL_APPS} property is not set to "true".
     *
     * @param context The {@link Context} to get error string.
     * @return Returns the {@code errmsg} if policy is violated, otherwise {@code null}.
     */
    public static String checkIfRunCommandServiceAllowExternalAppsPolicyIsViolated(final Context context) {
        String errmsg = null;
        if (!SharedProperties.isPropertyValueTrue(context, TermuxPropertyConstants.getTermuxPropertiesFile(), TermuxConstants.PROP_ALLOW_EXTERNAL_APPS)) {
            errmsg = context.getString(R.string.error_run_command_service_allow_external_apps_ungranted);
        }

        return errmsg;
    }

}
