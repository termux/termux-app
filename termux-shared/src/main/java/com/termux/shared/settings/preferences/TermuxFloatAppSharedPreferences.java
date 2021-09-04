package com.termux.shared.settings.preferences;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.data.DataUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.packages.PackageUtils;
import com.termux.shared.settings.preferences.TermuxPreferenceConstants.TERMUX_FLOAT_APP;
import com.termux.shared.termux.TermuxConstants;

public class TermuxFloatAppSharedPreferences {

    private final Context mContext;
    private final SharedPreferences mSharedPreferences;
    private final SharedPreferences mMultiProcessSharedPreferences;

    private int MIN_FONTSIZE;
    private int MAX_FONTSIZE;
    private int DEFAULT_FONTSIZE;

    private static final String LOG_TAG = "TermuxFloatAppSharedPreferences";

    private TermuxFloatAppSharedPreferences(@NonNull Context context) {
        mContext = context;
        mSharedPreferences = getPrivateSharedPreferences(mContext);
        mMultiProcessSharedPreferences = getPrivateAndMultiProcessSharedPreferences(mContext);

        setFontVariables(context);
    }

    /**
     * Get the {@link Context} for a package name.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the
     *                {@link TermuxConstants#TERMUX_FLOAT_PACKAGE_NAME}.
     * @return Returns the {@link TermuxFloatAppSharedPreferences}. This will {@code null} if an exception is raised.
     */
    @Nullable
    public static TermuxFloatAppSharedPreferences build(@NonNull final Context context) {
        Context termuxFloatPackageContext = PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_FLOAT_PACKAGE_NAME);
        if (termuxFloatPackageContext == null)
            return null;
        else
            return new TermuxFloatAppSharedPreferences(termuxFloatPackageContext);
    }

    /**
     * Get the {@link Context} for a package name.
     *
     * @param context The {@link Activity} to use to get the {@link Context} of the
     *                {@link TermuxConstants#TERMUX_FLOAT_PACKAGE_NAME}.
     * @param exitAppOnError If {@code true} and failed to get package context, then a dialog will
     *                       be shown which when dismissed will exit the app.
     * @return Returns the {@link TermuxFloatAppSharedPreferences}. This will {@code null} if an exception is raised.
     */
    public static TermuxFloatAppSharedPreferences build(@NonNull final Context context, final boolean exitAppOnError) {
        Context termuxFloatPackageContext = PackageUtils.getContextForPackageOrExitApp(context, TermuxConstants.TERMUX_FLOAT_PACKAGE_NAME, exitAppOnError);
        if (termuxFloatPackageContext == null)
            return null;
        else
            return new TermuxFloatAppSharedPreferences(termuxFloatPackageContext);
    }

    private static SharedPreferences getPrivateSharedPreferences(Context context) {
        if (context == null) return null;
        return SharedPreferenceUtils.getPrivateSharedPreferences(context, TermuxConstants.TERMUX_FLOAT_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION);
    }

    private static SharedPreferences getPrivateAndMultiProcessSharedPreferences(Context context) {
        if (context == null) return null;
        return SharedPreferenceUtils.getPrivateAndMultiProcessSharedPreferences(context, TermuxConstants.TERMUX_FLOAT_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION);
    }



    public int getWindowX() {
        return SharedPreferenceUtils.getInt(mSharedPreferences, TERMUX_FLOAT_APP.KEY_WINDOW_X, 200);

    }

    public void setWindowX(int value) {
        SharedPreferenceUtils.setInt(mSharedPreferences, TERMUX_FLOAT_APP.KEY_WINDOW_X, value, false);
    }

    public int getWindowY() {
        return SharedPreferenceUtils.getInt(mSharedPreferences, TERMUX_FLOAT_APP.KEY_WINDOW_Y, 200);

    }

    public void setWindowY(int value) {
        SharedPreferenceUtils.setInt(mSharedPreferences, TERMUX_FLOAT_APP.KEY_WINDOW_Y, value, false);
    }



    public int getWindowWidth() {
        return SharedPreferenceUtils.getInt(mSharedPreferences, TERMUX_FLOAT_APP.KEY_WINDOW_WIDTH, 500);

    }

    public void setWindowWidth(int value) {
        SharedPreferenceUtils.setInt(mSharedPreferences, TERMUX_FLOAT_APP.KEY_WINDOW_WIDTH, value, false);
    }

    public int getWindowHeight() {
        return SharedPreferenceUtils.getInt(mSharedPreferences, TERMUX_FLOAT_APP.KEY_WINDOW_HEIGHT, 500);

    }

    public void setWindowHeight(int value) {
        SharedPreferenceUtils.setInt(mSharedPreferences, TERMUX_FLOAT_APP.KEY_WINDOW_HEIGHT, value, false);
    }



    public void setFontVariables(Context context) {
        int[] sizes = TermuxAppSharedPreferences.getDefaultFontSizes(context);

        DEFAULT_FONTSIZE = sizes[0];
        MIN_FONTSIZE = sizes[1];
        MAX_FONTSIZE = sizes[2];
    }

    public int getFontSize() {
        int fontSize = SharedPreferenceUtils.getIntStoredAsString(mSharedPreferences, TERMUX_FLOAT_APP.KEY_FONTSIZE, DEFAULT_FONTSIZE);
        return DataUtils.clamp(fontSize, MIN_FONTSIZE, MAX_FONTSIZE);
    }

    public void setFontSize(int value) {
        SharedPreferenceUtils.setIntStoredAsString(mSharedPreferences, TERMUX_FLOAT_APP.KEY_FONTSIZE, value, false);
    }

    public void changeFontSize(boolean increase) {
        int fontSize = getFontSize();

        fontSize += (increase ? 1 : -1) * 2;
        fontSize = Math.max(MIN_FONTSIZE, Math.min(fontSize, MAX_FONTSIZE));

        setFontSize(fontSize);
    }


    public int getLogLevel(boolean readFromFile) {
        if (readFromFile)
            return SharedPreferenceUtils.getInt(mMultiProcessSharedPreferences, TERMUX_FLOAT_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
        else
            return SharedPreferenceUtils.getInt(mSharedPreferences, TERMUX_FLOAT_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
    }

    public void setLogLevel(Context context, int logLevel, boolean commitToFile) {
        logLevel = Logger.setLogLevel(context, logLevel);
        SharedPreferenceUtils.setInt(mSharedPreferences, TERMUX_FLOAT_APP.KEY_LOG_LEVEL, logLevel, commitToFile);
    }


    public boolean isTerminalViewKeyLoggingEnabled(boolean readFromFile) {
        if (readFromFile)
            return SharedPreferenceUtils.getBoolean(mMultiProcessSharedPreferences, TERMUX_FLOAT_APP.KEY_TERMINAL_VIEW_KEY_LOGGING_ENABLED, TERMUX_FLOAT_APP.DEFAULT_VALUE_TERMINAL_VIEW_KEY_LOGGING_ENABLED);
        else
            return SharedPreferenceUtils.getBoolean(mSharedPreferences, TERMUX_FLOAT_APP.KEY_TERMINAL_VIEW_KEY_LOGGING_ENABLED, TERMUX_FLOAT_APP.DEFAULT_VALUE_TERMINAL_VIEW_KEY_LOGGING_ENABLED);
    }

    public void setTerminalViewKeyLoggingEnabled(boolean value, boolean commitToFile) {
        SharedPreferenceUtils.setBoolean(mSharedPreferences, TERMUX_FLOAT_APP.KEY_TERMINAL_VIEW_KEY_LOGGING_ENABLED, value, commitToFile);
    }

}
