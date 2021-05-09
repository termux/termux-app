package com.termux.shared.view;

import android.app.Activity;
import android.content.Context;
import android.inputmethodservice.InputMethodService;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

import androidx.core.view.WindowInsetsCompat;

import com.termux.shared.logger.Logger;

public class KeyboardUtils {

    private static final String LOG_TAG = "KeyboardUtils";

    /**
     * Toggle the soft keyboard. The {@link InputMethodManager#SHOW_FORCED} is passed as
     * {@code showFlags} so that keyboard is forcefully shown if it needs to be enabled.
     *
     * This is also important for soft keyboard to be shown when a hardware keyboard is attached, and
     * user has disabled the {@code Show on-screen keyboard while hardware keyboard is attached} toggle
     * in Android "Language and Input" settings but the current soft keyboard app overrides the
     * default implementation of {@link InputMethodService#onEvaluateInputViewShown()} and returns
     * {@code true}.
     */
    public static void toggleSoftKeyboard(Context context) {
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        inputMethodManager.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
    }

    /**
     * Show the soft keyboard. The {@code 0} value is passed as {@code flags} so that keyboard is
     * forcefully shown.
     *
     * This is also important for soft keyboard to be shown on app startup when a hardware keyboard
     * is attached, and user has disabled the {@code Show on-screen keyboard while hardware keyboard
     * is attached} toggle in Android "Language and Input" settings but the current soft keyboard app
     * overrides the default implementation of {@link InputMethodService#onEvaluateInputViewShown()}
     * and returns {@code true}.
     * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:frameworks/base/core/java/android/inputmethodservice/InputMethodService.java;l=1751
     *
     * Also check {@link InputMethodService#onShowInputRequested(int, boolean)} which must return
     * {@code true}, which can be done by failing its {@code ((flags&InputMethod.SHOW_EXPLICIT) == 0)}
     * check by passing {@code 0} as {@code flags}.
     * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:frameworks/base/core/java/android/inputmethodservice/InputMethodService.java;l=2022
     */
    public static void showSoftKeyboard(Context context, View view) {
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        inputMethodManager.showSoftInput(view, 0);
    }

    public static void hideSoftKeyboard(Context context, View view) {
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        inputMethodManager.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

    public static void disableSoftKeyboard(Activity activity, View view) {
        hideSoftKeyboard(activity, view);
        setDisableSoftKeyboardFlags(activity);
    }

    public static void setDisableSoftKeyboardFlags(Activity activity) {
        activity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM, WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    }

    public static void clearDisableSoftKeyboardFlags(Activity activity) {
        activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    }

    public static boolean areDisableSoftKeyboardFlagsSet(Activity activity) {
        return (activity.getWindow().getAttributes().flags & WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM) != 0;
    }

    public static void setResizeTerminalViewForSoftKeyboardFlags(Activity activity) {
        // TODO: The flag is deprecated for API 30 and WindowInset API should be used
        // https://developer.android.com/reference/android/view/WindowManager.LayoutParams#SOFT_INPUT_ADJUST_RESIZE
        // https://medium.com/androiddevelopers/animating-your-keyboard-fb776a8fb66d
        // https://stackoverflow.com/a/65194077/14686958
        activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
    }

    /** Check if keyboard visible. Does not work on android 7 but does on android 11 avd. */
    public static boolean isSoftKeyboardVisible(Activity activity) {
        WindowInsets insets = activity.getWindow().getDecorView().getRootWindowInsets();

        if (insets != null) {
            WindowInsetsCompat insetsCompat = WindowInsetsCompat.toWindowInsetsCompat(insets);
            if (insetsCompat != null && insetsCompat.isVisible(WindowInsetsCompat.Type.ime())) {
                Logger.logVerbose(LOG_TAG, "Keyboard visible");
                return true;
            }
        }

        Logger.logVerbose(LOG_TAG, "Keyboard not visible");
        return false;
    }

}
