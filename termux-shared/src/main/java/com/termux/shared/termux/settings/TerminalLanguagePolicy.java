package com.termux.shared.termux.settings;

/** Pure preference policy, independent of Android configuration and terminal rendering. */
public final class TerminalLanguagePolicy {

    public static final String ENGLISH = "en";
    public static final String ARABIC = "ar";

    private TerminalLanguagePolicy() {}

    /** The system locale is not a selection. Only an explicitly selected Arabic locale enables Arabic. */
    public static String languageForSelection(String language) {
        return ARABIC.equals(language) ? ARABIC : ENGLISH;
    }

    /**
     * Precedence: explicit termux.properties override, then saved manual flow, then app language.
     * The caller clears manualFlow only on an explicit drawer language change, never on recreation.
     * The terminal renderer may temporarily use native layout for a TUI without changing this choice.
     */
    public static boolean useArabicTerminalFlow(String language, Boolean manualFlow, Boolean propertyOverride) {
        if (propertyOverride != null) return propertyOverride;
        if (manualFlow != null) return manualFlow;
        return ARABIC.equals(languageForSelection(language));
    }
}
