package com.termux.app.utils;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.os.Environment;

import androidx.annotation.Nullable;

import com.termux.R;
import com.termux.shared.activities.ReportActivity;
import com.termux.shared.models.errors.Error;
import com.termux.shared.notification.NotificationUtils;
import com.termux.shared.file.FileUtils;
import com.termux.shared.models.ReportInfo;
import com.termux.app.models.UserAction;
import com.termux.shared.notification.TermuxNotificationUtils;
import com.termux.shared.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.settings.preferences.TermuxPreferenceConstants;
import com.termux.shared.data.DataUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.AndroidUtils;
import com.termux.shared.termux.TermuxUtils;

import com.termux.shared.termux.TermuxConstants;

import java.nio.charset.Charset;

public class CrashUtils {

    private static final String LOG_TAG = "CrashUtils";

    /**
     * Notify the user of an app crash at last run by reading the crash info from the crash log file
     * at {@link TermuxConstants#TERMUX_CRASH_LOG_FILE_PATH}. The crash log file would have been
     * created by {@link com.termux.shared.crash.CrashHandler}.
     *
     * If the crash log file exists and is not empty and
     * {@link TermuxPreferenceConstants.TERMUX_APP#KEY_CRASH_REPORT_NOTIFICATIONS_ENABLED} is
     * enabled, then a notification will be shown for the crash on the
     * {@link TermuxConstants#TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_NAME} channel, otherwise nothing will be done.
     *
     * After reading from the crash log file, it will be moved to {@link TermuxConstants#TERMUX_CRASH_LOG_BACKUP_FILE_PATH}.
     *
     * @param context The {@link Context} for operations.
     * @param logTagParam The log tag to use for logging.
     */
    public static void notifyAppCrashOnLastRun(final Context context, final String logTagParam) {
        if (context == null) return;

        TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(context);
        if (preferences == null) return;

        // If user has disabled notifications for crashes
        if (!preferences.areCrashReportNotificationsEnabled())
            return;

        new Thread() {
            @Override
            public void run() {
                String logTag = DataUtils.getDefaultIfNull(logTagParam, LOG_TAG);

                if (!FileUtils.regularFileExists(TermuxConstants.TERMUX_CRASH_LOG_FILE_PATH, false))
                    return;

                Error error;
                StringBuilder reportStringBuilder = new StringBuilder();

                // Read report string from crash log file
                error = FileUtils.readStringFromFile("crash log", TermuxConstants.TERMUX_CRASH_LOG_FILE_PATH, Charset.defaultCharset(), reportStringBuilder, false);
                if (error != null) {
                    Logger.logErrorExtended(logTag, error.toString());
                    return;
                }

                // Move crash log file to backup location if it exists
                error = FileUtils.moveRegularFile("crash log", TermuxConstants.TERMUX_CRASH_LOG_FILE_PATH, TermuxConstants.TERMUX_CRASH_LOG_BACKUP_FILE_PATH, true);
                if (error != null) {
                    Logger.logErrorExtended(logTag, error.toString());
                }

                String reportString = reportStringBuilder.toString();

                if (reportString.isEmpty())
                    return;

                Logger.logDebug(logTag, "A crash log file found at \"" + TermuxConstants.TERMUX_CRASH_LOG_FILE_PATH +  "\".");

                sendCrashReportNotification(context, logTag, reportString, false, false);
            }
        }.start();
    }

    /**
     * Send a crash report notification for {@link TermuxConstants#TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param context The {@link Context} for operations.
     * @param logTag The log tag to use for logging.
     * @param message The message for the crash report.
     * @param forceNotification If set to {@code true}, then a notification will be shown
     *                          regardless of if pending intent is {@code null} or
     *                          {@link TermuxPreferenceConstants.TERMUX_APP#KEY_CRASH_REPORT_NOTIFICATIONS_ENABLED}
     *                          is {@code false}.
     * @param addAppAndDeviceInfo If set to {@code true}, then app and device info will be appended
     *                            to the message.
     */
    public static void sendCrashReportNotification(final Context context, String logTag, String message, boolean forceNotification, boolean addAppAndDeviceInfo) {
        if (context == null) return;

        TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(context);
        if (preferences == null) return;

        // If user has disabled notifications for crashes
        if (!preferences.areCrashReportNotificationsEnabled() && !forceNotification)
            return;

        logTag = DataUtils.getDefaultIfNull(logTag, LOG_TAG);

        // Send a notification to show the crash log which when clicked will open the {@link ReportActivity}
        // to show the details of the crash
        String title = TermuxConstants.TERMUX_APP_NAME + " Crash Report";

        Logger.logDebug(logTag, "Sending \"" + title + "\" notification.");

        StringBuilder reportString = new StringBuilder(message);

        if (addAppAndDeviceInfo) {
            reportString.append("\n\n").append(TermuxUtils.getAppInfoMarkdownString(context, true));
            reportString.append("\n\n").append(AndroidUtils.getDeviceInfoMarkdownString(context));
        }

        String userActionName = UserAction.CRASH_REPORT.getName();
        ReportActivity.NewInstanceResult result = ReportActivity.newInstance(context, new ReportInfo(userActionName,
            logTag, title, null, reportString.toString(),
            "\n\n" + TermuxUtils.getReportIssueMarkdownString(context), true,
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
        setupCrashReportsNotificationChannel(context);

        // Build the notification
        Notification.Builder builder = getCrashReportsNotificationBuilder(context, title, null,
            null, contentIntent, deleteIntent, NotificationUtils.NOTIFICATION_MODE_VIBRATE);
        if (builder == null) return;

        // Send the notification
        NotificationManager notificationManager = NotificationUtils.getNotificationManager(context);
        if (notificationManager != null)
            notificationManager.notify(nextNotificationId, builder.build());
    }

    /**
     * Get {@link Notification.Builder} for {@link TermuxConstants#TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_NAME}.
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
    public static Notification.Builder getCrashReportsNotificationBuilder(final Context context, final CharSequence title, final CharSequence notificationText, final CharSequence notificationBigText, final PendingIntent contentIntent, final PendingIntent deleteIntent, final int notificationMode) {

        Notification.Builder builder =  NotificationUtils.geNotificationBuilder(context,
            TermuxConstants.TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_ID, Notification.PRIORITY_HIGH,
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
     * Setup the notification channel for {@link TermuxConstants#TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_ID} and
     * {@link TermuxConstants#TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_NAME}.
     *
     * @param context The {@link Context} for operations.
     */
    public static void setupCrashReportsNotificationChannel(final Context context) {
        NotificationUtils.setupNotificationChannel(context, TermuxConstants.TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_ID,
            TermuxConstants.TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_NAME, NotificationManager.IMPORTANCE_HIGH);
    }

}
