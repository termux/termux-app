package com.termux.shared.termux.settings.properties;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.shared.termux.TermuxConstants;

public class TermuxAppSharedProperties extends TermuxSharedProperties {

    private static TermuxAppSharedProperties properties;


    private TermuxAppSharedProperties(@NonNull Context context) {
        super(context, TermuxConstants.TERMUX_APP_NAME,
            TermuxConstants.TERMUX_PROPERTIES_FILE_PATHS_LIST, TermuxPropertyConstants.TERMUX_APP_PROPERTIES_LIST,
            new TermuxSharedProperties.SharedPropertiesParserClient());
    }

    /**
     * Initialize the {@link #properties} and load properties from disk.
     *
     * @param context The {@link Context} for operations.
     * @return Returns the {@link TermuxAppSharedProperties}.
     */
    public static TermuxAppSharedProperties init(@NonNull Context context) {
        if (properties == null)
            properties = new TermuxAppSharedProperties(context);

        return properties;
    }

    /**
     * Get the {@link #properties}.
     *
     * @return Returns the {@link TermuxAppSharedProperties}.
     */
    public static TermuxAppSharedProperties getProperties() {
        return properties;
    }

}
