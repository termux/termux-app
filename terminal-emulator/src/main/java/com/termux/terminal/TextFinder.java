package com.termux.terminal;

public final class TextFinder {

    TerminalBuffer mTerminalBuffer;

    public TextFinder(TerminalBuffer terminalBuffer) {
        mTerminalBuffer = terminalBuffer;
    }

    public String getSelectedText(int selX1, int selY1, int selX2, int selY2, boolean joinBackLines, boolean joinFullLines) {
        final StringBuilder builder = new StringBuilder();

        if (selY1 < -mTerminalBuffer.getActiveTranscriptRows()) selY1 = -mTerminalBuffer.getActiveTranscriptRows();
        if (selY2 >= mTerminalBuffer.mScreenRows) selY2 = mTerminalBuffer.mScreenRows - 1;

        for (int row = selY1; row <= selY2; row++) {
            final int x1 = (row == selY1) ? selX1 : 0;
            final int x2 = getX2(selX2, selY2, row);

            final TerminalRow lineObject = mTerminalBuffer.mLines[mTerminalBuffer.externalToInternalRow(row)];
            final int x1Index = lineObject.findStartOfColumn(x1);
            final int x2Index = getX2Index(x2, lineObject, x1Index);

            final boolean rowLineWrap = mTerminalBuffer.getLineWrap(row);

            final char[] line = lineObject.mText;
            final int lastPrintingCharIndex = getLastPrintingCharIndex(x2, x1Index, x2Index, line, rowLineWrap);
            checkAndAppendLineText(builder, x1Index, line, lastPrintingCharIndex);

            final boolean lineFillsWidth = lastPrintingCharIndex == x2Index - 1;
            checkAndAppendNewLine(builder, row, selY2, joinBackLines, joinFullLines, rowLineWrap, lineFillsWidth);
        }
        return builder.toString();
    }

    private int getX2(int selX2, int selY2, int row) {
        if (row == selY2) {
            int x2 = selX2 + 1;
            if (x2 < mTerminalBuffer.mColumns) return x2;
        }
        return mTerminalBuffer.mColumns;
    }

    private int getX2Index(int x2, TerminalRow lineObject, int x1Index) {
        int x2Index = (x2 < mTerminalBuffer.mColumns) ? lineObject.findStartOfColumn(x2) : lineObject.getSpaceUsed();
        if (x2Index == x1Index) {
            // Selected the start of a wide character.
            x2Index = lineObject.findStartOfColumn(x2 + 1);
        }
        return x2Index;
    }

    private int getLastPrintingCharIndex(int x2, int x1Index, int x2Index, char[] line, boolean rowLineWrap) {
        if (rowLineWrap && x2 == mTerminalBuffer.mColumns){
            // If the line was wrapped, we shouldn't lose trailing space:
            return x2Index - 1;
        }
        return findLastPrintingCharIndex(line, x1Index, x2Index);
    }

    private int findLastPrintingCharIndex(char[] line, int x1Index, int x2Index) {
        int lastPrintingCharIndex = -1;
        for (int i = x1Index; i < x2Index; ++i) {
            char c = line[i];
            if (c != ' ') lastPrintingCharIndex = i;
        }
        return lastPrintingCharIndex;
    }

    private void checkAndAppendLineText(StringBuilder builder, int x1Index, char[] line, int lastPrintingCharIndex) {
        final int len = lastPrintingCharIndex - x1Index + 1;
        if (lastPrintingCharIndex != -1 && len > 0) builder.append(line, x1Index, len);
    }

    private void checkAndAppendNewLine(StringBuilder builder, int row, int selY2, boolean joinBackLines, boolean joinFullLines, boolean rowLineWrap, boolean lineFillsWidth) {
        boolean isBackLineRowNotWrapped = !(joinBackLines && rowLineWrap);
        boolean isFullLineNotFillsWidth = !(joinFullLines && lineFillsWidth);
        boolean isRowBeforeSelY2 = row < selY2 && row < mTerminalBuffer.mScreenRows - 1;
        if (isBackLineRowNotWrapped && isFullLineNotFillsWidth && isRowBeforeSelY2) builder.append('\n');
    }

    public String getWordAtLocation(int x, int y) {
        // Set y1 and y2 to the lines where the wrapped line starts and ends.
        // I.e. if a line that is wrapped to 3 lines starts at line 4, and this
        // is called with y=5, then y1 would be set to 4 and y2 would be set to 6.
        int y1 = lineWrapStarts(y);
        int y2 = lineWrapEnds(y);

        // Get the text for the whole wrapped line
        String text = getSelectedText(0, y1, mTerminalBuffer.mColumns, y2, true, true);
        // The index of x in text
        int textOffset = (y - y1) * mTerminalBuffer.mColumns + x;

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
        while (y1 > 0 && !getSelectedText(0, y1 - 1, mTerminalBuffer.mColumns, y, true, true).contains("\n")) {
            y1--;
        }
        return y1;
    }

    private int lineWrapEnds(int y) {
        int y2 = y;
        while (y2 < mTerminalBuffer.mScreenRows && !getSelectedText(0, y, mTerminalBuffer.mColumns, y2 + 1, true, true).contains("\n")) {
            y2++;
        }
        return y2;
    }

}
