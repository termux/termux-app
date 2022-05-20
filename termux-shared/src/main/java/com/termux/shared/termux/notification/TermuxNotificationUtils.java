package com.termux.shared.termux.notification;

import android.content.Context;

import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.termux.settings.preferences.TermuxPreferenceConstants;
import com.termux.shared.termux.TermuxConstants;

public class TermuxNotificationUtils {
    /**
     * Try to get the next unique notification id that isn't already being used by the app.
     *
     * Termux app and its plugin must use unique notification ids from the same pool due to usage of android:sharedUserId.
     * https://commonsware.com/blog/2017/06/07/jobscheduler-job-ids-libraries.html
     *
     * @param context The {@link Context} for operations.
     * @return Returns the notification id that should be safe to use.
     */
    public synchronized static int getNextNotificationId(final Context context) {
        if (context == null) return TermuxPreferenceConstants.TERMUX_APP.DEFAULT_VALUE_KEY_LAST_NOTIFICATION_ID;

        TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(context);
        if (preferences == null) return TermuxPreferenceConstants.TERMUX_APP.DEFAULT_VALUE_KEY_LAST_NOTIFICATION_ID;

        int lastNotificationId = preferences.getLastNotificationId();

        int nextNotificationId = lastNotificationId + 1;

        boolean isAppNotificationIdSame = nextNotificationId == TermuxConstants.TERMUX_APP_NOTIFICATION_ID;
        boolean isCommandNotificationIdSame = nextNotificationId == TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_ID;

        while( isAppNotificationIdSame || isCommandNotificationIdSame) {
            nextNotificationId++;

            isAppNotificationIdSame = nextNotificationId == TermuxConstants.TERMUX_APP_NOTIFICATION_ID;
            isCommandNotificationIdSame = nextNotificationId == TermuxConstants.TERMUX_RUN_COMMAND_NOTIFICATION_ID;
        }

        boolean isNotificationIdMaxValue = nextNotificationId == Integer.MAX_VALUE;
        boolean isNotificationIdUnderZero = nextNotificationId < 0;

        if (isNotificationIdMaxValue || isNotificationIdUnderZero)
            nextNotificationId = TermuxPreferenceConstants.TERMUX_APP.DEFAULT_VALUE_KEY_LAST_NOTIFICATION_ID;

        preferences.setLastNotificationId(nextNotificationId);
        return nextNotificationId;
    }
}
