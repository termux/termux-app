package com.termux.shared.termux.activities;

import android.content.Context;

import androidx.appcompat.app.AppCompatActivity;

import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.settings.TermuxAppLocaleUtils;

/**
 * Applies Termux's English default while leaving explicit app locales to AppCompat.
 * Other applications using termux-shared retain their own locale policy.
 * No process-wide Locale.setDefault or forced view direction is used.
 */
public class TermuxLocaleActivity extends AppCompatActivity {

    @Override
    protected void attachBaseContext(Context base) {
        // English is only a resource fallback, never a persisted user selection. In particular,
        // empty framework locales on API 33 may mean AppCompat's API 32 -> 33 migration is still
        // queued. Writing English here or in onCreate would prevent it from importing Arabic.
        // AppCompat still attaches its delegate, restores pre-33 locales and owns migration.
        super.attachBaseContext(TermuxConstants.TERMUX_PACKAGE_NAME.equals(base.getPackageName())
            ? TermuxAppLocaleUtils.getLocalizedContext(base) : base);
    }
}
