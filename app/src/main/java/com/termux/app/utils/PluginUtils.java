package com.termux.app.utils;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.os.Environment;

import androidx.annotation.Nullable;

import com.termux.R;
import com.termux.shared.activities.ReportActivity;
import com.termux.shared.file.FileUtils;
import com.termux.shared.file.TermuxFileUtils;
import com.termux.shared.models.ResultConfig;
import com.termux.shared.models.ResultData;
import com.termux.shared.models.errors.Errno;
import com.termux.shared.models.errors.Error;
import com.termux.shared.notification.NotificationUtils;
import com.termux.shared.notification.TermuxNotificationUtils;
import com.termux.shared.shell.ResultSender;
import com.termux.shared.shell.ShellUtils;
import com.termux.shared.termux.AndroidUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.shared.logger.Logger;
import com.termux.shared.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.settings.preferences.TermuxPreferenceConstants.TERMUX_APP;
import com.termux.shared.settings.properties.SharedProperties;
import com.termux.shared.settings.properties.TermuxPropertyConstants;
import com.termux.shared.models.ReportInfo;
import com.termux.shared.models.ExecutionCommand;
import com.termux.app.models.UserAction;
import com.termux.shared.data.DataUtils;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.termux.TermuxUtils;

public class PluginUtils {

    private static final String LOG_TAG = "PluginUtils";

    /**
     * Process {@link ExecutionCommand} result.
     *
     * The ExecutionCommand currentState must be greater or equal to
     * {@link ExecutionCommand.ExecutionState#EXECUTED}.
     * If the {@link ExecutionCommand#isPluginExecutionCommand} is {@code true} and
     * {@link ResultConfig#resultPendingIntent} or {@link ResultConfig#resultDirectoryPath}
     * is not {@code null}, then the result of commands are sent back to the command caller.
     *
     * @param context The {@link Context} that will be used to send result intent to the {@link PendingIntent} creator.
     * @param logTag The log tag to use for logging.
     * @param executionCommand The {@link ExecutionCommand} to process.
     */
    public static void processPluginExecutionCommandResult(final Context context, String logTag, final ExecutionCommand executionCommand) {
        if (executionCommand == null) return;

        logTag = DataUtils.getDefaultIfNull(logTag, LOG_TAG);
        Error error = null;
        ResultData resultData = executionCommand.resultData;

        if (!executionCommand.hasExecuted()) {
            Logger.logWarn(logTag, executionCommand.getCommandIdAndLabelLogString() + ": Ignoring call to processPluginExecutionCommandResult() since the execution command state is not higher than the ExecutionState.EXECUTED");
            return;
        }

        boolean isPluginExecutionCommandWithPendingResult = executionCommand.isPluginExecutionCommandWithPendingResult();
        boolean isExecutionCommandLoggingEnabled = Logger.shouldEnableLoggingForCustomLogLevel(executionCommand.backgroundCustomLogLevel);

        // Log the output. ResultData should not be logged if pending result since ResultSender will do it
        // or if logging is disabled
        Logger.logDebugExtended(logTag, ExecutionCommand.getExecutionOutputLogString(executionCommand, true,
            !isPluginExecutionCommandWithPendingResult, isExecutionCommandLoggingEnabled));

        // If execution command was started by a plugin which expects the result back
        if (isPluginExecutionCommandWithPendingResult) {
            // Set variables which will be used by sendCommandResultData to send back the result
            if (executionCommand.resultConfig.resultPendingIntent != null)
                setPluginResultPendingIntentVariables(executionCommand);
            if (executionCommand.resultConfig.resultDirectoryPath != null)
                setPluginResultDirectoryVariables(executionCommand);

            // Send result to caller
            error = ResultSender.sendCommandResultData(context, logTag, executionCommand.getCommandIdAndLabelLogString(),
                executionCommand.resultConfig, executionCommand.resultData, isExecutionCommandLoggingEnabled);
            if (error != null) {
                // error will be added to existing Errors
                resultData.setStateFailed(error);
                Logger.logDebugExtended(logTag, ExecutionCommand.getExecutionOutputLogString(executionCommand, true, true, isExecutionCommandLoggingEnabled));

                // Flash and send notification for the error
                Logger.showToast(context, ResultData.getErrorsListMinimalString(resultData), true);
                sendPluginCommandErrorNotification(context, logTag, executionCommand, ResultData.getErrorsListMinimalString(resultData));
            }

        }

        if (!executionCommand.isStateFailed() && error == null)
            executionCommand.setState(ExecutionCommand.ExecutionState.SUCCESS);
    }

    /**
     * Process {@link ExecutionCommand} error.
     *
     * The ExecutionCommand currentState must be equal to {@link ExecutionCommand.ExecutionState#FAILED}.
     * The {@link ResultData#getErrCode()} must have been set to a value greater than
     * {@link Errno#ERRNO_SUCCESS}.
     * The {@link ResultData#errorsList} must also be set with appropriate error info.
     *
     * If the {@link ExecutionCommand#isPluginExecutionCommand} is {@code true} and
     * {@link ResultConfig#resultPendingIntent} or {@link ResultConfig#resultDirectoryPath}
     * is not {@code null}, then the errors of commands are sent back to the command caller.
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
        Error error;
        ResultData resultData = executionCommand.resultData;

        if (!executionCommand.isStateFailed()) {
            Logger.logWarn(logTag, executionCommand.getCommandIdAndLabelLogString() + ": Ignoring call to processPluginExecutionCommandError() since the execution command is not in ExecutionState.FAILED");
            return;
        }

        boolean isPluginExecutionCommandWithPendingResult = executionCommand.isPluginExecutionCommandWithPendingResult();
        boolean isExecutionCommandLoggingEnabled = Logger.shouldEnableLoggingForCustomLogLevel(executionCommand.backgroundCustomLogLevel);

        // Log the error and any exception. ResultData should not be logged if pending result since ResultSender will do it
        Logger.logErrorExtended(logTag, ExecutionCommand.getExecutionOutputLogString(executionCommand, true,
            !isPluginExecutionCommandWithPendingResult, isExecutionCommandLoggingEnabled));

        // If execution command was started by a plugin which expects the result back
        if (isPluginExecutionCommandWithPendingResult) {
            // Set variables which will be used by sendCommandResultData to send back the result
            if (executionCommand.resultConfig.resultPendingIntent != null)
                setPluginResultPendingIntentVariables(executionCommand);
            if (executionCommand.resultConfig.resultDirectoryPath != null)
                setPluginResultDirectoryVariables(executionCommand);

            // Send result to caller
            error = ResultSender.sendCommandResultData(context, logTag, executionCommand.getCommandIdAndLabelLogString(),
                executionCommand.resultConfig, executionCommand.resultData, isExecutionCommandLoggingEnabled);
            if (error != null) {
                // error will be added to existing Errors
                resultData.setStateFailed(error);
                Logger.logErrorExtended(logTag, ExecutionCommand.getExecutionOutputLogString(executionCommand, true, true, isExecutionCommandLoggingEnabled));
                forceNotification = true;
            }

            // No need to show notifications if a pending intent was sent, let the caller handle the result himself
            if (!forceNotification) return;
        }

        TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(context);
        if (preferences == null) return;

        // If user has disabled notifications for plugin commands, then just return
        if (!preferences.arePluginErrorNotificationsEnabled() && !forceNotification)
            return;

        // Flash and send notification for the error
        Logger.showToast(context, ResultData.getErrorsListMinimalString(resultData), true);
        sendPluginCommandErrorNotification(context, logTag, executionCommand, ResultData.getErrorsListMinimalString(resultData));

    }

    /** Set variables which will be used by {@link ResultSender#sendCommandResultData(Context, String, String, ResultConfig, ResultData, boolean)}
     * to send back the result via {@link ResultConfig#resultPendingIntent}. */
    public static void setPluginResultPendingIntentVariables(ExecutionCommand executionCommand) {
        ResultConfig resultConfig = executionCommand.resultConfig;

        resultConfig.resultBundleKey = TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE;
        resultConfig.resultStdoutKey = TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT;
        resultConfig.resultStdoutOriginalLengthKey = TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDOUT_ORIGINAL_LENGTH;
        resultConfig.resultStderrKey = TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDERR;
        resultConfig.resultStderrOriginalLengthKey = TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_STDERR_ORIGINAL_LENGTH;
        resultConfig.resultExitCodeKey = TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_EXIT_CODE;
        resultConfig.resultErrCodeKey = TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERR;
        resultConfig.resultErrmsgKey = TERMUX_SERVICE.EXTRA_PLUGIN_RESULT_BUNDLE_ERRMSG;
    }

    /** Set variables which will be used by {@link ResultSender#sendCommandResultData(Context, String, String, ResultConfig, ResultData, boolean)}
     * to send back the result by writing it to files in {@link ResultConfig#resultDirectoryPath}. */
    public static void setPluginResultDirectoryVariables(ExecutionCommand executionCommand) {
        ResultConfig resultConfig = executionCommand.resultConfig;

        resultConfig.resultDirectoryPath = TermuxFileUtils.getCanonicalPath(resultConfig.resultDirectoryPath, null, true);
        resultConfig.resultDirectoryAllowedParentPath = TermuxFileUtils.getMatchedAllowedTermuxWorkingDirectoryParentPathForPath(resultConfig.resultDirectoryPath);

        // Set default resultFileBasename if resultSingleFile is true to `<executable_basename>-<timestamp>.log`
        if (resultConfig.resultSingleFile && resultConfig.resultFileBasename == null)
            resultConfig.resultFileBasename = ShellUtils.getExecutableBasename(executionCommand.executable) + "-" + AndroidUtils.getCurrentMilliSecondLocalTimeStamp() + ".log";
    }



    /**
     * Send an error notification for {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param context The {@link Context} for operations.
     * @param executionCommand The {@link ExecutionCommand} that failed.
     * @param notificationTextString The text of the notification.
     */
    public static void sendPluginCommandErrorNotification(Context context, String logTag, ExecutionCommand executionCommand, String notificationTextString) {
        // Send a notification to show the error which when clicked will open the ReportActivity
        // to show the details of the error
        String title = TermuxConstants.TERMUX_APP_NAME + " Plugin Execution Command Error";

        StringBuilder reportString = new StringBuilder();

        reportString.append(ExecutionCommand.getExecutionCommandMarkdownString(executionCommand));
        reportString.append("\n\n").append(TermuxUtils.getAppInfoMarkdownString(context, true));
        reportString.append("\n\n").append(AndroidUtils.getDeviceInfoMarkdownString(context));

        String userActionName = UserAction.PLUGIN_EXECUTION_COMMAND.getName();
        ReportActivity.NewInstanceResult result = ReportActivity.newInstance(context,
            new ReportInfo(userActionName, logTag, title, null,
                reportString.toString(), null,true,
                userActionName,
                Environment.getExternalStorageDirectory() + "/" +
                    FileUtils.sanitizeFileName(TermuxConstants.TERMUX_APP_NAME + "-" + userActionName + ".log", true, true)));
        if (result.contentIntent == null) return;

        // Must ensure result code for PendingIntents and id for notification are unique otherwise will override previous
        int nextNotificationId = TermuxNotificationUtils.getNextNotificationId(context);

        PendingIntent contentIntent = PendingIntent.getActivity(context, nextNotificationId, result.contentIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        PendingIntent deleteIntent = null;
        if (result.deleteIntent != null)
            deleteIntent = PendingIntent.getBroadcast(context, nextNotificationId, result.deleteIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        // Setup the notification channel if not already set up
        setupPluginCommandErrorsNotificationChannel(context);

        // Use markdown in notification
        CharSequence notificationTextCharSequence = MarkdownUtils.getSpannedMarkdownText(context, notificationTextString);
        //CharSequence notificationTextCharSequence = notificationTextString;

        // Build the notification
        Notification.Builder builder = getPluginCommandErrorsNotificationBuilder(context, title,
            notificationTextCharSequence, notificationTextCharSequence, contentIntent, deleteIntent, NotificationUtils.NOTIFICATION_MODE_VIBRATE);
        if (builder == null) return;

        // Send the notification
        NotificationManager notificationManager = NotificationUtils.getNotificationManager(context);
        if (notificationManager != null)
            notificationManager.notify(nextNotificationId, builder.build());
    }

    /**
     * Get {@link Notification.Builder} for {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param context The {@link Context} for operations.
     * @param title The title for the notification.
     * @param notificationText The second line text of the notification.
     * @param notificationBigText The full text of the notification that may optionally be styled.
     * @param contentIntent The {@link PendingIntent} which should be sent when notification is clicked.
     * @param deleteIntent The {@link PendingIntent} which should be sent when notification is deleted.
     * @param notificationMode The notification mode. It must be one of {@code NotificationUtils.NOTIFICATION_MODE_*}.
     * @return Returns the {@link Notification.Builder}.
     */
    @Nullable
    public static Notification.Builder getPluginCommandErrorsNotificationBuilder(
        final Context context, final CharSequence title, final CharSequence notificationText,
        final CharSequence notificationBigText, final PendingIntent contentIntent, final PendingIntent deleteIntent, final int notificationMode) {

        Notification.Builder builder =  NotificationUtils.geNotificationBuilder(context,
            TermuxConstants.TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID, Notification.PRIORITY_HIGH,
            title, notificationText, notificationBigText, contentIntent, deleteIntent, notificationMode);

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
     * @return Returns the {@code error} if policy is violated, otherwise {@code null}.
     */
    public static String checkIfAllowExternalAppsPolicyIsViolated(final Context context, String apiName) {
        String errmsg = null;
        if (!SharedProperties.isPropertyValueTrue(context, TermuxPropertyConstants.getTermuxPropertiesFile(),
            TermuxConstants.PROP_ALLOW_EXTERNAL_APPS, true)) {
            errmsg = context.getString(R.string.error_allow_external_apps_ungranted, apiName,
                TermuxFileUtils.getUnExpandedTermuxPath(TermuxConstants.TERMUX_PROPERTIES_PRIMARY_FILE_PATH));
        }

        return errmsg;
    }

}
