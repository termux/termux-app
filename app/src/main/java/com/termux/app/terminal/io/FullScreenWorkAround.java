package com.termux.app.terminal.io;

import android.graphics.Rect;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;

import com.termux.app.TermuxActivity;

/**
 * Work around for fullscreen mode in Termux to fix ExtraKeysView not being visible.
 * This class is derived from:
 * https://stackoverflow.com/questions/7417123/android-how-to-adjust-layout-in-full-screen-mode-when-softkeyboard-is-visible
 * and has some additional tweaks
 * ---
 * For more information, see https://issuetracker.google.com/issues/36911528
 */
public class FullScreenWorkAround {
    private static FullScreenWorkAround sInstance;

    private final View mChildOfContent;
    private int mUsableHeightPrevious;
    private final ViewGroup.LayoutParams mViewGroupLayoutParams;

    private final int mNavBarHeight;

    private final ViewTreeObserver.OnGlobalLayoutListener mGlobalLayoutListener = this::possiblyResizeChildOfContent;


    public static void apply(TermuxActivity activity) {
        // Remove any previously applied instance so that its global layout listener does not
        // keep running and duplicate listeners do not accumulate on activity recreation.
        remove();
        sInstance = new FullScreenWorkAround(activity);
    }

    /** Remove the global layout listener of the currently applied instance, if any. */
    public static void remove() {
        if (sInstance != null) {
            sInstance.removeGlobalLayoutListener();
            sInstance = null;
        }
    }

    private FullScreenWorkAround(TermuxActivity activity) {
        ViewGroup content = activity.findViewById(android.R.id.content);
        mChildOfContent = content.getChildAt(0);
        mViewGroupLayoutParams = mChildOfContent.getLayoutParams();
        mNavBarHeight = activity.getNavBarHeight();
        mChildOfContent.getViewTreeObserver().addOnGlobalLayoutListener(mGlobalLayoutListener);
    }

    private void removeGlobalLayoutListener() {
        mChildOfContent.getViewTreeObserver().removeOnGlobalLayoutListener(mGlobalLayoutListener);
    }

    private void possiblyResizeChildOfContent() {
        int usableHeightNow = computeUsableHeight();
        if (usableHeightNow != mUsableHeightPrevious) {
            int usableHeightSansKeyboard = mChildOfContent.getRootView().getHeight();
            int heightDifference = usableHeightSansKeyboard - usableHeightNow;
            if (heightDifference > (usableHeightSansKeyboard / 4)) {
                // keyboard probably just became visible

                // ensures that usable layout space does not extend behind the
                // soft keyboard, causing the extra keys to not be visible
                mViewGroupLayoutParams.height = (usableHeightSansKeyboard - heightDifference) + getNavBarHeight();
            } else {
                // keyboard probably just became hidden
                mViewGroupLayoutParams.height = usableHeightSansKeyboard;
            }
            mChildOfContent.requestLayout();
            mUsableHeightPrevious = usableHeightNow;
        }
    }

    private int getNavBarHeight() {
        return mNavBarHeight;
    }

    private int computeUsableHeight() {
        Rect r = new Rect();
        mChildOfContent.getWindowVisibleDisplayFrame(r);
        return (r.bottom - r.top);
    }

}
