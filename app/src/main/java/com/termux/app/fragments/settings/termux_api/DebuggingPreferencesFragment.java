package com.termux.app.fragments.settings.termux_api;

import android.content.Context;
import android.os.Bundle;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.ListPreference;
import androidx.preference.PreferenceCategory;
import androidx.preference.PreferenceDataStore;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;

import com.termux.R;
import com.termux.shared.termux.settings.preferences.TermuxAPIAppSharedPreferences;

@Keep
public class DebuggingPreferencesFragment extends PreferenceFragmentCompat {

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        Context context = getContext();
        if (context == null) return;

        PreferenceManager preferenceManager = getPreferenceManager();
        preferenceManager.setPreferenceDataStore(DebuggingPreferencesDataStore.getInstance(context));

        setPreferencesFromResource(R.xml.termux_api_debugging_preferences, rootKey);

        configureLoggingPreferences(context);
    }

    private void configureLoggingPreferences(@NonNull Context context) {
        PreferenceCategory loggingCategory = findPreference("logging");
        if (loggingCategory == null) return;

        ListPreference logLevelListPreference = findPreference("log_level");
        if (logLevelListPreference != null) {
            TermuxAPIAppSharedPreferences preferences = TermuxAPIAppSharedPreferences.build(context, true);
            if (preferences == null) return;

            com.termux.app.fragments.settings.termux.DebuggingPreferencesFragment.
                setLogLevelListPreferenceData(logLevelListPreference, context, preferences.getLogLevel(true));
            loggingCategory.addPreference(logLevelListPreference);
        }
    }
}

class DebuggingPreferencesDataStore extends PreferenceDataStore {

    private final Context mContext;
    private final TermuxAPIAppSharedPreferences mPreferences;

    private static DebuggingPreferencesDataStore mInstance;

    private DebuggingPreferencesDataStore(Context context) {
        mContext = context;
        mPreferences = TermuxAPIAppSharedPreferences.build(context, true);
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
                return String.valueOf(mPreferences.getLogLevel(true));
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
                    mPreferences.setLogLevel(mContext, Integer.parseInt(value), true);
                }
                break;
            default:
                break;
        }
    }

}
