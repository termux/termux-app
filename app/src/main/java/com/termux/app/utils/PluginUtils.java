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
import com.termux.shared.notification.NotificationUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.app.activities.ReportActivity;
import com.termux.shared.logger.Logger;
import com.termux.shared.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.settings.preferences.TermuxPreferenceConstants.TERMUX_APP;
import com.termux.shared.settings.properties.SharedProperties;
import com.termux.shared.settings.properties.TermuxPropertyConstants;
import com.termux.app.models.ReportInfo;
import com.termux.shared.models.ExecutionCommand;
import com.termux.app.models.UserAction;
import com.termux.shared.data.DataUtils;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.termux.TermuxUtils;

public class PluginUtils {

    /** Required file permissions for the executable file of execute intent. Executable file must have read and execute permissions */
    public static final String PLUGIN_EXECUTABLE_FILE_PERMISSIONS = "r-x"; // Default: "r-x"
    /** Required file permissions for the working directory of execute intent. Working directory must have read and write permissions.
     * Execute permissions should be attempted to be set, but ignored if they are missing */
    public static final String PLUGIN_WORKING_DIRECTORY_PERMISSIONS = "rwx"; // Default: "rwx"

    private static final String LOG_TAG = "PluginUtils";

    /**
     * Process {@link ExecutionCommand} result.
     *
     * The ExecutionCommand currentState must be greater or equal to
     * {@link ExecutionCommand.ExecutionState#EXECUTED}.
     * If the {@link ExecutionCommand#isPluginExecutionCommand} is {@code true} and
     * {@link ExecutionCommand#pluginPendingIntent} is not {@code null}, then the result of commands
     * are sent back to the {@link PendingIntent} creator.
     *
     * @param context The {@link Context} that will be used to send result intent to the {@link PendingIntent} creator.
     * @param logTag The log tag to use for logging.
     * @param executionCommand The {@link ExecutionCommand} to process.
     */
    public static void processPluginExecutionCommandResult(final Context context, String logTag, final ExecutionCommand executionCommand) {
        if (executionCommand == null) return;

        logTag = DataUtils.getDefaultIfNull(logTag, LOG_TAG);

        if (!executionCommand.hasExecuted()) {
            Logger.logWarn(logTag, "Ignoring call to processPluginExecutionCommandResult() since the execution command state is not higher than the ExecutionState.EXECUTED");
            return;
        }

        Logger.logDebug(LOG_TAG, executionCommand.toString());

        boolean result = true;

        // If isPluginExecutionCommand is true and pluginPendingIntent is not null, then
        // send pluginPendingIntent to its creator with the result
        if (executionCommand.isPluginExecutionCommand && executionCommand.pluginPendingIntent != null) {
            String errmsg = executionCommand.errmsg;

            //Combine errmsg and stacktraces
            if (executionCommand.isStateFailed()) {
                errmsg = Logger.getMessageAndStackTracesString(executionCommand.errmsg, executionCommand.throwableList);
            }

            // Send pluginPendingIntent to its creator
            result = sendPluginExecutionCommandResultPendingIntent(context, logTag, executionCommand.getCommandIdAndLabelLogString(), executionCommand.stdout, executionCommand.stderr, executionCommand.exitCode, executionCommand.errCode, errmsg, executionCommand.pluginPendingIntent);
        }

        if (!executionCommand.isStateFailed() && result)
            executionCommand.setState(ExecutionCommand.ExecutionState.SUCCESS);
    }

    /**
     * Process {@link ExecutionCommand} error.
     *
     * The ExecutionCommand currentState must be equal to {@link ExecutionCommand.ExecutionState#FAILED}.
     * The {@link ExecutionCommand#errCode} must have been set to a value greater than
     * {@link ExecutionCommand#RESULT_CODE_OK}.
     * The {@link ExecutionCommand#errmsg} and any {@link ExecutionCommand#throwableList} must also
     * be set with appropriate error info.
     *
     * If the {@link ExecutionCommand#isPluginExecutionCommand} is {@code true} and
     * {@link ExecutionCommand#pluginPendingIntent} is not {@code null}, then the errors of commands
     * are sent back to the {@link PendingIntent} creator.
     *
     * Otherwise if the {@link TERMUX_APP#KEY_PLUGIN_ERROR_NOTIFICATIONS_ENABLED} is
     * enabled, then a flash and a notification will be shown for the error as well
     * on the {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME} channel instead of just logging
     * the error.
     *
     * @param context The {@link Context} for operations.
     * @param logTag The log tag to use for logging.
     * @param executionCommand The {@link ExecutionCommand} that failed.
     * @param forceNotification If set to {@code true}, then a flash and notification will be shown
     *                          regardless of if pending intent is {@code null} or
     *                          {@link TERMUX_APP#KEY_PLUGIN_ERROR_NOTIFICATIONS_ENABLED}
     *                          is {@code false}.
     */
    public static void processPluginExecutionCommandError(final Context context, String logTag, final ExecutionCommand executionCommand, boolean forceNotification) {
        if (context == null || executionCommand == null) return;

        logTag = DataUtils.getDefaultIfNull(logTag, LOG_TAG);

        if (!executionCommand.isStateFailed()) {
            Logger.logWarn(logTag, "Ignoring call to processPluginExecutionCommandError() since the execution command is not in ExecutionState.FAILED");
            return;
        }

        // Log the error and any exception
        Logger.logStackTracesWithMessage(logTag, "(" + executionCommand.errCode + ") " + executionCommand.errmsg, executionCommand.throwableList);


        // If isPluginExecutionCommand is true and pluginPendingIntent is not null, then
        // send pluginPendingIntent to its creator with the errors
        if (executionCommand.isPluginExecutionCommand && executionCommand.pluginPendingIntent != null) {
            String errmsg = executionCommand.errmsg;

            //Combine errmsg and stacktraces
            if (executionCommand.isStateFailed()) {
                errmsg = Logger.getMessageAndStackTracesString(executionCommand.errmsg, executionCommand.throwableList);
            }

            sendPluginExecutionCommandResultPendingIntent(context, logTag, executionCommand.getCommandIdAndLabelLogString(), executionCommand.stdout, executionCommand.stderr, executionCommand.exitCode, executionCommand.errCode, errmsg, executionCommand.pluginPendingIntent);

            // No need to show notifications if a pending intent was sent, let the caller handle the result himself
            if (!forceNotification) return;
        }


        TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(context);
        if (preferences == null) return;

        // If user has disabled notifications for plugin, then just return
        if (!preferences.arePluginErrorNotificationsEnabled() && !forceNotification)
            return;

        // Flash the errmsg
        Logger.showToast(context, executionCommand.errmsg, true);

        // Send a notification to show the errmsg which when clicked will open the {@link ReportActivity}
        // to show the details of the error
        String title = TermuxConstants.TERMUX_APP_NAME + " Plugin Execution Command Error";

        StringBuilder reportString = new StringBuilder();

        reportString.append(ExecutionCommand.getExecutionCommandMarkdownString(executionCommand));
        reportString.append("\n\n").append(TermuxUtils.getAppInfoMarkdownString(context, true));
        reportString.append("\n\n").append(TermuxUtils.getDeviceInfoMarkdownString(context));

        Intent notificationIntent = ReportActivity.newInstance(context, new ReportInfo(UserAction.PLUGIN_EXECUTION_COMMAND, logTag, title, null, reportString.toString(), null,true));
        PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        // Setup the notification channel if not already set up
        setupPluginCommandErrorsNotificationChannel(context);

        // Use markdown in notification
        CharSequence notificationText = MarkdownUtils.getSpannedMarkdownText(context, executionCommand.errmsg);
        //CharSequence notificationText = executionCommand.errmsg;

        // Build the notification
        Notification.Builder builder = getPluginCommandErrorsNotificationBuilder(context, title, notificationText, notificationText, pendingIntent, NotificationUtils.NOTIFICATION_MODE_VIBRATE);
        if (builder == null)  return;

        // Send the notification
        int nextNotificationId = NotificationUtils.getNextNotificationId(context);
        NotificationManager notificationManager = NotificationUtils.getNotificationManager(context);
        if (notificationManager != null)
            notificationManager.notify(nextNotificationId, builder.build());

    }

    /**
     * Send {@link ExecutionCommand} result {@link PendingIntent} in the
     * {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE} bundle.
     *
     *
     * @param context The {@link Context} that will be used to send result intent to the {@link PendingIntent} creator.
     * @param logTag The log tag to use for logging.
     * @param label The label of {@link ExecutionCommand}.
     * @param stdout The stdout of {@link ExecutionCommand}.
     * @param stderr The stderr of {@link ExecutionCommand}.
     * @param exitCode The exitCode of {@link ExecutionCommand}.
     * @param errCode The errCode of {@link ExecutionCommand}.
     * @param errmsg The errmsg of {@link ExecutionCommand}.
     * @param pluginPendingIntent The pluginPendingIntent of {@link ExecutionCommand}.
     * @return Returns {@code true} if pluginPendingIntent was successfully send, otherwise [@code false}.
     */
    public static boolean sendPluginExecutionCommandResultPendingIntent(Context context, String logTag, String label, String stdout, String stderr, Integer exitCode, Integer errCode, String errmsg, PendingIntent pluginPendingIntent) {
        if (context == null || pluginPendingIntent == null) return false;

        logTag = DataUtils.getDefaultIfNull(logTag, LOG_TAG);

        Logger.logDebug(logTag,  "Sending execution result for Execution Command \"" + label +  "\" to " + pluginPendingIntent.getCreatorPackage());

        String truncatedStdout = null;
        String truncatedStderr = null;

        String stdoutOriginalLength = (stdout == null) ? null: String.valueOf(stdout.length());
        String stderrOriginalLength = (stderr == null) ? null: String.valueOf(stderr.length());

        // Truncate stdout and stdout to max TRANSACTION_SIZE_LIMIT_IN_BYTES
        if (stderr == null || stderr.isEmpty()) {
            truncatedStdout = DataUtils.getTruncatedCommandOutput(stdout, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, false, false, false);
        } else if (stdout == null || stdout.isEmpty()) {
            truncatedStderr = DataUtils.getTruncatedCommandOutput(stderr, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, false, false, false);
        } else {
            truncatedStdout = DataUtils.getTruncatedCommandOutput(stdout, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES / 2, false, false, false);
            truncatedStderr = DataUtils.getTruncatedCommandOutput(stderr, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES / 2, false, false, false);
        }

        if (truncatedStdout != null && truncatedStdout.length() < stdout.length()) {
            Logger.logWarn(logTag, "Execution Result for Execution Command \"" + label +  "\" stdout length truncated from " + stdoutOriginalLength + " to " + truncatedStdout.length());
            stdout = truncatedStdout;
        }

        if (truncatedStderr != null && truncatedStderr.length() < stderr.length()) {
            Logger.logWarn(logTag, "Execution Result for Execution Command \"" + label +  "\" stderr length truncated from " + stderrOriginalLength + " to " + truncatedStderr.length());
            stderr = truncatedStderr;
        }

        String errmsgOriginalLength = (errmsg == null) ? null: String.valueOf(errmsg.length());

        // Truncate errmsg to max TRANSACTION_SIZE_LIMIT_IN_BYTES / 4
        // trim from end to preserve start of stacktraces
        String truncatedErrmsg = DataUtils.getTruncatedCommandOutput(errmsg, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES / 4, true, false, false);
        if (truncatedErrmsg != null && truncatedErrmsg.length() < errmsg.length()) {
            Logger.logWarn(logTag, "Execution Result for Execution Command \"" + label +  "\" errmsg length truncated from " + errmsgOriginalLength + " to " + truncatedErrmsg.length());
            errmsg = truncatedErrmsg;
        }


        final Bundle resultBundle = new Bundle();
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT, stdout);
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT_ORIGINAL_LENGTH, stdoutOriginalLength);
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDERR, stderr);
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDERR_ORIGINAL_LENGTH, stderrOriginalLength);
        if (exitCode != null) resultBundle.putInt(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE, exitCode);
        if (errCode != null) resultBundle.putInt(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERR, errCode);
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERRMSG, errmsg);

        Intent resultIntent = new Intent();
        resultIntent.putExtra(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE, resultBundle);

        try {
            pluginPendingIntent.send(context, Activity.RESULT_OK, resultIntent);
        } catch (PendingIntent.CanceledException e) {
            // The caller doesn't want the result? That's fine, just ignore
            Logger.logDebug(logTag,  "The Execution Command \"" + label +  "\" creator " + pluginPendingIntent.getCreatorPackage() + " does not want the results anymore");
        }

        return true;
    }



    /**
     * Get {@link Notification.Builder} for {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param context The {@link Context} for operations.
     * @param title The title for the notification.
     * @param notificationText The second line text of the notification.
     * @param notificationBigText The full text of the notification that may optionally be styled.
     * @param pendingIntent The {@link PendingIntent} which should be sent when notification is clicked.
     * @param notificationMode The notification mode. It must be one of {@code NotificationUtils.NOTIFICATION_MODE_*}.
     * @return Returns the {@link Notification.Builder}.
     */
    @Nullable
    public static Notification.Builder getPluginCommandErrorsNotificationBuilder(final Context context, final CharSequence title, final CharSequence notificationText, final CharSequence notificationBigText, final PendingIntent pendingIntent, final int notificationMode) {

        Notification.Builder builder =  NotificationUtils.geNotificationBuilder(context,
            TermuxConstants.TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID, Notification.PRIORITY_HIGH,
            title, notificationText, notificationBigText, pendingIntent, notificationMode);

        if (builder == null)  return null;

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
     * Setup the notification channel for {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID} and
     * {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param context The {@link Context} for operations.
     */
    public static void setupPluginCommandErrorsNotificationChannel(final Context context) {
        NotificationUtils.setupNotificationChannel(context, TermuxConstants.TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID,
            TermuxConstants.TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME, NotificationManager.IMPORTANCE_HIGH);
    }



    /**
     * Check if {@link TermuxConstants#PROP_ALLOW_EXTERNAL_APPS} property is not set to "true".
     *
     * @param context The {@link Context} to get error string.
     * @return Returns the {@code errmsg} if policy is violated, otherwise {@code null}.
     */
    public static String checkIfRunCommandServiceAllowExternalAppsPolicyIsViolated(final Context context) {
        String errmsg = null;
        if (!SharedProperties.isPropertyValueTrue(context, TermuxPropertyConstants.getTermuxPropertiesFile(), TermuxConstants.PROP_ALLOW_EXTERNAL_APPS, true)) {
            errmsg = context.getString(R.string.error_run_command_service_allow_external_apps_ungranted);
        }

        return errmsg;
    }

}
