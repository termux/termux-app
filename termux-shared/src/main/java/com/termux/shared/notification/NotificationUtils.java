package com.termux.shared.notification;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.os.Build;

import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;

public class NotificationUtils {

    /** Do not show notification */
    public static final int NOTIFICATION_MODE_NONE = 0;
    /** Show notification without sound, vibration or lights */
    public static final int NOTIFICATION_MODE_SILENT = 1;
    /** Show notification with sound */
    public static final int NOTIFICATION_MODE_SOUND = 2;
    /** Show notification with vibration */
    public static final int NOTIFICATION_MODE_VIBRATE = 3;
    /** Show notification with lights */
    public static final int NOTIFICATION_MODE_LIGHTS = 4;
    /** Show notification with sound and vibration */
    public static final int NOTIFICATION_MODE_SOUND_AND_VIBRATE = 5;
    /** Show notification with sound and lights */
    public static final int NOTIFICATION_MODE_SOUND_AND_LIGHTS = 6;
    /** Show notification with vibration and lights */
    public static final int NOTIFICATION_MODE_VIBRATE_AND_LIGHTS = 7;
    /** Show notification with sound, vibration and lights */
    public static final int NOTIFICATION_MODE_ALL = 8;

    private static final String LOG_TAG = "NotificationUtils";

    /**
     * Get the {@link NotificationManager}.
     *
     * @param context The {@link Context} for operations.
     * @return Returns the {@link NotificationManager}.
     */
    @Nullable
    public static NotificationManager getNotificationManager(final Context context) {
        if (context == null) return null;
        return (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
    }

    /**
     * Get {@link Notification.Builder}.
     *
     * @param context The {@link Context} for operations.
     * @param title The title for the notification.
     * @param channelId The channel id for the notification.
     * @param priority The priority for the notification.
     * @param notificationText The second line text of the notification.
     * @param notificationBigText The full text of the notification that may optionally be styled.
     * @param contentIntent The {@link PendingIntent} which should be sent when notification is clicked.
     * @param deleteIntent The {@link PendingIntent} which should be sent when notification is deleted.
     * @param notificationMode The notification mode. It must be one of {@code NotificationUtils.NOTIFICATION_MODE_*}.
     *                         The builder returned will be {@code null} if {@link #NOTIFICATION_MODE_NONE}
     *                         is passed. That case should ideally be handled before calling this function.
     * @return Returns the {@link Notification.Builder}.
     */
    @Nullable
    public static Notification.Builder geNotificationBuilder(
        final Context context, final String channelId, final int priority, final CharSequence title,
        final CharSequence notificationText, final CharSequence notificationBigText,
        final PendingIntent contentIntent, final PendingIntent deleteIntent, final int notificationMode) {
        if (context == null) return null;
        Notification.Builder builder = new Notification.Builder(context);
        builder.setContentTitle(title);
        builder.setContentText(notificationText);
        builder.setStyle(new Notification.BigTextStyle().bigText(notificationBigText));
        builder.setContentIntent(contentIntent);
        builder.setDeleteIntent(deleteIntent);

        builder.setPriority(priority);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
            builder.setChannelId(channelId);

        builder = setNotificationDefaults(builder, notificationMode);

        return builder;
    }

    /**
     * Setup the notification channel if Android version is greater than or equal to
     * {@link Build.VERSION_CODES#O}.
     *
     * @param context The {@link Context} for operations.
     * @param channelId The id of the channel. Must be unique per package.
     * @param channelName The user visible name of the channel.
     * @param importance The importance of the channel. This controls how interruptive notifications
     *                   posted to this channel are.
     */
    public static void setupNotificationChannel(final Context context, final String channelId, final CharSequence channelName, final int importance) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return;

        NotificationChannel channel = new NotificationChannel(channelId, channelName, importance);

        NotificationManager notificationManager = getNotificationManager(context);
        if (notificationManager != null)
            notificationManager.createNotificationChannel(channel);
    }

    public static Notification.Builder setNotificationDefaults(Notification.Builder builder, final int notificationMode) {

        // TODO: setDefaults() is deprecated and should also implement setting notification mode via notification channel
        switch (notificationMode) {
            case NOTIFICATION_MODE_NONE:
                Logger.logWarn(LOG_TAG, "The NOTIFICATION_MODE_NONE passed to setNotificationDefaults(), force setting builder to null.");
                return null; // return null since notification is not supposed to be shown
            case NOTIFICATION_MODE_SILENT:
                break;
            case NOTIFICATION_MODE_SOUND:
                builder.setDefaults(Notification.DEFAULT_SOUND);
                break;
            case NOTIFICATION_MODE_VIBRATE:
                builder.setDefaults(Notification.DEFAULT_VIBRATE);
                break;
            case NOTIFICATION_MODE_LIGHTS:
                builder.setDefaults(Notification.DEFAULT_LIGHTS);
                break;
            case NOTIFICATION_MODE_SOUND_AND_VIBRATE:
                builder.setDefaults(Notification.DEFAULT_SOUND | Notification.DEFAULT_VIBRATE);
                break;
            case NOTIFICATION_MODE_SOUND_AND_LIGHTS:
                builder.setDefaults(Notification.DEFAULT_SOUND | Notification.DEFAULT_LIGHTS);
                break;
            case NOTIFICATION_MODE_VIBRATE_AND_LIGHTS:
                builder.setDefaults(Notification.DEFAULT_VIBRATE | Notification.DEFAULT_LIGHTS);
                break;
            case NOTIFICATION_MODE_ALL:
                builder.setDefaults(Notification.DEFAULT_ALL);
                break;
            default:
                Logger.logError(LOG_TAG, "Invalid notificationMode: \"" + notificationMode + "\" passed to setNotificationDefaults()");
                break;
        }

        return builder;
    }

}
