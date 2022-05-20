package com.termux.terminal;

public class FastResize implements ResizeBuffer {
    TerminalBuffer mTerminalBuffer;

    public FastResize(TerminalBuffer terminalBuffer) {
        mTerminalBuffer = terminalBuffer;
    }

    public void resize(int newColumns, int newRows, int newTotalRows, int[] cursor, long currentStyle, boolean isAltScreen) {
        int shiftDownOfTopRow = calcShiftDownOfTopRow(newRows, cursor, currentStyle);
        mTerminalBuffer.mScreenFirstRow += shiftDownOfTopRow;
        mTerminalBuffer.mScreenFirstRow = (mTerminalBuffer.mScreenFirstRow < 0) ? (mTerminalBuffer.mScreenFirstRow + mTerminalBuffer.mTotalRows) : (mTerminalBuffer.mScreenFirstRow % mTerminalBuffer.mTotalRows);
        mTerminalBuffer.mTotalRows = newTotalRows;
        mTerminalBuffer.mActiveTranscriptRows = isAltScreen ? 0 : Math.max(0, mTerminalBuffer.mActiveTranscriptRows + shiftDownOfTopRow);
        cursor[1] -= shiftDownOfTopRow;
        mTerminalBuffer.mScreenRows = newRows;
    }

    private int calcShiftDownOfTopRow(int newRows, int[] cursor, long currentStyle) {
        int shiftDownOfTopRow = mTerminalBuffer.mScreenRows - newRows;
        final boolean isShrinking = shiftDownOfTopRow > 0 && shiftDownOfTopRow < mTerminalBuffer.mScreenRows;
        final boolean isExpanding = shiftDownOfTopRow < 0;
        if (isShrinking) {
            // Shrinking. Check if we can skip blank rows at bottom below cursor.
            shiftDownOfTopRow = shrinkingRows(cursor, shiftDownOfTopRow);
        } else if (isExpanding) {
            // Negative shift down = expanding. Only move screen up if there is transcript to show:
            shiftDownOfTopRow = expandingRows(currentStyle, shiftDownOfTopRow);
        }
        return shiftDownOfTopRow;
    }

    private int shrinkingRows(int[] cursor, int shiftDownOfTopRow) {
        for (int i = mTerminalBuffer.mScreenRows - 1; i > 0; i--) {
            if (cursor[1] >= i) break;
            int r = mTerminalBuffer.externalToInternalRow(i);
            final boolean isLineEmpty = mTerminalBuffer.mLines[r] == null || mTerminalBuffer.mLines[r].isBlank();
            if (isLineEmpty) {
                final boolean isShrinkingEnd = --shiftDownOfTopRow == 0;
                if (isShrinkingEnd) break;
            }
        }
        return shiftDownOfTopRow;
    }

    private int expandingRows(long currentStyle, int shiftDownOfTopRow) {
        int actualShift = Math.max(shiftDownOfTopRow, -mTerminalBuffer.mActiveTranscriptRows);
        if (shiftDownOfTopRow != actualShift) {
            // The new lines revealed by the resizing are not all from the transcript. Blank the below ones.
            for (int i = 0; i < actualShift - shiftDownOfTopRow; i++)
                mTerminalBuffer.allocateFullLineIfNecessary((mTerminalBuffer.mScreenFirstRow + mTerminalBuffer.mScreenRows + i) % mTerminalBuffer.mTotalRows).clear(currentStyle);
            shiftDownOfTopRow = actualShift;
        }
        return shiftDownOfTopRow;
    }

}
