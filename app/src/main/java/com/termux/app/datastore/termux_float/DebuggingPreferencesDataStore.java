package com.termux.app.datastore.termux_float;

import android.content.Context;

import androidx.annotation.Nullable;
import androidx.preference.PreferenceDataStore;

import com.termux.shared.termux.settings.preferences.TermuxFloatAppSharedPreferences;

public class DebuggingPreferencesDataStore extends PreferenceDataStore {

    private final Context mContext;
    private final TermuxFloatAppSharedPreferences mPreferences;

    private static DebuggingPreferencesDataStore mInstance;

    private DebuggingPreferencesDataStore(Context context) {
        mContext = context;
        mPreferences = TermuxFloatAppSharedPreferences.build(context, true);
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
        return (key == "log_level")? String.valueOf(mPreferences.getLogLevel(true)) : null;

    }

    @Override
    public void putString(String key, @Nullable String value) {
        if (isDisability(key)) return;
        if (key == "log_level") {
            _put(value);

        }
    }

    private void _put(@Nullable String value) {
        if (value != null) {
            mPreferences.setLogLevel(mContext, Integer.parseInt(value), true);
        }
    }

    private boolean isDisability(String key) {
        return  (mPreferences == null || key == null) ? true : false;

    }

    @Override
    public void putBoolean(String key, boolean value) {
        if (isDisability(key)) return;

        if(key =="terminal_view_key_logging_enabled")
            mPreferences.setTerminalViewKeyLoggingEnabled(value, true);

    }

    @Override
    public boolean getBoolean(String key, boolean defValue) {
        if (mPreferences == null) return false;
        return  (key =="terminal_view_key_logging_enabled") ? mPreferences.isTerminalViewKeyLoggingEnabled(true):false;

    }

}
