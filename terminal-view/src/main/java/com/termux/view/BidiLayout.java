package com.termux.view;

import android.graphics.Typeface;

import com.termux.terminal.TerminalBuffer;
import com.termux.terminal.TerminalRow;
import com.termux.terminal.TextStyle;
import com.termux.terminal.WcWidth;

import java.text.Bidi;
import java.util.Arrays;

/**
 * Maps logical terminal grid cells to visual display columns using Unicode Bidirectional
 * Algorithm (UAX #9) reordering, shell-prompt isolation, and TUI box-border awareness.
 *
 * <p>Stores visual selection bounds in visual column space consistently across both cache-hit
 * and cache-miss paths, avoids heap boxing on reordering, and tracks paragraph base direction
 * so wrapped continuation rows preserve the paragraph's RTL/LTR base direction.
 */
public final class BidiLayout {

    /** Infer base direction from prompt-isolated content using first-strong UAX #9. */
    public static final byte DIR_DEFAULT = 0;
    /** Explicit Left-to-Right paragraph base direction. */
    public static final byte DIR_LTR = 1;
    /** Explicit Right-to-Left paragraph base direction. */
    public static final byte DIR_RTL = 2;
    /** Bitmask flag indicating an unwrapped paragraph root began with a left-anchored prompt. */
    static final int INHERITED_PROMPT_FLAG = 0x10;
    static final int INHERITED_DIR_MASK = 0x03;

    /** Cached visual pixel geometry per row so rendering, hit-testing, selection, and cursor agree to the sub-pixel. */
    public static final class PixelGeometry {
        public final float[] visLeft;
        public final float[] visRight;
        public final boolean continuousFlow;
        public final boolean rightAligned;
        public final float contentLeft;
        public final float contentRight;
        public final float scaleX;
        public final float rtlSpaceWidth;
        final int textSize;
        final Typeface typeface;
        final float fontWidth;
        final boolean altBuffer;

        PixelGeometry(float[] visLeft, float[] visRight, boolean continuousFlow, boolean rightAligned,
                      float contentLeft, float contentRight, float scaleX, float rtlSpaceWidth,
                      int textSize, Typeface typeface, float fontWidth, boolean altBuffer) {
            this.visLeft = visLeft;
            this.visRight = visRight;
            this.continuousFlow = continuousFlow;
            this.rightAligned = rightAligned;
            this.contentLeft = contentLeft;
            this.contentRight = contentRight;
            this.scaleX = scaleX;
            this.rtlSpaceWidth = rtlSpaceWidth;
            this.textSize = textSize;
            this.typeface = typeface;
            this.fontWidth = fontWidth;
            this.altBuffer = altBuffer;
        }

        boolean matches(int textSize, Typeface typeface, float fontWidth, boolean altBuffer) {
            return this.textSize == textSize
                    && this.typeface == typeface
                    && this.fontWidth == fontWidth
                    && this.altBuffer == altBuffer;
        }

        public int findVisualColAtX(float x, boolean clampToActive, int activeLength) {
            int columns = visLeft.length;
            if (columns <= 0) return 0;
            if (!continuousFlow) {
                int col = (fontWidth > 0f) ? (int) (x / fontWidth) : 0;
                return Math.max(0, Math.min(columns - 1, col));
            }
            if (activeLength <= 0) return 0;
            if (rightAligned) {
                if (x >= contentLeft) {
                    for (int v = 0; v < activeLength; v++) {
                        if (x < visRight[v]) return v;
                    }
                    return activeLength - 1;
                }
                if (clampToActive) return 0;
                for (int v = activeLength; v < columns; v++) {
                    if (x >= visLeft[v]) return v;
                }
                return columns - 1;
            } else {
                if (x < visLeft[0]) return 0;
                int limit = clampToActive ? activeLength : columns;
                for (int v = 0; v < limit; v++) {
                    if (x < visRight[v]) return v;
                }
                return limit - 1;
            }
        }
    }

    private static final int MAX_COMBINING = 15;
    private static final int MAX_WRAP_WALK = 32;
    private static final int PROMPT_SCAN_LIMIT = 64;

    private static final ThreadLocal<Scratch> SCRATCH = new ThreadLocal<Scratch>() {
        @Override
        protected Scratch initialValue() {
            return new Scratch();
        }
    };

    private static final class Scratch {
        char[] bidiChars = new char[256];
        byte[] levels = new byte[256];
        int[] activeVisToLog = new int[256];

        void ensureCapacity(int size) {
            if (bidiChars.length < size) {
                int newCap = Math.max(size, bidiChars.length * 2);
                bidiChars = new char[newCap];
                levels = new byte[newCap];
                activeVisToLog = new int[newCap];
            }
        }
    }

    /** Visual representation of a single terminal grid column. */
    public static final class LogicalCell {
        public int codePoint;
        public int[] combiningChars;
        public int combiningCount;
        public long style;
        public boolean insideCursor;
        public boolean insideSelection;
        public int displayWidth;
        public boolean isRtl;
        public int originalColumn;
        public boolean hasBitmap;

        void reset(long cellStyle, int column) {
            codePoint = ' ';
            combiningCount = 0;
            style = cellStyle;
            insideCursor = false;
            insideSelection = false;
            displayWidth = 1;
            isRtl = false;
            originalColumn = column;
            hasBitmap = TextStyle.isTerminalBitmap(cellStyle);
        }

        void addCombiningChar(int cp) {
            if (combiningChars == null) {
                combiningChars = new int[4];
            } else if (combiningCount >= combiningChars.length) {
                if (combiningCount >= MAX_COMBINING) return;
                combiningChars = Arrays.copyOf(combiningChars, Math.min(MAX_COMBINING, combiningChars.length * 2));
            }
            if (combiningCount < MAX_COMBINING) {
                combiningChars[combiningCount++] = cp;
            }
        }
    }

    public final LogicalCell[] visualCells;
    public final int[] logicalToVisual;
    public final int[] visualToLogical;
    public final byte inputParagraphDirection;
    public final boolean inputParagraphStartsWithPrompt;
    public final byte resolvedBaseDirection;
    public final boolean isIdentity;
    public final int trimmedActiveLength;
    public final int promptPrefixLength;
    public final boolean hasBoxBorder;
    public final boolean hasBitmap;
    public final boolean hasColumnarMultiSpace;
    public final boolean paragraphStartsWithPrompt;

    // Tracks last rendered dynamic cursor/selection state to skip per-cell loops on static frames.
    private int mRenderedCursorCol = -2;
    private int mRenderedVSelStart = -2;
    private int mRenderedVSelEnd = -2;
    public PixelGeometry mPixelGeometry;

    private BidiLayout(LogicalCell[] visualCells, int[] logicalToVisual, int[] visualToLogical,
                       byte inputParagraphDirection, boolean inputParagraphStartsWithPrompt,
                       byte resolvedBaseDirection, boolean isIdentity,
                       int trimmedActiveLength, int promptPrefixLength,
                       boolean hasBoxBorder, boolean hasBitmap, boolean hasColumnarMultiSpace,
                       boolean paragraphStartsWithPrompt) {
        this.visualCells = visualCells;
        this.logicalToVisual = logicalToVisual;
        this.visualToLogical = visualToLogical;
        this.inputParagraphDirection = inputParagraphDirection;
        this.inputParagraphStartsWithPrompt = inputParagraphStartsWithPrompt;
        this.resolvedBaseDirection = resolvedBaseDirection;
        this.isIdentity = isIdentity;
        this.trimmedActiveLength = trimmedActiveLength;
        this.promptPrefixLength = promptPrefixLength;
        this.hasBoxBorder = hasBoxBorder;
        this.hasBitmap = hasBitmap;
        this.hasColumnarMultiSpace = hasColumnarMultiSpace;
        this.paragraphStartsWithPrompt = paragraphStartsWithPrompt;
    }

    /** Backward-compatible entry point defaulting to {@link #DIR_DEFAULT}. */
    public static BidiLayout build(TerminalRow rowObject, int columns, int cursorCol, boolean cursorVisible,
                                   int selectionY1, int selectionY2, int selectionX1, int selectionX2,
                                   int row, boolean isForRendering) {
        return build(rowObject, columns, cursorCol, cursorVisible,
                selectionY1, selectionY2, selectionX1, selectionX2, row, isForRendering, DIR_DEFAULT, false);
    }

    /** Backward-compatible entry point preserving 11-argument callers. */
    public static BidiLayout build(TerminalRow rowObject, int columns, int cursorCol, boolean cursorVisible,
                                   int selectionY1, int selectionY2, int selectionX1, int selectionX2,
                                   int row, boolean isForRendering, byte paragraphDirection) {
        return build(rowObject, columns, cursorCol, cursorVisible,
                selectionY1, selectionY2, selectionX1, selectionX2, row, isForRendering, paragraphDirection, false);
    }

    /**
     * Builds or retrieves a cached visual layout for {@code rowObject}.
     * Selection coordinates {@code selectionX1}/{@code selectionX2} are always in visual column space.
     */
    public static BidiLayout build(TerminalRow rowObject, int columns, int cursorCol, boolean cursorVisible,
                                   int selectionY1, int selectionY2, int selectionX1, int selectionX2,
                                   int row, boolean isForRendering, byte paragraphDirection,
                                   boolean inheritedStartsWithPrompt) {
        int effectiveCursorCol = cursorVisible ? cursorCol : -1;
        int vSelStart = -1;
        int vSelEnd = -1;
        if (isForRendering && selectionY1 >= Integer.MIN_VALUE && row >= selectionY1 && row <= selectionY2 && selectionX1 >= 0 && selectionX2 >= 0) {
            if (selectionY1 == selectionY2) {
                vSelStart = Math.min(selectionX1, selectionX2);
                vSelEnd = Math.max(selectionX1, selectionX2);
            } else if (row == selectionY1) {
                vSelStart = selectionX1;
                vSelEnd = columns - 1;
            } else if (row == selectionY2) {
                vSelStart = 0;
                vSelEnd = selectionX2;
            } else {
                vSelStart = 0;
                vSelEnd = columns - 1;
            }
        }

        // 1. Fast-path: reuse cached layout on matching columns and paragraph context.
        if (rowObject.mCachedBidiLayout instanceof BidiLayout) {
            BidiLayout cached = (BidiLayout) rowObject.mCachedBidiLayout;
            if (cached.visualCells.length == columns
                    && cached.inputParagraphDirection == paragraphDirection
                    && cached.inputParagraphStartsWithPrompt == inheritedStartsWithPrompt) {
                if (isForRendering) {
                    if (cached.mRenderedCursorCol == effectiveCursorCol
                            && cached.mRenderedVSelStart == vSelStart
                            && cached.mRenderedVSelEnd == vSelEnd) {
                        return cached;
                    }
                    boolean hasSel = (vSelStart >= 0);
                    for (int i = 0; i < columns; i++) {
                        LogicalCell cell = cached.visualCells[i];
                        int lCol = cached.visualToLogical[i];
                        cell.style = rowObject.getStyle(lCol);
                        cell.insideCursor = (lCol == effectiveCursorCol);
                        cell.insideSelection = hasSel && (i >= vSelStart && i <= vSelEnd);
                    }
                    cached.mRenderedCursorCol = effectiveCursorCol;
                    cached.mRenderedVSelStart = vSelStart;
                    cached.mRenderedVSelEnd = vSelEnd;
                }
                return cached;
            }
        }

        // 2. Cache miss: extract logical cells from TerminalRow.
        LogicalCell[] logicalCells = new LogicalCell[columns];
        for (int i = 0; i < columns; i++) {
            LogicalCell c = new LogicalCell();
            c.reset(rowObject.getStyle(i), i);
            logicalCells[i] = c;
        }

        char[] rowChars = rowObject.mText;
        int rowCharsLen = rowObject.getSpaceUsed();
        int charIdx = 0;
        int col = 0;

        while (col < columns && charIdx < rowCharsLen) {
            int cp = Character.codePointAt(rowChars, charIdx);
            charIdx += Character.charCount(cp);

            int w = WcWidth.width(cp);
            if (w <= 0) {
                // Guard against orphan combining char at start of cell without stalling column cursor.
                w = 1;
            }

            LogicalCell cell = logicalCells[col];
            cell.codePoint = cp;
            cell.displayWidth = (w == 2 && col + 1 >= columns) ? 1 : w;

            // Consume all zero-width modifiers/diacritics attached to this cell (up to MAX_COMBINING).
            while (charIdx < rowCharsLen) {
                int nextCp = Character.codePointAt(rowChars, charIdx);
                if (WcWidth.width(nextCp) == 0) {
                    cell.addCombiningChar(nextCp);
                    charIdx += Character.charCount(nextCp);
                } else {
                    break;
                }
            }

            if (cell.displayWidth == 2) {
                LogicalCell cont = logicalCells[col + 1];
                cont.codePoint = 0;
                cont.displayWidth = 0;
                col += 2;
            } else {
                col += 1;
            }
        }

        // Trim trailing plain blank spaces so trailing padding stays on visual right.
        int activeLength = columns;
        while (activeLength > 0) {
            LogicalCell c = logicalCells[activeLength - 1];
            if (c.codePoint == ' ' && c.combiningCount == 0 && !c.hasBitmap) {
                activeLength--;
            } else {
                break;
            }
        }

        LogicalCell[] visualCells = new LogicalCell[columns];
        int[] logicalToVisual = new int[columns];
        int[] visualToLogical = new int[columns];

        byte resolvedDir = (paragraphDirection == DIR_RTL) ? DIR_RTL : DIR_LTR;
        boolean allIdentity = true;
        int promptEnd = 0;
        boolean hasBoxBorder = false;
        boolean hasBitmap = false;
        boolean hasColumnarMultiSpace = false;

        if (activeLength == 0 || (!rowObject.mMayContainRtlOrBidi && paragraphDirection != DIR_RTL)) {
            for (int i = 0; i < columns; i++) {
                logicalToVisual[i] = i;
                visualToLogical[i] = i;
                visualCells[i] = logicalCells[i];
                if (logicalCells[i].hasBitmap) hasBitmap = true;
            }
            resolvedDir = (paragraphDirection == DIR_RTL) ? DIR_RTL : DIR_LTR;
        } else {
            Scratch s = SCRATCH.get();
            s.ensureCapacity(activeLength);
            char[] bidiChars = s.bidiChars;
            byte[] levels = s.levels;
            int[] activeVisToLog = s.activeVisToLog;

            boolean hasVerticalBoxBorder = false;
            boolean hasAnyRtl = false;
            for (int i = 0; i < activeLength; i++) {
                LogicalCell lc = logicalCells[i];
                int cp = lc.codePoint;
                char mapped;
                if (lc.hasBitmap) {
                    hasBitmap = true;
                    mapped = 'A'; // Never reverse pixel tiles of a Sixel/inline bitmap.
                } else if (cp == 0) {
                    mapped = (i > 0) ? bidiChars[i - 1] : ' ';
                } else if (cp <= 0xFFFF) {
                    mapped = (char) cp;
                    if (cp >= 0x2500 && cp <= 0x257F) {
                        hasBoxBorder = true;
                        if (cp == 0x2502 || cp == 0x2503 || cp == 0x2551) {
                            hasVerticalBoxBorder = true;
                        }
                    }
                } else {
                    mapped = mapSupplementaryToDirectionChar(cp);
                }
                bidiChars[i] = mapped;
                activeVisToLog[i] = i;
                levels[i] = 0;
                if (!hasAnyRtl && isRtlCharacterDirection(mapped)) {
                    hasAnyRtl = true;
                }
            }

            if (paragraphDirection != DIR_DEFAULT) {
                int bidiFlag = (paragraphDirection == DIR_RTL)
                        ? Bidi.DIRECTION_RIGHT_TO_LEFT
                        : Bidi.DIRECTION_LEFT_TO_RIGHT;
                reorderSlice(bidiChars, 0, activeLength, bidiFlag, activeVisToLog, levels, logicalCells);
                resolvedDir = paragraphDirection;
            } else if (hasVerticalBoxBorder && hasAnyRtl) {
                reorderBoxTableCells(logicalCells, bidiChars, activeLength, activeVisToLog, levels);
                resolvedDir = DIR_LTR;
            } else {
                promptEnd = hasAnyRtl ? detectPromptPrefixLength(logicalCells, bidiChars, activeLength) : 0;
                if (promptEnd > 0) {
                    boolean payloadLtr = reorderSlice(bidiChars, promptEnd, activeLength,
                            Bidi.DIRECTION_DEFAULT_LEFT_TO_RIGHT, activeVisToLog, levels, logicalCells);
                    resolvedDir = payloadLtr ? DIR_LTR : DIR_RTL;
                } else {
                    boolean baseLtr = reorderSlice(bidiChars, 0, activeLength,
                            Bidi.DIRECTION_DEFAULT_LEFT_TO_RIGHT, activeVisToLog, levels, logicalCells);
                    resolvedDir = baseLtr ? DIR_LTR : DIR_RTL;
                }
            }

            for (int i = 0; i < activeLength; i++) {
                int lCol = activeVisToLog[i];
                visualToLogical[i] = lCol;
                logicalToVisual[lCol] = i;
                visualCells[i] = logicalCells[lCol];
                if (lCol != i || visualCells[i].isRtl) {
                    allIdentity = false;
                }
            }
            for (int i = activeLength; i < columns; i++) {
                logicalToVisual[i] = i;
                visualToLogical[i] = i;
                visualCells[i] = logicalCells[i];
                if (logicalCells[i].hasBitmap) hasBitmap = true;
            }

            hasColumnarMultiSpace = detectColumnarMultiSpace(visualCells, activeLength, promptEnd);
        }

        if (isForRendering) {
            boolean hasSel = (vSelStart >= 0);
            for (int i = 0; i < columns; i++) {
                LogicalCell cell = visualCells[i];
                int lCol = visualToLogical[i];
                cell.insideCursor = (lCol == effectiveCursorCol);
                cell.insideSelection = hasSel && (i >= vSelStart && i <= vSelEnd);
            }
        }

        boolean effectiveStartsWithPrompt = (paragraphDirection == DIR_DEFAULT)
                ? (promptEnd > 0)
                : inheritedStartsWithPrompt;

        BidiLayout newLayout = new BidiLayout(visualCells, logicalToVisual, visualToLogical,
                paragraphDirection, inheritedStartsWithPrompt,
                resolvedDir, allIdentity, activeLength, promptEnd,
                hasBoxBorder, hasBitmap, hasColumnarMultiSpace, effectiveStartsWithPrompt);
        if (isForRendering) {
            newLayout.mRenderedCursorCol = effectiveCursorCol;
            newLayout.mRenderedVSelStart = vSelStart;
            newLayout.mRenderedVSelEnd = vSelEnd;
        }
        rowObject.mLogicalToVisual = logicalToVisual;
        rowObject.mVisualToLogical = visualToLogical;
        rowObject.mCachedBidiLayout = newLayout;
        return newLayout;
    }

    /**
     * Resolves the inherited paragraph base direction for {@code row} by walking backward through
     * wrapped lines ({@code mLineWrap == true}). Returns {@link #DIR_DEFAULT} if {@code row} starts
     * a new unwrapped paragraph.
     */
    public static byte resolveInheritedDirection(TerminalBuffer screen, int row, int columns) {
        return (byte) (resolveInheritedContext(screen, row, columns) & INHERITED_DIR_MASK);
    }

    /**
     * Resolves both inherited base direction (low 2 bits) and whether the unwrapped paragraph root
     * started with a left-anchored shell prompt ({@link #INHERITED_PROMPT_FLAG}).
     */
    public static int resolveInheritedContext(TerminalBuffer screen, int row, int columns) {
        if (screen == null || columns <= 0) return DIR_DEFAULT;
        int minRow = -screen.getActiveTranscriptRows();
        if (row <= minRow || !screen.getLineWrap(row - 1)) {
            return DIR_DEFAULT;
        }

        int firstWrappedRow = row - 1;
        int steps = 1;
        while (firstWrappedRow > minRow && steps < MAX_WRAP_WALK && screen.getLineWrap(firstWrappedRow - 1)) {
            firstWrappedRow--;
            steps++;
        }

        byte dir = DIR_DEFAULT;
        boolean startsWithPrompt = false;
        for (int r = firstWrappedRow; r < row; r++) {
            int internalRow = screen.externalToInternalRow(r);
            TerminalRow tr = screen.allocateFullLineIfNecessary(internalRow);
            if (tr == null) {
                dir = DIR_LTR;
                startsWithPrompt = false;
                continue;
            }
            if (!tr.mMayContainRtlOrBidi && dir != DIR_RTL) {
                dir = DIR_LTR;
                if (r == firstWrappedRow) startsWithPrompt = false;
            } else {
                BidiLayout l = build(tr, columns, -1, false, -1, -1, -1, -1, r, false, dir, startsWithPrompt);
                dir = l.resolvedBaseDirection;
                startsWithPrompt = l.paragraphStartsWithPrompt;
            }
        }
        byte resolvedDir = (dir == DIR_DEFAULT) ? DIR_LTR : dir;
        return resolvedDir | (startsWithPrompt ? INHERITED_PROMPT_FLAG : 0);
    }

    /** Returns true when visual cells contain >= 3 consecutive plain spaces bounded by non-space tokens (table columns). */
    private static boolean detectColumnarMultiSpace(LogicalCell[] visualCells, int activeLength, int promptEnd) {
        int scanStart = Math.max(0, promptEnd);
        int i = scanStart;
        while (i < activeLength && isPlainBlank(visualCells[i])) {
            i++; // Skip leading indentation.
        }
        int consecutiveSpaces = 0;
        for (; i < activeLength; i++) {
            if (isPlainBlank(visualCells[i])) {
                consecutiveSpaces++;
                if (consecutiveSpaces >= 3) {
                    return true;
                }
            } else {
                consecutiveSpaces = 0;
            }
        }
        return false;
    }

    private static boolean isPlainBlank(LogicalCell cell) {
        return cell == null || (cell.codePoint == ' ' && cell.combiningCount == 0 && !cell.hasBitmap && cell.displayWidth == 1);
    }

    /**
     * Extracts clipboard text matching visual selection bounds across single-line, multi-line,
     * mixed RTL/LTR, wide characters, and wrapped rows.
     */
    public static String extractSelectedText(TerminalBuffer screen, int columns, int screenRows,
                                             int selX1, int selY1, int selX2, int selY2) {
        if (screen == null || columns <= 0 || selY1 > selY2) return "";
        int minRow = -screen.getActiveTranscriptRows();
        if (selY1 < minRow) selY1 = minRow;
        if (selY2 >= screenRows) selY2 = screenRows - 1;
        if (selY1 > selY2) return "";

        StringBuilder result = new StringBuilder();
        int inheritedCtx = resolveInheritedContext(screen, selY1, columns);
        byte paragraphDir = (byte) (inheritedCtx & INHERITED_DIR_MASK);
        boolean paragraphHasPrompt = (inheritedCtx & INHERITED_PROMPT_FLAG) != 0;

        for (int row = selY1; row <= selY2; row++) {
            int vStart;
            int vEnd;
            if (selY1 == selY2) {
                vStart = Math.min(selX1, selX2);
                vEnd = Math.max(selX1, selX2);
            } else if (row == selY1) {
                vStart = selX1;
                vEnd = columns - 1;
            } else if (row == selY2) {
                vStart = 0;
                vEnd = selX2;
            } else {
                vStart = 0;
                vEnd = columns - 1;
            }
            if (vStart < 0) vStart = 0;
            if (vEnd >= columns) vEnd = columns - 1;

            int internalRow = screen.externalToInternalRow(row);
            TerminalRow rowObject = screen.allocateFullLineIfNecessary(internalRow);
            boolean rowLineWrap = screen.getLineWrap(row);

            if (rowObject == null) {
                if (!rowLineWrap && row < selY2 && row < screenRows - 1) result.append('\n');
                paragraphDir = rowLineWrap ? DIR_LTR : DIR_DEFAULT;
                paragraphHasPrompt = rowLineWrap && paragraphHasPrompt;
                continue;
            }

            if (!rowObject.mMayContainRtlOrBidi && paragraphDir != DIR_RTL) {
                appendLtrRowSelection(result, rowObject, columns, vStart, vEnd, rowLineWrap);
                if (!rowLineWrap && row < selY2 && row < screenRows - 1) result.append('\n');
                paragraphDir = rowLineWrap ? DIR_LTR : DIR_DEFAULT;
                if (!rowLineWrap) paragraphHasPrompt = false;
                continue;
            }

            BidiLayout layout = build(rowObject, columns, -1, false, -1, -1, -1, -1, row, false,
                    paragraphDir, paragraphHasPrompt);
            if (layout.isIdentity) {
                appendLtrRowSelection(result, rowObject, columns, vStart, vEnd, rowLineWrap);
            } else {
                appendBidiRowSelection(result, rowObject, layout, columns, vStart, vEnd, rowLineWrap);
            }

            if (!rowLineWrap && row < selY2 && row < screenRows - 1) result.append('\n');
            paragraphDir = rowLineWrap ? layout.resolvedBaseDirection : DIR_DEFAULT;
            paragraphHasPrompt = rowLineWrap && layout.paragraphStartsWithPrompt;
        }
        return result.toString();
    }

    private static void appendLtrRowSelection(StringBuilder out, TerminalRow rowObject, int columns,
                                              int x1, int x2Inclusive, boolean rowLineWrap) {
        int x2 = Math.min(columns, x2Inclusive + 1);
        int x1Index = rowObject.findStartOfColumn(x1);
        int x2Index = (x2 < columns) ? rowObject.findStartOfColumn(x2) : rowObject.getSpaceUsed();
        if (x2Index == x1Index && x2 + 1 <= columns) {
            x2Index = (x2 + 1 < columns) ? rowObject.findStartOfColumn(x2 + 1) : rowObject.getSpaceUsed();
        }
        char[] line = rowObject.mText;
        int lastPrintingCharIndex = -1;
        if (rowLineWrap && x2 == columns) {
            lastPrintingCharIndex = x2Index - 1;
        } else {
            for (int i = x1Index; i < x2Index; i++) {
                if (line[i] != ' ') lastPrintingCharIndex = i;
            }
        }
        int len = lastPrintingCharIndex - x1Index + 1;
        if (lastPrintingCharIndex != -1 && len > 0) {
            out.append(line, x1Index, len);
        }
    }

    private static void appendBidiRowSelection(StringBuilder out, TerminalRow rowObject, BidiLayout layout,
                                               int columns, int vStart, int vEnd, boolean rowLineWrap) {
        boolean[] selectedLogical = new boolean[columns];
        for (int v = vStart; v <= vEnd; v++) {
            int l = layout.visualToLogical[v];
            selectedLogical[l] = true;
            if (l > 0 && rowObject.wideDisplayCharacterStartingAt(l - 1)) {
                selectedLogical[l - 1] = true;
            }
        }

        char[] line = rowObject.mText;
        int spaceUsed = rowObject.getSpaceUsed();
        int rowOutStart = out.length();
        int lastPrintingOutLen = rowOutStart;

        int c = 0;
        while (c < columns) {
            if (!selectedLogical[c]) {
                c++;
                continue;
            }
            if (c > 0 && rowObject.wideDisplayCharacterStartingAt(c - 1) && selectedLogical[c - 1]) {
                // Skip second half of wide char whose lead half was already emitted.
                c++;
                continue;
            }
            int runEndCol = c + 1;
            while (runEndCol < columns && selectedLogical[runEndCol]) {
                runEndCol++;
            }
            int startIdx = rowObject.findStartOfColumn(c);
            int endIdx = (runEndCol < columns) ? rowObject.findStartOfColumn(runEndCol) : spaceUsed;
            for (int i = startIdx; i < endIdx; i++) {
                char ch = line[i];
                out.append(ch);
                if (ch != ' ') {
                    lastPrintingOutLen = out.length();
                }
            }
            c = runEndCol;
        }

        if (!(rowLineWrap && vEnd == columns - 1)) {
            out.setLength(lastPrintingOutLen);
        }
    }

    private static boolean reorderSlice(char[] chars, int start, int end, int bidiDirection,
                                        int[] activeVisToLog, byte[] levels, LogicalCell[] logicalCells) {
        int len = end - start;
        if (len <= 0) return true;

        Bidi bidi = new Bidi(chars, start, null, 0, len, bidiDirection);
        boolean baseIsLtr = bidi.baseIsLeftToRight();

        if (bidi.isLeftToRight()) {
            for (int i = 0; i < len; i++) {
                int idx = start + i;
                levels[idx] = 0;
                activeVisToLog[idx] = idx;
                logicalCells[idx].isRtl = false;
            }
            return baseIsLtr;
        }

        byte minOdd = Byte.MAX_VALUE;
        byte maxLvl = 0;
        for (int i = 0; i < len; i++) {
            int idx = start + i;
            byte lvl = (byte) bidi.getLevelAt(i);
            levels[idx] = lvl;
            activeVisToLog[idx] = idx;
            logicalCells[idx].isRtl = ((lvl & 1) != 0);
            if (lvl > maxLvl) maxLvl = lvl;
            if ((lvl & 1) != 0 && lvl < minOdd) minOdd = lvl;
        }

        // UAX #9 L2: Reverse contiguous sequences >= lvl from highest level down to lowest odd level.
        if (minOdd <= maxLvl) {
            for (byte lvl = maxLvl; lvl >= minOdd; lvl--) {
                int i = 0;
                while (i < len) {
                    if (levels[start + i] >= lvl) {
                        int runEnd = i + 1;
                        while (runEnd < len && levels[start + runEnd] >= lvl) {
                            runEnd++;
                        }
                        int lo = start + i;
                        int hi = start + runEnd - 1;
                        while (lo < hi) {
                            int tmp = activeVisToLog[lo];
                            activeVisToLog[lo] = activeVisToLog[hi];
                            activeVisToLog[hi] = tmp;
                            lo++;
                            hi--;
                        }
                        i = runEnd;
                    } else {
                        i++;
                    }
                }
            }
        }

        // Preserve [lead, continuation] visual LTR pair order for width-2 characters on odd levels.
        for (int i = start + 1; i < end; i++) {
            int leadIdx = activeVisToLog[i];
            if (logicalCells[leadIdx].displayWidth == 2 && activeVisToLog[i - 1] == leadIdx + 1) {
                activeVisToLog[i - 1] = leadIdx;
                activeVisToLog[i] = leadIdx + 1;
            }
        }
        return baseIsLtr;
    }

    private static void reorderBoxTableCells(LogicalCell[] cells, char[] bidiChars, int activeLength,
                                             int[] activeVisToLog, byte[] levels) {
        int segStart = 0;
        for (int i = 0; i <= activeLength; i++) {
            boolean isBorder = (i == activeLength)
                    || (cells[i].codePoint == 0x2502 || cells[i].codePoint == 0x2503 || cells[i].codePoint == 0x2551);
            if (!isBorder) continue;

            if (i > segStart) {
                int innerStart = segStart;
                while (innerStart < i && cells[innerStart].codePoint == ' ' && cells[innerStart].combiningCount == 0) {
                    innerStart++;
                }
                int innerEnd = i;
                while (innerEnd > innerStart && cells[innerEnd - 1].codePoint == ' ' && cells[innerEnd - 1].combiningCount == 0) {
                    innerEnd--;
                }
                if (innerEnd > innerStart) {
                    reorderSlice(bidiChars, innerStart, innerEnd,
                            Bidi.DIRECTION_DEFAULT_LEFT_TO_RIGHT, activeVisToLog, levels, cells);
                }
            }
            if (i < activeLength) {
                levels[i] = 0;
                activeVisToLog[i] = i;
                cells[i].isRtl = false;
            }
            segStart = i + 1;
        }
    }

    /**
     * Detects an unambiguous shell/REPL prompt prefix before the first RTL character so prompts
     * remain anchored at visual left while the command/text after them resolves its own UAX #9 direction.
     */
    private static int detectPromptPrefixLength(LogicalCell[] cells, char[] bidiChars, int activeLength) {
        int limit = Math.min(activeLength, PROMPT_SCAN_LIMIT);
        int firstRtl = -1;
        for (int i = 0; i < limit; i++) {
            if (isRtlCharacterDirection(bidiChars[i])) {
                firstRtl = i;
                break;
            }
        }
        if (firstRtl <= 1) return 0;

        for (int k = firstRtl - 2; k >= 0; k--) {
            int cp = cells[k].codePoint;
            if (cells[k + 1].codePoint != ' ') continue;

            boolean strongPrompt = (cp == '$' || cp == 0x276F || cp == 0x279C || cp == 0x03BB);
            boolean prefixedPrompt = (cp == '#' || cp == '%' || cp == '>') && hasNonSpacePrefix(cells, k);
            if (!strongPrompt && !prefixedPrompt) continue;

            // Avoid splitting prose like "USD$ مرحبا" where '$' is attached to an alphanumeric token.
            if (cp == '$' && k > 0 && Character.isLetterOrDigit(cells[k - 1].codePoint)) {
                continue;
            }
            // Avoid splitting single leading "# مرحبا" or "> مرحبا" Markdown lines; allow ">>> " REPL.
            if (cp == '>' && k > 0) {
                boolean hasLetterOrDigitBefore = false;
                boolean allGreaterThan = true;
                for (int j = 0; j < k; j++) {
                    int p = cells[j].codePoint;
                    if (Character.isLetterOrDigit(p)) hasLetterOrDigitBefore = true;
                    if (p != '>') allGreaterThan = false;
                }
                if (!hasLetterOrDigitBefore && !allGreaterThan) continue;
            }

            int end = k + 2;
            while (end < firstRtl && cells[end].codePoint == ' ') {
                end++;
            }
            return end;
        }
        return 0;
    }

    private static boolean hasNonSpacePrefix(LogicalCell[] cells, int endExclusive) {
        for (int i = 0; i < endExclusive; i++) {
            if (cells[i].codePoint != ' ') return true;
        }
        return false;
    }

    private static boolean isRtlCharacterDirection(char ch) {
        byte dir = Character.getDirectionality(ch);
        return dir == Character.DIRECTIONALITY_RIGHT_TO_LEFT
                || dir == Character.DIRECTIONALITY_RIGHT_TO_LEFT_ARABIC
                || dir == Character.DIRECTIONALITY_RIGHT_TO_LEFT_EMBEDDING
                || dir == Character.DIRECTIONALITY_RIGHT_TO_LEFT_OVERRIDE;
    }

    private static char mapSupplementaryToDirectionChar(int cp) {
        byte dir = Character.getDirectionality(cp);
        if (dir == Character.DIRECTIONALITY_RIGHT_TO_LEFT) return '\u05D0';
        if (dir == Character.DIRECTIONALITY_RIGHT_TO_LEFT_ARABIC) return '\u0627';
        if (dir == Character.DIRECTIONALITY_LEFT_TO_RIGHT) return 'A';
        return '\uFFFD';
    }
}
