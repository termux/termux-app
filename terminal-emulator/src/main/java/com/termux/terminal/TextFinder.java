package com.termux.terminal;

public final class TextFinder {

    TerminalRow[] mLines;
    /** The length of {@link #mLines}. */
    int mTotalRows;
    /** The number of rows and columns visible on the screen. */
    int mScreenRows, mColumns;
    /** The number of rows kept in history. */
    private int mActiveTranscriptRows;
    /** The index in the circular buffer where the visible screen starts. */
    private int mScreenFirstRow;

    public TextFinder(TerminalBuffer terminalBuffer) {
        this.mLines = terminalBuffer.mLines;
        this.mColumns = terminalBuffer.mColumns;
        this.mTotalRows = terminalBuffer.mTotalRows;
        this.mScreenRows = terminalBuffer.mScreenRows;
        this.mActiveTranscriptRows = terminalBuffer.getActiveTranscriptRows();
        this.mScreenFirstRow = terminalBuffer.getScreenFirstRows();
    }

    public String getSelectedText(int selX1, int selY1, int selX2, int selY2, boolean joinBackLines, boolean joinFullLines) {
        final StringBuilder builder = new StringBuilder();
        final int columns = mColumns;

        if (selY1 < -mActiveTranscriptRows) selY1 = -mActiveTranscriptRows;
        if (selY2 >= mScreenRows) selY2 = mScreenRows - 1;

        for (int row = selY1; row <= selY2; row++) {
            int x1 = (row == selY1) ? selX1 : 0;
            int x2;
            if (row == selY2) {
                x2 = selX2 + 1;
                if (x2 > columns) x2 = columns;
            } else {
                x2 = columns;
            }
            TerminalRow lineObject = mLines[externalToInternalRow(row)];
            int x1Index = lineObject.findStartOfColumn(x1);
            int x2Index = (x2 < mColumns) ? lineObject.findStartOfColumn(x2) : lineObject.getSpaceUsed();
            if (x2Index == x1Index) {
                // Selected the start of a wide character.
                x2Index = lineObject.findStartOfColumn(x2 + 1);
            }
            char[] line = lineObject.mText;
            boolean rowLineWrap = getLineWrap(row);
            int lastPrintingCharIndex;
            if (rowLineWrap && x2 == columns) {
                // If the line was wrapped, we shouldn't lose trailing space:
                lastPrintingCharIndex = x2Index - 1;
            }
            else {
                lastPrintingCharIndex = findLastPrintingCharIndex(line, x1Index, x2Index);
            }

            int len = lastPrintingCharIndex - x1Index + 1;
            if (lastPrintingCharIndex != -1 && len > 0)
                builder.append(line, x1Index, len);

            boolean lineFillsWidth = lastPrintingCharIndex == x2Index - 1;
            boolean isBackLineRowNotWrapped = !(joinBackLines && rowLineWrap);
            boolean isFullLineNotFillsWidth = !(joinFullLines && lineFillsWidth);
            boolean isRowBeforeSelY2 = row < selY2 && row < mScreenRows - 1;
            if (isBackLineRowNotWrapped && isFullLineNotFillsWidth && isRowBeforeSelY2) builder.append('\n');
        }
        return builder.toString();
    }

    private int findLastPrintingCharIndex(char[] line, int x1Index, int x2Index) {
        int lastPrintingCharIndex = -1;
        for (int i = x1Index; i < x2Index; ++i) {
            char c = line[i];
            if (c != ' ') lastPrintingCharIndex = i;
        }
        return lastPrintingCharIndex;
    }

    public String getWordAtLocation(int x, int y) {
        // Set y1 and y2 to the lines where the wrapped line starts and ends.
        // I.e. if a line that is wrapped to 3 lines starts at line 4, and this
        // is called with y=5, then y1 would be set to 4 and y2 would be set to 6.
        int y1 = lineWrapStarts(y);
        int y2 = lineWrapEnds(y);

        // Get the text for the whole wrapped line
        String text = getSelectedText(0, y1, mColumns, y2, true, true);
        // The index of x in text
        int textOffset = (y - y1) * mColumns + x;

        if (textOffset >= text.length()) {
            // The click was to the right of the last word on the line, so
            // there's no word to return
            return "";
        }

        return getWordAtOffset(text, textOffset);
    }

    private String getWordAtOffset(String text, int textOffset) {
        // Set x1 and x2 to the indices of the last space before x and the
        // first space after x in text respectively
        int x1 = text.lastIndexOf(' ', textOffset);
        int x2 = text.indexOf(' ', textOffset);
        if (x2 == -1) {
            x2 = text.length();
        }

        if (x1 == x2) {
            // The click was on a space, so there's no word to return
            return "";
        }
        return text.substring(x1 + 1, x2);
    }

    private int lineWrapStarts(int y) {
        int y1 = y;
        while (y1 > 0 && !getSelectedText(0, y1 - 1, mColumns, y, true, true).contains("\n")) {
            y1--;
        }
        return y1;
    }

    private int lineWrapEnds(int y) {
        int y2 = y;
        while (y2 < mScreenRows && !getSelectedText(0, y, mColumns, y2 + 1, true, true).contains("\n")) {
            y2++;
        }
        return y2;
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

    public boolean getLineWrap(int row) {
        return mLines[externalToInternalRow(row)].mLineWrap;
    }


}
