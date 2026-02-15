package com.termux.app.fragments.settings.termux;

import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.preference.EditTextPreference;
import androidx.preference.ListPreference;
import androidx.preference.Preference;
import androidx.preference.PreferenceDataStore;
import androidx.preference.PreferenceFragmentCompat;
import androidx.preference.PreferenceManager;
import androidx.preference.SwitchPreferenceCompat;

import com.termux.R;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxConstants.TERMUX_APP.TERMUX_ACTIVITY;
import com.termux.shared.logger.Logger;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Properties;

/**
 * Fragment for termux.properties settings
 */
@Keep
public class TermuxPropertiesPreferencesFragment extends PreferenceFragmentCompat {

    private static final String LOG_TAG = "TermuxPropertiesPrefs";

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        Context context = getContext();
        if (context == null) return;

        // Set the DataStore for both reading and writing
        PreferenceManager preferenceManager = getPreferenceManager();
        preferenceManager.setPreferenceDataStore(TermuxPropertiesPreferencesDataStore.getInstance(context));

        setPreferencesFromResource(R.xml.termux_properties_preferences, rootKey);
        
        // Setup listeners for each preference to ensure saving
        setupPreferenceListeners(context);
    }
    
    private void setupPreferenceListeners(Context context) {
        if (context == null) return;
        
        final TermuxPropertiesPreferencesDataStore dataStore = TermuxPropertiesPreferencesDataStore.getInstance(context);
        
        // Switch preferences
        String[] switchKeys = {
            "disable-hardware-keyboard-shortcuts",
            "disable-terminal-session-change-toast",
            "enforce-char-based-input",
            "extra-keys-text-all-caps",
            "hide-soft-keyboard-on-startup",
            "run-termux-am-socket-server",
            "terminal-onclick-url-open",
            "ctrl-space-workaround",
            "fullscreen",
            "use-fullscreen-workaround",
            "disable-file-share-receiver",
            "disable-file-view-receiver"
        };
        
        for (String key : switchKeys) {
            SwitchPreferenceCompat pref = findPreference(key);
            if (pref != null) {
                pref.setOnPreferenceChangeListener((preference, newValue) -> {
                    dataStore.putBoolean(key, (Boolean) newValue);
                    Logger.logInfo(LOG_TAG, "Switch preference changed: " + key + " = " + newValue);
                    return true;
                });
            }
        }
        
        // List preferences
        String[] listKeys = {
            "bell-character",
            "back-key",
            "volume-keys",
            "soft-keyboard-toggle-behaviour",
            "night-mode",
            "extra-keys-style",
            "terminal-cursor-style",
            "terminal-cursor-blink-rate",
            "terminal-transcript-rows",
            "terminal-margin-horizontal",
            "terminal-margin-vertical",
            "terminal-toolbar-height",
            "delete-tmpdir-files-older-than-x-days-on-exit"
        };
        
        for (String key : listKeys) {
            ListPreference pref = findPreference(key);
            if (pref != null) {
                pref.setOnPreferenceChangeListener((preference, newValue) -> {
                    dataStore.putString(key, (String) newValue);
                    Logger.logInfo(LOG_TAG, "List preference changed: " + key + " = " + newValue);
                    return true;
                });
            }
        }
        
        // EditText preference
        EditTextPreference editPref = findPreference("default-working-directory");
        if (editPref != null) {
            editPref.setOnPreferenceChangeListener((preference, newValue) -> {
                dataStore.putString("default-working-directory", (String) newValue);
                Logger.logInfo(LOG_TAG, "EditText preference changed: default-working-directory = " + newValue);
                return true;
            });
        }
    }

}

/**
 * DataStore for termux.properties settings
 * Reads from and writes to termux.properties file
 */
class TermuxPropertiesPreferencesDataStore extends PreferenceDataStore {

    private final Context mContext;
    private static final String DATASTORE_LOG_TAG = "TermuxPropertiesDataStore";

    private static TermuxPropertiesPreferencesDataStore mInstance;

    private TermuxPropertiesPreferencesDataStore(Context context) {
        mContext = context;
    }

    public static synchronized TermuxPropertiesPreferencesDataStore getInstance(Context context) {
        if (mInstance == null) {
            mInstance = new TermuxPropertiesPreferencesDataStore(context);
        }
        return mInstance;
    }

    /**
     * Get the termux.properties file path
     * The correct path is /data/data/com.termux/files/home/.termux/termux.properties
     */
    @NonNull
    private File getPropertiesFile() {
        // Use the correct path from TermuxConstants
        File termuxDir = new File(TermuxConstants.TERMUX_DATA_HOME_DIR_PATH);
        if (!termuxDir.exists()) {
            termuxDir.mkdirs();
        }
        return new File(termuxDir, "termux.properties");
    }

    /**
     * Load properties from file
     */
    @NonNull
    private Properties loadProperties() {
        Properties props = new Properties();
        File propertiesFile = getPropertiesFile();
        
        if (propertiesFile.exists()) {
            try (FileInputStream fis = new FileInputStream(propertiesFile)) {
                props.load(fis);
            } catch (Exception e) {
                Logger.logStackTraceWithMessage(DATASTORE_LOG_TAG, "Failed to load properties file: " + propertiesFile.getAbsolutePath(), e);
            }
        }
        
        return props;
    }

    /**
     * Save properties to file
     */
    private void saveProperties(Properties props) {
        File propertiesFile = getPropertiesFile();
        
        try (FileOutputStream fos = new FileOutputStream(propertiesFile)) {
            props.store(fos, "Termux Properties");
            Logger.logDebug(DATASTORE_LOG_TAG, "Properties saved to file: " + propertiesFile.getAbsolutePath());
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(DATASTORE_LOG_TAG, "Failed to save properties file: " + propertiesFile.getAbsolutePath(), e);
        }
    }

    /**
     * Read a property from termux.properties
     */
    private String readProperty(String key) {
        Properties props = loadProperties();
        return props.getProperty(key);
    }

    /**
     * Write a property to termux.properties
     */
    private void writeProperty(String key, @Nullable String value) {
        Properties props = loadProperties();
        
        // Set the new value
        if (value == null || value.isEmpty()) {
            props.remove(key);
        } else {
            props.setProperty(key, value);
        }

        saveProperties(props);
        Logger.logInfo(DATASTORE_LOG_TAG, "Updated property: " + key + " = " + value);
        
        // Notify TermuxActivity to reload properties
        notifyPropertiesChanged();
    }
    
    /**
     * Mark that properties have changed and need to be reloaded
     * Also send broadcast for immediate reload if TermuxActivity is visible
     */
    private void notifyPropertiesChanged() {
        // Set flag to indicate properties need reload (will be checked in TermuxActivity.onResume)
        mContext.getSharedPreferences("termux_prefs", Context.MODE_PRIVATE)
            .edit()
            .putBoolean("properties_need_reload", true)
            .apply();
        
        // Also send broadcast for immediate reload if TermuxActivity is in foreground
        Intent intent = new Intent(TERMUX_ACTIVITY.ACTION_RELOAD_STYLE);
        intent.putExtra(TERMUX_ACTIVITY.EXTRA_RECREATE_ACTIVITY, false);
        intent.setPackage(mContext.getPackageName());
        mContext.sendBroadcast(intent);
        Logger.logDebug(DATASTORE_LOG_TAG, "Marked properties for reload and sent broadcast");
    }

    // Boolean getters and setters
    @Override
    public boolean getBoolean(String key, boolean defValue) {
        String value = readProperty(key);
        if (value == null) return defValue;
        return "true".equalsIgnoreCase(value);
    }

    @Override
    public void putBoolean(String key, boolean value) {
        writeProperty(key, value ? "true" : "false");
    }

    // String getters and setters
    @Override
    public String getString(String key, @Nullable String defValue) {
        String value = readProperty(key);
        return value != null ? value : defValue;
    }

    @Override
    public void putString(String key, @Nullable String value) {
        writeProperty(key, value);
    }

    // Int getters and setters
    public int getInt(String key, int defValue) {
        String value = readProperty(key);
        if (value == null) return defValue;
        try {
            return Integer.parseInt(value);
        } catch (NumberFormatException e) {
            return defValue;
        }
    }

    public void putInt(String key, int value) {
        writeProperty(key, String.valueOf(value));
    }

    // Float getters and setters
    public float getFloat(String key, float defValue) {
        String value = readProperty(key);
        if (value == null) return defValue;
        try {
            return Float.parseFloat(value);
        } catch (NumberFormatException e) {
            return defValue;
        }
    }

    public void putFloat(String key, float value) {
        writeProperty(key, String.valueOf(value));
    }
}
