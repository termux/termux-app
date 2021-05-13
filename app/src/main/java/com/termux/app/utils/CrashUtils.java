package com.termux.app.utils;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;

import androidx.annotation.Nullable;

import com.termux.R;
import com.termux.app.activities.ReportActivity;
import com.termux.shared.notification.NotificationUtils;
import com.termux.shared.file.FileUtils;
import com.termux.app.models.ReportInfo;
import com.termux.app.models.UserAction;
import com.termux.shared.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.settings.preferences.TermuxPreferenceConstants;
import com.termux.shared.data.DataUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxUtils;

import com.termux.shared.termux.TermuxConstants;

import java.nio.charset.Charset;

public class CrashUtils {

    private static final String LOG_TAG = "CrashUtils";

    /**
     * Notify the user of a previous app crash by reading the crash info from the crash log file at
     * {@link TermuxConstants#TERMUX_CRASH_LOG_FILE_PATH}. The crash log file would have been
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
    public static void notifyCrash(final Context context, final String logTagParam) {
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

                String errmsg;
                StringBuilder reportStringBuilder = new StringBuilder();

                // Read report string from crash log file
                errmsg = FileUtils.readStringFromFile(context, "crash log", TermuxConstants.TERMUX_CRASH_LOG_FILE_PATH, Charset.defaultCharset(), reportStringBuilder, false);
                if (errmsg != null) {
                    Logger.logError(logTag, errmsg);
                    return;
                }

                // Move crash log file to backup location if it exists
                FileUtils.moveRegularFile(context, "crash log", TermuxConstants.TERMUX_CRASH_LOG_FILE_PATH, TermuxConstants.TERMUX_CRASH_LOG_BACKUP_FILE_PATH, true);
                if (errmsg != null) {
                    Logger.logError(logTag, errmsg);
                }

                String reportString = reportStringBuilder.toString();

                if (reportString == null || reportString.isEmpty())
                    return;

                // Send a notification to show the crash log which when clicked will open the {@link ReportActivity}
                // to show the details of the crash
                String title = TermuxConstants.TERMUX_APP_NAME + " Crash Report";

                Logger.logDebug(logTag, "The crash log file at \"" + TermuxConstants.TERMUX_CRASH_LOG_FILE_PATH +  "\" found. Sending \"" + title + "\" notification.");

                Intent notificationIntent = ReportActivity.newInstance(context, new ReportInfo(UserAction.CRASH_REPORT, logTag, title, null, reportString, "\n\n" + TermuxUtils.getReportIssueMarkdownString(context), true));
                PendingIntent pendingIntent = PendingIntent.getActivity(context, 0, notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);

                // Setup the notification channel if not already set up
                setupCrashReportsNotificationChannel(context);

                // Build the notification
                Notification.Builder builder = getCrashReportsNotificationBuilder(context, title, null, null, pendingIntent, NotificationUtils.NOTIFICATION_MODE_VIBRATE);
                if (builder == null)  return;

                // Send the notification
                int nextNotificationId = NotificationUtils.getNextNotificationId(context);
                NotificationManager notificationManager = NotificationUtils.getNotificationManager(context);
                if (notificationManager != null)
                    notificationManager.notify(nextNotificationId, builder.build());
            }
        }.start();
    }

    /**
     * Get {@link Notification.Builder} for {@link TermuxConstants#TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_ID}
     * and {@link TermuxConstants#TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_NAME}.
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
    public static Notification.Builder getCrashReportsNotificationBuilder(final Context context, final CharSequence title, final CharSequence notificationText, final CharSequence notificationBigText, final PendingIntent pendingIntent, final int notificationMode) {

        Notification.Builder builder =  NotificationUtils.geNotificationBuilder(context,
            TermuxConstants.TERMUX_CRASH_REPORTS_NOTIFICATION_CHANNEL_ID, Notification.PRIORITY_HIGH,
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
