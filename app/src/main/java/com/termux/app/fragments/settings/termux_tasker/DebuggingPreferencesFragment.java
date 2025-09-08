package com.termux.app.fragments.settings.termux_tasker;

import android.content.Context;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.preference.PreferenceDataStore;

import com.termux.R;
import com.termux.app.fragments.settings.base.BaseDebuggingPreferencesFragment;
import com.termux.app.fragments.settings.base.BaseDebuggingPreferencesDataStore;
import com.termux.shared.termux.settings.preferences.TermuxTaskerAppSharedPreferences;
import com.termux.shared.logger.Logger;

/**
 * TermuxTasker-specific debugging preferences fragment.
 * Extends BaseDebuggingPreferencesFragment to follow DRY principle.
 */
@Keep
public class DebuggingPreferencesFragment extends BaseDebuggingPreferencesFragment<TermuxTaskerAppSharedPreferences> {

    @Override
    protected int getPreferencesResourceId() {
        return R.xml.termux_tasker_debugging_preferences;
    }

    @Override
    protected TermuxTaskerAppSharedPreferences createPreferences(@NonNull Context context) {
        return TermuxTaskerAppSharedPreferences.build(context, true);
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
 * TermuxTasker-specific PreferenceDataStore implementation.
 * Extends BaseDebuggingPreferencesDataStore to follow DRY principle.
 */
class DebuggingPreferencesDataStore extends BaseDebuggingPreferencesDataStore<TermuxTaskerAppSharedPreferences> {

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
    protected TermuxTaskerAppSharedPreferences createPreferences(Context context) {
        return TermuxTaskerAppSharedPreferences.build(context, true);
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
}