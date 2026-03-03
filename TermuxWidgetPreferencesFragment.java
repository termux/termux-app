package com.termux.app.fragments.settings;

import android.content.Context;
import android.os.Bundle;

import androidx.annotation.Keep;
import androidx.preference.PreferenceDataStore;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;

import com.termux.R;
import com.termux.shared.termux.settings.preferences.TermuxWidgetAppSharedPreferences;

@Keep
public class TermuxWidgetPreferencesFragment extends PreferenceFragmentCompat {

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        Context context = getContext();
        if (context == null) return;

        PreferenceManager preferenceManager = getPreferenceManager();
        preferenceManager.setPreferenceDataStore(TermuxWidgetPreferencesDataStore.getInstance(context));

        setPreferencesFromResource(R.xml.termux_widget_preferences, rootKey);
    }

}

class TermuxWidgetPreferencesDataStore extends PreferenceDataStore {

    private final Context mContext;
    private final TermuxWidgetAppSharedPreferences mPreferences;

    private static TermuxWidgetPreferencesDataStore mInstance;

    private TermuxWidgetPreferencesDataStore(Context context) {
        mContext = context;
        mPreferences = TermuxWidgetAppSharedPreferences.build(context, true);
    }

    public static synchronized TermuxWidgetPreferencesDataStore getInstance(Context context) {
        if (mInstance == null) {
            mInstance = new TermuxWidgetPreferencesDataStore(context);
        }
        return mInstance;
    }

}
