/*
 * Copyright (C) 2012 Capricorn
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

package com.termux.floatball.menu;

import android.app.Activity;
import android.content.Context;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.ImageView;

import com.termux.floatball.FloatBallManager;
import com.termux.floatball.FloatBallUtil;

public class FloatMenu extends FrameLayout {
    private MenuLayout mMenuLayout;

    private ImageView mIconView;
    private int mPosition;
    private int mItemSize;
    private int mSize;
    private int mDuration = 250;

    private FloatBallManager floatBallManager;
    private WindowManager.LayoutParams mLayoutParams;
    private boolean isAdded = false;
    private int mBallSize;
    private FloatMenuCfg mConfig;
    private boolean mListenBackEvent = true;

    public FloatMenu(Context context, final FloatBallManager floatBallManager, FloatMenuCfg config) {
        super(context);
        this.floatBallManager = floatBallManager;
        if (config == null) return;
        mConfig = config;
        mItemSize = mConfig.mItemSize;
        mSize = mConfig.mSize;
        init(context);
        mMenuLayout.setChildSize(mItemSize);
    }

    private void initLayoutParams(Context context) {
        mLayoutParams = FloatBallUtil.getLayoutParams(context, mListenBackEvent);
    }

    public void removeViewTreeObserver(ViewTreeObserver.OnGlobalLayoutListener listener) {
        getViewTreeObserver().removeOnGlobalLayoutListener(listener);
    }

    public int getSize() {
        return mSize;
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getAction();
        int x = (int) event.getRawX();
        int y = (int) event.getRawY();
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                break;
            case MotionEvent.ACTION_MOVE:
                break;
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_CANCEL:
            case MotionEvent.ACTION_OUTSIDE:
                if (mMenuLayout.isExpanded()) {
                    toggle(mDuration);
                }
                break;
        }
        return super.onTouchEvent(event);
    }

    public void attachToWindow(WindowManager windowManager) {
        if (!isAdded) {
            mBallSize = floatBallManager.getBallSize();
            mLayoutParams.x = floatBallManager.floatBallX;
            mLayoutParams.y = floatBallManager.floatBallY - mSize / 2;
            mPosition = computeMenuLayout(mLayoutParams);
            refreshPathMenu(mPosition);
            toggle(mDuration);
            windowManager.addView(this, mLayoutParams);
            isAdded = true;
        }
    }

    public void detachFromWindow(WindowManager windowManager) {
        if (isAdded) {
            toggle(0);
            mMenuLayout.setVisibility(GONE);
            if (getContext() instanceof Activity) {
                windowManager.removeViewImmediate(this);
            } else {
                windowManager.removeView(this);
            }
            isAdded = false;
        }
    }

    private void addMenuLayout(Context context) {
        mMenuLayout = new MenuLayout(context);
        ViewGroup.LayoutParams layoutParams = new ViewGroup.LayoutParams(mSize, mSize);
        addView(mMenuLayout, layoutParams);
        mMenuLayout.setVisibility(INVISIBLE);
    }

    private void addControllLayout(Context context) {
        mIconView = new ImageView(context);
        LayoutParams sublayoutParams = new LayoutParams(mBallSize, mBallSize);
        addView(mIconView, sublayoutParams);
    }

    private void init(Context context) {
        initLayoutParams(context);
        mLayoutParams.height = mSize;
        mLayoutParams.width = mSize;
        addMenuLayout(context);
        addControllLayout(context);
        mIconView.setOnClickListener(new OnClickListener() {
            @Override
            public void onClick(View v) {
                closeMenu();
            }
        });
        if (mListenBackEvent) {
            setOnKeyListener((v, keyCode, event) -> {
                int action = event.getAction();
                if (action == KeyEvent.ACTION_DOWN) {
                    if (keyCode == KeyEvent.KEYCODE_BACK) {
                        FloatMenu.this.floatBallManager.closeMenu();
                        return true;
                    }
                }
                return false;
            });
            setFocusableInTouchMode(true);
        }
    }

    public void closeMenu() {
        if (mMenuLayout.isExpanded()) {
            toggle(mDuration);
        }
    }

    public void remove() {
        floatBallManager.reset();
        mMenuLayout.setExpand(false);
    }

    private void toggle(final int duration) {
        //duration==0 indicate that close the menu, so if it has closed, do nothing.
        if (!mMenuLayout.isExpanded() && duration <= 0) return;
        mMenuLayout.setVisibility(VISIBLE);
        if (getWidth() == 0) {
            getViewTreeObserver().addOnGlobalLayoutListener(new ViewTreeObserver.OnGlobalLayoutListener() {
                @Override
                public void onGlobalLayout() {
                    mMenuLayout.switchState(mPosition, duration);
                    removeViewTreeObserver(this);
                }
            });
        } else {
            mMenuLayout.switchState(mPosition, duration);
        }
    }

    public boolean isMoving() {
        return mMenuLayout.isMoving();
    }

    public void addItem(final MenuItem menuItem) {
        if (mConfig == null) return;
        ImageView imageview = new ImageView(getContext());
        imageview.setBackground(menuItem.mDrawable);
        mMenuLayout.addView(imageview);
        imageview.setOnClickListener(v -> {
            if (!mMenuLayout.isMoving())
                menuItem.action();
        });
    }

    public void removeAllItemViews() {
        mMenuLayout.removeAllViews();
    }

    /**
     * adjustment menu layout with float ball position
     */
    public void refreshPathMenu(int position) {
        LayoutParams menuLp = (LayoutParams) mMenuLayout.getLayoutParams();
        LayoutParams iconLp = (LayoutParams) mIconView.getLayoutParams();

        switch (position) {
            case LEFT_TOP://left top
                iconLp.gravity = Gravity.LEFT | Gravity.TOP;
                menuLp.gravity = Gravity.LEFT | Gravity.TOP;
                mMenuLayout.setArc(0, 90, position);
                break;
            case LEFT_CENTER://left middle
                iconLp.gravity = Gravity.LEFT | Gravity.CENTER_VERTICAL;
                menuLp.gravity = Gravity.LEFT | Gravity.CENTER_VERTICAL;
                mMenuLayout.setArc(270, 270 + 180, position);
                break;
            case LEFT_BOTTOM://left bottom
                iconLp.gravity = Gravity.LEFT | Gravity.BOTTOM;
                menuLp.gravity = Gravity.LEFT | Gravity.BOTTOM;
                mMenuLayout.setArc(270, 360, position);
                break;
            case RIGHT_TOP://right top
                iconLp.gravity = Gravity.RIGHT | Gravity.TOP;
                menuLp.gravity = Gravity.RIGHT | Gravity.TOP;
                mMenuLayout.setArc(90, 180, position);
                break;
            case RIGHT_CENTER://right middle
                iconLp.gravity = Gravity.RIGHT | Gravity.CENTER_VERTICAL;
                menuLp.gravity = Gravity.RIGHT | Gravity.CENTER_VERTICAL;
                mMenuLayout.setArc(90, 270, position);
                break;
            case RIGHT_BOTTOM://right bottom
                iconLp.gravity = Gravity.BOTTOM | Gravity.RIGHT;
                menuLp.gravity = Gravity.BOTTOM | Gravity.RIGHT;
                mMenuLayout.setArc(180, 270, position);
                break;

            case CENTER_TOP://up middle
                iconLp.gravity = Gravity.TOP | Gravity.CENTER_HORIZONTAL;
                menuLp.gravity = Gravity.TOP | Gravity.CENTER_HORIZONTAL;
                mMenuLayout.setArc(0, 180, position);
                break;
            case CENTER_BOTTOM://bottom middle
                iconLp.gravity = Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;
                menuLp.gravity = Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL;
                mMenuLayout.setArc(180, 360, position);
                break;
            case CENTER:
                iconLp.gravity = Gravity.CENTER;
                menuLp.gravity = Gravity.CENTER;
                mMenuLayout.setArc(0, 360, position);
                break;
        }
        mIconView.setLayoutParams(iconLp);
        mMenuLayout.setLayoutParams(menuLp);
    }

    /**
     * calculate position of every menu item's position
     *
     * @return
     */
    public int computeMenuLayout(WindowManager.LayoutParams layoutParams) {
        int position = FloatMenu.RIGHT_CENTER;
        final int halfBallSize = mBallSize / 2;
        final int screenWidth = floatBallManager.mScreenWidth;
        final int screenHeight = floatBallManager.mScreenHeight;
        final int floatballCenterY = floatBallManager.floatBallY + halfBallSize;

        int wmX = floatBallManager.floatBallX;
        int wmY = floatballCenterY;

        if (wmX <= screenWidth / 3) //left  portrait
        {
            wmX = 0;
            if (wmY <= mSize / 2) {
                position = FloatMenu.LEFT_TOP;//left top
                wmY = floatballCenterY - halfBallSize;
            } else if (wmY > screenHeight - mSize / 2) {
                position = FloatMenu.LEFT_BOTTOM;//left bottom
                wmY = floatballCenterY - mSize + halfBallSize;
            } else {
                position = FloatMenu.LEFT_CENTER;//left middle
                wmY = floatballCenterY - mSize / 2;
            }
        } else if (wmX >= screenWidth * 2 / 3)//right portrait
        {
            wmX = screenWidth - mSize;
            if (wmY <= mSize / 2) {
                position = FloatMenu.RIGHT_TOP;//right top
                wmY = floatballCenterY - halfBallSize;
            } else if (wmY > screenHeight - mSize / 2) {
                position = FloatMenu.RIGHT_BOTTOM;//right bottom
                wmY = floatballCenterY - mSize + halfBallSize;
            } else {
                position = FloatMenu.RIGHT_CENTER;//right middle
                wmY = floatballCenterY - mSize / 2;
            }
        }
        layoutParams.x = wmX;
        layoutParams.y = wmY;
        return position;
    }

    public static final int LEFT_TOP = 1;
    public static final int CENTER_TOP = 2;
    public static final int RIGHT_TOP = 3;
    public static final int LEFT_CENTER = 4;
    public static final int CENTER = 5;
    public static final int RIGHT_CENTER = 6;
    public static final int LEFT_BOTTOM = 7;
    public static final int CENTER_BOTTOM = 8;
    public static final int RIGHT_BOTTOM = 9;
}
