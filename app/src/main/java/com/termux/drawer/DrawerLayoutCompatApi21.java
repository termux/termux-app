package com.termux.drawer;

/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowInsets;

/**
 * Provides functionality for DrawerLayout unique to API 21
 */
@SuppressLint("RtlHardcoded")
@TargetApi(Build.VERSION_CODES.LOLLIPOP)
class DrawerLayoutCompatApi21 {

	private static final int[] THEME_ATTRS = { android.R.attr.colorPrimaryDark };

	public static void configureApplyInsets(DrawerLayout drawerLayout) {
		drawerLayout.setOnApplyWindowInsetsListener(new InsetsListener());
		drawerLayout.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
	}

	public static void dispatchChildInsets(View child, Object insets, int gravity) {
		WindowInsets wi = (WindowInsets) insets;
		if (gravity == Gravity.LEFT) {
			wi = wi.replaceSystemWindowInsets(wi.getSystemWindowInsetLeft(), wi.getSystemWindowInsetTop(), 0, wi.getSystemWindowInsetBottom());
		} else if (gravity == Gravity.RIGHT) {
			wi = wi.replaceSystemWindowInsets(0, wi.getSystemWindowInsetTop(), wi.getSystemWindowInsetRight(), wi.getSystemWindowInsetBottom());
		}
		child.dispatchApplyWindowInsets(wi);
	}

	public static void applyMarginInsets(ViewGroup.MarginLayoutParams lp, Object insets, int gravity) {
		WindowInsets wi = (WindowInsets) insets;
		if (gravity == Gravity.LEFT) {
			wi = wi.replaceSystemWindowInsets(wi.getSystemWindowInsetLeft(), wi.getSystemWindowInsetTop(), 0, wi.getSystemWindowInsetBottom());
		} else if (gravity == Gravity.RIGHT) {
			wi = wi.replaceSystemWindowInsets(0, wi.getSystemWindowInsetTop(), wi.getSystemWindowInsetRight(), wi.getSystemWindowInsetBottom());
		}
		lp.leftMargin = wi.getSystemWindowInsetLeft();
		lp.topMargin = wi.getSystemWindowInsetTop();
		lp.rightMargin = wi.getSystemWindowInsetRight();
		lp.bottomMargin = wi.getSystemWindowInsetBottom();
	}

	public static int getTopInset(Object insets) {
		return insets != null ? ((WindowInsets) insets).getSystemWindowInsetTop() : 0;
	}

	public static Drawable getDefaultStatusBarBackground(Context context) {
		final TypedArray a = context.obtainStyledAttributes(THEME_ATTRS);
		try {
			return a.getDrawable(0);
		} finally {
			a.recycle();
		}
	}

	static class InsetsListener implements View.OnApplyWindowInsetsListener {
		@Override
		public WindowInsets onApplyWindowInsets(View v, WindowInsets insets) {
			final DrawerLayout drawerLayout = (DrawerLayout) v;
			drawerLayout.setChildInsets(insets, insets.getSystemWindowInsetTop() > 0);
			return insets.consumeSystemWindowInsets();
		}
	}
}