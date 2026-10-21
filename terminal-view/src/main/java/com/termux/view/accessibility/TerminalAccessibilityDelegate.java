package com.termux.view.accessibility;

import android.os.Bundle;
import android.view.View;
import android.view.accessibility.AccessibilityNodeInfo;

import androidx.annotation.NonNull;

import com.termux.view.TerminalView;

public class TerminalAccessibilityDelegate extends View.AccessibilityDelegate {

    private static final String LOG_TAG = "TerminalAccessibilityDelegate";

    public static String EXTRA_ACCESSIBILITY_NODE_TYPE = "accessibility-node-type";
    public static String EXTRA_TERMINAL_CURSOR_COL = "terminal-cursor-col";
    public static String EXTRA_TERMINAL_CURSOR_ROW = "terminal-cursor-row";

    private final TerminalView mTerminalView;

    public TerminalAccessibilityDelegate(@NonNull TerminalView terminalView) {
        mTerminalView = terminalView;
    }

    @Override
    public void onInitializeAccessibilityNodeInfo(@NonNull View host, @NonNull AccessibilityNodeInfo info) {
        mTerminalView.mClient.logInfo(LOG_TAG, "onInitializeAccessibilityNodeInfo");
        super.onInitializeAccessibilityNodeInfo(host, info);
        Bundle extra = info.getExtras();
        if (mTerminalView.mEmulator != null) {
            extra.putString(EXTRA_ACCESSIBILITY_NODE_TYPE, "terminal");
            extra.putInt(EXTRA_TERMINAL_CURSOR_COL, mTerminalView.mEmulator.getCursorCol());
            extra.putInt(EXTRA_TERMINAL_CURSOR_ROW, mTerminalView.mEmulator.getCursorRow());
            mTerminalView.mClient.logInfo(LOG_TAG, "col=" + mTerminalView.mEmulator.getCursorCol() + ", row=" + mTerminalView.mEmulator.getCursorRow());
        }

        info.setContentDescription(mTerminalView.getText());
    }
}
