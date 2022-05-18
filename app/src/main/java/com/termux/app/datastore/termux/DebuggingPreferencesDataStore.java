package com.termux.app.datastore.termux;

import android.content.Context;

import androidx.annotation.Nullable;
import androidx.preference.PreferenceDataStore;

import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;

public class DebuggingPreferencesDataStore extends PreferenceDataStore {

    private final Context mContext;
    private final TermuxAppSharedPreferences mPreferences;

    private static DebuggingPreferencesDataStore mInstance;

    private DebuggingPreferencesDataStore(Context context) {
        mContext = context;
        mPreferences = TermuxAppSharedPreferences.build(context, true);
    }

    public static synchronized DebuggingPreferencesDataStore getInstance(Context context) {
        if (mInstance == null) {
            mInstance = new DebuggingPreferencesDataStore(context);
        }
        return mInstance;
    }



    @Override
    @Nullable
    public String getString(String key, @Nullable String defValue) {
        if (mPreferences == null) return null;
        if (key == null) return null;

        switch (key) {
            case "log_level":
                return String.valueOf(mPreferences.getLogLevel());
            default:
                return null;
        }
    }

    @Override
    public void putString(String key, @Nullable String value) {
        if (mPreferences == null) return;
        if (key == null) return;

        switch (key) {
            case "log_level":
                if (value != null) {
                    mPreferences.setLogLevel(mContext, Integer.parseInt(value));
                }
                break;
            default:
                break;
        }
    }



    @Override
    public void putBoolean(String key, boolean value) {
        if (mPreferences == null) return;
        if (key == null) return;

        switch (key) {
            case "terminal_view_key_logging_enabled":
                mPreferences.setTerminalViewKeyLoggingEnabled(value);
                break;
            case "plugin_error_notifications_enabled":
                mPreferences.setPluginErrorNotificationsEnabled(value);
                break;
            case "crash_report_notifications_enabled":
                mPreferences.setCrashReportNotificationsEnabled(value);
                break;
            default:
                break;
        }
    }

    @Override
    public boolean getBoolean(String key, boolean defValue) {
        if (mPreferences == null) return false;
        switch (key) {
            case "terminal_view_key_logging_enabled":
                return mPreferences.isTerminalViewKeyLoggingEnabled();
            case "plugin_error_notifications_enabled":
                return mPreferences.arePluginErrorNotificationsEnabled(false);
            case "crash_report_notifications_enabled":
                return mPreferences.areCrashReportNotificationsEnabled(false);
            default:
                return false;
        }
    }

}
