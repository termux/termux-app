package com.termux.app.fragments.settings.base;

import android.content.Context;

import androidx.annotation.Nullable;
import androidx.preference.PreferenceDataStore;

/**
 * Abstract base class for DebuggingPreferences data stores.
 * Implements common data store functionality following DRY principle.
 * 
 * @param <T> The type of SharedPreferences implementation
 */
public abstract class BaseDebuggingPreferencesDataStore<T> extends PreferenceDataStore {

    protected final Context mContext;
    protected final T mPreferences;

    protected BaseDebuggingPreferencesDataStore(Context context) {
        mContext = context;
        mPreferences = createPreferences(context);
    }

    /**
     * Template method to create the specific preferences instance.
     */
    protected abstract T createPreferences(Context context);

    /**
     * Template method to get log level from preferences.
     */
    protected abstract int getLogLevel();

    /**
     * Template method to set log level in preferences.
     */
    protected abstract void setLogLevel(int level);

    @Override
    @Nullable
    public String getString(String key, @Nullable String defValue) {
        if (mPreferences == null || key == null) return defValue;

        switch (key) {
            case "log_level":
                return String.valueOf(getLogLevel());
            default:
                return getAdditionalString(key, defValue);
        }
    }

    @Override
    public void putString(String key, @Nullable String value) {
        if (mPreferences == null || key == null) return;

        switch (key) {
            case "log_level":
                if (value != null) {
                    try {
                        setLogLevel(Integer.parseInt(value));
                    } catch (NumberFormatException e) {
                        // Log error but don't crash
                    }
                }
                break;
            default:
                putAdditionalString(key, value);
                break;
        }
    }

    /**
     * Hook method for subclasses to handle additional string preferences.
     */
    protected String getAdditionalString(String key, @Nullable String defValue) {
        return defValue;
    }

    /**
     * Hook method for subclasses to handle additional string preference updates.
     */
    protected void putAdditionalString(String key, @Nullable String value) {
        // Default implementation does nothing
    }

    /**
     * Hook method for subclasses that need boolean preferences.
     */
    @Override
    public boolean getBoolean(String key, boolean defValue) {
        if (mPreferences == null) return defValue;
        return getAdditionalBoolean(key, defValue);
    }

    /**
     * Hook method for subclasses that need to store boolean preferences.
     */
    @Override
    public void putBoolean(String key, boolean value) {
        if (mPreferences == null) return;
        putAdditionalBoolean(key, value);
    }

    /**
     * Hook method for subclasses to handle additional boolean preferences.
     */
    protected boolean getAdditionalBoolean(String key, boolean defValue) {
        return defValue;
    }

    /**
     * Hook method for subclasses to handle additional boolean preference updates.
     */
    protected void putAdditionalBoolean(String key, boolean value) {
        // Default implementation does nothing
    }
}