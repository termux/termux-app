package com.termux.app;

import android.app.Application;

import com.termux.app.settings.preferences.TermuxSharedPreferences;
import com.termux.app.utils.Logger;


public class TermuxApplication extends Application {
    public void onCreate() {
        super.onCreate();

        updateLogLevel();
    }

    private void updateLogLevel() {
        // Load the log level from shared preferences and set it to the {@link Loggger.CURRENT_LOG_LEVEL}
        TermuxSharedPreferences preferences = new TermuxSharedPreferences(getApplicationContext());
        preferences.setLogLevel(null, preferences.getLogLevel());
        Logger.logDebug("Starting Application");
    }
}

