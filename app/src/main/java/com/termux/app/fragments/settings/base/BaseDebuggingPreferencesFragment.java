package com.termux.app.fragments.settings.base;

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
import com.termux.shared.logger.Logger;

/**
 * Abstract base class for all DebuggingPreferencesFragment implementations.
 * Follows DRY principle by extracting common functionality.
 * 
 * @param <T> The type of SharedPreferences implementation used by the concrete fragment
 */
@Keep
public abstract class BaseDebuggingPreferencesFragment<T> extends PreferenceFragmentCompat {

    private static final String LOG_TAG = "BaseDebuggingPreferencesFragment";
    
    protected T mPreferences;
    protected Context mContext;

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mContext = getContext();
        if (mContext == null) {
            Logger.logError(LOG_TAG, "Context is null in onCreatePreferences");
            return;
        }

        PreferenceManager preferenceManager = getPreferenceManager();
        PreferenceDataStore dataStore = createPreferenceDataStore(mContext);
        if (dataStore != null) {
            preferenceManager.setPreferenceDataStore(dataStore);
        }

        setPreferencesFromResource(getPreferencesResourceId(), rootKey);

        mPreferences = createPreferences(mContext);
        if (mPreferences != null) {
            configureLoggingPreferences(mContext);
            configureAdditionalPreferences(mContext);
        }
    }

    /**
     * Template method to get the XML resource ID for preferences.
     * Each subclass must provide its specific resource.
     */
    protected abstract int getPreferencesResourceId();

    /**
     * Template method to create the specific SharedPreferences implementation.
     * Each subclass must provide its specific preferences type.
     */
    protected abstract T createPreferences(@NonNull Context context);

    /**
     * Template method to create the PreferenceDataStore.
     * Each subclass must provide its specific data store.
     */
    protected abstract PreferenceDataStore createPreferenceDataStore(@NonNull Context context);

    /**
     * Template method to get the current log level from preferences.
     * Each subclass must implement based on its preferences type.
     */
    protected abstract int getLogLevel();

    /**
     * Common method to configure logging preferences.
     * Shared across all debugging fragments.
     */
    protected void configureLoggingPreferences(@NonNull Context context) {
        PreferenceCategory loggingCategory = findPreference("logging");
        if (loggingCategory == null) return;

        ListPreference logLevelListPreference = findPreference("log_level");
        if (logLevelListPreference != null && mPreferences != null) {
            setLogLevelListPreferenceData(logLevelListPreference, context, getLogLevel());
            loggingCategory.addPreference(logLevelListPreference);
        }
    }

    /**
     * Hook method for subclasses to add additional preference configurations.
     * Default implementation does nothing.
     */
    protected void configureAdditionalPreferences(@NonNull Context context) {
        // Subclasses can override to add specific configurations
    }

    /**
     * Common method to set log level preference data.
     * Extracted from the original TermuxDebuggingPreferencesFragment.
     */
    public static void setLogLevelListPreferenceData(ListPreference logLevelListPreference, 
                                                      Context context, int logLevel) {
        if (logLevelListPreference == null) return;
        
        String[] logLevels = new String[] {
            "Verbose", "Debug", "Info", "Warning", "Error", "Assert", "Off"
        };
        String[] logLevelValues = new String[] {
            "2", "3", "4", "5", "6", "7", "8"
        };
        
        logLevelListPreference.setEntries(logLevels);
        logLevelListPreference.setEntryValues(logLevelValues);
        logLevelListPreference.setValue(String.valueOf(logLevel));
        logLevelListPreference.setSummary(logLevels[Math.max(0, logLevel - 2)]);
        
        logLevelListPreference.setOnPreferenceChangeListener((preference, newValue) -> {
            int index = logLevelListPreference.findIndexOfValue(newValue.toString());
            preference.setSummary(logLevels[index]);
            return true;
        });
    }

    /**
     * Getter for preferences object.
     * Can be used by subclasses if needed.
     */
    protected T getPreferences() {
        return mPreferences;
    }
}