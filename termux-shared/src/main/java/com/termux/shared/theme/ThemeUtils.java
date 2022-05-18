package com.termux.shared.theme;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.content.res.TypedArray;

import androidx.appcompat.app.AppCompatActivity;

public class ThemeUtils {

    public static final int ATTR_TEXT_COLOR_PRIMARY = android.R.attr.textColorPrimary;
    public static final int ATTR_TEXT_COLOR_SECONDARY = android.R.attr.textColorSecondary;
    public static final int ATTR_TEXT_COLOR = android.R.attr.textColor;
    public static final int ATTR_TEXT_COLOR_LINK = android.R.attr.textColorLink;

    /**
     * Will return true if system has enabled night mode.
     * https://developer.android.com/guide/topics/resources/providing-resources#NightQualifier
     */
    public static boolean isNightModeEnabled(Context context) {
        if (context == null) return false;
        return (context.getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK) == Configuration.UI_MODE_NIGHT_YES;

    }

    /** Will return true if mode is set to {@link NightMode#TRUE}, otherwise will return true if
     * mode is set to {@link NightMode#SYSTEM} and night mode is enabled by system. */
    public static boolean shouldEnableDarkTheme(Context context, String name) {
        boolean isNightMode = (NightMode.TRUE.getName().equals(name));
//        boolean isDarkMode = (NightMode.FALSE.getName().equals(name));
        boolean isSystemMode = (NightMode.SYSTEM.getName().equals(name));

        if(isNightMode)
            return true;
        else if(isSystemMode)
            return isNightModeEnabled(context);
        else {
            return false;
        }
    }

    /** Get {@link #ATTR_TEXT_COLOR_PRIMARY} value being used by current theme. */
    public static int getTextColorPrimary(Context context) {
        return getSystemAttrColor(context, ATTR_TEXT_COLOR_PRIMARY, 0);
    }

    /** Get {@link #ATTR_TEXT_COLOR_SECONDARY} value being used by current theme. */
    public static int getTextColorSecondary(Context context) {
        return getSystemAttrColor(context, ATTR_TEXT_COLOR_SECONDARY, 0);
    }

    /** Get {@link #ATTR_TEXT_COLOR} value being used by current theme. */
    public static int getTextColor(Context context) {
        return getSystemAttrColor(context, ATTR_TEXT_COLOR, 0);
    }

    /** Get {@link #ATTR_TEXT_COLOR_LINK} value being used by current theme. */
    public static int getTextColorLink(Context context) {
        return getSystemAttrColor(context, ATTR_TEXT_COLOR_LINK, 0);
    }

    /**
     * Get a values defined by the current heme listed in attrs.
     *
     * @param context The context for operations. It must be an instance of {@link Activity} or
     *               {@link AppCompatActivity} or one with which a theme attribute can be got.
     *                Do no use application context.
     * @param attr The attr id.
     * @param def The def value to return.
     * @return Returns the {@code attr} value if found, otherwise {@code def}.
     */
    public static int getSystemAttrColor(Context context, int attr, int def) {
        TypedArray typedArray = context.getTheme().obtainStyledAttributes(new int[] { attr });
        int color = typedArray.getColor(0, def);
        typedArray.recycle();
        return color;
    }

}
