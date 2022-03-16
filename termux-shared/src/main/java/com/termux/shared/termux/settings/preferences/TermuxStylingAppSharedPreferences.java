package com.termux.shared.termux.settings.preferences;

import android.content.Context;
import android.content.SharedPreferences;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.android.PackageUtils;
import com.termux.shared.settings.preferences.SharedPreferenceUtils;
import com.termux.shared.termux.TermuxUtils;
import com.termux.shared.termux.settings.preferences.TermuxPreferenceConstants.TERMUX_STYLING_APP;
import com.termux.shared.termux.TermuxConstants;

public class TermuxStylingAppSharedPreferences {

    private final Context mContext;
    private final SharedPreferences mSharedPreferences;
    private final SharedPreferences mMultiProcessSharedPreferences;


    private static final String LOG_TAG = "TermuxStylingAppSharedPreferences";

    private TermuxStylingAppSharedPreferences(@NonNull Context context) {
        mContext = context;
        mSharedPreferences = getPrivateSharedPreferences(mContext);
        mMultiProcessSharedPreferences = getPrivateAndMultiProcessSharedPreferences(mContext);
    }

    /**
     * Get {@link TermuxStylingAppSharedPreferences}.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the
     *                {@link TermuxConstants#TERMUX_STYLING_PACKAGE_NAME}.
     * @return Returns the {@link TermuxStylingAppSharedPreferences}. This will {@code null} if an exception is raised.
     */
    @Nullable
    public static TermuxStylingAppSharedPreferences build(@NonNull final Context context) {
        Context termuxStylingPackageContext = PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_STYLING_PACKAGE_NAME);
        if (termuxStylingPackageContext == null)
            return null;
        else
            return new TermuxStylingAppSharedPreferences(termuxStylingPackageContext);
    }

    /**
     * Get {@link TermuxStylingAppSharedPreferences}.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the
     *                {@link TermuxConstants#TERMUX_STYLING_PACKAGE_NAME}.
     * @param exitAppOnError If {@code true} and failed to get package context, then a dialog will
     *                       be shown which when dismissed will exit the app.
     * @return Returns the {@link TermuxStylingAppSharedPreferences}. This will {@code null} if an exception is raised.
     */
    public static TermuxStylingAppSharedPreferences build(@NonNull final Context context, final boolean exitAppOnError) {
        Context termuxStylingPackageContext = TermuxUtils.getContextForPackageOrExitApp(context, TermuxConstants.TERMUX_STYLING_PACKAGE_NAME, exitAppOnError);
        if (termuxStylingPackageContext == null)
            return null;
        else
            return new TermuxStylingAppSharedPreferences(termuxStylingPackageContext);
    }

    private static SharedPreferences getPrivateSharedPreferences(Context context) {
        if (context == null) return null;
        return SharedPreferenceUtils.getPrivateSharedPreferences(context, TermuxConstants.TERMUX_STYLING_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION);
    }

    private static SharedPreferences getPrivateAndMultiProcessSharedPreferences(Context context) {
        if (context == null) return null;
        return SharedPreferenceUtils.getPrivateAndMultiProcessSharedPreferences(context, TermuxConstants.TERMUX_STYLING_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION);
    }



    public int getLogLevel(boolean readFromFile) {
        if (readFromFile)
            return SharedPreferenceUtils.getInt(mMultiProcessSharedPreferences, TERMUX_STYLING_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
        else
            return SharedPreferenceUtils.getInt(mSharedPreferences, TERMUX_STYLING_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
    }

    public void setLogLevel(Context context, int logLevel, boolean commitToFile) {
        logLevel = Logger.setLogLevel(context, logLevel);
        SharedPreferenceUtils.setInt(mSharedPreferences, TERMUX_STYLING_APP.KEY_LOG_LEVEL, logLevel, commitToFile);
    }

}
