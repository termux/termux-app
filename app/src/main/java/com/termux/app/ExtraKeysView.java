package com.termux.app;

import android.annotation.SuppressLint;
import android.content.Context;
import android.os.Build;
import android.provider.Settings;
import android.util.AttributeSet;

import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.ScheduledExecutorService;

import java.util.Map;
import java.util.HashMap;
import java.util.Arrays;

import android.view.Gravity;
import android.view.HapticFeedbackConstants;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.PopupWindow;
import android.widget.ToggleButton;

import com.termux.R;
import com.termux.view.TerminalView;

import androidx.drawerlayout.widget.DrawerLayout;

/**
 * A view showing extra keys (such as Escape, Ctrl, Alt) not normally available on an Android soft
 * keyboard.
 */
public final class ExtraKeysView extends GridLayout {

    private static final int TEXT_COLOR = 0xFFFFFFFF;
    private static final int BUTTON_COLOR = 0x00000000;
    private static final int INTERESTING_COLOR = 0xFF80DEEA;
    private static final int BUTTON_PRESSED_COLOR = 0xFF7F7F7F;

    public ExtraKeysView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    static final Map<String, Integer> keyCodesForString = new HashMap<String, Integer>() {{
        put("SPACE", KeyEvent.KEYCODE_SPACE);
        put("ESC", KeyEvent.KEYCODE_ESCAPE);
        put("TAB", KeyEvent.KEYCODE_TAB);
        put("HOME", KeyEvent.KEYCODE_MOVE_HOME);
        put("END", KeyEvent.KEYCODE_MOVE_END);
        put("PGUP", KeyEvent.KEYCODE_PAGE_UP);
        put("PGDN", KeyEvent.KEYCODE_PAGE_DOWN);
        put("INS", KeyEvent.KEYCODE_INSERT);
        put("DEL", KeyEvent.KEYCODE_FORWARD_DEL);
        put("BKSP", KeyEvent.KEYCODE_DEL);
        put("UP", KeyEvent.KEYCODE_DPAD_UP);
        put("LEFT", KeyEvent.KEYCODE_DPAD_LEFT);
        put("RIGHT", KeyEvent.KEYCODE_DPAD_RIGHT);
        put("DOWN", KeyEvent.KEYCODE_DPAD_DOWN);
        put("ENTER", KeyEvent.KEYCODE_ENTER);
        put("F1", KeyEvent.KEYCODE_F1);
        put("F2", KeyEvent.KEYCODE_F2);
        put("F3", KeyEvent.KEYCODE_F3);
        put("F4", KeyEvent.KEYCODE_F4);
        put("F5", KeyEvent.KEYCODE_F5);
        put("F6", KeyEvent.KEYCODE_F6);
        put("F7", KeyEvent.KEYCODE_F7);
        put("F8", KeyEvent.KEYCODE_F8);
        put("F9", KeyEvent.KEYCODE_F9);
        put("F10", KeyEvent.KEYCODE_F10);
        put("F11", KeyEvent.KEYCODE_F11);
        put("F12", KeyEvent.KEYCODE_F12);
    }};

    private void sendKey(View view, String keyName, boolean forceCtrlDown, boolean forceLeftAltDown) {
        TerminalView terminalView = view.findViewById(R.id.terminal_view);
        if ("KEYBOARD".equals(keyName)) {
            InputMethodManager imm = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            imm.toggleSoftInput(0, 0);
        } else if ("DRAWER".equals(keyName)) {
            DrawerLayout drawer = view.findViewById(R.id.drawer_layout);
            drawer.openDrawer(Gravity.LEFT);
        } else if (keyCodesForString.containsKey(keyName)) {
            int keyCode = keyCodesForString.get(keyName);
            int metaState = 0;
            if (forceCtrlDown) {
                metaState |= KeyEvent.META_CTRL_ON | KeyEvent.META_CTRL_LEFT_ON;
            }
            if (forceLeftAltDown) {
                metaState |= KeyEvent.META_ALT_ON | KeyEvent.META_ALT_LEFT_ON;
            }
            KeyEvent keyEvent = new KeyEvent(0, 0, KeyEvent.ACTION_UP, keyCode, 0, metaState);
            terminalView.onKeyDown(keyCode, keyEvent);
        } else {
            // not a control char
            keyName.codePoints().forEach(codePoint -> {
                terminalView.inputCodePoint(codePoint, forceCtrlDown, forceLeftAltDown);
            });
        }
    }

    private void sendKey(View view, ExtraKeyButton buttonInfo) {
        if (buttonInfo.isMacro()) {
            String[] keys = buttonInfo.getKey().split(" ");
            boolean ctrlDown = false;
            boolean altDown = false;
            for (String key : keys) {
                if ("CTRL".equals(key)) {
                    ctrlDown = true;
                } else if ("ALT".equals(key)) {
                    altDown = true;
                } else {
                    sendKey(view, key, ctrlDown, altDown);
                    ctrlDown = false;
                    altDown = false;
                }
            }
        } else {
            sendKey(view, buttonInfo.getKey(), false, false);
        }
    }

    public enum SpecialButton {
        CTRL, ALT, FN
    }

    private static class SpecialButtonState {
        boolean isOn = false;
        ToggleButton button = null;
    }

    private Map<SpecialButton, SpecialButtonState> specialButtons = new HashMap<SpecialButton, SpecialButtonState>() {{
        put(SpecialButton.CTRL, new SpecialButtonState());
        put(SpecialButton.ALT, new SpecialButtonState());
        put(SpecialButton.FN, new SpecialButtonState());
    }};

    private ScheduledExecutorService scheduledExecutor;
    private PopupWindow popupWindow;
    private int longPressCount;

    public boolean readSpecialButton(SpecialButton name) {
        SpecialButtonState state = specialButtons.get(name);
        if (state == null)
            throw new RuntimeException("Must be a valid special button (see source)");

        if (! state.isOn)
            return false;

        if (state.button == null) {
            return false;
        }

        if (state.button.isPressed())
            return true;

        if (! state.button.isChecked())
            return false;

        state.button.setChecked(false);
        state.button.setTextColor(TEXT_COLOR);
        return true;
    }

    void popup(View view, String text) {
        int width = view.getMeasuredWidth();
        int height = view.getMeasuredHeight();
        Button button = new Button(getContext(), null, android.R.attr.buttonBarButtonStyle);
        button.setText(text);
        button.setTextColor(TEXT_COLOR);
        button.setPadding(0, 0, 0, 0);
        button.setMinHeight(0);
        button.setMinWidth(0);
        button.setMinimumWidth(0);
        button.setMinimumHeight(0);
        button.setWidth(width);
        button.setHeight(height);
        button.setBackgroundColor(BUTTON_PRESSED_COLOR);
        popupWindow = new PopupWindow(this);
        popupWindow.setWidth(LayoutParams.WRAP_CONTENT);
        popupWindow.setHeight(LayoutParams.WRAP_CONTENT);
        popupWindow.setContentView(button);
        popupWindow.setOutsideTouchable(true);
        popupWindow.setFocusable(false);
        popupWindow.showAsDropDown(view, 0, -2 * height);
    }

    /**
     * General util function to compute the longest column length in a matrix.
     */
    static int maximumLength(Object[][] matrix) {
        int m = 0;
        for (Object[] row : matrix)
            m = Math.max(m, row.length);
        return m;
    }

    /**
     * Reload the view given parameters in termux.properties
     *
     * @param infos matrix as defined in termux.properties extrakeys
     * Can Contain The Strings CTRL ALT TAB FN ENTER LEFT RIGHT UP DOWN or normal strings
     * Some aliases are possible like RETURN for ENTER, LT for LEFT and more (@see controlCharsAliases for the whole list).
     * Any string of length > 1 in total Uppercase will print a warning
     *
     * Examples:
     * "ENTER" will trigger the ENTER keycode
     * "LEFT" will trigger the LEFT keycode and be displayed as "←"
     * "→" will input a "→" character
     * "−" will input a "−" character
     * "-_-" will input the string "-_-"
     */
    @SuppressLint("ClickableViewAccessibility")
    void reload(ExtraKeysInfos infos) {
        if(infos == null)
            return;

        for(SpecialButtonState state : specialButtons.values())
            state.button = null;

        removeAllViews();

        ExtraKeyButton[][] buttons = infos.getMatrix();

        setRowCount(buttons.length);
        setColumnCount(maximumLength(buttons));

        for (int row = 0; row < buttons.length; row++) {
            for (int col = 0; col < buttons[row].length; col++) {
                final ExtraKeyButton buttonInfo = buttons[row][col];

                Button button;
                if(Arrays.asList("CTRL", "ALT", "FN").contains(buttonInfo.getKey())) {
                    SpecialButtonState state = specialButtons.get(SpecialButton.valueOf(buttonInfo.getKey())); // for valueOf: https://stackoverflow.com/a/604426/1980630
                    state.isOn = true;
                    button = state.button = new ToggleButton(getContext(), null, android.R.attr.buttonBarButtonStyle);
                    button.setClickable(true);
                } else {
                    button = new Button(getContext(), null, android.R.attr.buttonBarButtonStyle);
                }

                button.setText(buttonInfo.getDisplay());
                button.setTextColor(TEXT_COLOR);
                button.setPadding(0, 0, 0, 0);

                final Button finalButton = button;
                button.setOnClickListener(v -> {
                    if (Settings.System.getInt(getContext().getContentResolver(),
                        Settings.System.HAPTIC_FEEDBACK_ENABLED, 0) != 0) {

                        if (Build.VERSION.SDK_INT >= 28) {
                            finalButton.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
                        } else {
                            // Perform haptic feedback only if no total silence mode enabled.
                            if (Settings.Global.getInt(getContext().getContentResolver(), "zen_mode", 0) != 2) {
                                finalButton.performHapticFeedback(HapticFeedbackConstants.KEYBOARD_TAP);
                            }
                        }
                    }

                    View root = getRootView();
                    if (Arrays.asList("CTRL", "ALT", "FN").contains(buttonInfo.getKey())) {
                        ToggleButton self = (ToggleButton) finalButton;
                        self.setChecked(self.isChecked());
                        self.setTextColor(self.isChecked() ? INTERESTING_COLOR : TEXT_COLOR);
                    } else {
                        sendKey(root, buttonInfo);
                    }
                });

                button.setOnTouchListener((v, event) -> {
                    final View root = getRootView();
                    switch (event.getAction()) {
                        case MotionEvent.ACTION_DOWN:
                            longPressCount = 0;
                            v.setBackgroundColor(BUTTON_PRESSED_COLOR);
                            if (Arrays.asList("UP", "DOWN", "LEFT", "RIGHT", "BKSP", "DEL").contains(buttonInfo.getKey())) {
                                // autorepeat
                                scheduledExecutor = Executors.newSingleThreadScheduledExecutor();
                                scheduledExecutor.scheduleWithFixedDelay(() -> {
                                    longPressCount++;
                                    sendKey(root, buttonInfo);
                                }, 400, 80, TimeUnit.MILLISECONDS);
                            }
                            return true;

                        case MotionEvent.ACTION_MOVE:
                            if (buttonInfo.getPopup() != null) {
                                if (popupWindow == null && event.getY() < 0) {
                                    if (scheduledExecutor != null) {
                                        scheduledExecutor.shutdownNow();
                                        scheduledExecutor = null;
                                    }
                                    v.setBackgroundColor(BUTTON_COLOR);
                                    String extraButtonDisplayedText = buttonInfo.getPopup().getDisplay();
                                    popup(v, extraButtonDisplayedText);
                                }
                                if (popupWindow != null && event.getY() > 0) {
                                    v.setBackgroundColor(BUTTON_PRESSED_COLOR);
                                    popupWindow.dismiss();
                                    popupWindow = null;
                                }
                            }
                            return true;

                        case MotionEvent.ACTION_CANCEL:
                            v.setBackgroundColor(BUTTON_COLOR);
                            if (scheduledExecutor != null) {
                                scheduledExecutor.shutdownNow();
                                scheduledExecutor = null;
                            }
                            return true;
                        case MotionEvent.ACTION_UP:
                            v.setBackgroundColor(BUTTON_COLOR);
                            if (scheduledExecutor != null) {
                                scheduledExecutor.shutdownNow();
                                scheduledExecutor = null;
                            }
                            if (longPressCount == 0 || popupWindow != null) {
                                if (popupWindow != null) {
                                    popupWindow.setContentView(null);
                                    popupWindow.dismiss();
                                    popupWindow = null;
                                    if (buttonInfo.getPopup() != null) {
                                        sendKey(root, buttonInfo.getPopup());
                                    }
                                } else {
                                    v.performClick();
                                }
                            }
                            return true;

                        default:
                            return true;
                    }
                });

                LayoutParams param = new GridLayout.LayoutParams();
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

}
