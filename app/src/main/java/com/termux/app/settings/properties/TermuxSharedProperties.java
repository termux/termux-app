package com.termux.app.settings.properties;

import android.content.Context;
import android.content.res.Configuration;

import com.termux.app.terminal.io.extrakeys.ExtraKeysInfo;
import com.termux.app.terminal.io.KeyboardShortcut;
import com.termux.app.utils.Logger;
import com.termux.app.utils.DataUtils;

import org.json.JSONException;

import java.io.File;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;

import javax.annotation.Nonnull;

public class TermuxSharedProperties implements SharedPropertiesParser {

    private final Context mContext;
    private final SharedProperties mSharedProperties;
    private final File mPropertiesFile;

    private ExtraKeysInfo mExtraKeysInfo;
    private final List<KeyboardShortcut> mSessionShortcuts = new ArrayList<>();

    private static final String LOG_TAG = "TermuxSharedProperties";

    public TermuxSharedProperties(@Nonnull Context context) {
        mContext = context;
        mPropertiesFile = TermuxPropertyConstants.getTermuxPropertiesFile();
        mSharedProperties = new SharedProperties(context, mPropertiesFile, TermuxPropertyConstants.TERMUX_PROPERTIES_LIST, this);
        loadTermuxPropertiesFromDisk();
    }

    /**
     * Reload the termux properties from disk into an in-memory cache.
     */
    public void loadTermuxPropertiesFromDisk() {
        mSharedProperties.loadPropertiesFromDisk();
        dumpPropertiesToLog();
        dumpInternalPropertiesToLog();
        setExtraKeys();
        setSessionShortcuts();
    }

    /**
     * Set the terminal extra keys and style.
     */
    private void setExtraKeys() {
        mExtraKeysInfo = null;

        try {
            // The mMap stores the extra key and style string values while loading properties
            // Check {@link #getExtraKeysInternalPropertyValueFromValue(String)} and
            // {@link #getExtraKeysStyleInternalPropertyValueFromValue(String)}
            String extrakeys = (String) getInternalPropertyValue(TermuxPropertyConstants.KEY_EXTRA_KEYS, true);
            String extraKeysStyle = (String) getInternalPropertyValue(TermuxPropertyConstants.KEY_EXTRA_KEYS_STYLE, true);
            mExtraKeysInfo = new ExtraKeysInfo(extrakeys, extraKeysStyle);
        } catch (JSONException e) {
            Logger.showToast(mContext, "Could not load and set the \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS + "\" property from the properties file: " + e.toString(), true);
            Logger.logStackTraceWithMessage(LOG_TAG, "Could not load and set the \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS + "\" property from the properties file: ", e);

            try {
                mExtraKeysInfo = new ExtraKeysInfo(TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE);
            } catch (JSONException e2) {
                Logger.showToast(mContext, "Can't create default extra keys",true);
                Logger.logStackTraceWithMessage(LOG_TAG, "Could create default extra keys: ", e);
                mExtraKeysInfo = null;
            }
        }
    }

    /**
     * Set the terminal sessions shortcuts.
     */
    private void setSessionShortcuts() {
        mSessionShortcuts.clear();
        // The {@link TermuxPropertyConstants#MAP_SESSION_SHORTCUTS} stores the session shortcut key and action pair
        for (Map.Entry<String, Integer> entry : TermuxPropertyConstants.MAP_SESSION_SHORTCUTS.entrySet()) {
            // The mMap stores the code points for the session shortcuts while loading properties
            Integer codePoint = (Integer) getInternalPropertyValue(entry.getKey(), true);
            // If codePoint is null, then session shortcut did not exist in properties or was invalid
            // as parsed by {@link #getCodePointForSessionShortcuts(String,String)}
            // If codePoint is not null, then get the action for the MAP_SESSION_SHORTCUTS key and
            // add the code point to sessionShortcuts
            if (codePoint != null)
                mSessionShortcuts.add(new KeyboardShortcut(codePoint, entry.getValue()));
        }
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
     * @return Returns the {@code true} if the {@link Properties} key {@link String} value equals "true",
     * regardless of case. If the key does not exist in the file or does not equal "true", then
     * {@code false} will be returned.
     */
    public boolean isPropertyValueTrue(String key, boolean cached) {
        return (boolean) SharedProperties.getBooleanValueForStringValue((String) getPropertyValue(key, null, cached), false);
    }

    /**
     * A function to check if the value is {@code false} for {@link Properties} key read from
     * the {@link #mPropertiesFile} file.
     *
     * @param key The key to read.
     * @param cached If {@code true}, then the value is checked from the the {@link Properties} in-memory cache.
     *               Otherwise the {@link Properties} object is read directly from the file
     *               and value is checked from it.
     * @return Returns {@code true} if the {@link Properties} key {@link String} value equals "false",
     * regardless of case. If the key does not exist in the file or does not equal "false", then
     * {@code true} will be returned.
     */
    public boolean isPropertyValueFalse(String key, boolean cached) {
        return (boolean) SharedProperties.getInvertedBooleanValueForStringValue((String) getPropertyValue(key, null, cached), true);
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
                value = getInternalPropertyValueFromValue(mContext, key, null);
                Logger.logWarn(LOG_TAG, "The value for \"" + key + "\" not found in SharedProperties cahce, force returning default value: `" + value +  "`");
                return value;
            }
        } else {
            // We get the property value directly from file and return its internal value
            return getInternalPropertyValueFromValue(mContext, key, mSharedProperties.getProperty(key, false));
        }
    }

    /**
     * Override the
     * {@link SharedPropertiesParser#getInternalPropertyValueFromValue(Context,String,String)}
     * interface function.
     */
    @Override
    public Object getInternalPropertyValueFromValue(Context context, String key, String value) {
        return getInternalTermuxPropertyValueFromValue(context, key, value);
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
            // boolean
            case TermuxPropertyConstants.KEY_USE_BACK_KEY_AS_ESCAPE_KEY:
                return (boolean) getUseBackKeyAsEscapeKeyInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_USE_BLACK_UI:
                return (boolean) getUseBlackUIInternalPropertyValueFromValue(context, value);
            case TermuxPropertyConstants.KEY_VIRTUAL_VOLUME_KEYS_DISABLED:
                return (boolean) getVolumeKeysDisabledInternalPropertyValueFromValue(value);

            // int
            case TermuxPropertyConstants.KEY_BELL_BEHAVIOUR:
                return (int) getBellBehaviourInternalPropertyValueFromValue(value);

            // float
            case TermuxPropertyConstants.KEY_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR:
                return (float) getTerminalToolbarHeightScaleFactorInternalPropertyValueFromValue(value);

            // Integer (may be null)
            case TermuxPropertyConstants.KEY_SHORTCUT_CREATE_SESSION:
            case TermuxPropertyConstants.KEY_SHORTCUT_NEXT_SESSION:
            case TermuxPropertyConstants.KEY_SHORTCUT_PREVIOUS_SESSION:
            case TermuxPropertyConstants.KEY_SHORTCUT_RENAME_SESSION:
                return (Integer) getCodePointForSessionShortcuts(key, value);

            // String (may be null)
            case TermuxPropertyConstants.KEY_DEFAULT_WORKING_DIRECTORY:
                return (String) getDefaultWorkingDirectoryInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_EXTRA_KEYS:
                return (String) getExtraKeysInternalPropertyValueFromValue(value);
            case TermuxPropertyConstants.KEY_EXTRA_KEYS_STYLE:
                return (String) getExtraKeysStyleInternalPropertyValueFromValue(value);
            default:
                // default boolean behaviour
                if (TermuxPropertyConstants.TERMUX_DEFAULT_BOOLEAN_BEHAVIOUR_PROPERTIES_LIST.contains(key))
                    return (boolean) SharedProperties.getBooleanValueForStringValue(value, false);
                // default inverted boolean behaviour
                else if (TermuxPropertyConstants.TERMUX_DEFAULT_INVERETED_BOOLEAN_BEHAVIOUR_PROPERTIES_LIST.contains(key))
                    return (boolean) SharedProperties.getInvertedBooleanValueForStringValue(value, true);
                // just use String object as is (may be null)
                else
                    return value;
        }
    }





    /**
     * Returns {@code true} if value is not {@code null} and equals {@link TermuxPropertyConstants#VALUE_BACK_KEY_BEHAVIOUR_ESCAPE}, otherwise false.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static boolean getUseBackKeyAsEscapeKeyInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNull(value, TermuxPropertyConstants.VALUE_BACK_KEY_BEHAVIOUR_BACK).equals(TermuxPropertyConstants.VALUE_BACK_KEY_BEHAVIOUR_ESCAPE);
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
        return SharedProperties.getBooleanValueForStringValue(value, nightMode == Configuration.UI_MODE_NIGHT_YES);
    }

    /**
     * Returns {@code true} if value is not {@code null} and equals
     * {@link TermuxPropertyConstants#VALUE_VOLUME_KEY_BEHAVIOUR_VOLUME}, otherwise {@code false}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static boolean getVolumeKeysDisabledInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNull(value, TermuxPropertyConstants.VALUE_VOLUME_KEY_BEHAVIOUR_VIRTUAL).equals(TermuxPropertyConstants.VALUE_VOLUME_KEY_BEHAVIOUR_VOLUME);
    }

    /**
     * Returns the internal value after mapping it based on
     * {@code TermuxPropertyConstants#MAP_BELL_BEHAVIOUR} if the value is not {@code null}
     * and is valid, otherwise returns {@code TermuxPropertyConstants#DEFAULT_IVALUE_BELL_BEHAVIOUR}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static int getBellBehaviourInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNull(TermuxPropertyConstants.MAP_BELL_BEHAVIOUR.get(SharedProperties.toLowerCase(value)), TermuxPropertyConstants.DEFAULT_IVALUE_BELL_BEHAVIOUR);
    }

    /**
     * Returns the int for the value if its not null and is between
     * {@code TermuxPropertyConstants#IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MIN} and
     * {@code TermuxPropertyConstants#IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MAX},
     * otherwise returns {@code TermuxPropertyConstants#DEFAULT_IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static float getTerminalToolbarHeightScaleFactorInternalPropertyValueFromValue(String value) {
        return rangeTerminalToolbarHeightScaleFactorValue(DataUtils.getFloatFromString(value, TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR));
    }

    /**
     * Returns the value itself if it is between
     * {@code TermuxPropertyConstants#IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MIN} and
     * {@code TermuxPropertyConstants#IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MAX},
     * otherwise returns {@code TermuxPropertyConstants#DEFAULT_IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR}.
     *
     * @param value The value to clamp.
     * @return Returns the clamped value.
     */
    public static float rangeTerminalToolbarHeightScaleFactorValue(float value) {
        return DataUtils.rangedOrDefault(value,
            TermuxPropertyConstants.DEFAULT_IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR,
            TermuxPropertyConstants.IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MIN,
            TermuxPropertyConstants.IVALUE_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR_MAX);
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
            // Fallback to default directory if user configured working directory does not exist
            // or is not a directory or is not readable.
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
        return SharedProperties.getDefaultIfNull(value, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS);
    }

    /**
     * Returns the value itself if it is not {@code null}, otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_EXTRA_KEYS_STYLE}.
     *
     * @param value {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static String getExtraKeysStyleInternalPropertyValueFromValue(String value) {
        return SharedProperties.getDefaultIfNull(value, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE);
    }





    public boolean isEnforcingCharBasedInput() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_ENFORCE_CHAR_BASED_INPUT, true);
    }

    public boolean shouldSoftKeyboardBeHiddenOnStartup() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_HIDE_SOFT_KEYBOARD_ON_STARTUP, true);
    }

    public boolean isBackKeyTheEscapeKey() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_USE_BACK_KEY_AS_ESCAPE_KEY, true);
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

    public boolean areVirtualVolumeKeysDisabled() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_VIRTUAL_VOLUME_KEYS_DISABLED, true);
    }

    public int getBellBehaviour() {
        return (int) getInternalPropertyValue(TermuxPropertyConstants.KEY_BELL_BEHAVIOUR, true);
    }

    public float getTerminalToolbarHeightScaleFactor() {
        return rangeTerminalToolbarHeightScaleFactorValue((float) getInternalPropertyValue(TermuxPropertyConstants.KEY_TERMINAL_TOOLBAR_HEIGHT_SCALE_FACTOR, true));
    }

    public List<KeyboardShortcut> getSessionShortcuts() {
        return mSessionShortcuts;
    }

    public String getDefaultWorkingDirectory() {
        return (String) getInternalPropertyValue(TermuxPropertyConstants.KEY_DEFAULT_WORKING_DIRECTORY, true);
    }

    public ExtraKeysInfo getExtraKeysInfo() {
        return mExtraKeysInfo;
    }





    public void dumpPropertiesToLog() {
        Properties properties = getProperties(true);
        StringBuilder propertiesDump = new StringBuilder();

        propertiesDump.append("Termux Properties:");
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

        internalPropertiesDump.append("Termux Internal Properties:");
        if (internalProperties != null) {
            for (String key : internalProperties.keySet()) {
                internalPropertiesDump.append("\n").append(key).append(": `").append(internalProperties.get(key)).append("`");
            }
        }

        Logger.logVerbose(LOG_TAG, internalPropertiesDump.toString());
    }

}
