package com.termux.app.activities;

import android.content.Context;
import android.os.Bundle;

import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

import com.termux.R;
import com.termux.app.models.ReportInfo;
import com.termux.app.models.UserAction;
import com.termux.shared.interact.ShareUtils;
import com.termux.shared.packages.PackageUtils;
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
            setPreferencesFromResource(R.xml.root_preferences, rootKey);

            configureAboutPreference();
            configureDonatePreference();
        }

        private void configureAboutPreference() {
            Context context = getContext();
            Preference about = findPreference("about");
            if (context != null && about != null) {
                about.setOnPreferenceClickListener(preference -> {
                    String title = "About";

                    StringBuilder aboutString = new StringBuilder();
                    aboutString.append(TermuxUtils.getAppInfoMarkdownString(context, false));

                    String termuxPluginAppsInfo =  TermuxUtils.getTermuxPluginAppsInfoMarkdownString(context);
                    if (termuxPluginAppsInfo != null)
                        aboutString.append("\n\n").append(termuxPluginAppsInfo);

                    aboutString.append("\n\n").append(TermuxUtils.getDeviceInfoMarkdownString(context));
                    aboutString.append("\n\n").append(TermuxUtils.getImportantLinksMarkdownString(context));

                    ReportActivity.startReportActivity(context, new ReportInfo(UserAction.ABOUT, TermuxConstants.TERMUX_APP.TERMUX_SETTINGS_ACTIVITY_NAME, title, null, aboutString.toString(), null, false));

                    return true;
                });
            }
        }

        private void configureDonatePreference() {
            Context context = getContext();
            Preference donate = findPreference("donate");
            if (context != null && donate != null) {
                String signingCertificateSHA256Digest = PackageUtils.getSigningCertificateSHA256DigestForPackage(context);
                if (signingCertificateSHA256Digest != null) {
                    // If APK is a Google Playstore release, then do not show the donation link
                    // since Termux isn't exempted from the playstore policy donation links restriction
                    // Check Fund solicitations: https://pay.google.com/intl/en_in/about/policy/
                    String apkRelease = TermuxUtils.getAPKRelease(signingCertificateSHA256Digest);
                    if (apkRelease == null || apkRelease.equals(TermuxConstants.APK_RELEASE_GOOGLE_PLAYSTORE_SIGNING_CERTIFICATE_SHA256_DIGEST)) {
                        donate.setVisible(false);
                        return;
                    } else {
                        donate.setVisible(true);
                    }
                }

                donate.setOnPreferenceClickListener(preference -> {
                    ShareUtils.openURL(context, TermuxConstants.TERMUX_DONATE_URL);
                    return true;
                });
            }
        }
    }

}
