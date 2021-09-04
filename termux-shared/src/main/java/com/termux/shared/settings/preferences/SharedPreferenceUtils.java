package com.termux.shared.settings.preferences;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.SharedPreferences;

import com.termux.shared.logger.Logger;

import java.util.Set;

public class SharedPreferenceUtils {

    private static final String LOG_TAG = "SharedPreferenceUtils";

    /**
     * Get {@link SharedPreferences} instance of the preferences file 'name' with the operating mode
     * {@link Context#MODE_PRIVATE}. This file will be created in the app package's default
     * shared preferences directory.
     *
     * @param context The {@link Context} to get the {@link SharedPreferences} instance.
     * @param name The preferences file basename without extension.
     * @return The single {@link SharedPreferences} instance that can be used to retrieve and
     * modify the preference values.
     */
    public static SharedPreferences getPrivateSharedPreferences(Context context, String name) {
        return context.getSharedPreferences(name, Context.MODE_PRIVATE);
    }

    /**
     * Get {@link SharedPreferences} instance of the preferences file 'name' with the operating mode
     * {@link Context#MODE_PRIVATE} and {@link Context#MODE_MULTI_PROCESS}. This file will be
     * created in the app package's default shared preferences directory.
     *
     * @param context The {@link Context} to get the {@link SharedPreferences} instance.
     * @param name The preferences file basename without extension.
     * @return The single {@link SharedPreferences} instance that can be used to retrieve and
     * modify the preference values.
     */
    public static SharedPreferences getPrivateAndMultiProcessSharedPreferences(Context context, String name) {
        return context.getSharedPreferences(name, Context.MODE_PRIVATE | Context.MODE_MULTI_PROCESS);
    }



    /**
     * Get a {@code boolean} from {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to get the value from.
     * @param key The key for the value.
     * @param def The default value if failed to read a valid value.
     * @return Returns the {@code boolean} value stored in {@link SharedPreferences}, otherwise returns
     * default if failed to read a valid value, like in case of an exception.
     */
    public static boolean getBoolean(SharedPreferences sharedPreferences, String key, boolean def) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Error getting boolean value for the \"" + key + "\" key from null shared preferences. Returning default value \"" + def + "\".");
            return def;
        }

        try {
            return sharedPreferences.getBoolean(key, def);
        }
        catch (ClassCastException e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error getting boolean value for the \"" + key + "\" key from shared preferences. Returning default value \"" + def + "\".", e);
            return def;
        }
    }

    /**
     * Set a {@code boolean} in {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to set the value in.
     * @param key The key for the value.
     * @param value The value to store.
     * @param commitToFile If set to {@code true}, then value will be set to shared preferences
     *                     in-memory cache and the file synchronously. Ideally, only to be used for
     *                     multi-process use-cases.
     */
    @SuppressLint("ApplySharedPref")
    public static void setBoolean(SharedPreferences sharedPreferences, String key, boolean value, boolean commitToFile) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting boolean value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        if (commitToFile)
            sharedPreferences.edit().putBoolean(key, value).commit();
        else
            sharedPreferences.edit().putBoolean(key, value).apply();

    }



    /**
     * Get a {@code float} from {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to get the value from.
     * @param key The key for the value.
     * @param def The default value if failed to read a valid value.
     * @return Returns the {@code float} value stored in {@link SharedPreferences}, otherwise returns
     * default if failed to read a valid value, like in case of an exception.
     */
    public static float getFloat(SharedPreferences sharedPreferences, String key, float def) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Error getting float value for the \"" + key + "\" key from null shared preferences. Returning default value \"" + def + "\".");
            return def;
        }

        try {
            return sharedPreferences.getFloat(key, def);
        }
        catch (ClassCastException e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error getting float value for the \"" + key + "\" key from shared preferences. Returning default value \"" + def + "\".", e);
            return def;
        }
    }

    /**
     * Set a {@code float} in {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to set the value in.
     * @param key The key for the value.
     * @param value The value to store.
     * @param commitToFile If set to {@code true}, then value will be set to shared preferences
     *                     in-memory cache and the file synchronously. Ideally, only to be used for
     *                     multi-process use-cases.
     */
    @SuppressLint("ApplySharedPref")
    public static void setFloat(SharedPreferences sharedPreferences, String key, float value, boolean commitToFile) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting float value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        if (commitToFile)
            sharedPreferences.edit().putFloat(key, value).commit();
        else
            sharedPreferences.edit().putFloat(key, value).apply();
    }



    /**
     * Get an {@code int} from {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to get the value from.
     * @param key The key for the value.
     * @param def The default value if failed to read a valid value.
     * @return Returns the {@code int} value stored in {@link SharedPreferences}, otherwise returns
     * default if failed to read a valid value, like in case of an exception.
     */
    public static int getInt(SharedPreferences sharedPreferences, String key, int def) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Error getting int value for the \"" + key + "\" key from null shared preferences. Returning default value \"" + def + "\".");
            return def;
        }

        try {
            return sharedPreferences.getInt(key, def);
        }
        catch (ClassCastException e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error getting int value for the \"" + key + "\" key from shared preferences. Returning default value \"" + def + "\".", e);
            return def;
        }
    }

    /**
     * Set an {@code int} in {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to set the value in.
     * @param key The key for the value.
     * @param value The value to store.
     * @param commitToFile If set to {@code true}, then value will be set to shared preferences
     *                     in-memory cache and the file synchronously. Ideally, only to be used for
     *                     multi-process use-cases.
     */
    @SuppressLint("ApplySharedPref")
    public static void setInt(SharedPreferences sharedPreferences, String key, int value, boolean commitToFile) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting int value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        if (commitToFile)
            sharedPreferences.edit().putInt(key, value).commit();
        else
            sharedPreferences.edit().putInt(key, value).apply();
    }



    /**
     * Get a {@code long} from {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to get the value from.
     * @param key The key for the value.
     * @param def The default value if failed to read a valid value.
     * @return Returns the {@code long} value stored in {@link SharedPreferences}, otherwise returns
     * default if failed to read a valid value, like in case of an exception.
     */
    public static long getLong(SharedPreferences sharedPreferences, String key, long def) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Error getting long value for the \"" + key + "\" key from null shared preferences. Returning default value \"" + def + "\".");
            return def;
        }

        try {
            return sharedPreferences.getLong(key, def);
        }
        catch (ClassCastException e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error getting long value for the \"" + key + "\" key from shared preferences. Returning default value \"" + def + "\".", e);
            return def;
        }
    }

    /**
     * Set a {@code long} in {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to set the value in.
     * @param key The key for the value.
     * @param value The value to store.
     * @param commitToFile If set to {@code true}, then value will be set to shared preferences
     *                     in-memory cache and the file synchronously. Ideally, only to be used for
     *                     multi-process use-cases.
     */
    @SuppressLint("ApplySharedPref")
    public static void setLong(SharedPreferences sharedPreferences, String key, long value, boolean commitToFile) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting long value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        if (commitToFile)
            sharedPreferences.edit().putLong(key, value).commit();
        else
            sharedPreferences.edit().putLong(key, value).apply();
    }



    /**
     * Get a {@code String} from {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to get the value from.
     * @param key The key for the value.
     * @param def The default value if failed to read a valid value.
     * @param defIfEmpty If set to {@code true}, then {@code def} will be returned if value is empty.
     * @return Returns the {@code String} value stored in {@link SharedPreferences}, otherwise returns
     * default if failed to read a valid value, like in case of an exception.
     */
    public static String getString(SharedPreferences sharedPreferences, String key, String def, boolean defIfEmpty) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Error getting String value for the \"" + key + "\" key from null shared preferences. Returning default value \"" + def + "\".");
            return def;
        }

        try {
            String value = sharedPreferences.getString(key, def);
            if (defIfEmpty && (value == null || value.isEmpty()))
                return def;
            else
                return value;
        }
        catch (ClassCastException e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error getting String value for the \"" + key + "\" key from shared preferences. Returning default value \"" + def + "\".", e);
            return def;
        }
    }

    /**
     * Set a {@code String} in {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to set the value in.
     * @param key The key for the value.
     * @param value The value to store.
     * @param commitToFile If set to {@code true}, then value will be set to shared preferences
     *                     in-memory cache and the file synchronously. Ideally, only to be used for
     *                     multi-process use-cases.
     */
    @SuppressLint("ApplySharedPref")
    public static void setString(SharedPreferences sharedPreferences, String key, String value, boolean commitToFile) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting String value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        if (commitToFile)
            sharedPreferences.edit().putString(key, value).commit();
        else
            sharedPreferences.edit().putString(key, value).apply();
    }



    /**
     * Get a {@code Set<String>} from {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to get the value from.
     * @param key The key for the value.
     * @param def The default value if failed to read a valid value.
     * @return Returns the {@code Set<String>} value stored in {@link SharedPreferences}, otherwise returns
     * default if failed to read a valid value, like in case of an exception.
     */
    public static Set<String> getStringSet(SharedPreferences sharedPreferences, String key, Set<String> def) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Error getting Set<String> value for the \"" + key + "\" key from null shared preferences. Returning default value \"" + def + "\".");
            return def;
        }

        try {
            return sharedPreferences.getStringSet(key, def);
        }
        catch (ClassCastException e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Error getting Set<String> value for the \"" + key + "\" key from shared preferences. Returning default value \"" + def + "\".", e);
            return def;
        }
    }

    /**
     * Set a {@code Set<String>} in {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to set the value in.
     * @param key The key for the value.
     * @param value The value to store.
     * @param commitToFile If set to {@code true}, then value will be set to shared preferences
     *                     in-memory cache and the file synchronously. Ideally, only to be used for
     *                     multi-process use-cases.
     */
    @SuppressLint("ApplySharedPref")
    public static void setStringSet(SharedPreferences sharedPreferences, String key, Set<String> value, boolean commitToFile) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting Set<String> value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        if (commitToFile)
            sharedPreferences.edit().putStringSet(key, value).commit();
        else
            sharedPreferences.edit().putStringSet(key, value).apply();
    }



    /**
     * Get an {@code int} from {@link SharedPreferences} that is stored as a {@link String}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to get the value from.
     * @param key The key for the value.
     * @param def The default value if failed to read a valid value.
     * @return Returns the {@code int} value after parsing the {@link String} value stored in
     * {@link SharedPreferences}, otherwise returns default if failed to read a valid value,
     * like in case of an exception.
     */
    public static int getIntStoredAsString(SharedPreferences sharedPreferences, String key, int def) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Error getting int value for the \"" + key + "\" key from null shared preferences. Returning default value \"" + def + "\".");
            return def;
        }

        String stringValue;
        int intValue;

        try {
            stringValue = sharedPreferences.getString(key, Integer.toString(def));
            if (stringValue != null)
                intValue =  Integer.parseInt(stringValue);
            else
                intValue = def;
        } catch (NumberFormatException | ClassCastException e) {
            intValue = def;
        }

        return intValue;
    }

    /**
     * Set an {@code int} into {@link SharedPreferences} that is stored as a {@link String}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to set the value in.
     * @param key The key for the value.
     * @param value The value to store.
     * @param commitToFile If set to {@code true}, then value will be set to shared preferences
     *                     in-memory cache and the file synchronously. Ideally, only to be used for
     *                     multi-process use-cases.
     */
    @SuppressLint("ApplySharedPref")
    public static void setIntStoredAsString(SharedPreferences sharedPreferences, String key, int value, boolean commitToFile) {
        if (sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting int value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        if (commitToFile)
            sharedPreferences.edit().putString(key, Integer.toString(value)).commit();
        else
            sharedPreferences.edit().putString(key, Integer.toString(value)).apply();
    }

}
