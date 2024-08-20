package com.termux.x11.utils;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.util.Log;

import java.lang.reflect.Method;

public class SamsungDexUtils {
    private static final String TAG = SamsungDexUtils.class.getSimpleName();
    static public boolean available() {
        try {
            Class<?> semWindowManager = Class.forName("com.samsung.android.view.SemWindowManager");
            semWindowManager.getMethod("getInstance");
            semWindowManager.getDeclaredMethod("requestMetaKeyEvent", android.content.ComponentName.class, boolean.class);
            return true;
        } catch (Exception ignored) {}
        return false;
    }

    static public void dexMetaKeyCapture(Activity activity, boolean enable) {
        try {
            Class<?> semWindowManager = Class.forName("com.samsung.android.view.SemWindowManager");
            Method getInstanceMethod = semWindowManager.getMethod("getInstance");
            Object manager = getInstanceMethod.invoke(null);

            Method requestMetaKeyEvent = semWindowManager.getDeclaredMethod("requestMetaKeyEvent", android.content.ComponentName.class, boolean.class);
            requestMetaKeyEvent.invoke(manager, activity.getComponentName(), enable);

            Log.d(TAG, "com.samsung.android.view.SemWindowManager.requestMetaKeyEvent: success");
        } catch (Exception it) {
            Log.d(TAG, "Could not call com.samsung.android.view.SemWindowManager.requestMetaKeyEvent ");
            Log.d(TAG, it.getClass().getCanonicalName() + ": " + it.getMessage());
        }
    }

    @SuppressWarnings("JavaReflectionMemberAccess")
    public static boolean checkDeXEnabled(Context ctx) {
        Configuration config = ctx.getResources().getConfiguration();
        try {
            Class<?> c = config.getClass();
            return c.getField("SEM_DESKTOP_MODE_ENABLED").getInt(c)
                    == c.getField("semDesktopModeEnabled").getInt(config);
        } catch (NoSuchFieldException | IllegalArgumentException | IllegalAccessException ignored) {}
        return false;
    }
}
