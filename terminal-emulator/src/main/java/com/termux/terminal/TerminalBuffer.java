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
    private int mActiveTranscriptRows = 0;
    /** The index in the circular buffer where the visible screen starts. */
    private int mScreenFirstRow = 0;

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
            fastResize(newRows, newTotalRows, cursor, currentStyle, isAltScreen);
        } else {
            // Copy away old state and update new:
            updateOldState(newColumns, newRows, newTotalRows, cursor, currentStyle);
        }

        // Handle cursor scrolling off screen:
        if (cursor[0] < 0 || cursor[1] < 0) cursor[0] = cursor[1] = 0;
    }

    private void fastResize(int newRows, int newTotalRows, int[] cursor, long currentStyle, boolean isAltScreen) {
        int shiftDownOfTopRow = calcShiftDownOfTopRow(newRows, cursor, currentStyle);
        mScreenFirstRow += shiftDownOfTopRow;
        mScreenFirstRow = (mScreenFirstRow < 0) ? (mScreenFirstRow + mTotalRows) : (mScreenFirstRow % mTotalRows);
        mTotalRows = newTotalRows;
        mActiveTranscriptRows = isAltScreen ? 0 : Math.max(0, mActiveTranscriptRows + shiftDownOfTopRow);
        cursor[1] -= shiftDownOfTopRow;
        mScreenRows = newRows;
    }

    private int calcShiftDownOfTopRow(int newRows, int[] cursor, long currentStyle) {
        int shiftDownOfTopRow = mScreenRows - newRows;
        final boolean isShrinking = shiftDownOfTopRow > 0 && shiftDownOfTopRow < mScreenRows;
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
        for (int i = mScreenRows - 1; i > 0; i--) {
            if (cursor[1] >= i) break;
            int r = externalToInternalRow(i);
            final boolean isLineEmpty = mLines[r] == null || mLines[r].isBlank();
            if (isLineEmpty) {
                final boolean isShrinkingEnd = --shiftDownOfTopRow == 0;
                if (isShrinkingEnd) break;
            }
        }
        return shiftDownOfTopRow;
    }

    private int expandingRows(long currentStyle, int shiftDownOfTopRow) {
        int actualShift = Math.max(shiftDownOfTopRow, -mActiveTranscriptRows);
        if (shiftDownOfTopRow != actualShift) {
            // The new lines revealed by the resizing are not all from the transcript. Blank the below ones.
            for (int i = 0; i < actualShift - shiftDownOfTopRow; i++)
                allocateFullLineIfNecessary((mScreenFirstRow + mScreenRows + i) % mTotalRows).clear(currentStyle);
            shiftDownOfTopRow = actualShift;
        }
        return shiftDownOfTopRow;
    }

    private void updateOldState(int newColumns, int newRows, int newTotalRows, int[] cursor, long currentStyle) {
        final TerminalRow[] oldLines = mLines;

        final int oldActiveTranscriptRows = mActiveTranscriptRows;
        final int oldScreenFirstRow = mScreenFirstRow;
        final int oldScreenRows = mScreenRows;
        final int oldTotalRows = mTotalRows;

        final Cursor oldCursor = new Cursor(cursor[1], cursor[0]);

        mLines = new TerminalRow[newTotalRows];
        for (int i = 0; i < newTotalRows; i++)
            mLines[i] = new TerminalRow(newColumns, currentStyle);

        mTotalRows = newTotalRows;
        mScreenRows = newRows;
        mActiveTranscriptRows = mScreenFirstRow = 0;
        mColumns = newColumns;

        Cursor newCursor = new Cursor(-1, -1);

        Cursor currentOutputExternal = new Cursor(0, 0);

        // Loop over every character in the initial state.
        // Blank lines should be skipped only if at end of transcript (just as is done in the "fast" resize), so we
        // keep track how many blank lines we have skipped if we later on find a non-blank line.
        int skippedBlankLines = 0;
        for (int externalOldRow = -oldActiveTranscriptRows; externalOldRow < oldScreenRows; externalOldRow++) {
            final TerminalRow oldLine = getOldTerminalRow(oldLines, oldScreenFirstRow, oldTotalRows, externalOldRow);
            final boolean isCursorAtThisRow = externalOldRow == oldCursor.getRow();

            // The cursor may only be on a non-null line, which we should not skip:
            final boolean isOldCursorAtThisRow = (newCursor.getRow() == -1) && isCursorAtThisRow;
            final boolean isCursorAtBlankLine = !isOldCursorAtThisRow && oldLine.isBlank();
            if (oldLine == null || isCursorAtBlankLine) {
                skippedBlankLines++;
                continue;
            }

            if (skippedBlankLines > 0) {
                // After skipping some blank lines we encounter a non-blank line. Insert the skipped blank lines.
                insertSkippedBlankLines(currentStyle, currentOutputExternal, skippedBlankLines);
                skippedBlankLines = 0;
            }

            int lastNonSpaceIndex = 0;
            boolean isJustToCursor = false;
            if (isCursorAtThisRow || oldLine.mLineWrap) {
                // Take the whole line, either because of cursor on it, or if line wrapping.
                lastNonSpaceIndex = oldLine.getSpaceUsed();
                if (isCursorAtThisRow) isJustToCursor = true;
            } else {
                for (int i = 0; i < oldLine.getSpaceUsed(); i++)
                    // NEWLY INTRODUCED BUG! Should not index oldLine.mStyle with char indices
                    if (oldLine.mText[i] != ' '/* || oldLine.mStyle[i] != currentStyle */)
                        lastNonSpaceIndex = i + 1;
            }

            copyLine(currentStyle, oldCursor, newCursor, currentOutputExternal, externalOldRow, oldLine, lastNonSpaceIndex, isJustToCursor);

            // Old row has been copied. Check if we need to insert newline if old line was not wrapping:
            final boolean isOldRowCopied = externalOldRow != (oldScreenRows - 1);
            final boolean isOldLineNotWrapping = !oldLine.mLineWrap;
            if (isOldRowCopied && isOldLineNotWrapping) {
                setNewCursorRow(currentStyle, newCursor, currentOutputExternal);
            }
        }

        cursor[0] = newCursor.getColumn();
        cursor[1] = newCursor.getRow();
    }

    private void copyLine(long currentStyle, Cursor oldCursor, Cursor newCursor, Cursor currentOutputExternal, int externalOldRow, TerminalRow oldLine, int lastNonSpaceIndex, boolean isJustToCursor) {
        int currentOldCol = 0;
        long styleAtCol = 0;
        for (int i = 0; i < lastNonSpaceIndex; i++) {
            // Note that looping over java character, not cells.
            final char c = oldLine.mText[i];
            final int codePoint = (Character.isHighSurrogate(c)) ? Character.toCodePoint(c, oldLine.mText[++i]) : c;
            final int displayWidth = WcWidth.width(codePoint);
            // Use the last style if this is a zero-width character:
            if (displayWidth > 0) styleAtCol = oldLine.getStyle(currentOldCol);

            // Line wrap as necessary:
            lineWrap(currentStyle, newCursor, currentOutputExternal, displayWidth);

            final int offsetDueToCombiningChar = ((displayWidth <= 0 && currentOutputExternal.getColumn() > 0) ? 1 : 0);
            final int outputColumn = currentOutputExternal.getColumn() - offsetDueToCombiningChar;
            setChar(outputColumn, currentOutputExternal.getRow(), codePoint, styleAtCol);

            if (displayWidth > 0) {
                if (oldCursor.getRow() == externalOldRow && oldCursor.getColumn() == currentOldCol) {
                    newCursor.setCursor(currentOutputExternal);
                }
                currentOldCol += displayWidth;
                currentOutputExternal.addToColumn(displayWidth);
                if (isJustToCursor && newCursor.getRow() != -1) break;
            }
        }
    }

    private void lineWrap(long currentStyle, Cursor newCursor, Cursor currentOutputExternal, int displayWidth) {
        final boolean isLineLong = currentOutputExternal.getColumn() + displayWidth > mColumns;
        if (isLineLong) {
            setLineWrap(currentOutputExternal.getRow());
            setNewCursorRow(currentStyle, newCursor, currentOutputExternal);
        }
    }

    private TerminalRow getOldTerminalRow(TerminalRow[] oldLines, int oldScreenFirstRow, int oldTotalRows, int externalOldRow) {
        // Do what externalToInternalRow() does but for the old state:
        int internalOldRow = oldScreenFirstRow + externalOldRow;
        internalOldRow = (internalOldRow < 0) ? (oldTotalRows + internalOldRow) : (internalOldRow % oldTotalRows);
        return oldLines[internalOldRow];
    }

    private void insertSkippedBlankLines(long currentStyle, Cursor currentOutputExternal, int skippedBlankLines) {
        for (int i = 0; i < skippedBlankLines; i++) {
            if (currentOutputExternal.getRow() == mScreenRows - 1) {
                scrollDownOneLine(0, mScreenRows, currentStyle);
            } else {
                currentOutputExternal.addToRow(1);
            }
            currentOutputExternal.setColumn(0);
        }
    }

    private void setNewCursorRow(long currentStyle, Cursor newCursor, Cursor currentOutputExternal) {
        if (currentOutputExternal.getRow() == mScreenRows - 1) {
            if (newCursor.getRow() != -1) newCursor.addToRow(-1);
            scrollDownOneLine(0, mScreenRows, currentStyle);
        } else {
            currentOutputExternal.addToRow(1);
        }
        currentOutputExternal.setColumn(0);
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

