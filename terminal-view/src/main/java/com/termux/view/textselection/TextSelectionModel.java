package com.termux.view.textselection;

import android.text.TextUtils;
import android.view.ActionMode;

import com.termux.terminal.TerminalBuffer;
import com.termux.terminal.WcWidth;
import com.termux.view.TerminalView;

public class TextSelectionModel {
    private boolean mIsSelectingText = false;
    private long mShowStartTime = System.currentTimeMillis();

    private final int mHandleHeight;
    private int mSelX1 = -1, mSelX2 = -1, mSelY1 = -1, mSelY2 = -1;

    TextSelectionModel(int handleHeight) {
        this.mHandleHeight = handleHeight;
    }

    public int getmSelX1() {
        return mSelX1;
    }

    public int getmSelX2() {
        return mSelX2;
    }

    public int getmSelY1() {
        return mSelY1;
    }

    public int getmSelY2() {
        return mSelY2;
    }

    public int[] getSelPos() {
        int[] selectionPosArray = {mSelX1, mSelY1, mSelX2, mSelY2};
        return selectionPosArray;
    }

    public boolean getIsSelcetingText() {
        return mIsSelectingText;
    }

    public long getmShowStartTime() {
        return mShowStartTime;
    }

    public int getmHandleHeight() {
        return mHandleHeight;
    }

    public void setSelectionPosition(int[] columnAndRow) {
        mSelX1 = mSelX2 = columnAndRow[0];
        mSelY1 = mSelY2 = columnAndRow[1];
    }

    public void setSelectionPosition(int pos) {
        mSelX1 = mSelY1 = mSelX2 = mSelY2 = pos;
    }

    public void setShorStartTime(long time) {
        this.mShowStartTime = time;
    }

    public void setIsSelectingText(boolean is) {
        this.mIsSelectingText = is;
    }

    public void setSelectionPositionBlank(TerminalBuffer screen, TerminalView terminalView) {
        String blankSpace = " ";
        String blank = "";

        if (!blankSpace.equals(screen.getSelectedText(mSelX1, mSelY1, mSelX1, mSelY1))) {
            // Selecting something other than whitespace. Expand to word.
            while (mSelX1 > 0 && !blank.equals(screen.getSelectedText(mSelX1 - 1, mSelY1, mSelX1 - 1, mSelY1))) {
                mSelX1--;
            }
            while (mSelX2 < terminalView.mEmulator.mColumns - 1 && !blank.equals(screen.getSelectedText(mSelX2 + 1, mSelY1, mSelX2 + 1, mSelY1))) {
                mSelX2++;
            }
        }
    }

    public void updatePosAtStartHandle(TerminalBuffer screen, int x, int y, int scrollRows, TerminalView terminalView) {
        mSelX1 = terminalView.getCursorX(x);
        mSelY1 = terminalView.getCursorY(y);
        if (mSelX1 < 0) {
            mSelX1 = 0;
        }

        if (mSelY1 < -scrollRows) {
            mSelY1 = -scrollRows;

        } else if (mSelY1 > terminalView.mEmulator.mRows - 1) {
            mSelY1 = terminalView.mEmulator.mRows - 1;

        }

        if (mSelY1 > mSelY2) {
            mSelY1 = mSelY2;
        }
        if (mSelY1 == mSelY2 && mSelX1 > mSelX2) {
            mSelX1 = mSelX2;
        }

        if (!terminalView.mEmulator.isAlternateBufferActive()) {
            int topRow = terminalView.getTopRow();

            if (mSelY1 <= topRow) {
                topRow--;
                if (topRow < -scrollRows) {
                    topRow = -scrollRows;
                }
            } else if (mSelY1 >= topRow + terminalView.mEmulator.mRows) {
                topRow++;
                if (topRow > 0) {
                    topRow = 0;
                }
            }

            terminalView.setTopRow(topRow);
        }

        mSelX1 = getValidCurX(screen, mSelY1, mSelX1);
    }

    public void updatePosAtEndHandle(TerminalBuffer screen, int x, int y, int scrollRows, TerminalView terminalView) {
        mSelX2 = terminalView.getCursorX(x);
        mSelY2 = terminalView.getCursorY(y);
        if (mSelX2 < 0) {
            mSelX2 = 0;
        }

        if (mSelY2 < -scrollRows) {
            mSelY2 = -scrollRows;
        } else if (mSelY2 > terminalView.mEmulator.mRows - 1) {
            mSelY2 = terminalView.mEmulator.mRows - 1;
        }

        if (mSelY1 > mSelY2) {
            mSelY2 = mSelY1;
        }
        if (mSelY1 == mSelY2 && mSelX1 > mSelX2) {
            mSelX2 = mSelX1;
        }

        if (!terminalView.mEmulator.isAlternateBufferActive()) {
            int topRow = terminalView.getTopRow();

            if (mSelY2 <= topRow) {
                topRow--;
                if (topRow < -scrollRows) {
                    topRow = -scrollRows;
                }
            } else if (mSelY2 >= topRow + terminalView.mEmulator.mRows) {
                topRow++;
                if (topRow > 0) {
                    topRow = 0;
                }
            }

            terminalView.setTopRow(topRow);
        }

        mSelX2 = getValidCurX(screen, mSelY2, mSelX2);
    }

    private int getValidCurX(TerminalBuffer screen, int cy, int cx) {
        String line = screen.getSelectedText(0, cy, cx, cy);
        if (!TextUtils.isEmpty(line)) {
            int col = 0;
            for (int i = 0, len = line.length(); i < len; i++) {
                char ch1 = line.charAt(i);
                if (ch1 == 0) {
                    break;
                }

                int wc;
                if (Character.isHighSurrogate(ch1) && i + 1 < len) {
                    char ch2 = line.charAt(++i);
                    wc = WcWidth.width(Character.toCodePoint(ch1, ch2));
                } else {
                    wc = WcWidth.width(ch1);
                }

                final int cend = col + wc;
                if (cx > col && cx < cend) {
                    return cend;
                }
                if (cend == col) {
                    return col;
                }
                col = cend;
            }
        }
        return cx;
    }

    public void decrementYSelectionPos(int decrement) {
        mSelY1 -= decrement;
        mSelY2 -= decrement;
    }
}
