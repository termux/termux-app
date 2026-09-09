package com.termux.app;

import android.app.Application;
import android.app.LocaleManager;
import android.content.Context;
import android.os.LocaleList;

import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.app.LocaleManagerCompat;

import com.termux.R;
import com.termux.shared.termux.settings.TermuxAppLocaleUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.android.controller.ActivityController;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

import java.util.ArrayDeque;
import java.util.Queue;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/**
 * Exercises AppCompat 1.6.1's real API 33 migration with its worker deliberately delayed until
 * after Activity.onCreate. Only the legacy storage read is a fixture representing API 32 data;
 * tests never read or write AppCompat's internal XML format.
 */
@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = 33, qualifiers = "ar",
    shadows = {TermuxLocaleMigrationTest.LegacyLocaleStorage.class, TermuxAppLocaleTest.PackageLocaleManager.class})
public class TermuxLocaleMigrationTest {

    @Implements(className = "androidx.appcompat.app.AppLocalesStorageHelper", isInAndroidSdk = false)
    public static class LegacyLocaleStorage {
        static String locales;
        static int reads;

        @Implementation
        protected static String readLocales(Context context) {
            reads++;
            return locales;
        }
    }

    private final Queue<Runnable> migrationTasks = new ArrayDeque<>();

    private Context app() {
        return RuntimeEnvironment.getApplication();
    }

    @Before
    public void delayMigration() {
        LegacyLocaleStorage.locales = "ar";
        LegacyLocaleStorage.reads = 0;
        // Test-only scheduling seam: retain AppCompat's SerialExecutor and real migration logic.
        TermuxAppLocaleTest.resetAppCompat(migrationTasks::add);
    }

    private ActivityController<TermuxAppLocaleTest.LocaleActivity> createActivity() {
        return Robolectric.buildActivity(TermuxAppLocaleTest.LocaleActivity.class).create().start().resume();
    }

    private void finishMigration() {
        assertFalse("AppCompat must actually have scheduled migration", migrationTasks.isEmpty());
        while (!migrationTasks.isEmpty()) migrationTasks.remove().run();
    }

    @Test
    public void delayedLegacyArabicImportIsNotReplacedWithEnglishByOnCreate() {
        ActivityController<TermuxAppLocaleTest.LocaleActivity> activity = createActivity();
        assertEquals("New session", activity.get().getString(R.string.action_new_session));
        assertTrue(AppCompatDelegate.getApplicationLocales().isEmpty());
        assertTrue(LocaleManagerCompat.getApplicationLocales(app()).isEmpty());
        assertEquals(0, LegacyLocaleStorage.reads);
        // Service resource lookup during the same race must also be read-only.
        assertEquals("en", TermuxAppLocaleUtils.getSelectedLanguage(app()));
        assertEquals("New session", TermuxAppLocaleUtils.getLocalizedContext(app()).getString(R.string.action_new_session));
        assertTrue(LocaleManagerCompat.getApplicationLocales(app()).isEmpty());

        finishMigration();
        assertEquals(1, LegacyLocaleStorage.reads);
        assertEquals("ar", LocaleManagerCompat.getApplicationLocales(app()).toLanguageTags());
        assertEquals("ar", TermuxAppLocaleUtils.getSelectedLanguage(app()));
        activity.pause().stop().destroy();
        // Android recreates activities for a locale change; model the new instance explicitly.
        activity = createActivity();
        assertEquals("جلسة جديدة", activity.get().getString(R.string.action_new_session));
        activity.pause().stop().destroy();
    }

    @Test
    public void freshInstallRemainsEnglishWithNoExplicitStoredChoiceAfterMigration() {
        LegacyLocaleStorage.locales = "";
        ActivityController<TermuxAppLocaleTest.LocaleActivity> activity = createActivity();
        assertTrue(LocaleManagerCompat.getApplicationLocales(app()).isEmpty());
        finishMigration();
        assertTrue(LocaleManagerCompat.getApplicationLocales(app()).isEmpty());
        assertEquals("New session", activity.get().getString(R.string.action_new_session));
        activity.pause().stop().destroy();
    }

    @Test
    public void explicitFrameworkEnglishTakesPrecedenceOverLegacyArabic() {
        app().getSystemService(LocaleManager.class).setApplicationLocales(LocaleList.forLanguageTags("en"));
        ActivityController<TermuxAppLocaleTest.LocaleActivity> activity = createActivity();
        finishMigration();
        assertEquals(0, LegacyLocaleStorage.reads);
        assertEquals("en", LocaleManagerCompat.getApplicationLocales(app()).toLanguageTags());
        assertEquals("New session", activity.get().getString(R.string.action_new_session));
        activity.pause().stop().destroy();
    }
}
