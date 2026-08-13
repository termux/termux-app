package com.termux.view;

import com.termux.terminal.TerminalRow;
import com.termux.terminal.WcWidth;
import java.text.Bidi;

public final class BidiLayout {
    
    public static class LogicalCell {
        public int codePoint;
        public int displayWidth;
        public long style;
        public boolean insideCursor;
        public boolean insideSelection;
        public int originalColumn;
        public boolean isRtl;
        public int[] combiningChars = null;
        public int combiningCount = 0;

        public void addCombining(int cp) {
            if (combiningChars == null) {
                combiningChars = new int[4];
            } else if (combiningCount == combiningChars.length) {
                int[] newArr = new int[combiningChars.length * 2];
                System.arraycopy(combiningChars, 0, newArr, 0, combiningChars.length);
                combiningChars = newArr;
            }
            combiningChars[combiningCount++] = cp;
        }
    }

    public final LogicalCell[] visualCells;
    public final int[] logicalToVisual;
    public final int[] visualToLogical;

    public BidiLayout(LogicalCell[] visualCells, int[] logicalToVisual, int[] visualToLogical) {
        this.visualCells = visualCells;
        this.logicalToVisual = logicalToVisual;
        this.visualToLogical = visualToLogical;
    }

    /**
     * Builds or retrieves a cached visual layout for a terminal row.
     * Maps logical terminal grid columns to visual characters and manages RTL/LTR reordering.
     * Uses a fast-path caching strategy to update cursor and selection states in-place,
     * avoiding Bidi calculations and array allocations on frame refreshes.
     */
    public static BidiLayout build(TerminalRow rowObject, int columns, int cursorCol, boolean cursorVisible,
                                   int selectionY1, int selectionY2, int selectionX1, int selectionX2, int row,
                                   boolean isForRendering) {
        // 1. Fast-path: Check cached layout
        if (rowObject.mCachedBidiLayout instanceof BidiLayout) {
            BidiLayout cached = (BidiLayout) rowObject.mCachedBidiLayout;
            if (cached.visualCells.length == columns) {
                if (isForRendering) {
                    // Update cursor and selection flags in-place (extremely fast, zero-allocation)
                    int vSelStart = -1;
                    int vSelEnd = -1;
                    boolean hasSel = false;
                    if (row >= selectionY1 && row <= selectionY2) {
                        hasSel = true;
                        if (selectionY1 == selectionY2) {
                            int v1 = cached.logicalToVisual[Math.min(columns - 1, Math.max(0, selectionX1))];
                            int v2 = cached.logicalToVisual[Math.min(columns - 1, Math.max(0, selectionX2))];
                            vSelStart = Math.min(v1, v2);
                            vSelEnd = Math.max(v1, v2);
                        } else if (row == selectionY1) {
                            vSelStart = cached.logicalToVisual[Math.min(columns - 1, Math.max(0, selectionX1))];
                            vSelEnd = columns - 1;
                        } else if (row == selectionY2) {
                            vSelStart = 0;
                            vSelEnd = cached.logicalToVisual[Math.min(columns - 1, Math.max(0, selectionX2))];
                        } else {
                            vSelStart = 0;
                            vSelEnd = columns - 1;
                        }
                    }

                    for (int i = 0; i < columns; i++) {
                        LogicalCell cell = cached.visualCells[i];
                        int lCol = cached.visualToLogical[i];
                        cell.insideCursor = (lCol == cursorCol && cursorVisible);
                        cell.insideSelection = hasSel && (i >= vSelStart && i <= vSelEnd);
                    }
                }
                return cached;
            }
        }

        // 2. Cache miss: Build a new layout
        LogicalCell[] logicalCells = new LogicalCell[columns];
        for (int i = 0; i < columns; i++) {
            logicalCells[i] = new LogicalCell();
            logicalCells[i].codePoint = ' ';
            logicalCells[i].displayWidth = 1;
            logicalCells[i].style = rowObject.getStyle(i);
            logicalCells[i].insideCursor = (i == cursorCol && cursorVisible);
            logicalCells[i].insideSelection = false;
            logicalCells[i].originalColumn = i;
            logicalCells[i].isRtl = false;
        }

        char[] line = rowObject.mText;
        int charsUsedInLine = rowObject.getSpaceUsed();
        int currentCharIndex = 0;

        for (int column = 0; column < columns; ) {
            if (currentCharIndex >= charsUsedInLine) {
                break;
            }
            char c = line[currentCharIndex];
            boolean isHigh = Character.isHighSurrogate(c);
            int charsCount = isHigh ? 2 : 1;

            int codePoint = isHigh ? Character.toCodePoint(c, line[currentCharIndex + 1]) : c;
            int w = WcWidth.width(codePoint);
            if (w <= 0) w = 1;

            if (column < columns) {
                LogicalCell cell = logicalCells[column];
                cell.codePoint = codePoint;
                cell.displayWidth = w;

                if (w == 2 && column + 1 < columns) {
                    logicalCells[column + 1].codePoint = 0;
                    logicalCells[column + 1].displayWidth = 0;
                    logicalCells[column + 1].insideCursor = (column + 1 == cursorCol && cursorVisible);
                }
            }

            currentCharIndex += charsCount;

            while (currentCharIndex < charsUsedInLine) {
                char nextChar = line[currentCharIndex];
                boolean nextIsHigh = Character.isHighSurrogate(nextChar);
                int nextCp = nextIsHigh ? Character.toCodePoint(nextChar, line[currentCharIndex + 1]) : nextChar;
                if (WcWidth.width(nextCp) <= 0) {
                    if (column < columns) {
                        logicalCells[column].addCombining(nextCp);
                    }
                    currentCharIndex += nextIsHigh ? 2 : 1;
                } else {
                    break;
                }
            }
            column += w;
        }

        // 3. Trailing Space Protection: Scan for the last active column
        int activeLength = 0;
        for (int i = columns - 1; i >= 0; i--) {
            if (logicalCells[i].codePoint != ' ' && logicalCells[i].codePoint != 0) {
                activeLength = i + 1;
                break;
            }
        }

        int[] logicalToVisual = new int[columns];
        int[] visualToLogical = new int[columns];
        LogicalCell[] visualCells = new LogicalCell[columns];

        if (activeLength <= 0) {
            // Whole line is blank, identity mapping
            for (int i = 0; i < columns; i++) {
                logicalToVisual[i] = i;
                visualToLogical[i] = i;
                visualCells[i] = logicalCells[i];
            }
            BidiLayout newLayout = new BidiLayout(visualCells, logicalToVisual, visualToLogical);
            rowObject.mLogicalToVisual = logicalToVisual;
            rowObject.mVisualToLogical = visualToLogical;
            rowObject.mCachedBidiLayout = newLayout;
            return newLayout;
        }

        // Convert to Char array for Bidi (active portion only)
        char[] bidiChars = new char[activeLength];
        for (int i = 0; i < activeLength; i++) {
            int cp = logicalCells[i].codePoint;
            if (cp == 0) {
                bidiChars[i] = ' ';
            } else if (Character.isSupplementaryCodePoint(cp)) {
                bidiChars[i] = ' '; // standard LTR placeholder
            } else {
                bidiChars[i] = (char) cp;
            }
        }

        Bidi bidi = new Bidi(bidiChars, 0, null, 0, activeLength, Bidi.DIRECTION_DEFAULT_LEFT_TO_RIGHT);

        if (bidi.isLeftToRight()) {
            for (int i = 0; i < columns; i++) {
                logicalToVisual[i] = i;
                visualToLogical[i] = i;
                visualCells[i] = logicalCells[i];
            }
        } else {
            byte[] levels = new byte[activeLength];
            Integer[] activeVisualToLogical = new Integer[activeLength];
            for (int i = 0; i < activeLength; i++) {
                levels[i] = (byte) bidi.getLevelAt(i);
                activeVisualToLogical[i] = i;
                logicalCells[i].isRtl = (levels[i] % 2 != 0);
            }
            Bidi.reorderVisually(levels, 0, activeVisualToLogical, 0, activeLength);

            // Map active reordered portion
            for (int i = 0; i < activeLength; i++) {
                visualToLogical[i] = activeVisualToLogical[i];
                logicalToVisual[visualToLogical[i]] = i;
                visualCells[i] = logicalCells[visualToLogical[i]];
            }

            // Map trailing portion as identity (forces spaces to remain LTR on the right)
            for (int i = activeLength; i < columns; i++) {
                logicalToVisual[i] = i;
                visualToLogical[i] = i;
                visualCells[i] = logicalCells[i];
            }
        }

        // Populate visual selection
        int vSelStart = -1;
        int vSelEnd = -1;
        boolean hasSel = false;
        if (row >= selectionY1 && row <= selectionY2) {
            hasSel = true;
            if (selectionY1 == selectionY2) {
                int v1 = logicalToVisual[Math.min(columns - 1, Math.max(0, selectionX1))];
                int v2 = logicalToVisual[Math.min(columns - 1, Math.max(0, selectionX2))];
                vSelStart = Math.min(v1, v2);
                vSelEnd = Math.max(v1, v2);
            } else if (row == selectionY1) {
                vSelStart = logicalToVisual[Math.min(columns - 1, Math.max(0, selectionX1))];
                vSelEnd = columns - 1;
            } else if (row == selectionY2) {
                vSelStart = 0;
                vSelEnd = logicalToVisual[Math.min(columns - 1, Math.max(0, selectionX2))];
            } else {
                vSelStart = 0;
                vSelEnd = columns - 1;
            }
        }

        for (int i = 0; i < columns; i++) {
            LogicalCell cell = visualCells[i];
            int lCol = visualToLogical[i];
            cell.insideCursor = (lCol == cursorCol && cursorVisible);
            cell.insideSelection = hasSel && (i >= vSelStart && i <= vSelEnd);
        }

        BidiLayout newLayout = new BidiLayout(visualCells, logicalToVisual, visualToLogical);
        rowObject.mLogicalToVisual = logicalToVisual;
        rowObject.mVisualToLogical = visualToLogical;
        rowObject.mCachedBidiLayout = newLayout;
        return newLayout;
    }
}
