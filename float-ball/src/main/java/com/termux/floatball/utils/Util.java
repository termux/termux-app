package com.termux.floatball.utils;

import android.graphics.drawable.Drawable;
import android.os.Build;
import android.view.View;


public class Util {
    public static void setBackground(View view, Drawable drawable) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
            view.setBackground(drawable);
        } else {
            view.setBackgroundDrawable(drawable);
        }
    }

    public static boolean isOnePlus() {
        return getManufacturer().contains("oneplus");
    }

    public static String getManufacturer() {
        String manufacturer = Build.MANUFACTURER;
        manufacturer = manufacturer.toLowerCase();
        return manufacturer;
    }
}
