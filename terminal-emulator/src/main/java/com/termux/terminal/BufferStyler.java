package com.termux.terminal;

public class BufferStyler {

    TerminalBuffer mTerminalBuffer;

    public BufferStyler(TerminalBuffer terminalBuffer) {
        mTerminalBuffer = terminalBuffer;
    }

    /** Support for http://vt100.net/docs/vt510-rm/DECCARA and http://vt100.net/docs/vt510-rm/DECCARA */
    public void setOrClearEffect(int bits, boolean isSetOrClear, boolean isReverse, boolean isRectangular, int leftMargin, int rightMargin, int top, int left,
                                 int bottom, int right) {
        for (int row = top; row < bottom; row++) {
            TerminalRow line = mTerminalBuffer.mLines[mTerminalBuffer.externalToInternalRow(row)];
            int startOfLine = (isRectangular || row == top) ? left : leftMargin;
            int endOfLine = (isRectangular || row + 1 == bottom) ? right : rightMargin;
            for (int col = startOfLine; col < endOfLine; col++) {
                setStyle(bits, isSetOrClear, isReverse, line, col);
            }
        }
    }

    private void setStyle(int bits, boolean isSetOrClear, boolean isReverse, TerminalRow line, int col) {
        final long currentStyle = line.getStyle(col);
        final int foreColor = TextStyle.decodeForeColor(currentStyle);
        final int backColor = TextStyle.decodeBackColor(currentStyle);
        int effect = getEffect(bits, isSetOrClear, isReverse, currentStyle);
        line.mStyle[col] = TextStyle.encode(foreColor, backColor, effect);
    }

    private int getEffect(int bits, boolean isSetOrClear, boolean isReverse, long currentStyle) {
        int effect = TextStyle.decodeEffect(currentStyle);
        if (isReverse) {
            // Clear out the bits to reverse and add them back in reversed:
            effect = (effect & ~bits) | (bits & ~effect);
        } else if (isSetOrClear) {
            effect |= bits;
        } else {
            effect &= ~bits;
        }
        return effect;
    }

}
