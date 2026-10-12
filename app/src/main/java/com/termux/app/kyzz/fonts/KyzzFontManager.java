package com.termux.app.kyzz.fonts;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Typeface;

import java.io.File;

public class KyzzFontManager {
    private static final String PREF_NAME = "kyzz_font_prefs";
    private static final String KEY_FONT_PATH = "custom_font_path";
    private static final String KEY_FONT_SIZE = "custom_font_size";
    private static final String KEY_LINE_SPACING = "custom_line_spacing";

    private final SharedPreferences mPrefs;
    private final Context mContext;

    public KyzzFontManager(Context context) {
        mContext = context;
        mPrefs = context.getSharedPreferences(PREF_NAME, Context.MODE_PRIVATE);
    }

    public String getCustomFontPath() {
        return mPrefs.getString(KEY_FONT_PATH, null);
    }

    public void setCustomFontPath(String path) {
        mPrefs.edit().putString(KEY_FONT_PATH, path).apply();
    }

    public int getFontSize() {
        return mPrefs.getInt(KEY_FONT_SIZE, 14);
    }

    public void setFontSize(int size) {
        mPrefs.edit().putInt(KEY_FONT_SIZE, Math.max(8, Math.min(36, size))).apply();
    }

    public float getLineSpacing() {
        return mPrefs.getFloat(KEY_LINE_SPACING, 1.0f);
    }

    public void setLineSpacing(float spacing) {
        mPrefs.edit().putFloat(KEY_LINE_SPACING, Math.max(0.8f, Math.min(2.0f, spacing))).apply();
    }

    public Typeface loadTypeface() {
        String path = getCustomFontPath();
        if (path != null && !path.trim().isEmpty()) {
            File fontFile = new File(path);
            if (fontFile.exists() && fontFile.canRead()) {
                try {
                    return Typeface.createFromFile(fontFile);
                } catch (Exception ignored) {
                    // Fallback to default monospace if font corrupted
                }
            }
        }
        return Typeface.MONOSPACE;
    }

    public boolean isValidFontFile(File file) {
        if (file == null || !file.exists() || !file.canRead() || file.length() == 0) {
            return false;
        }
        try {
            Typeface tf = Typeface.createFromFile(file);
            return tf != null;
        } catch (Exception e) {
            return false;
        }
    }

    public void reset() {
        mPrefs.edit().clear().apply();
    }
}
