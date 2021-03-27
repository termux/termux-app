package com.termux.app.fragments.settings;

import android.content.Context;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.preference.ListPreference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceDataStore;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;

import com.termux.R;
import com.termux.app.settings.preferences.TermuxAppSharedPreferences;
import com.termux.app.utils.Logger;

public class DebuggingPreferencesFragment extends PreferenceFragmentCompat {

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        PreferenceManager preferenceManager = getPreferenceManager();
        preferenceManager.setPreferenceDataStore(DebuggingPreferencesDataStore.getInstance(getContext()));

        setPreferencesFromResource(R.xml.debugging_preferences, rootKey);

        PreferenceCategory loggingCategory = findPreference("logging");

        if (loggingCategory != null) {
            final ListPreference logLevelListPreference = setLogLevelListPreferenceData(findPreference("log_level"), getActivity());
            loggingCategory.addPreference(logLevelListPreference);
        }
    }

    protected ListPreference setLogLevelListPreferenceData(ListPreference logLevelListPreference, Context context) {
        if(logLevelListPreference == null)
            logLevelListPreference = new ListPreference(context);

        CharSequence[] logLevels = Logger.getLogLevelsArray();
        CharSequence[] logLevelLabels = Logger.getLogLevelLabelsArray(context, logLevels, true);

        logLevelListPreference.setEntryValues(logLevels);
        logLevelListPreference.setEntries(logLevelLabels);

        logLevelListPreference.setValue(String.valueOf(Logger.getLogLevel()));
        logLevelListPreference.setDefaultValue(Logger.getLogLevel());

        return logLevelListPreference;
    }

}

class DebuggingPreferencesDataStore extends PreferenceDataStore {

    private final Context mContext;
    private final TermuxAppSharedPreferences mPreferences;

    private static DebuggingPreferencesDataStore mInstance;

    private DebuggingPreferencesDataStore(Context context) {
        mContext = context;
        mPreferences = new TermuxAppSharedPreferences(context);
    }

    public static synchronized DebuggingPreferencesDataStore getInstance(Context context) {
        if (mInstance == null) {
            mInstance = new DebuggingPreferencesDataStore(context.getApplicationContext());
        }
        return mInstance;
    }



    @Override
    @Nullable
    public String getString(String key, @Nullable String defValue) {
        if(key == null) return null;

        switch (key) {
            case "log_level":
                return String.valueOf(mPreferences.getLogLevel());
            default:
                return null;
        }
    }

    @Override
    public void putString(String key, @Nullable String value) {
        if(key == null) return;

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
        if(key == null) return;

        switch (key) {
            case "terminal_view_key_logging_enabled":
                    mPreferences.setTerminalViewKeyLoggingEnabled(value);
                break;
            case "plugin_error_notifications_enabled":
                mPreferences.setPluginErrorNotificationsEnabled(value);
                break;
            default:
                break;
        }
    }

    @Override
    public boolean getBoolean(String key, boolean defValue) {
        switch (key) {
            case "terminal_view_key_logging_enabled":
                return mPreferences.getTerminalViewKeyLoggingEnabled();
            case "plugin_error_notifications_enabled":
                return mPreferences.getPluginErrorNotificationsEnabled();
            default:
                return false;
        }
    }

}
