package com.termux.app;

import com.termux.shared.termux.settings.TerminalLanguagePolicy;

import org.junit.Test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

public class TerminalLanguagePolicyTest {

    @Test
    public void absentOrUnsupportedSelectionUsesEnglish() {
        assertEquals("en", TerminalLanguagePolicy.languageForSelection(null));
        assertEquals("en", TerminalLanguagePolicy.languageForSelection(""));
        assertEquals("en", TerminalLanguagePolicy.languageForSelection("he"));
        assertEquals("en", TerminalLanguagePolicy.languageForSelection("fa"));
        assertEquals("en", TerminalLanguagePolicy.languageForSelection("en"));
    }

    @Test
    public void explicitlySelectedArabicUsesArabic() {
        assertEquals("ar", TerminalLanguagePolicy.languageForSelection("ar"));
        assertTrue(TerminalLanguagePolicy.useArabicTerminalFlow("ar", null, null));
        assertFalse(TerminalLanguagePolicy.useArabicTerminalFlow("en", null, null));
        assertFalse(TerminalLanguagePolicy.useArabicTerminalFlow(null, null, null));
    }

    @Test
    public void manualChoiceOverridesLanguageInBothDirections() {
        assertFalse(TerminalLanguagePolicy.useArabicTerminalFlow("ar", false, null));
        assertTrue(TerminalLanguagePolicy.useArabicTerminalFlow("en", true, null));
    }

    @Test
    public void explicitPropertyOverridesLanguageAndManualChoice() {
        for (String language : new String[] { "en", "ar" }) {
            for (Boolean manual : new Boolean[] { null, false, true }) {
                assertFalse(TerminalLanguagePolicy.useArabicTerminalFlow(language, manual, false));
                assertTrue(TerminalLanguagePolicy.useArabicTerminalFlow(language, manual, true));
            }
        }
    }

    @Test
    public void reapplyingPolicyDoesNotResetManualChoice() {
        for (int recreation = 0; recreation < 5; recreation++) {
            assertFalse(TerminalLanguagePolicy.useArabicTerminalFlow("ar", false, null));
            assertTrue(TerminalLanguagePolicy.useArabicTerminalFlow("en", true, null));
        }
    }

    @Test
    public void clearingOverrideRestoresSelectedLanguageDefault() {
        assertTrue(TerminalLanguagePolicy.useArabicTerminalFlow("ar", null, null));
        assertFalse(TerminalLanguagePolicy.useArabicTerminalFlow("en", null, null));
    }
}
