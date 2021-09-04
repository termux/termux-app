package com.termux.app.settings.properties;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.app.terminal.io.KeyboardShortcut;
import com.termux.shared.terminal.io.extrakeys.ExtraKeysConstants;
import com.termux.shared.terminal.io.extrakeys.ExtraKeysConstants.EXTRA_KEY_DISPLAY_MAPS;
import com.termux.shared.terminal.io.extrakeys.ExtraKeysInfo;
import com.termux.shared.logger.Logger;
import com.termux.shared.settings.properties.TermuxPropertyConstants;
import com.termux.shared.settings.properties.TermuxSharedProperties;
import com.termux.shared.termux.TermuxConstants;

import org.json.JSONException;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class TermuxAppSharedProperties extends TermuxSharedProperties {

    private ExtraKeysInfo mExtraKeysInfo;
    private List<KeyboardShortcut> mSessionShortcuts = new ArrayList<>();

    private static final String LOG_TAG = "TermuxAppSharedProperties";

    public TermuxAppSharedProperties(@NonNull Context context) {
        super(context, TermuxConstants.TERMUX_APP_NAME, TermuxPropertyConstants.getTermuxPropertiesFile(),
            TermuxPropertyConstants.TERMUX_PROPERTIES_LIST, new SharedPropertiesParserClient());
    }

    /**
     * Reload the termux properties from disk into an in-memory cache.
     */
    @Override
    public void loadTermuxPropertiesFromDisk() {
        super.loadTermuxPropertiesFromDisk();

        setExtraKeys();
        setSessionShortcuts();
    }

    /**
     * Set the terminal extra keys and style.
     */
    private void setExtraKeys() {
        mExtraKeysInfo = null;

        try {
            // The mMap stores the extra key and style string values while loading properties
            // Check {@link #getExtraKeysInternalPropertyValueFromValue(String)} and
            // {@link #getExtraKeysStyleInternalPropertyValueFromValue(String)}
            String extrakeys = (String) getInternalPropertyValue(TermuxPropertyConstants.KEY_EXTRA_KEYS, true);
            String extraKeysStyle = (String) getInternalPropertyValue(TermuxPropertyConstants.KEY_EXTRA_KEYS_STYLE, true);

            ExtraKeysConstants.ExtraKeyDisplayMap extraKeyDisplayMap = ExtraKeysInfo.getCharDisplayMapForStyle(extraKeysStyle);
            if (EXTRA_KEY_DISPLAY_MAPS.DEFAULT_CHAR_DISPLAY.equals(extraKeyDisplayMap) && !TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE.equals(extraKeysStyle)) {
                Logger.logError(TermuxSharedProperties.LOG_TAG, "The style \"" + extraKeysStyle + "\" for the key \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS_STYLE + "\" is invalid. Using default style instead.");
                extraKeysStyle = TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE;
            }

            mExtraKeysInfo = new ExtraKeysInfo(extrakeys, extraKeysStyle, ExtraKeysConstants.CONTROL_CHARS_ALIASES);
        } catch (JSONException e) {
            Logger.showToast(mContext, "Could not load and set the \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS + "\" property from the properties file: " + e.toString(), true);
            Logger.logStackTraceWithMessage(LOG_TAG, "Could not load and set the \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS + "\" property from the properties file: ", e);

            try {
                mExtraKeysInfo = new ExtraKeysInfo(TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS, TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE, ExtraKeysConstants.CONTROL_CHARS_ALIASES);
            } catch (JSONException e2) {
                Logger.showToast(mContext, "Can't create default extra keys",true);
                Logger.logStackTraceWithMessage(LOG_TAG, "Could create default extra keys: ", e);
                mExtraKeysInfo = null;
            }
        }
    }

    /**
     * Set the terminal sessions shortcuts.
     */
    private void setSessionShortcuts() {
        if (mSessionShortcuts == null)
            mSessionShortcuts = new ArrayList<>();
        else
            mSessionShortcuts.clear();

        // The {@link TermuxPropertyConstants#MAP_SESSION_SHORTCUTS} stores the session shortcut key and action pair
        for (Map.Entry<String, Integer> entry : TermuxPropertyConstants.MAP_SESSION_SHORTCUTS.entrySet()) {
            // The mMap stores the code points for the session shortcuts while loading properties
            Integer codePoint = (Integer) getInternalPropertyValue(entry.getKey(), true);
            // If codePoint is null, then session shortcut did not exist in properties or was invalid
            // as parsed by {@link #getCodePointForSessionShortcuts(String,String)}
            // If codePoint is not null, then get the action for the MAP_SESSION_SHORTCUTS key and
            // add the code point to sessionShortcuts
            if (codePoint != null)
                mSessionShortcuts.add(new KeyboardShortcut(codePoint, entry.getValue()));
        }
    }

    public List<KeyboardShortcut> getSessionShortcuts() {
        return mSessionShortcuts;
    }

    public ExtraKeysInfo getExtraKeysInfo() {
        return mExtraKeysInfo;
    }



    /**
     * Load the {@link TermuxPropertyConstants#KEY_TERMINAL_TRANSCRIPT_ROWS} value from termux properties file on disk.
     */
    public static int getTerminalTranscriptRows(Context context) {
        return  (int) TermuxSharedProperties.getInternalPropertyValue(context, TermuxPropertyConstants.getTermuxPropertiesFile(),
            TermuxPropertyConstants.KEY_TERMINAL_TRANSCRIPT_ROWS, new SharedPropertiesParserClient());
    }

}
