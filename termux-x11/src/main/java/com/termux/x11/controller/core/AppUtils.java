package com.termux.x11.controller.core;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.content.res.Resources;
import android.graphics.Rect;
import android.os.Build;
import android.os.Looper;
import android.text.Html;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;
import android.widget.LinearLayout;
import android.widget.PopupWindow;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.tabs.TabLayout;
import com.termux.x11.R;

import java.lang.ref.WeakReference;
import java.util.ArrayList;

public abstract class AppUtils {
    private static WeakReference<Toast> globalToastReference = null;

    public static void keepScreenOn(Activity activity) {
        activity.getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    public static String getArchName() {
        for (String arch : Build.SUPPORTED_ABIS) {
            switch (arch) {
                case "arm64-v8a": return "arm64";
                case "armeabi-v7a": return "armhf";
                case "x86_64": return "x86_64";
                case "x86": return "x86";
            }
        }
        return "armhf";
    }

    public static void restartActivity(AppCompatActivity activity) {
        Intent intent = activity.getIntent();
        activity.finish();
        activity.startActivity(intent);
        activity.overridePendingTransition(0, 0);
    }

    public static void restartApplication(Context context) {
        restartApplication(context, 0);
    }

    public static void restartApplication(Context context, int selectedMenuItemId) {
        Intent intent = context.getPackageManager().getLaunchIntentForPackage(context.getPackageName());
        Intent mainIntent = Intent.makeRestartActivityTask(intent.getComponent());
        if (selectedMenuItemId > 0) mainIntent.putExtra("selected_menu_item_id", selectedMenuItemId);
        context.startActivity(mainIntent);
        Runtime.getRuntime().exit(0);
    }

    public static void showKeyboard(AppCompatActivity activity) {
        final InputMethodManager imm = (InputMethodManager)activity.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.Q) {
            activity.getWindow().getDecorView().postDelayed(() -> imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0), 500L);
        }
        else imm.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
    }

    public static void hideSystemUI(final Activity activity) {
        Window window = activity.getWindow();
        final View decorView = window.getDecorView();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false);
            final WindowInsetsController insetsController = decorView.getWindowInsetsController();
            if (insetsController != null) {
                insetsController.hide(WindowInsets.Type.statusBars() | WindowInsets.Type.navigationBars());
                insetsController.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            }
        }
        else {
            final int flags = View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN
                | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY;

            decorView.setSystemUiVisibility(flags);
            decorView.setOnSystemUiVisibilityChangeListener((visibility) -> {
                if ((visibility & View.SYSTEM_UI_FLAG_FULLSCREEN) == 0) decorView.setSystemUiVisibility(flags);
            });
        }
    }

    public static boolean isUiThread() {
        return Looper.getMainLooper().getThread() == Thread.currentThread();
    }

    public static int getScreenWidth() {
        return Resources.getSystem().getDisplayMetrics().widthPixels;
    }

    public static int getScreenHeight() {
        return Resources.getSystem().getDisplayMetrics().heightPixels;
    }

    public static int getPreferredDialogWidth(Context context) {
        int orientation = context.getResources().getConfiguration().orientation;
        float scale = orientation == Configuration.ORIENTATION_PORTRAIT ? 0.8f : 0.5f;
        return (int)UnitUtils.dpToPx(UnitUtils.pxToDp(AppUtils.getScreenWidth()) * scale);
    }

    public static Toast showToast(Context context, int textResId) {
        return showToast(context, context.getString(textResId));
    }

    public static Toast showToast(final Context context, final String text) {
        if (!isUiThread()) {
            if (context instanceof Activity) {
                ((Activity)context).runOnUiThread(() -> showToast(context, text));
            }
            return null;
        }

        if (globalToastReference != null) {
            Toast toast = globalToastReference.get();
            if (toast != null) toast.cancel();
            globalToastReference = null;
        }

        View view = LayoutInflater.from(context).inflate(R.layout.custom_toast, null);
        ((TextView)view.findViewById(R.id.TextView)).setText(text);

        Toast toast = new Toast(context);
        toast.setGravity(Gravity.CENTER | Gravity.BOTTOM, 0, 50);
        toast.setDuration(text.length() >= 40 ? Toast.LENGTH_LONG : Toast.LENGTH_SHORT);
        toast.setView(view);
        toast.show();
        globalToastReference = new WeakReference<>(toast);
        return toast;
    }

    public static PopupWindow showPopupWindow(View anchor, View contentView) {
        return showPopupWindow(anchor, contentView, 0, 0);
    }

    public static PopupWindow showPopupWindow(View anchor, View contentView, int width, int height) {
        Context context = anchor.getContext();
        PopupWindow popupWindow = new PopupWindow(context);
        popupWindow.setElevation(5.0f);

        if (width == 0 && height == 0) {
            int widthMeasureSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
            int heightMeasureSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
            contentView.measure(widthMeasureSpec, heightMeasureSpec);
            popupWindow.setWidth(contentView.getMeasuredWidth());
            popupWindow.setHeight(contentView.getMeasuredHeight());
        }
        else {
            if (width > 0) {
                popupWindow.setWidth((int)UnitUtils.dpToPx(width));
            }
            else popupWindow.setWidth(LinearLayout.LayoutParams.WRAP_CONTENT);

            if (height > 0) {
                popupWindow.setHeight((int)UnitUtils.dpToPx(height));
            }
            else popupWindow.setHeight(LinearLayout.LayoutParams.WRAP_CONTENT);
        }

        popupWindow.setContentView(contentView);
        popupWindow.setFocusable(false);
        popupWindow.setOutsideTouchable(true);

        popupWindow.update();
        popupWindow.showAsDropDown(anchor);

        popupWindow.setFocusable(true);
        popupWindow.update();
        return popupWindow;
    }

    public static void showHelpBox(Context context, View anchor, int textResId) {
        showHelpBox(context, anchor, context.getString(textResId));
    }

    public static void showHelpBox(Context context, View anchor, String text) {
        int padding = (int)UnitUtils.dpToPx(8);
        TextView textView = new TextView(context);
        textView.setLayoutParams(new ViewGroup.LayoutParams((int)UnitUtils.dpToPx(284), ViewGroup.LayoutParams.WRAP_CONTENT));
        textView.setPadding(padding, padding, padding, padding);
        textView.setTextSize(TypedValue.COMPLEX_UNIT_DIP, 16);
        textView.setText(Html.fromHtml(text, Html.FROM_HTML_MODE_LEGACY));
        int widthMeasureSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
        int heightMeasureSpec = View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED);
        textView.measure(widthMeasureSpec, heightMeasureSpec);
        showPopupWindow(anchor, textView, 300, textView.getMeasuredHeight());
    }

    public static int getVersionCode(Context context) {
        try {
            PackageInfo pInfo = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            return pInfo.versionCode;
        }
        catch (PackageManager.NameNotFoundException e) {
            return 0;
        }
    }

    public static void observeSoftKeyboardVisibility(View rootView, Callback<Boolean> callback) {
        final boolean[] visible = {false};
        rootView.getViewTreeObserver().addOnGlobalLayoutListener(() -> {
            Rect rect = new Rect();
            rootView.getWindowVisibleDisplayFrame(rect);
            int screenHeight = rootView.getRootView().getHeight();
            int keypadHeight = screenHeight - rect.bottom;

            if (keypadHeight > screenHeight * 0.15f) {
                if (!visible[0]) {
                    visible[0] = true;
                    callback.call(true);
                }
            }
            else {
                if (visible[0]) {
                    visible[0] = false;
                    callback.call(false);
                }
            }
        });
    }

    public static boolean setSpinnerSelectionFromValue(Spinner spinner, String value) {
        spinner.setSelection(0, false);
        for (int i = 0; i < spinner.getCount(); i++) {
            if (spinner.getItemAtPosition(i).toString().equalsIgnoreCase(value)) {
                spinner.setSelection(i, false);
                return true;
            }
        }
        return false;
    }

    public static boolean setSpinnerSelectionFromIdentifier(Spinner spinner, String identifier) {
        spinner.setSelection(0, false);
        for (int i = 0; i < spinner.getCount(); i++) {
            if (StringUtils.parseIdentifier(spinner.getItemAtPosition(i)).equals(identifier)) {
                spinner.setSelection(i, false);
                return true;
            }
        }
        return false;
    }

    public static boolean setSpinnerSelectionFromNumber(Spinner spinner, String number) {
        spinner.setSelection(0, false);
        for (int i = 0; i < spinner.getCount(); i++) {
            if (StringUtils.parseNumber(spinner.getItemAtPosition(i)).equals(number)) {
                spinner.setSelection(i, false);
                return true;
            }
        }
        return false;
    }

    public static void setupTabLayout(final View view, int tabLayoutResId, final int... tabResIds) {
        final Callback<Integer> tabSelectedCallback = (position) -> {
            for (int i = 0; i < tabResIds.length; i++) {
                View tabView = view.findViewById(tabResIds[i]);
                tabView.setVisibility(position == i ? View.VISIBLE : View.GONE);
            }
        };

        TabLayout tabLayout = view.findViewById(tabLayoutResId);
        tabLayout.addOnTabSelectedListener(new TabLayout.OnTabSelectedListener() {
            @Override
            public void onTabSelected(TabLayout.Tab tab) {
                tabSelectedCallback.call(tab.getPosition());
            }

            @Override
            public void onTabUnselected(TabLayout.Tab tab) {}

            @Override
            public void onTabReselected(TabLayout.Tab tab) {
                tabSelectedCallback.call(tab.getPosition());
            }
        });
        tabLayout.getTabAt(0).select();
    }

    public static void findViewsWithClass(ViewGroup parent, Class viewClass, ArrayList<View> outViews) {
        for (int i = 0, childCount = parent.getChildCount(); i < childCount; i++) {
            View child = parent.getChildAt(i);
            Class _class = child.getClass();
            if (_class == viewClass || _class.getSuperclass() == viewClass) {
                outViews.add(child);
            }
            else if (child instanceof ViewGroup) {
                findViewsWithClass((ViewGroup)child, viewClass, outViews);
            }
        }
    }
}
