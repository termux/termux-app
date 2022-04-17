package com.termux.shared.android;

import android.content.Context;
import android.content.pm.PackageManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.reflection.ReflectionUtils;

import java.lang.reflect.Method;

public class UserUtils {

    public static final String LOG_TAG = "UserUtils";

    /**
     * Get the user name for user id with a call to {@link #getNameForUidFromPackageManager(Context, int)}
     * and if that fails, then a call to {@link #getNameForUidFromLibcore(int)}.
     *
     * @param context The {@link Context} for operations.
     * @param uid The user id.
     * @return Returns the user name if found, otherwise {@code null}.
     */
    @Nullable
    public static String getNameForUid(@NonNull Context context, int uid) {
        String name = getNameForUidFromPackageManager(context, uid);
        if (name == null)
            name = getNameForUidFromLibcore(uid);
        return name;
    }

    /**
     * Get the user name for user id with a call to {@link PackageManager#getNameForUid(int)}.
     *
     * This will not return user names for non app user id like for root user 0, use {@link #getNameForUidFromLibcore(int)}
     * to get those.
     *
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:frameworks/base/core/java/android/content/pm/PackageManager.java;l=5556
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:frameworks/base/core/java/android/app/ApplicationPackageManager.java;l=1028
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:frameworks/base/services/core/java/com/android/server/pm/PackageManagerService.java;l=10293
     *
     * @param context The {@link Context} for operations.
     * @param uid The user id.
     * @return Returns the user name if found, otherwise {@code null}.
     */
    @Nullable
    public static String getNameForUidFromPackageManager(@NonNull Context context, int uid) {
        if (uid < 0) return null;

        try {
            String name = context.getPackageManager().getNameForUid(uid);
            if (name != null && name.endsWith(":" + uid))
                name = name.replaceAll(":" + uid + "$", ""); // Remove ":<uid>" suffix
            return name;
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to get name for uid \"" + uid + "\" from package manager", e);
            return null;
        }
    }

    /**
     * Get the user name for user id with a call to `Libcore.os.getpwuid()`.
     *
     * This will return user names for non app user id like for root user 0 as well, but this call
     * is expensive due to usage of reflection, and requires hidden API bypass, check
     * {@link ReflectionUtils#bypassHiddenAPIReflectionRestrictions()} for details.
     *
     * `BlockGuardOs` implements the `Os` interface and its instance is stored in `Libcore` class static `os` field.
     * The `getpwuid` method is implemented by `ForwardingOs`, which is the super class of `BlockGuardOs`.
     * The `getpwuid` method returns `StructPasswd` object whose `pw_name` contains the user name for id.
     *
     * https://stackoverflow.com/a/28057167/14686958
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:libcore/luni/src/main/java/libcore/io/Libcore.java;l=39
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:libcore/luni/src/main/java/libcore/io/Os.java;l=279
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:libcore/luni/src/main/java/libcore/io/BlockGuardOs.java
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:libcore/luni/src/main/java/libcore/io/ForwardingOs.java;l=340
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:libcore/luni/src/main/java/android/system/StructPasswd.java
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:bionic/libc/bionic/grp_pwd.cpp;l=553
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:system/core/libcutils/include/private/android_filesystem_config.h;l=43
     *
     * @param uid The user id.
     * @return Returns the user name if found, otherwise {@code null}.
     */
    @Nullable
    public static String getNameForUidFromLibcore(int uid) {
        if (uid < 0) return null;

        ReflectionUtils.bypassHiddenAPIReflectionRestrictions();
        try {
            String libcoreClassName = "libcore.io.Libcore";
            Class<?> clazz = Class.forName(libcoreClassName);
            Object os; // libcore.io.BlockGuardOs
            try {
                os = ReflectionUtils.invokeField(Class.forName(libcoreClassName), "os", null).value;
            } catch (Exception e) {
                // ClassCastException may be thrown
                Logger.logStackTraceWithMessage(LOG_TAG, "Failed to get \"os\" field value for " + libcoreClassName + " class", e);
                return null;
            }

            if (os == null) {
                Logger.logError(LOG_TAG, "Failed to get BlockGuardOs class obj from Libcore");
                return null;
            }

            clazz = os.getClass().getSuperclass();  // libcore.io.ForwardingOs
            if (clazz == null) {
                Logger.logError(LOG_TAG, "Failed to find super class ForwardingOs from object of class " + os.getClass().getName());
                return null;
            }

            Object structPasswd; // android.system.StructPasswd
            try {
                Method getpwuidMethod = ReflectionUtils.getDeclaredMethod(clazz, "getpwuid", int.class);
                if (getpwuidMethod == null) return null;
                structPasswd = ReflectionUtils.invokeMethod(getpwuidMethod, os, uid).value;
            } catch (Exception e) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Failed to invoke getpwuid() method of " + clazz.getName() + " class", e);
                return null;
            }

            if (structPasswd == null) {
                Logger.logError(LOG_TAG, "Failed to get StructPasswd obj from call to ForwardingOs.getpwuid()");
                return null;
            }

            try {
                clazz = structPasswd.getClass();
                return (String) ReflectionUtils.invokeField(clazz, "pw_name", structPasswd).value;
            } catch (Exception e) {
                // ClassCastException may be thrown
                Logger.logStackTraceWithMessage(LOG_TAG, "Failed to get \"pw_name\" field value for " + clazz.getName() + " class", e);
                return null;
            }
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to get name for uid \"" + uid + "\" from Libcore", e);
            return null;
        }
    }

}
