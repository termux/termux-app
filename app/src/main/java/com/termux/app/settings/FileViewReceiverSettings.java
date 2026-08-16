package com.termux.app.settings;

import android.content.ComponentName;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;

import androidx.annotation.NonNull;
import androidx.preference.PreferenceManager;

public final class FileViewReceiverSettings {

    public static final String PREFERENCE_KEY = "file_view_receiver_enabled";

    private static final String FILE_VIEW_RECEIVER_ACTIVITY = ".app.api.file.FileViewReceiverActivity";
    private static final boolean DEFAULT_ENABLED = true;

    private FileViewReceiverSettings() {}

    public static boolean isEnabled(@NonNull Context context) {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(context);
        return preferences.getBoolean(PREFERENCE_KEY, DEFAULT_ENABLED);
    }

    public static void setEnabled(@NonNull Context context, boolean enabled) {
        PreferenceManager.getDefaultSharedPreferences(context)
            .edit()
            .putBoolean(PREFERENCE_KEY, enabled)
            .apply();

        applyComponentState(context, enabled);
    }

    public static void applySavedState(@NonNull Context context) {
        applyComponentState(context, isEnabled(context));
    }

    private static void applyComponentState(@NonNull Context context, boolean enabled) {
        ComponentName componentName = new ComponentName(
            context.getPackageName(), context.getPackageName() + FILE_VIEW_RECEIVER_ACTIVITY);

        context.getPackageManager().setComponentEnabledSetting(
            componentName,
            enabled
                ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                : PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
            PackageManager.DONT_KILL_APP);
    }
}
