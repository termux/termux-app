package com.termux.shared.termux.settings;

import android.content.Context;
import android.content.res.Configuration;
import android.os.Build;

import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.app.LocaleManagerCompat;
import androidx.core.os.LocaleListCompat;

import java.util.Locale;

/** Locale-aware resources for non-activity contexts, without changing process-wide defaults. */
public final class TermuxAppLocaleUtils {

    private TermuxAppLocaleUtils() {}

    public static String getSelectedLanguage(Context context) {
        // On API 33+ use the context directly: an activity delegate might not be registered yet
        // (attachBaseContext or a service-only cold start). Never interpret empty locales as a
        // selection to store; AppCompat may still be migrating the pre-33 preference.
        LocaleListCompat locales = Build.VERSION.SDK_INT >= 33
            ? LocaleManagerCompat.getApplicationLocales(context)
            : AppCompatDelegate.getApplicationLocales();
        // Below 33 the in-memory choice wins, including before its asynchronous persistence.
        // With no active choice, the public AndroidX API also supports service-only cold starts.
        if (Build.VERSION.SDK_INT < 33 && locales.isEmpty())
            locales = LocaleManagerCompat.getApplicationLocales(context);
        return TerminalLanguagePolicy.languageForSelection(locales.isEmpty() ? null : locales.get(0).getLanguage());
    }

    public static Context getLocalizedContext(Context context) {
        // Retain overrides on the supplied context; createConfigurationContext may otherwise
        // resolve unspecified fields against the system rather than this context's configuration.
        Configuration configuration = new Configuration(context.getResources().getConfiguration());
        configuration.setLocale(Locale.forLanguageTag(getSelectedLanguage(context)));
        return context.createConfigurationContext(configuration);
    }
}
