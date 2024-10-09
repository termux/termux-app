package com.termux.app.terminal;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.drawable.Drawable;
import android.preference.PreferenceManager;
import android.widget.Toast;

import androidx.appcompat.content.res.AppCompatResources;

import com.termux.R;
import com.termux.app.TermuxActivity;
import com.termux.floatball.FloatBallManager;
import com.termux.floatball.menu.FloatMenuCfg;
import com.termux.floatball.menu.MenuItem;
import com.termux.floatball.permission.FloatPermissionManager;
import com.termux.floatball.utils.DensityUtil;
import com.termux.floatball.widget.FloatBallCfg;

public class FloatBallMenuClient {
    private FloatBallManager mFloatballManager;
    private FloatPermissionManager mFloatPermissionManager;
    //    private ActivityLifeCycleListener mActivityLifeCycleListener = new ActivityLifeCycleListener();
    private int resumed;
    private TermuxActivity mTermuxActivity;

    private FloatBallMenuClient() {
    }

    public FloatBallMenuClient(TermuxActivity termuxActivity) {
        mTermuxActivity = termuxActivity;
    }

    public void showFloatBall() {
        mFloatballManager.show();
    }

    public void onCreate() {
        boolean showMenu = true;
        init(showMenu);
        mFloatballManager.show();
        //5 set float ball click handler
        if (mFloatballManager.getMenuItemSize() == 0) {
            mFloatballManager.setOnFloatBallClickListener(new FloatBallManager.OnFloatBallClickListener() {
                @Override
                public void onFloatBallClick() {
                    toast("点击了悬浮球");
                }
            });
        }
        //     6 if only float ball within app, register it to Application(out data, actually, it is enough within activity )
//      mTermuxActivity.getApplication().registerActivityLifecycleCallbacks(mActivityLifeCycleListener);
    }

    public void onAttachedToWindow() {
        try {
            mFloatballManager.show();
            mFloatballManager.onFloatBallClick();
        } catch (RuntimeException e) {
            e.printStackTrace();
            toast(mTermuxActivity.getString(R.string.apply_display_over_other_app_permission));
        }

    }

    public void onDetachedFromWindow() {
        mFloatballManager.hide();
    }

    private void init(boolean showMenu) {
//      1 set position of float ball, set size, icon and drawable
        int ballSize = DensityUtil.dip2px(mTermuxActivity, 45);
        Drawable ballIcon = AppCompatResources.getDrawable(mTermuxActivity, R.drawable.icon_float_ball_shape);
//      different config below
//      FloatBallCfg ballCfg = new FloatBallCfg(ballSize, ballIcon);
//      FloatBallCfg ballCfg = new FloatBallCfg(ballSize, ballIcon, FloatBallCfg.Gravity.LEFT_CENTER,false);
//      FloatBallCfg ballCfg = new FloatBallCfg(ballSize, ballIcon, FloatBallCfg.Gravity.LEFT_BOTTOM, -100);
//      FloatBallCfg ballCfg = new FloatBallCfg(ballSize, ballIcon, FloatBallCfg.Gravity.RIGHT_TOP, 100);
        FloatBallCfg ballCfg = new FloatBallCfg(ballSize, ballIcon, FloatBallCfg.Gravity.RIGHT_CENTER);
        //     set float ball weather hide
        ballCfg.setHideHalfLater(true);
        SharedPreferences preferences = PreferenceManager.getDefaultSharedPreferences(mTermuxActivity);
        boolean floatBallOverOtherApp = preferences.getBoolean("floatBallOverApp", false);
        Context ctx = mTermuxActivity;
        if (floatBallOverOtherApp) {
            ctx = mTermuxActivity.getApplicationContext();
        }
        if (showMenu) {
            //2 display float ball menu
            //2.1 init float ball menu config, every size of menu item and number of item
            int menuSize = DensityUtil.dip2px(mTermuxActivity, 160);
            int menuItemSize = DensityUtil.dip2px(mTermuxActivity, 30);
            FloatMenuCfg menuCfg = new FloatMenuCfg(menuSize, menuItemSize);
            //3 create float ball Manager
            mFloatballManager = new FloatBallManager(ctx, ballCfg, menuCfg);
            addFloatMenuItem();

        } else {
            mFloatballManager = new FloatBallManager(ctx, ballCfg);
        }
        mFloatballManager.setFloatBallOverOtherApp(floatBallOverOtherApp);
        if (floatBallOverOtherApp) {
            setFloatPermission();
        }
    }

    private void setFloatPermission() {
        // set 'display over other app' permission of float bal menu
        //once permission, float ball never show
        mFloatPermissionManager = new FloatPermissionManager();
        mFloatballManager.setPermission(new FloatBallManager.IFloatBallPermission() {
            @Override
            public boolean onRequestFloatBallPermission() {
                requestFloatBallPermission(mTermuxActivity);
                return true;
            }

            @Override
            public boolean hasFloatBallPermission(Context context) {
                return mFloatPermissionManager.checkPermission(context);
            }

            @Override
            public void requestFloatBallPermission(Activity activity) {
                mFloatPermissionManager.applyPermission(activity);
            }

        });
    }

//    public class ActivityLifeCycleListener implements Application.ActivityLifecycleCallbacks {
//
//        @Override
//        public void onActivityCreated(Activity activity, Bundle savedInstanceState) {
//        }
//
//        @Override
//        public void onActivityStarted(Activity activity) {
//        }
//
//        @Override
//        public void onActivityResumed(Activity activity) {
//            ++resumed;
//            setFloatballVisible(true);
//        }
//
//        @Override
//        public void onActivityPaused(Activity activity) {
//            --resumed;
//            if (!isApplicationInForeground()) {
//                setFloatballVisible(false);
//            }
//        }
//
//        @Override
//        public void onActivityStopped(Activity activity) {
//        }
//
//        @Override
//        public void onActivityDestroyed(Activity activity) {
//        }
//
//        @Override
//        public void onActivitySaveInstanceState(Activity activity, Bundle outState) {
//        }
//    }

    private void toast(String msg) {
        Toast.makeText(mTermuxActivity, msg, Toast.LENGTH_SHORT).show();
    }

    private void addFloatMenuItem() {
        MenuItem terminalItem = new MenuItem(mTermuxActivity.getDrawable(R.drawable.icon_menu_start_terminal_shape)) {
            @Override
            public void action() {
                mTermuxActivity.getmSlideWindowLayout().setTerminalViewSwitchSlider(true);
                toast(mTermuxActivity.getString(R.string.open_terminal));
                mFloatballManager.closeMenu();
            }
        };
        MenuItem stopItem = new MenuItem(mTermuxActivity.getDrawable(R.drawable.icon_menu_kill_current_process_shape)) {
            @Override
            public void action() {
                toast(mTermuxActivity.getString(R.string.terminate_current_process));
                mFloatballManager.closeMenu();
            }
        };
        MenuItem gamePadItem = new MenuItem(mTermuxActivity.getDrawable(R.drawable.icon_menu_game_pad_shape)) {
            @Override
            public void action() {
                toast(mTermuxActivity.getString(com.termux.x11.R.string.open_controller));
                mFloatballManager.closeMenu();
            }
        };
        MenuItem unLockLayoutItem = new MenuItem(mTermuxActivity.getDrawable(R.drawable.icon_menu_unlock_layout_shape)) {
            @Override
            public void action() {
                mTermuxActivity.getmSlideWindowLayout().releaseSlider(true);
                toast(mTermuxActivity.getString(R.string.lock_layout));
                mFloatballManager.closeMenu();
            }
        };
        MenuItem keyboardItem = new MenuItem(mTermuxActivity.getDrawable(R.drawable.icon_menu_show_keyboard_shape)) {
            @Override
            public void action() {
                mTermuxActivity.switchSoftKeyboard(true);
                toast(mTermuxActivity.getString(com.termux.x11.R.string.open_keyboard));
            }
        };
        MenuItem taskManagerItem = new MenuItem(mTermuxActivity.getDrawable(R.drawable.icon_menu_show_task_manager_shape)) {
            @Override
            public void action() {
                mTermuxActivity.showProgressManagerDialog();
                toast(mTermuxActivity.getString(com.termux.x11.R.string.task_manager));
                mFloatballManager.closeMenu();
            }
        };
        MenuItem settingItem = new MenuItem(mTermuxActivity.getDrawable(R.drawable.icon_menu_show_setting_shape)) {
            @Override
            public void action() {
                mTermuxActivity.getmSlideWindowLayout().setX11PreferenceSwitchSlider(true);
                toast(mTermuxActivity.getString(com.termux.x11.R.string.settings));
                mFloatballManager.closeMenu();
            }
        };
        mFloatballManager.addMenuItem(terminalItem)
            .addMenuItem(stopItem)
            .addMenuItem(keyboardItem)
            .addMenuItem(gamePadItem)
            .addMenuItem(unLockLayoutItem)
            .addMenuItem(taskManagerItem)
            .addMenuItem(settingItem)
            .buildMenu();
    }

//    private void setFloatballVisible(boolean visible) {
//        if (visible) {
//            mFloatballManager.show();
//        } else {
//            mFloatballManager.hide();
//        }
//    }

    public boolean isApplicationInForeground() {
        return resumed > 0;
    }

    public void onDestroy() {
        mTermuxActivity.onDestroy();
        //unregister ActivityLifeCycle listener once register it, in case of memory leak
//        mTermuxActivity.getApplication().unregisterActivityLifecycleCallbacks(mActivityLifeCycleListener);
    }
}
