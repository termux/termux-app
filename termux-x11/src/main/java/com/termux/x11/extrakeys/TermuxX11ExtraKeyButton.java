package com.termux.x11.extrakeys;

import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.stream.Collectors;

public class TermuxX11ExtraKeyButton {

    /** The key name for the name of the extra key if using a dict to define the extra key. {key: name, ...} */
    public static final String KEY_KEY_NAME = "key";

    /** The key name for the macro value of the extra key if using a dict to define the extra key. {macro: value, ...} */
    public static final String KEY_MACRO = "macro";

    /** The key name for the alternate display name of the extra key if using a dict to define the extra key. {display: name, ...} */
    public static final String KEY_DISPLAY_NAME = "display";

    /** The key name for the nested dict to define popup extra key info if using a dict to define the extra key. {popup: {key: name, ...}, ...} */
    public static final String KEY_POPUP = "popup";


    /**
     * The key that will be sent to the terminal, either a control character, like defined in
     * {@link TermuxX11ExtraKeysConstants#PRIMARY_KEY_CODES_FOR_STRINGS} (LEFT, RIGHT, PGUP...) or some text.
     */
    public final String key;

    /**
     * If the key is a macro, i.e. a sequence of keys separated by space.
     */
    public final boolean macro;

    /**
     * The text that will be displayed on the button.
     */
    public final String display;

    /**
     * The {@link TermuxX11ExtraKeyButton} containing the information of the popup button (triggered by swipe up).
     */
    @Nullable
    public final TermuxX11ExtraKeyButton popup;


    /**
     * Initialize a {@link TermuxX11ExtraKeyButton}.
     *
     * @param config The {@link JSONObject} containing the info to create the {@link TermuxX11ExtraKeyButton}.
     * @param extraKeyDisplayMap The {@link TermuxX11ExtraKeysConstants.ExtraKeyDisplayMap} that defines the
     *                           x11 text mapping for the keys if a custom value is not defined
     *                           by {@link #KEY_DISPLAY_NAME}.
     * @param extraKeyAliasMap The {@link TermuxX11ExtraKeysConstants.ExtraKeyDisplayMap} that defines the
     *                           aliases for the actual key names.
     */
    public TermuxX11ExtraKeyButton(@NonNull JSONObject config,
                                   @NonNull TermuxX11ExtraKeysConstants.ExtraKeyDisplayMap extraKeyDisplayMap,
                                   @NonNull TermuxX11ExtraKeysConstants.ExtraKeyDisplayMap extraKeyAliasMap) throws JSONException {
        this(config, null, extraKeyDisplayMap, extraKeyAliasMap);
    }

    /**
     * Initialize a {@link TermuxX11ExtraKeyButton}.
     *
     * @param config The {@link JSONObject} containing the info to create the {@link TermuxX11ExtraKeyButton}.
     * @param popup The {@link TermuxX11ExtraKeyButton} optional {@link #popup} button.
     * @param extraKeyDisplayMap The {@link TermuxX11ExtraKeysConstants.ExtraKeyDisplayMap} that defines the
     *                           x11 text mapping for the keys if a custom value is not defined
     *                           by {@link #KEY_DISPLAY_NAME}.
     * @param extraKeyAliasMap The {@link TermuxX11ExtraKeysConstants.ExtraKeyDisplayMap} that defines the
     *                           aliases for the actual key names.
     */
    public TermuxX11ExtraKeyButton(@NonNull JSONObject config, @Nullable TermuxX11ExtraKeyButton popup,
                                   @NonNull TermuxX11ExtraKeysConstants.ExtraKeyDisplayMap extraKeyDisplayMap,
                                   @NonNull TermuxX11ExtraKeysConstants.ExtraKeyDisplayMap extraKeyAliasMap) throws JSONException {
        String keyFromConfig = getStringFromJson(config, KEY_KEY_NAME);
        String macroFromConfig = getStringFromJson(config, KEY_MACRO);
        String[] keys;
        if (keyFromConfig != null && macroFromConfig != null) {
            throw new JSONException("Both key and macro can't be set for the same key. key: \"" + keyFromConfig + "\", macro: \"" + macroFromConfig + "\"");
        } else if (keyFromConfig != null) {
            keys = new String[]{keyFromConfig};
            this.macro = false;
        } else if (macroFromConfig != null) {
            keys = macroFromConfig.split(" ");
            this.macro = true;
        } else {
            throw new JSONException("All keys have to specify either key or macro");
        }

        for (int i = 0; i < keys.length; i++) {
            keys[i] = replaceAlias(extraKeyAliasMap, keys[i]);
        }

        this.key = TextUtils.join(" ", keys);

        String displayFromConfig = getStringFromJson(config, KEY_DISPLAY_NAME);
        if (displayFromConfig != null) {
            this.display = displayFromConfig;
        } else {
            this.display = Arrays.stream(keys)
                .map(key -> extraKeyDisplayMap.get(key, key))
                .collect(Collectors.joining(" "));
        }

        this.popup = popup;
    }

    public String getStringFromJson(@NonNull JSONObject config, @NonNull String key) {
        try {
            return config.getString(key);
        } catch (JSONException e) {
            return null;
        }
    }

    /**
     * Replace the alias with its actual key name if found in extraKeyAliasMap.
     */
    public static String replaceAlias(@NonNull TermuxX11ExtraKeysConstants.ExtraKeyDisplayMap extraKeyAliasMap, String key) {
        return extraKeyAliasMap.get(key, key);
    }
}
