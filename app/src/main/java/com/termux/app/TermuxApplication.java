package com.termux.app;

import android.app.Application;
import android.content.Context;

import com.termux.am.Am;
import com.termux.shared.logger.Logger;
import com.termux.shared.shell.LocalSocketListener;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.crash.TermuxCrashUtils;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.termux.settings.properties.TermuxAppSharedProperties;
import com.termux.shared.termux.theme.TermuxThemeUtils;

import java.io.File;

public class TermuxApplication extends Application {

    public void onCreate() {
        super.onCreate();

        Context context = getApplicationContext();

        // Set crash handler for the app
        TermuxCrashUtils.setCrashHandler(this);

        // Set log config for the app
        setLogConfig(context);

        Logger.logDebug("Starting Application");

        // Init app wide SharedProperties loaded from termux.properties
        TermuxAppSharedProperties properties = TermuxAppSharedProperties.init(context);

        // Set NightMode.APP_NIGHT_MODE
        TermuxThemeUtils.setAppNightMode(properties.getNightMode());

        if (LocalSocketListener.tryEstablishLocalSocketListener(this, (args, out, err) -> {
            try {
                new Am(out, err, this).run(args);
                return 0;
            } catch (Exception e) {
                return 1;
            }
        }, new File(getFilesDir(), "am-socket").getAbsolutePath(), 100, 1000) == null) {
            Logger.logWarn("TermuxApplication", "am socket cannot be created");
        }
    }

    public static void setLogConfig(Context context) {
        Logger.setDefaultLogTag(TermuxConstants.TERMUX_APP_NAME);

        // Load the log level from shared preferences and set it to the {@link Logger.CURRENT_LOG_LEVEL}
        TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(context);
        if (preferences == null) return;
        preferences.setLogLevel(null, preferences.getLogLevel());
    }

}

