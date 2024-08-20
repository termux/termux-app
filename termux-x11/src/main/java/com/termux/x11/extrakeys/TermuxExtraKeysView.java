package com.termux.x11.extrakeys;

import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.provider.Settings;
import android.util.AttributeSet;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.ScheduledExecutorService;

import java.util.Map;
import java.util.stream.Collectors;

import android.view.HapticFeedbackConstants;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewConfiguration;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.PopupWindow;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * A {@link View} showing extra keys (such as Escape, Ctrl, Alt) not normally available on an Android soft
 * keyboards.
 * <p>
 * To use it, add following to a layout file and import it in your activity layout file or inflate
 * it with a {@link androidx.viewpager.widget.ViewPager}.:
 * {@code
 * <?xml version="1.0" encoding="utf-8"?>
 * <com.termux.x11.extrakeys.ExtraKeysView xmlns:android="http://schemas.android.com/apk/res/android"
 *     android:id="@+id/extra_keys"
 *     style="?android:attr/buttonBarStyle"
 *     android:layout_width="match_parent"
 *     android:layout_height="match_parent"
 *     android:layout_alignParentBottom="true"
 *     android:orientation="horizontal" />
 * }
 *
 * Then in your activity, get its reference by a call to {@link android.app.Activity#findViewById(int)}
 * or {@link LayoutInflater#inflate(int, ViewGroup)} if using {@link androidx.viewpager.widget.ViewPager}.
 * Then call {@link #setExtraKeysViewClient(IExtraKeysView)} and pass it the implementation of
 * {@link IExtraKeysView} so that you can receive callbacks. You can also override other values set
 * in {@link TermuxExtraKeysView#TermuxExtraKeysView(Context, AttributeSet)} by calling the respective functions.
 * If you extend {@link TermuxExtraKeysView}, you can also set them in the constructor, but do call super().
 * <p>
 * After this you will have to make a call to {@link TermuxExtraKeysView#reload(TermuxX11ExtraKeysInfo, float) and pass
 * it the {@link TermuxX11ExtraKeysInfo } to load and x11 the extra keys. Read its class javadocs for more
 * info on how to create it.
 * <p>
 * Termux app defines the view in res/layout/view_terminal_toolbar_extra_keys and
 * inflates it in TerminalToolbarViewPager.instantiateItem() and sets the {@link TermuxExtraKeysView } client
 * and calls {@link TermuxExtraKeysView#reload(TermuxX11ExtraKeysInfo).
 * The {@link TermuxX11ExtraKeysInfo } is created by TermuxAppSharedProperties.setExtraKeys().
 * Then its got and the view height is adjusted in TermuxActivity.setTerminalToolbarHeight().
 * The client used is TermuxTerminalExtraKeys, which extends
 * {@link com.termux.x11.utils.TermuxX11ExtraKeys } to handle Termux app specific logic and
 * leave the rest to the super class.
 */
public final class TermuxExtraKeysView extends GridLayout {
    /** The client for the {@link TermuxExtraKeysView}. */
    public interface IExtraKeysView {
        /**
         * This is called by {@link TermuxExtraKeysView} when a button is clicked. This is also called
         * for {@link #mRepetitiveKeys} and {@link TermuxX11ExtraKeyButton} that have a popup set.
         * However, this is not called for {@link #mSpecialButtons}, whose state can instead be read
         * via a call to {@link #readSpecialButton(TermuxX11SpecialButton, boolean)}.
         *
         * @param view The view that was clicked.
         * @param buttonInfo The {@link TermuxX11ExtraKeyButton} for the button that was clicked.
         *                   The button may be a {@link TermuxX11ExtraKeyButton#KEY_MACRO} set which can be
         *                   checked with a call to {@link TermuxX11ExtraKeyButton#macro}.
         * @param button The {@link Button} that was clicked.
         */
        void onExtraKeyButtonClick(View view, TermuxX11ExtraKeyButton buttonInfo, Button button);

        /**
         * This is called by {@link TermuxExtraKeysView} when a button is clicked so that the client
         * can perform any hepatic feedback. This is only called in the {@link OnClickListener}
         * and not for every repeat. Its also called for {@link #mSpecialButtons}.
         *
         * @param view The view that was clicked.
         * @param buttonInfo The {@link TermuxX11ExtraKeyButton} for the button that was clicked.
         * @param button The {@link Button} that was clicked.
         * @return Return {@code true} if the client handled the feedback, otherwise {@code false}
         * so that {@link TermuxExtraKeysView#performExtraKeyButtonHapticFeedback(View, TermuxX11ExtraKeyButton, Button)}
         * can handle it depending on system settings.
         */
        boolean performExtraKeyButtonHapticFeedback(View view, TermuxX11ExtraKeyButton buttonInfo, Button button);
    }

    /** Defines the default value for {@link #mButtonTextColor} */
    public static final int DEFAULT_BUTTON_TEXT_COLOR = 0xFFFFFFFF;
    /** Defines the default value for {@link #mButtonActiveTextColor} */
    public static final int DEFAULT_BUTTON_ACTIVE_TEXT_COLOR = 0xFF80DEEA;
    /** Defines the default value for {@link #mButtonBackgroundColor} */
    public static final int DEFAULT_BUTTON_BACKGROUND_COLOR = 0x00000000;
    /** Defines the default value for {@link #mButtonActiveBackgroundColor} */
    public static final int DEFAULT_BUTTON_ACTIVE_BACKGROUND_COLOR = 0xFF7F7F7F;

    /** Defines the minimum allowed duration in milliseconds for {@link #mLongPressTimeout}. */
    public static final int MIN_LONG_PRESS_DURATION = 200;
    /** Defines the maximum allowed duration in milliseconds for {@link #mLongPressTimeout}. */
    public static final int MAX_LONG_PRESS_DURATION = 3000;
    /** Defines the fallback duration in milliseconds for {@link #mLongPressTimeout}. */
    public static final int FALLBACK_LONG_PRESS_DURATION = 400;

    /** Defines the minimum allowed duration in milliseconds for {@link #mLongPressRepeatDelay}. */
    public static final int MIN_LONG_PRESS__REPEAT_DELAY = 5;
    /** Defines the maximum allowed duration in milliseconds for {@link #mLongPressRepeatDelay}. */
    public static final int MAX_LONG_PRESS__REPEAT_DELAY = 2000;
    /** Defines the default duration in milliseconds for {@link #mLongPressRepeatDelay}. */
    public static final int DEFAULT_LONG_PRESS_REPEAT_DELAY = 80;



    /** The implementation of the {@link IExtraKeysView} that acts as a client for the {@link TermuxExtraKeysView}. */
    private IExtraKeysView mExtraKeysViewClient;

    /** The map for the {@link TermuxX11SpecialButton} and their {@link TermuxX11SpecialButtonState}. Defaults to
     * the one returned by {@link #getDefaultSpecialButtons(TermuxExtraKeysView)}. */
    private Map<TermuxX11SpecialButton, TermuxX11SpecialButtonState> mSpecialButtons;

    /** The keys for the {@link TermuxX11SpecialButton} added to {@link #mSpecialButtons}. This is automatically
     * set when the call to {@link #setSpecialButtons(Map)} is made. */
    private Set<String> mSpecialButtonsKeys;


    /**
     * The list of keys for which auto repeat of key should be triggered if its extra keys button
     * is long pressed. This is done by calling {@link IExtraKeysView#onExtraKeyButtonClick(View, TermuxX11ExtraKeyButton, Button)}
     * every {@link #mLongPressRepeatDelay} seconds after {@link #mLongPressTimeout} has passed.
     * The default keys are defined by {@link TermuxX11ExtraKeysConstants#PRIMARY_REPETITIVE_KEYS}.
     */
    private List<String> mRepetitiveKeys;


    /** The text color for the extra keys button. Defaults to {@link #DEFAULT_BUTTON_TEXT_COLOR}. */
    private int mButtonTextColor;
    /** The text color for the extra keys button when its active.
     * Defaults to {@link #DEFAULT_BUTTON_ACTIVE_TEXT_COLOR}. */
    private int mButtonActiveTextColor;
    /** The background color for the extra keys button. Defaults to {@link #DEFAULT_BUTTON_BACKGROUND_COLOR}. */
    private int mButtonBackgroundColor;
    /** The background color for the extra keys button when its active. Defaults to
     * {@link #DEFAULT_BUTTON_ACTIVE_BACKGROUND_COLOR}. */
    private int mButtonActiveBackgroundColor;

    /** Defines whether text for the extra keys button should be all capitalized automatically. */
    private boolean mButtonTextAllCaps = true;


    /**
     * Defines the duration in milliseconds before a press turns into a long press. The default
     * duration used is the one returned by a call to {@link ViewConfiguration#getLongPressTimeout()}
     * which will return the system defined duration which can be changed in accessibility settings.
     * The duration must be in between {@link #MIN_LONG_PRESS_DURATION} and {@link #MAX_LONG_PRESS_DURATION},
     * otherwise {@link #FALLBACK_LONG_PRESS_DURATION} is used.
     */
    private int mLongPressTimeout;

    /**
     * Defines the duration in milliseconds for the delay between trigger of each repeat of
     * {@link #mRepetitiveKeys}. The default value is defined by {@link #DEFAULT_LONG_PRESS_REPEAT_DELAY}.
     * The duration must be in between {@link #MIN_LONG_PRESS__REPEAT_DELAY} and
     * {@link #MAX_LONG_PRESS__REPEAT_DELAY}, otherwise {@link #DEFAULT_LONG_PRESS_REPEAT_DELAY} is used.
     */
    private int mLongPressRepeatDelay;


    /** The popup window shown if {@link TermuxX11ExtraKeyButton#popup} returns a {@code non-null} value
     * and a swipe up action is done on an extra key. */
    private PopupWindow mPopupWindow;

    private ScheduledExecutorService mScheduledExecutor;
    private Handler mHandler;
    private SpecialButtonsLongHoldRunnable mSpecialButtonsLongHoldRunnable;
    private int mLongPressCount;


    public TermuxExtraKeysView(Context context, AttributeSet attrs) {
        super(context, attrs);

        setRepetitiveKeys(TermuxX11ExtraKeysConstants.PRIMARY_REPETITIVE_KEYS);
        setSpecialButtons(getDefaultSpecialButtons(this));

        setButtonColors(DEFAULT_BUTTON_TEXT_COLOR, DEFAULT_BUTTON_ACTIVE_TEXT_COLOR, DEFAULT_BUTTON_BACKGROUND_COLOR, DEFAULT_BUTTON_ACTIVE_BACKGROUND_COLOR);

        setLongPressTimeout(ViewConfiguration.getLongPressTimeout());
        setLongPressRepeatDelay(DEFAULT_LONG_PRESS_REPEAT_DELAY);
    }


    /** Set {@link #mExtraKeysViewClient}. */
    public void setExtraKeysViewClient(IExtraKeysView extraKeysViewClient) {
        mExtraKeysViewClient = extraKeysViewClient;
    }

    /** Set {@link #mRepetitiveKeys}. Must not be {@code null}. */
    public void setRepetitiveKeys(@NonNull List<String> repetitiveKeys) {
        mRepetitiveKeys = repetitiveKeys;
    }

    /** Set {@link #mSpecialButtonsKeys}. Must not be {@code null}. */
    public void setSpecialButtons(@NonNull Map<TermuxX11SpecialButton, TermuxX11SpecialButtonState> specialButtons) {
        mSpecialButtons = specialButtons;
        mSpecialButtonsKeys = this.mSpecialButtons.keySet().stream().map(TermuxX11SpecialButton::getKey).collect(Collectors.toSet());
    }


    /**
     * Set the {@link TermuxExtraKeysView} button colors.
     *
     * @param buttonTextColor The value for {@link #mButtonTextColor}.
     * @param buttonActiveTextColor The value for {@link #mButtonActiveTextColor}.
     * @param buttonBackgroundColor The value for {@link #mButtonBackgroundColor}.
     * @param buttonActiveBackgroundColor The value for {@link #mButtonActiveBackgroundColor}.
     */
    public void setButtonColors(int buttonTextColor, int buttonActiveTextColor, int buttonBackgroundColor, int buttonActiveBackgroundColor) {
        mButtonTextColor = buttonTextColor;
        mButtonActiveTextColor = buttonActiveTextColor;
        mButtonBackgroundColor = buttonBackgroundColor;
        mButtonActiveBackgroundColor = buttonActiveBackgroundColor;
    }


    /** Get {@link #mButtonTextColor}. */
    public int getButtonTextColor() {
        return mButtonTextColor;
    }

    /** Get {@link #mButtonActiveTextColor}. */
    public int getButtonActiveTextColor() {
        return mButtonActiveTextColor;
    }

    /** Set {@link #mLongPressTimeout}. */
    public void setLongPressTimeout(int longPressDuration) {
        if (longPressDuration >= MIN_LONG_PRESS_DURATION && longPressDuration <= MAX_LONG_PRESS_DURATION) {
            mLongPressTimeout = longPressDuration;
        } else {
            mLongPressTimeout = FALLBACK_LONG_PRESS_DURATION;
        }
    }

    /** Set {@link #mLongPressRepeatDelay}. */
    public void setLongPressRepeatDelay(int longPressRepeatDelay) {
        if (mLongPressRepeatDelay >= MIN_LONG_PRESS__REPEAT_DELAY && mLongPressRepeatDelay <= MAX_LONG_PRESS__REPEAT_DELAY) {
            mLongPressRepeatDelay = longPressRepeatDelay;
        } else {
            mLongPressRepeatDelay = DEFAULT_LONG_PRESS_REPEAT_DELAY;
        }
    }

    /** Get the default map that can be used for {@link #mSpecialButtons}. */
    @NonNull
    public Map<TermuxX11SpecialButton, TermuxX11SpecialButtonState> getDefaultSpecialButtons(TermuxExtraKeysView termuxExtraKeysView) {
        return new HashMap<>() {{
            put(TermuxX11SpecialButton.CTRL, new TermuxX11SpecialButtonState(termuxExtraKeysView));
            put(TermuxX11SpecialButton.ALT, new TermuxX11SpecialButtonState(termuxExtraKeysView));
            put(TermuxX11SpecialButton.SHIFT, new TermuxX11SpecialButtonState(termuxExtraKeysView));
            put(TermuxX11SpecialButton.META, new TermuxX11SpecialButtonState(termuxExtraKeysView));
            put(TermuxX11SpecialButton.FN, new TermuxX11SpecialButtonState(termuxExtraKeysView));
        }};
    }

    /**
     * Reload this instance of {@link TermuxExtraKeysView} with the info passed in {@code extraKeysInfo}.
     *
     * @param termuxX11ExtraKeysInfo The {@link TermuxX11ExtraKeysInfo} that defines the necessary info for the extra keys.
     * @param heightPx The height in pixels of the parent surrounding the {@link TermuxExtraKeysView}. It must
     *                 be a single child.
     */
    @SuppressLint("ClickableViewAccessibility")
    public void reload(TermuxX11ExtraKeysInfo termuxX11ExtraKeysInfo, float heightPx) {
        if (termuxX11ExtraKeysInfo == null)
            return;

        for(TermuxX11SpecialButtonState state : mSpecialButtons.values())
            state.buttons = new ArrayList<>();

        removeAllViews();

        TermuxX11ExtraKeyButton[][] buttons = termuxX11ExtraKeysInfo.getMatrix();

        setRowCount(buttons.length);
        setColumnCount(maximumLength(buttons));

        for (int row = 0; row < buttons.length; row++) {
            for (int col = 0; col < buttons[row].length; col++) {
                final TermuxX11ExtraKeyButton buttonInfo = buttons[row][col];

                Button button;
                if (isSpecialButton(buttonInfo)) {
                    button = createSpecialButton(buttonInfo.key, true);
                    if (button == null) return;
                } else {
                    button = new Button(getContext(), null, android.R.attr.buttonBarButtonStyle);
                }

                button.setBackground(new ColorDrawable(Color.BLACK) {
                    public boolean isStateful() {
                        return true;
                    }
                    public boolean hasFocusStateSpecified() {
                        return true;
                    }
                });
                button.setText(buttonInfo.display);
                button.setTextColor(mButtonTextColor);
                button.setAllCaps(mButtonTextAllCaps);
                button.setPadding(0, 0, 0, 0);

                button.setOnClickListener(view -> {
                    performExtraKeyButtonHapticFeedback(view, buttonInfo, button);
                    onAnyExtraKeyButtonClick(view, buttonInfo, button);
                });

                button.setOnTouchListener((view, event) -> {
                    switch (event.getAction()) {
                        case MotionEvent.ACTION_DOWN:
                            view.setBackgroundColor(mButtonActiveBackgroundColor);
                            // Start long press scheduled executors which will be stopped in next MotionEvent
                            startScheduledExecutors(view, buttonInfo, button);
                            return true;

                        case MotionEvent.ACTION_MOVE:
                            if (buttonInfo.popup != null) {
                                // Show popup on swipe up
                                if (mPopupWindow == null && event.getY() < 0) {
                                    stopScheduledExecutors();
                                    view.setBackgroundColor(mButtonBackgroundColor);
                                    showPopup(view, buttonInfo.popup);
                                }
                                if (mPopupWindow != null && event.getY() > 0) {
                                    view.setBackgroundColor(mButtonActiveBackgroundColor);
                                    dismissPopup();
                                }
                            }
                            return true;

                        case MotionEvent.ACTION_CANCEL:
                            view.setBackgroundColor(mButtonBackgroundColor);
                            stopScheduledExecutors();
                            return true;

                        case MotionEvent.ACTION_UP:
                            view.setBackgroundColor(mButtonBackgroundColor);
                            stopScheduledExecutors();
                            // If ACTION_UP up was not from a repetitive key or was with a key with a popup button
                            if (mLongPressCount == 0 || mPopupWindow != null) {
                                // Trigger popup button click if swipe up complete
                                if (mPopupWindow != null) {
                                    dismissPopup();
                                    if (buttonInfo.popup != null) {
                                        onAnyExtraKeyButtonClick(view, buttonInfo.popup, button);
                                    }
                                } else {
                                    view.performClick();
                                }
                            }
                            return true;

                        default:
                            return true;
                    }
                });

                LayoutParams param = new LayoutParams();
                param.width = 0;
                param.height = 0;
                param.setMargins(0, 0, 0, 0);
                param.columnSpec = GridLayout.spec(col, GridLayout.FILL, 1.f);
                param.rowSpec = GridLayout.spec(row, GridLayout.FILL, 1.f);
                button.setLayoutParams(param);

                addView(button);
            }
        }
    }

    public void onExtraKeyButtonClick(View view, TermuxX11ExtraKeyButton buttonInfo, Button button) {
        if (mExtraKeysViewClient != null)
            mExtraKeysViewClient.onExtraKeyButtonClick(view, buttonInfo, button);
    }

    public void performExtraKeyButtonHapticFeedback(View view, TermuxX11ExtraKeyButton buttonInfo, Button button) {
        if (mExtraKeysViewClient != null) {
            // If client handled the feedback, then just return
            if (mExtraKeysViewClient.performExtraKeyButtonHapticFeedback(view, buttonInfo, button))
                return;
        }

        if (Settings.System.getInt(getContext().getContentResolver(),
            Settings.System.HAPTIC_FEEDBACK_ENABLED, 0) != 0) {

            if (Build.VERSION.SDK_INT >= 28) {
                button.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
            } else {
                // Perform haptic feedback only if no total silence mode enabled.
                if (Settings.Global.getInt(getContext().getContentResolver(), "zen_mode", 0) != 2) {
                    button.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
                }
            }
        }
    }

    public void onAnyExtraKeyButtonClick(View view, @NonNull TermuxX11ExtraKeyButton buttonInfo, Button button) {
        if (isSpecialButton(buttonInfo)) {
            if (mLongPressCount > 0) return;
            TermuxX11SpecialButtonState state = mSpecialButtons.get(TermuxX11SpecialButton.valueOf(buttonInfo.key));
            if (state == null) return;

            // Toggle active state and disable lock state if new state is not active
            state.setIsActive(!state.isActive);
            if (!state.isActive)
                state.setIsLocked(false);
        } else {
            onExtraKeyButtonClick(view, buttonInfo, button);
        }
    }

    public void startScheduledExecutors(View view, TermuxX11ExtraKeyButton buttonInfo, Button button) {
        stopScheduledExecutors();
        mLongPressCount = 0;
        if (mRepetitiveKeys.contains(buttonInfo.key)) {
            // Auto repeat key if long pressed until ACTION_UP stops it by calling stopScheduledExecutors.
            // Currently, only one (last) repeat key can run at a time. Old ones are stopped.
            mScheduledExecutor = Executors.newSingleThreadScheduledExecutor();
            mScheduledExecutor.scheduleWithFixedDelay(() -> {
                mLongPressCount++;
                onExtraKeyButtonClick(view, buttonInfo, button);
            }, mLongPressTimeout, mLongPressRepeatDelay, TimeUnit.MILLISECONDS);
        } else if (isSpecialButton(buttonInfo)) {
            // Lock the key if long pressed by running mSpecialButtonsLongHoldRunnable after
            // waiting for mLongPressTimeout milliseconds. If user does not long press, then the
            // ACTION_UP triggered will cancel the runnable by calling stopScheduledExecutors before
            // it has a chance to run.
            TermuxX11SpecialButtonState state = mSpecialButtons.get(TermuxX11SpecialButton.valueOf(buttonInfo.key));
            if (state == null) return;
            if (mHandler == null)
                mHandler = new Handler(Looper.getMainLooper());
            mSpecialButtonsLongHoldRunnable = new SpecialButtonsLongHoldRunnable(state);
            mHandler.postDelayed(mSpecialButtonsLongHoldRunnable, mLongPressTimeout);
        }
    }

    public void stopScheduledExecutors() {
        if (mScheduledExecutor != null) {
            mScheduledExecutor.shutdownNow();
            mScheduledExecutor = null;
        }

        if (mSpecialButtonsLongHoldRunnable != null && mHandler != null) {
            mHandler.removeCallbacks(mSpecialButtonsLongHoldRunnable);
            mSpecialButtonsLongHoldRunnable = null;
        }
    }

    public class SpecialButtonsLongHoldRunnable implements Runnable {
        public final TermuxX11SpecialButtonState mState;

        public SpecialButtonsLongHoldRunnable(TermuxX11SpecialButtonState state) {
            mState = state;
        }

        public void run() {
            // Toggle active and lock state
            mState.setIsLocked(!mState.isActive);
            mState.setIsActive(!mState.isActive);
            mLongPressCount++;
        }
    }

    void showPopup(View view, TermuxX11ExtraKeyButton extraButton) {
        int width = view.getMeasuredWidth();
        int height = view.getMeasuredHeight();
        Button button;
        if (isSpecialButton(extraButton)) {
            button = createSpecialButton(extraButton.key, false);
            if (button == null) return;
        } else {
            button = new Button(getContext(), null, android.R.attr.buttonBarButtonStyle);
            button.setTextColor(mButtonTextColor);
        }
        button.setText(extraButton.display);
        button.setAllCaps(mButtonTextAllCaps);
        button.setPadding(0, 0, 0, 0);
        button.setMinHeight(0);
        button.setMinWidth(0);
        button.setMinimumWidth(0);
        button.setMinimumHeight(0);
        button.setWidth(width);
        button.setHeight(height);
        button.setBackgroundColor(mButtonActiveBackgroundColor);
        mPopupWindow = new PopupWindow(this);
        mPopupWindow.setWidth(LayoutParams.WRAP_CONTENT);
        mPopupWindow.setHeight(LayoutParams.WRAP_CONTENT);
        mPopupWindow.setContentView(button);
        mPopupWindow.setOutsideTouchable(true);
        mPopupWindow.setFocusable(false);
        mPopupWindow.showAsDropDown(view, 0, -2 * height);
    }

    public void dismissPopup() {
        mPopupWindow.setContentView(null);
        mPopupWindow.dismiss();
        mPopupWindow = null;
    }

    /** Check whether a {@link TermuxX11ExtraKeyButton} is a {@link TermuxX11SpecialButton}. */
    public boolean isSpecialButton(TermuxX11ExtraKeyButton button) {
        return mSpecialButtonsKeys.contains(button.key);
    }

    /**
     * Read whether {@link TermuxX11SpecialButton} registered in {@link #mSpecialButtons} is active or not.
     *
     * @param termuxX11SpecialButton The {@link TermuxX11SpecialButton} to read.
     * @param autoSetInActive Set to {@code true} if {@link TermuxX11SpecialButtonState#isActive} should be
     *                        set {@code false} if button is not locked.
     * @return Returns {@code null} if button does not exist in {@link #mSpecialButtons}. If button
     *         exists, then returns {@code true} if the button is created in {@link TermuxExtraKeysView}
     *         and is active, otherwise {@code false}.
     */
    @Nullable
    public Boolean readSpecialButton(TermuxX11SpecialButton termuxX11SpecialButton, boolean autoSetInActive) {
        TermuxX11SpecialButtonState state = mSpecialButtons.get(termuxX11SpecialButton);
        if (state == null) return null;

        if (!state.isCreated || !state.isActive)
            return false;

        // Disable active state only if not locked
        if (autoSetInActive && !state.isLocked)
            state.setIsActive(false);

        return true;
    }

    public Button createSpecialButton(String buttonKey, boolean needUpdate) {
        TermuxX11SpecialButtonState state = mSpecialButtons.get(TermuxX11SpecialButton.valueOf(buttonKey));
        if (state == null) return null;
        state.setIsCreated(true);
        Button button = new Button(getContext(), null, android.R.attr.buttonBarButtonStyle);
        button.setTextColor(state.isActive ? mButtonActiveTextColor : mButtonTextColor);
        if (needUpdate) {
            state.buttons.add(button);
        }
        return button;
    }

    /**
     * General util function to compute the longest column length in a matrix.
     */
    public static int maximumLength(Object[][] matrix) {
        int m = 0;
        for (Object[] row : matrix)
            m = Math.max(m, row.length);
        return m;
    }
}
