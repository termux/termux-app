package com.termux.shared.termux.plugins;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.os.Build;
import android.os.Environment;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.R;
import com.termux.shared.activities.ReportActivity;
import com.termux.shared.file.FileUtils;
import com.termux.shared.termux.file.TermuxFileUtils;
import com.termux.shared.shell.command.result.ResultConfig;
import com.termux.shared.shell.command.result.ResultData;
import com.termux.shared.errors.Errno;
import com.termux.shared.errors.Error;
import com.termux.shared.notification.NotificationUtils;
import com.termux.shared.termux.models.UserAction;
import com.termux.shared.termux.notification.TermuxNotificationUtils;
import com.termux.shared.termux.settings.preferences.TermuxPreferenceConstants;
import com.termux.shared.shell.command.result.ResultSender;
import com.termux.shared.shell.ShellUtils;
import com.termux.shared.android.AndroidUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.termux.settings.preferences.TermuxPreferenceConstants.TERMUX_APP;
import com.termux.shared.models.ReportInfo;
import com.termux.shared.termux.settings.properties.TermuxAppSharedProperties;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.data.DataUtils;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.termux.TermuxUtils;

public class TermuxPluginUtils {

    private static final String LOG_TAG = "TermuxPluginUtils";

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
                sendPluginCommandErrorNotification(context, logTag, null,
                    ResultData.getErrorsListMinimalString(resultData),
                    ExecutionCommand.getExecutionCommandMarkdownString(executionCommand),
                    false, true, TermuxUtils.AppInfoMode.TERMUX_AND_CALLING_PACKAGE,true,
                    executionCommand.resultConfig.resultPendingIntent != null ? executionCommand.resultConfig.resultPendingIntent.getCreatorPackage(): null);
            }

        }

        if (!executionCommand.isStateFailed() && error == null)
            executionCommand.setState(ExecutionCommand.ExecutionState.SUCCESS);
    }

    /**
     * Set {@link ExecutionCommand} state to {@link Errno#ERRNO_FAILED} with {@code errmsg} and
     * process error with {@link #processPluginExecutionCommandError(Context, String, ExecutionCommand, boolean)}.
     *
     *
     * @param context The {@link Context} for operations.
     * @param logTag The log tag to use for logging.
     * @param executionCommand The {@link ExecutionCommand} that failed.
     * @param forceNotification If set to {@code true}, then a flash and notification will be shown
     *                          regardless of if pending intent is {@code null} or
     *                          {@link TERMUX_APP#KEY_PLUGIN_ERROR_NOTIFICATIONS_ENABLED}
     *                          is {@code false}.
     * @param errmsg The error message to set.
     */
    public static void setAndProcessPluginExecutionCommandError(final Context context, String logTag,
                                                          final ExecutionCommand executionCommand,
                                                          boolean forceNotification,
                                                          @NonNull String errmsg) {
        executionCommand.setStateFailed(Errno.ERRNO_FAILED.getCode(), errmsg);
        processPluginExecutionCommandError(context, logTag, executionCommand, forceNotification);
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
    public static void processPluginExecutionCommandError(final Context context, String logTag,
                                                          final ExecutionCommand executionCommand,
                                                          boolean forceNotification) {
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
        Logger.logError(logTag, "Processing plugin execution error for:\n" + executionCommand.getCommandIdAndLabelLogString());
        Logger.logError(logTag, "Set log level to debug or higher to see error in logs");
        Logger.logErrorPrivateExtended(logTag, ExecutionCommand.getExecutionOutputLogString(executionCommand, true,
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
                Logger.logErrorPrivateExtended(logTag, ExecutionCommand.getExecutionOutputLogString(executionCommand, true, true, isExecutionCommandLoggingEnabled));
                forceNotification = true;
            }

            // No need to show notifications if a pending intent was sent, let the caller handle the result himself
            if (!forceNotification) return;
        }

        // Flash and send notification for the error
        sendPluginCommandErrorNotification(context, logTag, null,
            ResultData.getErrorsListMinimalString(resultData),
            ExecutionCommand.getExecutionCommandMarkdownString(executionCommand),
            forceNotification, true, TermuxUtils.AppInfoMode.TERMUX_AND_CALLING_PACKAGE, true,
            executionCommand.resultConfig.resultPendingIntent != null ? executionCommand.resultConfig.resultPendingIntent.getCreatorPackage(): null);
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
     * Send a plugin error report notification for {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param currentPackageContext The {@link Context} of current package.
     * @param logTag The log tag to use for logging.
     * @param title The title for the error report and notification.
     * @param message The message for the error report.
     * @param throwable The {@link Throwable} for the error report.
     */
    public static void sendPluginCommandErrorNotification(final Context currentPackageContext, String logTag,
                                                         CharSequence title, String message, Throwable throwable) {
        sendPluginCommandErrorNotification(currentPackageContext, logTag,
            title, message,
            MarkdownUtils.getMarkdownCodeForString(Logger.getMessageAndStackTraceString(message, throwable), true),
            false, false, true);
    }

    /**
     * Send a plugin error report notification for {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param currentPackageContext The {@link Context} of current package.
     * @param logTag The log tag to use for logging.
     * @param title The title for the error report and notification.
     * @param notificationTextString The text of the notification.
     * @param message The message for the error report.
     */
    public static void sendPluginCommandErrorNotification(final Context currentPackageContext, String logTag,
                                                         CharSequence title, String notificationTextString,
                                                         String message) {
        sendPluginCommandErrorNotification(currentPackageContext, logTag,
            title, notificationTextString, message,
            false, false, true);
    }

    /**
     * Send a plugin error report notification for {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param currentPackageContext The {@link Context} of current package.
     * @param logTag The log tag to use for logging.
     * @param title The title for the error report and notification.
     * @param notificationTextString The text of the notification.
     * @param message The message for the error report.
     * @param forceNotification If set to {@code true}, then a notification will be shown
     *                          regardless of if pending intent is {@code null} or
     *                          {@link TermuxPreferenceConstants.TERMUX_APP#KEY_PLUGIN_ERROR_NOTIFICATIONS_ENABLED}
     *                          is {@code false}.
     * @param showToast If set to {@code true}, then a toast will be shown for {@code notificationTextString}.
     * @param addDeviceInfo If set to {@code true}, then device info should be appended to the message.
     */
    public static void sendPluginCommandErrorNotification(final Context currentPackageContext, String logTag,
                                                         CharSequence title, String notificationTextString,
                                                         String message, boolean forceNotification,
                                                         boolean showToast,
                                                         boolean addDeviceInfo) {
        sendPluginCommandErrorNotification(currentPackageContext, logTag,
            title, notificationTextString, "## " + title + "\n\n" + message + "\n\n",
            forceNotification, showToast, TermuxUtils.AppInfoMode.TERMUX_AND_PLUGIN_PACKAGE, addDeviceInfo, null);
    }

    /**
     * Send a plugin error notification for {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param currentPackageContext The {@link Context} of current package.
     * @param logTag The log tag to use for logging.
     * @param title The title for the error report and notification.
     * @param notificationTextString The text of the notification.
     * @param message The message for the error report.
     * @param forceNotification If set to {@code true}, then a notification will be shown
     *                          regardless of if pending intent is {@code null} or
     *                          {@link TermuxPreferenceConstants.TERMUX_APP#KEY_PLUGIN_ERROR_NOTIFICATIONS_ENABLED}
     *                          is {@code false}.
     * @param showToast If set to {@code true}, then a toast will be shown for {@code notificationTextString}.
     * @param appInfoMode The {@link TermuxUtils.AppInfoMode} to use to add app info to the message.
     *                    Set to {@code null} if app info should not be appended to the message.
     * @param addDeviceInfo If set to {@code true}, then device info should be appended to the message.
     * @param callingPackageName The optional package name of the app for which the plugin command
     *                           was run.
     */
    public static void sendPluginCommandErrorNotification(Context currentPackageContext, String logTag,
                                                          CharSequence title,
                                                          String notificationTextString,
                                                          String message, boolean forceNotification,
                                                          boolean showToast,
                                                          TermuxUtils.AppInfoMode appInfoMode,
                                                          boolean addDeviceInfo,
                                                          String callingPackageName) {
        // Note: Do not change currentPackageContext or termuxPackageContext passed to functions or things will break

        if (currentPackageContext == null) return;
        String currentPackageName = currentPackageContext.getPackageName();

        final Context termuxPackageContext = TermuxUtils.getTermuxPackageContext(currentPackageContext);
        if (termuxPackageContext == null) {
            Logger.logWarn(LOG_TAG, "Ignoring call to sendPluginCommandErrorNotification() since failed to get \"" + TermuxConstants.TERMUX_PACKAGE_NAME + "\" package context from \"" + currentPackageName + "\" context");
            return;
        }

        TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(termuxPackageContext);
        if (preferences == null) return;

        // If user has disabled notifications for plugin commands, then just return
        if (!preferences.arePluginErrorNotificationsEnabled(true) && !forceNotification)
            return;

        logTag = DataUtils.getDefaultIfNull(logTag, LOG_TAG);

        if (showToast)
            Logger.showToast(currentPackageContext, notificationTextString, true);

        // Send a notification to show the error which when clicked will open the ReportActivity
        // to show the details of the error
        if (title == null || title.toString().isEmpty())
            title = TermuxConstants.TERMUX_APP_NAME + " Plugin Execution Command Error";

        Logger.logDebug(logTag, "Sending \"" + title + "\" notification.");

        StringBuilder reportString = new StringBuilder(message);

        if (appInfoMode != null)
            reportString.append("\n\n").append(TermuxUtils.getAppInfoMarkdownString(currentPackageContext, appInfoMode,
                callingPackageName != null ? callingPackageName : currentPackageName));

        if (addDeviceInfo)
            reportString.append("\n\n").append(AndroidUtils.getDeviceInfoMarkdownString(currentPackageContext, true));

        String userActionName = UserAction.PLUGIN_EXECUTION_COMMAND.getName();

        ReportInfo reportInfo = new ReportInfo(userActionName, logTag, title.toString());
        reportInfo.setReportString(reportString.toString());
        reportInfo.setReportStringSuffix("\n\n" + TermuxUtils.getReportIssueMarkdownString(currentPackageContext));
        reportInfo.setAddReportInfoHeaderToMarkdown(true);
        reportInfo.setReportSaveFileLabelAndPath(userActionName,
            Environment.getExternalStorageDirectory() + "/" +
                FileUtils.sanitizeFileName(TermuxConstants.TERMUX_APP_NAME + "-" + userActionName + ".log", true, true));

        ReportActivity.NewInstanceResult result = ReportActivity.newInstance(termuxPackageContext, reportInfo);
        if (result.contentIntent == null) return;

        // Must ensure result code for PendingIntents and id for notification are unique otherwise will override previous
        int nextNotificationId = TermuxNotificationUtils.getNextNotificationId(termuxPackageContext);

        PendingIntent contentIntent = PendingIntent.getActivity(termuxPackageContext, nextNotificationId, result.contentIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        PendingIntent deleteIntent = null;
        if (result.deleteIntent != null)
            deleteIntent = PendingIntent.getBroadcast(termuxPackageContext, nextNotificationId, result.deleteIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        // Setup the notification channel if not already set up
        setupPluginCommandErrorsNotificationChannel(termuxPackageContext);

        // Use markdown in notification
        CharSequence notificationTextCharSequence = MarkdownUtils.getSpannedMarkdownText(termuxPackageContext, notificationTextString);
        //CharSequence notificationTextCharSequence = notificationTextString;

        // Build the notification
        Notification.Builder builder = getPluginCommandErrorsNotificationBuilder(currentPackageContext, termuxPackageContext,
            title, notificationTextCharSequence, notificationTextCharSequence, contentIntent, deleteIntent,
            NotificationUtils.NOTIFICATION_MODE_VIBRATE);
        if (builder == null) return;

        // Send the notification
        NotificationManager notificationManager = NotificationUtils.getNotificationManager(termuxPackageContext);
        if (notificationManager != null)
            notificationManager.notify(nextNotificationId, builder.build());
    }

    /**
     * Get {@link Notification.Builder} for {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param currentPackageContext The {@link Context} of current package.
     * @param termuxPackageContext The {@link Context} of termux package.
     * @param title The title for the notification.
     * @param notificationText The second line text of the notification.
     * @param notificationBigText The full text of the notification that may optionally be styled.
     * @param contentIntent The {@link PendingIntent} which should be sent when notification is clicked.
     * @param deleteIntent The {@link PendingIntent} which should be sent when notification is deleted.
     * @param notificationMode The notification mode. It must be one of {@code NotificationUtils.NOTIFICATION_MODE_*}.
     * @return Returns the {@link Notification.Builder}.
     */
    @Nullable
    public static Notification.Builder getPluginCommandErrorsNotificationBuilder(final Context currentPackageContext,
                                                                                 final Context termuxPackageContext,
                                                                                 final CharSequence title,
                                                                                 final CharSequence notificationText,
                                                                                 final CharSequence notificationBigText,
                                                                                 final PendingIntent contentIntent,
                                                                                 final PendingIntent deleteIntent,
                                                                                 final int notificationMode) {
        return TermuxNotificationUtils.getTermuxOrPluginAppNotificationBuilder(
            currentPackageContext, termuxPackageContext,
            TermuxConstants.TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID, Notification.PRIORITY_HIGH,
            title, notificationText, notificationBigText, contentIntent, deleteIntent, notificationMode);
    }

    /**
     * Setup the notification channel for {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_ID} and
     * {@link TermuxConstants#TERMUX_PLUGIN_COMMAND_ERRORS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param context The {@link Context} for operations.
     */
    public static void setupPluginCommandErrorsNotificationChannel(final Context context) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;
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

        TermuxAppSharedProperties mProperties = TermuxAppSharedProperties.getProperties();
        if (mProperties == null || !mProperties.shouldAllowExternalApps()) {
            errmsg = context.getString(R.string.error_allow_external_apps_ungranted, apiName,
                TermuxFileUtils.getUnExpandedTermuxPath(TermuxConstants.TERMUX_PROPERTIES_PRIMARY_FILE_PATH));
        }

        return errmsg;
    }

}
