package com.termux.shared.view;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.graphics.Rect;
import android.util.TypedValue;
import android.view.View;

import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

public class ViewUtils {

    /**
     * Check if a {@link View} is fully visible and not hidden or partially covered by another view.
     *
     * https://stackoverflow.com/a/51078418/14686958
     *
     * @param view The {@link View} to check.
     * @return Returns {@code true} if view is fully visible.
     */
    public static boolean isViewFullyVisible(View view) {
        Rect[] windowAndViewRects = getWindowAndViewRects(view);
        if (windowAndViewRects == null)
            return false;
        return windowAndViewRects[0].contains(windowAndViewRects[1]);
    }

    /**
     * Get the {@link Rect} of a {@link View} and the  {@link Rect} of the window inside which it
     * exists.
     *
     * https://stackoverflow.com/a/51078418/14686958
     *
     * @param view The {@link View} inside the window whose {@link Rect} to get.
     * @return Returns {@link Rect[]} if view is visible where Rect[0] will contain window
     * {@link Rect} and Rect[1] will contain view {@link Rect}. This will be {@code null}
     * if view is not visible.
     */
    @Nullable
    public static Rect[] getWindowAndViewRects(View view) {
        if (view == null || !view.isShown())
            return null;

        // windowRect - will hold available area where content remain visible to users
        // Takes into account screen decorations (e.g. statusbar)
        Rect windowRect = new Rect();
        view.getWindowVisibleDisplayFrame(windowRect);

        // If there is actionbar, get his height
        int actionBarHeight = 0;
        Context context = view.getContext();
        if (context instanceof AppCompatActivity) {
            androidx.appcompat.app.ActionBar actionBar = ((AppCompatActivity) context).getSupportActionBar();
            if (actionBar != null) actionBarHeight = actionBar.getHeight();
        } else if (context instanceof Activity) {
            android.app.ActionBar actionBar = ((Activity) context).getActionBar();
            if (actionBar != null) actionBarHeight = actionBar.getHeight();
        }

        // windowAvailableRect - takes into account actionbar and statusbar height
        Rect windowAvailableRect;
        windowAvailableRect = new Rect(windowRect.left, windowRect.top + actionBarHeight, windowRect.right, windowRect.bottom);

        // viewRect - holds position of the view in window
        // (methods as getGlobalVisibleRect, getHitRect, getDrawingRect can return different result,
        // when partialy visible)
        Rect viewRect;
        final int[] viewsLocationInWindow = new int[2];
        view.getLocationInWindow(viewsLocationInWindow);
        int viewLeft = viewsLocationInWindow[0];
        int viewTop = viewsLocationInWindow[1];
        int viewRight = viewLeft + view.getWidth();
        int viewBottom = viewTop + view.getHeight();
        viewRect = new Rect(viewLeft, viewTop, viewRight, viewBottom);

        return new Rect[]{windowAvailableRect, viewRect};
    }

    /** Get the {@link Activity} associated with the {@link Context} if available. */
    @Nullable
    public static Activity getActivity(Context context) {
        while (context instanceof ContextWrapper) {
            if (context instanceof Activity) {
                return (Activity)context;
            }
            context = ((ContextWrapper)context).getBaseContext();
        }
        return null;
    }

    /** Convert value in device independent pixels (dp) to pixels (px) units. */
    public static int dpToPx(Context context, int dp) {
        return (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, dp, context.getResources().getDisplayMetrics());
    }

}
