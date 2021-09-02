package com.termux.shared.view;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Configuration;
import android.graphics.Point;
import android.graphics.Rect;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.termux.shared.logger.Logger;

public class ViewUtils {

    /** Log root view events. */
    public static boolean VIEW_UTILS_LOGGING_ENABLED = false;

    private static final String LOG_TAG = "ViewUtils";

    /**
     * Sets whether view utils logging is enabled or not.
     *
     * @param value The boolean value that defines the state.
     */
    public static void setIsViewUtilsLoggingEnabled(boolean value) {
        VIEW_UTILS_LOGGING_ENABLED = value;
    }

    /**
     * Check if a {@link View} is fully visible and not hidden or partially covered by another view.
     *
     * https://stackoverflow.com/a/51078418/14686958
     *
     * @param view The {@link View} to check.
     * @param statusBarHeight The status bar height received by {@link View.OnApplyWindowInsetsListener}.
     * @return Returns {@code true} if view is fully visible.
     */
    public static boolean isViewFullyVisible(View view, int statusBarHeight) {
        Rect[] windowAndViewRects = getWindowAndViewRects(view, statusBarHeight);
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
     * @param statusBarHeight The status bar height received by {@link View.OnApplyWindowInsetsListener}.
     * @return Returns {@link Rect[]} if view is visible where Rect[0] will contain window
     * {@link Rect} and Rect[1] will contain view {@link Rect}. This will be {@code null}
     * if view is not visible.
     */
    @Nullable
    public static Rect[] getWindowAndViewRects(View view, int statusBarHeight) {
        if (view == null || !view.isShown())
            return null;

        boolean view_utils_logging_enabled = VIEW_UTILS_LOGGING_ENABLED;

        // windowRect - will hold available area where content remain visible to users
        // Takes into account screen decorations (e.g. statusbar)
        Rect windowRect = new Rect();
        view.getWindowVisibleDisplayFrame(windowRect);

        // If there is actionbar, get his height
        int actionBarHeight = 0;
        boolean isInMultiWindowMode = false;
        Context context = view.getContext();
        if (context instanceof AppCompatActivity) {
            androidx.appcompat.app.ActionBar actionBar = ((AppCompatActivity) context).getSupportActionBar();
            if (actionBar != null) actionBarHeight = actionBar.getHeight();
            isInMultiWindowMode = ((AppCompatActivity) context).isInMultiWindowMode();
        } else if (context instanceof Activity) {
            android.app.ActionBar actionBar = ((Activity) context).getActionBar();
            if (actionBar != null) actionBarHeight = actionBar.getHeight();
            isInMultiWindowMode = ((Activity) context).isInMultiWindowMode();
        }

        int displayOrientation = getDisplayOrientation(context);

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

        if (view_utils_logging_enabled) {
            Logger.logVerbose(LOG_TAG, "getWindowAndViewRects:");
            Logger.logVerbose(LOG_TAG, "windowRect: " + toRectString(windowRect) + ", windowAvailableRect: " + toRectString(windowAvailableRect));
            Logger.logVerbose(LOG_TAG, "viewsLocationInWindow: " + toPointString(new Point(viewLeft, viewTop)));
            Logger.logVerbose(LOG_TAG, "activitySize: " + toPointString(getDisplaySize(context, true)) +
                ", displaySize: " + toPointString(getDisplaySize(context, false)) +
                ", displayOrientation=" + displayOrientation);
        }

        if (isInMultiWindowMode) {
            if (displayOrientation == Configuration.ORIENTATION_PORTRAIT) {
                // The windowRect.top of the window at the of split screen mode should start right
                // below the status bar
                if (statusBarHeight != windowRect.top) {
                    if (view_utils_logging_enabled)
                        Logger.logVerbose(LOG_TAG, "Window top does not equal statusBarHeight " + statusBarHeight + " in multi-window portrait mode. Window is possibly bottom app in split screen mode. Adding windowRect.top " + windowRect.top + " to viewTop.");
                    viewTop += windowRect.top;
                } else {
                    if (view_utils_logging_enabled)
                        Logger.logVerbose(LOG_TAG, "windowRect.top equals statusBarHeight " + statusBarHeight + " in multi-window portrait mode. Window is possibly top app in split screen mode.");
                }

            } else if (displayOrientation == Configuration.ORIENTATION_LANDSCAPE) {
                // If window is on the right in landscape mode of split screen, the viewLeft actually
                // starts at windowRect.left instead of 0 returned by getLocationInWindow
                viewLeft += windowRect.left;
            }
        }

        int viewRight = viewLeft + view.getWidth();
        int viewBottom = viewTop + view.getHeight();
        viewRect = new Rect(viewLeft, viewTop, viewRight, viewBottom);

        if (displayOrientation == Configuration.ORIENTATION_LANDSCAPE && viewRight > windowAvailableRect.right) {
            if (view_utils_logging_enabled)
                Logger.logVerbose(LOG_TAG, "viewRight " + viewRight + " is greater than windowAvailableRect.right " + windowAvailableRect.right + " in landscape mode. Setting windowAvailableRect.right to viewRight since it may not include navbar height.");
            windowAvailableRect.right = viewRight;
        }

        return new Rect[]{windowAvailableRect, viewRect};
    }

    /**
     * Check if {@link Rect} r2 is above r2. An empty rectangle never contains another rectangle.
     *
     * @param r1 The base rectangle.
     * @param r2 The rectangle being tested that should be above.
     * @return Returns {@code true} if r2 is above r1.
     */
    public static boolean isRectAbove(@NonNull Rect r1, @NonNull Rect r2) {
        // check for empty first
        return r1.left < r1.right && r1.top < r1.bottom
            // now check if above
            && r1.left <= r2.left && r1.bottom >= r2.bottom;
    }

    /**
     * Get device orientation.
     *
     * Related: https://stackoverflow.com/a/29392593/14686958
     *
     * @param context The {@link Context} to check with.
     * @return {@link Configuration#ORIENTATION_PORTRAIT} or {@link Configuration#ORIENTATION_LANDSCAPE}.
     */
    public static int getDisplayOrientation(@NonNull Context context) {
        Point size = getDisplaySize(context, false);
        return (size.x < size.y) ? Configuration.ORIENTATION_PORTRAIT : Configuration.ORIENTATION_LANDSCAPE;
    }

    /**
     * Get device display size.
     *
     * @param context The {@link Context} to check with. It must be {@link Activity} context, otherwise
     *                android will throw:
     *                `java.lang.IllegalArgumentException: Used non-visual Context to obtain an instance of WindowManager. Please use an Activity or a ContextWrapper around one instead.`
     * @param activitySize The set to {@link true}, then size returned will be that of the activity
     *                     and can be smaller than physical display size in multi-window mode.
     * @return Returns the display size as {@link Point}.
     */
    public static Point getDisplaySize( @NonNull Context context, boolean activitySize) {
        // android.view.WindowManager.getDefaultDisplay() and Display.getSize() are deprecated in
        // API 30 and give wrong values in API 30 for activitySize=false in multi-window
        androidx.window.WindowManager windowManager = new androidx.window.WindowManager(context);
        androidx.window.WindowMetrics windowMetrics;
        if (activitySize)
            windowMetrics = windowManager.getCurrentWindowMetrics();
        else
            windowMetrics = windowManager.getMaximumWindowMetrics();
        return new Point(windowMetrics.getBounds().width(), windowMetrics.getBounds().height());
    }

    /** Convert {@link Rect} to {@link String}. */
    public static String toRectString(Rect rect) {
        if (rect == null) return "null";
        return "(" + rect.left + "," + rect.top + "), (" + rect.right + "," + rect.bottom + ")";
    }

    /** Convert {@link Point} to {@link String}. */
    public static String toPointString(Point point) {
        if (point == null) return "null";
        return "(" + point.x + "," + point.y + ")";
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


    public static void setLayoutMarginsInDp(@NonNull View view, int left, int top, int right, int bottom) {
        Context context = view.getContext();
        setLayoutMarginsInPixels(view, dpToPx(context, left), dpToPx(context, top), dpToPx(context, right), dpToPx(context, bottom));
    }

    public static void setLayoutMarginsInPixels(@NonNull View view, int left, int top, int right, int bottom) {
        if (view.getLayoutParams() instanceof ViewGroup.MarginLayoutParams) {
            ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) view.getLayoutParams();
            params.setMargins(left, top, right, bottom);
            view.setLayoutParams(params);
        }
    }

}
