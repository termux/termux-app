package com.termux.app.fragments.settings;

import android.os.Bundle;
import androidx.preference.PreferenceFragmentCompat;
import com.termux.R;

public class AISettingsFragment extends PreferenceFragmentCompat {
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.ai_preferences, rootKey);
    }
}
