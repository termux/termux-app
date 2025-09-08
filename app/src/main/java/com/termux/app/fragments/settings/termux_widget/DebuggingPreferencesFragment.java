package com.termux.app.fragments.settings.termux_widget;

import android.content.Context;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.preference.PreferenceDataStore;

import com.termux.R;
import com.termux.app.fragments.settings.base.BaseDebuggingPreferencesFragment;
import com.termux.app.fragments.settings.base.BaseDebuggingPreferencesDataStore;
import com.termux.shared.termux.settings.preferences.TermuxWidgetAppSharedPreferences;
import com.termux.shared.logger.Logger;

/**
 * TermuxWidget-specific debugging preferences fragment.
 * Extends BaseDebuggingPreferencesFragment to follow DRY principle.
 */
@Keep
public class DebuggingPreferencesFragment extends BaseDebuggingPreferencesFragment<TermuxWidgetAppSharedPreferences> {

    @Override
    protected int getPreferencesResourceId() {
        return R.xml.termux_widget_debugging_preferences;
    }

    @Override
    protected TermuxWidgetAppSharedPreferences createPreferences(@NonNull Context context) {
        return TermuxWidgetAppSharedPreferences.build(context, true);
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
 * TermuxWidget-specific PreferenceDataStore implementation.
 * Extends BaseDebuggingPreferencesDataStore to follow DRY principle.
 */
class DebuggingPreferencesDataStore extends BaseDebuggingPreferencesDataStore<TermuxWidgetAppSharedPreferences> {

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
    protected TermuxWidgetAppSharedPreferences createPreferences(Context context) {
        return TermuxWidgetAppSharedPreferences.build(context, true);
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