package com.termux.app.terminal;

import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;

import static com.termux.display.utils.TouchScreenUtils.getOrientation;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.widget.HorizontalScrollView;
import android.widget.LinearLayout;

import com.nineoldandroids.view.ViewHelper;
import com.termux.display.utils.ScreenUtils;
import com.termux.R;

public class DisplaySlidingWindow extends HorizontalScrollView {
    private int mMenuWidth;
    private int mHalfMenuWidth;
    private boolean isOperateRight;
    private boolean isOperateLeft;
    private boolean once;
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
    private int mScreenHeight;
    public static boolean landscape = false;
    /**
     * dp menu padding from screen edge
     */
    private int mMenuRightPadding;
    private int verticalPadding;
    private boolean switchSlider;
    private float downX,downY;
    private boolean moving;

    public DisplaySlidingWindow(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        setClickable(true);
        switchSlider = true;
        mScreenWidth = ScreenUtils.getScreenWidth(context);
        mScreenHeight = ScreenUtils.getScreenHeight(context);

        TypedArray a = context.getTheme().obtainStyledAttributes(attrs,
            R.styleable.BinarySlidingMenu, defStyle, 0);
        int n = a.getIndexCount();
        for (int i = 0; i < n; i++) {
            int attr = a.getIndex(i);
            if (attr == R.styleable.BinarySlidingMenu_rightPadding) {
                mMenuRightPadding = a.getDimensionPixelSize(attr,
                    (int) TypedValue.applyDimension(
                        TypedValue.COMPLEX_UNIT_DIP, 50f,
                        getResources().getDisplayMetrics()));// 默认为10DP
                break;
            }
        }
        a.recycle();
        if (landscape) {
            mContentWidth = mScreenHeight;
            mMenuRightPadding = mContentWidth * 3 / 5;
        } else {
            verticalPadding = mMenuRightPadding;
            mContentWidth = mScreenWidth;
        }
    }

    public DisplaySlidingWindow(Context context) {
        this(context, null, 0);
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        /**
         * measure width of content and menu
         */
        if (!once) {
            if (landscape) {
                mContentWidth = mScreenHeight > mScreenWidth ? mScreenHeight : mScreenWidth;
                mMenuRightPadding = mContentWidth * 3 / 5;
            } else {
                mContentWidth = mScreenWidth < mScreenHeight ? mScreenWidth : mScreenHeight;
                mMenuRightPadding = verticalPadding;
            }
            mWrapper = (LinearLayout) getChildAt(0);
            mLeftMenu = (ViewGroup) mWrapper.getChildAt(0);
            mContent = (ViewGroup) mWrapper.getChildAt(1);
            mRightMenu = (ViewGroup) mWrapper.getChildAt(2);

            mMenuWidth = mContentWidth - mMenuRightPadding;
            mHalfMenuWidth = mMenuWidth / 2;
            mLeftMenu.getLayoutParams().width = mMenuWidth;
            mContent.getLayoutParams().width = mContentWidth;
            mRightMenu.getLayoutParams().width = mMenuWidth;

            Log.d("changeLayoutOrientation", "landscape:" + String.valueOf(landscape) + ", mContentWidth" + ":" + String.valueOf(mContentWidth) + ",mScreenHeight:" + String.valueOf(mScreenHeight) + ",mScreenWidth:" + String.valueOf(mScreenWidth));
        }
        super.onMeasure(widthMeasureSpec, heightMeasureSpec);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        if (changed) {
            // hide menu at start up
            this.scrollTo(mMenuWidth, 0);
        }
        once = true;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
//        Log.d("onInterceptTouchEvent",String.valueOf(ev.getAction()));
        if(!switchSlider){
           mOnMenuChangeListener.sendTouchEvent(ev);
           return false;
        }
        return super.onInterceptTouchEvent(ev);
    }

    @Override
    public boolean onTouchEvent(MotionEvent ev) {
//        Log.d("onTouchEvent",String.valueOf(ev.getAction()));
        int action = ev.getAction();
        switch(action){
            case MotionEvent.ACTION_MOVE:{
                if (!moving){
                    downX = ev.getRawX();
                    downY = ev.getRawY();
                    moving=true;
                }
                break;
            }
        }
        switch (action) {
            // open menu if scroll to distance that more than half menu width
            case MotionEvent.ACTION_UP:
                int scrollX = getScrollX();
                moving=false;
                float dx= ev.getRawX()-downX;
                float dy = ev.getRawY()-downY;
                if (scrollX <= 0) {
                    if (dx>mMenuWidth*0.6&&Math.abs(dx)>Math.abs(dy)){
                        mOnMenuChangeListener.onEdgeReached();
                    }
                }
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
                        switchSlider = false;
                    } else//open left menu
                    {
                        this.smoothScrollTo(0, 0);
                        if (!isLeftMenuOpen) {
                            mOnMenuChangeListener.onMenuOpen(true, 0);
                        }
                        isLeftMenuOpen = true;
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
//					mRightMenu.bringToFront();
                    } else//cloae right menu
                    {
                        this.smoothScrollTo(mMenuWidth, 0);
                        if (isRightMenuOpen) {
                            mOnMenuChangeListener.onMenuOpen(false, 1);
                        }
                        isRightMenuOpen = false;
                        switchSlider = false;
                    }
                }
                return false;
        }
        if (switchSlider) {
            super.onTouchEvent(ev);
        } else {
//           return mOnMenuChangeListener.sendTouchEvent(ev);
            return true;
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
        this.switchSlider = openSlider;
        if (!openSlider) {
            this.smoothScrollTo(mMenuWidth, 0);
        } else {
            this.smoothScrollTo(mMenuWidth + mMenuWidth, 0);
        }
    }

    public void setTerminalViewSwitchSlider(boolean openSlider) {
        this.switchSlider = openSlider;
        if (!openSlider) {
            this.smoothScrollTo(mMenuWidth, 0);
        } else {
            this.smoothScrollTo(0, 0);
        }
    }

    public void changeLayoutOrientation(int landscapeOriention) {
        once = false;
        landscape = landscapeOriention == SCREEN_ORIENTATION_LANDSCAPE;
    }
    public void releaseSlider(boolean open){
        this.switchSlider=open;
    }

}
