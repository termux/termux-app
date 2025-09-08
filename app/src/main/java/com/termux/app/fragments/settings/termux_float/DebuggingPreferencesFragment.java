package com.termux.app.fragments.settings.termux_float;

import android.content.Context;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.preference.PreferenceDataStore;

import com.termux.R;
import com.termux.app.fragments.settings.base.BaseDebuggingPreferencesFragment;
import com.termux.app.fragments.settings.base.BaseDebuggingPreferencesDataStore;
import com.termux.shared.termux.settings.preferences.TermuxFloatAppSharedPreferences;
import com.termux.shared.logger.Logger;

/**
 * TermuxFloat-specific debugging preferences fragment.
 * Extends BaseDebuggingPreferencesFragment to follow DRY principle.
 */
@Keep
public class DebuggingPreferencesFragment extends BaseDebuggingPreferencesFragment<TermuxFloatAppSharedPreferences> {

    @Override
    protected int getPreferencesResourceId() {
        return R.xml.termux_float_debugging_preferences;
    }

    @Override
    protected TermuxFloatAppSharedPreferences createPreferences(@NonNull Context context) {
        return TermuxFloatAppSharedPreferences.build(context, true);
    }

    @Override
    protected PreferenceDataStore createPreferenceDataStore(@NonNull Context context) {
        return DebuggingPreferencesDataStore.getInstance(context);
    }

    @Override
    protected int getLogLevel() {
        return mPreferences != null ? mPreferences.getLogLevel(true) : Logger.DEFAULT_LOG_LEVEL;
    }
}

/**
 * TermuxFloat-specific PreferenceDataStore implementation.
 * Extends BaseDebuggingPreferencesDataStore to follow DRY principle.
 */
class DebuggingPreferencesDataStore extends BaseDebuggingPreferencesDataStore<TermuxFloatAppSharedPreferences> {

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
    protected TermuxFloatAppSharedPreferences createPreferences(Context context) {
        return TermuxFloatAppSharedPreferences.build(context, true);
    }

    @Override
    protected int getLogLevel() {
        return mPreferences != null ? mPreferences.getLogLevel(true) : Logger.DEFAULT_LOG_LEVEL;
    }

    @Override
    protected void setLogLevel(int level) {
        if (mPreferences != null) {
            mPreferences.setLogLevel(mContext, level, true);
        }
    }

    @Override
    protected boolean getAdditionalBoolean(String key, boolean defValue) {
        if (mPreferences == null) return defValue;
        
        switch (key) {
            case "terminal_view_key_logging_enabled":
                return mPreferences.isTerminalViewKeyLoggingEnabled(true);
            default:
                return defValue;
        }
    }

    @Override
    protected void putAdditionalBoolean(String key, boolean value) {
        if (mPreferences == null) return;
        
        switch (key) {
            case "terminal_view_key_logging_enabled":
                mPreferences.setTerminalViewKeyLoggingEnabled(value, true);
                break;
        }
    }
}
