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

            setAboutOnPreferenceClickListener();
        }

        private void setAboutOnPreferenceClickListener() {
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
    }

}
