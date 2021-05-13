package com.termux.shared.settings.preferences;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;

import androidx.annotation.NonNull;

import com.termux.shared.packages.PackageUtils;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.settings.preferences.TermuxPreferenceConstants.TERMUX_TASKER_APP;
import com.termux.shared.logger.Logger;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

public class TermuxTaskerAppSharedPreferences {

    private final Context mContext;
    private final SharedPreferences mSharedPreferences;
    private final SharedPreferences mMultiProcessSharedPreferences;


    private static final String LOG_TAG = "TermuxTaskerAppSharedPreferences";

    private  TermuxTaskerAppSharedPreferences(@Nonnull Context context) {
        mContext = context;
        mSharedPreferences = getPrivateSharedPreferences(mContext);
        mMultiProcessSharedPreferences = getPrivateAndMultiProcessSharedPreferences(mContext);
    }

    /**
     * Get the {@link Context} for a package name.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the
     *                {@link TermuxConstants#TERMUX_TASKER_PACKAGE_NAME}.
     * @return Returns the {@link TermuxTaskerAppSharedPreferences}. This will {@code null} if an exception is raised.
     */
    @Nullable
    public static TermuxTaskerAppSharedPreferences build(@NonNull final Context context) {
        Context termuxTaskerPackageContext = PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_TASKER_PACKAGE_NAME);
        if (termuxTaskerPackageContext == null)
            return null;
        else
            return new  TermuxTaskerAppSharedPreferences(termuxTaskerPackageContext);
    }

    /**
     * Get the {@link Context} for a package name.
     *
     * @param context The {@link Activity} to use to get the {@link Context} of the
     *                {@link TermuxConstants#TERMUX_TASKER_PACKAGE_NAME}.
     * @param exitAppOnError If {@code true} and failed to get package context, then a dialog will
     *                       be shown which when dismissed will exit the app.
     * @return Returns the {@link TermuxAppSharedPreferences}. This will {@code null} if an exception is raised.
     */
    public static  TermuxTaskerAppSharedPreferences build(@NonNull final Context context, final boolean exitAppOnError) {
        Context termuxTaskerPackageContext = PackageUtils.getContextForPackageOrExitApp(context, TermuxConstants.TERMUX_TASKER_PACKAGE_NAME, exitAppOnError);
        if (termuxTaskerPackageContext == null)
            return null;
        else
            return new  TermuxTaskerAppSharedPreferences(termuxTaskerPackageContext);
    }

    private static SharedPreferences getPrivateSharedPreferences(Context context) {
        if (context == null) return null;
        return SharedPreferenceUtils.getPrivateSharedPreferences(context, TermuxConstants.TERMUX_TASKER_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION);
    }

    private static SharedPreferences getPrivateAndMultiProcessSharedPreferences(Context context) {
        if (context == null) return null;
        return SharedPreferenceUtils.getPrivateAndMultiProcessSharedPreferences(context, TermuxConstants.TERMUX_TASKER_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION);
    }



    public int getLogLevel(boolean readFromFfile) {
        if (readFromFfile)
            return SharedPreferenceUtils.getInt(mMultiProcessSharedPreferences, TERMUX_TASKER_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
        else
            return SharedPreferenceUtils.getInt(mSharedPreferences, TERMUX_TASKER_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
    }

    public void setLogLevel(Context context, int logLevel, boolean commitToFile) {
        logLevel = Logger.setLogLevel(context, logLevel);
        SharedPreferenceUtils.setInt(mSharedPreferences, TERMUX_TASKER_APP.KEY_LOG_LEVEL, logLevel, commitToFile);
    }

}
