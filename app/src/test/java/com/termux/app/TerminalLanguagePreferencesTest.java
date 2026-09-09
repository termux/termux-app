package com.termux.app;

import android.app.Application;

import com.termux.shared.termux.settings.TerminalLanguagePolicy;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

/** Rebuilding preferences models a new activity instance without starting terminal processes. */
@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = 28)
public class TerminalLanguagePreferencesTest {

    private TermuxAppSharedPreferences preferences;

    private TermuxAppSharedPreferences rebuildPreferences() {
        TermuxAppSharedPreferences rebuilt = TermuxAppSharedPreferences.build(RuntimeEnvironment.getApplication());
        assertNotNull(rebuilt);
        return rebuilt;
    }

    @Before
    public void clearManualChoice() {
        preferences = rebuildPreferences();
        preferences.clearTerminalRtlTextShapingOverride();
    }

    @Test
    public void absentPreferenceFollowsLanguageWithoutWritingDefault() {
        assertNull(preferences.getTerminalRtlTextShapingOverride());
        assertTrue(TerminalLanguagePolicy.useArabicTerminalFlow("ar", preferences.getTerminalRtlTextShapingOverride(), null));
        assertFalse(TerminalLanguagePolicy.useArabicTerminalFlow("en", preferences.getTerminalRtlTextShapingOverride(), null));
        assertNull(rebuildPreferences().getTerminalRtlTextShapingOverride());
    }

    @Test
    public void manualTrueAndFalseSurviveRecreation() {
        preferences.setTerminalRtlTextShapingOverride(false);
        assertEquals(Boolean.FALSE, rebuildPreferences().getTerminalRtlTextShapingOverride());
        preferences.setTerminalRtlTextShapingOverride(true);
        assertEquals(Boolean.TRUE, rebuildPreferences().getTerminalRtlTextShapingOverride());
    }

    @Test
    public void explicitLanguageChangeCanClearManualChoice() {
        preferences.setTerminalRtlTextShapingOverride(true);
        preferences.clearTerminalRtlTextShapingOverride();
        assertNull(rebuildPreferences().getTerminalRtlTextShapingOverride());
    }
}
