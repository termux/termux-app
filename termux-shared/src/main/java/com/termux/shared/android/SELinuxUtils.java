package com.termux.shared.android;

import android.annotation.SuppressLint;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.reflection.ReflectionUtils;

import java.lang.reflect.Method;

public class SELinuxUtils {

    public static final String ANDROID_OS_SELINUX_CLASS = "android.os.SELinux";

    private static final String LOG_TAG = "SELinuxUtils";

    /**
     * Gets the security context of the current process.
     *
     * @return Returns a {@link String} representing the security context of the current process.
     * This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static String getContext() {
        ReflectionUtils.bypassHiddenAPIReflectionRestrictions();
        String methodName = "getContext";
        try {
            @SuppressLint("PrivateApi") Class<?> clazz = Class.forName(ANDROID_OS_SELINUX_CLASS);
            Method method = ReflectionUtils.getDeclaredMethod(clazz, methodName);
            if (method == null) {
                Logger.logError(LOG_TAG, "Failed to get " + methodName + "() method of " + ANDROID_OS_SELINUX_CLASS + " class");
                return null;
            }

            return (String) ReflectionUtils.invokeMethod(method, null).value;
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to call " + methodName + "() method of " + ANDROID_OS_SELINUX_CLASS + " class", e);
            return null;
        }
    }

    /**
     * Get the security context of a given process id.
     *
     * @param pid The pid of process.
     * @return Returns a {@link String} representing the security context of the given pid.
     * This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static String getPidContext(int pid) {
        ReflectionUtils.bypassHiddenAPIReflectionRestrictions();
        String methodName = "getPidContext";
        try {
            @SuppressLint("PrivateApi") Class<?> clazz = Class.forName(ANDROID_OS_SELINUX_CLASS);
            Method method = ReflectionUtils.getDeclaredMethod(clazz, methodName, int.class);
            if (method == null) {
                Logger.logError(LOG_TAG, "Failed to get " + methodName + "() method of " + ANDROID_OS_SELINUX_CLASS + " class");
                return null;
            }

            return (String) ReflectionUtils.invokeMethod(method, null, pid).value;
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to call " + methodName + "() method of " + ANDROID_OS_SELINUX_CLASS + " class", e);
            return null;
        }
    }

    /**
     * Get the security context of a file object.
     *
     * @param path The pathname of the file object.
     * @return Returns a {@link String} representing the security context of the file.
     * This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static String getFileContext(@NonNull String path) {
        ReflectionUtils.bypassHiddenAPIReflectionRestrictions();
        String methodName = "getFileContext";
        try {
            @SuppressLint("PrivateApi") Class<?> clazz = Class.forName(ANDROID_OS_SELINUX_CLASS);
            Method method = ReflectionUtils.getDeclaredMethod(clazz, methodName, String.class);
            if (method == null) {
                Logger.logError(LOG_TAG, "Failed to get " + methodName + "() method of " + ANDROID_OS_SELINUX_CLASS + " class");
                return null;
            }

            return (String) ReflectionUtils.invokeMethod(method, null, path).value;
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to call " + methodName + "() method of " + ANDROID_OS_SELINUX_CLASS + " class", e);
            return null;
        }
    }

}
