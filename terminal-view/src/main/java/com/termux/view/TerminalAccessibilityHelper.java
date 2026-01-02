package com.termux.view;

import android.content.Context;
import android.graphics.Rect;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.MotionEvent;
import android.view.inputmethod.InputMethodManager;

import androidx.annotation.NonNull;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import androidx.customview.widget.ExploreByTouchHelper;

import com.termux.terminal.TerminalEmulator;

import java.util.List;

public class TerminalAccessibilityHelper extends ExploreByTouchHelper {
    private final TerminalView mView;

    public TerminalAccessibilityHelper(@NonNull TerminalView view) {
        super(view);
        mView = view;
    }

    @Override
    protected int getVirtualViewAt(float x, float y) {
        if (mView.mEmulator == null || mView.mRenderer == null) return HOST_ID;
        int row = (int) (y / mView.mRenderer.getFontLineSpacing());
        if (row >= 0 && row < mView.mEmulator.mRows) {
            // Find the start of the logical block (paragraph) this row belongs to
            int startRow = row;
            while (startRow > 0 && mView.mEmulator.getScreen().getLineWrap(mView.mTopRow + startRow - 1)) {
                startRow--;
            }
            return startRow;
        }
        return HOST_ID;
    }

    @Override
    protected void getVisibleVirtualViews(List<Integer> virtualViewIds) {
        if (mView.mEmulator == null) return;
        for (int i = 0; i < mView.mEmulator.mRows; ) {
            virtualViewIds.add(i);
            int current = i;
            while (current < mView.mEmulator.mRows - 1 && 
                   mView.mEmulator.getScreen().getLineWrap(mView.mTopRow + current)) {
                current++;
            }
            i = current + 1;
        }
    }

    @Override
    protected void onPopulateNodeForVirtualView(int virtualViewId, @NonNull AccessibilityNodeInfoCompat node) {
        if (mView.mEmulator == null || mView.mRenderer == null) {
            node.setText("");
            node.setBoundsInParent(new Rect(0, 0, 1, 1));
            return;
        }

        int startRow = virtualViewId;
        int endRow = startRow;
        while (endRow < mView.mEmulator.mRows - 1 && 
               mView.mEmulator.getScreen().getLineWrap(mView.mTopRow + endRow)) {
            endRow++;
        }

        // Get text for the logical block
        int externalStartRow = mView.mTopRow + startRow;
        int externalEndRow = mView.mTopRow + endRow;
        
        // getSelectedText(..., Y2) is inclusive in TerminalBuffer
        String text = mView.mEmulator.getScreen().getSelectedText(0, externalStartRow, mView.mEmulator.mColumns, externalEndRow, true, false).trim();
        if (text.isEmpty()) text = "Blank";

        node.setText(text);
        node.setContentDescription(text);
        
        // Bounds: from top of startRow to bottom of endRow
        int top = startRow * mView.mRenderer.getFontLineSpacing();
        int bottom = (endRow + 1) * mView.mRenderer.getFontLineSpacing();
        int width = mView.getWidth();
        node.setBoundsInParent(new Rect(0, top, width, bottom));
        
        node.addAction(AccessibilityNodeInfoCompat.ACTION_CLICK);
    }

    @Override
    protected boolean onPerformActionForVirtualView(int virtualViewId, int action, Bundle arguments) {
        if (action == AccessibilityNodeInfoCompat.ACTION_CLICK) {
            // 1. Request Focus & Show Keyboard
            mView.requestFocus();
            InputMethodManager imm = (InputMethodManager) mView.getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm != null) {
                imm.showSoftInput(mView, 0);
            }

            // 2. Simulate Mouse Click to move cursor (if supported by app like Emacs)
            // We aim for the start of the line (x=5 pixels buffer) vertically centered in the row
            // Check if mouse tracking is active to avoid sending escape codes to shells that don't support it.
            if (mView.mEmulator.isMouseTrackingActive()) {
                int row = virtualViewId; 
                float y = (row + 0.5f) * mView.mRenderer.getFontLineSpacing();
                float x = 5.0f; 
                
                long downTime = SystemClock.uptimeMillis();
                MotionEvent eventDown = MotionEvent.obtain(downTime, downTime, MotionEvent.ACTION_DOWN, x, y, 0);
                eventDown.setSource(InputDevice.SOURCE_MOUSE);
                mView.sendMouseEventCode(eventDown, TerminalEmulator.MOUSE_LEFT_BUTTON, true);
                eventDown.recycle();
                
                MotionEvent eventUp = MotionEvent.obtain(downTime, downTime + 10, MotionEvent.ACTION_UP, x, y, 0);
                eventUp.setSource(InputDevice.SOURCE_MOUSE);
                mView.sendMouseEventCode(eventUp, TerminalEmulator.MOUSE_LEFT_BUTTON, false);
                eventUp.recycle();
            }

            return true;
        }
        return false;
    }
}
