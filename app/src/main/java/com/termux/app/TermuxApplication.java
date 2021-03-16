package com.termux.app;

import android.app.Application;

import com.termux.app.settings.preferences.TermuxAppSharedPreferences;
import com.termux.app.utils.Logger;


public class TermuxApplication extends Application {
    public void onCreate() {
        super.onCreate();

        updateLogLevel();
    }

    private void updateLogLevel() {
        // Load the log level from shared preferences and set it to the {@link Loggger.CURRENT_LOG_LEVEL}
        TermuxAppSharedPreferences preferences = new TermuxAppSharedPreferences(getApplicationContext());
        preferences.setLogLevel(null, preferences.getLogLevel());
        Logger.logDebug("Starting Application");
    }
}

