package com.termux.app.crash;

import android.content.Context;

import androidx.annotation.NonNull;

/**
 * Catches uncaught exceptions and logs them.
 */
public class CrashHandler implements Thread.UncaughtExceptionHandler {

    private final Context context;
    private final Thread.UncaughtExceptionHandler defaultUEH;

    private CrashHandler(final Context context) {
        this.context = context;
        this.defaultUEH = Thread.getDefaultUncaughtExceptionHandler();
    }

    public void uncaughtException(@NonNull Thread thread, @NonNull Throwable throwable) {
        CrashUtils.logCrash(context,thread, throwable);
        defaultUEH.uncaughtException(thread, throwable);
    }

    /**
     * Set default uncaught crash handler of current thread to {@link CrashHandler}.
     */
    public static void setCrashHandler(final Context context) {
        if (!(Thread.getDefaultUncaughtExceptionHandler() instanceof CrashHandler)) {
            Thread.setDefaultUncaughtExceptionHandler(new CrashHandler(context));
        }
    }

}
