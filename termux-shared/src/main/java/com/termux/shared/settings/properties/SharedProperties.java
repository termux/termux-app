package com.termux.shared.settings.properties;

import android.content.Context;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.google.common.collect.BiMap;
import com.google.common.collect.ImmutableBiMap;
import com.google.common.primitives.Primitives;
import com.termux.shared.file.FileUtils;
import com.termux.shared.file.filesystem.FileType;
import com.termux.shared.logger.Logger;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Set;

/**
 * An implementation similar to android's {@link android.content.SharedPreferences} interface for
 * reading and writing to and from ".properties" files which also maintains an in-memory cache for
 * the key/value pairs when an instance object is used. Operations are done under
 * synchronization locks and should be thread safe.
 *
 * If {@link SharedProperties} instance object is used, then two types of in-memory cache maps are
 * maintained, one for the literal {@link String} values found in the file for the keys and an
 * additional one that stores (near) primitive {@link Object} values for internal use by the caller.
 *
 * The {@link SharedProperties} also provides static functions that can be used to read properties
 * from files or individual key values or even their internal values. An automatic mapping to a
 * boolean as internal value can also be done. An in-memory cache is not maintained, nor are locks used.
 *
 * This currently only has read support, write support can/will be added later if needed. Check android's
 * SharedPreferencesImpl class for reference implementation.
 *
 * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:frameworks/base/core/java/android/app/SharedPreferencesImpl.java
 */
public class SharedProperties {

    /**
     * The {@link Properties} object that maintains an in-memory cache of values loaded from the
     * {@link #mPropertiesFile} file. The key/value pairs are of any keys that are found in the file
     * against their literal values in the file.
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

    /** Defines the bidirectional map for boolean values and their internal values  */
    public static final ImmutableBiMap<String, Boolean> MAP_GENERIC_BOOLEAN =
        new ImmutableBiMap.Builder<String, Boolean>()
            .put("true", true)
            .put("false", false)
            .build();

    /** Defines the bidirectional map for inverted boolean values and their internal values  */
    public static final ImmutableBiMap<String, Boolean> MAP_GENERIC_INVERTED_BOOLEAN =
        new ImmutableBiMap.Builder<String, Boolean>()
            .put("true", false)
            .put("false", true)
            .build();

    private static final String LOG_TAG = "SharedProperties";

    /**
     * Constructor for the SharedProperties class.
     *
     * @param context The Context for operations.
     * @param propertiesFile The {@link File} object to load properties from.
     * @param propertiesList The {@link Set<String>} object that defined which properties to load.
     *                       If this is set to {@code null}, then all properties that exist in
     *                       {@code propertiesFile} will be read by {@link #loadPropertiesFromDisk()}
     * @param sharedPropertiesParser The implementation of the {@link SharedPropertiesParser} interface.
     */
    public SharedProperties(@NonNull Context context, @Nullable File propertiesFile, Set<String> propertiesList, @NonNull SharedPropertiesParser sharedPropertiesParser) {
        mContext = context.getApplicationContext();
        mPropertiesFile = propertiesFile;
        mPropertiesList = propertiesList;
        mSharedPropertiesParser = sharedPropertiesParser;

        mProperties = new Properties();
        mMap = new HashMap<>();
    }

    /**
     * Load the properties defined by {@link #mPropertiesList} or all properties if its {@code null}
     * from the {@link #mPropertiesFile} file to update the {@link #mProperties} and {@link #mMap}
     * in-memory cache.
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

            HashMap<String, Object> map = new HashMap<>();
            Properties newProperties = new Properties();

            Set<String> propertiesList = mPropertiesList;
            if (propertiesList == null)
                propertiesList = properties.stringPropertyNames();

            String value;
            Object internalValue;
            for (String key : propertiesList) {
                value = properties.getProperty(key); // value will be null if key does not exist in propertiesFile
                // Logger.logVerbose(LOG_TAG, key + " : " + value);

                // Call the {@link SharedPropertiesParser#getInternalPropertyValueFromValue(Context,String,String)}
                // interface method to get the internal value to store in the {@link #mMap}.
                internalValue = mSharedPropertiesParser.getInternalPropertyValueFromValue(mContext, key, value);

                // If the internal value was successfully added to map, then also add value to newProperties
                // We only store values in-memory defined by propertiesList
                if (putToMap(map, key, internalValue)) { // null internalValue will be put into map
                    putToProperties(newProperties, key, value); // null value will **not** be into properties
                }
            }

            mMap = map;
            mProperties = newProperties;
        }
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
                return getPropertiesFromFile(mContext, mPropertiesFile, mSharedPropertiesParser);
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
            if (mMap == null) mMap = new HashMap<>();
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
    public static Properties getPropertiesFromFile(Context context, File propertiesFile, @Nullable SharedPropertiesParser sharedPropertiesParser) {
        Properties properties = new Properties();

        if (propertiesFile == null) {
            Logger.logWarn(LOG_TAG, "Not loading properties since file is null");
            return properties;
        }

        try {
            try (FileInputStream in = new FileInputStream(propertiesFile)) {
                Logger.logVerbose(LOG_TAG, "Loading properties from \"" + propertiesFile.getAbsolutePath() + "\" file");
                properties.load(new InputStreamReader(in, StandardCharsets.UTF_8));
            }
        } catch (Exception e) {
            if (context != null)
                Toast.makeText(context, "Could not open properties file \"" + propertiesFile.getAbsolutePath() + "\": " + e.getMessage(), Toast.LENGTH_LONG).show();
            Logger.logStackTraceWithMessage(LOG_TAG, "Error loading properties file \"" + propertiesFile.getAbsolutePath() + "\"", e);
            return null;
        }

        if (sharedPropertiesParser != null && context != null)
            return sharedPropertiesParser.preProcessPropertiesOnReadFromDisk(context, properties);
        else
            return properties;
    }

    /** Returns the first {@link File} found in
     * {@code propertiesFilePaths} from which app properties can be loaded. If the {@link File} found
     * is not a regular file or is not readable, then {@code null} is returned. Symlinks **will not**
     * be followed for potential security reasons.
     *
     * @param propertiesFilePaths The {@link List<String>} containing properties file paths.
     * @param logTag If log tag to use for logging errors.
     * @return Returns the {@link File} object for Termux:Float app properties.
     */
    public static File getPropertiesFileFromList(List<String> propertiesFilePaths, @NonNull String logTag) {
        if (propertiesFilePaths == null || propertiesFilePaths.size() == 0)
            return null;

        for(String propertiesFilePath : propertiesFilePaths) {
            File propertiesFile = new File(propertiesFilePath);

            // Symlinks **will not** be followed.
            FileType fileType = FileUtils.getFileType(propertiesFilePath, false);
            if (fileType == FileType.REGULAR) {
                if (propertiesFile.canRead())
                    return propertiesFile;
                else
                    Logger.logWarn(logTag, "Ignoring properties file at \"" + propertiesFilePath + "\" since it is not readable");
            } else if (fileType != FileType.NO_EXIST) {
                Logger.logWarn(logTag, "Ignoring properties file at \"" + propertiesFilePath + "\" of type: \"" + fileType.getName() + "\"");
            }
        }

        Logger.logDebug(logTag, "No readable properties file found at: " + propertiesFilePaths);
        return null;
    }



    public static String getProperty(Context context, File propertiesFile, String key, String def) {
        return getProperty(context, propertiesFile, key, def, null);
    }

    /**
     * A static function to get the {@link String} value for the {@link Properties} key read from
     * the propertiesFile file.
     *
     * @param context The {@link Context} for the {@link #getPropertiesFromFile(Context,File,SharedPropertiesParser)} call.
     * @param propertiesFile The {@link File} to read the {@link Properties} from.
     * @param key The key to read.
     * @param def The default value.
     * @param sharedPropertiesParser The implementation of the {@link SharedPropertiesParser} interface.
     * @return Returns the {@link String} object. This will be {@code null} if key is not found.
     */
    public static String getProperty(Context context, File propertiesFile, String key, String def, @Nullable SharedPropertiesParser sharedPropertiesParser) {
        return (String) getDefaultIfNull(getDefaultIfNull(getPropertiesFromFile(context, propertiesFile, sharedPropertiesParser), new Properties()).get(key), def);
    }

    /**
     * A static function to get the internal {@link Object} value for the {@link String} value for
     * the {@link Properties} key read from the propertiesFile file.
     *
     * @param context The {@link Context} for the {@link #getPropertiesFromFile(Context,File,SharedPropertiesParser)} call.
     * @param propertiesFile The {@link File} to read the {@link Properties} from.
     * @param key The key to read.
     * @param sharedPropertiesParser The implementation of the {@link SharedPropertiesParser} interface.
     * @return Returns the {@link String} Object returned by the call to
     * {@link SharedPropertiesParser#getInternalPropertyValueFromValue(Context,String,String)}.
     */
    public static Object getInternalProperty(Context context, File propertiesFile, String key, @NonNull SharedPropertiesParser sharedPropertiesParser) {
        String value = (String) getDefaultIfNull(getPropertiesFromFile(context, propertiesFile, sharedPropertiesParser), new Properties()).get(key);

        // Call the {@link SharedPropertiesParser#getInternalPropertyValueFromValue(Context,String,String)}
        // interface method to get the internal value to return.
        return sharedPropertiesParser.getInternalPropertyValueFromValue(context, key, value);
    }


    public static boolean isPropertyValueTrue(Context context, File propertiesFile, String key, boolean logErrorOnInvalidValue) {
        return isPropertyValueTrue(context, propertiesFile, key, logErrorOnInvalidValue, null);
    }

    /**
     * A static function to check if the value is {@code true} for {@link Properties} key read from
     * the propertiesFile file.
     *
     * @param context The {@link Context} for the {@link #getPropertiesFromFile(Context,File,SharedPropertiesParser)} call.
     * @param propertiesFile The {@link File} to read the {@link Properties} from.
     * @param key The key to read.
     * @param logErrorOnInvalidValue If {@code true}, then an error will be logged if key value
     *                               was found in {@link Properties} but was invalid.
     * @param sharedPropertiesParser The implementation of the {@link SharedPropertiesParser} interface.
     * @return Returns the {@code true} if the {@link Properties} key {@link String} value equals "true",
     * regardless of case. If the key does not exist in the file or does not equal "true", then
     * {@code false} will be returned.
     */
    public static boolean isPropertyValueTrue(Context context, File propertiesFile, String key, boolean logErrorOnInvalidValue, @Nullable SharedPropertiesParser sharedPropertiesParser) {
        return (boolean) getBooleanValueForStringValue(key, (String) getProperty(context, propertiesFile, key, null, sharedPropertiesParser), false, logErrorOnInvalidValue, LOG_TAG);
    }


    public static boolean isPropertyValueFalse(Context context, File propertiesFile, String key, boolean logErrorOnInvalidValue) {
        return isPropertyValueFalse(context, propertiesFile, key, logErrorOnInvalidValue, null);
    }

    /**
     * A static function to check if the value is {@code false} for {@link Properties} key read from
     * the propertiesFile file.
     *
     * @param context The {@link Context} for the {@link #getPropertiesFromFile(Context,File,SharedPropertiesParser)} call.
     * @param propertiesFile The {@link File} to read the {@link Properties} from.
     * @param key The key to read.
     * @param logErrorOnInvalidValue If {@code true}, then an error will be logged if key value
     *                               was found in {@link Properties} but was invalid.
     * @param sharedPropertiesParser The implementation of the {@link SharedPropertiesParser} interface.
     * @return Returns the {@code true} if the {@link Properties} key {@link String} value equals "false",
     * regardless of case. If the key does not exist in the file or does not equal "false", then
     * {@code true} will be returned.
     */
    public static boolean isPropertyValueFalse(Context context, File propertiesFile, String key, boolean logErrorOnInvalidValue, @Nullable SharedPropertiesParser sharedPropertiesParser) {
        return (boolean) getInvertedBooleanValueForStringValue(key, (String) getProperty(context, propertiesFile, key, null, sharedPropertiesParser), true, logErrorOnInvalidValue, LOG_TAG);
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
            Logger.logError(LOG_TAG, "Map passed to SharedProperties.putToProperties() is null");
            return false;
        }

        // null keys are not allowed to be stored in mMap
        if (key == null) {
            Logger.logError(LOG_TAG, "Cannot put a null key into properties map");
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
            Logger.logError(LOG_TAG, "Cannot put a non-primitive value for the key \"" + key + "\" into properties map");
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
            Logger.logError(LOG_TAG, "Properties passed to SharedProperties.putToProperties() is null");
            return false;
        }

        // null keys are not allowed to be stored in mMap
        if (key == null) {
            Logger.logError(LOG_TAG, "Cannot put a null key into properties");
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

    public static Properties getPropertiesCopy(Properties inputProperties) {
        if (inputProperties == null) return null;

        Properties outputProperties = new Properties();
        for (String key : inputProperties.stringPropertyNames()) {
            outputProperties.put(key, inputProperties.get(key));
        }

        return outputProperties;
    }

    public static Map<String, Object> getMapCopy(Map<String, Object> map) {
        if (map == null) return null;
        return new HashMap<>(map);
    }






    /**
     * Get the boolean value for the {@link String} value.
     *
     * @param value The {@link String} value to convert.
     * @return Returns {@code true} or {@code false} if value is the literal string "true" or "false" respectively,
     * regardless of case. Otherwise returns {@code null}.
     */
    public static Boolean getBooleanValueForStringValue(String value) {
        return MAP_GENERIC_BOOLEAN.get(toLowerCase(value));
    }

    /**
     * Get the boolean value for the {@link String} value.
     *
     * @param value The {@link String} value to convert.
     * @param def The default {@link boolean} value to return.
     * @param logErrorOnInvalidValue If {@code true}, then an error will be logged if {@code value}
     *                               was not {@code null} and was invalid.
     * @param logTag If log tag to use for logging errors.
     * @return Returns {@code true} or {@code false} if value is the literal string "true" or "false" respectively,
     * regardless of case. Otherwise returns default value.
     */
    public static boolean getBooleanValueForStringValue(String key, String value, boolean def, boolean logErrorOnInvalidValue, String logTag) {
        return (boolean) getDefaultIfNotInMap(key, MAP_GENERIC_BOOLEAN, toLowerCase(value), def, logErrorOnInvalidValue, logTag);
    }

    /**
     * Get the inverted boolean value for the {@link String} value.
     *
     * @param value The {@link String} value to convert.
     * @param def The default {@link boolean} value to return.
     * @param logErrorOnInvalidValue If {@code true}, then an error will be logged if {@code value}
     *                               was not {@code null} and was invalid.
     * @param logTag If log tag to use for logging errors.
     * @return Returns {@code true} or {@code false} if value is the literal string "false" or "true" respectively,
     * regardless of case. Otherwise returns default value.
     */
    public static boolean getInvertedBooleanValueForStringValue(String key, String value, boolean def, boolean logErrorOnInvalidValue, String logTag) {
        return (boolean) getDefaultIfNotInMap(key, MAP_GENERIC_INVERTED_BOOLEAN, toLowerCase(value), def, logErrorOnInvalidValue, logTag);
    }

    /**
     * Get the value for the {@code inputValue} {@link Object} key from a {@link BiMap<>}, otherwise
     * default value if key not found in {@code map}.
     *
     * @param key The shared properties {@link String} key value for which the value is being returned.
     * @param map The {@link BiMap<>} value to get the value from.
     * @param inputValue The {@link Object} key value of the map.
     * @param defaultOutputValue The default {@link boolean} value to return if {@code inputValue} not found in map.
     *            The default value must exist as a value in the {@link BiMap<>} passed.
     * @param logErrorOnInvalidValue If {@code true}, then an error will be logged if {@code inputValue}
     *                               was not {@code null} and was not found in the map.
     * @param logTag If log tag to use for logging errors.
     * @return Returns the value for the {@code inputValue} key from the map if it exists. Otherwise
     * returns default value.
     */
    public static Object getDefaultIfNotInMap(String key, @NonNull BiMap<?, ?> map, Object inputValue, Object defaultOutputValue, boolean logErrorOnInvalidValue, String logTag) {
        Object outputValue = map.get(inputValue);
        if (outputValue == null) {
            Object defaultInputValue = map.inverse().get(defaultOutputValue);
            if (defaultInputValue == null)
                Logger.logError(LOG_TAG, "The default output value \"" + defaultOutputValue + "\" for the key \"" + key + "\" does not exist as a value in the BiMap passed to getDefaultIfNotInMap(): " + map.values());

            if (logErrorOnInvalidValue && inputValue != null) {
                if (key != null)
                    Logger.logError(logTag, "The value \"" + inputValue + "\" for the key \"" + key + "\" is invalid. Using default value \"" + defaultInputValue + "\" instead.");
                else
                    Logger.logError(logTag, "The value \"" + inputValue + "\" is invalid. Using default value \"" + defaultInputValue + "\" instead.");
            }

            return defaultOutputValue;
        } else {
            return outputValue;
        }
    }

    /**
     * Get the {@code int} {@code value} as is if between {@code min} and {@code max} (inclusive), otherwise
     * return default value.
     *
     * @param key The shared properties {@link String} key value for which the value is being returned.
     * @param value The {@code int} value to check.
     * @param def The default {@code int} value if {@code value} not in range.
     * @param min The min allowed {@code int} value.
     * @param max The max allowed {@code int} value.
     * @param logErrorOnInvalidValue If {@code true}, then an error will be logged if {@code value}
     *                               not in range.
     * @param ignoreErrorIfValueZero If logging error should be ignored if value equals 0.
     * @param logTag If log tag to use for logging errors.
     * @return Returns the {@code value} as is if within range. Otherwise returns default value.
     */
    public static int getDefaultIfNotInRange(String key, int value, int def, int min, int max, boolean logErrorOnInvalidValue, boolean ignoreErrorIfValueZero, String logTag) {
        if (value < min || value > max) {
            if (logErrorOnInvalidValue && (!ignoreErrorIfValueZero || value != 0)) {
                if (key != null)
                    Logger.logError(logTag, "The value \"" + value + "\" for the key \"" + key + "\" is not within the range " + min +  "-" + max +  " (inclusive). Using default value \"" + def + "\" instead.");
                else
                    Logger.logError(logTag, "The value \"" + value + "\" is not within the range " + min +  "-" + max +  " (inclusive). Using default value \"" + def + "\" instead.");
            }
            return def;
        } else {
            return value;
        }
    }

    /**
     * Get the {@code float} {@code value} as is if between {@code min} and {@code max} (inclusive), otherwise
     * return default value.
     *
     * @param key The shared properties {@link String} key value for which the value is being returned.
     * @param value The {@code float} value to check.
     * @param def The default {@code float} value if {@code value} not in range.
     * @param min The min allowed {@code float} value.
     * @param max The max allowed {@code float} value.
     * @param logErrorOnInvalidValue If {@code true}, then an error will be logged if {@code value}
     *                               not in range.
     * @param ignoreErrorIfValueZero If logging error should be ignored if value equals 0.
     * @param logTag If log tag to use for logging errors.
     * @return Returns the {@code value} as is if within range. Otherwise returns default value.
     */
    public static float getDefaultIfNotInRange(String key, float value, float def, float min, float max, boolean logErrorOnInvalidValue, boolean ignoreErrorIfValueZero, String logTag) {
        if (value < min || value > max) {
            if (logErrorOnInvalidValue && (!ignoreErrorIfValueZero || value != 0)) {
                if (key != null)
                    Logger.logError(logTag, "The value \"" + value + "\" for the key \"" + key + "\" is not within the range " + min +  "-" + max +  " (inclusive). Using default value \"" + def + "\" instead.");
                else
                    Logger.logError(logTag, "The value \"" + value + "\" is not within the range " + min +  "-" + max +  " (inclusive). Using default value \"" + def + "\" instead.");
            }
            return def;
        } else {
            return value;
        }
    }

    /**
     * Get the object itself if it is not {@code null}, otherwise default.
     *
     * @param object The {@link Object} to check.
     * @param def The default {@link Object}.
     * @return Returns {@code object} if it is not {@code null}, otherwise returns {@code def}.
     */
    public static <T> T getDefaultIfNull(@Nullable T object, @Nullable T def) {
        return (object == null) ? def : object;
    }

    /**
     * Get the {@link String} object itself if it is not {@code null} or empty, otherwise default.
     *
     * @param object The {@link String} to check.
     * @param def The default {@link String}.
     * @return Returns {@code object} if it is not {@code null}, otherwise returns {@code def}.
     */
    public static String getDefaultIfNullOrEmpty(@Nullable String object, @Nullable String def) {
        return (object == null || object.isEmpty()) ? def : object;
    }

    /**
     * Covert the {@link String} value to lowercase.
     *
     * @param value The {@link String} value to convert.
     * @return Returns the lowercased value.
     */
    public static String toLowerCase(String value) {
        if (value == null) return null; else return value.toLowerCase();
    }

}
