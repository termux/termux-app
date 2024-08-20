package com.termux.x11.extrakeys;

import androidx.annotation.NonNull;

import java.util.HashMap;

/** The {@link Class} that implements special buttons for {@link TermuxExtraKeysView}. */
public class TermuxX11SpecialButton {

    private static final HashMap<String, TermuxX11SpecialButton> map = new HashMap<>();

    public static final TermuxX11SpecialButton CTRL = new TermuxX11SpecialButton("CTRL");
    public static final TermuxX11SpecialButton ALT = new TermuxX11SpecialButton("ALT");
    public static final TermuxX11SpecialButton SHIFT = new TermuxX11SpecialButton("SHIFT");
    public static final TermuxX11SpecialButton META = new TermuxX11SpecialButton("META");
    public static final TermuxX11SpecialButton FN = new TermuxX11SpecialButton("FN");

    /** The special button key. */
    private final String key;

    /**
     * Initialize a {@link TermuxX11SpecialButton}.
     *
     * @param key The unique key name for the special button. The key is registered in {@link #map}
     *            with which the {@link TermuxX11SpecialButton} can be retrieved via a call to
     *            {@link #valueOf(String)}.
     */
    public TermuxX11SpecialButton(@NonNull final String key) {
        this.key = key;
        map.put(key, this);
    }

    /** Get {@link #key} for this {@link TermuxX11SpecialButton}. */
    public String getKey() {
        return key;
    }

    /**
     * Get the {@link TermuxX11SpecialButton} for {@code key}.
     *
     * @param key The unique key name for the special button.
     */
    public static TermuxX11SpecialButton valueOf(String key) {
        return map.get(key);
    }

    @NonNull
    @Override
    public String toString() {
        return key;
    }

}
