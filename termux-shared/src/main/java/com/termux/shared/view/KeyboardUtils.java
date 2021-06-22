package com.termux.shared.view;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.inputmethodservice.InputMethodService;
import android.view.View;
import android.view.WindowInsets;
import android.view.WindowManager;
import android.view.inputmethod.InputMethodManager;

import androidx.annotation.NonNull;
import androidx.core.view.WindowInsetsCompat;

import com.termux.shared.logger.Logger;

public class KeyboardUtils {

    private static final String LOG_TAG = "KeyboardUtils";

    public static void setSoftKeyboardVisibility(@NonNull final Runnable showSoftKeyboardRunnable, final Activity activity, final View view, final boolean visible) {
        if (visible) {
            // A Runnable with a delay is used, otherwise soft keyboard may not automatically open
            // on some devices, but still may fail
            view.postDelayed(showSoftKeyboardRunnable, 500);
        } else {
            view.removeCallbacks(showSoftKeyboardRunnable);
            hideSoftKeyboard(activity, view);
        }
    }

    /**
     * Toggle the soft keyboard. The {@link InputMethodManager#SHOW_FORCED} is passed as
     * {@code showFlags} so that keyboard is forcefully shown if it needs to be enabled.
     *
     * This is also important for soft keyboard to be shown when a hardware keyboard is connected, and
     * user has disabled the {@code Show on-screen keyboard while hardware keyboard is connected} toggle
     * in Android "Language and Input" settings but the current soft keyboard app overrides the
     * default implementation of {@link InputMethodService#onEvaluateInputViewShown()} and returns
     * {@code true}.
     */
    public static void toggleSoftKeyboard(final Context context) {
        if (context == null) return;
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (inputMethodManager != null)
            inputMethodManager.toggleSoftInput(InputMethodManager.SHOW_FORCED, 0);
    }

    /**
     * Show the soft keyboard. The {@code 0} value is passed as {@code flags} so that keyboard is
     * forcefully shown.
     *
     * This is also important for soft keyboard to be shown on app startup when a hardware keyboard
     * is connected, and user has disabled the {@code Show on-screen keyboard while hardware keyboard
     * is connected} toggle in Android "Language and Input" settings but the current soft keyboard app
     * overrides the default implementation of {@link InputMethodService#onEvaluateInputViewShown()}
     * and returns {@code true}.
     * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:frameworks/base/core/java/android/inputmethodservice/InputMethodService.java;l=1751
     *
     * Also check {@link InputMethodService#onShowInputRequested(int, boolean)} which must return
     * {@code true}, which can be done by failing its {@code ((flags&InputMethod.SHOW_EXPLICIT) == 0)}
     * check by passing {@code 0} as {@code flags}.
     * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:frameworks/base/core/java/android/inputmethodservice/InputMethodService.java;l=2022
     */
    public static void showSoftKeyboard(final Context context, final View view) {
        if (context == null || view == null) return;
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (inputMethodManager != null)
            inputMethodManager.showSoftInput(view, 0);
    }

    public static void hideSoftKeyboard(final Context context, final View view) {
        if (context == null || view == null) return;
        InputMethodManager inputMethodManager = (InputMethodManager) context.getSystemService(Context.INPUT_METHOD_SERVICE);
        if (inputMethodManager != null)
            inputMethodManager.hideSoftInputFromWindow(view.getWindowToken(), 0);
    }

    public static void disableSoftKeyboard(final Activity activity, final View view) {
        if (activity == null || view == null) return;
        hideSoftKeyboard(activity, view);
        setDisableSoftKeyboardFlags(activity);
    }

    public static void setDisableSoftKeyboardFlags(final Activity activity) {
        if (activity != null && activity.getWindow() != null)
            activity.getWindow().setFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM, WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    }

    public static void clearDisableSoftKeyboardFlags(final Activity activity) {
        if (activity != null && activity.getWindow() != null)
            activity.getWindow().clearFlags(WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM);
    }

    public static boolean areDisableSoftKeyboardFlagsSet(final Activity activity) {
        if (activity == null ||  activity.getWindow() == null) return false;
        return (activity.getWindow().getAttributes().flags & WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM) != 0;
    }

    public static void setSoftKeyboardAlwaysHiddenFlags(final Activity activity) {
        if (activity != null && activity.getWindow() != null)
            activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_HIDDEN);
    }

    public static void setSoftInputModeAdjustResize(final Activity activity) {
        // TODO: The flag is deprecated for API 30 and WindowInset API should be used
        // https://developer.android.com/reference/android/view/WindowManager.LayoutParams#SOFT_INPUT_ADJUST_RESIZE
        // https://medium.com/androiddevelopers/animating-your-keyboard-fb776a8fb66d
        // https://stackoverflow.com/a/65194077/14686958
        if (activity != null && activity.getWindow() != null)
            activity.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE);
    }

    /**
     * Check if soft keyboard is visible.
     * Does not work on android 7 but does on android 11 avd.
     *
     * @param activity The Activity of the root view for which the visibility should be checked.
     * @return Returns {@code true} if soft keyboard is visible, otherwise {@code false}.
     */
    public static boolean isSoftKeyboardVisible(final Activity activity) {
        if (activity != null && activity.getWindow() != null) {
            WindowInsets insets = activity.getWindow().getDecorView().getRootWindowInsets();
            if (insets != null) {
                WindowInsetsCompat insetsCompat = WindowInsetsCompat.toWindowInsetsCompat(insets);
                if (insetsCompat.isVisible(WindowInsetsCompat.Type.ime())) {
                    Logger.logVerbose(LOG_TAG, "Soft keyboard visible");
                    return true;
                }
            }
        }

        Logger.logVerbose(LOG_TAG, "Soft keyboard not visible");
        return false;
    }

    /**
     * Check if hardware keyboard is connected.
     * Based on default implementation of {@link InputMethodService#onEvaluateInputViewShown()}.
     *
     * https://developer.android.com/guide/topics/resources/providing-resources#ImeQualifier
     *
     * @param context The Context for operations.
     * @return Returns {@code true} if device has hardware keys for text input or an external hardware
     * keyboard is connected, otherwise {@code false}.
     */
    public static boolean isHardKeyboardConnected(final Context context) {
        if (context == null) return false;

        Configuration config = context.getResources().getConfiguration();
        return config.keyboard != Configuration.KEYBOARD_NOKEYS
            || config.hardKeyboardHidden == Configuration.HARDKEYBOARDHIDDEN_NO;
    }

    /**
     * Check if soft keyboard should be disabled based on user configuration.
     *
     * @param context The Context for operations.
     * @return Returns {@code true} if device has soft keyboard should be disabled, otherwise {@code false}.
     */
    public static boolean shouldSoftKeyboardBeDisabled(final Context context, final boolean isSoftKeyboardEnabled, final boolean isSoftKeyboardEnabledOnlyIfNoHardware) {
        // If soft keyboard is disabled by user regardless of hardware keyboard
        if (!isSoftKeyboardEnabled) {
            return true;
        } else {
            /*
             * Currently, for this case, soft keyboard will be disabled on Termux app startup and
             * when switching back from another app. Soft keyboard can be temporarily enabled in
             * show/hide soft keyboard toggle behaviour with keyboard toggle buttons and will continue
             * to work when tapping on terminal view for opening and back button for closing, until
             * Termux app is switched to another app. After returning back, keyboard will be disabled
             * until toggle is pressed again.
             * This may also be helpful for the Lineage OS bug where if "Show soft keyboard" toggle
             * in "Language and Input" is disabled and Termux is started without a hardware keyboard
             * in landscape mode, and then the keyboard is connected and phone is rotated to portrait
             * mode and then keyboard is toggled with Termux keyboard toggle buttons, then a blank
             * space is shown in-place of the soft keyboard. Its likely related to
             * WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE which pushes up the view when
             * keyboard is opened instead of the keyboard opening on top of the view (hiding stuff).
             * If the "Show soft keyboard" toggle was disabled, then this resizing shouldn't happen.
             * But it seems resizing does happen, but keyboard is never opened since its not supposed to.
             * https://github.com/termux/termux-app/issues/1995#issuecomment-837080079
             */
            // If soft keyboard is disabled by user only if hardware keyboard is connected
            if(isSoftKeyboardEnabledOnlyIfNoHardware) {
                boolean isHardKeyboardConnected = KeyboardUtils.isHardKeyboardConnected(context);
                Logger.logVerbose(LOG_TAG, "Hardware keyboard connected=" + isHardKeyboardConnected);
                return isHardKeyboardConnected;
            } else {
                return false;
            }
        }
    }

}
