package com.termux.app.kyzz.appearance;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Color;

public class KyzzAppearanceManager {
    private static final String PREF_NAME = "kyzz_appearance_prefs";
    private static final String KEY_WALLPAPER_URI = "wallpaper_uri";
    private static final String KEY_WALLPAPER_OPACITY = "wallpaper_opacity"; // 0 - 100
    private static final String KEY_OVERLAY_COLOR = "overlay_color";
    private static final String KEY_WALLPAPER_BRIGHTNESS = "wallpaper_brightness"; // 0 - 100
    private static final String KEY_THEME_ID = "theme_id";
    private static final String KEY_CUSTOM_BG_COLOR = "custom_bg_color";
    private static final String KEY_CUSTOM_FG_COLOR = "custom_fg_color";
    private static final String KEY_CUSTOM_ACCENT_COLOR = "custom_accent_color";

    private final SharedPreferences mPrefs;

    public KyzzAppearanceManager(Context context) {
        mPrefs = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
    }

    public String getWallpaperUri() {
        return mPrefs.getString(KEY_WALLPAPER_URI, null);
    }

    public void setWallpaperUri(String uri) {
        mPrefs.edit().putString(KEY_WALLPAPER_URI, uri).apply();
    }

    public int getWallpaperOpacity() {
        return mPrefs.getInt(KEY_WALLPAPER_OPACITY, 50); // Default 50%
    }

    public void setWallpaperOpacity(int opacity) {
        mPrefs.edit().putInt(KEY_WALLPAPER_OPACITY, Math.max(0, Math.min(100, opacity))).apply();
    }

    public int getOverlayColor() {
        return mPrefs.getInt(KEY_OVERLAY_COLOR, Color.parseColor("#181824")); // Kyzz dark charcoal
    }

    public void setOverlayColor(int color) {
        mPrefs.edit().putInt(KEY_OVERLAY_COLOR, color).apply();
    }

    public int getWallpaperBrightness() {
        return mPrefs.getInt(KEY_WALLPAPER_BRIGHTNESS, 80);
    }

    public void setWallpaperBrightness(int brightness) {
        mPrefs.edit().putInt(KEY_WALLPAPER_BRIGHTNESS, Math.max(0, Math.min(100, brightness))).apply();
    }

    public String getThemeId() {
        return mPrefs.getString(KEY_THEME_ID, "dark_kyzz");
    }

    public void setThemeId(String themeId) {
        mPrefs.edit().putString(KEY_THEME_ID, themeId).apply();
    }

    public int getCustomBgColor() {
        return mPrefs.getInt(KEY_CUSTOM_BG_COLOR, Color.parseColor("#12121e"));
    }

    public void setCustomBgColor(int color) {
        mPrefs.edit().putInt(KEY_CUSTOM_BG_COLOR, color).apply();
    }

    public int getCustomFgColor() {
        return mPrefs.getInt(KEY_CUSTOM_FG_COLOR, Color.parseColor("#e0e6ed"));
    }

    public void setCustomFgColor(int color) {
        mPrefs.edit().putInt(KEY_CUSTOM_FG_COLOR, color).apply();
    }

    public int getCustomAccentColor() {
        return mPrefs.getInt(KEY_CUSTOM_ACCENT_COLOR, Color.parseColor("#7c4dff"));
    }

    public void setCustomAccentColor(int color) {
        mPrefs.edit().putInt(KEY_CUSTOM_ACCENT_COLOR, color).apply();
    }

    public void resetToDefault() {
        mPrefs.edit().clear().apply();
    }
}
