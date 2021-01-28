package com.termux.app;

import android.content.Context;
import android.graphics.Rect;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;

import static com.termux.terminal.EmulatorDebug.LOG_TAG;

/**
 * Work around for fullscreen mode in Termux to fix ExtraKeysView not being visible.
 * This class is derived from:
 * https://stackoverflow.com/questions/7417123/android-how-to-adjust-layout-in-full-screen-mode-when-softkeyboard-is-visible
 * and has some additional tweaks
 * ---
 * For more information, see https://issuetracker.google.com/issues/36911528
 */
public class FullScreenWorkAround {
    /**
     * TODO add documentation so user can know available options and know they may need to experiment
     * for their specific device
     */
    private static final int WORK_AROUND_METHOD_1 = 1;
    private static final int WORK_AROUND_METHOD_2 = 2;
    private static final int WORK_AROUND_METHOD_3 = 3;

    private int mWorkAroundMethod;

    private View mChildOfContent;
    private int mUsableHeightPrevious;
    private ViewGroup.LayoutParams mViewGroupLayoutParams;

    private int mStatusBarHeight;
    private int mExtraKeysHeight;


    public static void apply(TermuxActivity activity, ExtraKeysView view, int workAroundMethod) {
        new FullScreenWorkAround(activity, view, workAroundMethod);
    }

    private FullScreenWorkAround(TermuxActivity activity, ExtraKeysView view, int workAroundMethod) {
        ViewGroup content = activity.findViewById(android.R.id.content);
        mChildOfContent = content.getChildAt(0);
        mViewGroupLayoutParams = mChildOfContent.getLayoutParams();
        mStatusBarHeight = lookupStatusBarHeight(activity);
        mWorkAroundMethod = workAroundMethod;

        // ExtraKeysView height is 0 at this point, so we need this to get actual size
        view.getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
            @Override
            public void onGlobalLayout() {
                mExtraKeysHeight = view.getMeasuredHeight();

                // only needed one time so we can remove
                view.getViewTreeObserver().removeOnGlobalLayoutListener(this);

                // now that we have ExtraKeysView height, we can apply this work-around so that we can see ExtraKeysView when keyboard
                // is shown and dismissed
                mChildOfContent.getViewTreeObserver().addOnGlobalLayoutListener(FullScreenWorkAround.this::possiblyResizeChildOfContent);
            }
        });
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
                int adjustedHeight = mViewGroupLayoutParams.height;

                // height adjustment does not behave consistently between devices so you may need to do
                // a bit of trial and error to see if one of these works for your device
                int workAroundMethod = getWorkAroundMethod();

                switch (workAroundMethod) {
                    case WORK_AROUND_METHOD_1:
                        // tested w/ Pixel (emulator)
                        adjustedHeight = (usableHeightSansKeyboard - heightDifference) + getExtraKeysHeight() + (getStatusBarHeight() / 2);
                        break;
                    case WORK_AROUND_METHOD_2:
                        // tested w/ Samsung S7 and OnePlus 6T
                        adjustedHeight = (usableHeightSansKeyboard - heightDifference);
                        break;
                    case WORK_AROUND_METHOD_3:
                        // tested w/ Sony Xperia XZ
                        adjustedHeight = (usableHeightSansKeyboard - heightDifference) + getExtraKeysHeight();
                        break;
                    default:
                        Log.w(LOG_TAG, "Unknown / Unsupported workAroundMethod for height offset: [" + workAroundMethod + "]");
                    // TODO may need to add more cases for other devices as they're tested..
                }
                mViewGroupLayoutParams.height = adjustedHeight;
            } else {
                // keyboard probably just became hidden
                mViewGroupLayoutParams.height = usableHeightSansKeyboard;
            }
            mChildOfContent.requestLayout();
            mUsableHeightPrevious = usableHeightNow;
        }
    }

    private int computeUsableHeight() {
        Rect r = new Rect();
        mChildOfContent.getWindowVisibleDisplayFrame(r);
        return (r.bottom - r.top);
    }

    private int getExtraKeysHeight() {
        return mExtraKeysHeight;
    }

    private int getStatusBarHeight() {
        return mStatusBarHeight;
    }

    private int lookupStatusBarHeight(Context context) {
        int height = 0;
        int resourceId = context.getResources().getIdentifier("status_bar_height", "dimen", "android");
        if (resourceId > 0) {
            height = context.getResources().getDimensionPixelSize(resourceId);
        }
        return height;
    }

    private int getWorkAroundMethod() {
        return mWorkAroundMethod;
    }
}

