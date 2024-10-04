package com.termux.app.fragments.settings.termux;

import android.content.Context;
import android.os.Bundle;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.preference.PreferenceDataStore;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;
import androidx.preference.SwitchPreferenceCompat;

import com.termux.R;
import com.termux.app.style.TermuxBackgroundManager;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;

@Keep
public class TermuxStylePreferencesFragment extends PreferenceFragmentCompat {

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        Context context = getContext();
        if (context == null) return;

        PreferenceManager preferenceManager = getPreferenceManager();
        preferenceManager.setPreferenceDataStore(TermuxStylePreferencesDataStore.getInstance(context));

        setPreferencesFromResource(R.xml.termux_style_preferences, rootKey);

        configureBackgroundPreferences(context);
    }

    /**
     * Configure background preferences and make appropriate changes in the state of components.
     *
     * @param context The context for operations.
     */
    private void configureBackgroundPreferences(@NonNull Context context) {
        SwitchPreferenceCompat backgroundImagePreference = findPreference("background_image_enabled");

        if (backgroundImagePreference != null) {
            TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(context, true);

            if (preferences == null) return;

            // If background image preference is disabled and background images are
            // missing, then don't allow user to enable it from setting.
            if (!preferences.isBackgroundImageEnabled() && !TermuxBackgroundManager.isImageFilesExist(context, false)) {
                backgroundImagePreference.setEnabled(false);
            }
        }
    }

}

class TermuxStylePreferencesDataStore extends PreferenceDataStore {

    private final Context mContext;

    private final TermuxAppSharedPreferences mPreferences;

    private static TermuxStylePreferencesDataStore mInstance;

    private TermuxStylePreferencesDataStore(Context context) {
        mContext = context;
        mPreferences = TermuxAppSharedPreferences.build(context, true);
    }

    public static synchronized TermuxStylePreferencesDataStore getInstance(Context context) {
        if (mInstance == null) {
            mInstance = new TermuxStylePreferencesDataStore(context);
        }

        return mInstance;
    }



    @Override
    public void putBoolean(String key, boolean value) {
        if (mPreferences == null) return;
        if (key == null) return;

        switch (key) {
            case "background_image_enabled":
                mPreferences.setBackgroundImageEnabled(value);
            default:
                break;
        }
    }

    @Override
    public boolean getBoolean(String key, boolean defValue) {
        if (mPreferences == null) return false;

        switch (key) {
            case "background_image_enabled":
                return mPreferences.isBackgroundImageEnabled();
            default:
                return false;
        }
    }

}
