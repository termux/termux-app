package com.termux.app.settings.properties;

import android.content.Context;
import android.content.res.Configuration;
import android.util.Log;
import android.widget.Toast;

import androidx.annotation.Nullable;

import com.termux.app.terminal.extrakeys.ExtraKeysInfo;
import com.termux.app.terminal.KeyboardShortcut;

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
            Toast.makeText(mContext, "Could not load and set the \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS + "\" property from the properties file: " + e.toString(), Toast.LENGTH_LONG).show();
            Log.e("termux", "Could not load and set the \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS + "\" property from the properties file: ", e);

            try {
                mExtraKeysInfo = new ExtraKeysInfo(TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE);
            } catch (JSONException e2) {
                Toast.makeText(mContext, "Can't create default extra keys", Toast.LENGTH_LONG).show();
                Log.e("termux", "Could create default extra keys: ", e);
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
     * A static function to get the {@link Properties} from the propertiesFile file.
     *
     * @param context The {@link Context} for the {@link SharedProperties#getPropertiesFromFile(Context,File)} call.
     * @param propertiesFile The {@link File} to read the {@link Properties} from.
     * @return Returns the {@link Properties} object. It will be {@code null} if an exception is
     * raised while reading the file.
     */
    public static Properties getPropertiesFromFile(Context context, File propertiesFile) {
        return SharedProperties.getPropertiesFromFile(context, propertiesFile);
    }

    /**
     * A static function to get the {@link String} value for the {@link Properties} key read from
     * the propertiesFile file.
     *
     * @param context The {@link Context} for the {@link SharedProperties#getPropertiesFromFile(Context,File)} call.
     * @param propertiesFile The {@link File} to read the {@link Properties} from.
     * @param key The key to read.
     * @param def The default value.
     * @return Returns the {@link String} object. This will be {@code null} if key is not found.
     */
    public static String getPropertyValue(Context context, File propertiesFile, String key, String def) {
        return (String) getDefaultIfNull(getDefaultIfNull(getPropertiesFromFile(context, propertiesFile), new Properties()).get(key), def);
    }

    /**
     * A static function to check if the value is {@code true} for {@link Properties} key read from
     * the propertiesFile file.
     *
     * @param context The {@link Context} for the {@link SharedProperties#getPropertiesFromFile(Context,File)}call.
     * @param propertiesFile The {@link File} to read the {@link Properties} from.
     * @param key The key to read.
     * @return Returns the {@code true} if the {@link Properties} key {@link String} value equals "true",
     * regardless of case. If the key does not exist in the file or does not equal "true", then
     * {@code false} will be returned.
     */
    public static boolean isPropertyValueTrue(Context context, File propertiesFile, String key) {
        return (boolean) getBooleanValueForStringValue((String) getPropertyValue(context, propertiesFile, key, null), false);
    }

    /**
     * A static function to check if the value is {@code false} for {@link Properties} key read from
     * the propertiesFile file.
     *
     * @param context The {@link Context} for the {@link SharedProperties#getPropertiesFromFile(Context,File)} call.
     * @param propertiesFile The {@link File} to read the {@link Properties} from.
     * @param key The key to read.
     * @return Returns the {@code true} if the {@link Properties} key {@link String} value equals "false",
     * regardless of case. If the key does not exist in the file or does not equal "false", then
     * {@code true} will be returned.
     */
    public static boolean isPropertyValueFalse(Context context, File propertiesFile, String key) {
        return (boolean) getInvertedBooleanValueForStringValue((String) getPropertyValue(context, propertiesFile, key, null), true);
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
        return getDefaultIfNull(mSharedProperties.getProperty(key, cached), def);
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
        return (boolean) getBooleanValueForStringValue((String) getPropertyValue(key, null, cached), false);
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
        return (boolean) getInvertedBooleanValueForStringValue((String) getPropertyValue(key, null, cached), true);
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
                Log.w("termux", "The value for \"" + key + "\" not found in SharedProperties cahce, force returning default value: `" + value +  "`");
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
        if(key == null) return null;
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
                if(TermuxPropertyConstants.TERMUX_DEFAULT_BOOLEAN_BEHAVIOUR_PROPERTIES_LIST.contains(key))
                    return (boolean) getBooleanValueForStringValue(value, false);
                // default inverted boolean behaviour
                else if(TermuxPropertyConstants.TERMUX_DEFAULT_INVERETED_BOOLEAN_BEHAVIOUR_PROPERTIES_LIST.contains(key))
                    return (boolean) getInvertedBooleanValueForStringValue(value, true);
                // just use String object as is (may be null)
                else
                    return value;
        }
    }





    /**
     * Get the boolean value for the {@link String} value.
     *
     * @param value The {@link String} value to convert.
     * @param def The default {@link boolean} value to return.
     * @return Returns {@code true} or {@code false} if value is the literal string "true" or "false" respectively,
     * regardless of case. Otherwise returns default value.
     */
    public static boolean getBooleanValueForStringValue(String value, boolean def) {
        return (boolean) getDefaultIfNull(TermuxPropertyConstants.MAP_GENERIC_BOOLEAN.get(toLowerCase(value)), def);
    }

    /**
     * Get the inverted boolean value for the {@link String} value.
     *
     * @param value The {@link String} value to convert.
     * @param def The default {@link boolean} value to return.
     * @return Returns {@code true} or {@code false} if value is the literal string "false" or "true" respectively,
     * regardless of case. Otherwise returns default value.
     */
    public static boolean getInvertedBooleanValueForStringValue(String value, boolean def) {
        return (boolean) getDefaultIfNull(TermuxPropertyConstants.MAP_GENERIC_INVERTED_BOOLEAN.get(toLowerCase(value)), def);
    }

    /**
     * Returns {@code true} if value is not {@code null} and equals {@link TermuxPropertyConstants#VALUE_BACK_KEY_BEHAVIOUR_ESCAPE}, otherwise false.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static boolean getUseBackKeyAsEscapeKeyInternalPropertyValueFromValue(String value) {
        return getDefaultIfNull(value, TermuxPropertyConstants.VALUE_BACK_KEY_BEHAVIOUR_BACK).equals(TermuxPropertyConstants.VALUE_BACK_KEY_BEHAVIOUR_ESCAPE);
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
        return getBooleanValueForStringValue(value, nightMode == Configuration.UI_MODE_NIGHT_YES);
    }

    /**
     * Returns {@code true} if value is not {@code null} and equals {@link TermuxPropertyConstants#VALUE_VOLUME_KEY_BEHAVIOUR_VOLUME}, otherwise {@code false}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static boolean getVolumeKeysDisabledInternalPropertyValueFromValue(String value) {
        return getDefaultIfNull(value, TermuxPropertyConstants.VALUE_VOLUME_KEY_BEHAVIOUR_VIRTUAL).equals(TermuxPropertyConstants.VALUE_VOLUME_KEY_BEHAVIOUR_VOLUME);
    }

    /**
     * Returns {@code true} if value is not {@code null} and equals {@link TermuxPropertyConstants#VALUE_BACK_KEY_BEHAVIOUR_ESCAPE}, otherwise {@code false}.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static int getBellBehaviourInternalPropertyValueFromValue(String value) {
        return getDefaultIfNull(TermuxPropertyConstants.MAP_BELL_BEHAVIOUR.get(toLowerCase(value)), TermuxPropertyConstants.DEFAULT_IVALUE_BELL_BEHAVIOUR);
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
            Log.e("termux", "Keyboard shortcut '" + key + "' is not Ctrl+<something>");
            return null;
        }

        char c = input.charAt(0);
        int codePoint = c;
        if (Character.isLowSurrogate(c)) {
            if (input.length() != 2 || Character.isHighSurrogate(input.charAt(1))) {
                Log.e("termux", "Keyboard shortcut '" + key + "' is not Ctrl+<something>");
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
        return getDefaultIfNull(value, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS);
    }

    /**
     * Returns the value itself if it is not {@code null}, otherwise returns {@link TermuxPropertyConstants#DEFAULT_IVALUE_EXTRA_KEYS_STYLE}.
     *
     * @param value {@link String} value to convert.
     * @return Returns the internal value for value.
     */
    public static String getExtraKeysStyleInternalPropertyValueFromValue(String value) {
        return getDefaultIfNull(value, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE);
    }





    public boolean isEnforcingCharBasedInput() {
        return (boolean) getInternalPropertyValue(TermuxPropertyConstants.KEY_ENFORCE_CHAR_BASED_INPUT, true);
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

    public List<KeyboardShortcut> getSessionShortcuts() {
        return mSessionShortcuts;
    }

    public String getDefaultWorkingDirectory() {
        return (String) getInternalPropertyValue(TermuxPropertyConstants.KEY_DEFAULT_WORKING_DIRECTORY, true);
    }

    public ExtraKeysInfo getExtraKeysInfo() {
        return mExtraKeysInfo;
    }





    public static <T> T getDefaultIfNull(@Nullable T object, @Nullable T def) {
        return (object == null) ? def : object;
    }

    private static String toLowerCase(String value) {
        if (value == null) return null; else return value.toLowerCase();
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

        Log.d("termux", propertiesDump.toString());
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

        Log.d("termux", internalPropertiesDump.toString());
    }

}
