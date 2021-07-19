package com.termux.app.terminal.io.extrakeys;

import com.termux.shared.logger.Logger;
import com.termux.shared.settings.properties.TermuxPropertyConstants;
import com.termux.shared.settings.properties.TermuxSharedProperties;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.HashMap;

public class ExtraKeysInfo {

    /**
     * Matrix of buttons displayed
     */
    private final ExtraKeyButton[][] buttons;

    /**
     * This corresponds to one of the CharMapDisplay below
     */
    private String style;

    public ExtraKeysInfo(String propertiesInfo, String style) throws JSONException {
        this.style = style;

        // Convert String propertiesInfo to Array of Arrays
        JSONArray arr = new JSONArray(propertiesInfo);
        Object[][] matrix = new Object[arr.length()][];
        for (int i = 0; i < arr.length(); i++) {
            JSONArray line = arr.getJSONArray(i);
            matrix[i] = new Object[line.length()];
            for (int j = 0; j < line.length(); j++) {
                matrix[i][j] = line.get(j);
            }
        }

        // convert matrix to buttons
        this.buttons = new ExtraKeyButton[matrix.length][];
        for (int i = 0; i < matrix.length; i++) {
            this.buttons[i] = new ExtraKeyButton[matrix[i].length];
            for (int j = 0; j < matrix[i].length; j++) {
                Object key = matrix[i][j];

                JSONObject jobject = normalizeKeyConfig(key);

                ExtraKeyButton button;

                if (! jobject.has("popup")) {
                    // no popup
                    button = new ExtraKeyButton(getSelectedCharMap(), jobject);
                } else {
                    // a popup
                    JSONObject popupJobject = normalizeKeyConfig(jobject.get("popup"));
                    ExtraKeyButton popup = new ExtraKeyButton(getSelectedCharMap(), popupJobject);
                    button = new ExtraKeyButton(getSelectedCharMap(), jobject, popup);
                }

                this.buttons[i][j] = button;
            }
        }
    }

    /**
     * "hello" -> {"key": "hello"}
     */
    private static JSONObject normalizeKeyConfig(Object key) throws JSONException {
        JSONObject jobject;
        if (key instanceof String) {
            jobject = new JSONObject();
            jobject.put("key", key);
        } else if (key instanceof JSONObject) {
            jobject = (JSONObject) key;
        } else {
            throw new JSONException("An key in the extra-key matrix must be a string or an object");
        }
        return jobject;
    }

    public ExtraKeyButton[][] getMatrix() {
        return buttons;
    }

    /**
     * HashMap that implements Python dict.get(key, default) function.
     * Default java.util .get(key) is then the same as .get(key, null);
     */
    static class CleverMap<K,V> extends HashMap<K,V> {
        V get(K key, V defaultValue) {
            if (containsKey(key))
                return get(key);
            else
                return defaultValue;
        }
    }

    static class CharDisplayMap extends CleverMap<String, String> {}

    /**
     * Keys are displayed in a natural looking way, like "→" for "RIGHT"
     */
    static final CharDisplayMap classicArrowsDisplay = new CharDisplayMap() {{
        // classic arrow keys (for ◀ ▶ ▲ ▼ @see arrowVariationDisplay)
        put("LEFT", "←"); // U+2190 ← LEFTWARDS ARROW
        put("RIGHT", "→"); // U+2192 → RIGHTWARDS ARROW
        put("UP", "↑"); // U+2191 ↑ UPWARDS ARROW
        put("DOWN", "↓"); // U+2193 ↓ DOWNWARDS ARROW
    }};

    static final CharDisplayMap wellKnownCharactersDisplay = new CharDisplayMap() {{
        // well known characters // https://en.wikipedia.org/wiki/{Enter_key, Tab_key, Delete_key}
        put("ENTER", "↲"); // U+21B2 ↲ DOWNWARDS ARROW WITH TIP LEFTWARDS
        put("TAB", "↹"); // U+21B9 ↹ LEFTWARDS ARROW TO BAR OVER RIGHTWARDS ARROW TO BAR
        put("BKSP", "⌫"); // U+232B ⌫ ERASE TO THE LEFT sometimes seen and easy to understand
        put("DEL", "⌦"); // U+2326 ⌦ ERASE TO THE RIGHT not well known but easy to understand
        put("DRAWER", "☰"); // U+2630 ☰ TRIGRAM FOR HEAVEN not well known but easy to understand
        put("KEYBOARD", "⌨"); // U+2328 ⌨ KEYBOARD not well known but easy to understand
        //put("PASTE", "📋"); // U+2328 ⌨ KEYBOARD not well known but easy to understand
        put("PASTE", "⎘"); // U+2328 ⌨ KEYBOARD not well known but easy to understand
    }};

    static final CharDisplayMap lessKnownCharactersDisplay = new CharDisplayMap() {{
        // https://en.wikipedia.org/wiki/{Home_key, End_key, Page_Up_and_Page_Down_keys}
        // home key can mean "goto the beginning of line" or "goto first page" depending on context, hence the diagonal
        put("HOME", "⇱"); // from IEC 9995 // U+21F1 ⇱ NORTH WEST ARROW TO CORNER
        put("END", "⇲"); // from IEC 9995 // ⇲ // U+21F2 ⇲ SOUTH EAST ARROW TO CORNER
        put("PGUP", "⇑"); // no ISO character exists, U+21D1 ⇑ UPWARDS DOUBLE ARROW will do the trick
        put("PGDN", "⇓"); // no ISO character exists, U+21D3 ⇓ DOWNWARDS DOUBLE ARROW will do the trick
    }};

    static final CharDisplayMap arrowTriangleVariationDisplay = new CharDisplayMap() {{
        // alternative to classic arrow keys
        put("LEFT", "◀"); // U+25C0 ◀ BLACK LEFT-POINTING TRIANGLE
        put("RIGHT", "▶"); // U+25B6 ▶ BLACK RIGHT-POINTING TRIANGLE
        put("UP", "▲"); // U+25B2 ▲ BLACK UP-POINTING TRIANGLE
        put("DOWN", "▼"); // U+25BC ▼ BLACK DOWN-POINTING TRIANGLE
    }};

    static final CharDisplayMap notKnownIsoCharacters = new CharDisplayMap() {{
        // Control chars that are more clear as text // https://en.wikipedia.org/wiki/{Function_key, Alt_key, Control_key, Esc_key}
        // put("FN", "FN"); // no ISO character exists
        put("CTRL", "⎈"); // ISO character "U+2388 ⎈ HELM SYMBOL" is unknown to people and never printed on computers, however "U+25C7 ◇ WHITE DIAMOND" is a nice presentation, and "^" for terminal app and mac is often used
        put("ALT", "⎇"); // ISO character "U+2387 ⎇ ALTERNATIVE KEY SYMBOL'" is unknown to people and only printed as the Option key "⌥" on Mac computer
        put("ESC", "⎋"); // ISO character "U+238B ⎋ BROKEN CIRCLE WITH NORTHWEST ARROW" is unknown to people and not often printed on computers
    }};

    static final CharDisplayMap nicerLookingDisplay = new CharDisplayMap() {{
        // nicer looking for most cases
        put("-", "―"); // U+2015 ― HORIZONTAL BAR
    }};

    /*
     * Multiple maps are available to quickly change
     * the style of the keys.
     */

    /**
     * Some classic symbols everybody knows
     */
    private static final CharDisplayMap defaultCharDisplay = new CharDisplayMap() {{
        putAll(classicArrowsDisplay);
        putAll(wellKnownCharactersDisplay);
        putAll(nicerLookingDisplay);
        // all other characters are displayed as themselves
    }};

    /**
     * Classic symbols and less known symbols
     */
    private static final CharDisplayMap lotsOfArrowsCharDisplay = new CharDisplayMap() {{
        putAll(classicArrowsDisplay);
        putAll(wellKnownCharactersDisplay);
        putAll(lessKnownCharactersDisplay); // NEW
        putAll(nicerLookingDisplay);
    }};

    /**
     * Only arrows
     */
    private static final CharDisplayMap arrowsOnlyCharDisplay = new CharDisplayMap() {{
        putAll(classicArrowsDisplay);
        // putAll(wellKnownCharactersDisplay); // REMOVED
        // putAll(lessKnownCharactersDisplay); // REMOVED
        putAll(nicerLookingDisplay);
    }};

    /**
     * Full Iso
     */
    private static final CharDisplayMap fullIsoCharDisplay = new CharDisplayMap() {{
        putAll(classicArrowsDisplay);
        putAll(wellKnownCharactersDisplay);
        putAll(lessKnownCharactersDisplay); // NEW
        putAll(nicerLookingDisplay);
        putAll(notKnownIsoCharacters); // NEW
    }};

    /**
     * Some people might call our keys differently
     */
    static private final CharDisplayMap controlCharsAliases = new CharDisplayMap() {{
        put("ESCAPE", "ESC");
        put("CONTROL", "CTRL");
        put("RETURN", "ENTER"); // Technically different keys, but most applications won't see the difference
        put("FUNCTION", "FN");
        // no alias for ALT

        // Directions are sometimes written as first and last letter for brevety
        put("LT", "LEFT");
        put("RT", "RIGHT");
        put("DN", "DOWN");
        // put("UP", "UP"); well, "UP" is already two letters

        put("PAGEUP", "PGUP");
        put("PAGE_UP", "PGUP");
        put("PAGE UP", "PGUP");
        put("PAGE-UP", "PGUP");

        // no alias for HOME
        // no alias for END

        put("PAGEDOWN", "PGDN");
        put("PAGE_DOWN", "PGDN");
        put("PAGE-DOWN", "PGDN");

        put("DELETE", "DEL");
        put("BACKSPACE", "BKSP");

        // easier for writing in termux.properties
        put("BACKSLASH", "\\");
        put("QUOTE", "\"");
        put("APOSTROPHE", "'");
    }};

    CharDisplayMap getSelectedCharMap() {
        switch (style) {
            case "arrows-only":
                return arrowsOnlyCharDisplay;
            case "arrows-all":
                return lotsOfArrowsCharDisplay;
            case "all":
                return fullIsoCharDisplay;
            case "none":
                return new CharDisplayMap();
            default:
                if (!TermuxPropertyConstants.DEFAULT_IVALUE_EXTRA_KEYS_STYLE.equals(style))
                    Logger.logError(TermuxSharedProperties.LOG_TAG, "The style \"" + style + "\" for the key \"" + TermuxPropertyConstants.KEY_EXTRA_KEYS_STYLE + "\" is invalid. Using default style instead.");
                return defaultCharDisplay;
        }
    }

    /**
     * Applies the 'controlCharsAliases' mapping to all the strings in *buttons*
     * Modifies the array, doesn't return a new one.
     */
    public static String replaceAlias(String key) {
        return controlCharsAliases.get(key, key);
    }
}

