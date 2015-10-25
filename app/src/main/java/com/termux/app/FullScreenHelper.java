package com.termux.app;

import android.app.Activity;
import android.graphics.Rect;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.FrameLayout.LayoutParams;

/**
 * Utility to make the touch keyboard and immersive mode work with full screen activities.
 * 
 * See https://code.google.com/p/android/issues/detail?id=5497
 */
final class FullScreenHelper implements ViewTreeObserver.OnGlobalLayoutListener {

	private boolean mEnabled = false;
	private final Activity mActivity;
	private final Rect mWindowRect = new Rect();

	public FullScreenHelper(Activity activity) {
		this.mActivity = activity;
	}

	public void setImmersive(boolean enabled) {
		Window win = mActivity.getWindow();

		if (enabled == mEnabled) {
			if (!enabled) win.setFlags(0, WindowManager.LayoutParams.FLAG_FULLSCREEN);
			return;
		}
		mEnabled = enabled;

		final View childViewOfContent = ((FrameLayout) mActivity.findViewById(android.R.id.content)).getChildAt(0);
		if (enabled) {
			win.setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN, WindowManager.LayoutParams.FLAG_FULLSCREEN);
			setImmersiveMode();
			childViewOfContent.getViewTreeObserver().addOnGlobalLayoutListener(this);
		} else {
			win.setFlags(0, WindowManager.LayoutParams.FLAG_FULLSCREEN);
			win.getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);
			childViewOfContent.getViewTreeObserver().removeOnGlobalLayoutListener(this);
			((LayoutParams) childViewOfContent.getLayoutParams()).height = android.view.ViewGroup.LayoutParams.MATCH_PARENT;
		}
	}

	private void setImmersiveMode() {
		mActivity.getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
	}

	@Override
	public void onGlobalLayout() {
		final View childViewOfContent = ((FrameLayout) mActivity.findViewById(android.R.id.content)).getChildAt(0);

		if (mEnabled) setImmersiveMode();

		childViewOfContent.getWindowVisibleDisplayFrame(mWindowRect);
		int usableHeightNow = Math.min(mWindowRect.height(), childViewOfContent.getRootView().getHeight());
		FrameLayout.LayoutParams layout = (LayoutParams) childViewOfContent.getLayoutParams();
		if (layout.height != usableHeightNow) {
			layout.height = usableHeightNow;
			childViewOfContent.requestLayout();
		}
	}

}
