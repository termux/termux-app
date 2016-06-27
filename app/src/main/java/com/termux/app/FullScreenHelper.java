package com.termux.app;

import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.view.View;

import com.termux.R;

/**
 * Utility to manage full screen immersive mode.
 * <p/>
 * See https://code.google.com/p/android/issues/detail?id=5497
 */
final class FullScreenHelper {

    private boolean mEnabled = false;
    final TermuxActivity mActivity;

    public FullScreenHelper(TermuxActivity activity) {
        this.mActivity = activity;
    }

    public void setImmersive(boolean enabled) {
        if (enabled == mEnabled) return;
        mEnabled = enabled;

        View decorView = mActivity.getWindow().getDecorView();

        if (enabled) {
            decorView.setOnSystemUiVisibilityChangeListener
                (new View.OnSystemUiVisibilityChangeListener() {
                    @Override
                    public void onSystemUiVisibilityChange(int visibility) {
                        if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) {
                            if (mActivity.mSettings.isShowExtraKeys()) {
                                mActivity.findViewById(R.id.viewpager).setVisibility(View.VISIBLE);
                            }
                            setImmersiveMode();
                        } else {
                            mActivity.findViewById(R.id.viewpager).setVisibility(View.GONE);
                        }
                    }
                });
            setImmersiveMode();
        } else {
            decorView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);
            decorView.setOnSystemUiVisibilityChangeListener(null);
        }
    }

    private static boolean isColorLight(int color) {
        double darkness = 1 - (0.299 * Color.red(color) + 0.587 * Color.green(color) + 0.114 * Color.blue(color)) / 255;
        return darkness < 0.5;
    }

    void setImmersiveMode() {
        int flags = View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
            | View.SYSTEM_UI_FLAG_FULLSCREEN;
        int color = ((ColorDrawable) mActivity.getWindow().getDecorView().getBackground()).getColor();
        if (isColorLight(color)) flags |= View.SYSTEM_UI_FLAG_LIGHT_STATUS_BAR;
        mActivity.getWindow().getDecorView().setSystemUiVisibility(flags);
    }

}
