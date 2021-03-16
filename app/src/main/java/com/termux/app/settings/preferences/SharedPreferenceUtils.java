package com.termux.app.settings.preferences;

import android.content.SharedPreferences;

import com.termux.app.utils.Logger;

public class SharedPreferenceUtils {

    private static final String LOG_TAG = "SharedPreferenceUtils";

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
        if(sharedPreferences == null) {
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
     */
    public static void setBoolean(SharedPreferences sharedPreferences, String key, boolean value) {
        if(sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting boolean value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        sharedPreferences.edit().putBoolean(key, value).apply();
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
        if(sharedPreferences == null) {
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
     */
    public static void setInt(SharedPreferences sharedPreferences, String key, int value) {
        if(sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting int value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        sharedPreferences.edit().putInt(key, value).apply();
    }



    /**
     * Get a {@code String} from {@link SharedPreferences}.
     *
     * @param sharedPreferences The {@link SharedPreferences} to get the value from.
     * @param key The key for the value.
     * @param def The default value if failed to read a valid value.
     * @return Returns the {@code String} value stored in {@link SharedPreferences}, otherwise returns
     * default if failed to read a valid value, like in case of an exception.
     */
    public static String getString(SharedPreferences sharedPreferences, String key, String def) {
        if(sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Error getting String value for the \"" + key + "\" key from null shared preferences. Returning default value \"" + def + "\".");
            return def;
        }

        try {
            return sharedPreferences.getString(key, def);
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
     */
    public static void setString(SharedPreferences sharedPreferences, String key, String value) {
        if(sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting String value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        sharedPreferences.edit().putString(key, value).apply();
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
        if(sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Error getting int value for the \"" + key + "\" key from null shared preferences. Returning default value \"" + def + "\".");
            return def;
        }

        String stringValue;
        int intValue;

        try {
            stringValue = sharedPreferences.getString(key, Integer.toString(def));
            if(stringValue != null)
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
     */
    public static void setIntStoredAsString(SharedPreferences sharedPreferences, String key, int value) {
        if(sharedPreferences == null) {
            Logger.logError(LOG_TAG, "Ignoring setting int value \"" + value + "\" for the \"" + key + "\" key into null shared preferences.");
            return;
        }

        sharedPreferences.edit().putString(key, Integer.toString(value)).apply();
    }

}
