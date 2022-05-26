package com.termux.app.fragments.settings;

import android.content.Context;
import android.os.Bundle;

import androidx.preference.Preference;

import com.termux.R;
import com.termux.shared.termux.settings.preferences.TermuxAPIAppSharedPreferences;

public class configureTermuxAPIPreference extends configureTermuxPreference{
    private Preference termuxAPIPreference;

    @Override
    Preference findPreferenceMethod() {
        return termuxAPIPreference = findPreference("termux_api");
    }

    @Override
    void build(Context context, Preference termuxPreference) {
        if (termuxAPIPreference != null) {
            TermuxAPIAppSharedPreferences preferences = TermuxAPIAppSharedPreferences.build(context, false);
            termuxAPIPreference.setVisible(preferences != null);
        }
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.root_preferences, rootKey);
        Context context = getContext();
        generate(context);
        new Thread(new Runnable() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                generate(context);
            }
        }).start();

    }


}
