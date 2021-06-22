package com.termux.app.terminal;

import android.content.Context;
import android.graphics.Rect;
import android.inputmethodservice.InputMethodService;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.inputmethod.EditorInfo;
import android.widget.FrameLayout;
import android.widget.LinearLayout;

import androidx.annotation.Nullable;

import com.termux.app.TermuxActivity;
import com.termux.shared.logger.Logger;
import com.termux.shared.view.ViewUtils;


/**
 * The {@link TermuxActivity} relies on {@link android.view.WindowManager.LayoutParams#SOFT_INPUT_ADJUST_RESIZE)}
 * set by {@link TermuxTerminalViewClient#setSoftKeyboardState(boolean, boolean)} to automatically
 * resize the view and push the terminal up when soft keyboard is opened. However, this does not
 * always work properly. When `enforce-char-based-input=true` is set in `termux.properties`
 * and {@link com.termux.view.TerminalView#onCreateInputConnection(EditorInfo)} sets the inputType
 * to `InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD | InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS`
 * instead of the default `InputType.TYPE_NULL` for termux, some keyboards may still show suggestions.
 * Gboard does too, but only when text is copied and clipboard suggestions **and** number keys row
 * toggles are enabled in its settings. When number keys row toggle is not enabled, Gboard will still
 * show the row but will switch it with suggestions if needed. If its enabled, then number keys row
 * is always shown and suggestions are shown in an additional row on top of it. This additional row is likely
 * part of the candidates view returned by the keyboard app in {@link InputMethodService#onCreateCandidatesView()}.
 *
 * With the above configuration, the additional clipboard suggestions row partially covers the
 * extra keys/terminal. Reopening the keyboard/activity does not fix the issue. This is either a bug
 * in the Android OS where it does not consider the candidate's view height in its calculation to push
 * up the view or because Gboard does not include the candidate's view height in the height reported
 * to android that should be used, hence causing an overlap.
 *
 * Gboard logs the following entry to `logcat` when its opened with or without the suggestions bar showing:
 * I/KeyboardViewUtil: KeyboardViewUtil.calculateMaxKeyboardBodyHeight():62 leave 500 height for app when screen height:2392, header height:176 and isFullscreenMode:false, so the max keyboard body height is:1716
 * where `keyboard_height = screen_height - height_for_app - header_height` (62 is a hardcoded value in Gboard source code and may be a version number)
 * So this may in fact be due to Gboard but https://stackoverflow.com/questions/57567272 suggests
 * otherwise. Another similar report https://stackoverflow.com/questions/66761661.
 * Also check https://github.com/termux/termux-app/issues/1539.
 *
 * This overlap may happen even without `enforce-char-based-input=true` for keyboards with extended layouts
 * like number row, etc.
 *
 * To fix these issues, `activity_termux.xml` has the constant 1sp transparent
 * `activity_termux_bottom_space_view` View at the bottom. This will appear as a line matching the
 * activity theme. When {@link TermuxActivity} {@link ViewTreeObserver.OnGlobalLayoutListener} is
 * called when any of the sub view layouts change,  like keyboard opening/closing keyboard,
 * extra keys/input view switched, etc, we check if the bottom space view is visible or not.
 * If its not, then we add a margin to the bottom of the root view, so that the keyboard does not
 * overlap the extra keys/terminal, since the margin will push up the view. By default the margin
 * added is equal to the height of the hidden part of extra keys/terminal. For Gboard's case, the
 * hidden part equals the `header_height`. The updates to margins may cause a jitter in some cases
 * when the view is redrawn if the margin is incorrect, but logic has been implemented to avoid that.
 */
public class TermuxActivityRootView extends LinearLayout implements ViewTreeObserver.OnGlobalLayoutListener {

    public TermuxActivity mActivity;
    public Integer marginBottom;
    public Integer lastMarginBottom;

    /** Log root view events. */
    private boolean ROOT_VIEW_LOGGING_ENABLED = false;

    private static final String LOG_TAG = "TermuxActivityRootView";

    public TermuxActivityRootView(Context context) {
        super(context);

    }

    public TermuxActivityRootView(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public TermuxActivityRootView(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void setActivity(TermuxActivity activity) {
        mActivity = activity;
    }

    /**
     * Sets whether root view logging is enabled or not.
     *
     * @param value The boolean value that defines the state.
     */
    public void setIsRootViewLoggingEnabled(boolean value) {
        ROOT_VIEW_LOGGING_ENABLED = value;
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);

        if (marginBottom != null) {
            if (ROOT_VIEW_LOGGING_ENABLED)
                Logger.logVerbose(LOG_TAG, "onMeasure: Setting bottom margin to " + marginBottom);
            ViewGroup.MarginLayoutParams params = (ViewGroup.MarginLayoutParams) getLayoutParams();
            params.setMargins(0, 0, 0, marginBottom);
            setLayoutParams(params);
            marginBottom = null;
            requestLayout();
        }
    }

    @Override
    public void onGlobalLayout() {
        if (mActivity == null || !mActivity.isVisible()) return;

        View bottomSpaceView = mActivity.getTermuxActivityBottomSpaceView();
        if (bottomSpaceView == null) return;

        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) getLayoutParams();

        // Get the position Rects of the bottom space view and the main window holding it
        Rect[] windowAndViewRects = ViewUtils.getWindowAndViewRects(bottomSpaceView);
        if (windowAndViewRects == null)
            return;

        Rect windowAvailableRect = windowAndViewRects[0];
        Rect bottomSpaceViewRect = windowAndViewRects[1];

        // If the bottomSpaceViewRect is inside the windowAvailableRect, then it must be completely visible
        boolean isVisible = windowAvailableRect.contains(bottomSpaceViewRect);
        boolean isVisibleBecauseMargin = (windowAvailableRect.bottom == bottomSpaceViewRect.bottom) && params.bottomMargin > 0;
        boolean isVisibleBecauseExtraMargin = (bottomSpaceViewRect.bottom - windowAvailableRect.bottom) < 0;

        if (ROOT_VIEW_LOGGING_ENABLED)
            Logger.logVerbose(LOG_TAG, "onGlobalLayout: windowAvailableRect " + windowAvailableRect.bottom + ", bottomSpaceViewRect " + bottomSpaceViewRect.bottom + ", diff " + (bottomSpaceViewRect.bottom - windowAvailableRect.bottom) + ", bottom " + params.bottomMargin + ", isVisible " + isVisible + ", isVisibleBecauseMargin " + isVisibleBecauseMargin + ", isVisibleBecauseExtraMargin " + isVisibleBecauseExtraMargin);

        // If the bottomSpaceViewRect is visible, then remove the margin if needed
        if (isVisible) {
            // If visible because of margin, i.e the bottom of bottomSpaceViewRect equals that of windowAvailableRect
            // and a margin has been added
            // Necessary so that we don't get stuck in an infinite loop since setting margin
            // will call OnGlobalLayoutListener again and next time bottom space view
            // will be visible and margin will be set to 0, which again will call
            // OnGlobalLayoutListener...
            // Calling addTermuxActivityRootViewGlobalLayoutListener with a delay fails to
            // set appropriate margins when views are changed quickly since some changes
            // may be missed.
            if (isVisibleBecauseMargin) {
                if (ROOT_VIEW_LOGGING_ENABLED)
                    Logger.logVerbose(LOG_TAG, "onGlobalLayout: Visible due to margin");
                // Once the view has been redrawn with new margin, we set margin back to 0 so that
                // when next time onMeasure() is called, margin 0 is used. This is necessary for
                // cases when view has been redrawn with new margin because bottom space view was
                // hidden by keyboard and then view was redrawn again due to layout change (like
                // keyboard symbol view is switched to), android will add margin below its new position
                // if its greater than 0, which was already above the keyboard creating x2x margin.
                marginBottom = 0;
                return;
            }

            boolean setMargin = params.bottomMargin != 0;

            // If visible because of extra margin, i.e the bottom of bottomSpaceViewRect is above that of windowAvailableRect
            // onGlobalLayout: windowAvailableRect 1408, bottomSpaceViewRect 1232, diff -176, bottom 0, isVisible true, isVisibleBecauseMargin false, isVisibleBecauseExtraMargin false
            // onGlobalLayout: Bottom margin already equals 0
            if (isVisibleBecauseExtraMargin) {
                if (ROOT_VIEW_LOGGING_ENABLED)
                    Logger.logVerbose(LOG_TAG, "onGlobalLayout: Resetting margin since visible due to extra margin");
                setMargin = true;
                // lastMarginBottom must be invalid. May also happen when keyboards are changed.
                lastMarginBottom = null;
            }

            if (setMargin) {
                if (ROOT_VIEW_LOGGING_ENABLED)
                    Logger.logVerbose(LOG_TAG, "onGlobalLayout: Setting bottom margin to 0");
                params.setMargins(0, 0, 0, 0);
                setLayoutParams(params);
            } else {
                if (ROOT_VIEW_LOGGING_ENABLED)
                    Logger.logVerbose(LOG_TAG, "onGlobalLayout: Bottom margin already equals 0");
                // This is done so that when next time onMeasure() is called, lastMarginBottom is used.
                // This is done since we **expect** the keyboard to have same dimensions next time layout
                // changes, so best set margin while view is drawn the first time, otherwise it will
                // cause a jitter when OnGlobalLayoutListener is called with margin 0 and it sets the
                // likely same lastMarginBottom again and requesting a redraw. Hopefully, this logic
                // works fine for all cases.
                marginBottom = lastMarginBottom;
            }
        }
        // ELse find the part of the extra keys/terminal that is hidden and add a margin accordingly
        else {
            int pxHidden = bottomSpaceViewRect.bottom - windowAvailableRect.bottom;
            if (ROOT_VIEW_LOGGING_ENABLED)
                Logger.logVerbose(LOG_TAG, "onGlobalLayout: pxHidden " + pxHidden + ", bottom " + params.bottomMargin);

            boolean setMargin = params.bottomMargin != pxHidden;

            // If invisible despite margin, i.e a margin was added, but the bottom of bottomSpaceViewRect
            // is still below that of windowAvailableRect, this will trigger OnGlobalLayoutListener
            // again, so that margins are set properly. May happen when toolbar/extra keys is disabled
            // and enabled from left drawer, just like case for isVisibleBecauseExtraMargin.
            // onMeasure: Setting bottom margin to 176
            // onGlobalLayout: windowAvailableRect 1232, bottomSpaceViewRect 1408, diff 176, bottom 176, isVisible false, isVisibleBecauseMargin false, isVisibleBecauseExtraMargin false
            // onGlobalLayout: Bottom margin already equals 176
            if ((bottomSpaceViewRect.bottom - windowAvailableRect.bottom) > 0) {
                if (ROOT_VIEW_LOGGING_ENABLED)
                    Logger.logVerbose(LOG_TAG, "onGlobalLayout: Force setting margin since not visible despite margin");
                setMargin = true;
            }

            if (setMargin) {
                if (ROOT_VIEW_LOGGING_ENABLED)
                    Logger.logVerbose(LOG_TAG, "onGlobalLayout: Setting bottom margin to " + pxHidden);
                params.setMargins(0, 0, 0, pxHidden);
                setLayoutParams(params);
                lastMarginBottom = pxHidden;
            } else {
                if (ROOT_VIEW_LOGGING_ENABLED)
                    Logger.logVerbose(LOG_TAG, "onGlobalLayout: Bottom margin already equals " + pxHidden);
            }
        }
    }

}
