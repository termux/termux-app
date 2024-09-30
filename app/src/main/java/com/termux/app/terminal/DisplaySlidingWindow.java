package com.termux.app.terminal;

import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;

import android.content.Context;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.widget.HorizontalScrollView;

import com.nineoldandroids.view.ViewHelper;
import com.termux.app.terminal.utils.ScreenUtils;

public class DisplaySlidingWindow extends HorizontalScrollView {
    private int mMenuWidth;
    private int mHalfMenuWidth;
    private boolean isOperateRight;
    private boolean isOperateLeft;
    private boolean refreshEnd;
    private ViewGroup mLeftMenu;
    private ViewGroup mContent;
    private ViewGroup mRightMenu;
    private ViewGroup mWrapper;
    private boolean isLeftMenuOpen;
    private boolean isRightMenuOpen;

    /**
     * listener for menu changed
     */
    public interface OnMenuChangeListener {
        /**
         * @param isOpen true open menu，false close menu
         * @param flag   0 left， 1 right
         */
        void onMenuOpen(boolean isOpen, int flag);

        boolean sendTouchEvent(MotionEvent ev);

        void onEdgeReached();
    }

    public OnMenuChangeListener mOnMenuChangeListener;

    public void setOnMenuOpenListener(OnMenuChangeListener mOnMenuChangeListener) {
        this.mOnMenuChangeListener = mOnMenuChangeListener;
    }

    public DisplaySlidingWindow(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    /**
     * content width
     */
    private int mContentWidth;
    private int mScreenWidth;
    public static boolean landscape = false;
    /**
     * dp menu padding from screen edge
     */
    private int mMenuRightPadding;
    private int verticalPadding;

    public boolean isContentSwitchSlider() {
        return contentSwitchSlider;
    }

    private boolean contentSwitchSlider;
    private boolean menuSwitchSlider;
    private float downX, downY;
    private boolean moving;

    public static void setLandscape(boolean isLandscape) {
        DisplaySlidingWindow.landscape = isLandscape;
    }

    public DisplaySlidingWindow(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);

        setClickable(true);
        contentSwitchSlider = true;
        mScreenWidth = ScreenUtils.getScreenWidth(context);
        remeasure();
    }

    public DisplaySlidingWindow(Context context) {
        this(context, null, 0);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        /**
         * measure width of content and menu
         */
        if (!refreshEnd) {
            remeasure();
            mWrapper = (DisplayWindowLinearLayout) getChildAt(0);
            mLeftMenu = (ViewGroup) mWrapper.getChildAt(0);
            mContent = (ViewGroup) mWrapper.getChildAt(1);
            mRightMenu = (ViewGroup) mWrapper.getChildAt(2);
            mMenuWidth = mContentWidth - mMenuRightPadding;
            mHalfMenuWidth = mMenuWidth / 2;
            mLeftMenu.getLayoutParams().width = mMenuWidth;
            mContent.getLayoutParams().width = mContentWidth;
            mRightMenu.getLayoutParams().width = mMenuWidth;
//            Log.d("changeLayoutOrientation", "landscape:" + String.valueOf(landscape) + ", mContentWidth" + ":" + String.valueOf(mContentWidth) + ",mScreenHeight:" + String.valueOf(mScreenHeight) + ",mScreenWidth:" + String.valueOf(mScreenWidth));
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    private void remeasure() {
        mContentWidth = ScreenUtils.getScreenWidth(getContext());
        if (landscape) {
            mMenuRightPadding = mContentWidth / 2;
        } else {
            mMenuRightPadding = verticalPadding;
        }
//        Log.d("remeasure","landscape:"+landscape+",mContentWidth:"+mContentWidth);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        if (changed) {
            // hide menu at start up
            showContent();
        }
        refreshEnd = true;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
//        Log.d("onInterceptTouchEvent",String.valueOf(ev.getAction()));
        if (!contentSwitchSlider) {
            mOnMenuChangeListener.sendTouchEvent(ev);
            return false;
        }
        if (menuSwitchSlider) {
            switch (ev.getAction()) {
                case MotionEvent.ACTION_DOWN: {
                    if (!moving) {
                        downX = ev.getRawX();
                        downY = ev.getRawY();
                        moving = true;
                    }
                    break;
                }
                case MotionEvent.ACTION_UP:
                    int scrollX = getScrollX();
                    moving = false;
                    float dx = ev.getRawX() - downX;
                    float dy = ev.getRawY() - downY;
                    if (scrollX <= 0) {
                        if (dx > mMenuWidth * 0.6 && Math.abs(dx) > Math.abs(dy)) {
                            mOnMenuChangeListener.onEdgeReached();
                        }
                    }
            }
            return false;
        }
        return super.onInterceptTouchEvent(ev);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
        int action = ev.getAction();
        switch (action) {
            // open menu if scroll to distance that more than half menu width
            case MotionEvent.ACTION_UP:
                int scrollX = getScrollX();
                //operate left
                if (isOperateLeft) {
                    // area hidden more than half of menu width close it
                    if (scrollX > mHalfMenuWidth) {
                        this.smoothScrollTo(mMenuWidth, 0);
                        //notify listener that left meun opened
                        if (isLeftMenuOpen) {
                            mOnMenuChangeListener.onMenuOpen(false, 0);
                        }
                        isLeftMenuOpen = false;
                        contentSwitchSlider = false;
                        menuSwitchSlider = false;
                    } else//open left menu
                    {
                        this.smoothScrollTo(0, 0);
                        if (!isLeftMenuOpen) {
                            mOnMenuChangeListener.onMenuOpen(true, 0);
                        }
                        isLeftMenuOpen = true;
                        menuSwitchSlider = true;
                    }
                }
                //operate right
                if (isOperateRight) {
                    if (scrollX > mHalfMenuWidth + mMenuWidth) {
                        this.smoothScrollTo(mMenuWidth + mMenuWidth, 0);
                        if (!isRightMenuOpen) {
                            mOnMenuChangeListener.onMenuOpen(true, 1);
                        }
                        isRightMenuOpen = true;
                        menuSwitchSlider = true;
//					mRightMenu.bringToFront();
                    } else//close right menu
                    {
                        this.smoothScrollTo(mMenuWidth, 0);
                        if (isRightMenuOpen) {
                            mOnMenuChangeListener.onMenuOpen(false, 1);
                        }
                        isRightMenuOpen = false;
                        contentSwitchSlider = false;
                        menuSwitchSlider = false;
                    }
                }
                return false;
        }
        if (contentSwitchSlider) {
            super.onTouchEvent(ev);
        } else {
            return false;
        }
        return false;
    }

    @Override
    protected void onScrollChanged(int l, int t, int oldl, int oldt) {
        super.onScrollChanged(l, t, oldl, oldt);
//        Log.d("onScrollChanged","l:"+l+", t:"+t+",oldl:"+oldl+",oldt:"+oldt);
        if (l > mMenuWidth) {
            isOperateRight = true;
            isOperateLeft = false;
        } else {
            isOperateRight = false;
            isOperateLeft = true;
        }
        float scale = l * 1.0f / mMenuWidth;
        ViewHelper.setTranslationX(mContent, mMenuWidth * (scale - 1));
    }

    public void setX11PreferenceSwitchSlider(boolean openSlider) {
        this.contentSwitchSlider = openSlider;
        if (!openSlider) {
            this.smoothScrollTo(mMenuWidth, 0);
        } else {
            this.smoothScrollTo(mMenuWidth + mMenuWidth, 0);
            if (!isRightMenuOpen) {
                mOnMenuChangeListener.onMenuOpen(true, 1);
            }
            isRightMenuOpen = true;
        }
    }

    public void setTerminalViewSwitchSlider(boolean openSlider) {
        this.contentSwitchSlider = openSlider;
        this.menuSwitchSlider = !openSlider;
        if (!openSlider) {
            this.smoothScrollTo(mMenuWidth, 0);
        } else {
            this.smoothScrollTo(0, 0);
        }
    }

    public void changeLayoutOrientation(int landscapeOrientation) {
        refreshEnd = false;
        landscape = !(landscapeOrientation == SCREEN_ORIENTATION_PORTRAIT);
        requestLayout();
    }

    public void releaseSlider(boolean open) {
        this.contentSwitchSlider = open;
        this.menuSwitchSlider = !open;
    }

    public void showContent() {
        this.contentSwitchSlider = false;
        this.menuSwitchSlider = true;
        refreshEnd = false;
        remeasure();
        this.scrollTo(mMenuWidth, 0);
    }
}
