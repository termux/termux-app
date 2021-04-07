package com.termux.app.terminal.io.extrakeys;

import android.text.TextUtils;

import androidx.annotation.Nullable;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.stream.Collectors;

public class ExtraKeyButton {

    /**
     * The key that will be sent to the terminal, either a control character
     * defined in ExtraKeysView.keyCodesForString (LEFT, RIGHT, PGUP...) or
     * some text.
     */
    private final String key;

    /**
     * If the key is a macro, i.e. a sequence of keys separated by space.
     */
    private final boolean macro;

    /**
     * The text that will be shown on the button.
     */
    private final String display;

    /**
     * The information of the popup (triggered by swipe up).
     */
    @Nullable
    private ExtraKeyButton popup;

    public ExtraKeyButton(ExtraKeysInfo.CharDisplayMap charDisplayMap, JSONObject config) throws JSONException {
        this(charDisplayMap, config, null);
    }

    public ExtraKeyButton(ExtraKeysInfo.CharDisplayMap charDisplayMap, JSONObject config, @Nullable ExtraKeyButton popup) throws JSONException {
        String keyFromConfig = config.optString("key", null);
        String macroFromConfig = config.optString("macro", null);
        String[] keys;
        if (keyFromConfig != null && macroFromConfig != null) {
            throw new JSONException("Both key and macro can't be set for the same key");
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
            keys[i] = ExtraKeysInfo.replaceAlias(keys[i]);
        }

        this.key = TextUtils.join(" ", keys);

        String displayFromConfig = config.optString("display", null);
        if (displayFromConfig != null) {
            this.display = displayFromConfig;
        } else {
            this.display = Arrays.stream(keys)
                .map(key -> charDisplayMap.get(key, key))
                .collect(Collectors.joining(" "));
        }

        this.popup = popup;
    }

    public String getKey() {
        return key;
    }

    public boolean isMacro() {
        return macro;
    }

    public String getDisplay() {
        return display;
    }

    @Nullable
    public ExtraKeyButton getPopup() {
        return popup;
    }
}
