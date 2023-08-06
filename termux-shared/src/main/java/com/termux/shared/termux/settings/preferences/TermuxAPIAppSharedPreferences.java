package com.termux.shared.termux.settings.preferences;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.android.PackageUtils;
import com.termux.shared.settings.preferences.AppSharedPreferences;
import com.termux.shared.settings.preferences.SharedPreferenceUtils;
import com.termux.shared.termux.TermuxUtils;
import com.termux.shared.termux.settings.preferences.TermuxPreferenceConstants.TERMUX_API_APP;
import com.termux.shared.termux.TermuxConstants;

public class TermuxAPIAppSharedPreferences extends AppSharedPreferences {

    private static final String LOG_TAG = "TermuxAPIAppSharedPreferences";

    private TermuxAPIAppSharedPreferences(@NonNull Context context) {
        super(context,
            SharedPreferenceUtils.getPrivateSharedPreferences(context,
                TermuxConstants.TERMUX_API_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION),
            SharedPreferenceUtils.getPrivateAndMultiProcessSharedPreferences(context,
                TermuxConstants.TERMUX_API_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION));
    }

    /**
     * Get {@link TermuxAPIAppSharedPreferences}.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the
     *                {@link TermuxConstants#TERMUX_API_PACKAGE_NAME}.
     * @return Returns the {@link TermuxAPIAppSharedPreferences}. This will {@code null} if an exception is raised.
     */
    @Nullable
    public static TermuxAPIAppSharedPreferences build(@NonNull final Context context) {
        Context termuxAPIPackageContext = PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_API_PACKAGE_NAME);
        if (termuxAPIPackageContext == null)
            return null;
        else
            return new TermuxAPIAppSharedPreferences(termuxAPIPackageContext);
    }

    /**
     * Get {@link TermuxAPIAppSharedPreferences}.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the
     *                {@link TermuxConstants#TERMUX_API_PACKAGE_NAME}.
     * @param exitAppOnError If {@code true} and failed to get package context, then a dialog will
     *                       be shown which when dismissed will exit the app.
     * @return Returns the {@link TermuxAPIAppSharedPreferences}. This will {@code null} if an exception is raised.
     */
    public static TermuxAPIAppSharedPreferences build(@NonNull final Context context, final boolean exitAppOnError) {
        Context termuxAPIPackageContext = TermuxUtils.getContextForPackageOrExitApp(context, TermuxConstants.TERMUX_API_PACKAGE_NAME, exitAppOnError);
        if (termuxAPIPackageContext == null)
            return null;
        else
            return new TermuxAPIAppSharedPreferences(termuxAPIPackageContext);
    }



    public int getLogLevel(boolean readFromFile) {
        if (readFromFile)
            return SharedPreferenceUtils.getInt(mMultiProcessSharedPreferences, TERMUX_API_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
        else
            return SharedPreferenceUtils.getInt(mSharedPreferences, TERMUX_API_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
    }

    public void setLogLevel(Context context, int logLevel, boolean commitToFile) {
        logLevel = Logger.setLogLevel(context, logLevel);
        SharedPreferenceUtils.setInt(mSharedPreferences, TERMUX_API_APP.KEY_LOG_LEVEL, logLevel, commitToFile);
    }


    public int getLastPendingIntentRequestCode() {
        return SharedPreferenceUtils.getInt(mSharedPreferences, TERMUX_API_APP.KEY_LAST_PENDING_INTENT_REQUEST_CODE, TERMUX_API_APP.DEFAULT_VALUE_KEY_LAST_PENDING_INTENT_REQUEST_CODE);
    }

    public void setLastPendingIntentRequestCode(int lastPendingIntentRequestCode) {
        SharedPreferenceUtils.setInt(mSharedPreferences, TERMUX_API_APP.KEY_LAST_PENDING_INTENT_REQUEST_CODE, lastPendingIntentRequestCode, true);
    }

}
