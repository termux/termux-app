package com.termux.app.utils;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.Toast;

import androidx.appcompat.app.AlertDialog;

import com.termux.R;
import com.termux.app.TermuxConstants;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

public class Logger {

    public static final String DEFAULT_LOG_TAG = TermuxConstants.TERMUX_APP_NAME;

    public static final int LOG_LEVEL_OFF = 0; // log nothing
    public static final int LOG_LEVEL_NORMAL = 1; // start logging error, warn and info messages and stacktraces
    public static final int LOG_LEVEL_DEBUG = 2; // start logging debug messages
    public static final int LOG_LEVEL_VERBOSE = 3; // start logging verbose messages

    public static final int DEFAULT_LOG_LEVEL = LOG_LEVEL_NORMAL;

    private static int CURRENT_LOG_LEVEL = DEFAULT_LOG_LEVEL;

    static public void logMesssage(int logLevel, String tag, String message) {
        if(logLevel == Log.ERROR && CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL)
            Log.e(getFullTag(tag), message);
        else if(logLevel == Log.WARN && CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL)
            Log.w(getFullTag(tag), message);
        else if(logLevel == Log.INFO && CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL)
            Log.i(getFullTag(tag), message);
        else if(logLevel == Log.DEBUG && CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG)
            Log.d(getFullTag(tag), message);
        else if(logLevel == Log.VERBOSE && CURRENT_LOG_LEVEL >= LOG_LEVEL_VERBOSE)
            Log.v(getFullTag(tag), message);
    }



    static public void logError(String tag, String message) {
        logMesssage(Log.ERROR, tag, message);
    }

    static public void logError(String message) {
        logMesssage(Log.ERROR, DEFAULT_LOG_TAG, message);
    }



    static public void logWarn(String tag, String message) {
        logMesssage(Log.WARN, tag, message);
    }

    static public void logWarn(String message) {
        logMesssage(Log.WARN, DEFAULT_LOG_TAG, message);
    }



    static public void logInfo(String tag, String message) {
        logMesssage(Log.INFO, tag, message);
    }

    static public void logInfo(String message) {
        logMesssage(Log.INFO, DEFAULT_LOG_TAG, message);
    }



    static public void logDebug(String tag, String message) {
        logMesssage(Log.DEBUG, tag, message);
    }

    static public void logDebug(String message) {
        logMesssage(Log.DEBUG, DEFAULT_LOG_TAG, message);
    }



    static public void logVerbose(String tag, String message) {
        logMesssage(Log.VERBOSE, tag, message);
    }

    static public void logVerbose(String message) {
        logMesssage(Log.VERBOSE, DEFAULT_LOG_TAG, message);
    }



    static public void logErrorAndShowToast(Context context, String tag, String message) {
        if (context == null) return;

        if(CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL) {
            logError(tag, message);
            showToast(context, message, true);
        }
    }

    static public void logErrorAndShowToast(Context context, String message) {
        logErrorAndShowToast(context, DEFAULT_LOG_TAG, message);
    }



    static public void logDebugAndShowToast(Context context, String tag, String message) {
        if (context == null) return;

        if(CURRENT_LOG_LEVEL >= LOG_LEVEL_DEBUG) {
            logDebug(tag, message);
            showToast(context, message, true);
        }
    }

    static public void logDebugAndShowToast(Context context, String message) {
        logDebugAndShowToast(context, DEFAULT_LOG_TAG, message);
    }



    static public void logStackTraceWithMessage(String tag, String message, Exception e) {

        if(CURRENT_LOG_LEVEL >= LOG_LEVEL_NORMAL)
        {
            try {
                StringWriter errors = new StringWriter();
                PrintWriter pw = new PrintWriter(errors);
                e.printStackTrace(pw);
                pw.close();
                if(message != null)
                    Log.e(getFullTag(tag), message + ":\n" + errors.toString());
                else
                    Log.e(getFullTag(tag), errors.toString());
                errors.close();
            } catch (IOException e1) {
                e1.printStackTrace();
            }
        }
    }

    static public void logStackTraceWithMessage(String message, Exception e) {
        logStackTraceWithMessage(DEFAULT_LOG_TAG, message, e);
    }

    static public void logStackTrace(String tag, Exception e) {
        logStackTraceWithMessage(tag, null, e);
    }

    static public void logStackTrace(Exception e) {
        logStackTraceWithMessage(DEFAULT_LOG_TAG, null, e);
    }
    


    static public void showToast(final Context context, final String toastText, boolean longDuration) {
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
        if(logLevel >= LOG_LEVEL_OFF && logLevel <= LOG_LEVEL_VERBOSE)
            CURRENT_LOG_LEVEL = logLevel;
        else
            CURRENT_LOG_LEVEL = DEFAULT_LOG_LEVEL;

        if(context != null)
            showToast(context, context.getString(R.string.log_level_value, getLogLevelLabel(context, CURRENT_LOG_LEVEL, false)),true);

        return CURRENT_LOG_LEVEL;
    }

    static public String getFullTag(String tag) {
        if(DEFAULT_LOG_TAG.equals(tag))
            return tag;
        else
            return DEFAULT_LOG_TAG + ":" + tag;
    }

}
