package com.termux.shared.settings.properties;

import android.content.Context;

import androidx.annotation.NonNull;

import java.util.HashMap;
import java.util.Properties;

/**
 * An interface that must be defined by the caller of the {@link SharedProperties} class.
 */
public interface SharedPropertiesParser {

    /**
     * Called when properties are loaded from file to allow client to update the {@link Properties}
     * loaded from properties file before key/value pairs are stored in the {@link HashMap <>} in-memory
     * cache.
     *
     * @param context The context for operations.
     * @param properties The key for which the internal object is required.
     */
    @NonNull
    Properties preProcessPropertiesOnReadFromDisk(@NonNull Context context, @NonNull Properties properties);

    /**
     * A function that should return the internal {@link Object} to be stored for a key/value pair
     * read from properties file in the {@link HashMap <>} in-memory cache.
     *
     * @param context The context for operations.
     * @param key The key for which the internal object is required.
     * @param value The literal value for the property found is the properties file.
     * @return Returns the {@link Object} object to store in the {@link HashMap <>} in-memory cache.
     */
    Object getInternalPropertyValueFromValue(@NonNull Context context, String key, String value);

}
