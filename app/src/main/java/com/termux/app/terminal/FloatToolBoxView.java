package com.termux.app.terminal;

import android.app.Activity;
import android.content.Context;
import android.graphics.PixelFormat;
import android.os.CountDownTimer;
import android.util.DisplayMetrics;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.RelativeLayout;
import android.widget.Toast;

import com.termux.R;

public class FloatToolBoxView extends RelativeLayout {

    private int millisInFuture = 3;//半隐藏悬浮球倒计时 秒
    private CountDownTimer countDownTimer;//倒计时 半隐藏悬浮球logo定时器

    private final static int LEFT = 0;
    private final static int RIGHT = 1;

    private int defPosition = RIGHT; //可变参数，随着吸附左右改变
    private WindowManager.LayoutParams wmParams;
    private WindowManager wm;
    private int screenHeight;
    private int screenWidth;
    private float mTouchStartX, mTouchStartY;
    private float x, y;
    private boolean isScroll;
    private int dpi;
    private Activity activity;
    private View view;

    public FloatToolBoxView(Activity activity) {
        super(activity);
        this.activity = activity;
        init(activity);
        initTimer();
    }

    public void init(Activity activity) {

        view = LayoutInflater.from(activity).inflate(R.layout.float_window_layout, this);

        DisplayMetrics dm = activity.getResources().getDisplayMetrics();
        int widthPixels = dm.widthPixels;
        int heightPixels = dm.heightPixels;

        wm = (WindowManager) activity.getSystemService(Context.WINDOW_SERVICE);

        //屏宽
        screenWidth = wm.getDefaultDisplay().getWidth();
        //屏高
        screenHeight = wm.getDefaultDisplay().getHeight();
        //通过像素密度来设置按钮的大小
        dpi = dpi(dm.densityDpi);

        wmParams = new WindowManager.LayoutParams();
        wmParams.type = WindowManager.LayoutParams.TYPE_APPLICATION;
        wmParams.format = PixelFormat.RGBA_8888;//设置背景图片
        wmParams.flags = WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE;//
        wmParams.gravity = Gravity.LEFT | Gravity.TOP;//
        wmParams.x = widthPixels; //设置位置像素
        wmParams.y = heightPixels;
        wmParams.width = 150; //设置图片大小
        wmParams.height = 150;
        wm.addView(view, wmParams);


    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // 获取相对屏幕的坐标， 以屏幕左上角为原点
        x = event.getRawX();
        y = event.getRawY();

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                // 获取相对View的坐标，即以此View左上角为原点
                mTouchStartX = event.getX();
                mTouchStartY = event.getY();
                //无论悬浮窗是否处于隐藏状态，点击以后让悬浮窗显示出来即可
                view.setScrollX(0);
                break;
            case MotionEvent.ACTION_MOVE:
                if (isScroll) {
                    updateViewPosition();
                } else {
// 当前不处于连续滑动状态 则滑动小于图标1/3则不滑动
                    if (Math.abs(mTouchStartX - event.getX()) > dpi / 3
                        || Math.abs(mTouchStartY - event.getY()) > dpi / 3) {
                        updateViewPosition();
                    } else {
                        break;
                    }
                }
                isScroll = true;
                break;
            case MotionEvent.ACTION_UP:
                // 拖动
                if (isScroll) {
                    //自动贴边代码增加在此处
                    autoView();
                    //倒计时自动半隐藏
                    countDownTimer.start();
                } else {
                    //点击悬浮窗
                    clickView();
                }
                isScroll = false;
                mTouchStartX = mTouchStartY = 0;
                break;
        }
        return true;
    }

    /**
     * 自动移动位置
     */
    private void autoView() {
        // 得到view在屏幕中的位置
        int[] location = new int[2];
        getLocationOnScreen(location);
        //左侧
        if (location[0] < screenWidth / 2 - getWidth() / 2) {
            updateViewPosition(LEFT);
        } else {
            updateViewPosition(RIGHT);
        }
    }

    /**
     * 更新浮动窗口位置参数
     */
    private void updateViewPosition() {
        wmParams.x = (int) (x - mTouchStartX);
        // 不设置为全屏(状态栏存在) 标题栏是屏幕的1/25
        wmParams.y = (int) (y - mTouchStartY - screenHeight / 25);
        wm.updateViewLayout(this, wmParams);
    }

    /**
     * 手指释放更新悬浮窗位置
     */
    private void updateViewPosition(int l) {
        switch (l) {
            case LEFT:
                defPosition = LEFT;
                //吸附后开启倒计时,倒计时结束后缩小图标
                wmParams.x = 0;
                break;
            case RIGHT:
                defPosition = RIGHT;
                int x = screenWidth - dpi;
                wmParams.x = x;
                break;
        }
        wm.updateViewLayout(this, wmParams);
    }

    /**
     * 根据密度选择控件大小
     */
    private int dpi(int densityDpi) {

        if (densityDpi <= 120) {

            return 36;

        } else if (densityDpi <= 160) {

            return 48;

        } else if (densityDpi <= 240) {

            return 72;

        } else if (densityDpi <= 320) {

            return 96;

        }

        return 108;

    }

    private void initTimer() {
        countDownTimer = new CountDownTimer(millisInFuture * 1000, 1000) {
            @Override
            public void onTick(long millisUntilFinished) {
                if (isScroll) {
                    timeCancel();
                }
            }

            @Override
            public void onFinish() {
                System.out.println("倒计时完成");
                if (!isScroll) {
                    if (defPosition == LEFT) {

                        view.setScrollX(view.getWidth() / 2);
                    } else {

                        view.setScrollX(-view.getWidth() / 2);
                    }
                    wm.updateViewLayout(view, wmParams);

                } else {
                    timeCancel();
                }
            }
        };
        countDownTimer.start();
    }

    /**
     * 取消倒计时
     */
    private void timeCancel() {
        countDownTimer.cancel();
    }

    //当悬浮按钮被点击
    public void clickView() {
        Toast.makeText(activity, "悬浮按钮被点击！", Toast.LENGTH_SHORT).show();
    }
}
