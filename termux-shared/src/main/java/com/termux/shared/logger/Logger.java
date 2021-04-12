package com.termux.shared.logger;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import com.termux.shared.R;
import com.termux.shared.termux.TermuxConstants;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Collections;
import java.util.List;

public class Logger {

    public static final String DEFAULT_LOG_TAG = TermuxConstants.TERMUX_APP_NAME;

    public static final int LOG_LEVEL_OFF = 0; // log nothing
    public static final int LOG_LEVEL_NORMAL = 1; // start logging error, warn and info messages and stacktraces
    public static final int LOG_LEVEL_DEBUG = 2; // start logging debug messages
    public static final int LOG_LEVEL_VERBOSE = 3; // start logging verbose messages

    public static final int DEFAULT_LOG_LEVEL = LOG_LEVEL_NORMAL;
    private static int CURRENT_LOG_LEVEL = DEFAULT_LOG_LEVEL;

    public static final int LOGGER_ENTRY_SIZE_LIMIT_IN_BYTES = 4 * 1024; // 4KB



    public static void logMessage(int logLevel, String tag, String message) {
        if (logLevel == Log.ERROR && CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL)
            Log.e(getFullTag(tag), message);
        else if (logLevel == Log.WARN && CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL)
            Log.w(getFullTag(tag), message);
        else if (logLevel == Log.INFO && CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL)
            Log.i(getFullTag(tag), message);
        else if (logLevel == Log.DEBUG && CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG)
            Log.d(getFullTag(tag), message);
        else if (logLevel == Log.VERBOSE && CURRENT_LOG_LEVEL >= LOG_LEVEL_VERBOSE)
            Log.v(getFullTag(tag), message);
    }



    public static void logError(String tag, String message) {
        logMessage(Log.ERROR, tag, message);
    }

    public static void logError(String message) {
        logMessage(Log.ERROR, DEFAULT_LOG_TAG, message);
    }



    public static void logWarn(String tag, String message) {
        logMessage(Log.WARN, tag, message);
    }

    public static void logWarn(String message) {
        logMessage(Log.WARN, DEFAULT_LOG_TAG, message);
    }



    public static void logInfo(String tag, String message) {
        logMessage(Log.INFO, tag, message);
    }

    public static void logInfo(String message) {
        logMessage(Log.INFO, DEFAULT_LOG_TAG, message);
    }



    public static void logDebug(String tag, String message) {
        logMessage(Log.DEBUG, tag, message);
    }

    public static void logDebug(String message) {
        logMessage(Log.DEBUG, DEFAULT_LOG_TAG, message);
    }



    public static void logVerbose(String tag, String message) {
        logMessage(Log.VERBOSE, tag, message);
    }

    public static void logVerbose(String message) {
        logMessage(Log.VERBOSE, DEFAULT_LOG_TAG, message);
    }



    public static void logErrorAndShowToast(Context context, String tag, String message) {
        if (context == null) return;

        if (CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL) {
            logError(tag, message);
            showToast(context, message, true);
        }
    }

    public static void logErrorAndShowToast(Context context, String message) {
        logErrorAndShowToast(context, DEFAULT_LOG_TAG, message);
    }



    public static void logDebugAndShowToast(Context context, String tag, String message) {
        if (context == null) return;

        if (CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG) {
            logDebug(tag, message);
            showToast(context, message, true);
        }
    }

    public static void logDebugAndShowToast(Context context, String message) {
        logDebugAndShowToast(context, DEFAULT_LOG_TAG, message);
    }



    public static void logStackTraceWithMessage(String tag, String message, Throwable throwable) {
        if (CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL)
            Log.e(getFullTag(tag), getMessageAndStackTraceString(message, throwable));
    }

    public static void logStackTraceWithMessage(String message, Throwable throwable) {
        logStackTraceWithMessage(DEFAULT_LOG_TAG, message, throwable);
    }

    public static void logStackTrace(String tag, Throwable throwable) {
        logStackTraceWithMessage(tag, null, throwable);
    }

    public static void logStackTrace(Throwable throwable) {
        logStackTraceWithMessage(DEFAULT_LOG_TAG, null, throwable);
    }

    public static void logStackTracesWithMessage(String tag, String message, List<Throwable> throwableList) {
        if (CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL)
            Log.e(getFullTag(tag), getMessageAndStackTracesString(message, throwableList));
    }

    public static String getMessageAndStackTraceString(String message, Throwable throwable) {
        if (message == null && throwable == null)
            return null;
        else if (message != null && throwable != null)
            return message + ":\n" + getStackTraceString(throwable);
        else if (throwable == null)
            return message;
        else
            return getStackTraceString(throwable);
    }

    public static String getMessageAndStackTracesString(String message, List<Throwable> throwableList) {
        if (message == null && (throwableList == null || throwableList.size() == 0))
            return null;
        else if (message != null && (throwableList != null && throwableList.size() != 0))
            return message + ":\n" + getStackTracesString(null, getStackTraceStringArray(throwableList));
        else if (throwableList == null || throwableList.size() == 0)
            return message;
        else
            return getStackTracesString(null, getStackTraceStringArray(throwableList));
    }

    public static String getStackTraceString(Throwable throwable) {
        if (throwable == null) return null;

        String stackTraceString = null;

        try {
            StringWriter errors = new StringWriter();
            PrintWriter pw = new PrintWriter(errors);
            throwable.printStackTrace(pw);
            pw.close();
            stackTraceString = errors.toString();
            errors.close();
        } catch (IOException e1) {
            e1.printStackTrace();
        }

        return stackTraceString;
    }

    public static String[] getStackTraceStringArray(Throwable throwable) {
        return getStackTraceStringArray(Collections.singletonList(throwable));
    }

    public static String[] getStackTraceStringArray(List<Throwable> throwableList) {
        if (throwableList == null) return null;

        final String[] stackTraceStringArray = new String[throwableList.size()];
        for (int i = 0; i < throwableList.size(); i++) {
            stackTraceStringArray[i] = getStackTraceString(throwableList.get(i));
        }
        return stackTraceStringArray;
    }

    public static String getStackTracesString(String label, String[] stackTraceStringArray) {
        if (label == null) label = "StackTraces:";
        StringBuilder stackTracesString = new StringBuilder(label);

        if (stackTraceStringArray == null || stackTraceStringArray.length == 0) {
            stackTracesString.append(" -");
        } else {
            for (int i = 0; i != stackTraceStringArray.length; i++) {
                if (stackTraceStringArray.length > 1)
                    stackTracesString.append("\n\nStacktrace ").append(i + 1);

                stackTracesString.append("\n```\n").append(stackTraceStringArray[i]).append("\n```\n");
            }
        }

        return stackTracesString.toString();
    }

    public static String getStackTracesMarkdownString(String label, String[] stackTraceStringArray) {
        if (label == null) label = "StackTraces";
        StringBuilder stackTracesString = new StringBuilder("### " + label);

        if (stackTraceStringArray == null || stackTraceStringArray.length == 0) {
            stackTracesString.append("\n\n`-`");
        } else {
            for (int i = 0; i != stackTraceStringArray.length; i++) {
                if (stackTraceStringArray.length > 1)
                    stackTracesString.append("\n\n\n#### Stacktrace ").append(i + 1);

                stackTracesString.append("\n\n```\n").append(stackTraceStringArray[i]).append("\n```");
            }
        }

        stackTracesString.append("\n##\n");

        return stackTracesString.toString();
    }

    public static String getSingleLineLogStringEntry(String label, Object object, String def) {
        if (object != null)
            return label + ": `" + object + "`";
        else
            return  label + ": "  +  def;
    }

    public static String getMultiLineLogStringEntry(String label, Object object, String def) {
        if (object != null)
            return label + ":\n```\n" + object + "\n```\n";
        else
            return  label + ": "  +  def;
    }
    


    public static void showToast(final Context context, final String toastText, boolean longDuration) {
        if (context == null) return;

        new Handler(Looper.getMainLooper()).post(() -> Toast.makeText(context, toastText, longDuration ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT).show());
    }



    public static CharSequence[] getLogLevelsArray() {
        return new CharSequence[]{
            String.valueOf(LOG_LEVEL_OFF),
            String.valueOf(LOG_LEVEL_NORMAL),
            String.valueOf(LOG_LEVEL_DEBUG),
            String.valueOf(LOG_LEVEL_VERBOSE)
        };
    }

    public static CharSequence[] getLogLevelLabelsArray(Context context, CharSequence[] logLevels, boolean addDefaultTag) {
        if (logLevels == null) return null;

        CharSequence[] logLevelLabels = new CharSequence[logLevels.length];

        for(int i=0; i<logLevels.length; i++) {
            logLevelLabels[i] = getLogLevelLabel(context, Integer.parseInt(logLevels[i].toString()), addDefaultTag);
        }

       return logLevelLabels;
    }

    public static String getLogLevelLabel(final Context context, final int logLevel, final boolean addDefaultTag) {
        String logLabel;
        switch (logLevel) {
            case LOG_LEVEL_OFF: logLabel = context.getString(R.string.log_level_off); break;
            case LOG_LEVEL_NORMAL: logLabel = context.getString(R.string.log_level_normal); break;
            case LOG_LEVEL_DEBUG: logLabel = context.getString(R.string.log_level_debug); break;
            case LOG_LEVEL_VERBOSE: logLabel = context.getString(R.string.log_level_verbose); break;
            default: logLabel = context.getString(R.string.log_level_unknown); break;
        }

        if (addDefaultTag && logLevel == DEFAULT_LOG_LEVEL)
            return logLabel + " (default)";
        else
            return logLabel;
    }



    public static int getLogLevel() {
        return CURRENT_LOG_LEVEL;
    }

    public static int setLogLevel(Context context, int logLevel) {
        if (logLevel >= LOG_LEVEL_OFF && logLevel <= LOG_LEVEL_VERBOSE)
            CURRENT_LOG_LEVEL = logLevel;
        else
            CURRENT_LOG_LEVEL = DEFAULT_LOG_LEVEL;

        if (context != null)
            showToast(context, context.getString(R.string.log_level_value, getLogLevelLabel(context, CURRENT_LOG_LEVEL, false)),true);

        return CURRENT_LOG_LEVEL;
    }

    public static String getFullTag(String tag) {
        if (DEFAULT_LOG_TAG.equals(tag))
            return tag;
        else
            return DEFAULT_LOG_TAG + ":" + tag;
    }

}
