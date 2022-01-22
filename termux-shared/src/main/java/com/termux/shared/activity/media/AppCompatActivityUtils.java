package com.termux.shared.activity.media;

import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.StyleRes;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.appcompat.widget.Toolbar;

import com.termux.shared.logger.Logger;
import com.termux.shared.theme.NightMode;

public class AppCompatActivityUtils {

    private static final String LOG_TAG = "AppCompatActivityUtils";


    /** Set activity night mode.
     *
     * @param activity The host {@link AppCompatActivity}.
     * @param name The {@link String} representing the name for a {@link NightMode}.
     * @param local If set to {@code true}, then a call to {@link AppCompatDelegate#setLocalNightMode(int)}
     *              will be made, otherwise to {@link AppCompatDelegate#setDefaultNightMode(int)}.
     */
    public static void setNightMode(AppCompatActivity activity, String name, boolean local) {
        if (name == null) return;
        NightMode nightMode = NightMode.modeOf(name);
        if (nightMode != null) {
            if (local) {
                if (activity != null) {
                    activity.getDelegate().setLocalNightMode(nightMode.getMode());
                }
            } else {
                AppCompatDelegate.setDefaultNightMode(nightMode.getMode());
            }
        }

    }

    /** Set activity toolbar.
     *
     * @param activity The host {@link AppCompatActivity}.
     * @param id The toolbar resource id.
     */
    public static void setToolbar(@NonNull AppCompatActivity activity, @IdRes int id) {
        Toolbar toolbar = activity.findViewById(id);
        if (toolbar != null)
            activity.setSupportActionBar(toolbar);
    }

    /** Set activity toolbar title.
     *
     * @param activity The host {@link AppCompatActivity}.
     * @param id The toolbar resource id.
     * @param title The toolbar title {@link String}.
     * @param titleAppearance The toolbar title TextAppearance resource id.
     */
    public static void setToolbarTitle(@NonNull AppCompatActivity activity, @IdRes int id,
                                       String title, @StyleRes int titleAppearance) {
        Toolbar toolbar = activity.findViewById(id);
        if (toolbar != null) {
            //toolbar.setTitle(title); // Does not work
            final ActionBar actionBar = activity.getSupportActionBar();
            if (actionBar != null)
                actionBar.setTitle(title);

            try {
                if (titleAppearance != 0)
                    toolbar.setTitleTextAppearance(activity, titleAppearance);
            } catch (Exception e) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Failed to set toolbar title appearance to style resource id " + titleAppearance, e);
            }


        }
    }

    /** Set activity toolbar subtitle.
     *
     * @param activity The host {@link AppCompatActivity}.
     * @param id The toolbar resource id.
     * @param subtitle The toolbar subtitle {@link String}.
     * @param subtitleAppearance The toolbar subtitle TextAppearance resource id.
     */
    public static void setToolbarSubtitle(@NonNull AppCompatActivity activity, @IdRes int id,
                                          String subtitle, @StyleRes int subtitleAppearance) {
        Toolbar toolbar = activity.findViewById(id);
        if (toolbar != null) {
            toolbar.setSubtitle(subtitle);
            try {
                if (subtitleAppearance != 0)
                    toolbar.setSubtitleTextAppearance(activity, subtitleAppearance);
            } catch (Exception e) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Failed to set toolbar subtitle appearance to style resource id " + subtitleAppearance, e);
            }
        }
    }


    /** Set whether back button should be shown in activity toolbar.
     *
     * @param activity The host {@link AppCompatActivity}.
     * @param showBackButtonInActionBar Set to {@code true} to enable and {@code false} to disable.
     */
    public static void setShowBackButtonInActionBar(@NonNull AppCompatActivity activity,
                                                    boolean showBackButtonInActionBar) {
        final ActionBar actionBar = activity.getSupportActionBar();
        if (actionBar != null) {
            if (showBackButtonInActionBar) {
                actionBar.setDisplayHomeAsUpEnabled(true);
                actionBar.setDisplayShowHomeEnabled(true);
            } else {
                actionBar.setDisplayHomeAsUpEnabled(false);
                actionBar.setDisplayShowHomeEnabled(false);
            }
        }
    }

}
