package com.termux.shared.theme;

import android.content.Context;
import android.content.res.Configuration;

import com.termux.shared.models.theme.NightMode;

public class ThemeUtils {

    /**
     * Will return true if system has enabled night mode.
     * https://developer.android.com/guide/topics/resources/providing-resources#NightQualifier
     */
    public static boolean isNightModeEnabled(Context context) {
        if (context == null) return false;
        return (context.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;

    }

    /** Will return true if mode is set to {@link NightMode#TRUE}, otherwise will return true if
     * mode is set to {@link NightMode#SYSTEM} and night mode is enabled by system. */
    public static boolean shouldEnableDarkTheme(Context context, String name) {
        if (NightMode.TRUE.getName().equals(name))
            return true;
        else if (NightMode.FALSE.getName().equals(name))
            return false;
        else if (NightMode.SYSTEM.getName().equals(name)) {
            return isNightModeEnabled(context);
        } else {
            return false;
        }
    }
}
