package com.termux.shared.terminal.io.extrakeys;

import android.view.KeyEvent;

import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ExtraKeysConstants {

    /** Defines the repetitive keys that can be passed to {@link ExtraKeysView#setRepetitiveKeys(List)}. */
    public static List<String> PRIMARY_REPETITIVE_KEYS = Arrays.asList("UP", "DOWN", "LEFT", "RIGHT", "BKSP", "DEL");



    /** Defines the {@link KeyEvent} for common keys. */
    public static Map<String, Integer> PRIMARY_KEY_CODES_FOR_STRINGS = new HashMap<String, Integer>() {{
        put("SPACE", KeyEvent.KEYCODE_SPACE);
        put("ESC", KeyEvent.KEYCODE_ESCAPE);
        put("TAB", KeyEvent.KEYCODE_TAB);
        put("HOME", KeyEvent.KEYCODE_MOVE_HOME);
        put("END", KeyEvent.KEYCODE_MOVE_END);
        put("PGUP", KeyEvent.KEYCODE_PAGE_UP);
        put("PGDN", KeyEvent.KEYCODE_PAGE_DOWN);
        put("INS", KeyEvent.KEYCODE_INSERT);
        put("DEL", KeyEvent.KEYCODE_FORWARD_DEL);
        put("BKSP", KeyEvent.KEYCODE_DEL);
        put("UP", KeyEvent.KEYCODE_DPAD_UP);
        put("LEFT", KeyEvent.KEYCODE_DPAD_LEFT);
        put("RIGHT", KeyEvent.KEYCODE_DPAD_RIGHT);
        put("DOWN", KeyEvent.KEYCODE_DPAD_DOWN);
        put("ENTER", KeyEvent.KEYCODE_ENTER);
        put("F1", KeyEvent.KEYCODE_F1);
        put("F2", KeyEvent.KEYCODE_F2);
        put("F3", KeyEvent.KEYCODE_F3);
        put("F4", KeyEvent.KEYCODE_F4);
        put("F5", KeyEvent.KEYCODE_F5);
        put("F6", KeyEvent.KEYCODE_F6);
        put("F7", KeyEvent.KEYCODE_F7);
        put("F8", KeyEvent.KEYCODE_F8);
        put("F9", KeyEvent.KEYCODE_F9);
        put("F10", KeyEvent.KEYCODE_F10);
        put("F11", KeyEvent.KEYCODE_F11);
        put("F12", KeyEvent.KEYCODE_F12);
    }};



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

    public static class ExtraKeyDisplayMap extends CleverMap<String, String> {}



    /*
     * Multiple maps are available to quickly change
     * the style of the keys.
     */

    public static class EXTRA_KEY_DISPLAY_MAPS {
        /**
         * Keys are displayed in a natural looking way, like "→" for "RIGHT"
         */
        public static final ExtraKeyDisplayMap CLASSIC_ARROWS_DISPLAY = new ExtraKeyDisplayMap() {{
            // classic arrow keys (for ◀ ▶ ▲ ▼ @see arrowVariationDisplay)
            put("LEFT", "←"); // U+2190 ← LEFTWARDS ARROW
            put("RIGHT", "→"); // U+2192 → RIGHTWARDS ARROW
            put("UP", "↑"); // U+2191 ↑ UPWARDS ARROW
            put("DOWN", "↓"); // U+2193 ↓ DOWNWARDS ARROW
        }};

        public static final ExtraKeyDisplayMap WELL_KNOWN_CHARACTERS_DISPLAY = new ExtraKeyDisplayMap() {{
            // well known characters // https://en.wikipedia.org/wiki/{Enter_key, Tab_key, Delete_key}
            put("ENTER", "↲"); // U+21B2 ↲ DOWNWARDS ARROW WITH TIP LEFTWARDS
            put("TAB", "↹"); // U+21B9 ↹ LEFTWARDS ARROW TO BAR OVER RIGHTWARDS ARROW TO BAR
            put("BKSP", "⌫"); // U+232B ⌫ ERASE TO THE LEFT sometimes seen and easy to understand
            put("DEL", "⌦"); // U+2326 ⌦ ERASE TO THE RIGHT not well known but easy to understand
            put("DRAWER", "☰"); // U+2630 ☰ TRIGRAM FOR HEAVEN not well known but easy to understand
            put("KEYBOARD", "⌨"); // U+2328 ⌨ KEYBOARD not well known but easy to understand
            put("PASTE", "⎘"); // U+2398
        }};

        public static final ExtraKeyDisplayMap LESS_KNOWN_CHARACTERS_DISPLAY = new ExtraKeyDisplayMap() {{
            // https://en.wikipedia.org/wiki/{Home_key, End_key, Page_Up_and_Page_Down_keys}
            // home key can mean "goto the beginning of line" or "goto first page" depending on context, hence the diagonal
            put("HOME", "⇱"); // from IEC 9995 // U+21F1 ⇱ NORTH WEST ARROW TO CORNER
            put("END", "⇲"); // from IEC 9995 // ⇲ // U+21F2 ⇲ SOUTH EAST ARROW TO CORNER
            put("PGUP", "⇑"); // no ISO character exists, U+21D1 ⇑ UPWARDS DOUBLE ARROW will do the trick
            put("PGDN", "⇓"); // no ISO character exists, U+21D3 ⇓ DOWNWARDS DOUBLE ARROW will do the trick
        }};

        public static final ExtraKeyDisplayMap ARROW_TRIANGLE_VARIATION_DISPLAY = new ExtraKeyDisplayMap() {{
            // alternative to classic arrow keys
            put("LEFT", "◀"); // U+25C0 ◀ BLACK LEFT-POINTING TRIANGLE
            put("RIGHT", "▶"); // U+25B6 ▶ BLACK RIGHT-POINTING TRIANGLE
            put("UP", "▲"); // U+25B2 ▲ BLACK UP-POINTING TRIANGLE
            put("DOWN", "▼"); // U+25BC ▼ BLACK DOWN-POINTING TRIANGLE
        }};

        public static final ExtraKeyDisplayMap NOT_KNOWN_ISO_CHARACTERS = new ExtraKeyDisplayMap() {{
            // Control chars that are more clear as text // https://en.wikipedia.org/wiki/{Function_key, Alt_key, Control_key, Esc_key}
            // put("FN", "FN"); // no ISO character exists
            put("CTRL", "⎈"); // ISO character "U+2388 ⎈ HELM SYMBOL" is unknown to people and never printed on computers, however "U+25C7 ◇ WHITE DIAMOND" is a nice presentation, and "^" for terminal app and mac is often used
            put("ALT", "⎇"); // ISO character "U+2387 ⎇ ALTERNATIVE KEY SYMBOL'" is unknown to people and only printed as the Option key "⌥" on Mac computer
            put("ESC", "⎋"); // ISO character "U+238B ⎋ BROKEN CIRCLE WITH NORTHWEST ARROW" is unknown to people and not often printed on computers
        }};

        public static final ExtraKeyDisplayMap NICER_LOOKING_DISPLAY = new ExtraKeyDisplayMap() {{
            // nicer looking for most cases
            put("-", "―"); // U+2015 ― HORIZONTAL BAR
        }};

        /**
         * Full Iso
         */
        public static final ExtraKeyDisplayMap FULL_ISO_CHAR_DISPLAY = new ExtraKeyDisplayMap() {{
            putAll(CLASSIC_ARROWS_DISPLAY);
            putAll(WELL_KNOWN_CHARACTERS_DISPLAY);
            putAll(LESS_KNOWN_CHARACTERS_DISPLAY); // NEW
            putAll(NICER_LOOKING_DISPLAY);
            putAll(NOT_KNOWN_ISO_CHARACTERS); // NEW
        }};

        /**
         * Only arrows
         */
        public static final ExtraKeyDisplayMap ARROWS_ONLY_CHAR_DISPLAY = new ExtraKeyDisplayMap() {{
            putAll(CLASSIC_ARROWS_DISPLAY);
            // putAll(wellKnownCharactersDisplay); // REMOVED
            // putAll(lessKnownCharactersDisplay); // REMOVED
            putAll(NICER_LOOKING_DISPLAY);
        }};

        /**
         * Classic symbols and less known symbols
         */
        public static final ExtraKeyDisplayMap LOTS_OF_ARROWS_CHAR_DISPLAY = new ExtraKeyDisplayMap() {{
            putAll(CLASSIC_ARROWS_DISPLAY);
            putAll(WELL_KNOWN_CHARACTERS_DISPLAY);
            putAll(LESS_KNOWN_CHARACTERS_DISPLAY); // NEW
            putAll(NICER_LOOKING_DISPLAY);
        }};

        /**
         * Some classic symbols everybody knows
         */
        public static final ExtraKeyDisplayMap DEFAULT_CHAR_DISPLAY = new ExtraKeyDisplayMap() {{
            putAll(CLASSIC_ARROWS_DISPLAY);
            putAll(WELL_KNOWN_CHARACTERS_DISPLAY);
            putAll(NICER_LOOKING_DISPLAY);
            // all other characters are displayed as themselves
        }};

    }



    /**
     * Aliases for the keys
     */
    public static final ExtraKeyDisplayMap CONTROL_CHARS_ALIASES = new ExtraKeyDisplayMap() {{
        put("ESCAPE", "ESC");
        put("CONTROL", "CTRL");
        put("SHFT", "SHIFT");
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

}
