package com.termux.app;

import android.content.Context;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.View;
import android.widget.Button;
import android.widget.GridLayout;
import android.widget.ToggleButton;

import com.termux.R;
import com.termux.terminal.TerminalSession;
import com.termux.view.TerminalView;

/**
 * A view showing extra keys (such as Escape, Ctrl, Alt) not normally available on an Android soft
 * keyboard.
 */
public final class ExtraKeysView extends GridLayout {

    private static final int TEXT_COLOR = 0xFFFFFFFF;

    public ExtraKeysView(Context context, AttributeSet attrs) {
        super(context, attrs);

        reload();
    }

    static void sendKey(View view, String keyName) {
        int keyCode = 0;
        String chars = null;
        switch (keyName) {
            case "ESC":
                keyCode = KeyEvent.KEYCODE_ESCAPE;
                break;
            case "TAB":
                keyCode = KeyEvent.KEYCODE_TAB;
                break;
            case "▲":
                keyCode = KeyEvent.KEYCODE_DPAD_UP;
                break;
            case "◀":
                keyCode = KeyEvent.KEYCODE_DPAD_LEFT;
                break;
            case "▶":
                keyCode = KeyEvent.KEYCODE_DPAD_RIGHT;
                break;
            case "▼":
                keyCode = KeyEvent.KEYCODE_DPAD_DOWN;
                break;
            case "―":
                chars = "-";
                break;
            default:
                chars = keyName;
        }

        if (keyCode > 0) {
            view.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_DOWN, keyCode));
            view.dispatchKeyEvent(new KeyEvent(KeyEvent.ACTION_UP, keyCode));
        } else {
            TerminalView terminalView = (TerminalView) view.findViewById(R.id.terminal_view);
            TerminalSession session = terminalView.getCurrentSession();
            if (session != null) session.write(chars);
        }
    }

    private ToggleButton controlButton;
    private ToggleButton altButton;
    private ToggleButton fnButton;

    public boolean readControlButton() {
        if (controlButton.isPressed()) return true;
        boolean result = controlButton.isChecked();
        if (result) {
            controlButton.setChecked(false);
            controlButton.setTextColor(TEXT_COLOR);
        }
        return result;
    }

    public boolean readAltButton() {
        if (altButton.isPressed()) return true;
        boolean result = altButton.isChecked();
        if (result) {
            altButton.setChecked(false);
            altButton.setTextColor(TEXT_COLOR);
        }
        return result;
    }

    public boolean readFnButton() {
        if (fnButton.isPressed()) return true;
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
            {"ESC", "CTRL", "ALT", "TAB", "―", "/", "|"}
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
                        button.setClickable(true);
                        break;
                    case "ALT":
                        button = altButton = new ToggleButton(getContext(), null, android.R.attr.buttonBarButtonStyle);
                        button.setClickable(true);
                        break;
                    case "FN":
                        button = fnButton = new ToggleButton(getContext(), null, android.R.attr.buttonBarButtonStyle);
                        button.setClickable(true);
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
                                sendKey(root, buttonText);
                                break;
                        }
                    }
                });

                GridLayout.LayoutParams param = new GridLayout.LayoutParams();
                param.height = param.width = 0;
                param.rightMargin = param.topMargin = 0;
                param.setGravity(Gravity.LEFT);
                float weight = "▲▼◀▶".contains(buttonText) ? 0.7f : 1.f;
                param.columnSpec = GridLayout.spec(col, GridLayout.FILL, weight);
                param.rowSpec = GridLayout.spec(row, GridLayout.FILL, 1.f);
                button.setLayoutParams(param);

                addView(button);
            }
        }
    }

}
