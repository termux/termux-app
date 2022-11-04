package com.termux.shared.termux.settings.preferences;

/*
 * Version: v0.16.0
 *
 * Changelog
 *
 * - 0.1.0 (2021-03-12)
 *      - Initial Release.
 *
 * - 0.2.0 (2021-03-13)
 *      - Added `KEY_LOG_LEVEL` and `KEY_TERMINAL_VIEW_LOGGING_ENABLED`.
 *
 * - 0.3.0 (2021-03-16)
 *      - Changed to per app scoping of variables so that the same file can store all constants of
 *          Termux app and its plugins. This will allow {@link com.termux.app.TermuxSettings} to
 *          manage preferences of plugins as well if they don't have launcher activity themselves
 *          and also allow plugin apps to make changes to preferences from background.
 *      - Added following to `TERMUX_TASKER_APP`:
 *           `KEY_LOG_LEVEL`.
 *
 * - 0.4.0 (2021-03-13)
 *      - Added following to `TERMUX_APP`:
 *          `KEY_PLUGIN_ERROR_NOTIFICATIONS_ENABLED` and `DEFAULT_VALUE_PLUGIN_ERROR_NOTIFICATIONS_ENABLED`.
 *
 * - 0.5.0 (2021-03-24)
 *      - Added following to `TERMUX_APP`:
 *          `KEY_LAST_NOTIFICATION_ID` and `DEFAULT_VALUE_KEY_LAST_NOTIFICATION_ID`.
 *
 * - 0.6.0 (2021-03-24)
 *      - Change `DEFAULT_VALUE_KEEP_SCREEN_ON` value to `false` in `TERMUX_APP`.
 *
 * - 0.7.0 (2021-03-27)
 *      - Added following to `TERMUX_APP`:
 *          `KEY_SOFT_KEYBOARD_ENABLED` and `DEFAULT_VALUE_KEY_SOFT_KEYBOARD_ENABLED`.
 *
 * - 0.8.0 (2021-04-06)
 *      - Added following to `TERMUX_APP`:
 *          `KEY_CRASH_REPORT_NOTIFICATIONS_ENABLED` and `DEFAULT_VALUE_CRASH_REPORT_NOTIFICATIONS_ENABLED`.
 *
 * - 0.9.0 (2021-04-07)
 *      - Updated javadocs.
 *
 * - 0.10.0 (2021-05-12)
 *      - Added following to `TERMUX_APP`:
 *          `KEY_SOFT_KEYBOARD_ENABLED_ONLY_IF_NO_HARDWARE` and `DEFAULT_VALUE_KEY_SOFT_KEYBOARD_ENABLED_ONLY_IF_NO_HARDWARE`.
 *
 * - 0.11.0 (2021-07-08)
 *      - Added following to `TERMUX_APP`:
 *          `KEY_DISABLE_TERMINAL_MARGIN_ADJUSTMENT`.
 *
 * - 0.12.0 (2021-08-27)
 *      - Added `TERMUX_API_APP.KEY_LOG_LEVEL`, `TERMUX_BOOT_APP.KEY_LOG_LEVEL`,
 *          `TERMUX_FLOAT_APP.KEY_LOG_LEVEL`, `TERMUX_STYLING_APP.KEY_LOG_LEVEL`,
 *          `TERMUX_Widget_APP.KEY_LOG_LEVEL`.
 *
 * - 0.13.0 (2021-09-02)
 *      - Added following to `TERMUX_FLOAT_APP`:
 *          `KEY_WINDOW_X`, `KEY_WINDOW_Y`, `KEY_WINDOW_WIDTH`, `KEY_WINDOW_HEIGHT`, `KEY_FONTSIZE`,
 *          `KEY_TERMINAL_VIEW_KEY_LOGGING_ENABLED`.
 *
 * - 0.14.0 (2021-09-04)
 *      - Added `TERMUX_WIDGET_APP.KEY_TOKEN`.
 *
 * - 0.15.0 (2021-09-05)
 *      - Added following to `TERMUX_TASKER_APP`:
 *          `KEY_LAST_PENDING_INTENT_REQUEST_CODE` and `DEFAULT_VALUE_KEY_LAST_PENDING_INTENT_REQUEST_CODE`.
 *
 * - 0.16.0 (2022-06-11)
 *      - Added following to `TERMUX_APP`:
 *          `KEY_APP_SHELL_NUMBER_SINCE_BOOT` and `KEY_TERMINAL_SESSION_NUMBER_SINCE_BOOT`.
 *
 * - 0.17.0 (2022-11-04)
 *      - Added following to `TERMUX_APP`:
 *          `KEY_BACKGROUND_IMAGE_ENABLED` and `DEFAULT_VALUE_BACKGROUND_IMAGE_ENABLED`.
 */

import com.termux.shared.shell.command.ExecutionCommand;

/**
 * A class that defines shared constants of the SharedPreferences used by Termux app and its plugins.
 * This class will be hosted by termux-shared lib and should be imported by other termux plugin
 * apps as is instead of copying constants to random classes. The 3rd party apps can also import
 * it for interacting with termux apps. If changes are made to this file, increment the version number
 * and add an entry in the Changelog section above.
 */
public final class TermuxPreferenceConstants {

    /**
     * Termux app constants.
     */
    public static final class TERMUX_APP {

        /**
         * Defines the key for whether terminal view margin adjustment that is done to prevent soft
         * keyboard from covering bottom part of terminal view on some devices is enabled or not.
         * Margin adjustment may cause screen flickering on some devices and so should be disabled.
         */
        public static final String KEY_TERMINAL_MARGIN_ADJUSTMENT =  "terminal_margin_adjustment";
        public static final boolean DEFAULT_TERMINAL_MARGIN_ADJUSTMENT = true;


        /**
         * Defines the key for whether to show terminal toolbar containing extra keys and text input field.
         */
        public static final String KEY_SHOW_TERMINAL_TOOLBAR = "show_extra_keys";
        public static final boolean DEFAULT_VALUE_SHOW_TERMINAL_TOOLBAR = true;


        /**
         * Defines the key for whether the soft keyboard will be enabled, for cases where users want
         * to use a hardware keyboard instead.
         */
        public static final String KEY_SOFT_KEYBOARD_ENABLED = "soft_keyboard_enabled";
        public static final boolean DEFAULT_VALUE_KEY_SOFT_KEYBOARD_ENABLED = true;

        /**
         * Defines the key for whether the soft keyboard will be enabled only if no hardware keyboard
         * attached, for cases where users want to use a hardware keyboard instead.
         */
        public static final String KEY_SOFT_KEYBOARD_ENABLED_ONLY_IF_NO_HARDWARE = "soft_keyboard_enabled_only_if_no_hardware";
        public static final boolean DEFAULT_VALUE_KEY_SOFT_KEYBOARD_ENABLED_ONLY_IF_NO_HARDWARE = false;


        /**
         * Defines the key for whether to always keep screen on.
         */
        public static final String KEY_KEEP_SCREEN_ON = "screen_always_on";
        public static final boolean DEFAULT_VALUE_KEEP_SCREEN_ON = false;


        /**
         * Defines the key for font size of termux terminal view.
         */
        public static final String KEY_FONTSIZE = "fontsize";


        /**
         * Defines the key for current termux terminal session.
         */
        public static final String KEY_CURRENT_SESSION = "current_session";


        /**
         * Defines the key for current log level.
         */
        public static final String KEY_LOG_LEVEL = "log_level";


        /**
         * Defines the key for last used notification id.
         */
        public static final String KEY_LAST_NOTIFICATION_ID = "last_notification_id";
        public static final int DEFAULT_VALUE_KEY_LAST_NOTIFICATION_ID = 0;

        /**
         * The {@link ExecutionCommand.Runner#APP_SHELL} number after termux app process since boot.
         */
        public static final String KEY_APP_SHELL_NUMBER_SINCE_BOOT = "app_shell_number_since_boot";
        public static final int DEFAULT_VALUE_APP_SHELL_NUMBER_SINCE_BOOT = 0;

        /**
         * The {@link ExecutionCommand.Runner#TERMINAL_SESSION} number after termux app process since boot.
         */
        public static final String KEY_TERMINAL_SESSION_NUMBER_SINCE_BOOT = "terminal_session_number_since_boot";
        public static final int DEFAULT_VALUE_TERMINAL_SESSION_NUMBER_SINCE_BOOT = 0;


        /**
         * Defines the key for whether termux terminal view key logging is enabled or not
         */
        public static final String KEY_TERMINAL_VIEW_KEY_LOGGING_ENABLED = "terminal_view_key_logging_enabled";
        public static final boolean DEFAULT_VALUE_TERMINAL_VIEW_KEY_LOGGING_ENABLED = false;

        /**
         * Defines the key for whether flashes and notifications for plugin errors are enabled or not.
         */
        public static final String KEY_PLUGIN_ERROR_NOTIFICATIONS_ENABLED = "plugin_error_notifications_enabled";
        public static final boolean DEFAULT_VALUE_PLUGIN_ERROR_NOTIFICATIONS_ENABLED = true;

        /**
         * Defines the key for whether notifications for crash reports are enabled or not.
         */
        public static final String KEY_CRASH_REPORT_NOTIFICATIONS_ENABLED = "crash_report_notifications_enabled";
        public static final boolean DEFAULT_VALUE_CRASH_REPORT_NOTIFICATIONS_ENABLED = true;

        /**
         * Defines the key for whether background image is enabled or not.
         */
        public static final String KEY_BACKGROUND_IMAGE_ENABLED = "background_image_enabled";
        public static final boolean DEFAULT_VALUE_BACKGROUND_IMAGE_ENABLED = false;

    }



    /**
     * Termux:API app constants.
     */
    public static final class TERMUX_API_APP {

        /**
         * Defines the key for current log level.
         */
        public static final String KEY_LOG_LEVEL = "log_level";


        /**
         * Defines the key for last used PendingIntent request code.
         */
        public static final String KEY_LAST_PENDING_INTENT_REQUEST_CODE = "last_pending_intent_request_code";
        public static final int DEFAULT_VALUE_KEY_LAST_PENDING_INTENT_REQUEST_CODE = 0;

    }



    /**
     * Termux:Boot app constants.
     */
    public static final class TERMUX_BOOT_APP {

        /**
         * Defines the key for current log level.
         */
        public static final String KEY_LOG_LEVEL = "log_level";

    }



    /**
     * Termux:Float app constants.
     */
    public static final class TERMUX_FLOAT_APP {

        /**
         * The float window x coordinate.
         */
        public static final String KEY_WINDOW_X = "window_x";

        /**
         * The float window y coordinate.
         */
        public static final String KEY_WINDOW_Y = "window_y";

        /**
         * The float window width.
         */
        public static final String KEY_WINDOW_WIDTH = "window_width";

        /**
         * The float window height.
         */
        public static final String KEY_WINDOW_HEIGHT = "window_height";

        /**
         * Defines the key for font size of termux terminal view.
         */
        public static final String KEY_FONTSIZE = "fontsize";

        /**
         * Defines the key for current log level.
         */
        public static final String KEY_LOG_LEVEL = "log_level";

        /**
         * Defines the key for whether termux terminal view key logging is enabled or not
         */
        public static final String KEY_TERMINAL_VIEW_KEY_LOGGING_ENABLED = "terminal_view_key_logging_enabled";
        public static final boolean DEFAULT_VALUE_TERMINAL_VIEW_KEY_LOGGING_ENABLED = false;

    }



    /**
     * Termux:Styling app constants.
     */
    public static final class TERMUX_STYLING_APP {

        /**
         * Defines the key for current log level.
         */
        public static final String KEY_LOG_LEVEL = "log_level";

    }



    /**
     * Termux:Tasker app constants.
     */
    public static final class TERMUX_TASKER_APP {

        /**
         * Defines the key for current log level.
         */
        public static final String KEY_LOG_LEVEL = "log_level";


        /**
         * Defines the key for last used PendingIntent request code.
         */
        public static final String KEY_LAST_PENDING_INTENT_REQUEST_CODE = "last_pending_intent_request_code";
        public static final int DEFAULT_VALUE_KEY_LAST_PENDING_INTENT_REQUEST_CODE = 0;

    }



    /**
     * Termux:Widget app constants.
     */
    public static final class TERMUX_WIDGET_APP {

        /**
         * Defines the key for current log level.
         */
        public static final String KEY_LOG_LEVEL = "log_level";

        /**
         * Defines the key for current token for shortcuts.
         */
        public static final String KEY_TOKEN = "token";

    }

}
