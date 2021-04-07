package com.termux.shared.crash;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.TermuxUtils;

import java.nio.charset.Charset;

/**
 * Catches uncaught exceptions and logs them.
 */
public class CrashHandler implements Thread.UncaughtExceptionHandler {

    private final Context context;
    private final Thread.UncaughtExceptionHandler defaultUEH;

    private static final String LOG_TAG = "CrashUtils";

    private CrashHandler(final Context context) {
        this.context = context;
        this.defaultUEH = Thread.getDefaultUncaughtExceptionHandler();
    }

    public void uncaughtException(@NonNull Thread thread, @NonNull Throwable throwable) {
        logCrash(context,thread, throwable);
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

    /**
     * Log a crash in the crash log file at
     * {@link TermuxConstants#TERMUX_CRASH_LOG_FILE_PATH}.
     *
     * @param context The {@link Context} for operations.
     * @param thread The {@link Thread} in which the crash happened.
     * @param throwable The {@link Throwable} thrown for the crash.
     */
    public static void logCrash(final Context context, final Thread thread, final Throwable throwable) {

        StringBuilder reportString = new StringBuilder();

        reportString.append("## Crash Details\n");
        reportString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Crash Thread", thread.toString(), "-"));
        reportString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Crash Timestamp", TermuxUtils.getCurrentTimeStamp(), "-"));

        reportString.append("\n\n").append(Logger.getStackTracesMarkdownString("Stacktrace", Logger.getStackTraceStringArray(throwable)));
        reportString.append("\n\n").append(TermuxUtils.getAppInfoMarkdownString(context, true));
        reportString.append("\n\n").append(TermuxUtils.getDeviceInfoMarkdownString(context));

        // Log report string to logcat
        Logger.logError(reportString.toString());

        // Write report string to crash log file
        String errmsg = FileUtils.writeStringToFile(context, "crash log", TermuxConstants.TERMUX_CRASH_LOG_FILE_PATH, Charset.defaultCharset(), reportString.toString(), false);
        if (errmsg != null) {
            Logger.logError(LOG_TAG, errmsg);
        }
    }

}
