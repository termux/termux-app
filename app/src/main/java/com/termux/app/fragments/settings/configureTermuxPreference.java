package com.termux.app.fragments.settings;

import android.content.Context;

import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

public abstract class configureTermuxPreference extends PreferenceFragmentCompat {
    abstract Preference  findPreferenceMethod();
    abstract void build(Context context, Preference termuxPreference );
    final public void generate(Context context) {
        findPreferenceMethod();
        build(context, findPreferenceMethod());
    }

}
