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
        if (isDisability(key)) return null;
        return (key == "log_level") ? String.valueOf(mPreferences.getLogLevel()): null;
    }

    private boolean isDisability(String key) {
        return (mPreferences == null || key == null) ? true : false;
    }

    @Override
    public void putString(String key, @Nullable String value) {
        if (isDisability(key)) return;

        if ( key == "log_level"  )
            _put(value);
    }

    private void _put(@Nullable String value) {
        if (value != null) mPreferences.setLogLevel(mContext, Integer.parseInt(value));
    }


    @Override
    public void putBoolean(String key, boolean value) {
        if (isDisability(key)) return;

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

    private boolean isDisability(String key) {
        return mPreferences == null || key == null;
    }

    @Override
    public boolean getBoolean(String key, boolean defValue) {
        if (isDisability(key)) return false;
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
