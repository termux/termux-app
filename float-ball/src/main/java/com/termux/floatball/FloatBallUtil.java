package com.termux.floatball;

import android.app.Activity;
import android.content.Context;
import android.graphics.PixelFormat;
import android.os.Build;
import android.view.Gravity;
import android.view.WindowManager;


public class FloatBallUtil {
    public static boolean inSingleActivity;

    public static WindowManager.LayoutParams getLayoutParams(Context context) {
        return getLayoutParams(context, false);
    }

    public static WindowManager.LayoutParams getLayoutParams(Context context, boolean listenBackEvent) {
        WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();

        layoutParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE | WindowManager.LayoutParams.FLAG_WATCH_OUTSIDE_TOUCH
                | WindowManager.LayoutParams.FLAG_LAYOUT_NO_LIMITS | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL;
        if (listenBackEvent) {
            layoutParams.flags = layoutParams.flags & ~WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;
        }
        if (context == null || !(context instanceof Activity)) {
            final int sdkInt = Build.VERSION.SDK_INT;
            if (sdkInt < Build.VERSION_CODES.KITKAT) {
                layoutParams.type = WindowManager.LayoutParams.TYPE_PHONE;
            } else if (sdkInt < Build.VERSION_CODES.N_MR1) {
                if ("Xiaomi".equalsIgnoreCase(Build.MANUFACTURER)) {
                    layoutParams.type = WindowManager.LayoutParams.TYPE_PHONE;
                } else {
                    layoutParams.type = WindowManager.LayoutParams.TYPE_TOAST;
                }
            } else if (sdkInt < Build.VERSION_CODES.O) {
                layoutParams.type = WindowManager.LayoutParams.TYPE_PHONE;
            } else {//8.0以后
                layoutParams.type = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
            }
        } else {
            layoutParams.type = WindowManager.LayoutParams.TYPE_APPLICATION;
        }
        layoutParams.format = PixelFormat.RGBA_8888;
        layoutParams.gravity = Gravity.TOP | Gravity.LEFT;
        layoutParams.width = WindowManager.LayoutParams.WRAP_CONTENT;
        layoutParams.height = WindowManager.LayoutParams.WRAP_CONTENT;
        return layoutParams;
    }

    public static WindowManager.LayoutParams getStatusBarLayoutParams(Context context) {
        WindowManager.LayoutParams layoutParams = new WindowManager.LayoutParams();
        layoutParams.width = 0;
        layoutParams.height = 0;
        layoutParams.flags = WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL;
        layoutParams.gravity = Gravity.LEFT | Gravity.TOP;
        if (context == null || !(context instanceof Activity)) {
            final int sdkInt = Build.VERSION.SDK_INT;
            if (sdkInt < Build.VERSION_CODES.KITKAT) {
                layoutParams.type = WindowManager.LayoutParams.TYPE_PHONE;
            } else if (sdkInt < Build.VERSION_CODES.N_MR1) {
                layoutParams.type = WindowManager.LayoutParams.TYPE_TOAST;
            } else if (sdkInt < Build.VERSION_CODES.O) {
                layoutParams.type = WindowManager.LayoutParams.TYPE_PHONE;
            } else {//8.0以后
                layoutParams.type = WindowManager.LayoutParams.TYPE_APPLICATION_OVERLAY;
            }
        } else {
            layoutParams.type = WindowManager.LayoutParams.TYPE_APPLICATION;
        }
        return layoutParams;
    }
}
