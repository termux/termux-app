package com.termux.shared.theme;

import androidx.appcompat.app.AppCompatDelegate;

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

    private final String name;
    private final int mode;

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

    public static Integer modeOf(String name) {
        if (TRUE.name.equals(name))
            return TRUE.mode;
        else if (FALSE.name.equals(name))
            return FALSE.mode;
        else if (SYSTEM.name.equals(name)) {
            return SYSTEM.mode;
        } else {
            return null;
        }
    }

}
