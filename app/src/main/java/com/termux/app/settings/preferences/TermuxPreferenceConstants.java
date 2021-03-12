package com.termux.app.settings.preferences;

import com.termux.app.TermuxConstants;

/*
 * Version: v0.1.0
 *
 * Changelog
 *
 * - 0.1.0 (2021-03-12)
 *      - Initial Release.
 *
 */

/**
 * A class that defines shared constants of the Shared preferences used by Termux app and its plugins.
 * This class will be hosted by termux-app and should be imported by other termux plugin apps as is
 * instead of copying constants to random classes. The 3rd party apps can also import it for
 * interacting with termux apps. If changes are made to this file, increment the version number
 * and add an entry in the Changelog section above.
 *
 * The properties are loaded from the first file found at
 * {@link TermuxConstants#TERMUX_PROPERTIES_PRIMARY_FILE_PATH} or
 * {@link TermuxConstants#TERMUX_PROPERTIES_SECONDARY_FILE_PATH}
 */
public final class TermuxPreferenceConstants {

    /** Defines the key for whether to show extra keys in termux terminal view */
    public static final String KEY_SHOW_EXTRA_KEYS = "show_extra_keys";
    public static final boolean DEFAULT_VALUE_SHOW_EXTRA_KEYS = true;



    /** Defines the key for whether to always keep screen on */
    public static final String KEY_KEEP_SCREEN_ON = "screen_always_on";
    public static final boolean DEFAULT_VALUE_KEEP_SCREEN_ON = true;



    /** Defines the key for font size of termux terminal view */
    public static final String KEY_FONTSIZE = "fontsize";



    /** Defines the key for current termux terminal session */
    public static final String KEY_CURRENT_SESSION = "current_session";



}
