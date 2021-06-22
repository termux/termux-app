package com.termux.app.activities;

import android.content.Context;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

import com.termux.R;
import com.termux.app.models.ReportInfo;
import com.termux.app.models.UserAction;
import com.termux.shared.interact.ShareUtils;
import com.termux.shared.packages.PackageUtils;
import com.termux.shared.settings.preferences.TermuxTaskerAppSharedPreferences;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxUtils;

public class SettingsActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_settings);
        if (savedInstanceState == null) {
            getSupportFragmentManager()
                .beginTransaction()
                .replace(R.id.settings, new RootPreferencesFragment())
                .commit();
        }
        ActionBar actionBar = getSupportActionBar();
        if (actionBar != null) {
            actionBar.setDisplayHomeAsUpEnabled(true);
            actionBar.setDisplayShowHomeEnabled(true);
        }
    }

    @Override
    public boolean onSupportNavigateUp() {
        onBackPressed();
        return true;
    }

    public static class RootPreferencesFragment extends PreferenceFragmentCompat {
        @Override
        public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
            Context context = getContext();
            if (context == null) return;

            setPreferencesFromResource(R.xml.root_preferences, rootKey);

            configureTermuxTaskerPreference(context);
            configureAboutPreference(context);
            configureDonatePreference(context);
        }

        private void configureTermuxTaskerPreference(@NonNull Context context) {
            Preference termuxTaskerPrefernce = findPreference("termux_tasker");
            if (termuxTaskerPrefernce != null) {
                TermuxTaskerAppSharedPreferences preferences = TermuxTaskerAppSharedPreferences.build(context, false);
                // If failed to get app preferences, then likely app is not installed, so do not show its preference
                termuxTaskerPrefernce.setVisible(preferences != null);
            }
        }

        private void configureAboutPreference(@NonNull Context context) {
            Preference aboutPreference = findPreference("about");
            if (aboutPreference != null) {
                aboutPreference.setOnPreferenceClickListener(preference -> {
                    new Thread() {
                        @Override
                        public void run() {
                            String title = "About";

                            StringBuilder aboutString = new StringBuilder();
                            aboutString.append(TermuxUtils.getAppInfoMarkdownString(context, false));

                            String termuxPluginAppsInfo =  TermuxUtils.getTermuxPluginAppsInfoMarkdownString(context);
                            if (termuxPluginAppsInfo != null)
                                aboutString.append("\n\n").append(termuxPluginAppsInfo);

                            aboutString.append("\n\n").append(TermuxUtils.getDeviceInfoMarkdownString(context));
                            aboutString.append("\n\n").append(TermuxUtils.getImportantLinksMarkdownString(context));

                            ReportActivity.startReportActivity(context, new ReportInfo(UserAction.ABOUT, TermuxConstants.TERMUX_APP.TERMUX_SETTINGS_ACTIVITY_NAME, title, null, aboutString.toString(), null, false));
                        }
                    }.start();

                    return true;
                });
            }
        }

        private void configureDonatePreference(@NonNull Context context) {
            Preference donatePreference = findPreference("donate");
            if (donatePreference != null) {
                String signingCertificateSHA256Digest = PackageUtils.getSigningCertificateSHA256DigestForPackage(context);
                if (signingCertificateSHA256Digest != null) {
                    // If APK is a Google Playstore release, then do not show the donation link
                    // since Termux isn't exempted from the playstore policy donation links restriction
                    // Check Fund solicitations: https://pay.google.com/intl/en_in/about/policy/
                    String apkRelease = TermuxUtils.getAPKRelease(signingCertificateSHA256Digest);
                    if (apkRelease == null || apkRelease.equals(TermuxConstants.APK_RELEASE_GOOGLE_PLAYSTORE_SIGNING_CERTIFICATE_SHA256_DIGEST)) {
                        donatePreference.setVisible(false);
                        return;
                    } else {
                        donatePreference.setVisible(true);
                    }
                }

                donatePreference.setOnPreferenceClickListener(preference -> {
                    ShareUtils.openURL(context, TermuxConstants.TERMUX_DONATE_URL);
                    return true;
                });
            }
        }
    }

}
