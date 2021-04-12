package com.termux.shared.settings.properties;

import com.google.common.collect.ImmutableBiMap;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.logger.Logger;

import java.io.File;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/*
 * Version: v0.6.0
 *
 * Changelog
 *
 * - 0.1.0 (2021-03-11)
 *      - Initial Release.
 *
 * - 0.2.0 (2021-03-11)
 *      - Renamed `HOME_PATH` to `TERMUX_HOME_DIR_PATH`.
 *      - Renamed `TERMUX_PROPERTIES_PRIMARY_PATH` to `TERMUX_PROPERTIES_PRIMARY_FILE_PATH`.
 *      - Renamed `TERMUX_PROPERTIES_SECONDARY_FILE_PATH` to `TERMUX_PROPERTIES_SECONDARY_FILE_PATH`.
 *
 * - 0.3.0 (2021-03-16)
 *      - Add `*TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR*`.
 *
 * - 0.4.0 (2021-03-16)
 *      - Removed `MAP_GENERIC_BOOLEAN` and `MAP_GENERIC_INVERTED_BOOLEAN`.
 *
 * - 0.5.0 (2021-03-25)
 *      - Add `KEY_HIDE_SOFT_KEYBOARD_ON_STARTUP`.
 *
 * - 0.6.0 (2021-04-07)
 *      - Updated javadocs.
 */

/**
 * A class that defines shared constants of the SharedProperties used by Termux app and its plugins.
 * This class will be hosted by termux-shared lib and should be imported by other termux plugin
 * apps as is instead of copying constants to random classes. The 3rd party apps can also import
 * it for interacting with termux apps. If changes are made to this file, increment the version number
 * and add an entry in the Changelog section above.
 *
 * The properties are loaded from the first file found at
 * {@link TermuxConstants#TERMUX_PROPERTIES_PRIMARY_FILE_PATH} or
 * {@link TermuxConstants#TERMUX_PROPERTIES_SECONDARY_FILE_PATH}
 */
public final class TermuxPropertyConstants {

    /** Defines the key for whether to use back key as the escape key */
    public static final String KEY_USE_BACK_KEY_AS_ESCAPE_KEY =  "back-key"; // Default: "back-key"

    public static final String VALUE_BACK_KEY_BEHAVIOUR_BACK = "back";
    public static final String VALUE_BACK_KEY_BEHAVIOUR_ESCAPE = "escape";



    /** Defines the key for whether to enforce character based input to fix the issue where for some devices like Samsung, the letters might not appear until enter is pressed */
    public static final String KEY_ENFORCE_CHAR_BASED_INPUT =  "enforce-char-based-input"; // Default: "enforce-char-based-input"



    /** Defines the key for whether to hide soft keyboard when termux app is started */
    public static final String KEY_HIDE_SOFT_KEYBOARD_ON_STARTUP =  "hide-soft-keyboard-on-startup"; // Default: "hide-soft-keyboard-on-startup"



    /** Defines the key for whether to use black UI */
    public static final String KEY_USE_BLACK_UI =  "use-black-ui"; // Default: "use-black-ui"



    /** Defines the key for whether to use ctrl space workaround to fix the issue where ctrl+space does not work on some ROMs */
    public static final String KEY_USE_CTRL_SPACE_WORKAROUND =  "ctrl-space-workaround"; // Default: "ctrl-space-workaround"



    /** Defines the key for whether to use fullscreen */
    public static final String KEY_USE_FULLSCREEN =  "fullscreen"; // Default: "fullscreen"



    /** Defines the key for whether to use fullscreen workaround */
    public static final String KEY_USE_FULLSCREEN_WORKAROUND =  "use-fullscreen-workaround"; // Default: "use-fullscreen-workaround"



    /** Defines the key for whether virtual volume keys are disabled */
    public static final String KEY_VIRTUAL_VOLUME_KEYS_DISABLED =  "volume-keys"; // Default: "volume-keys"

    public static final String VALUE_VOLUME_KEY_BEHAVIOUR_VOLUME = "volume";
    public static final String VALUE_VOLUME_KEY_BEHAVIOUR_VIRTUAL = "virtual";



    /** Defines the key for the bell behaviour */
    public static final String KEY_BELL_BEHAVIOUR =  "bell-character"; // Default: "bell-character"

    public static final String VALUE_BELL_BEHAVIOUR_VIBRATE = "vibrate";
    public static final String VALUE_BELL_BEHAVIOUR_BEEP = "beep";
    public static final String VALUE_BELL_BEHAVIOUR_IGNORE = "ignore";
    public static final String DEFAULT_VALUE_BELL_BEHAVIOUR = VALUE_BELL_BEHAVIOUR_VIBRATE;

    public static final int IVALUE_BELL_BEHAVIOUR_VIBRATE = 1;
    public static final int IVALUE_BELL_BEHAVIOUR_BEEP = 2;
    public static final int IVALUE_BELL_BEHAVIOUR_IGNORE = 3;
    public static final int DEFAULT_IVALUE_BELL_BEHAVIOUR = IVALUE_BELL_BEHAVIOUR_VIBRATE;

    /** Defines the bidirectional map for bell behaviour values and their internal values */
    public static final ImmutableBiMap<String, Integer> MAP_BELL_BEHAVIOUR =
        new ImmutableBiMap.Builder<String, Integer>()
            .put(VALUE_BELL_BEHAVIOUR_VIBRATE, IVALUE_BELL_BEHAVIOUR_VIBRATE)
            .put(VALUE_BELL_BEHAVIOUR_BEEP, IVALUE_BELL_BEHAVIOUR_BEEP)
            .put(VALUE_BELL_BEHAVIOUR_IGNORE, IVALUE_BELL_BEHAVIOUR_IGNORE)
            .build();



    /** Defines the key for the bell behaviour */
    public static final String KEY_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR =  "terminal-toolbar-height"; // Default: "terminal-toolbar-height"
    public static final float IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MIN = 0.4f;
    public static final float IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MAX = 3;
    public static final float DEFAULT_IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR = 1;



    /** Defines the key for create session shortcut */
    public static final String KEY_SHORTCUT_CREATE_SESSION =  "shortcut.create-session"; // Default: "shortcut.create-session"
    /** Defines the key for next session shortcut */
    public static final String KEY_SHORTCUT_NEXT_SESSION =  "shortcut.next-session"; // Default: "shortcut.next-session"
    /** Defines the key for previous session shortcut */
    public static final String KEY_SHORTCUT_PREVIOUS_SESSION =  "shortcut.previous-session"; // Default: "shortcut.previous-session"
    /** Defines the key for rename session shortcut */
    public static final String KEY_SHORTCUT_RENAME_SESSION =  "shortcut.rename-session"; // Default: "shortcut.rename-session"

    public static final int ACTION_SHORTCUT_CREATE_SESSION = 1;
    public static final int ACTION_SHORTCUT_NEXT_SESSION = 2;
    public static final int ACTION_SHORTCUT_PREVIOUS_SESSION = 3;
    public static final int ACTION_SHORTCUT_RENAME_SESSION = 4;

    /** Defines the bidirectional map for session shortcut values and their internal actions */
    public static final ImmutableBiMap<String, Integer> MAP_SESSION_SHORTCUTS =
        new ImmutableBiMap.Builder<String, Integer>()
            .put(KEY_SHORTCUT_CREATE_SESSION, ACTION_SHORTCUT_CREATE_SESSION)
            .put(KEY_SHORTCUT_NEXT_SESSION, ACTION_SHORTCUT_NEXT_SESSION)
            .put(KEY_SHORTCUT_PREVIOUS_SESSION, ACTION_SHORTCUT_PREVIOUS_SESSION)
            .put(KEY_SHORTCUT_RENAME_SESSION, ACTION_SHORTCUT_RENAME_SESSION)
            .build();



    /** Defines the key for the default working directory */
    public static final String KEY_DEFAULT_WORKING_DIRECTORY =  "default-working-directory"; // Default: "default-working-directory"
    /** Defines the default working directory */
    public static final String DEFAULT_IVALUE_DEFAULT_WORKING_DIRECTORY = TermuxConstants.TERMUX_HOME_DIR_PATH;



    /** Defines the key for extra keys */
    public static final String KEY_EXTRA_KEYS =  "extra-keys"; // Default: "extra-keys"
    /** Defines the key for extra keys style */
    public static final String KEY_EXTRA_KEYS_STYLE =  "extra-keys-style"; // Default: "extra-keys-style"
    public static final String DEFAULT_IVALUE_EXTRA_KEYS = "[[ESC, TAB, CTRL, ALT, {key: '-', popup: '|'}, DOWN, UP]]";
    public static final String DEFAULT_IVALUE_EXTRA_KEYS_STYLE = "default";





    /** Defines the set for keys loaded by termux
     * Setting this to {@code null} will make {@link SharedProperties} throw an exception.
     * */
    public static final Set<String> TERMUX_PROPERTIES_LIST = new HashSet<>(Arrays.asList(
        // boolean
        KEY_ENFORCE_CHAR_BASED_INPUT,
        KEY_HIDE_SOFT_KEYBOARD_ON_STARTUP,
        KEY_USE_BACK_KEY_AS_ESCAPE_KEY,
        KEY_USE_BLACK_UI,
        KEY_USE_CTRL_SPACE_WORKAROUND,
        KEY_USE_FULLSCREEN,
        KEY_USE_FULLSCREEN_WORKAROUND,
        KEY_VIRTUAL_VOLUME_KEYS_DISABLED,
        TermuxConstants.PROP_ALLOW_EXTERNAL_APPS,

        // int
        KEY_BELL_BEHAVIOUR,

        // float
        KEY_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR,

        // Integer
        KEY_SHORTCUT_CREATE_SESSION,
        KEY_SHORTCUT_NEXT_SESSION,
        KEY_SHORTCUT_PREVIOUS_SESSION,
        KEY_SHORTCUT_RENAME_SESSION,

        // String
        KEY_DEFAULT_WORKING_DIRECTORY,
        KEY_EXTRA_KEYS,
        KEY_EXTRA_KEYS_STYLE
    ));

    /** Defines the set for keys loaded by termux that have default boolean behaviour
     * "true" -> true
     * "false" -> false
     * default: false
     * */
    public static final Set<String> TERMUX_DEFAULT_BOOLEAN_BEHAVIOUR_PROPERTIES_LIST = new HashSet<>(Arrays.asList(
        KEY_ENFORCE_CHAR_BASED_INPUT,
        KEY_HIDE_SOFT_KEYBOARD_ON_STARTUP,
        KEY_USE_CTRL_SPACE_WORKAROUND,
        KEY_USE_FULLSCREEN,
        KEY_USE_FULLSCREEN_WORKAROUND,
        TermuxConstants.PROP_ALLOW_EXTERNAL_APPS
    ));

    /** Defines the set for keys loaded by termux that have default inverted boolean behaviour
     * "false" -> true
     * "true" -> false
     * default: true
     * */
    public static final Set<String> TERMUX_DEFAULT_INVERETED_BOOLEAN_BEHAVIOUR_PROPERTIES_LIST = new HashSet<>(Arrays.asList(
    ));




    /** Returns the first {@link File} found at
     * {@link TermuxConstants#TERMUX_PROPERTIES_PRIMARY_FILE_PATH} or
     * {@link TermuxConstants#TERMUX_PROPERTIES_SECONDARY_FILE_PATH}
     * from which termux properties can be loaded.
     * If the {@link File} found is not a regular file or is not readable then null is returned.
     *
     * @return Returns the {@link File} object for termux properties.
     */
    public static File getTermuxPropertiesFile() {
        String[] possiblePropertiesFileLocations = {
            TermuxConstants.TERMUX_PROPERTIES_PRIMARY_FILE_PATH,
            TermuxConstants.TERMUX_PROPERTIES_SECONDARY_FILE_PATH
        };

        File propertiesFile = new File(possiblePropertiesFileLocations[0]);
        int i = 0;
        while (!propertiesFile.exists() && i < possiblePropertiesFileLocations.length) {
            propertiesFile = new File(possiblePropertiesFileLocations[i]);
            i += 1;
        }

        if (propertiesFile.isFile() && propertiesFile.canRead()) {
            return propertiesFile;
        } else {
            Logger.logDebug("No readable termux.properties file found");
            return null;
        }
    }

}
