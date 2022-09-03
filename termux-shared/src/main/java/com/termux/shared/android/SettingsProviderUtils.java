package com.termux.shared.android;

import android.content.Context;
import android.provider.Settings;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;

public class SettingsProviderUtils {

    private static final String LOG_TAG = "SettingsProviderUtils";

    /** The namespaces for {@link Settings} provider. */
    public enum SettingNamespace {
        /** The {@link Settings.Global} namespace */
        GLOBAL,

        /** The {@link Settings.Secure} namespace */
        SECURE,

        /** The {@link Settings.System} namespace */
        SYSTEM
    }

    /** The type of values for {@link Settings} provider. */
    public enum SettingType {
        FLOAT,
        INT,
        LONG,
        STRING,
        URI
    }

    /**
     * Get settings key value from {@link SettingNamespace} namespace and of {@link SettingType} type.
     *
     * @param context The {@link Context} for operations.
     * @param namespace The {@link SettingNamespace} in which to get key value from.
     * @param type The {@link SettingType} for the key.
     * @param key The {@link String} name for key.
     * @param def The {@link Object} default value for key.
     * @return Returns the key value. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static Object getSettingsValue(@NonNull Context context, @NonNull SettingNamespace namespace,
                                          @NonNull SettingType type, @NonNull String key, @Nullable Object def) {
        try {
            switch (namespace) {
                case GLOBAL:
                    switch (type) {
                        case FLOAT:
                            return Settings.Global.getFloat(context.getContentResolver(), key);
                        case INT:
                            return Settings.Global.getInt(context.getContentResolver(), key);
                        case LONG:
                            return Settings.Global.getLong(context.getContentResolver(), key);
                        case STRING:
                            return Settings.Global.getString(context.getContentResolver(), key);
                        case URI:
                            return Settings.Global.getUriFor(key);
                    }
                case SECURE:
                    switch (type) {
                        case FLOAT:
                            return Settings.Secure.getFloat(context.getContentResolver(), key);
                        case INT:
                            return Settings.Secure.getInt(context.getContentResolver(), key);
                        case LONG:
                            return Settings.Secure.getLong(context.getContentResolver(), key);
                        case STRING:
                            return Settings.Secure.getString(context.getContentResolver(), key);
                        case URI:
                            return Settings.Secure.getUriFor(key);
                    }
                case SYSTEM:
                    switch (type) {
                        case FLOAT:
                            return Settings.System.getFloat(context.getContentResolver(), key);
                        case INT:
                            return Settings.System.getInt(context.getContentResolver(), key);
                        case LONG:
                            return Settings.System.getLong(context.getContentResolver(), key);
                        case STRING:
                            return Settings.System.getString(context.getContentResolver(), key);
                        case URI:
                            return Settings.System.getUriFor(key);
                    }
            }
        } catch (Settings.SettingNotFoundException e) {
            // Ignore
        } catch (Exception e) {
            Logger.logError(LOG_TAG, "Failed to get \"" + key + "\" key value from settings \"" + namespace.name() + "\" namespace of type \"" + type.name() + "\"");
        }
        return def;
    }

}
