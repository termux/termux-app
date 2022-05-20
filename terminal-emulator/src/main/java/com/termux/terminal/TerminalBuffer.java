package com.termux.terminal;

import java.util.Arrays;

/**
 * A circular buffer of {@link TerminalRow}:s which keeps notes about what is visible on a logical screen and the scroll
 * history.
 * <p>
 * See {@link #externalToInternalRow(int)} for how to map from logical screen rows to array indices.
 */
public final class TerminalBuffer {

    TerminalRow[] mLines;
    /** The length of {@link #mLines}. */
    int mTotalRows;
    /** The number of rows and columns visible on the screen. */
    int mScreenRows, mColumns;
    /** The number of rows kept in history. */
    int mActiveTranscriptRows = 0;
    /** The index in the circular buffer where the visible screen starts. */
    int mScreenFirstRow = 0;

    /**
     * Create a transcript screen.
     *
     * @param columns    the width of the screen in characters.
     * @param totalRows  the height of the entire text area, in rows of text.
     * @param screenRows the height of just the screen, not including the transcript that holds lines that have scrolled off
     *                   the top of the screen.
     */
    public TerminalBuffer(int columns, int totalRows, int screenRows) {
        mColumns = columns;
        mTotalRows = totalRows;
        mScreenRows = screenRows;
        mLines = new TerminalRow[totalRows];

        blockSet(0, 0, columns, screenRows, ' ', TextStyle.NORMAL);
    }

    public String getTranscriptText() {
        return getSelectedText(0, -getActiveTranscriptRows(), mColumns, mScreenRows).trim();
    }

    public String getTranscriptTextWithoutJoinedLines() {
        return getSelectedText(0, -getActiveTranscriptRows(), mColumns, mScreenRows, false).trim();
    }

    public String getTranscriptTextWithFullLinesJoined() {
        return getSelectedText(0, -getActiveTranscriptRows(), mColumns, mScreenRows, true, true).trim();
    }

    public String getSelectedText(int selX1, int selY1, int selX2, int selY2) {
        return getSelectedText(selX1, selY1, selX2, selY2, true);
    }

    public String getSelectedText(int selX1, int selY1, int selX2, int selY2, boolean joinBackLines) {
        return getSelectedText(selX1, selY1, selX2, selY2, true, false);
    }

    public String getSelectedText(int selX1, int selY1, int selX2, int selY2, boolean joinBackLines, boolean joinFullLines) {
        return new TextFinder(this).getSelectedText(new Cursor(selY1, selX1), new Cursor(selY2, selX2), joinBackLines, joinFullLines);
    }

    public String getWordAtLocation(int x, int y) {
        return new TextFinder(this).getWordAtLocation(x, y);
    }

    public int getActiveTranscriptRows() {
        return mActiveTranscriptRows;
    }

    public int getActiveRows() {
        return mActiveTranscriptRows + mScreenRows;
    }

    /**
     * Convert a row value from the public external coordinate system to our internal private coordinate system.
     *
     * <pre>
     * - External coordinate system: -mActiveTranscriptRows to mScreenRows-1, with the screen being 0..mScreenRows-1.
     * - Internal coordinate system: the mScreenRows lines starting at mScreenFirstRow comprise the screen, while the
     *   mActiveTranscriptRows lines ending at mScreenFirstRow-1 form the transcript (as a circular buffer).
     *
     * External ↔ Internal:
     *
     * [ ...                            ]     [ ...                                     ]
     * [ -mActiveTranscriptRows         ]     [ mScreenFirstRow - mActiveTranscriptRows ]
     * [ ...                            ]     [ ...                                     ]
     * [ 0 (visible screen starts here) ]  ↔  [ mScreenFirstRow                         ]
     * [ ...                            ]     [ ...                                     ]
     * [ mScreenRows-1                  ]     [ mScreenFirstRow + mScreenRows-1         ]
     * </pre>
     *
     * @param externalRow a row in the external coordinate system.
     * @return The row corresponding to the input argument in the private coordinate system.
     */
    public int externalToInternalRow(int externalRow) {
        if (externalRow < -mActiveTranscriptRows || externalRow > mScreenRows)
            throw new IllegalArgumentException("extRow=" + externalRow + ", mScreenRows=" + mScreenRows + ", mActiveTranscriptRows=" + mActiveTranscriptRows);
        final int internalRow = mScreenFirstRow + externalRow;
        return (internalRow < 0) ? (mTotalRows + internalRow) : (internalRow % mTotalRows);
    }

    public void setLineWrap(int row) {
        mLines[externalToInternalRow(row)].mLineWrap = true;
    }

    public boolean getLineWrap(int row) {
        return mLines[externalToInternalRow(row)].mLineWrap;
    }

    public void clearLineWrap(int row) {
        mLines[externalToInternalRow(row)].mLineWrap = false;
    }

    /**
     * Resize the screen which this transcript backs. Currently, this only works if the number of columns does not
     * change or the rows expand (that is, it only works when shrinking the number of rows).
     *
     * @param newColumns The number of columns the screen should have.
     * @param newRows    The number of rows the screen should have.
     * @param cursor     An int[2] containing the (column, row) cursor location.
     */
    public void resize(int newColumns, int newRows, int newTotalRows, int[] cursor, long currentStyle, boolean isAltScreen) {
        // newRows > mTotalRows should not normally happen since mTotalRows is TRANSCRIPT_ROWS (10000):
        if (newColumns == mColumns && newRows <= mTotalRows) {
            // Fast resize where just the rows changed.
            new FastResize(this).resize(newColumns, newRows, newTotalRows, cursor, currentStyle, isAltScreen);
        } else {
            // Copy away old state and update new:
            new UpdateOldBuffer(this).resize(newColumns, newRows, newTotalRows, cursor, currentStyle, isAltScreen);
        }

        // Handle cursor scrolling off screen:
        if (cursor[0] < 0 || cursor[1] < 0) cursor[0] = cursor[1] = 0;
    }

    /**
     * Block copy lines and associated metadata from one location to another in the circular buffer, taking wraparound
     * into account.
     *
     * @param srcInternal The first line to be copied.
     * @param len         The number of lines to be copied.
     */
    private void blockCopyLinesDown(int srcInternal, int len) {
        if (len == 0) return;
        int totalRows = mTotalRows;

        int start = len - 1;
        // Save away line to be overwritten:
        TerminalRow lineToBeOverWritten = mLines[(srcInternal + start + 1) % totalRows];
        // Do the copy from bottom to top.
        for (int i = start; i >= 0; --i)
            mLines[(srcInternal + i + 1) % totalRows] = mLines[(srcInternal + i) % totalRows];
        // Put back overwritten line, now above the block:
        mLines[(srcInternal) % totalRows] = lineToBeOverWritten;
    }

    /**
     * Scroll the screen down one line. To scroll the whole screen of a 24 line screen, the arguments would be (0, 24).
     *
     * @param topMargin    First line that is scrolled.
     * @param bottomMargin One line after the last line that is scrolled.
     * @param style        the style for the newly exposed line.
     */
    public void scrollDownOneLine(int topMargin, int bottomMargin, long style) {
        if (topMargin > bottomMargin - 1 || topMargin < 0 || bottomMargin > mScreenRows)
            throw new IllegalArgumentException("topMargin=" + topMargin + ", bottomMargin=" + bottomMargin + ", mScreenRows=" + mScreenRows);

        // Copy the fixed topMargin lines one line down so that they remain on screen in same position:
        blockCopyLinesDown(mScreenFirstRow, topMargin);
        // Copy the fixed mScreenRows-bottomMargin lines one line down so that they remain on screen in same
        // position:
        blockCopyLinesDown(externalToInternalRow(bottomMargin), mScreenRows - bottomMargin);

        // Update the screen location in the ring buffer:
        mScreenFirstRow = (mScreenFirstRow + 1) % mTotalRows;
        // Note that the history has grown if not already full:
        if (mActiveTranscriptRows < mTotalRows - mScreenRows) mActiveTranscriptRows++;

        // Blank the newly revealed line above the bottom margin:
        int blankRow = externalToInternalRow(bottomMargin - 1);
        if (mLines[blankRow] == null) {
            mLines[blankRow] = new TerminalRow(mColumns, style);
        } else {
            mLines[blankRow].clear(style);
        }
    }

    /**
     * Block copy characters from one position in the screen to another. The two positions can overlap. All characters
     * of the source and destination must be within the bounds of the screen, or else an InvalidParameterException will
     * be thrown.
     *
     * @param sx source X coordinate
     * @param sy source Y coordinate
     * @param w  width
     * @param h  height
     * @param dx destination X coordinate
     * @param dy destination Y coordinate
     */
    public void blockCopy(int sx, int sy, int w, int h, int dx, int dy) {
        if (w == 0) return;
        if (sx < 0 || sx + w > mColumns || sy < 0 || sy + h > mScreenRows || dx < 0 || dx + w > mColumns || dy < 0 || dy + h > mScreenRows)
            throw new IllegalArgumentException();
        boolean isCopyingUp = sy > dy;
        for (int y = 0; y < h; y++) {
            int y2 = isCopyingUp ? y : (h - (y + 1));
            TerminalRow sourceRow = allocateFullLineIfNecessary(externalToInternalRow(sy + y2));
            allocateFullLineIfNecessary(externalToInternalRow(dy + y2)).copyInterval(sourceRow, sx, sx + w, dx);
        }
    }

    /**
     * Block set characters. All characters must be within the bounds of the screen, or else and
     * InvalidParemeterException will be thrown. Typically this is called with a "val" argument of 32 to clear a block
     * of characters.
     */
    public void blockSet(int sx, int sy, int w, int h, int val, long style) {
        if (sx < 0 || sx + w > mColumns || sy < 0 || sy + h > mScreenRows) {
            throw new IllegalArgumentException(
                "Illegal arguments! blockSet(" + sx + ", " + sy + ", " + w + ", " + h + ", " + val + ", " + mColumns + ", " + mScreenRows + ")");
        }
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++)
                setChar(sx + x, sy + y, val, style);
    }

    public TerminalRow allocateFullLineIfNecessary(int row) {
        return (mLines[row] == null) ? (mLines[row] = new TerminalRow(mColumns, 0)) : mLines[row];
    }

    public void setChar(int column, int row, int codePoint, long style) {
        if (row  < 0 || row >= mScreenRows || column < 0 || column >= mColumns)
            throw new IllegalArgumentException("TerminalBuffer.setChar(): row=" + row + ", column=" + column + ", mScreenRows=" + mScreenRows + ", mColumns=" + mColumns);
        row = externalToInternalRow(row);
        allocateFullLineIfNecessary(row).setChar(column, codePoint, style);
    }

    public long getStyleAt(int externalRow, int column) {
        return allocateFullLineIfNecessary(externalToInternalRow(externalRow)).getStyle(column);
    }

    /** Support for http://vt100.net/docs/vt510-rm/DECCARA and http://vt100.net/docs/vt510-rm/DECCARA */
    public void setOrClearEffect(int bits, boolean isSetOrClear, boolean isReverse, boolean isRectangular, int leftMargin, int rightMargin, int top, int left,
                                 int bottom, int right) {
        for (int row = top; row < bottom; row++) {
            TerminalRow line = mLines[externalToInternalRow(row)];
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

    public void clearTranscript() {
        if (mScreenFirstRow < mActiveTranscriptRows) {
            Arrays.fill(mLines, mTotalRows + mScreenFirstRow - mActiveTranscriptRows, mTotalRows, null);
            Arrays.fill(mLines, 0, mScreenFirstRow, null);
        } else {
            Arrays.fill(mLines, mScreenFirstRow - mActiveTranscriptRows, mScreenFirstRow, null);
        }
        mActiveTranscriptRows = 0;
    }

}

