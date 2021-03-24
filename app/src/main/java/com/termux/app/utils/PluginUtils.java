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
     * Send execution result of commands to the {@link PendingIntent} creator received by
     * execution service if {@code pendingIntent} is not {@code null}.
     *
     * @param context The {@link Context} that will be used to send result intent to the {@link PendingIntent} creator.
     * @param logLevel The log level to dump the result.
     * @param logTag The log tag to use for logging.
     * @param pendingIntent The {@link PendingIntent} sent by creator to the execution service.
     * @param stdout The value for {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT} extra of {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE} bundle of intent.
     * @param stderr The value for {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_STDERR} extra of {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE} bundle of intent.
     * @param exitCode The value for {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE} extra of {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE} bundle of intent.
     * @param errCode The value for {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_ERR} extra of {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE} bundle of intent.
     * @param errmsg The value for {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE_ERRMSG} extra of {@link TERMUX_SERVICE#EXTRA_PLUGIN_RESULT_BUNDLE} bundle of intent.
     */
    public static void sendExecuteResultToResultsService(final Context context, final int logLevel, final String logTag, final PendingIntent pendingIntent, final String stdout, final String stderr, final String exitCode, final String errCode, final String errmsg) {
        String label;

        if(pendingIntent == null)
            label = "Execution Result";
        else
            label = "Sending execution result to " + pendingIntent.getCreatorPackage();

        Logger.logMesssage(logLevel, logTag, label + ":\n" +
            TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT + ":\n```\n" + stdout + "\n```\n" +
            TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDERR + ":\n```\n" + stderr + "\n```\n" +
            TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE + ": `" + exitCode + "`\n" +
            TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERR + ": `" + errCode + "`\n" +
            TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERRMSG + ": `" + errmsg + "`");

        // If pendingIntent is null, then just return
        if(pendingIntent == null) return;

        final Bundle resultBundle = new Bundle();

        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT, stdout);
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDERR, stderr);
        if (exitCode != null && !exitCode.isEmpty()) resultBundle.putInt(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE, Integer.parseInt(exitCode));
        if (errCode != null && !errCode.isEmpty()) resultBundle.putInt(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERR, Integer.parseInt(errCode));
        resultBundle.putString(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERRMSG, errmsg);

        Intent resultIntent = new Intent();
        resultIntent.putExtra(TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE, resultBundle);

        if(context != null) {
            try {
                pendingIntent.send(context, Activity.RESULT_OK, resultIntent);
            } catch (PendingIntent.CanceledException e) {
                // The caller doesn't want the result? That's fine, just ignore
            }
        }
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

    /**
     * Proceses {@link ExecutionCommand} error.
     * The {@link ExecutionCommand#errCode} must have been set to a non-zero value.
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

        if(executionCommand.errCode == null || executionCommand.errCode == 0) {
            Logger.logWarn(LOG_TAG, "Ignoring call to processPluginExecutionCommandError() since the execution command errCode has not been set to a non-zero value");
            return;
        }

        // Log the error and any exception
        logTag = TextDataUtils.getDefaultIfNull(logTag, LOG_TAG);
        Logger.logStackTracesWithMessage(logTag, executionCommand.errmsg, executionCommand.throwableList);

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

}
