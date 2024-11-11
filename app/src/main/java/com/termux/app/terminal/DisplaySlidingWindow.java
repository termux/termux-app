package com.termux.app.terminal;

import static android.content.pm.ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.ViewGroup;
import android.widget.HorizontalScrollView;

import com.nineoldandroids.view.ViewHelper;
import com.termux.app.TermuxActivity;
import com.termux.app.terminal.utils.ScreenUtils;

public class DisplaySlidingWindow extends HorizontalScrollView {
    public enum ContentType {
        LEFT_CONTENT, CENTER_CONTENT, RIGHT_CONTENT
    }

    private TermuxActivity mTermuxActivity;
    private int mMenuWidth;
    private int mHalfMenuWidth;
    private boolean mIsOperateRight;
    private boolean mIsOperateLeft;
    private boolean mRefreshEnd;
    private ViewGroup mContent;
    private ViewGroup mLeftMenu;
    private ViewGroup mRightMenu;
    private boolean mIsLeftMenuOpen;
    private boolean mIsRightMenuOpen;
    private ContentType mContentType;

    public void setTermuxActivity(TermuxActivity activity) {
        this.mTermuxActivity = activity;
    }

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
    private int mStatusHeight;
    public static boolean mLandscape = false;
    /**
     * dp menu padding from screen edge
     */
    private int mMenuRightPadding;

    public boolean isContentSwitchSlider() {
        return mLockContentSlider;
    }

    private boolean mLockContentSlider;
    private boolean mMenuSwitchSlider;
    private float mDownX, mDownY;
    private boolean mMoving;

    public static void setLandscape(boolean isLandscape) {
        DisplaySlidingWindow.mLandscape = isLandscape;
    }

    public DisplaySlidingWindow(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
        setClickable(true);
        mLockContentSlider = true;
        mContentType = ContentType.CENTER_CONTENT;
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
        if (!mRefreshEnd) {
            remeasure();
            ViewGroup mWrapper = (DisplayWindowLinearLayout) getChildAt(0);
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
        mStatusHeight = ScreenUtils.getStatusHeight();
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(getContext());
        boolean hideCutout = preferences.getBoolean("hideCutout", false);
        if (mLandscape) {
            if (hideCutout) {
                mStatusHeight = 0;
            }
            mMenuRightPadding = mContentWidth * 3 / 5;
        } else {
            mMenuRightPadding = 0;
        }
//        Log.d("remeasure","landscape:"+landscape+",mContentWidth:"+mContentWidth+",mStatusHeight:"+mStatusHeight);
    }

    @Override
    protected void onLayout(boolean changed, int l, int t, int r, int b) {
        super.onLayout(changed, l, t, r, b);
        if (changed) {
            // hide menu at start up
            onResume();
        }
        mRefreshEnd = true;
    }

    @Override
    public boolean onInterceptTouchEvent(MotionEvent ev) {
//        Log.d("onInterceptTouchEvent",String.valueOf(ev.getAction()));
        if (!mLockContentSlider) {
            mTermuxActivity.sendTouchEvent(ev);
            return false;
        }
        if (mMenuSwitchSlider) {
            switch (ev.getAction()) {
                case MotionEvent.ACTION_DOWN: {
                    if (!mMoving) {
                        mDownX = ev.getRawX();
                        mDownY = ev.getRawY();
                        mMoving = true;
                    }
                    break;
                }
                case MotionEvent.ACTION_UP:
                    int scrollX = getScrollX();
                    mMoving = false;
                    float dx = ev.getRawX() - mDownX;
                    float dy = ev.getRawY() - mDownY;
                    if (scrollX <= 0) {
                        if (dx > mMenuWidth * 0.6 && Math.abs(dx) > Math.abs(dy)) {
                            mTermuxActivity.onEdgeReached();
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
                if (mIsOperateLeft) {
                    // area hidden more than half of menu width close it
                    if (scrollX > mHalfMenuWidth) {
                        this.smoothScrollTo(mMenuWidth, 0);
                        //notify listener that left meun opened
                        if (mIsLeftMenuOpen) {
                            mTermuxActivity.onMenuOpen(false, 0);
                            mContentType = ContentType.CENTER_CONTENT;
                        }
                        mIsLeftMenuOpen = false;
                        mLockContentSlider = false;
                        mMenuSwitchSlider = false;
                    } else//open left menu
                    {
                        this.smoothScrollTo(0, 0);
                        if (!mIsLeftMenuOpen) {
                            mTermuxActivity.onMenuOpen(true, 0);
                            mContentType = ContentType.LEFT_CONTENT;
                        }
                        mIsLeftMenuOpen = true;
                        mMenuSwitchSlider = true;
                    }
                }
                //operate right
                if (mIsOperateRight) {
                    if (scrollX > mHalfMenuWidth + mMenuWidth) {
                        this.smoothScrollTo(mMenuWidth + mMenuWidth + mStatusHeight * 4, 0);
                        if (!mIsRightMenuOpen) {
                            mTermuxActivity.onMenuOpen(true, 1);
                            mContentType = ContentType.RIGHT_CONTENT;
                        }
                        mIsRightMenuOpen = true;
                        mMenuSwitchSlider = true;
//					mRightMenu.bringToFront();
                    } else//close right menu
                    {
                        this.smoothScrollTo(mMenuWidth, 0);
                        if (mIsRightMenuOpen) {
                            mTermuxActivity.onMenuOpen(false, 1);
                            mContentType = ContentType.CENTER_CONTENT;
                        }
                        mIsRightMenuOpen = false;
                        mLockContentSlider = false;
                        mMenuSwitchSlider = false;
                    }
                }
                return false;
        }
        if (mLockContentSlider) {
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
            mIsOperateRight = true;
            mIsOperateLeft = false;
        } else {
            mIsOperateRight = false;
            mIsOperateLeft = true;
        }
        float scale = l * 1.0f / mMenuWidth;
        ViewHelper.setTranslationX(mContent, mMenuWidth * (scale - 1));
    }

    public void setX11PreferenceSwitchSlider(boolean openSlider) {
        this.mMenuSwitchSlider = openSlider;
        if (!openSlider) {
            this.smoothScrollTo(mMenuWidth, 0);
            mTermuxActivity.onMenuOpen(false, 1);
            mContentType = ContentType.CENTER_CONTENT;
            mIsRightMenuOpen = false;
            this.mLockContentSlider=false;
        } else {
            this.smoothScrollTo(mMenuWidth + mMenuWidth + mStatusHeight * 4, 0);
            mTermuxActivity.onMenuOpen(true, 1);
            mContentType = ContentType.RIGHT_CONTENT;
            mIsRightMenuOpen = true;
            this.mLockContentSlider=true;
        }
    }

    public void setTerminalViewSwitchSlider(boolean openSlider) {
        this.mMenuSwitchSlider = openSlider;
        if (!openSlider) {
            this.smoothScrollTo(mMenuWidth, 0);
            mTermuxActivity.onMenuOpen(false, 0);
            mContentType = ContentType.CENTER_CONTENT;
            mIsLeftMenuOpen = false;
            this.mLockContentSlider=false;
        } else {
            this.smoothScrollTo(0, 0);
            mTermuxActivity.onMenuOpen(true, 0);
            mContentType = ContentType.LEFT_CONTENT;
            mIsLeftMenuOpen = true;
            this.mLockContentSlider=true;
        }
    }

    public void changeLayoutOrientation(int landscapeOrientation) {
        mRefreshEnd = false;
        mLandscape = (landscapeOrientation != SCREEN_ORIENTATION_PORTRAIT);
        requestLayout();
    }

    public void releaseSlider(boolean open) {
        this.mLockContentSlider = open;
        this.mMenuSwitchSlider = !open;
    }

    public void showContent() {
        this.mLockContentSlider = false;
        this.mMenuSwitchSlider = false;
        mRefreshEnd = false;
        remeasure();
        this.smoothScrollTo(mMenuWidth, 0);
        if (mOnMenuChangeListener != null) {
            mOnMenuChangeListener.onMenuOpen(true, 1);
            mContentType = ContentType.CENTER_CONTENT;
        }
    }

    public void onResume() {
        switch (mContentType) {
            case LEFT_CONTENT: {
                setTerminalViewSwitchSlider(true);
                break;
            }
            case RIGHT_CONTENT: {
                setX11PreferenceSwitchSlider(true);
                break;
            }
            default:
                showContent();
        }
    }
}
