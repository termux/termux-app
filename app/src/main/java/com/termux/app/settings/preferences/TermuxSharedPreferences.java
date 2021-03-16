package com.termux.app.settings.preferences;

import android.content.Context;
import android.content.SharedPreferences;
import android.util.TypedValue;

import com.termux.app.TermuxConstants;
import com.termux.app.utils.Logger;
import com.termux.app.utils.TermuxUtils;
import com.termux.app.utils.TextDataUtils;
import com.termux.app.settings.preferences.TermuxPreferenceConstants.TERMUX_APP;

import javax.annotation.Nonnull;

public class TermuxSharedPreferences {

    private final Context mContext;
    private final SharedPreferences mSharedPreferences;


    private int MIN_FONTSIZE;
    private int MAX_FONTSIZE;
    private int DEFAULT_FONTSIZE;

    public TermuxSharedPreferences(@Nonnull Context context) {
        mContext = TermuxUtils.getTermuxPackageContext(context);
        mSharedPreferences = getSharedPreferences(mContext);

        setFontVariables(context);
    }

    private static SharedPreferences getSharedPreferences(Context context) {
        return context.getSharedPreferences(TermuxConstants.TERMUX_DEFAULT_PREFERENCES_FILE_BASENAME_WITHOUT_EXTENSION, Context.MODE_PRIVATE);
    }



    public boolean getShowTerminalToolbar() {
        return mSharedPreferences.getBoolean(TERMUX_APP.KEY_SHOW_TERMINAL_TOOLBAR, TERMUX_APP.DEFAULT_VALUE_SHOW_TERMINAL_TOOLBAR);
    }

    public void setShowTerminalToolbar(boolean value) {
        mSharedPreferences.edit().putBoolean(TERMUX_APP.KEY_SHOW_TERMINAL_TOOLBAR, value).apply();
    }

    public boolean toogleShowTerminalToolbar() {
        boolean currentValue = getShowTerminalToolbar();
        setShowTerminalToolbar(!currentValue);
        return !currentValue;
    }



    public boolean getKeepScreenOn() {
        return mSharedPreferences.getBoolean(TERMUX_APP.KEY_KEEP_SCREEN_ON, TERMUX_APP.DEFAULT_VALUE_KEEP_SCREEN_ON);
    }

    public void setKeepScreenOn(boolean value) {
        mSharedPreferences.edit().putBoolean(TERMUX_APP.KEY_KEEP_SCREEN_ON, value).apply();
    }



    private void setFontVariables(Context context) {
        float dipInPixels = TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, context.getResources().getDisplayMetrics());

        // This is a bit arbitrary and sub-optimal. We want to give a sensible default for minimum font size
        // to prevent invisible text due to zoom be mistake:
        MIN_FONTSIZE = (int) (4f * dipInPixels);

        // http://www.google.com/design/spec/style/typography.html#typography-line-height
        int defaultFontSize = Math.round(12 * dipInPixels);
        // Make it divisible by 2 since that is the minimal adjustment step:
        if (defaultFontSize % 2 == 1) defaultFontSize--;

        DEFAULT_FONTSIZE = defaultFontSize;

        MAX_FONTSIZE = 256;
    }

    public int getFontSize() {
        int fontSize;
        String fontString;

        try {
            fontString = mSharedPreferences.getString(TERMUX_APP.KEY_FONTSIZE, Integer.toString(DEFAULT_FONTSIZE));
            if(fontString != null)
                fontSize =  Integer.parseInt(fontString);
            else
                fontSize = DEFAULT_FONTSIZE;
        } catch (NumberFormatException | ClassCastException e) {
            fontSize = DEFAULT_FONTSIZE;
        }
        fontSize = TextDataUtils.clamp(fontSize, MIN_FONTSIZE, MAX_FONTSIZE);

        return fontSize;
    }

    public void setFontSize(String value) {
        mSharedPreferences.edit().putString(TERMUX_APP.KEY_FONTSIZE, value).apply();
    }

    public void changeFontSize(Context context, boolean increase) {

        int fontSize = getFontSize();

        fontSize += (increase ? 1 : -1) * 2;
        fontSize = Math.max(MIN_FONTSIZE, Math.min(fontSize, MAX_FONTSIZE));

        setFontSize(Integer.toString(fontSize));
    }



    public String getCurrentSession() {
        return mSharedPreferences.getString(TERMUX_APP.KEY_CURRENT_SESSION, "");
    }

    public void setCurrentSession(String value) {
        mSharedPreferences.edit().putString(TERMUX_APP.KEY_CURRENT_SESSION, value).apply();
    }



    public int getLogLevel() {
        try {
            return mSharedPreferences.getInt(TERMUX_APP.KEY_LOG_LEVEL, Logger.DEFAULT_LOG_LEVEL);
        }
        catch (Exception e) {
            Logger.logStackTraceWithMessage("Error getting \"" + TERMUX_APP.KEY_LOG_LEVEL + "\" from shared preferences", e);
            return Logger.DEFAULT_LOG_LEVEL;
        }
    }

    public void setLogLevel(Context context, int logLevel) {
        logLevel = Logger.setLogLevel(context, logLevel);
        mSharedPreferences.edit().putInt(TERMUX_APP.KEY_LOG_LEVEL, logLevel).apply();
    }



    public boolean getTerminalViewKeyLoggingEnabled() {
        return mSharedPreferences.getBoolean(TERMUX_APP.KEY_TERMINAL_VIEW_KEY_LOGGING_ENABLED, TERMUX_APP.DEFAULT_VALUE_TERMINAL_VIEW_KEY_LOGGING_ENABLED);
    }

    public void setTerminalViewKeyLoggingEnabled(boolean value) {
        mSharedPreferences.edit().putBoolean(TERMUX_APP.KEY_TERMINAL_VIEW_KEY_LOGGING_ENABLED, value).apply();
    }

}
