package com.termux.app.fragments.settings.termux;

import android.content.Context;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.preference.ListPreference;
import androidx.preference.PreferenceDataStore;

import com.termux.R;
import com.termux.app.fragments.settings.base.BaseDebuggingPreferencesFragment;
import com.termux.app.fragments.settings.base.BaseDebuggingPreferencesDataStore;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.logger.Logger;

/**
 * Termux-specific debugging preferences fragment.
 * Extends BaseDebuggingPreferencesFragment to follow DRY principle.
 */
@Keep
public class DebuggingPreferencesFragment extends BaseDebuggingPreferencesFragment<TermuxAppSharedPreferences> {

    @Override
    protected int getPreferencesResourceId() {
        return R.xml.termux_debugging_preferences;
    }

    @Override
    protected TermuxAppSharedPreferences createPreferences(@NonNull Context context) {
        return TermuxAppSharedPreferences.build(context, true);
    }

    @Override
    protected PreferenceDataStore createPreferenceDataStore(@NonNull Context context) {
        return DebuggingPreferencesDataStore.getInstance(context);
    }

    @Override
    protected int getLogLevel() {
        return mPreferences != null ? mPreferences.getLogLevel() : Logger.DEFAULT_LOG_LEVEL;
    }

    /**
     * Static helper method retained for backward compatibility.
     * Uses enhanced Logger methods for labels.
     */
    public static ListPreference setLogLevelListPreferenceData(ListPreference logLevelListPreference, 
                                                                Context context, int logLevel) {
        if (logLevelListPreference == null)
            logLevelListPreference = new ListPreference(context);

        CharSequence[] logLevels = Logger.getLogLevelsArray();
        CharSequence[] logLevelLabels = Logger.getLogLevelLabelsArray(context, logLevels, true);

        logLevelListPreference.setEntryValues(logLevels);
        logLevelListPreference.setEntries(logLevelLabels);

        logLevelListPreference.setValue(String.valueOf(logLevel));
        logLevelListPreference.setDefaultValue(Logger.DEFAULT_LOG_LEVEL);

        return logLevelListPreference;
    }
}

/**
 * Termux-specific PreferenceDataStore implementation.
 * Extends BaseDebuggingPreferencesDataStore to follow DRY principle.
 */
class DebuggingPreferencesDataStore extends BaseDebuggingPreferencesDataStore<TermuxAppSharedPreferences> {

    private static DebuggingPreferencesDataStore mInstance;

    private DebuggingPreferencesDataStore(Context context) {
        super(context);
    }

    public static synchronized DebuggingPreferencesDataStore getInstance(Context context) {
        if (mInstance == null) {
            mInstance = new DebuggingPreferencesDataStore(context);
        }
        return mInstance;
    }

    @Override
    protected TermuxAppSharedPreferences createPreferences(Context context) {
        return TermuxAppSharedPreferences.build(context, true);
    }

    @Override
    protected int getLogLevel() {
        return mPreferences != null ? mPreferences.getLogLevel() : Logger.DEFAULT_LOG_LEVEL;
    }

    @Override
    protected void setLogLevel(int level) {
        if (mPreferences != null) {
            mPreferences.setLogLevel(mContext, level);
        }
    }

    @Override
    protected boolean getAdditionalBoolean(String key, boolean defValue) {
        if (mPreferences == null) return defValue;
        
        switch (key) {
            case "terminal_view_key_logging_enabled":
                return mPreferences.isTerminalViewKeyLoggingEnabled();
            case "plugin_error_notifications_enabled":
                return mPreferences.arePluginErrorNotificationsEnabled(false);
            case "crash_report_notifications_enabled":
                return mPreferences.areCrashReportNotificationsEnabled(false);
            default:
                return defValue;
        }
    }

    @Override
    protected void putAdditionalBoolean(String key, boolean value) {
        if (mPreferences == null) return;
        
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
        }
    }
}
