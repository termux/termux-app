package com.termux.shared.settings.preferences;

import android.content.Context;
import android.content.SharedPreferences;

import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.settings.preferences.TermuxPreferenceConstants.TERMUX_TASKER_APP;
import com.termux.shared.data.DataUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxUtils;

import javax.annotation.Nonnull;

public class TermuxTaskerAppSharedPreferences {

    private final Context mContext;
    private final SharedPreferences mSharedPreferences;
    private final SharedPreferences mMultiProcessSharedPreferences;


    private static final String LOG_TAG = "TermuxTaskerAppSharedPreferences";

    public TermuxTaskerAppSharedPreferences(@Nonnull Context context) {
        // We use the default context if failed to get termux-tasker package context
        mContext = DataUtils.getDefaultIfNull(TermuxUtils.getTermuxTaskerPackageContext(context), context);
        mSharedPreferences = getPrivateSharedPreferences(mContext);
        mMultiProcessSharedPreferences = getPrivateAndMultiProcessSharedPreferences(mContext);
    }

    private static SharedPreferences getPrivateSharedPreferences(Context context) {
        return SharedPreferenceUtils.getPrivateSharedPreferences(context, TermuxConstants.TERMUX_TASKER_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION);
    }

    private static SharedPreferences getPrivateAndMultiProcessSharedPreferences(Context context) {
        return SharedPreferenceUtils.getPrivateAndMultiProcessSharedPreferences(context, TermuxConstants.TERMUX_TASKER_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION);
    }



    public int getLogLevel(boolean readFromFfile) {
        if(readFromFfile)
            return SharedPreferenceUtils.getInt(mMultiProcessSharedPreferences, TERMUX_TASKER_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
        else
            return SharedPreferenceUtils.getInt(mSharedPreferences, TERMUX_TASKER_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
    }

    public void setLogLevel(Context context, int logLevel, boolean commitToFile) {
        logLevel = Logger.setLogLevel(context, logLevel);
        SharedPreferenceUtils.setInt(mSharedPreferences, TERMUX_TASKER_APP.KEY_LOG_LEVEL, logLevel, commitToFile);
    }

}
