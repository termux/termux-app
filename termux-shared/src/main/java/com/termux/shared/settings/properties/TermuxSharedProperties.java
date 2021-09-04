package com.termux.shared.settings.properties;

import android.content.Context;
import android.content.res.Configuration;

import androidx.annotation.NonNull;

import com.termux.shared.logger.Logger;
import com.termux.shared.data.DataUtils;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;
import java.util.Set;

public abstract class TermuxSharedProperties {

    protected final Context mContext;
    protected final String mLabel;
    protected final File mPropertiesFile;
    protected final SharedProperties mSharedProperties;

    public static final String LOG_TAG = "TermuxSharedProperties";

    public TermuxSharedProperties(@NonNull Context context, @NonNull String label, File propertiesFile,
                                  @NonNull Set<String> propertiesList, @NonNull SharedPropertiesParser sharedPropertiesParser) {
        mContext = context;
        mLabel = label;
        mPropertiesFile = propertiesFile;
        mSharedProperties = new SharedProperties(context, mPropertiesFile, propertiesList, sharedPropertiesParser);
        loadTermuxPropertiesFromDisk();
    }

    /**
     * Reload the termux properties from disk into an in-memory cache.
     */
    public void loadTermuxPropertiesFromDisk() {
        mSharedProperties.loadPropertiesFromDisk();
        dumpPropertiesToLog();
        dumpInternalPropertiesToLog();
    }





    /**
     * Get the {@link Properties} from the {@link #mPropertiesFile} file.
     *
     * @param cached If {@code true}, then the {@link Properties} in-memory cache is returned.
     *               Otherwise the {@link Properties} object is read directly from the
     *               {@link #mPropertiesFile} file.
     * @return Returns the {@link Properties} object. It will be {@code null} if an exception is
     * raised while reading the file.
     */
    public Properties getProperties(boolean cached) {
        return mSharedProperties.getProperties(cached);
    }

    /**
     * Get the {@link String} value for the key passed from the {@link #mPropertiesFile} file.
     *
     * @param key The key to read.
     * @param def The default value.
     * @param cached If {@code true}, then the value is returned from the the {@link Properties} in-memory cache.
     *               Otherwise the {@link Properties} object is read directly from the file
     *               and value is returned from it against the key.
     * @return Returns the {@link String} object. This will be {@code null} if key is not found.
     */
    public String getPropertyValue(String key, String def, boolean cached) {
        return SharedProperties.getDefaultIfNull(mSharedProperties.getProperty(key, cached), def);
    }

    /**
     * A function to check if the value is {@code true} for {@link Properties} key read from
     * the {@link #mPropertiesFile} file.
     *
     * @param key The key to read.
     * @param cached If {@code true}, then the value is checked from the the {@link Properties} in-memory cache.
     *               Otherwise the {@link Properties} object is read directly from the file
     *               and value is checked from it.
     * @param logErrorOnInvalidValue If {@code true}, then an error will be logged if key value
     *                               was found in {@link Properties} but was invalid.
     * @return Returns the {@code true} if the {@link Properties} key {@link String} value equals "true",
     * regardless of case. If the key does not exist in the file or does not equal "true", then
     * {@code false} will be returned.
     */
    public boolean isPropertyValueTrue(String key, boolean cached, boolean logErrorOnInvalidValue) {
        return (boolean) SharedProperties.getBooleanValueForStringValue(key, (String) getPropertyValue(key, null, cached), false, logErrorOnInvalidValue, LOG_TAG);
    }

    /**
     * A function to check if the value is {@code false} for {@link Properties} key read from
     * the {@link #mPropertiesFile} file.
     *
     * @param key The key to read.
     * @param cached If {@code true}, then the value is checked from the the {@link Properties} in-memory cache.
     *               Otherwise the {@link Properties} object is read directly from the file
     *               and value is checked from it.
     * @param logErrorOnInvalidValue If {@code true}, then an error will be logged if key value
     *                               was found in {@link Properties} but was invalid.
     * @return Returns {@code true} if the {@link Properties} key {@link String} value equals "false",
     * regardless of case. If the key does not exist in the file or does not equal "false", then
     * {@code true} will be returned.
     */
    public boolean isPropertyValueFalse(String key, boolean cached, boolean logErrorOnInvalidValue) {
        return (boolean) SharedProperties.getInvertedBooleanValueForStringValue(key, (String) getPropertyValue(key, null, cached), true, logErrorOnInvalidValue, LOG_TAG);
    }





    /**
     * Get the internal value {@link Object} {@link HashMap <>} in-memory cache for the
     * {@link #mPropertiesFile} file. A call to {@link #loadTermuxPropertiesFromDisk()} must be made
     * before this.
     *
     * @return Returns a copy of {@link Map} object.
     */
    public Map<String, Object> getInternalProperties() {
        return mSharedProperties.getInternalProperties();
    }

    /**
     * Get the internal {@link Object} value for the key passed from the {@link #mPropertiesFile} file.
     * If cache is {@code true}, then value is returned from the {@link HashMap <>} in-memory cache,
     * so a call to {@link #loadTermuxPropertiesFromDisk()} must be made before this.
     *
     * @param key The key to read from the {@link HashMap<>} in-memory cache.
     * @param cached If {@code true}, then the value is returned from the the {@link HashMap <>} in-memory cache,
     *               but if the value is null, then an attempt is made to return the default value.
     *               If {@code false}, then the {@link Properties} object is read directly from the file
     *               and internal value is returned for the property value against the key.
     * @return Returns the {@link Object} object. This will be {@code null} if key is not found or
     * the object stored against the key is {@code null}.
     */
    public Object getInternalPropertyValue(String key, boolean cached) {
        Object value;
        if (cached) {
            value = mSharedProperties.getInternalProperty(key);
            // If the value is not null since key was found or if the value was null since the
            // object stored for the key was itself null, we detect the later by checking if the key
            // exists in the map.
            if (value != null || mSharedProperties.getInternalProperties().containsKey(key)) {
                return value;
            } else {
                // This should not happen normally unless mMap was modified after the
                // {@link #loadTermuxPropertiesFromDisk()} call
                // A null value can still be returned by
                // {@link #getInternalPropertyValueFromValue(Context,String,String)} for some keys
                value = getInternalTermuxPropertyValueFromValue(mContext, key, null);
                Logger.logWarn(LOG_TAG, "The value for \"" + key + "\" not found in SharedProperties cache, force returning default value: `" + value +  "`");
                return value;
            }
        } else {
            // We get the property value directly from file and return its internal value
            return getInternalTermuxPropertyValueFromValue(mContext, key, mSharedProperties.getProperty(key, false));
        }
    }





    /**
     * Get the internal {@link Object} value for the key passed from the file returned by
     * {@code propertiesFile}. The {@link Properties} object is
     * read directly from the file and internal value is returned for the property value against the key.
     *
     * @param context The context for operations.
     * @param key The key for which the internal object is required.
     * @return Returns the {@link Object} object. This will be {@code null} if key is not found or
     * the object stored against the key is {@code null}.
     */
    public static Object getInternalPropertyValue(Context context, File propertiesFile, String key,
                                                  @NonNull SharedPropertiesParser sharedPropertiesParser) {
        return SharedProperties.getInternalProperty(context, propertiesFile, key, sharedPropertiesParser);
    }

    /**
     * The class that implements the {@link SharedPropertiesParser} interface.
     */
    public static class SharedPropertiesParserClient implements SharedPropertiesParser {
        /**
         * Override the
         * {@link SharedPropertiesParser#getInternalPropertyValueFromValue(Context,String,String)}
         * interface function.
         */
        @Override
        public Object getInternalPropertyValueFromValue(Context context, String key, String value) {
            return getInternalTermuxPropertyValueFromValue(context, key, value);
        }
    }

    /**
     * A static function that should return the internal termux {@link Object} for a key/value pair
     * read from properties file.
     *
     * @param context The context for operations.
     * @param key The key for which the internal object is required.
     * @param value The literal value for the property found is the properties file.
     * @return Returns the internal termux {@link Object} object.
     */
    public static Object getInternalTermuxPropertyValueFromValue(Context context, String key, String value) {
        if (key == null) return null;
        /*
          For keys where a MAP_* is checked by respective functions. Note that value to this function
          would actually be the key for the MAP_*:
          - If the value is currently null, then searching MAP_* should also return null and internal default value will be used.
          - If the value is not null and does not exist in MAP_*, then internal default value will be used.
          - If the value is not null and does exist in MAP_*, then internal value returned by map will be used.
         */
        switch (key) {
            /* boolean */
            case TermuxPropertyConstants.KEY_USE_BLACK_UI:
                return (boolean) getUseBlackUIInternalPropertyValueFromValue(context, value);

            /* int */
            case TermuxPropertyConstants.KEY_BELL_BEHAVIOUR:
                return (int) getBellBehaviourInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_TERMINAL_CURSOR_BLINK_RATE:
                return (int) getTerminalCursorBlinkRateInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_TERMINAL_CURSOR_STYLE:
                return (int) getTerminalCursorStyleInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_TERMINAL_MARGIN_HORIZONTAL:
                return (int) getTerminalMarginHorizontalInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_TERMINAL_MARGIN_VERTICAL:
                return (int) getTerminalMarginVerticalInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_TERMINAL_TRANSCRIPT_ROWS:
                return (int) getTerminalTranscriptRowsInternalPropertyValueFromValue(value);

            /* float */
            case TermuxPropertyConstants.KEY_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR:
                return (float) getTerminalToolbarHeightScaleFactorInternalPropertyValueFromValue(value);

            /* Integer (may be null) */
            case TermuxPropertyConstants.KEY_SHORTCUT_CREATE_SESSION:
            case TermuxPropertyConstants.KEY_SHORTCUT_NEXT_SESSION:
            case TermuxPropertyConstants.KEY_SHORTCUT_PREVIOUS_SESSION:
            case TermuxPropertyConstants.KEY_SHORTCUT_RENAME_SESSION:
                return (Integer) getCodePointForSessionShortcuts(key, value);

            /* String (may be null) */
            case TermuxPropertyConstants.KEY_BACK_KEY_BEHAVIOUR:
                return (String) getBackKeyBehaviourInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_DEFAULT_WORKING_DIRECTORY:
                return (String) getDefaultWorkingDirectoryInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_EXTRA_KEYS:
                return (String) getExtraKeysInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_EXTRA_KEYS_STYLE:
                return (String) getExtraKeysStyleInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_SOFT_KEYBOARD_TOGGLE_BEHAVIOUR:
                return (String) getSoftKeyboardToggleBehaviourInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_VOLUME_KEYS_BEHAVIOUR:
                return (String) getVolumeKeysBehaviourInternalPropertyValueFromValue(value);

            default:
                // default false boolean behaviour
                if (TermuxPropertyConstants.TERMUX_DEFAULT_FALSE_BOOLEAN_BEHAVIOUR_PROPERTIES_LIST.contains(key))
                    return (boolean) SharedProperties.getBooleanValueForStringValue(key, value, false, true, LOG_TAG);
                // default true boolean behaviour
                if (TermuxPropertyConstants.TERMUX_DEFAULT_TRUE_BOOLEAN_BEHAVIOUR_PROPERTIES_LIST.contains(key))
                    return (boolean) SharedProperties.getBooleanValueForStringValue(key, value, true, true, LOG_TAG);
                // default inverted false boolean behaviour
                //else if (TermuxPropertyConstants.TERMUX_DEFAULT_INVERETED_FALSE_BOOLEAN_BEHAVIOUR_PROPERTIES_LIST.contains(key))
                //    return (boolean) SharedProperties.getInvertedBooleanValueForStringValue(key, value, false, true, LOG_TAG);
                // default inverted true boolean behaviour
                // else if (TermuxPropertyConstants.TERMUX_DEFAULT_INVERETED_TRUE_BOOLEAN_BEHAVIOUR_PROPERTIES_LIST.contains(key))
                //    return (boolean) SharedProperties.getInvertedBooleanValueForStringValue(key, value, true, true, LOG_TAG);
                // just use String object as is (may be null)
                else
                    return value;
        }
    }





    /**
     * Returns {@code true} or {@code false} if value is the literal string "true" or "false" respectively regardless of case.
     * Otherwise returns {@code true} if the night mode is currently enabled in the system.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static boolean getUseBlackUIInternalPropertyValueFromValue(Context context, String value) {
        int nightMode = context.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK;
        return SharedProperties.getBooleanValueForStringValue(TermuxPropertyConstants.KEY_USE_BLACK_UI, value, nightMode == Configuration.UI_MODE_NIGHT_YES, true, LOG_TAG);
    }

    /**
     * Returns the internal value after mapping it based on
     * {@code TermuxPropertyConstants#MAP_BELL_BEHAVIOUR} if the value is not {@code null}
     * and is valid, otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_BELL_BEHAVIOUR}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static int getBellBehaviourInternalPropertyValueFromValue(String value) {
        return (int) SharedProperties.getDefaultIfNotInMap(TermuxPropertyConstants.KEY_BELL_BEHAVIOUR, TermuxPropertyConstants.MAP_BELL_BEHAVIOUR, SharedProperties.toLowerCase(value), TermuxPropertyConstants.DEFAULT_IVALUE_BELL_BEHAVIOUR, true, LOG_TAG);
    }

    /**
     * Returns the int for the value if its not null and is between
     * {@link TermuxPropertyConstants#IVALUE_TERMINAL_CURSOR_BLINK_RATE_MIN} and
     * {@link TermuxPropertyConstants#IVALUE_TERMINAL_CURSOR_BLINK_RATE_MAX},
     * otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_TERMINAL_CURSOR_BLINK_RATE}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static int getTerminalCursorBlinkRateInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNotInRange(TermuxPropertyConstants.KEY_TERMINAL_CURSOR_BLINK_RATE,
                    DataUtils.getIntFromString(value, TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_CURSOR_BLINK_RATE),
                    TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_CURSOR_BLINK_RATE,
                    TermuxPropertyConstants.IVALUE_TERMINAL_CURSOR_BLINK_RATE_MIN,
                    TermuxPropertyConstants.IVALUE_TERMINAL_CURSOR_BLINK_RATE_MAX,
                    true, true, LOG_TAG);
    }

    /**
     * Returns the internal value after mapping it based on
     * {@link TermuxPropertyConstants#MAP_TERMINAL_CURSOR_STYLE} if the value is not {@code null}
     * and is valid, otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_TERMINAL_CURSOR_STYLE}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static int getTerminalCursorStyleInternalPropertyValueFromValue(String value) {
        return (int) SharedProperties.getDefaultIfNotInMap(TermuxPropertyConstants.KEY_TERMINAL_CURSOR_STYLE, TermuxPropertyConstants.MAP_TERMINAL_CURSOR_STYLE, SharedProperties.toLowerCase(value), TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_CURSOR_STYLE, true, LOG_TAG);
    }

    /**
     * Returns the int for the value if its not null and is between
     * {@link TermuxPropertyConstants#IVALUE_TERMINAL_MARGIN_HORIZONTAL_MIN} and
     * {@link TermuxPropertyConstants#IVALUE_TERMINAL_MARGIN_HORIZONTAL_MAX},
     * otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_TERMINAL_HORIZONTAL_MARGIN}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static int getTerminalMarginHorizontalInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNotInRange(TermuxPropertyConstants.KEY_TERMINAL_MARGIN_HORIZONTAL,
            DataUtils.getIntFromString(value, TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_HORIZONTAL_MARGIN),
            TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_HORIZONTAL_MARGIN,
            TermuxPropertyConstants.IVALUE_TERMINAL_MARGIN_HORIZONTAL_MIN,
            TermuxPropertyConstants.IVALUE_TERMINAL_MARGIN_HORIZONTAL_MAX,
            true, true, LOG_TAG);
    }

    /**
     * Returns the int for the value if its not null and is between
     * {@link TermuxPropertyConstants#IVALUE_TERMINAL_MARGIN_VERTICAL_MIN} and
     * {@link TermuxPropertyConstants#IVALUE_TERMINAL_MARGIN_VERTICAL_MAX},
     * otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_TERMINAL_VERTICAL_MARGIN}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static int getTerminalMarginVerticalInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNotInRange(TermuxPropertyConstants.KEY_TERMINAL_MARGIN_VERTICAL,
            DataUtils.getIntFromString(value, TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_VERTICAL_MARGIN),
            TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_VERTICAL_MARGIN,
            TermuxPropertyConstants.IVALUE_TERMINAL_MARGIN_VERTICAL_MIN,
            TermuxPropertyConstants.IVALUE_TERMINAL_MARGIN_VERTICAL_MAX,
            true, true, LOG_TAG);
    }

    /**
     * Returns the int for the value if its not null and is between
     * {@link TermuxPropertyConstants#IVALUE_TERMINAL_TRANSCRIPT_ROWS_MIN} and
     * {@link TermuxPropertyConstants#IVALUE_TERMINAL_TRANSCRIPT_ROWS_MAX},
     * otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_TERMINAL_TRANSCRIPT_ROWS}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static int getTerminalTranscriptRowsInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNotInRange(TermuxPropertyConstants.KEY_TERMINAL_TRANSCRIPT_ROWS,
            DataUtils.getIntFromString(value, TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_TRANSCRIPT_ROWS),
            TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_TRANSCRIPT_ROWS,
            TermuxPropertyConstants.IVALUE_TERMINAL_TRANSCRIPT_ROWS_MIN,
            TermuxPropertyConstants.IVALUE_TERMINAL_TRANSCRIPT_ROWS_MAX,
            true, true, LOG_TAG);
    }

    /**
     * Returns the int for the value if its not null and is between
     * {@link TermuxPropertyConstants#IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MIN} and
     * {@link TermuxPropertyConstants#IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MAX},
     * otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static float getTerminalToolbarHeightScaleFactorInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNotInRange(TermuxPropertyConstants.KEY_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR,
            DataUtils.getFloatFromString(value, TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR),
            TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR,
            TermuxPropertyConstants.IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MIN,
            TermuxPropertyConstants.IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MAX,
            true, true, LOG_TAG);
    }

    /**
     * Returns the code point for the value if key is not {@code null} and value is not {@code null} and is valid,
     * otherwise returns {@code null}.
     *
     * @param key The key for session shortcut.
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static Integer getCodePointForSessionShortcuts(String key, String value) {
        if (key == null) return null;
        if (value == null) return null;
        String[] parts = value.toLowerCase().trim().split("\\+");
        String input = parts.length == 2 ? parts[1].trim() : null;
        if (!(parts.length == 2 && parts[0].trim().equals("ctrl")) || input.isEmpty() || input.length() > 2) {
            Logger.logError(LOG_TAG, "Keyboard shortcut '" + key + "' is not Ctrl+<something>");
            return null;
        }

        char c = input.charAt(0);
        int codePoint = c;
        if (Character.isLowSurrogate(c)) {
            if (input.length() != 2 || Character.isHighSurrogate(input.charAt(1))) {
                Logger.logError(LOG_TAG, "Keyboard shortcut '" + key + "' is not Ctrl+<something>");
                return null;
            } else {
                codePoint = Character.toCodePoint(input.charAt(1), c);
            }
        }

        return codePoint;
    }

    /**
     * Returns the value itself if it is not {@code null}, otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_BACK_KEY_BEHAVIOUR}.
     *
     * @param value {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static String getBackKeyBehaviourInternalPropertyValueFromValue(String value) {
        return (String) SharedProperties.getDefaultIfNotInMap(TermuxPropertyConstants.KEY_BACK_KEY_BEHAVIOUR, TermuxPropertyConstants.MAP_BACK_KEY_BEHAVIOUR, SharedProperties.toLowerCase(value), TermuxPropertyConstants.DEFAULT_IVALUE_BACK_KEY_BEHAVIOUR, true, LOG_TAG);
    }

    /**
     * Returns the path itself if a directory exists at it and is readable, otherwise returns
     *  {@link TermuxPropertyConstants#DEFAULT_IVALUE_DEFAULT_WORKING_DIRECTORY}.
     *
     * @param path The {@link String} path to check.
     * @return Returns the internal value for value.
     */
    public static String getDefaultWorkingDirectoryInternalPropertyValueFromValue(String path) {
        if (path == null || path.isEmpty()) return TermuxPropertyConstants.DEFAULT_IVALUE_DEFAULT_WORKING_DIRECTORY;
        File workDir = new File(path);
        if (!workDir.exists() || !workDir.isDirectory() || !workDir.canRead()) {
            // Fallback to default directory if user configured working directory does not exist,
            // is not a directory or is not readable.
            Logger.logError(LOG_TAG, "The path \"" + path + "\" for the key \"" + TermuxPropertyConstants.KEY_DEFAULT_WORKING_DIRECTORY + "\" does not exist, is not a directory or is not readable. Using default value \"" + TermuxPropertyConstants.DEFAULT_IVALUE_DEFAULT_WORKING_DIRECTORY + "\" instead.");
            return TermuxPropertyConstants.DEFAULT_IVALUE_DEFAULT_WORKING_DIRECTORY;
        } else {
            return path;
        }
    }

    /**
     * Returns the value itself if it is not {@code null}, otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_EXTRA_KEYS}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static String getExtraKeysInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNullOrEmpty(value, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS);
    }

    /**
     * Returns the value itself if it is not {@code null}, otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_EXTRA_KEYS_STYLE}.
     *
     * @param value {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static String getExtraKeysStyleInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNullOrEmpty(value, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE);
    }

    /**
     * Returns the value itself if it is not {@code null}, otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_SOFT_KEYBOARD_TOGGLE_BEHAVIOUR}.
     *
     * @param value {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static String getSoftKeyboardToggleBehaviourInternalPropertyValueFromValue(String value) {
        return (String) SharedProperties.getDefaultIfNotInMap(TermuxPropertyConstants.KEY_SOFT_KEYBOARD_TOGGLE_BEHAVIOUR, TermuxPropertyConstants.MAP_SOFT_KEYBOARD_TOGGLE_BEHAVIOUR, SharedProperties.toLowerCase(value), TermuxPropertyConstants.DEFAULT_IVALUE_SOFT_KEYBOARD_TOGGLE_BEHAVIOUR, true, LOG_TAG);
    }

    /**
     * Returns the value itself if it is not {@code null}, otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_VOLUME_KEYS_BEHAVIOUR}.
     *
     * @param value {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static String getVolumeKeysBehaviourInternalPropertyValueFromValue(String value) {
        return (String) SharedProperties.getDefaultIfNotInMap(TermuxPropertyConstants.KEY_VOLUME_KEYS_BEHAVIOUR, TermuxPropertyConstants.MAP_VOLUME_KEYS_BEHAVIOUR, SharedProperties.toLowerCase(value), TermuxPropertyConstants.DEFAULT_IVALUE_VOLUME_KEYS_BEHAVIOUR, true, LOG_TAG);
    }





    public boolean areHardwareKeyboardShortcutsDisabled() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_DISABLE_TERMINAL_SESSION_CHANGE_TOAST, true);
    }

    public boolean areTerminalSessionChangeToastsDisabled() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_DISABLE_TERMINAL_SESSION_CHANGE_TOAST, true);
    }

    public boolean isEnforcingCharBasedInput() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_ENFORCE_CHAR_BASED_INPUT, true);
    }

    public boolean shouldExtraKeysTextBeAllCaps() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_EXTRA_KEYS_TEXT_ALL_CAPS, true);
    }

    public boolean shouldSoftKeyboardBeHiddenOnStartup() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_HIDE_SOFT_KEYBOARD_ON_STARTUP, true);
    }

    public boolean shouldOpenTerminalTranscriptURLOnClick() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_TERMINAL_ONCLICK_URL_OPEN, true);
    }

    public boolean isUsingBlackUI() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_USE_BLACK_UI, true);
    }

    public boolean isUsingCtrlSpaceWorkaround() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_USE_CTRL_SPACE_WORKAROUND, true);
    }

    public boolean isUsingFullScreen() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_USE_FULLSCREEN, true);
    }

    public boolean isUsingFullScreenWorkAround() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_USE_FULLSCREEN_WORKAROUND, true);
    }

    public int getBellBehaviour() {
        return (int) getInternalPropertyValue(TermuxPropertyConstants.KEY_BELL_BEHAVIOUR, true);
    }

    public int getTerminalCursorBlinkRate() {
        return (int) getInternalPropertyValue(TermuxPropertyConstants.KEY_TERMINAL_CURSOR_BLINK_RATE, true);
    }

    public int getTerminalCursorStyle() {
        return (int) getInternalPropertyValue(TermuxPropertyConstants.KEY_TERMINAL_CURSOR_STYLE, true);
    }

    public int getTerminalMarginHorizontal() {
        return (int) getInternalPropertyValue(TermuxPropertyConstants.KEY_TERMINAL_MARGIN_HORIZONTAL, true);
    }

    public int getTerminalMarginVertical() {
        return (int) getInternalPropertyValue(TermuxPropertyConstants.KEY_TERMINAL_MARGIN_VERTICAL, true);
    }

    public int getTerminalTranscriptRows() {
        return (int) getInternalPropertyValue(TermuxPropertyConstants.KEY_TERMINAL_TRANSCRIPT_ROWS, true);
    }

    public float getTerminalToolbarHeightScaleFactor() {
        return (float) getInternalPropertyValue(TermuxPropertyConstants.KEY_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR, true);
    }

    public boolean isBackKeyTheEscapeKey() {
        return (boolean) TermuxPropertyConstants.IVALUE_BACK_KEY_BEHAVIOUR_ESCAPE.equals(getInternalPropertyValue(TermuxPropertyConstants.KEY_BACK_KEY_BEHAVIOUR, true));
    }

    public String getDefaultWorkingDirectory() {
        return (String) getInternalPropertyValue(TermuxPropertyConstants.KEY_DEFAULT_WORKING_DIRECTORY, true);
    }

    public boolean shouldEnableDisableSoftKeyboardOnToggle() {
        return (boolean) TermuxPropertyConstants.IVALUE_SOFT_KEYBOARD_TOGGLE_BEHAVIOUR_ENABLE_DISABLE.equals(getInternalPropertyValue(TermuxPropertyConstants.KEY_SOFT_KEYBOARD_TOGGLE_BEHAVIOUR, true));
    }

    public boolean areVirtualVolumeKeysDisabled() {
        return (boolean) TermuxPropertyConstants.IVALUE_VOLUME_KEY_BEHAVIOUR_VOLUME.equals(getInternalPropertyValue(TermuxPropertyConstants.KEY_VOLUME_KEYS_BEHAVIOUR, true));
    }





    public void dumpPropertiesToLog() {
        Properties properties = getProperties(true);
        StringBuilder propertiesDump = new StringBuilder();

        propertiesDump.append(mLabel).append(" Termux Properties:");
        if (properties != null) {
            for (String key : properties.stringPropertyNames()) {
                propertiesDump.append("\n").append(key).append(": `").append(properties.get(key)).append("`");
            }
        } else {
            propertiesDump.append(" null");
        }

        Logger.logVerbose(LOG_TAG, propertiesDump.toString());
    }

    public void dumpInternalPropertiesToLog() {
        HashMap<String, Object> internalProperties = (HashMap<String, Object>) getInternalProperties();
        StringBuilder internalPropertiesDump = new StringBuilder();

        internalPropertiesDump.append(mLabel).append(" Internal Properties:");
        if (internalProperties != null) {
            for (String key : internalProperties.keySet()) {
                internalPropertiesDump.append("\n").append(key).append(": `").append(internalProperties.get(key)).append("`");
            }
        }

        Logger.logVerbose(LOG_TAG, internalPropertiesDump.toString());
    }

}
