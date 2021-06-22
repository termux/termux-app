package com.termux.shared.settings.properties;

import android.content.Context;

import java.util.HashMap;

/**
 * An interface that must be defined by the caller of the {@link SharedProperties} class.
 */
public interface SharedPropertiesParser {

    /**
     * A function that should return the internal {@link Object} to be stored for a key/value pair
     * read from properties file in the {@link HashMap <>} in-memory cache.
     *
     * @param context The context for operations.
     * @param key The key for which the internal object is required.
     * @param value The literal value for the property found is the properties file.
     * @return Returns the {@link Object} object to store in the {@link HashMap <>} in-memory cache.
     */
    Object getInternalPropertyValueFromValue(Context context, String key, String value);

}
