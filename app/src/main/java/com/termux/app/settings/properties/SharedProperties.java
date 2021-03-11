package com.termux.app.settings.properties;

import android.content.Context;
import android.util.Log;
import android.widget.Toast;

import com.google.common.primitives.Primitives;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

/**
 * An implementation similar to android's {@link android.content.SharedPreferences} interface for
 * reading and writing to and from ".properties" files which also maintains an in-memory cache for
 * the key/value pairs. Operations are does under synchronization locks and should be thread safe.
 *
 * Two types of in-memory cache maps are maintained, one for the literal {@link String} values found
 * in the file for the keys and an additional one that stores (near) primitive {@link Object} values for
 * internal use by the caller.
 *
 * This currently only has read support, write support can/will be added later if needed. Check android's
 * SharedPreferencesImpl class for reference implementation.
 *
 * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:frameworks/base/core/java/android/app/SharedPreferencesImpl.java
 */
public class SharedProperties {

    /**
     * The {@link Properties} object that maintains an in-memory cache of values loaded from the
     * {@link #mPropertiesFile} file. The key/value pairs are of any keys defined by
     * {@link #mPropertiesList} that are found in the file against their literal values in the file.
     */
    private Properties mProperties;

    /**
     * The {@link HashMap<>} object that maintains an in-memory cache of internal values for the values
     * loaded from the {@link #mPropertiesFile} file. The key/value pairs are of any keys defined by
     * {@link #mPropertiesList} that are found in the file against their internal {@link Object} values
     * returned by the call to
     * {@link SharedPropertiesParser#getInternalPropertyValueFromValue(Context, String, String)} interface.
     */
    private Map<String, Object> mMap;

    private final Context mContext;
    private final File mPropertiesFile;
    private final Set<String> mPropertiesList;
    private final SharedPropertiesParser mSharedPropertiesParser;

    private final Object mLock = new Object();

    /**
     * Constructor for the SharedProperties class.
     *
     * @param context The Context for operations.
     * @param propertiesFile The {@link File} object to load properties from.
     * @param propertiesList The {@link Set<String>} object that defined which properties to load.
     * @param sharedPropertiesParser that implements the {@link SharedPropertiesParser} interface.
     */
    public SharedProperties(@Nonnull Context context, @Nullable File propertiesFile, @Nonnull Set<String> propertiesList, @Nonnull SharedPropertiesParser sharedPropertiesParser) {
        mContext = context;
        mPropertiesFile = propertiesFile;
        mPropertiesList = propertiesList;
        mSharedPropertiesParser = sharedPropertiesParser;

        mProperties = new Properties();
        mMap = new HashMap<String, Object>();
    }

    /**
     * Load the properties defined by {@link #mPropertiesList} from the {@link #mPropertiesFile} file
     * to update the {@link #mProperties} and {@link #mMap} in-memory cache.
     * Properties are not loading automatically when constructor is called and must be manually called.
     */
    public void loadPropertiesFromDisk() {
        synchronized (mLock) {
            // Get properties from mPropertiesFile
            Properties properties = getProperties(false);

            // We still need to load default values into mMap, so we assume no properties defined if
            // reading from mPropertiesFile failed
            if (properties == null)
                properties = new Properties();

            HashMap<String, Object> map = new HashMap<String, Object>();
            Properties newProperties = new Properties();

            String value;
            Object internalValue;
            for (String key : mPropertiesList) {
                value = properties.getProperty(key); // value will be null if key does not exist in propertiesFile
                Log.d("termux", key + " : " + value);

                // Call the {@link SharedPropertiesParser#getInternalPropertyValueFromValue(Context,String,String)}
                // interface method to get the internal value to store in the  {@link #mMap}.
                internalValue = mSharedPropertiesParser.getInternalPropertyValueFromValue(mContext, key, value);

                // If the internal value was successfully added to map, then also add value to newProperties
                // We only store values in-memory defined by {@link #mPropertiesList}
                if (putToMap(map, key, internalValue)) { // null internalValue will be put into map
                    putToProperties(newProperties, key, value); // null value will **not** be into properties
                }
            }

            mMap = map;
            mProperties = newProperties;
        }
    }

    /**
     * Put a value in a {@link #mMap}.
     * The key cannot be {@code null}.
     * Only {@code null}, primitive or their wrapper classes or String class objects are allowed to be added to
     * the map, although this limitation may be changed.
     *
     * @param map The {@link Map} object to add value to.
     * @param key The key for which to add the value to the map.
     * @param value The {@link Object} to add to the map.
     * @return Returns {@code true} if value was successfully added, otherwise {@code false}.
     */
    public static boolean putToMap(HashMap<String, Object> map, String key, Object value) {

        if (map == null) {
            Log.e("termux", "Map passed to SharedProperties.putToProperties() is null");
            return false;
        }

        // null keys are not allowed to be stored in mMap
        if (key == null) {
            Log.e("termux", "Cannot put a null key into properties map");
            return false;
        }

        boolean put = false;
        if (value != null) {
            Class<?> clazz = value.getClass();
            if (clazz.isPrimitive() || Primitives.isWrapperType(clazz) || value instanceof String) {
                put = true;
            }
        } else {
            put = true;
        }

        if (put) {
            map.put(key, value);
            return true;
        } else {
            Log.e("termux", "Cannot put a non-primitive value for the key \"" + key + "\" into properties map");
            return false;
        }
    }

    /**
     * Put a value in a {@link Map}.
     * The key cannot be {@code null}.
     * Passing {@code null} as the value argument is equivalent to removing the key from the
     * properties.
     *
     * @param properties The {@link Properties} object to add value to.
     * @param key The key for which to add the value to the properties.
     * @param value The {@link String} to add to the properties.
     * @return Returns {@code true} if value was successfully added, otherwise {@code false}.
     */
    public static boolean putToProperties(Properties properties, String key, String value) {

        if (properties == null) {
            Log.e("termux", "Properties passed to SharedProperties.putToProperties() is null");
            return false;
        }

        // null keys are not allowed to be stored in mMap
        if (key == null) {
            Log.e("termux", "Cannot put a null key into properties");
            return false;
        }

        if (value != null) {
            properties.put(key, value);
            return true;
        } else {
            properties.remove(key);
        }

        return true;
    }

    /**
     * A static function to get the {@link Properties} object for the propertiesFile. A lock is not
     * taken when this function is called.
     *
     * @param context The {@link Context} to use to show a flash if an exception is raised while
     *                reading the file. If context is {@code null}, then flash will not be shown.
     * @param propertiesFile The {@link File} to read the {@link Properties} from.
     * @return Returns the {@link Properties} object. It will be {@code null} if an exception is
     * raised while reading the file.
     */
    public static Properties getPropertiesFromFile(Context context, File propertiesFile) {
        Properties properties = new Properties();

        if (propertiesFile == null) {
            Log.e("termux", "Not loading properties since file is null");
            return properties;
        }

        try {
            try (FileInputStream in = new FileInputStream(propertiesFile)) {
                Log.v("termux", "Loading properties from \"" + propertiesFile.getAbsolutePath() + "\" file");
                properties.load(new InputStreamReader(in, StandardCharsets.UTF_8));
            }
        } catch (Exception e) {
            if(context != null)
                Toast.makeText(context, "Could not open properties file \"" + propertiesFile.getAbsolutePath() + "\": " + e.getMessage(), Toast.LENGTH_LONG).show();
            Log.e("termux", "Error loading properties file \"" + propertiesFile.getAbsolutePath() + "\"", e);
            return null;
        }

        return properties;
    }

    /**
     * Get the {@link Properties} object for the {@link #mPropertiesFile}. The {@link Properties}
     * object will also contain properties not defined by the {@link #mPropertiesList} if cache
     * value is {@code false}.
     *
     * @param cached If {@code true}, then the {@link #mProperties} in-memory cache is returned. Otherwise
     *               the {@link Properties} object is directly read from the {@link #mPropertiesFile}.
     * @return Returns the {@link Properties} object if read from file, otherwise a copy of {@link #mProperties}.
     */
    public Properties getProperties(boolean cached) {
        synchronized (mLock) {
            if (cached) {
                if (mProperties == null) mProperties = new Properties();
                return getPropertiesCopy(mProperties);
            } else {
                return getPropertiesFromFile(mContext, mPropertiesFile);
            }
        }
    }

    /**
     * Get the {@link String} value for the key passed from the {@link #mPropertiesFile}.
     *
     * @param key The key to read from the {@link Properties} object.
     * @param cached If {@code true}, then the value is returned from the {@link #mProperties} in-memory cache.
     *               Otherwise the {@link Properties} object is read directly from the {@link #mPropertiesFile}
     *               and value is returned from it against the key.
     * @return Returns the {@link String} object. This will be {@code null} if key is not found.
     */
    public String getProperty(String key, boolean cached) {
        synchronized (mLock) {
            return (String) getProperties(cached).get(key);
        }
    }

    /**
     * Get the {@link #mMap} object for the {@link #mPropertiesFile}. A call to
     * {@link #loadPropertiesFromDisk()} must be made before this.
     *
     * @return Returns a copy of {@link #mMap} object.
     */
    public Map<String, Object> getInternalProperties() {
        synchronized (mLock) {
            if (mMap == null) mMap = new HashMap<String, Object>();
            return getMapCopy(mMap);
        }
    }

    /**
     * Get the internal {@link Object} value for the key passed from the {@link #mPropertiesFile}.
     * The value is returned from the {@link #mMap} in-memory cache, so a call to
     * {@link #loadPropertiesFromDisk()} must be made before this.
     *
     * @param key The key to read from the {@link #mMap} object.
     * @return Returns the {@link Object} object. This will be {@code null} if key is not found or
     * if object was {@code null}. Use {@link HashMap#containsKey(Object)} to detect the later.
     * situation.
     */
    public Object getInternalProperty(String key) {
        synchronized (mLock) {
            // null keys are not allowed to be stored in mMap
            if (key != null)
                return getInternalProperties().get(key);
            else
                return null;
        }
    }

    public static Properties getPropertiesCopy(Properties inputProperties) {
        if(inputProperties == null) return null;

        Properties outputProperties = new Properties();
        for (String key : inputProperties.stringPropertyNames()) {
            outputProperties.put(key, inputProperties.get(key));
        }

        return outputProperties;
    }

    public static Map<String, Object> getMapCopy(Map<String, Object> map) {
        if(map == null) return null;
        return new HashMap<String, Object>(map);
    }

}
