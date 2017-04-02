package com.termux.window;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.view.WindowManager;

public class TermuxFloatPrefs {

    private static final String PREF_X = "window_x";
    private static final String PREF_Y = "window_y";
    private static final String PREF_WIDTH = "window_width";
    private static final String PREF_HEIGHT = "window_height";

    public static void saveWindowSize(Context context, int width, int height) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        prefs.edit().putInt(PREF_WIDTH, width).putInt(PREF_HEIGHT, height).apply();
    }

    public static void saveWindowPosition(Context context, int x, int y) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        prefs.edit().putInt(PREF_X, x).putInt(PREF_Y, y).apply();
    }

    public static void applySavedGeometry(Context context, WindowManager.LayoutParams layout) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
        layout.x = prefs.getInt(PREF_X, 200);
        layout.y = prefs.getInt(PREF_Y, 200);
        layout.width = prefs.getInt(PREF_WIDTH, 500);
        layout.height = prefs.getInt(PREF_HEIGHT, 800);
    }

}
