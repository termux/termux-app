package com.termux.app.settings.preferences;

/*
 * Version: v0.3.0
 *
 * Changelog
 *
 * - 0.1.0 (2021-03-12)
 *      - Initial Release.
 *
 * - 0.2.0 (2021-03-13)
 *      - Added `KEY_LOG_LEVEL` and `KEY_TERMINAL_VIEW_LOGGING_ENABLED`
 * 
 * - 0.3.0 (2021-03-16)
 *      - Changed to per app scoping of variables so that the same file can store all constants of
 *          Termux app and its plugins. This will allow {@link com.termux.app.TermuxSettings} to
 *          manage preferences of plugins as well if they don't have launcher activity themselves
 *          and also allow plugin apps to make changes to preferences from background.
 *      - Added `KEY_LOG_LEVEL` to `TERMUX_TASKER_APP` scope.
 */

/**
 * A class that defines shared constants of the Shared preferences used by Termux app and its plugins.
 * This class will be hosted by termux-app and should be imported by other termux plugin apps as is
 * instead of copying constants to random classes. The 3rd party apps can also import it for
 * interacting with termux apps. If changes are made to this file, increment the version number
 * and add an entry in the Changelog section above.
 */
public final class TermuxPreferenceConstants {

    /**
     * Termux app constants.
     */
    public static final class TERMUX_APP {

        /**
         * Defines the key for whether to show terminal toolbar containing extra keys and text input field
         */
        public static final String KEY_SHOW_TERMINAL_TOOLBAR = "show_extra_keys";
        public static final boolean DEFAULT_VALUE_SHOW_TERMINAL_TOOLBAR = true;


        /**
         * Defines the key for whether to always keep screen on
         */
        public static final String KEY_KEEP_SCREEN_ON = "screen_always_on";
        public static final boolean DEFAULT_VALUE_KEEP_SCREEN_ON = true;


        /**
         * Defines the key for font size of termux terminal view
         */
        public static final String KEY_FONTSIZE = "fontsize";


        /**
         * Defines the key for current termux terminal session
         */
        public static final String KEY_CURRENT_SESSION = "current_session";


        /**
         * Defines the key for current termux log level
         */
        public static final String KEY_LOG_LEVEL = "log_level";


        /**
         * Defines the key for whether termux terminal view key logging is enabled or not
         */
        public static final String KEY_TERMINAL_VIEW_KEY_LOGGING_ENABLED = "terminal_view_key_logging_enabled";
        public static final boolean DEFAULT_VALUE_TERMINAL_VIEW_KEY_LOGGING_ENABLED = false;

    }

    /**
     * Termux Tasker app constants.
     */
    public static final class TERMUX_TASKER_APP {

        /**
         * Defines the key for current termux log level
         */
        public static final String KEY_LOG_LEVEL = "log_level";

    }

}
