package com.termux.shared.termux.settings.properties;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.shared.termux.TermuxConstants;

public class TermuxAppSharedProperties extends TermuxSharedProperties {

    private static TermuxAppSharedProperties properties;

    // --- Cached variables ---
    private boolean mHorizontalScrollEnabled = true;
    private int mHorizontalScrollLeftCode = 66;
    private int mHorizontalScrollRightCode = 67;

    private TermuxAppSharedProperties(@NonNull Context context) {
        super(context, TermuxConstants.TERMUX_APP_NAME,
            TermuxConstants.TERMUX_PROPERTIES_FILE_PATHS_LIST, TermuxPropertyConstants.TERMUX_APP_PROPERTIES_LIST,
            new TermuxSharedProperties.SharedPropertiesParserClient());
        
        // Loading values during initialization
        loadHorizontalScrollProperties();
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

    @Override
    public void loadTermuxPropertiesFromDisk() {
        super.loadTermuxPropertiesFromDisk();
        // Reread configs when calling termux-reload-settings
        loadHorizontalScrollProperties();
    }

    // --- Single load logic ---
    public void loadHorizontalScrollProperties() {
        String enabled = getPropertyValue("horizontal-scroll-enabled", "1", false);
        mHorizontalScrollEnabled = "1".equals(enabled.trim()) || "true".equalsIgnoreCase(enabled.trim());

        try {
            String left = getPropertyValue("horizontal-scroll-left-code", "66", false);
            mHorizontalScrollLeftCode = Integer.parseInt(left.trim());
        } catch (Exception e) {
            mHorizontalScrollLeftCode = 66;
        }

        try {
            String right = getPropertyValue("horizontal-scroll-right-code", "67", false);
            mHorizontalScrollRightCode = Integer.parseInt(right.trim());
        } catch (Exception e) {
            mHorizontalScrollRightCode = 67;
        }
    }

    // --- Getters for quick access ---
    public boolean isHorizontalScrollEnabled() { return mHorizontalScrollEnabled; }
    public int getHorizontalScrollLeftCode() { return mHorizontalScrollLeftCode; }
    public int getHorizontalScrollRightCode() { return mHorizontalScrollRightCode; }

}
