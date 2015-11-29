package com.termux.drawer;

/*
 * Copyright (C) 2013 The Android Open Source Project
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

import java.util.List;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PixelFormat;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.os.Build;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.SystemClock;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityNodeInfo;

/**
 * DrawerLayout acts as a top-level container for window content that allows for interactive "drawer" views to be pulled
 * out from the edge of the window.
 *
 * <p>
 * Drawer positioning and layout is controlled using the <code>android:layout_gravity</code> attribute on child views
 * corresponding to which side of the view you want the drawer to emerge from: left or right. (Or start/end on platform
 * versions that support layout direction.)
 * </p>
 *
 * <p>
 * To use a DrawerLayout, position your primary content view as the first child with a width and height of
 * <code>match_parent</code>. Add drawers as child views after the main content view and set the
 * <code>layout_gravity</code> appropriately. Drawers commonly use <code>match_parent</code> for height with a fixed
 * width.
 * </p>
 *
 * <p>
 * {@link DrawerListener} can be used to monitor the state and motion of drawer views. Avoid performing expensive
 * operations such as layout during animation as it can cause stuttering; try to perform expensive operations during the
 * {@link #STATE_IDLE} state. {@link SimpleDrawerListener} offers default/no-op implementations of each callback method.
 * </p>
 *
 * <p>
 * As per the <a href="{@docRoot}design/patterns/navigation-drawer.html">Android Design guide</a>, any drawers
 * positioned to the left/start should always contain content for navigating around the application, whereas any drawers
 * positioned to the right/end should always contain actions to take on the current content. This preserves the same
 * navigation left, actions right structure present in the Action Bar and elsewhere.
 * </p>
 *
 * <p>
 * For more information about how to use DrawerLayout, read <a href="{@docRoot}
 * training/implementing-navigation/nav-drawer.html">Creating a Navigation Drawer</a>.
 * </p>
 */
@SuppressLint("RtlHardcoded")
public class DrawerLayout extends ViewGroup {
	private static final String TAG = "DrawerLayout";

	/**
	 * Indicates that any drawers are in an idle, settled state. No animation is in progress.
	 */
	public static final int STATE_IDLE = ViewDragHelper.STATE_IDLE;

	/**
	 * Indicates that a drawer is currently being dragged by the user.
	 */
	public static final int STATE_DRAGGING = ViewDragHelper.STATE_DRAGGING;

	/**
	 * Indicates that a drawer is in the process of settling to a final position.
	 */
	public static final int STATE_SETTLING = ViewDragHelper.STATE_SETTLING;

	/**
	 * The drawer is unlocked.
	 */
	public static final int LOCK_MODE_UNLOCKED = 0;

	/**
	 * The drawer is locked closed. The user may not open it, though the app may open it programmatically.
	 */
	public static final int LOCK_MODE_LOCKED_CLOSED = 1;

	/**
	 * The drawer is locked open. The user may not close it, though the app may close it programmatically.
	 */
	public static final int LOCK_MODE_LOCKED_OPEN = 2;

	private static final int MIN_DRAWER_MARGIN = 64; // dp

	private static final int DEFAULT_SCRIM_COLOR = 0x99000000;

	/**
	 * Length of time to delay before peeking the drawer.
	 */
	private static final int PEEK_DELAY = 160; // ms

	/**
	 * Minimum velocity that will be detected as a fling
	 */
	private static final int MIN_FLING_VELOCITY = 400; // dips per second

	/**
	 * Experimental feature.
	 */
	private static final boolean ALLOW_EDGE_LOCK = false;

	private static final boolean CHILDREN_DISALLOW_INTERCEPT = true;

	private static final float TOUCH_SLOP_SENSITIVITY = 1.f;

	static final int[] LAYOUT_ATTRS = new int[] { android.R.attr.layout_gravity };

	/** Whether we can use NO_HIDE_DESCENDANTS accessibility importance. */
	static final boolean CAN_HIDE_DESCENDANTS = Build.VERSION.SDK_INT >= 19;

	private final ChildAccessibilityDelegate mChildAccessibilityDelegate = new ChildAccessibilityDelegate();

	private int mMinDrawerMargin;

	private int mScrimColor = DEFAULT_SCRIM_COLOR;
	private float mScrimOpacity;
	private Paint mScrimPaint = new Paint();

	private final ViewDragHelper mLeftDragger;
	private final ViewDragHelper mRightDragger;
	private final ViewDragCallback mLeftCallback;
	private final ViewDragCallback mRightCallback;
	private int mDrawerState;
	private boolean mInLayout;
	private boolean mFirstLayout = true;
	private int mLockModeLeft;
	private int mLockModeRight;
	private boolean mChildrenCanceledTouch;

	private DrawerListener mListener;

	private float mInitialMotionX;
	private float mInitialMotionY;

	private Drawable mShadowLeft;
	private Drawable mShadowRight;
	private Drawable mStatusBarBackground;

	private CharSequence mTitleLeft;
	private CharSequence mTitleRight;

	private Object mLastInsets;
	private boolean mDrawStatusBarBackground;

	/**
	 * Listener for monitoring events about drawers.
	 */
	public interface DrawerListener {
		/**
		 * Called when a drawer's position changes.
		 * 
		 * @param drawerView
		 *            The child view that was moved
		 * @param slideOffset
		 *            The new offset of this drawer within its range, from 0-1
		 */
		void onDrawerSlide(View drawerView, float slideOffset);

		/**
		 * Called when a drawer has settled in a completely open state. The drawer is interactive at this point.
		 *
		 * @param drawerView
		 *            Drawer view that is now open
		 */
		void onDrawerOpened(View drawerView);

		/**
		 * Called when a drawer has settled in a completely closed state.
		 *
		 * @param drawerView
		 *            Drawer view that is now closed
		 */
		void onDrawerClosed(View drawerView);

		/**
		 * Called when the drawer motion state changes. The new state will be one of {@link #STATE_IDLE},
		 * {@link #STATE_DRAGGING} or {@link #STATE_SETTLING}.
		 *
		 * @param newState
		 *            The new drawer motion state
		 */
		void onDrawerStateChanged(int newState);
	}

	/**
	 * Stub/no-op implementations of all methods of {@link DrawerListener}. Override this if you only care about a few
	 * of the available callback methods.
	 */
	public static abstract class SimpleDrawerListener implements DrawerListener {
		@Override
		public void onDrawerSlide(View drawerView, float slideOffset) {
			// Empty.
		}

		@Override
		public void onDrawerOpened(View drawerView) {
			// Empty.
		}

		@Override
		public void onDrawerClosed(View drawerView) {
			// Empty.
		}

		@Override
		public void onDrawerStateChanged(int newState) {
			// Empty.
		}
	}

	interface DrawerLayoutCompatImpl {
		void configureApplyInsets(View drawerLayout);

		void dispatchChildInsets(View child, Object insets, int drawerGravity);

		void applyMarginInsets(MarginLayoutParams lp, Object insets, int drawerGravity);

		int getTopInset(Object lastInsets);

		Drawable getDefaultStatusBarBackground(Context context);
	}

	static class DrawerLayoutCompatImplBase implements DrawerLayoutCompatImpl {
		@Override
		public void configureApplyInsets(View drawerLayout) {
			// This space for rent
		}

		@Override
		public void dispatchChildInsets(View child, Object insets, int drawerGravity) {
			// This space for rent
		}

		@Override
		public void applyMarginInsets(MarginLayoutParams lp, Object insets, int drawerGravity) {
			// This space for rent
		}

		@Override
		public int getTopInset(Object insets) {
			return 0;
		}

		@Override
		public Drawable getDefaultStatusBarBackground(Context context) {
			return null;
		}
	}

	public DrawerLayout(Context context) {
		this(context, null);
	}

	public DrawerLayout(Context context, AttributeSet attrs) {
		this(context, attrs, 0);
	}

	public DrawerLayout(Context context, AttributeSet attrs, int defStyle) {
		super(context, attrs, defStyle);
		setDescendantFocusability(ViewGroup.FOCUS_AFTER_DESCENDANTS);
		final float density = getResources().getDisplayMetrics().density;
		mMinDrawerMargin = (int) (MIN_DRAWER_MARGIN * density + 0.5f);
		final float minVel = MIN_FLING_VELOCITY * density;

		mLeftCallback = new ViewDragCallback(Gravity.LEFT);
		mRightCallback = new ViewDragCallback(Gravity.RIGHT);

		mLeftDragger = ViewDragHelper.create(this, TOUCH_SLOP_SENSITIVITY, mLeftCallback);
		mLeftDragger.setEdgeTrackingEnabled(ViewDragHelper.EDGE_LEFT);
		mLeftDragger.setMinVelocity(minVel);
		mLeftCallback.setDragger(mLeftDragger);

		mRightDragger = ViewDragHelper.create(this, TOUCH_SLOP_SENSITIVITY, mRightCallback);
		mRightDragger.setEdgeTrackingEnabled(ViewDragHelper.EDGE_RIGHT);
		mRightDragger.setMinVelocity(minVel);
		mRightCallback.setDragger(mRightDragger);

		// So that we can catch the back button
		setFocusableInTouchMode(true);

		this.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);

		this.setAccessibilityDelegate(new AccessibilityDelegate());
		this.setMotionEventSplittingEnabled(false);
		if (this.getFitsSystemWindows()) {
			DrawerLayoutCompatApi21.configureApplyInsets(this);
			mStatusBarBackground = DrawerLayoutCompatApi21.getDefaultStatusBarBackground(context);
		}
	}

	/**
	 * Internal use only; called to apply window insets when configured with fitsSystemWindows="true"
	 */
	public void setChildInsets(Object insets, boolean draw) {
		mLastInsets = insets;
		mDrawStatusBarBackground = draw;
		setWillNotDraw(!draw && getBackground() == null);
		requestLayout();
	}

	/**
	 * Set a simple drawable used for the left or right shadow. The drawable provided must have a nonzero intrinsic
	 * width.
	 *
	 * @param shadowDrawable
	 *            Shadow drawable to use at the edge of a drawer
	 * @param gravity
	 *            Which drawer the shadow should apply to
	 */
	public void setDrawerShadow(Drawable shadowDrawable, int gravity) {
		/*
		 * TODO Someone someday might want to set more complex drawables here. They're probably nuts, but we might want
		 * to consider registering callbacks, setting states, etc. properly.
		 */

		final int absGravity = Gravity.getAbsoluteGravity(gravity, this.getLayoutDirection());
		if ((absGravity & Gravity.LEFT) == Gravity.LEFT) {
			mShadowLeft = shadowDrawable;
			invalidate();
		}
		if ((absGravity & Gravity.RIGHT) == Gravity.RIGHT) {
			mShadowRight = shadowDrawable;
			invalidate();
		}
	}

	/**
	 * Set a simple drawable used for the left or right shadow. The drawable provided must have a nonzero intrinsic
	 * width.
	 *
	 * @param resId
	 *            Resource id of a shadow drawable to use at the edge of a drawer
	 * @param gravity
	 *            Which drawer the shadow should apply to
	 */
	public void setDrawerShadow(int resId, int gravity) {
		setDrawerShadow(getResources().getDrawable(resId), gravity);
	}

	/**
	 * Set a color to use for the scrim that obscures primary content while a drawer is open.
	 *
	 * @param color
	 *            Color to use in 0xAARRGGBB format.
	 */
	public void setScrimColor(int color) {
		mScrimColor = color;
		invalidate();
	}

	/**
	 * Set a listener to be notified of drawer events.
	 *
	 * @param listener
	 *            Listener to notify when drawer events occur
	 * @see DrawerListener
	 */
	public void setDrawerListener(DrawerListener listener) {
		mListener = listener;
	}

	/**
	 * Enable or disable interaction with all drawers.
	 *
	 * <p>
	 * This allows the application to restrict the user's ability to open or close any drawer within this layout.
	 * DrawerLayout will still respond to calls to {@link #openDrawer(int)}, {@link #closeDrawer(int)} and friends if a
	 * drawer is locked.
	 * </p>
	 *
	 * <p>
	 * Locking drawers open or closed will implicitly open or close any drawers as appropriate.
	 * </p>
	 *
	 * @param lockMode
	 *            The new lock mode for the given drawer. One of {@link #LOCK_MODE_UNLOCKED},
	 *            {@link #LOCK_MODE_LOCKED_CLOSED} or {@link #LOCK_MODE_LOCKED_OPEN}.
	 */
	public void setDrawerLockMode(int lockMode) {
		setDrawerLockMode(lockMode, Gravity.LEFT);
		setDrawerLockMode(lockMode, Gravity.RIGHT);
	}

	/**
	 * Enable or disable interaction with the given drawer.
	 *
	 * <p>
	 * This allows the application to restrict the user's ability to open or close the given drawer. DrawerLayout will
	 * still respond to calls to {@link #openDrawer(int)}, {@link #closeDrawer(int)} and friends if a drawer is locked.
	 * </p>
	 *
	 * <p>
	 * Locking a drawer open or closed will implicitly open or close that drawer as appropriate.
	 * </p>
	 *
	 * @param lockMode
	 *            The new lock mode for the given drawer. One of {@link #LOCK_MODE_UNLOCKED},
	 *            {@link #LOCK_MODE_LOCKED_CLOSED} or {@link #LOCK_MODE_LOCKED_OPEN}.
	 * @param edgeGravity
	 *            Gravity.LEFT, RIGHT, START or END. Expresses which drawer to change the mode for.
	 *
	 * @see #LOCK_MODE_UNLOCKED
	 * @see #LOCK_MODE_LOCKED_CLOSED
	 * @see #LOCK_MODE_LOCKED_OPEN
	 */
	public void setDrawerLockMode(int lockMode, int edgeGravity) {
		final int absGravity = Gravity.getAbsoluteGravity(edgeGravity, this.getLayoutDirection());
		if (absGravity == Gravity.LEFT) {
			mLockModeLeft = lockMode;
		} else if (absGravity == Gravity.RIGHT) {
			mLockModeRight = lockMode;
		}
		if (lockMode != LOCK_MODE_UNLOCKED) {
			// Cancel interaction in progress
			final ViewDragHelper helper = absGravity == Gravity.LEFT ? mLeftDragger : mRightDragger;
			helper.cancel();
		}
		switch (lockMode) {
		case LOCK_MODE_LOCKED_OPEN:
			final View toOpen = findDrawerWithGravity(absGravity);
			if (toOpen != null) {
				openDrawer(toOpen);
			}
			break;
		case LOCK_MODE_LOCKED_CLOSED:
			final View toClose = findDrawerWithGravity(absGravity);
			if (toClose != null) {
				closeDrawer(toClose);
			}
			break;
		// default: do nothing
		}
	}

	/**
	 * Enable or disable interaction with the given drawer.
	 *
	 * <p>
	 * This allows the application to restrict the user's ability to open or close the given drawer. DrawerLayout will
	 * still respond to calls to {@link #openDrawer(int)}, {@link #closeDrawer(int)} and friends if a drawer is locked.
	 * </p>
	 *
	 * <p>
	 * Locking a drawer open or closed will implicitly open or close that drawer as appropriate.
	 * </p>
	 *
	 * @param lockMode
	 *            The new lock mode for the given drawer. One of {@link #LOCK_MODE_UNLOCKED},
	 *            {@link #LOCK_MODE_LOCKED_CLOSED} or {@link #LOCK_MODE_LOCKED_OPEN}.
	 * @param drawerView
	 *            The drawer view to change the lock mode for
	 *
	 * @see #LOCK_MODE_UNLOCKED
	 * @see #LOCK_MODE_LOCKED_CLOSED
	 * @see #LOCK_MODE_LOCKED_OPEN
	 */
	public void setDrawerLockMode(int lockMode, View drawerView) {
		if (!isDrawerView(drawerView)) {
			throw new IllegalArgumentException("View " + drawerView + " is not a " + "drawer with appropriate layout_gravity");
		}
		final int gravity = ((LayoutParams) drawerView.getLayoutParams()).gravity;
		setDrawerLockMode(lockMode, gravity);
	}

	/**
	 * Check the lock mode of the drawer with the given gravity.
	 *
	 * @param edgeGravity
	 *            Gravity of the drawer to check
	 * @return one of {@link #LOCK_MODE_UNLOCKED}, {@link #LOCK_MODE_LOCKED_CLOSED} or {@link #LOCK_MODE_LOCKED_OPEN}.
	 */
	public int getDrawerLockMode(int edgeGravity) {
		final int absGravity = Gravity.getAbsoluteGravity(edgeGravity, this.getLayoutDirection());
		if (absGravity == Gravity.LEFT) {
			return mLockModeLeft;
		} else if (absGravity == Gravity.RIGHT) {
			return mLockModeRight;
		}
		return LOCK_MODE_UNLOCKED;
	}

	/**
	 * Check the lock mode of the given drawer view.
	 *
	 * @param drawerView
	 *            Drawer view to check lock mode
	 * @return one of {@link #LOCK_MODE_UNLOCKED}, {@link #LOCK_MODE_LOCKED_CLOSED} or {@link #LOCK_MODE_LOCKED_OPEN}.
	 */
	public int getDrawerLockMode(View drawerView) {
		final int absGravity = getDrawerViewAbsoluteGravity(drawerView);
		if (absGravity == Gravity.LEFT) {
			return mLockModeLeft;
		} else if (absGravity == Gravity.RIGHT) {
			return mLockModeRight;
		}
		return LOCK_MODE_UNLOCKED;
	}

	/**
	 * Sets the title of the drawer with the given gravity.
	 * <p>
	 * When accessibility is turned on, this is the title that will be used to identify the drawer to the active
	 * accessibility service.
	 *
	 * @param edgeGravity
	 *            Gravity.LEFT, RIGHT, START or END. Expresses which drawer to set the title for.
	 * @param title
	 *            The title for the drawer.
	 */
	public void setDrawerTitle(int edgeGravity, CharSequence title) {
		final int absGravity = Gravity.getAbsoluteGravity(edgeGravity, this.getLayoutDirection());
		if (absGravity == Gravity.LEFT) {
			mTitleLeft = title;
		} else if (absGravity == Gravity.RIGHT) {
			mTitleRight = title;
		}
	}

	/**
	 * Returns the title of the drawer with the given gravity.
	 *
	 * @param edgeGravity
	 *            Gravity.LEFT, RIGHT, START or END. Expresses which drawer to return the title for.
	 * @return The title of the drawer, or null if none set.
	 * @see #setDrawerTitle(int, CharSequence)
	 */
	public CharSequence getDrawerTitle(int edgeGravity) {
		final int absGravity = Gravity.getAbsoluteGravity(edgeGravity, this.getLayoutDirection());
		if (absGravity == Gravity.LEFT) {
			return mTitleLeft;
		} else if (absGravity == Gravity.RIGHT) {
			return mTitleRight;
		}
		return null;
	}

	/**
	 * Resolve the shared state of all drawers from the component ViewDragHelpers. Should be called whenever a
	 * ViewDragHelper's state changes.
	 */
	void updateDrawerState(int activeState, View activeDrawer) {
		final int leftState = mLeftDragger.getViewDragState();
		final int rightState = mRightDragger.getViewDragState();

		final int state;
		if (leftState == STATE_DRAGGING || rightState == STATE_DRAGGING) {
			state = STATE_DRAGGING;
		} else if (leftState == STATE_SETTLING || rightState == STATE_SETTLING) {
			state = STATE_SETTLING;
		} else {
			state = STATE_IDLE;
		}

		if (activeDrawer != null && activeState == STATE_IDLE) {
			final LayoutParams lp = (LayoutParams) activeDrawer.getLayoutParams();
			if (lp.onScreen == 0) {
				dispatchOnDrawerClosed(activeDrawer);
			} else if (lp.onScreen == 1) {
				dispatchOnDrawerOpened(activeDrawer);
			}
		}

		if (state != mDrawerState) {
			mDrawerState = state;

			if (mListener != null) {
				mListener.onDrawerStateChanged(state);
			}
		}
	}

	void dispatchOnDrawerClosed(View drawerView) {
		final LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
		if (lp.knownOpen) {
			lp.knownOpen = false;
			if (mListener != null) {
				mListener.onDrawerClosed(drawerView);
			}

			updateChildrenImportantForAccessibility(drawerView, false);

			// Only send WINDOW_STATE_CHANGE if the host has window focus. This
			// may change if support for multiple foreground windows (e.g. IME)
			// improves.
			if (hasWindowFocus()) {
				final View rootView = getRootView();
				if (rootView != null) {
					rootView.sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED);
				}
			}
		}
	}

	void dispatchOnDrawerOpened(View drawerView) {
		final LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
		if (!lp.knownOpen) {
			lp.knownOpen = true;
			if (mListener != null) {
				mListener.onDrawerOpened(drawerView);
			}

			updateChildrenImportantForAccessibility(drawerView, true);

			// Only send WINDOW_STATE_CHANGE if the host has window focus.
			if (hasWindowFocus()) {
				sendAccessibilityEvent(AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED);
			}

			drawerView.requestFocus();
		}
	}

	private void updateChildrenImportantForAccessibility(View drawerView, boolean isDrawerOpen) {
		final int childCount = getChildCount();
		for (int i = 0; i < childCount; i++) {
			final View child = getChildAt(i);
			if (!isDrawerOpen && !isDrawerView(child) || isDrawerOpen && child == drawerView) {
				// Drawer is closed and this is a content view or this is an
				// open drawer view, so it should be visible.
				child.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
			} else {
				child.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS);
			}
		}
	}

	void dispatchOnDrawerSlide(View drawerView, float slideOffset) {
		if (mListener != null) {
			mListener.onDrawerSlide(drawerView, slideOffset);
		}
	}

	void setDrawerViewOffset(View drawerView, float slideOffset) {
		final LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
		if (slideOffset == lp.onScreen) {
			return;
		}

		lp.onScreen = slideOffset;
		dispatchOnDrawerSlide(drawerView, slideOffset);
	}

	float getDrawerViewOffset(View drawerView) {
		return ((LayoutParams) drawerView.getLayoutParams()).onScreen;
	}

	/**
	 * @return the absolute gravity of the child drawerView, resolved according to the current layout direction
	 */
	int getDrawerViewAbsoluteGravity(View drawerView) {
		final int gravity = ((LayoutParams) drawerView.getLayoutParams()).gravity;
		return Gravity.getAbsoluteGravity(gravity, this.getLayoutDirection());
	}

	boolean checkDrawerViewAbsoluteGravity(View drawerView, int checkFor) {
		final int absGravity = getDrawerViewAbsoluteGravity(drawerView);
		return (absGravity & checkFor) == checkFor;
	}

	View findOpenDrawer() {
		final int childCount = getChildCount();
		for (int i = 0; i < childCount; i++) {
			final View child = getChildAt(i);
			if (((LayoutParams) child.getLayoutParams()).knownOpen) {
				return child;
			}
		}
		return null;
	}

	void moveDrawerToOffset(View drawerView, float slideOffset) {
		final float oldOffset = getDrawerViewOffset(drawerView);
		final int width = drawerView.getWidth();
		final int oldPos = (int) (width * oldOffset);
		final int newPos = (int) (width * slideOffset);
		final int dx = newPos - oldPos;

		drawerView.offsetLeftAndRight(checkDrawerViewAbsoluteGravity(drawerView, Gravity.LEFT) ? dx : -dx);
		setDrawerViewOffset(drawerView, slideOffset);
	}

	/**
	 * @param gravity
	 *            the gravity of the child to return. If specified as a relative value, it will be resolved according to
	 *            the current layout direction.
	 * @return the drawer with the specified gravity
	 */
	View findDrawerWithGravity(int gravity) {
		final int absHorizGravity = Gravity.getAbsoluteGravity(gravity, this.getLayoutDirection()) & Gravity.HORIZONTAL_GRAVITY_MASK;
		final int childCount = getChildCount();
		for (int i = 0; i < childCount; i++) {
			final View child = getChildAt(i);
			final int childAbsGravity = getDrawerViewAbsoluteGravity(child);
			if ((childAbsGravity & Gravity.HORIZONTAL_GRAVITY_MASK) == absHorizGravity) {
				return child;
			}
		}
		return null;
	}

	/**
	 * Simple gravity to string - only supports LEFT and RIGHT for debugging output.
	 *
	 * @param gravity
	 *            Absolute gravity value
	 * @return LEFT or RIGHT as appropriate, or a hex string
	 */
	static String gravityToString(int gravity) {
		if ((gravity & Gravity.LEFT) == Gravity.LEFT) {
			return "LEFT";
		}
		if ((gravity & Gravity.RIGHT) == Gravity.RIGHT) {
			return "RIGHT";
		}
		return Integer.toHexString(gravity);
	}

	@Override
	protected void onDetachedFromWindow() {
		super.onDetachedFromWindow();
		mFirstLayout = true;
	}

	@Override
	protected void onAttachedToWindow() {
		super.onAttachedToWindow();
		mFirstLayout = true;
	}

	@Override
	protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
		int widthMode = MeasureSpec.getMode(widthMeasureSpec);
		int heightMode = MeasureSpec.getMode(heightMeasureSpec);
		int widthSize = MeasureSpec.getSize(widthMeasureSpec);
		int heightSize = MeasureSpec.getSize(heightMeasureSpec);

		if (widthMode != MeasureSpec.EXACTLY || heightMode != MeasureSpec.EXACTLY) {
			if (isInEditMode()) {
				// Don't crash the layout editor. Consume all of the space if specified
				// or pick a magic number from thin air otherwise.
				// TODO Better communication with tools of this bogus state.
				// It will crash on a real device.
				if (widthMode == MeasureSpec.UNSPECIFIED) {
					widthSize = 300;
				}
				if (heightMode == MeasureSpec.UNSPECIFIED) {
					heightSize = 300;
				}
			} else {
				throw new IllegalArgumentException("DrawerLayout must be measured with MeasureSpec.EXACTLY.");
			}
		}

		setMeasuredDimension(widthSize, heightSize);

		final boolean applyInsets = mLastInsets != null && this.getFitsSystemWindows();
		final int layoutDirection = this.getLayoutDirection();

		// Gravity value for each drawer we've seen. Only one of each permitted.
		int foundDrawers = 0;
		final int childCount = getChildCount();
		for (int i = 0; i < childCount; i++) {
			final View child = getChildAt(i);

			if (child.getVisibility() == GONE) {
				continue;
			}

			final LayoutParams lp = (LayoutParams) child.getLayoutParams();

			if (applyInsets) {
				final int cgrav = Gravity.getAbsoluteGravity(lp.gravity, layoutDirection);
				if (child.getFitsSystemWindows()) {
					DrawerLayoutCompatApi21.dispatchChildInsets(child, mLastInsets, cgrav);
				} else {
					DrawerLayoutCompatApi21.applyMarginInsets(lp, mLastInsets, cgrav);
				}
			}

			if (isContentView(child)) {
				// Content views get measured at exactly the layout's size.
				final int contentWidthSpec = MeasureSpec.makeMeasureSpec(widthSize - lp.leftMargin - lp.rightMargin, MeasureSpec.EXACTLY);
				final int contentHeightSpec = MeasureSpec.makeMeasureSpec(heightSize - lp.topMargin - lp.bottomMargin, MeasureSpec.EXACTLY);
				child.measure(contentWidthSpec, contentHeightSpec);
			} else if (isDrawerView(child)) {
				final int childGravity = getDrawerViewAbsoluteGravity(child) & Gravity.HORIZONTAL_GRAVITY_MASK;
				if ((foundDrawers & childGravity) != 0) {
					throw new IllegalStateException("Child drawer has absolute gravity " + gravityToString(childGravity) + " but this " + TAG
							+ " already has a " + "drawer view along that edge");
				}
				final int drawerWidthSpec = getChildMeasureSpec(widthMeasureSpec, mMinDrawerMargin + lp.leftMargin + lp.rightMargin, lp.width);
				final int drawerHeightSpec = getChildMeasureSpec(heightMeasureSpec, lp.topMargin + lp.bottomMargin, lp.height);
				child.measure(drawerWidthSpec, drawerHeightSpec);
			} else {
				throw new IllegalStateException("Child " + child + " at index " + i + " does not have a valid layout_gravity - must be Gravity.LEFT, "
						+ "Gravity.RIGHT or Gravity.NO_GRAVITY");
			}
		}
	}

	@Override
	protected void onLayout(boolean changed, int l, int t, int r, int b) {
		mInLayout = true;
		final int width = r - l;
		final int childCount = getChildCount();
		for (int i = 0; i < childCount; i++) {
			final View child = getChildAt(i);

			if (child.getVisibility() == GONE) {
				continue;
			}

			final LayoutParams lp = (LayoutParams) child.getLayoutParams();

			if (isContentView(child)) {
				child.layout(lp.leftMargin, lp.topMargin, lp.leftMargin + child.getMeasuredWidth(), lp.topMargin + child.getMeasuredHeight());
			} else { // Drawer, if it wasn't onMeasure would have thrown an exception.
				final int childWidth = child.getMeasuredWidth();
				final int childHeight = child.getMeasuredHeight();
				int childLeft;

				final float newOffset;
				if (checkDrawerViewAbsoluteGravity(child, Gravity.LEFT)) {
					childLeft = -childWidth + (int) (childWidth * lp.onScreen);
					newOffset = (float) (childWidth + childLeft) / childWidth;
				} else { // Right; onMeasure checked for us.
					childLeft = width - (int) (childWidth * lp.onScreen);
					newOffset = (float) (width - childLeft) / childWidth;
				}

				final boolean changeOffset = newOffset != lp.onScreen;

				final int vgrav = lp.gravity & Gravity.VERTICAL_GRAVITY_MASK;

				switch (vgrav) {
				default:
				case Gravity.TOP: {
					child.layout(childLeft, lp.topMargin, childLeft + childWidth, lp.topMargin + childHeight);
					break;
				}

				case Gravity.BOTTOM: {
					final int height = b - t;
					child.layout(childLeft, height - lp.bottomMargin - child.getMeasuredHeight(), childLeft + childWidth, height - lp.bottomMargin);
					break;
				}

				case Gravity.CENTER_VERTICAL: {
					final int height = b - t;
					int childTop = (height - childHeight) / 2;

					// Offset for margins. If things don't fit right because of
					// bad measurement before, oh well.
					if (childTop < lp.topMargin) {
						childTop = lp.topMargin;
					} else if (childTop + childHeight > height - lp.bottomMargin) {
						childTop = height - lp.bottomMargin - childHeight;
					}
					child.layout(childLeft, childTop, childLeft + childWidth, childTop + childHeight);
					break;
				}
				}

				if (changeOffset) {
					setDrawerViewOffset(child, newOffset);
				}

				final int newVisibility = lp.onScreen > 0 ? VISIBLE : INVISIBLE;
				if (child.getVisibility() != newVisibility) {
					child.setVisibility(newVisibility);
				}
			}
		}
		mInLayout = false;
		mFirstLayout = false;
	}

	@Override
	public void requestLayout() {
		if (!mInLayout) {
			super.requestLayout();
		}
	}

	@Override
	public void computeScroll() {
		final int childCount = getChildCount();
		float scrimOpacity = 0;
		for (int i = 0; i < childCount; i++) {
			final float onscreen = ((LayoutParams) getChildAt(i).getLayoutParams()).onScreen;
			scrimOpacity = Math.max(scrimOpacity, onscreen);
		}
		mScrimOpacity = scrimOpacity;

		// "|" used on purpose; both need to run.
		if (mLeftDragger.continueSettling(true) | mRightDragger.continueSettling(true)) {
			this.postInvalidateOnAnimation();
		}
	}

	private static boolean hasOpaqueBackground(View v) {
		final Drawable bg = v.getBackground();
		return bg != null && bg.getOpacity() == PixelFormat.OPAQUE;
	}

	/**
	 * Set a drawable to draw in the insets area for the status bar. Note that this will only be activated if this
	 * DrawerLayout fitsSystemWindows.
	 *
	 * @param bg
	 *            Background drawable to draw behind the status bar
	 */
	public void setStatusBarBackground(Drawable bg) {
		mStatusBarBackground = bg;
		invalidate();
	}

	/**
	 * Gets the drawable used to draw in the insets area for the status bar.
	 *
	 * @return The status bar background drawable, or null if none set
	 */
	public Drawable getStatusBarBackgroundDrawable() {
		return mStatusBarBackground;
	}

	/**
	 * Set a drawable to draw in the insets area for the status bar. Note that this will only be activated if this
	 * DrawerLayout fitsSystemWindows.
	 *
	 * @param resId
	 *            Resource id of a background drawable to draw behind the status bar
	 */
	public void setStatusBarBackground(int resId) {
		mStatusBarBackground = resId != 0 ? getContext().getDrawable(resId) : null;
		invalidate();
	}

	/**
	 * Set a drawable to draw in the insets area for the status bar. Note that this will only be activated if this
	 * DrawerLayout fitsSystemWindows.
	 *
	 * @param color
	 *            Color to use as a background drawable to draw behind the status bar in 0xAARRGGBB format.
	 */
	public void setStatusBarBackgroundColor(int color) {
		mStatusBarBackground = new ColorDrawable(color);
		invalidate();
	}

	@Override
	public void onDraw(Canvas c) {
		super.onDraw(c);
		if (mDrawStatusBarBackground && mStatusBarBackground != null) {
			final int inset = DrawerLayoutCompatApi21.getTopInset(mLastInsets);
			if (inset > 0) {
				mStatusBarBackground.setBounds(0, 0, getWidth(), inset);
				mStatusBarBackground.draw(c);
			}
		}
	}

	@Override
	protected boolean drawChild(Canvas canvas, View child, long drawingTime) {
		final int height = getHeight();
		final boolean drawingContent = isContentView(child);
		int clipLeft = 0, clipRight = getWidth();

		final int restoreCount = canvas.save();
		if (drawingContent) {
			final int childCount = getChildCount();
			for (int i = 0; i < childCount; i++) {
				final View v = getChildAt(i);
				if (v == child || v.getVisibility() != VISIBLE || !hasOpaqueBackground(v) || !isDrawerView(v) || v.getHeight() < height) {
					continue;
				}

				if (checkDrawerViewAbsoluteGravity(v, Gravity.LEFT)) {
					final int vright = v.getRight();
					if (vright > clipLeft) clipLeft = vright;
				} else {
					final int vleft = v.getLeft();
					if (vleft < clipRight) clipRight = vleft;
				}
			}
			canvas.clipRect(clipLeft, 0, clipRight, getHeight());
		}
		final boolean result = super.drawChild(canvas, child, drawingTime);
		canvas.restoreToCount(restoreCount);

		if (mScrimOpacity > 0 && drawingContent) {
			final int baseAlpha = (mScrimColor & 0xff000000) >>> 24;
			final int imag = (int) (baseAlpha * mScrimOpacity);
			final int color = imag << 24 | (mScrimColor & 0xffffff);
			mScrimPaint.setColor(color);

			canvas.drawRect(clipLeft, 0, clipRight, getHeight(), mScrimPaint);
		} else if (mShadowLeft != null && checkDrawerViewAbsoluteGravity(child, Gravity.LEFT)) {
			final int shadowWidth = mShadowLeft.getIntrinsicWidth();
			final int childRight = child.getRight();
			final int drawerPeekDistance = mLeftDragger.getEdgeSize();
			final float alpha = Math.max(0, Math.min((float) childRight / drawerPeekDistance, 1.f));
			mShadowLeft.setBounds(childRight, child.getTop(), childRight + shadowWidth, child.getBottom());
			mShadowLeft.setAlpha((int) (0xff * alpha));
			mShadowLeft.draw(canvas);
		} else if (mShadowRight != null && checkDrawerViewAbsoluteGravity(child, Gravity.RIGHT)) {
			final int shadowWidth = mShadowRight.getIntrinsicWidth();
			final int childLeft = child.getLeft();
			final int showing = getWidth() - childLeft;
			final int drawerPeekDistance = mRightDragger.getEdgeSize();
			final float alpha = Math.max(0, Math.min((float) showing / drawerPeekDistance, 1.f));
			mShadowRight.setBounds(childLeft - shadowWidth, child.getTop(), childLeft, child.getBottom());
			mShadowRight.setAlpha((int) (0xff * alpha));
			mShadowRight.draw(canvas);
		}
		return result;
	}

	boolean isContentView(View child) {
		return ((LayoutParams) child.getLayoutParams()).gravity == Gravity.NO_GRAVITY;
	}

	boolean isDrawerView(View child) {
		final int gravity = ((LayoutParams) child.getLayoutParams()).gravity;
		final int absGravity = Gravity.getAbsoluteGravity(gravity, child.getLayoutDirection());
		return (absGravity & (Gravity.LEFT | Gravity.RIGHT)) != 0;
	}

	@Override
	public boolean onInterceptTouchEvent(MotionEvent ev) {
		final int action = ev.getActionMasked();

		// "|" used deliberately here; both methods should be invoked.
		final boolean interceptForDrag = mLeftDragger.shouldInterceptTouchEvent(ev) | mRightDragger.shouldInterceptTouchEvent(ev);

		boolean interceptForTap = false;

		switch (action) {
		case MotionEvent.ACTION_DOWN: {
			final float x = ev.getX();
			final float y = ev.getY();
			mInitialMotionX = x;
			mInitialMotionY = y;
			if (mScrimOpacity > 0) {
				final View child = mLeftDragger.findTopChildUnder((int) x, (int) y);
				if (child != null && isContentView(child)) {
					interceptForTap = true;
				}
			}
			mChildrenCanceledTouch = false;
			break;
		}

		case MotionEvent.ACTION_MOVE: {
			// If we cross the touch slop, don't perform the delayed peek for an edge touch.
			if (mLeftDragger.checkTouchSlop(ViewDragHelper.DIRECTION_ALL)) {
				mLeftCallback.removeCallbacks();
				mRightCallback.removeCallbacks();
			}
			break;
		}

		case MotionEvent.ACTION_CANCEL:
		case MotionEvent.ACTION_UP: {
			closeDrawers(true);
			mChildrenCanceledTouch = false;
		}
		}

		return interceptForDrag || interceptForTap || hasPeekingDrawer() || mChildrenCanceledTouch;
	}

	@SuppressLint("ClickableViewAccessibility")
	@Override
	public boolean onTouchEvent(MotionEvent ev) {
		mLeftDragger.processTouchEvent(ev);
		mRightDragger.processTouchEvent(ev);

		final int action = ev.getAction();

		switch (action & MotionEvent.ACTION_MASK) {
		case MotionEvent.ACTION_DOWN: {
			final float x = ev.getX();
			final float y = ev.getY();
			mInitialMotionX = x;
			mInitialMotionY = y;
			mChildrenCanceledTouch = false;
			break;
		}

		case MotionEvent.ACTION_UP: {
			final float x = ev.getX();
			final float y = ev.getY();
			boolean peekingOnly = true;
			final View touchedView = mLeftDragger.findTopChildUnder((int) x, (int) y);
			if (touchedView != null && isContentView(touchedView)) {
				final float dx = x - mInitialMotionX;
				final float dy = y - mInitialMotionY;
				final int slop = mLeftDragger.getTouchSlop();
				if (dx * dx + dy * dy < slop * slop) {
					// Taps close a dimmed open drawer but only if it isn't locked open.
					final View openDrawer = findOpenDrawer();
					if (openDrawer != null) {
						peekingOnly = getDrawerLockMode(openDrawer) == LOCK_MODE_LOCKED_OPEN;
					}
				}
			}
			closeDrawers(peekingOnly);
			break;
		}

		case MotionEvent.ACTION_CANCEL: {
			closeDrawers(true);
			mChildrenCanceledTouch = false;
			break;
		}
		}

		return true;
	}

	@Override
	public void requestDisallowInterceptTouchEvent(boolean disallowIntercept) {
		if (CHILDREN_DISALLOW_INTERCEPT/*
										 * || (!mLeftDragger.isEdgeTouched(ViewDragHelper.EDGE_LEFT) &&
										 * !mRightDragger.isEdgeTouched(ViewDragHelper.EDGE_RIGHT))
										 */) {
			// If we have an edge touch we want to skip this and track it for later instead.
			super.requestDisallowInterceptTouchEvent(disallowIntercept);
		}
		if (disallowIntercept) {
			closeDrawers(true);
		}
	}

	/**
	 * Close all currently open drawer views by animating them out of view.
	 */
	public void closeDrawers() {
		closeDrawers(false);
	}

	void closeDrawers(boolean peekingOnly) {
		boolean needsInvalidate = false;
		final int childCount = getChildCount();
		for (int i = 0; i < childCount; i++) {
			final View child = getChildAt(i);
			final LayoutParams lp = (LayoutParams) child.getLayoutParams();

			if (!isDrawerView(child) || (peekingOnly && !lp.isPeeking)) {
				continue;
			}

			final int childWidth = child.getWidth();

			if (checkDrawerViewAbsoluteGravity(child, Gravity.LEFT)) {
				needsInvalidate |= mLeftDragger.smoothSlideViewTo(child, -childWidth, child.getTop());
			} else {
				needsInvalidate |= mRightDragger.smoothSlideViewTo(child, getWidth(), child.getTop());
			}

			lp.isPeeking = false;
		}

		mLeftCallback.removeCallbacks();
		mRightCallback.removeCallbacks();

		if (needsInvalidate) {
			invalidate();
		}
	}

	/**
	 * Open the specified drawer view by animating it into view.
	 *
	 * @param drawerView
	 *            Drawer view to open
	 */
	public void openDrawer(View drawerView) {
		if (!isDrawerView(drawerView)) {
			throw new IllegalArgumentException("View " + drawerView + " is not a sliding drawer");
		}

		if (mFirstLayout) {
			final LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
			lp.onScreen = 1.f;
			lp.knownOpen = true;

			updateChildrenImportantForAccessibility(drawerView, true);
		} else {
			if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.LEFT)) {
				mLeftDragger.smoothSlideViewTo(drawerView, 0, drawerView.getTop());
			} else {
				mRightDragger.smoothSlideViewTo(drawerView, getWidth() - drawerView.getWidth(), drawerView.getTop());
			}
		}
		invalidate();
	}

	/**
	 * Open the specified drawer by animating it out of view.
	 *
	 * @param gravity
	 *            Gravity.LEFT to move the left drawer or Gravity.RIGHT for the right. Gravity.START or Gravity.END may
	 *            also be used.
	 */
	public void openDrawer(int gravity) {
		final View drawerView = findDrawerWithGravity(gravity);
		if (drawerView == null) {
			throw new IllegalArgumentException("No drawer view found with gravity " + gravityToString(gravity));
		}
		openDrawer(drawerView);
	}

	/**
	 * Close the specified drawer view by animating it into view.
	 *
	 * @param drawerView
	 *            Drawer view to close
	 */
	public void closeDrawer(View drawerView) {
		if (!isDrawerView(drawerView)) {
			throw new IllegalArgumentException("View " + drawerView + " is not a sliding drawer");
		}

		if (mFirstLayout) {
			final LayoutParams lp = (LayoutParams) drawerView.getLayoutParams();
			lp.onScreen = 0.f;
			lp.knownOpen = false;
		} else {
			if (checkDrawerViewAbsoluteGravity(drawerView, Gravity.LEFT)) {
				mLeftDragger.smoothSlideViewTo(drawerView, -drawerView.getWidth(), drawerView.getTop());
			} else {
				mRightDragger.smoothSlideViewTo(drawerView, getWidth(), drawerView.getTop());
			}
		}
		invalidate();
	}

	/**
	 * Close the specified drawer by animating it out of view.
	 *
	 * @param gravity
	 *            Gravity.LEFT to move the left drawer or Gravity.RIGHT for the right. Gravity.START or Gravity.END may
	 *            also be used.
	 */
	public void closeDrawer(int gravity) {
		final View drawerView = findDrawerWithGravity(gravity);
		if (drawerView == null) {
			throw new IllegalArgumentException("No drawer view found with gravity " + gravityToString(gravity));
		}
		closeDrawer(drawerView);
	}

	/**
	 * Check if the given drawer view is currently in an open state. To be considered "open" the drawer must have
	 * settled into its fully visible state. To check for partial visibility use
	 * {@link #isDrawerVisible(android.view.View)}.
	 *
	 * @param drawer
	 *            Drawer view to check
	 * @return true if the given drawer view is in an open state
	 * @see #isDrawerVisible(android.view.View)
	 */
	public boolean isDrawerOpen(View drawer) {
		if (!isDrawerView(drawer)) {
			throw new IllegalArgumentException("View " + drawer + " is not a drawer");
		}
		return ((LayoutParams) drawer.getLayoutParams()).knownOpen;
	}

	/**
	 * Check if the given drawer view is currently in an open state. To be considered "open" the drawer must have
	 * settled into its fully visible state. If there is no drawer with the given gravity this method will return false.
	 *
	 * @param drawerGravity
	 *            Gravity of the drawer to check
	 * @return true if the given drawer view is in an open state
	 */
	public boolean isDrawerOpen(int drawerGravity) {
		final View drawerView = findDrawerWithGravity(drawerGravity);
		return drawerView != null && isDrawerOpen(drawerView);
	}

	/**
	 * Check if a given drawer view is currently visible on-screen. The drawer may be only peeking onto the screen,
	 * fully extended, or anywhere inbetween.
	 *
	 * @param drawer
	 *            Drawer view to check
	 * @return true if the given drawer is visible on-screen
	 * @see #isDrawerOpen(android.view.View)
	 */
	public boolean isDrawerVisible(View drawer) {
		if (!isDrawerView(drawer)) {
			throw new IllegalArgumentException("View " + drawer + " is not a drawer");
		}
		return ((LayoutParams) drawer.getLayoutParams()).onScreen > 0;
	}

	/**
	 * Check if a given drawer view is currently visible on-screen. The drawer may be only peeking onto the screen,
	 * fully extended, or anywhere in between. If there is no drawer with the given gravity this method will return
	 * false.
	 *
	 * @param drawerGravity
	 *            Gravity of the drawer to check
	 * @return true if the given drawer is visible on-screen
	 */
	public boolean isDrawerVisible(int drawerGravity) {
		final View drawerView = findDrawerWithGravity(drawerGravity);
		return drawerView != null && isDrawerVisible(drawerView);
	}

	private boolean hasPeekingDrawer() {
		final int childCount = getChildCount();
		for (int i = 0; i < childCount; i++) {
			final LayoutParams lp = (LayoutParams) getChildAt(i).getLayoutParams();
			if (lp.isPeeking) {
				return true;
			}
		}
		return false;
	}

	@Override
	protected ViewGroup.LayoutParams generateDefaultLayoutParams() {
		return new LayoutParams(android.view.ViewGroup.LayoutParams.MATCH_PARENT, android.view.ViewGroup.LayoutParams.MATCH_PARENT);
	}

	@Override
	protected ViewGroup.LayoutParams generateLayoutParams(ViewGroup.LayoutParams p) {
		return p instanceof LayoutParams ? new LayoutParams((LayoutParams) p) : p instanceof ViewGroup.MarginLayoutParams ? new LayoutParams(
				(MarginLayoutParams) p) : new LayoutParams(p);
	}

	@Override
	protected boolean checkLayoutParams(ViewGroup.LayoutParams p) {
		return p instanceof LayoutParams && super.checkLayoutParams(p);
	}

	@Override
	public ViewGroup.LayoutParams generateLayoutParams(AttributeSet attrs) {
		return new LayoutParams(getContext(), attrs);
	}

	private boolean hasVisibleDrawer() {
		return findVisibleDrawer() != null;
	}

	View findVisibleDrawer() {
		final int childCount = getChildCount();
		for (int i = 0; i < childCount; i++) {
			final View child = getChildAt(i);
			if (isDrawerView(child) && isDrawerVisible(child)) {
				return child;
			}
		}
		return null;
	}

	void cancelChildViewTouch() {
		// Cancel child touches
		if (!mChildrenCanceledTouch) {
			final long now = SystemClock.uptimeMillis();
			final MotionEvent cancelEvent = MotionEvent.obtain(now, now, MotionEvent.ACTION_CANCEL, 0.0f, 0.0f, 0);
			final int childCount = getChildCount();
			for (int i = 0; i < childCount; i++) {
				getChildAt(i).dispatchTouchEvent(cancelEvent);
			}
			cancelEvent.recycle();
			mChildrenCanceledTouch = true;
		}
	}

	@Override
	public boolean onKeyDown(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK && hasVisibleDrawer()) {
			event.startTracking();
			return true;
		}
		return super.onKeyDown(keyCode, event);
	}

	@Override
	public boolean onKeyUp(int keyCode, KeyEvent event) {
		if (keyCode == KeyEvent.KEYCODE_BACK) {
			final View visibleDrawer = findVisibleDrawer();
			if (visibleDrawer != null && getDrawerLockMode(visibleDrawer) == LOCK_MODE_UNLOCKED) {
				closeDrawers();
			}
			return visibleDrawer != null;
		}
		return super.onKeyUp(keyCode, event);
	}

	@Override
	protected void onRestoreInstanceState(Parcelable state) {
		final SavedState ss = (SavedState) state;
		super.onRestoreInstanceState(ss.getSuperState());

		if (ss.openDrawerGravity != Gravity.NO_GRAVITY) {
			final View toOpen = findDrawerWithGravity(ss.openDrawerGravity);
			if (toOpen != null) {
				openDrawer(toOpen);
			}
		}

		setDrawerLockMode(ss.lockModeLeft, Gravity.LEFT);
		setDrawerLockMode(ss.lockModeRight, Gravity.RIGHT);
	}

	@Override
	protected Parcelable onSaveInstanceState() {
		final Parcelable superState = super.onSaveInstanceState();
		final SavedState ss = new SavedState(superState);

		final View openDrawer = findOpenDrawer();
		if (openDrawer != null) {
			ss.openDrawerGravity = ((LayoutParams) openDrawer.getLayoutParams()).gravity;
		}

		ss.lockModeLeft = mLockModeLeft;
		ss.lockModeRight = mLockModeRight;

		return ss;
	}

	@Override
	public void addView(View child, int index, ViewGroup.LayoutParams params) {
		super.addView(child, index, params);

		final View openDrawer = findOpenDrawer();
		if (openDrawer != null || isDrawerView(child)) {
			// A drawer is already open or the new view is a drawer, so the
			// new view should start out hidden.
			child.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS);
		} else {
			// Otherwise this is a content view and no drawer is open, so the
			// new view should start out visible.
			child.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_YES);
		}

		// We only need a delegate here if the framework doesn't understand
		// NO_HIDE_DESCENDANTS importance.
		if (!CAN_HIDE_DESCENDANTS) {
			child.setAccessibilityDelegate(mChildAccessibilityDelegate);
		}
	}

	static boolean includeChildForAccessibility(View child) {
		// If the child is not important for accessibility we make
		// sure this hides the entire subtree rooted at it as the
		// IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDATS is not
		// supported on older platforms but we want to hide the entire
		// content and not opened drawers if a drawer is opened.
		return child.getImportantForAccessibility() != View.IMPORTANT_FOR_ACCESSIBILITY_NO_HIDE_DESCENDANTS
				&& child.getImportantForAccessibility() != View.IMPORTANT_FOR_ACCESSIBILITY_NO;
	}

	/**
	 * State persisted across instances
	 */
	protected static class SavedState extends BaseSavedState {
		int openDrawerGravity = Gravity.NO_GRAVITY;
		int lockModeLeft = LOCK_MODE_UNLOCKED;
		int lockModeRight = LOCK_MODE_UNLOCKED;

		public SavedState(Parcel in) {
			super(in);
			openDrawerGravity = in.readInt();
		}

		public SavedState(Parcelable superState) {
			super(superState);
		}

		@Override
		public void writeToParcel(Parcel dest, int flags) {
			super.writeToParcel(dest, flags);
			dest.writeInt(openDrawerGravity);
		}
	}

	private class ViewDragCallback extends ViewDragHelper.Callback {
		private final int mAbsGravity;
		private ViewDragHelper mDragger;

		private final Runnable mPeekRunnable = new Runnable() {
			@Override
			public void run() {
				peekDrawer();
			}
		};

		public ViewDragCallback(int gravity) {
			mAbsGravity = gravity;
		}

		public void setDragger(ViewDragHelper dragger) {
			mDragger = dragger;
		}

		public void removeCallbacks() {
			DrawerLayout.this.removeCallbacks(mPeekRunnable);
		}

		@Override
		public boolean tryCaptureView(View child, int pointerId) {
			// Only capture views where the gravity matches what we're looking for.
			// This lets us use two ViewDragHelpers, one for each side drawer.
			return isDrawerView(child) && checkDrawerViewAbsoluteGravity(child, mAbsGravity) && getDrawerLockMode(child) == LOCK_MODE_UNLOCKED;
		}

		@Override
		public void onViewDragStateChanged(int state) {
			updateDrawerState(state, mDragger.getCapturedView());
		}

		@Override
		public void onViewPositionChanged(View changedView, int left, int top, int dx, int dy) {
			float offset;
			final int childWidth = changedView.getWidth();

			// This reverses the positioning shown in onLayout.
			if (checkDrawerViewAbsoluteGravity(changedView, Gravity.LEFT)) {
				offset = (float) (childWidth + left) / childWidth;
			} else {
				final int width = getWidth();
				offset = (float) (width - left) / childWidth;
			}
			setDrawerViewOffset(changedView, offset);
			changedView.setVisibility(offset == 0 ? INVISIBLE : VISIBLE);
			invalidate();
		}

		@Override
		public void onViewCaptured(View capturedChild, int activePointerId) {
			final LayoutParams lp = (LayoutParams) capturedChild.getLayoutParams();
			lp.isPeeking = false;

			closeOtherDrawer();
		}

		private void closeOtherDrawer() {
			final int otherGrav = mAbsGravity == Gravity.LEFT ? Gravity.RIGHT : Gravity.LEFT;
			final View toClose = findDrawerWithGravity(otherGrav);
			if (toClose != null) {
				closeDrawer(toClose);
			}
		}

		@Override
		public void onViewReleased(View releasedChild, float xvel, float yvel) {
			// Offset is how open the drawer is, therefore left/right values
			// are reversed from one another.
			final float offset = getDrawerViewOffset(releasedChild);
			final int childWidth = releasedChild.getWidth();

			int left;
			if (checkDrawerViewAbsoluteGravity(releasedChild, Gravity.LEFT)) {
				left = xvel > 0 || xvel == 0 && offset > 0.5f ? 0 : -childWidth;
			} else {
				final int width = getWidth();
				left = xvel < 0 || xvel == 0 && offset > 0.5f ? width - childWidth : width;
			}

			mDragger.settleCapturedViewAt(left, releasedChild.getTop());
			invalidate();
		}

		@Override
		public void onEdgeTouched(int edgeFlags, int pointerId) {
			postDelayed(mPeekRunnable, PEEK_DELAY);
		}

		void peekDrawer() {
			final View toCapture;
			final int childLeft;
			final int peekDistance = mDragger.getEdgeSize();
			final boolean leftEdge = mAbsGravity == Gravity.LEFT;
			if (leftEdge) {
				toCapture = findDrawerWithGravity(Gravity.LEFT);
				childLeft = (toCapture != null ? -toCapture.getWidth() : 0) + peekDistance;
			} else {
				toCapture = findDrawerWithGravity(Gravity.RIGHT);
				childLeft = getWidth() - peekDistance;
			}
			// Only peek if it would mean making the drawer more visible and the drawer isn't locked
			if (toCapture != null && ((leftEdge && toCapture.getLeft() < childLeft) || (!leftEdge && toCapture.getLeft() > childLeft))
					&& getDrawerLockMode(toCapture) == LOCK_MODE_UNLOCKED) {
				final LayoutParams lp = (LayoutParams) toCapture.getLayoutParams();
				mDragger.smoothSlideViewTo(toCapture, childLeft, toCapture.getTop());
				lp.isPeeking = true;
				invalidate();

				closeOtherDrawer();

				cancelChildViewTouch();
			}
		}

		@Override
		public boolean onEdgeLock(int edgeFlags) {
			if (ALLOW_EDGE_LOCK) {
				final View drawer = findDrawerWithGravity(mAbsGravity);
				if (drawer != null && !isDrawerOpen(drawer)) {
					closeDrawer(drawer);
				}
				return true;
			}
			return false;
		}

		@Override
		public void onEdgeDragStarted(int edgeFlags, int pointerId) {
			final View toCapture;
			if ((edgeFlags & ViewDragHelper.EDGE_LEFT) == ViewDragHelper.EDGE_LEFT) {
				toCapture = findDrawerWithGravity(Gravity.LEFT);
			} else {
				toCapture = findDrawerWithGravity(Gravity.RIGHT);
			}

			if (toCapture != null && getDrawerLockMode(toCapture) == LOCK_MODE_UNLOCKED) {
				mDragger.captureChildView(toCapture, pointerId);
			}
		}

		@Override
		public int getViewHorizontalDragRange(View child) {
			return isDrawerView(child) ? child.getWidth() : 0;
		}

		@Override
		public int clampViewPositionHorizontal(View child, int left, int dx) {
			if (checkDrawerViewAbsoluteGravity(child, Gravity.LEFT)) {
				return Math.max(-child.getWidth(), Math.min(left, 0));
			} else {
				final int width = getWidth();
				return Math.max(width - child.getWidth(), Math.min(left, width));
			}
		}

		@Override
		public int clampViewPositionVertical(View child, int top, int dy) {
			return child.getTop();
		}
	}

	public static class LayoutParams extends ViewGroup.MarginLayoutParams {

		public int gravity = Gravity.NO_GRAVITY;
		float onScreen;
		boolean isPeeking;
		boolean knownOpen;

		public LayoutParams(Context c, AttributeSet attrs) {
			super(c, attrs);

			final TypedArray a = c.obtainStyledAttributes(attrs, LAYOUT_ATTRS);
			this.gravity = a.getInt(0, Gravity.NO_GRAVITY);
			a.recycle();
		}

		public LayoutParams(int width, int height) {
			super(width, height);
		}

		public LayoutParams(int width, int height, int gravity) {
			this(width, height);
			this.gravity = gravity;
		}

		public LayoutParams(LayoutParams source) {
			super(source);
			this.gravity = source.gravity;
		}

		public LayoutParams(ViewGroup.LayoutParams source) {
			super(source);
		}

		public LayoutParams(ViewGroup.MarginLayoutParams source) {
			super(source);
		}
	}

	class AccessibilityDelegate extends android.view.View.AccessibilityDelegate {
		@Override
		public void onInitializeAccessibilityNodeInfo(View host, AccessibilityNodeInfo info) {
			super.onInitializeAccessibilityNodeInfo(host, info);

			info.setClassName(DrawerLayout.class.getName());

			// This view reports itself as focusable so that it can intercept
			// the back button, but we should prevent this view from reporting
			// itself as focusable to accessibility services.
			info.setFocusable(false);
			info.setFocused(false);
		}

		@Override
		public void onInitializeAccessibilityEvent(View host, AccessibilityEvent event) {
			super.onInitializeAccessibilityEvent(host, event);

			event.setClassName(DrawerLayout.class.getName());
		}

		@Override
		public boolean dispatchPopulateAccessibilityEvent(View host, AccessibilityEvent event) {
			// Special case to handle window state change events. As far as
			// accessibility services are concerned, state changes from
			// DrawerLayout invalidate the entire contents of the screen (like
			// an Activity or Dialog) and they should announce the title of the
			// new content.
			if (event.getEventType() == AccessibilityEvent.TYPE_WINDOW_STATE_CHANGED) {
				final List<CharSequence> eventText = event.getText();
				final View visibleDrawer = findVisibleDrawer();
				if (visibleDrawer != null) {
					final int edgeGravity = getDrawerViewAbsoluteGravity(visibleDrawer);
					final CharSequence title = getDrawerTitle(edgeGravity);
					if (title != null) {
						eventText.add(title);
					}
				}

				return true;
			}

			return super.dispatchPopulateAccessibilityEvent(host, event);
		}

		@Override
		public boolean onRequestSendAccessibilityEvent(ViewGroup host, View child, AccessibilityEvent event) {
			return (CAN_HIDE_DESCENDANTS || includeChildForAccessibility(child)) && super.onRequestSendAccessibilityEvent(host, child, event);
		}
	}

	final class ChildAccessibilityDelegate extends AccessibilityDelegate {
		@Override
		public void onInitializeAccessibilityNodeInfo(View child, AccessibilityNodeInfo info) {
			super.onInitializeAccessibilityNodeInfo(child, info);

			if (!includeChildForAccessibility(child)) {
				// If we are ignoring the sub-tree rooted at the child,
				// break the connection to the rest of the node tree.
				// For details refer to includeChildForAccessibility.
				info.setParent(null);
			}
		}
	}
}
