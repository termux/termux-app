package com.termux.x11.utils;

import static android.view.WindowManager.LayoutParams.FLAG_FULLSCREEN;

import android.app.Activity;
import android.content.SharedPreferences;
import android.graphics.Rect;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.widget.FrameLayout;

@SuppressWarnings("deprecation")
public class FullscreenWorkaround {
    // For more information, see https://issuetracker.google.com/issues/36911528
    // To use this class, simply invoke assistActivity() on an Activity that already has its content view set.

    public static void assistActivity(Activity activity) {
        new FullscreenWorkaround(activity);
    }

    private final Activity mActivity;

    private int usableHeightPrevious;
    private static boolean x11Focused = true;

    public static void setX11Focused(boolean focused) {
        x11Focused = focused;
    }

    private FullscreenWorkaround(Activity activity) {
        mActivity = activity;
        FrameLayout content = activity.findViewById(android.R.id.content);
        content.getViewTreeObserver().addOnGlobalLayoutListener(this::possiblyResizeChildOfContent);
    }

    private void possiblyResizeChildOfContent() {
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(mActivity);
        if (
            !mActivity.hasWindowFocus() ||
                !((mActivity.getWindow().getAttributes().flags & FLAG_FULLSCREEN) == FLAG_FULLSCREEN) ||
                !preferences.getBoolean("Reseed", true) ||
                !x11Focused ||
                !preferences.getBoolean("fullscreen", false) ||
                SamsungDexUtils.checkDeXEnabled(mActivity)
        )
            return;
        FrameLayout content = (FrameLayout) ((FrameLayout) mActivity.findViewById(android.R.id.content)).getChildAt(0);
        FrameLayout.LayoutParams frameLayoutParams = (FrameLayout.LayoutParams) content.getLayoutParams();

        int usableHeightNow = computeUsableHeight(content);
        if (usableHeightNow != usableHeightPrevious) {
            int usableHeightSansKeyboard = content.getRootView().getHeight();
            int heightDifference = usableHeightSansKeyboard - usableHeightNow;
            if (heightDifference > (usableHeightSansKeyboard / 4)) {
                // keyboard probably just became visible
                frameLayoutParams.height = usableHeightSansKeyboard - heightDifference;
            } else {
                // keyboard probably just became hidden
                frameLayoutParams.height = usableHeightSansKeyboard;
            }
            content.requestLayout();
            usableHeightPrevious = usableHeightNow;
        }
    }

    private int computeUsableHeight(View v) {
        Rect r = new Rect();
        v.getWindowVisibleDisplayFrame(r);
        return (r.bottom - r.top);
    }
}
