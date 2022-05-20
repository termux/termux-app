package com.termux.terminal;

public final class UpdateOldBuffer implements ResizeBuffer {
    TerminalBuffer mTerminalBuffer;

    public UpdateOldBuffer(TerminalBuffer terminalBuffer) {
        mTerminalBuffer = terminalBuffer;
    }

    public void resize(int newColumns, int newRows, int newTotalRows, int[] cursor, long currentStyle, boolean isAltScreen) {
        final TerminalRow[] oldLines = mTerminalBuffer.mLines;

        final int oldActiveTranscriptRows = mTerminalBuffer.mActiveTranscriptRows;
        final int oldScreenFirstRow = mTerminalBuffer.mScreenFirstRow;
        final int oldScreenRows = mTerminalBuffer.mScreenRows;
        final int oldTotalRows = mTerminalBuffer.mTotalRows;

        final Cursor oldCursor = new Cursor(cursor[1], cursor[0]);

        mTerminalBuffer.mLines = new TerminalRow[newTotalRows];
        for (int i = 0; i < newTotalRows; i++)
            mTerminalBuffer.mLines[i] = new TerminalRow(newColumns, currentStyle);

        mTerminalBuffer.mTotalRows = newTotalRows;
        mTerminalBuffer.mScreenRows = newRows;
        mTerminalBuffer.mActiveTranscriptRows = mTerminalBuffer.mScreenFirstRow = 0;
        mTerminalBuffer.mColumns = newColumns;

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
            mTerminalBuffer.setChar(outputColumn, currentOutputExternal.getRow(), codePoint, styleAtCol);

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
        final boolean isLineLong = currentOutputExternal.getColumn() + displayWidth > mTerminalBuffer.mColumns;
        if (isLineLong) {
            mTerminalBuffer.setLineWrap(currentOutputExternal.getRow());
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
            if (currentOutputExternal.getRow() == mTerminalBuffer.mScreenRows - 1) {
                mTerminalBuffer.scrollDownOneLine(0, mTerminalBuffer.mScreenRows, currentStyle);
            } else {
                currentOutputExternal.addToRow(1);
            }
            currentOutputExternal.setColumn(0);
        }
    }

    private void setNewCursorRow(long currentStyle, Cursor newCursor, Cursor currentOutputExternal) {
        if (currentOutputExternal.getRow() == mTerminalBuffer.mScreenRows - 1) {
            if (newCursor.getRow() != -1) newCursor.addToRow(-1);
            mTerminalBuffer.scrollDownOneLine(0, mTerminalBuffer.mScreenRows, currentStyle);
        } else {
            currentOutputExternal.addToRow(1);
        }
        currentOutputExternal.setColumn(0);
    }

}
