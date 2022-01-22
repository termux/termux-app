package com.termux.shared.theme;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatDelegate;

import com.termux.shared.logger.Logger;

/** The modes used by to decide night mode for themes. */
public enum NightMode {

    /** Night theme should be enabled. */
    TRUE("true", AppCompatDelegate.MODE_NIGHT_YES),

    /** Dark theme should be enabled. */
    FALSE("false", AppCompatDelegate.MODE_NIGHT_NO),

    /**
     * Use night or dark theme depending on system night mode.
     * https://developer.android.com/guide/topics/resources/providing-resources#NightQualifier
     */
    SYSTEM("system", AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM);

    /** The current app wide night mode used by various libraries. Defaults to {@link #SYSTEM}. */
    private static NightMode APP_NIGHT_MODE;

    private static final String LOG_TAG = "NightMode";

    private final String name;
    private final @AppCompatDelegate.NightMode int mode;

    NightMode(final String name, int mode) {
        this.name = name;
        this.mode = mode;
    }

    public String getName() {
        return name;
    }

    public int getMode() {
        return mode;
    }


    /** Get {@link NightMode} for {@code name} if found, otherwise {@code null}. */
    @Nullable
    public static NightMode modeOf(String name) {
        for (NightMode v : NightMode.values()) {
            if (v.name.equals(name)) {
                return v;
            }
        }

        return null;
    }

    /** Get {@link NightMode} for {@code name} if found, otherwise {@code def}. */
    @NonNull
    public static NightMode modeOf(@Nullable String name, @NonNull NightMode def) {
        NightMode nightMode = modeOf(name);
        return nightMode != null ? nightMode : def;
    }


    /** Set {@link #APP_NIGHT_MODE}. */
    public static void setAppNightMode(@Nullable String name) {
        if (name == null || name.isEmpty()) {
            APP_NIGHT_MODE = SYSTEM;
        } else {
            NightMode nightMode = NightMode.modeOf(name);
            if (nightMode == null) {
                Logger.logError(LOG_TAG, "Invalid APP_NIGHT_MODE \"" + name + "\"");
                return;
            }
            APP_NIGHT_MODE = nightMode;
        }

        Logger.logVerbose(LOG_TAG, "Set APP_NIGHT_MODE to \"" + APP_NIGHT_MODE.getName() + "\"");
    }

    /** Get {@link #APP_NIGHT_MODE}. */
    @NonNull
    public static NightMode getAppNightMode() {
        if (APP_NIGHT_MODE == null)
            APP_NIGHT_MODE =  SYSTEM;

        return APP_NIGHT_MODE;
    }

}
