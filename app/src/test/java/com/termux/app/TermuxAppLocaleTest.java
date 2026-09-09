package com.termux.app;

import android.app.Application;
import android.app.LocaleManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.os.IBinder;
import android.os.LocaleList;

import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.app.LocaleManagerCompat;
import androidx.core.os.LocaleListCompat;

import com.termux.R;
import com.termux.shared.termux.activities.TermuxLocaleActivity;
import com.termux.shared.termux.settings.TermuxAppLocaleUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.Robolectric;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.android.controller.ActivityController;
import org.robolectric.android.controller.ServiceController;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;
import org.robolectric.annotation.Resetter;
import org.robolectric.shadows.ShadowLocaleManager;
import org.robolectric.util.ReflectionHelpers;

import java.util.ArrayDeque;
import java.util.Collection;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;
import java.util.Queue;
import java.util.concurrent.Executor;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

/** Real AppCompat lifecycle and public AndroidX storage reads; no direct access to its XML. */
@RunWith(RobolectricTestRunner.class)
@Config(application = Application.class, sdk = {28, 32, 33}, qualifiers = "ar",
    shadows = TermuxAppLocaleTest.PackageLocaleManager.class)
public class TermuxAppLocaleTest {

    /** Robolectric 4.10 stores framework locales per Context; Android stores them per package. */
    @Implements(value = LocaleManager.class, minSdk = 33)
    public static class PackageLocaleManager extends ShadowLocaleManager {
        private static final Map<String, LocaleList> locales = new HashMap<>();

        @Implementation
        @Override
        protected LocaleList getApplicationLocales(String packageName) {
            return locales.getOrDefault(packageName, LocaleList.getEmptyLocaleList());
        }

        @Implementation
        @Override
        protected void setApplicationLocales(String packageName, LocaleList value) {
            locales.put(packageName, value);
        }

        @Resetter
        public static void reset() {
            locales.clear();
        }
    }

    private final Queue<Runnable> localeTasks = new ArrayDeque<>();

    public static class LocaleActivity extends TermuxLocaleActivity {
        @Override
        protected void onCreate(Bundle savedInstanceState) {
            setTheme(androidx.appcompat.R.style.Theme_AppCompat);
            super.onCreate(savedInstanceState);
        }
    }

    public static class LocaleService extends Service {
        @Override
        public IBinder onBind(Intent intent) {
            return null;
        }
    }

    @Before
    public void resetLocaleState() {
        resetAppCompat(Build.VERSION.SDK_INT >= 33 ? localeTasks::add : Runnable::run);
    }

    static void resetAppCompat(Executor executor) {
        if (Build.VERSION.SDK_INT >= 33) PackageLocaleManager.reset();
        // Robolectric resets Android state between methods, but not these AndroidX caches.
        // Do not call setApplicationLocales(empty): that is an explicit request, not a cold start.
        ReflectionHelpers.callStaticMethod(AppCompatDelegate.class, "resetStaticRequestedAndStoredLocales");
        ReflectionHelpers.setStaticField(AppCompatDelegate.class, "sIsAutoStoreLocalesOptedIn", null);
        ReflectionHelpers.setStaticField(AppCompatDelegate.class, "sIsFrameworkSyncChecked", false);
        ((Collection<?>) ReflectionHelpers.getStaticField(AppCompatDelegate.class, "sActivityDelegates")).clear();
        Object serial = ReflectionHelpers.getStaticField(AppCompatDelegate.class, "sSerialExecutorForLocalesStorage");
        ((Queue<?>) ReflectionHelpers.getField(serial, "mTasks")).clear();
        ReflectionHelpers.setField(serial, "mActive", null);
        ReflectionHelpers.setField(serial, "mExecutor", executor);
    }

    private Context app() {
        return RuntimeEnvironment.getApplication();
    }

    private void select(String language) {
        if (Build.VERSION.SDK_INT >= 33) {
            app().getSystemService(LocaleManager.class)
                .setApplicationLocales(LocaleList.forLanguageTags(language));
        } else {
            AppCompatDelegate.setApplicationLocales(LocaleListCompat.forLanguageTags(language));
        }
    }

    private ActivityController<LocaleActivity> createActivity() {
        ActivityController<LocaleActivity> activity = Robolectric.buildActivity(LocaleActivity.class).create().start().resume();
        while (!localeTasks.isEmpty()) localeTasks.remove().run();
        return activity;
    }

    private void destroy(ActivityController<LocaleActivity> activity) {
        activity.pause().stop().destroy();
    }

    private void assertActivityLanguage(String language, String label) {
        ActivityController<LocaleActivity> activity = createActivity();
        assertEquals(language, activity.get().getResources().getConfiguration().getLocales().get(0).getLanguage());
        assertEquals(label, activity.get().getString(R.string.action_new_session));
        destroy(activity);
    }

    @Test
    public void freshActivityOnArabicDeviceUsesEnglishWithoutSavingAChoice() {
        assertEquals("ar", app().getResources().getConfiguration().getLocales().get(0).getLanguage());
        assertActivityLanguage("en", "New session");
        assertTrue(AppCompatDelegate.getApplicationLocales().isEmpty());
        assertTrue(LocaleManagerCompat.getApplicationLocales(app()).isEmpty());
        assertActivityLanguage("en", "New session");
        assertTrue(LocaleManagerCompat.getApplicationLocales(app()).isEmpty());
    }

    @Test
    @Config(qualifiers = "en")
    public void explicitArabicSurvivesActivityRecreation() {
        select("ar");
        assertActivityLanguage("ar", "جلسة جديدة");
        assertActivityLanguage("ar", "جلسة جديدة");
        assertEquals("ar", LocaleManagerCompat.getApplicationLocales(app()).toLanguageTags());
    }

    @Test
    public void explicitEnglishSurvivesActivityRecreationOnArabicDevice() {
        select("en");
        assertActivityLanguage("en", "New session");
        assertActivityLanguage("en", "New session");
        assertEquals("en", LocaleManagerCompat.getApplicationLocales(app()).toLanguageTags());
    }

    @Test
    public void freshServiceOnlyStartUsesEnglishWithoutSavingAChoice() {
        ServiceController<LocaleService> service = Robolectric.buildService(LocaleService.class).create();
        assertEquals("New session", TermuxAppLocaleUtils.getLocalizedContext(service.get()).getString(R.string.action_new_session));
        assertTrue(AppCompatDelegate.getApplicationLocales().isEmpty());
        assertTrue(LocaleManagerCompat.getApplicationLocales(app()).isEmpty());
        service.destroy();
    }

    private void assertServiceColdStart(String language, String label) {
        select(language);
        // Let AppCompat persist via its normal lifecycle on pre-33, then simulate process death
        // using its test reset hook. On 33, Android's LocaleManager owns persistence instead.
        ActivityController<LocaleActivity> activity = createActivity();
        destroy(activity);
        ReflectionHelpers.callStaticMethod(AppCompatDelegate.class, "resetStaticRequestedAndStoredLocales");
        assertTrue(AppCompatDelegate.getApplicationLocales().isEmpty());
        assertEquals(language, LocaleManagerCompat.getApplicationLocales(app()).toLanguageTags());
        ServiceController<LocaleService> service = Robolectric.buildService(LocaleService.class).create();
        assertEquals(language, TermuxAppLocaleUtils.getSelectedLanguage(service.get()));
        assertEquals(label, TermuxAppLocaleUtils.getLocalizedContext(service.get()).getString(R.string.action_new_session));
        assertTrue(AppCompatDelegate.getApplicationLocales().isEmpty());
        service.destroy();
    }

    @Test
    @Config(qualifiers = "en")
    public void serviceOnlyColdStartRestoresStoredArabicWithoutAnActivityDelegate() {
        assertServiceColdStart("ar", "جلسة جديدة");
    }

    @Test
    public void serviceOnlyColdStartRestoresStoredEnglishOnArabicDevice() {
        assertServiceColdStart("en", "New session");
    }

    @Test
    public void resourceFallbackDoesNotReplaceUnsupportedExplicitChoice() {
        select("fr");
        assertEquals("New session", TermuxAppLocaleUtils.getLocalizedContext(app()).getString(R.string.action_new_session));
        if (Build.VERSION.SDK_INT < 33) {
            assertEquals("fr", AppCompatDelegate.getApplicationLocales().toLanguageTags());
        } else {
            assertEquals("fr", LocaleManagerCompat.getApplicationLocales(app()).toLanguageTags());
        }
    }

    @Test
    public void localizedContextPreservesNonLocaleConfigurationAndDoesNotChangeSourceOrDefaultLocale() {
        Configuration overrides = new Configuration();
        overrides.fontScale = 1.4f;
        overrides.densityDpi = 240;
        overrides.orientation = Configuration.ORIENTATION_LANDSCAPE;
        overrides.uiMode = Configuration.UI_MODE_NIGHT_YES;
        Context source = app().createConfigurationContext(overrides);
        Configuration before = new Configuration(source.getResources().getConfiguration());
        Locale defaultBefore = Locale.getDefault();
        Context localized = TermuxAppLocaleUtils.getLocalizedContext(source);
        Configuration after = localized.getResources().getConfiguration();
        assertEquals("en", after.getLocales().get(0).getLanguage());
        assertEquals(before.fontScale, after.fontScale, 0f);
        assertEquals(before.densityDpi, after.densityDpi);
        assertEquals(before.orientation, after.orientation);
        assertEquals(before.uiMode, after.uiMode);
        assertEquals(before, source.getResources().getConfiguration());
        assertEquals(defaultBefore, Locale.getDefault());
    }

    @Test
    public void notificationCountsAndWakeLockAreLocalizedWithoutEnglishPluralRules() {
        select("en");
        Context english = TermuxAppLocaleUtils.getLocalizedContext(app());
        assertEquals("Sessions: 0", TermuxService.getNotificationText(english.getResources(), 0, 0, false));
        assertEquals("Sessions: 1, tasks: 1 (wake lock held)",
            TermuxService.getNotificationText(english.getResources(), 1, 1, true));
        select("ar");
        Context arabic = TermuxAppLocaleUtils.getLocalizedContext(app());
        for (int count : new int[] {0, 1, 2, 3, 11, 100}) {
            String text = TermuxService.getNotificationText(arabic.getResources(), count, count, true);
            assertTrue(text.contains("الجلسات:"));
            assertEquals(count > 0, text.contains("المهام:"));
            assertTrue(text.contains("منع السكون مفعّل"));
            assertFalse(text.contains("session"));
            assertFalse(text.contains("task"));
        }
    }
}
