package com.termux.app;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.ToggleButton;

import com.termux.view.TerminalView;

/**
 * A view showing extra keys (such as Escape, Ctrl, Alt) not normally available on an Android soft
 * keyboard.
 */
public final class ExtraKeysView extends GridLayout implements TerminalView.KeyboardModifiers {

    private static final int TEXT_COLOR = 0xFFFFFFFF;

    public ExtraKeysView(Context context, AttributeSet attrs) {
        super(context, attrs);

        reload();
    }

    private static int keyNameToKeyCode(String keyName) {
        switch (keyName) {
            case "ESC": return KeyEvent.KEYCODE_ESCAPE;
            case "TAB": return KeyEvent.KEYCODE_TAB;
            case "▲": return KeyEvent.KEYCODE_DPAD_UP;
            case "◀": return KeyEvent.KEYCODE_DPAD_LEFT;
            case "▶": return KeyEvent.KEYCODE_DPAD_RIGHT;
            case "▼": return KeyEvent.KEYCODE_DPAD_DOWN;
            default: return -1;
        }
    }

    private ToggleButton controlButton;
    private ToggleButton altButton;
    private ToggleButton fnButton;

    public boolean readControlButton() {
        boolean result = controlButton.isChecked();
        if (result) {
            controlButton.setChecked(false);
            controlButton.setTextColor(TEXT_COLOR);
        }
        return result;
    }

    public boolean readAltButton() {
        boolean result = altButton.isChecked();
        if (result) {
            altButton.setChecked(false);
            altButton.setTextColor(TEXT_COLOR);
        }
        return result;
    }

    public boolean readFnButton() {
        boolean result = fnButton.isChecked();
        if (result) {
            fnButton.setChecked(false);
            fnButton.setTextColor(TEXT_COLOR);
        }
        return result;
    }

    void reload() {
        altButton = controlButton = null;
        removeAllViews();

        String[][] buttons = {
            {"ESC", "CTRL", "ALT", "▲", "▼", "◀", "▶"}
        };

        final int rows = buttons.length;
        final int cols = buttons[0].length;

        setRowCount(rows);
        setColumnCount(cols);

        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                final String buttonText = buttons[row][col];

                Button button;
                switch (buttonText) {
                    case "CTRL":
                        button = controlButton = new ToggleButton(getContext(), null, android.R.attr.buttonBarButtonStyle);
                        break;
                    case "ALT":
                        button = altButton = new ToggleButton(getContext(), null, android.R.attr.buttonBarButtonStyle);
                        break;
                    case "FN":
                        button = fnButton = new ToggleButton(getContext(), null, android.R.attr.buttonBarButtonStyle);
                        break;
                    default:
                        button = new Button(getContext(), null, android.R.attr.buttonBarButtonStyle);
                        break;
                }

                button.setText(buttonText);
                button.setTextColor(TEXT_COLOR);

                final Button finalButton = button;
                button.setOnClickListener(new OnClickListener() {
                    @Override
                    public void onClick(View v) {
                        View root = getRootView();
                        switch (buttonText) {
                            case "CTRL":
                            case "ALT":
                            case "FN":
                                ToggleButton self = (ToggleButton) finalButton;
                                self.setChecked(self.isChecked());
                                self.setTextColor(self.isChecked() ? 0xFF80DEEA : TEXT_COLOR);
                                break;
                            default:
                                int keyCode = keyNameToKeyCode(buttonText);
                                root.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, keyCode));
                                root.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, keyCode));
                                break;
                        }
                    }
                });

                GridLayout.LayoutParams param = new GridLayout.LayoutParams();
                param.height = param.width = 0;
                param.rightMargin = param.topMargin = 0;
                param.setGravity(Gravity.LEFT);
                float weight = "▲▼◀▶".contains(buttonText) ? 0.7f : 1.f;
                param.columnSpec = GridLayout.spec(col, weight);
                param.rowSpec = GridLayout.spec(row, 1.f);
                button.setLayoutParams(param);

                addView(button);
            }
        }
    }

}
