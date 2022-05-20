package com.termux.app.datastore.termux;

import android.content.Context;

import androidx.preference.PreferenceDataStore;

import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;

public class TerminalViewPreferencesDataStore extends PreferenceDataStore {

    private final Context mContext;
    private final TermuxAppSharedPreferences mPreferences;

    private static TerminalViewPreferencesDataStore mInstance;

    private TerminalViewPreferencesDataStore(Context context) {
        mContext = context;
        mPreferences = TermuxAppSharedPreferences.build(context, true);
    }

    public static synchronized TerminalViewPreferencesDataStore getInstance(Context context) {
        if (mInstance == null) {
            mInstance = new TerminalViewPreferencesDataStore(context);
        }
        return mInstance;
    }



    @Override
    public void putBoolean(String key, boolean value) {
        if (isDisability(key)) return;

        if(key == "terminal_margin_adjustment")
            mPreferences.setTerminalMarginAdjustment(value);

    }

    private boolean isDisability(String key) {
        return (mPreferences == null || key == null) ? true : false;
    }

    @Override
    public boolean getBoolean(String key, boolean defValue) {
        if (mPreferences == null) return false;
        return(key == "terminal_margin_adjustment")? mPreferences.isTerminalMarginAdjustmentEnabled(): false;

        }
    }

}

