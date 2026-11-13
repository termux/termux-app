package com.termux.view;

import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.os.Build;

import com.termux.terminal.TerminalBuffer;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalRow;
import com.termux.terminal.TextStyle;
import com.termux.terminal.WcWidth;

/**
 * Renders a {@link TerminalEmulator} into a {@link Canvas}.
 *
 * <p>Pure-LTR / ASCII rows bypass {@link BidiLayout} and cell boxing entirely for native
 * zero-allocation scrolling and frame throughput. Rows containing RTL scripts or inherited RTL
 * wrapped paragraphs use {@link BidiLayout}, keep spaces aligned to the monospace cell grid,
 * cap RTL horizontal expansion to preserve glyph proportions, and keep cursive joining unbroken
 * across mid-word color, selection, and cursor transitions.
 */
public final class TerminalRenderer {

    /** Maximum horizontal stretch factor for longer RTL phrases so glyphs remain balanced. */
    static final float MAX_RTL_HORIZONTAL_STRETCH = 1.48f;
    /** Uniform proportional scale factor for RTL scripts so Arabic matches Latin visual height cleanly. */
    static final float RTL_TEXT_SIZE_SCALE = 1.15f;

    static final int ALIGN_LEFT = -1;
    static final int ALIGN_CENTER = 0;
    static final int ALIGN_RIGHT = 1;

    final int mTextSize;
    final Typeface mTypeface;
    private final Paint mTextPaint = new Paint();

    /** Width of a single monospaced character, measured as {@code measureText("X")}. */
    final float mFontWidth;

    /** {@link Paint#getFontSpacing()} rounded up to the nearest pixel. */
    final int mFontLineSpacing;

    /** {@link Paint#ascent()} rounded up to the nearest pixel. */
    private final int mFontAscent;

    /** {@link #mFontLineSpacing} + {@link #mFontAscent}. */
    final int mFontLineSpacingAndAscent;

    /** Pre-computed advance widths for plain ASCII codepoints. */
    private final float[] asciiMeasures = new float[127];

    /** Scratch buffers reused across frames to avoid per-frame GC allocations. */
    private final char[] mCellMeasureBuffer = new char[64];
    private BidiLayout.LogicalCell[] mRunCells = new BidiLayout.LogicalCell[256];
    private char[] mRunCharBuffer = new char[1024];
    private int[] mCellCharStart = new int[256];
    private int[] mCellCharEnd = new int[256];
    private int[] mVisualToRunCell = new int[256];
    private float[] mVisCellLeft = new float[256];
    private float[] mVisCellRight = new float[256];

    public TerminalRenderer(int textSize, Typeface typeface) {
        mTextSize = textSize;
        mTypeface = typeface;

        mTextPaint.setTypeface(typeface);
        mTextPaint.setAntiAlias(true);
        mTextPaint.setTextSize(textSize);

        mFontLineSpacing = (int) Math.ceil(mTextPaint.getFontSpacing());
        mFontAscent = (int) Math.ceil(mTextPaint.ascent());
        mFontLineSpacingAndAscent = mFontLineSpacing + mFontAscent;
        mFontWidth = mTextPaint.measureText("X");

        StringBuilder sb = new StringBuilder(" ");
        for (int i = 0; i < asciiMeasures.length; i++) {
            sb.setCharAt(0, (char) i);
            asciiMeasures[i] = mTextPaint.measureText(sb, 0, 1);
        }
    }

    static float getMaxRtlHorizontalStretch(int runColumns) {
        if (runColumns <= 1) return 1.20f;
        if (runColumns == 2) return 1.26f;
        if (runColumns == 3) return 1.34f;
        if (runColumns < 8) return 1.42f;
        return MAX_RTL_HORIZONTAL_STRETCH;
    }

    /** Computes horizontal X scale factor for an RTL word or phrase within its grid column slot. */
    static float computeRtlRunScaleX(float measuredWidth, int runColumns, float fontWidth) {
        if (measuredWidth <= 0f || runColumns <= 0 || fontWidth <= 0f) return 1.0f;
        float slotWidth = runColumns * fontWidth;
        float rawScale = slotWidth / measuredWidth;
        if (rawScale < 1.0f) return rawScale;
        return Math.min(rawScale, getMaxRtlHorizontalStretch(runColumns));
    }

    /** Computes centered left X coordinate in grid pixels for drawing an RTL run inside its slot. */
    static float computeRtlRunLeftX(int startCol, int runColumns, float fontWidth, float measuredWidth, float scaleX) {
        return computeRtlRunLeftX(startCol, runColumns, fontWidth, measuredWidth, scaleX, ALIGN_CENTER);
    }

    /** Computes left X coordinate in grid pixels for drawing an RTL run with neighbor-aware alignment. */
    static float computeRtlRunLeftX(int startCol, int runColumns, float fontWidth,
                                    float measuredWidth, float scaleX, int alignment) {
        float slotLeft = startCol * fontWidth;
        float slotWidth = runColumns * fontWidth;
        float drawnWidth = measuredWidth * scaleX;
        if (drawnWidth >= slotWidth || alignment == ALIGN_LEFT) return slotLeft;
        if (alignment == ALIGN_RIGHT) return slotLeft + (slotWidth - drawnWidth);
        return slotLeft + (slotWidth - drawnWidth) * 0.5f;
    }

    private static boolean isPlainSpaceCell(BidiLayout.LogicalCell cell) {
        return cell == null
                || (cell.codePoint == ' ' && cell.combiningCount == 0 && !cell.hasBitmap && cell.displayWidth == 1);
    }

    private static boolean hasAdjacentTextOnLeft(BidiLayout.LogicalCell[] visualCells, int startCol) {
        if (startCol <= 0) return false;
        if (!isPlainSpaceCell(visualCells[startCol - 1])) return true;
        return startCol >= 2 && !isPlainSpaceCell(visualCells[startCol - 2]);
    }

    private static boolean hasAdjacentTextOnRight(BidiLayout.LogicalCell[] visualCells, int endCol, int columns) {
        if (endCol >= columns) return false;
        if (!isPlainSpaceCell(visualCells[endCol])) return true;
        return endCol + 1 < columns && !isPlainSpaceCell(visualCells[endCol + 1]);
    }

    static int resolveRtlAlignment(BidiLayout.LogicalCell[] visualCells, int startCol, int endCol, int columns) {
        boolean textOnLeft = hasAdjacentTextOnLeft(visualCells, startCol);
        boolean textOnRight = hasAdjacentTextOnRight(visualCells, endCol, columns);
        if (textOnLeft && textOnRight) return ALIGN_CENTER;
        if (textOnRight && startCol > 0 && !textOnLeft) return ALIGN_RIGHT;
        return ALIGN_LEFT;
    }

    /**
     * Resolves whether a single space at {@code vCol} bridges two adjacent shaped RTL words
     * into one continuous HarfBuzz phrase while leaving multi-space indentation/tables on the grid.
     */
    static boolean canBridgeSpaceIntoRtlRun(BidiLayout.LogicalCell[] visualCells, int columns,
                                            int vCol, BidiLayout.LogicalCell lastRtlCell) {
        if (lastRtlCell == null || vCol + 1 >= columns) return false;
        BidiLayout.LogicalCell curr = visualCells[vCol];
        if (curr.hasBitmap || curr.codePoint != ' ' || curr.combiningCount != 0 || curr.displayWidth != 1) {
            return false;
        }
        BidiLayout.LogicalCell next = visualCells[vCol + 1];
        if (!isShapedRtlCell(next)) return false;
        return Math.abs(curr.originalColumn - lastRtlCell.originalColumn) == 1
                && Math.abs(next.originalColumn - curr.originalColumn) == 1;
    }

    /**
     * Resolves whether a cell should use the shaped RTL rendering path. Spaces without combining
     * marks and width-2 glyphs remain on the grid-exact LTR path so inter-word alignment holds
     * and emojis/CJK keep full 2-column geometry.
     */
    static boolean isShapedRtlCell(BidiLayout.LogicalCell cell) {
        if (!cell.isRtl || cell.hasBitmap || cell.displayWidth != 1) return false;
        return !(cell.codePoint == ' ' && cell.combiningCount == 0);
    }

    /** Renders the terminal into {@code canvas}, starting at {@code topRow} with an optional selection range. */
    public final void render(TerminalEmulator mEmulator, Canvas canvas, int topRow,
                             int selectionY1, int selectionY2, int selectionX1, int selectionX2) {
        final boolean reverseVideo = mEmulator.isReverseVideo();
        final int endRow = topRow + mEmulator.mRows;
        final int columns = mEmulator.mColumns;
        final int cursorCol = mEmulator.getCursorCol();
        final int cursorRow = mEmulator.getCursorRow();
        final boolean cursorVisible = mEmulator.shouldCursorBeVisible();
        final TerminalBuffer screen = mEmulator.getScreen();
        final int[] palette = mEmulator.mColors.mCurrentColors;
        final int cursorShape = mEmulator.getCursorStyle();

        if (reverseVideo) {
            canvas.drawColor(palette[TextStyle.COLOR_INDEX_FOREGROUND], PorterDuff.Mode.SRC);
        }

        byte paragraphDir = BidiLayout.resolveInheritedDirection(screen, topRow, columns);
        float heightOffset = mFontLineSpacingAndAscent;

        for (int row = topRow; row < endRow; row++) {
            heightOffset += mFontLineSpacing;

            final int cursorX = (row == cursorRow && cursorVisible) ? cursorCol : -1;
            int selx1 = -1;
            int selx2 = -1;
            if (row >= selectionY1 && row <= selectionY2 && selectionX1 >= 0 && selectionX2 >= 0) {
                if (selectionY1 == selectionY2) {
                    selx1 = Math.min(selectionX1, selectionX2);
                    selx2 = Math.max(selectionX1, selectionX2);
                } else if (row == selectionY1) {
                    selx1 = selectionX1;
                    selx2 = columns - 1;
                } else if (row == selectionY2) {
                    selx1 = 0;
                    selx2 = selectionX2;
                } else {
                    selx1 = 0;
                    selx2 = columns - 1;
                }
            }

            TerminalRow lineObject = screen.allocateFullLineIfNecessary(screen.externalToInternalRow(row));
            if (lineObject == null) {
                paragraphDir = screen.getLineWrap(row) ? BidiLayout.DIR_LTR : BidiLayout.DIR_DEFAULT;
                continue;
            }

            if (!lineObject.mMayContainRtlOrBidi && paragraphDir != BidiLayout.DIR_RTL) {
                renderPureLtrRow(lineObject, screen, canvas, palette, columns,
                        cursorX, selx1, selx2, cursorShape, reverseVideo, heightOffset);
                paragraphDir = screen.getLineWrap(row) ? BidiLayout.DIR_LTR : BidiLayout.DIR_DEFAULT;
                continue;
            }

            BidiLayout layout = BidiLayout.build(lineObject, columns, cursorCol, row == cursorRow && cursorVisible,
                    selectionY1, selectionY2, selectionX1, selectionX2, row, true, paragraphDir);

            if (layout.isIdentity) {
                renderPureLtrRow(lineObject, screen, canvas, palette, columns,
                        cursorX, selx1, selx2, cursorShape, reverseVideo, heightOffset);
            } else {
                renderBidiRow(layout, screen, canvas, palette, columns,
                        cursorShape, reverseVideo, heightOffset);
            }

            paragraphDir = screen.getLineWrap(row) ? layout.resolvedBaseDirection : BidiLayout.DIR_DEFAULT;
        }
    }

    /** Zero-allocation fast path matching upstream Termux performance for pure LTR rows. */
    private void renderPureLtrRow(TerminalRow lineObject, TerminalBuffer screen, Canvas canvas, int[] palette,
                                  int columns, int cursorX, int selx1, int selx2,
                                  int cursorShape, boolean reverseVideo, float heightOffset) {
        final char[] line = lineObject.mText;
        final int charsUsedInLine = lineObject.getSpaceUsed();

        long lastRunStyle = 0;
        boolean lastRunInsideCursor = false;
        boolean lastRunInsideSelection = false;
        int lastRunStartColumn = -1;
        int lastRunStartCharIndex = 0;
        boolean lastRunFontWidthMismatch = false;
        int currentCharIndex = 0;
        float measuredWidthForRun = 0.f;

        for (int column = 0; column < columns; ) {
            final long style = lineObject.getStyle(column);
            if (TextStyle.isTerminalBitmap(style)) {
                if (lastRunStartColumn >= 0 && column > lastRunStartColumn) {
                    int cursorColor = lastRunInsideCursor ? palette[TextStyle.COLOR_INDEX_CURSOR] : 0;
                    boolean invertCursor = lastRunInsideCursor && cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK;
                    drawTextRun(canvas, line, palette, heightOffset, lastRunStartColumn, column - lastRunStartColumn,
                            lastRunStartCharIndex, currentCharIndex - lastRunStartCharIndex, measuredWidthForRun,
                            cursorColor, cursorShape, lastRunStyle,
                            reverseVideo || invertCursor || lastRunInsideSelection, false);
                    lastRunStartColumn = -1;
                    measuredWidthForRun = 0.f;
                }
                Bitmap bm = screen.getSixelBitmap(style);
                drawSixelBitmap(bm, screen.getSixelRect(style), canvas, column, heightOffset);
                int charIdx = currentCharIndex;
                if (charIdx < charsUsedInLine) {
                    char c = line[charIdx++];
                    int cp = Character.isHighSurrogate(c) && charIdx < charsUsedInLine
                            ? Character.toCodePoint(c, line[charIdx++]) : c;
                    int w = Math.max(1, WcWidth.width(cp));
                    while (charIdx < charsUsedInLine && WcWidth.width(line, charIdx) <= 0) {
                        charIdx += Character.isHighSurrogate(line[charIdx]) ? 2 : 1;
                    }
                    currentCharIndex = charIdx;
                    column += w;
                } else {
                    column++;
                }
                continue;
            }

            final char charAtIndex = line[currentCharIndex];
            final boolean highSurrogate = Character.isHighSurrogate(charAtIndex);
            final int charsForCodePoint = highSurrogate ? 2 : 1;
            final int codePoint = highSurrogate && currentCharIndex + 1 < charsUsedInLine
                    ? Character.toCodePoint(charAtIndex, line[currentCharIndex + 1]) : charAtIndex;
            final int codePointWcWidth = WcWidth.width(codePoint);
            final boolean insideCursor = (column == cursorX || (codePointWcWidth == 2 && column == cursorX - 1));
            final boolean insideSelection = column >= selx1 && column <= selx2;

            int nextCharIndex = currentCharIndex + charsForCodePoint;
            int combiningChars = 0;
            while (nextCharIndex < charsUsedInLine && WcWidth.width(line, nextCharIndex) <= 0) {
                int combLen = Character.isHighSurrogate(line[nextCharIndex]) ? 2 : 1;
                nextCharIndex += combLen;
                combiningChars += combLen;
            }

            final float measuredCodePointWidth = (codePoint < asciiMeasures.length && combiningChars == 0)
                    ? asciiMeasures[codePoint]
                    : mTextPaint.measureText(line, currentCharIndex, charsForCodePoint + combiningChars);
            final boolean fontWidthMismatch = Math.abs(measuredCodePointWidth / mFontWidth - codePointWcWidth) > 0.01;

            if (lastRunStartColumn < 0
                    || style != lastRunStyle
                    || insideCursor != lastRunInsideCursor
                    || insideSelection != lastRunInsideSelection
                    || fontWidthMismatch
                    || lastRunFontWidthMismatch) {
                if (lastRunStartColumn >= 0) {
                    final int columnWidthSinceLastRun = column - lastRunStartColumn;
                    final int charsSinceLastRun = currentCharIndex - lastRunStartCharIndex;
                    int cursorColor = lastRunInsideCursor ? palette[TextStyle.COLOR_INDEX_CURSOR] : 0;
                    boolean invertCursor = lastRunInsideCursor && cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK;
                    drawTextRun(canvas, line, palette, heightOffset, lastRunStartColumn, columnWidthSinceLastRun,
                            lastRunStartCharIndex, charsSinceLastRun, measuredWidthForRun,
                            cursorColor, cursorShape, lastRunStyle,
                            reverseVideo || invertCursor || lastRunInsideSelection, false);
                }
                measuredWidthForRun = 0.0f;
                lastRunStyle = style;
                lastRunInsideCursor = insideCursor;
                lastRunInsideSelection = insideSelection;
                lastRunStartColumn = column;
                lastRunStartCharIndex = currentCharIndex;
                lastRunFontWidthMismatch = fontWidthMismatch;
            }
            measuredWidthForRun += measuredCodePointWidth;
            currentCharIndex += charsForCodePoint + combiningChars;
            column += codePointWcWidth;
        }

        if (lastRunStartColumn >= 0 && columns > lastRunStartColumn) {
            final int columnWidthSinceLastRun = columns - lastRunStartColumn;
            final int charsSinceLastRun = currentCharIndex - lastRunStartCharIndex;
            int cursorColor = lastRunInsideCursor ? palette[TextStyle.COLOR_INDEX_CURSOR] : 0;
            boolean invertCursor = lastRunInsideCursor && cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK;
            drawTextRun(canvas, line, palette, heightOffset, lastRunStartColumn, columnWidthSinceLastRun,
                    lastRunStartCharIndex, charsSinceLastRun, measuredWidthForRun,
                    cursorColor, cursorShape, lastRunStyle,
                    reverseVideo || invertCursor || lastRunInsideSelection, false);
        }
    }

    /** Renders a BiDi-reordered row, preserving Arabic cursive shaping and cursor/selection parity. */
    private void renderBidiRow(BidiLayout layout, TerminalBuffer screen, Canvas canvas, int[] palette,
                               int columns, int cursorShape, boolean reverseVideo, float heightOffset) {
        final BidiLayout.LogicalCell[] visualCells = layout.visualCells;

        long lastRunStyle = 0;
        boolean lastRunInsideCursor = false;
        boolean lastRunInsideSelection = false;
        boolean lastRunIsRtl = false;
        boolean lastRunFontWidthMismatch = false;
        int lastRunStartColumn = -1;
        BidiLayout.LogicalCell lastRtlCell = null;

        int vCol = 0;
        while (vCol < columns) {
            final BidiLayout.LogicalCell cell = visualCells[vCol];

            if (cell.hasBitmap) {
                if (lastRunStartColumn >= 0 && vCol > lastRunStartColumn) {
                    flushRun(canvas, visualCells, palette, heightOffset,
                            lastRunStartColumn, vCol, lastRunStyle,
                            lastRunInsideCursor, lastRunInsideSelection, lastRunIsRtl,
                            cursorShape, reverseVideo);
                }
                Bitmap bm = screen.getSixelBitmap(cell.style);
                drawSixelBitmap(bm, screen.getSixelRect(cell.style), canvas, vCol, heightOffset);
                lastRunStyle = 0;
                lastRunInsideCursor = false;
                lastRunInsideSelection = false;
                lastRunIsRtl = false;
                lastRunFontWidthMismatch = false;
                lastRunStartColumn = -1;
                lastRtlCell = null;
                vCol += Math.max(1, cell.displayWidth);
                continue;
            }

            if (cell.displayWidth == 0 && cell.codePoint == 0) {
                vCol++;
                continue;
            }

            final int codePoint = cell.codePoint;
            final int codePointWcWidth = cell.displayWidth;
            final boolean isShapedRtl = isShapedRtlCell(cell)
                    || (lastRunStartColumn >= 0 && lastRunIsRtl
                    && canBridgeSpaceIntoRtlRun(visualCells, columns, vCol, lastRtlCell));
            final long style = cell.style;
            boolean insideSelection = cell.insideSelection;
            if (!insideSelection && codePointWcWidth == 2 && vCol + 1 < columns && visualCells[vCol + 1].insideSelection) {
                insideSelection = true;
            }
            boolean insideCursor = cell.insideCursor;
            if (!insideCursor && codePointWcWidth == 2 && vCol + 1 < columns && visualCells[vCol + 1].insideCursor) {
                insideCursor = true;
            }

            boolean fontWidthMismatch = false;
            if (!isShapedRtl) {
                final float measuredCodePointWidth;
                if (codePoint < asciiMeasures.length && cell.combiningCount == 0) {
                    measuredCodePointWidth = asciiMeasures[codePoint];
                } else {
                    int tl = encodeCellToBuffer(cell, mCellMeasureBuffer, 0);
                    measuredCodePointWidth = mTextPaint.measureText(mCellMeasureBuffer, 0, tl);
                }
                fontWidthMismatch = codePointWcWidth > 0
                        && Math.abs(measuredCodePointWidth / mFontWidth - codePointWcWidth) > 0.01;
            }

            final boolean splitRun;
            if (lastRunStartColumn < 0) {
                splitRun = true;
            } else if (isShapedRtl != lastRunIsRtl) {
                splitRun = true;
            } else if (isShapedRtl) {
                // Keep connected RTL words unbroken across color/selection/cursor changes;
                // only split if neighboring visual cells come from non-adjacent logical slices.
                splitRun = lastRtlCell != null && Math.abs(cell.originalColumn - lastRtlCell.originalColumn) > 2;
            } else {
                splitRun = (style != lastRunStyle
                        || insideCursor != lastRunInsideCursor
                        || insideSelection != lastRunInsideSelection
                        || fontWidthMismatch
                        || lastRunFontWidthMismatch);
            }

            if (splitRun) {
                if (lastRunStartColumn >= 0 && vCol > lastRunStartColumn) {
                    flushRun(canvas, visualCells, palette, heightOffset,
                            lastRunStartColumn, vCol, lastRunStyle,
                            lastRunInsideCursor, lastRunInsideSelection, lastRunIsRtl,
                            cursorShape, reverseVideo);
                }
                lastRunStyle = style;
                lastRunInsideCursor = insideCursor;
                lastRunInsideSelection = insideSelection;
                lastRunIsRtl = isShapedRtl;
                lastRunFontWidthMismatch = fontWidthMismatch;
                lastRunStartColumn = vCol;
            }

            lastRtlCell = isShapedRtl ? cell : null;
            vCol += Math.max(1, codePointWcWidth);
        }

        if (lastRunStartColumn >= 0 && columns > lastRunStartColumn) {
            flushRun(canvas, visualCells, palette, heightOffset,
                    lastRunStartColumn, columns, lastRunStyle,
                    lastRunInsideCursor, lastRunInsideSelection, lastRunIsRtl,
                    cursorShape, reverseVideo);
        }
    }

    private void drawSixelBitmap(Bitmap bitmap, Rect srcRect, Canvas canvas, int column, float heightOffset) {
        if (bitmap == null || bitmap.isRecycled()) return;
        float left = column * mFontWidth;
        float top = heightOffset - mFontLineSpacing;
        RectF dst = new RectF(left, top, left + mFontWidth, heightOffset);
        canvas.drawBitmap(bitmap, srcRect, dst, null);
    }

    private int encodeCellToBuffer(BidiLayout.LogicalCell cell, char[] dst, int offset) {
        int written = 0;
        if (cell.codePoint != 0) {
            written += Character.toChars(cell.codePoint, dst, offset + written);
        }
        if (cell.combiningChars != null) {
            for (int k = 0; k < cell.combiningCount; k++) {
                written += Character.toChars(cell.combiningChars[k], dst, offset + written);
            }
        }
        return written;
    }

    private void flushRun(Canvas canvas, BidiLayout.LogicalCell[] visualCells,
                          int[] palette, float heightOffset,
                          int startCol, int endCol, long style,
                          boolean insideCursor, boolean insideSelection, boolean isRtl,
                          int cursorShape, boolean reverseVideo) {
        if (startCol < 0 || endCol <= startCol) return;
        int runColumns = endCol - startCol;

        if (mRunCells.length < runColumns) {
            mRunCells = new BidiLayout.LogicalCell[Math.max(runColumns, mRunCells.length * 2)];
        }
        int count = 0;
        int neededChars = 0;
        for (int c = startCol; c < endCol; c++) {
            BidiLayout.LogicalCell cell = visualCells[c];
            if (cell.displayWidth == 0 && cell.codePoint == 0) continue;
            mRunCells[count++] = cell;
            if (cell.codePoint != 0) neededChars += Character.charCount(cell.codePoint);
            if (cell.combiningChars != null) {
                for (int k = 0; k < cell.combiningCount; k++) {
                    neededChars += Character.charCount(cell.combiningChars[k]);
                }
            }
        }
        if (count == 0) return;

        if (mRunCharBuffer.length < neededChars) {
            mRunCharBuffer = new char[Math.max(neededChars, mRunCharBuffer.length * 2)];
        }

        if (isRtl) {
            flushRtlWordRun(canvas, visualCells, palette, heightOffset,
                    startCol, endCol, runColumns, count, cursorShape, reverseVideo);
            return;
        }

        int used = 0;
        for (int i = 0; i < count; i++) {
            used += encodeCellToBuffer(mRunCells[i], mRunCharBuffer, used);
        }

        int cursorColor = insideCursor ? palette[TextStyle.COLOR_INDEX_CURSOR] : 0;
        boolean invertCursor = insideCursor && cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK;

        final float measuredWidth;
        if (lastRunNeedsMeasure(mRunCells, count)) {
            float total = 0f;
            for (int i = 0; i < count; i++) {
                BidiLayout.LogicalCell rc = mRunCells[i];
                if (rc.codePoint != 0) {
                    int tl = encodeCellToBuffer(rc, mCellMeasureBuffer, 0);
                    total += mTextPaint.measureText(mCellMeasureBuffer, 0, tl);
                } else {
                    total += mFontWidth;
                }
            }
            measuredWidth = total;
        } else {
            measuredWidth = runColumns * mFontWidth;
        }

        drawTextRun(canvas, mRunCharBuffer, palette, heightOffset,
                startCol, runColumns, 0, used, measuredWidth,
                cursorColor, cursorShape, style,
                reverseVideo || invertCursor || insideSelection, false);
    }

    private boolean lastRunNeedsMeasure(BidiLayout.LogicalCell[] cells, int count) {
        for (int i = 0; i < count; i++) {
            BidiLayout.LogicalCell rc = cells[i];
            if (rc.combiningCount > 0 || rc.codePoint >= asciiMeasures.length) return true;
            if (Math.abs(asciiMeasures[rc.codePoint] / mFontWidth - rc.displayWidth) > 0.01) return true;
        }
        return false;
    }

    /**
     * Renders an RTL word or bridged multi-word phrase using native cursive shaping, proportional
     * size balance, neighbor-aware placement, and HarfBuzz-aligned per-segment horizontal clipping.
     */
    private void flushRtlWordRun(Canvas canvas, BidiLayout.LogicalCell[] visualCells,
                                 int[] palette, float heightOffset,
                                 int startCol, int endCol, int runColumns, int count,
                                 int cursorShape, boolean reverseVideo) {
        int neededMeta = Math.max(count, runColumns);
        if (mCellCharStart.length < neededMeta) {
            int newCap = Math.max(neededMeta, mCellCharStart.length * 2);
            mCellCharStart = new int[newCap];
            mCellCharEnd = new int[newCap];
            mVisualToRunCell = new int[newCap];
            mVisCellLeft = new float[newCap];
            mVisCellRight = new float[newCap];
        }

        // Reorder mRunCells into logical column order without heap allocation.
        if (count > 1) {
            boolean strictlyDescending = true;
            for (int i = 1; i < count; i++) {
                if (mRunCells[i].originalColumn >= mRunCells[i - 1].originalColumn) {
                    strictlyDescending = false;
                    break;
                }
            }
            if (strictlyDescending) {
                for (int lo = 0, hi = count - 1; lo < hi; lo++, hi--) {
                    BidiLayout.LogicalCell tmp = mRunCells[lo];
                    mRunCells[lo] = mRunCells[hi];
                    mRunCells[hi] = tmp;
                }
            } else {
                for (int i = 1; i < count; i++) {
                    BidiLayout.LogicalCell key = mRunCells[i];
                    int j = i - 1;
                    while (j >= 0 && mRunCells[j].originalColumn > key.originalColumn) {
                        mRunCells[j + 1] = mRunCells[j];
                        j--;
                    }
                    mRunCells[j + 1] = key;
                }
            }
        }

        int used = 0;
        for (int i = 0; i < count; i++) {
            mCellCharStart[i] = used;
            used += encodeCellToBuffer(mRunCells[i], mRunCharBuffer, used);
            mCellCharEnd[i] = used;
        }
        if (used == 0) return;

        for (int i = 0; i < runColumns; i++) {
            BidiLayout.LogicalCell target = visualCells[startCol + i];
            int revIdx = count - 1 - i;
            if (revIdx >= 0 && revIdx < count && mRunCells[revIdx] == target) {
                mVisualToRunCell[i] = revIdx;
            } else {
                int found = -1;
                for (int r = 0; r < count; r++) {
                    if (mRunCells[r] == target) {
                        found = r;
                        break;
                    }
                }
                mVisualToRunCell[i] = found;
            }
        }

        final Typeface originalTypeface = mTextPaint.getTypeface();
        final float originalTextSize = mTextPaint.getTextSize();
        mTextPaint.setTypeface(Typeface.DEFAULT);
        if (originalTextSize > 0f) {
            mTextPaint.setTextSize(originalTextSize * RTL_TEXT_SIZE_SCALE);
        }
        try {
            final float top = heightOffset - mFontLineSpacingAndAscent + mFontAscent;
            final float bottom = heightOffset;
            final float clipTop = top - mFontLineSpacing;
            final float clipBottom = bottom + mFontLineSpacing;
            final int defaultBg = palette[TextStyle.COLOR_INDEX_BACKGROUND];
            final int cursorColor = palette[TextStyle.COLOR_INDEX_CURSOR];
            final boolean isBlockCursor = (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK);

            // Check whether all cells share uniform appearance and whether any overlay rects are needed.
            final long firstStyle = visualCells[startCol].style;
            boolean uniformText = true;
            boolean hasOverlay = false;
            boolean refInv = visualCells[startCol].insideSelection ^ (visualCells[startCol].insideCursor && isBlockCursor);
            for (int c = startCol; c < endCol; c++) {
                BidiLayout.LogicalCell cell = visualCells[c];
                if (cell.insideCursor || cell.insideSelection) hasOverlay = true;
                int back = resolveBackColor(TextStyle.decodeBackColor(cell.style), palette);
                if (back != defaultBg || (TextStyle.decodeEffect(cell.style) & TextStyle.CHARACTER_ATTRIBUTE_INVERSE) != 0) {
                    hasOverlay = true;
                }
                boolean cellInv = cell.insideSelection ^ (cell.insideCursor && isBlockCursor);
                if (cell.style != firstStyle || cellInv != refInv) {
                    uniformText = false;
                }
            }
            if (reverseVideo) hasOverlay = true;

            // Measure with dominant bold/italic flags before positioning and segment calculation.
            int firstEffect = TextStyle.decodeEffect(firstStyle);
            mTextPaint.setFakeBoldText((firstEffect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK)) != 0);
            mTextPaint.setTextSkewX((firstEffect & TextStyle.CHARACTER_ATTRIBUTE_ITALIC) != 0 ? -0.35f : 0.f);
            final float measuredWidth = mTextPaint.measureText(mRunCharBuffer, 0, used);

            final float scaleX = computeRtlRunScaleX(measuredWidth, runColumns, mFontWidth);
            final int alignment = resolveRtlAlignment(visualCells, startCol, endCol, visualCells.length);
            final float drawLeft = computeRtlRunLeftX(startCol, runColumns, mFontWidth, measuredWidth, scaleX, alignment);
            final float drawnWidth = measuredWidth * scaleX;
            final float slotLeft = startCol * mFontWidth;
            final float slotRight = endCol * mFontWidth;
            final boolean needScale = Math.abs(scaleX - 1.0f) > 0.005f;
            final float baseline = heightOffset - mFontLineSpacingAndAscent;

            if (hasOverlay || !uniformText) {
                computeVisualCellBounds(runColumns, used, measuredWidth, scaleX, drawLeft, drawnWidth);
            }

            if (hasOverlay) {
                final int totalColumns = visualCells.length;
                for (int i = 0; i < runColumns; i++) {
                    int c = startCol + i;
                    BidiLayout.LogicalCell cell = visualCells[c];
                    long cellStyle = cell.style;
                    int effect = TextStyle.decodeEffect(cellStyle);
                    boolean bold = (effect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK)) != 0;
                    int fore = resolveForeColor(TextStyle.decodeForeColor(cellStyle), bold, palette);
                    int back = resolveBackColor(TextStyle.decodeBackColor(cellStyle), palette);
                    boolean cellInvert = reverseVideo
                            ^ ((effect & TextStyle.CHARACTER_ATTRIBUTE_INVERSE) != 0)
                            ^ cell.insideSelection
                            ^ (cell.insideCursor && isBlockCursor);
                    int resolvedBg = cellInvert ? fore : back;

                    float gLeft = mVisCellLeft[i];
                    float gRight = mVisCellRight[i];
                    float bgLeft = (i == 0 && (back != defaultBg
                            || (startCol > 0 && visualCells[startCol - 1].insideSelection && cell.insideSelection)))
                            ? Math.min(slotLeft, gLeft) : gLeft;
                    float bgRight = (i == runColumns - 1 && (back != defaultBg
                            || (endCol < totalColumns && visualCells[endCol].insideSelection && cell.insideSelection)))
                            ? Math.max(slotRight, gRight) : gRight;

                    if (resolvedBg != defaultBg) {
                        mTextPaint.setColor(resolvedBg);
                        canvas.drawRect(bgLeft, top, bgRight, bottom, mTextPaint);
                    }

                    if (cell.insideCursor) {
                        mTextPaint.setColor(cursorColor);
                        float cursorHeight = mFontLineSpacingAndAscent - mFontAscent;
                        float cursorRight = Math.max(gRight, gLeft + mFontWidth * 0.45f);
                        if (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE) {
                            canvas.drawRect(gLeft, bottom - cursorHeight / 4.f, cursorRight, bottom, mTextPaint);
                        } else if (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR) {
                            canvas.drawRect(gLeft, bottom - cursorHeight, gLeft + mFontWidth / 4.f, bottom, mTextPaint);
                        } else {
                            canvas.drawRect(gLeft, bottom - cursorHeight, cursorRight, bottom, mTextPaint);
                        }
                    }
                }
            }

            if (uniformText) {
                drawRtlSegment(canvas, visualCells[startCol], palette, mRunCharBuffer, used,
                        scaleX, drawLeft, needScale, baseline, cursorShape, reverseVideo, false,
                        0f, 0f, 0f, 0f);
            } else {
                int segStart = startCol;
                while (segStart < endCol) {
                    BidiLayout.LogicalCell startCell = visualCells[segStart];
                    boolean startInv = startCell.insideSelection ^ (startCell.insideCursor && isBlockCursor);
                    int segEnd = segStart + 1;
                    while (segEnd < endCol) {
                        BidiLayout.LogicalCell next = visualCells[segEnd];
                        boolean nextInv = next.insideSelection ^ (next.insideCursor && isBlockCursor);
                        if (next.style != startCell.style || nextInv != startInv) break;
                        segEnd++;
                    }
                    float clipL = (segStart == startCol)
                            ? Math.min(drawLeft, slotLeft) - mFontWidth
                            : mVisCellLeft[segStart - startCol];
                    float clipR = (segEnd == endCol)
                            ? Math.max(drawLeft + drawnWidth, slotRight) + mFontWidth
                            : mVisCellRight[segEnd - 1 - startCol];
                    drawRtlSegment(canvas, startCell, palette, mRunCharBuffer, used,
                            scaleX, drawLeft, needScale, baseline, cursorShape, reverseVideo, true,
                            clipL, clipTop, clipR, clipBottom);
                    segStart = segEnd;
                }
            }
        } finally {
            mTextPaint.setTypeface(originalTypeface);
            mTextPaint.setTextSize(originalTextSize);
        }
    }

    private void computeVisualCellBounds(int runColumns, int used, float measuredWidth,
                                         float scaleX, float drawLeft, float drawnWidth) {
        boolean useRunAdvance = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && measuredWidth > 0f);
        for (int i = 0; i < runColumns; i++) {
            float gLeft;
            float gRight;
            int runIdx = mVisualToRunCell[i];
            if (useRunAdvance && runIdx >= 0) {
                int cStart = mCellCharStart[runIdx];
                int cEnd = mCellCharEnd[runIdx];
                float advStart = (cStart <= 0) ? 0f
                        : mTextPaint.getRunAdvance(mRunCharBuffer, 0, used, 0, used, true, cStart);
                float advEnd = (cEnd >= used) ? measuredWidth
                        : mTextPaint.getRunAdvance(mRunCharBuffer, 0, used, 0, used, true, cEnd);
                float loAdv = Math.min(advStart, advEnd);
                float hiAdv = Math.max(advStart, advEnd);
                gLeft = drawLeft + Math.max(0f, measuredWidth - hiAdv) * scaleX;
                gRight = drawLeft + Math.min(measuredWidth, measuredWidth - loAdv) * scaleX;
            } else {
                gLeft = drawLeft + drawnWidth * ((float) i / runColumns);
                gRight = drawLeft + drawnWidth * ((float) (i + 1) / runColumns);
            }
            mVisCellLeft[i] = gLeft;
            mVisCellRight[i] = gRight;
        }
    }

    private void drawRtlSegment(Canvas canvas, BidiLayout.LogicalCell repCell, int[] palette,
                                char[] text, int used, float scaleX, float drawLeft, boolean needScale,
                                float baseline, int cursorShape, boolean reverseVideo,
                                boolean clip, float clipL, float clipT, float clipR, float clipB) {
        long style = repCell.style;
        int effect = TextStyle.decodeEffect(style);
        if ((effect & TextStyle.CHARACTER_ATTRIBUTE_INVISIBLE) != 0) return;

        boolean bold = (effect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK)) != 0;
        boolean underline = (effect & TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE) != 0;
        boolean italic = (effect & TextStyle.CHARACTER_ATTRIBUTE_ITALIC) != 0;
        boolean strikeThrough = (effect & TextStyle.CHARACTER_ATTRIBUTE_STRIKETHROUGH) != 0;
        boolean dim = (effect & TextStyle.CHARACTER_ATTRIBUTE_DIM) != 0;
        boolean isBlockCursor = (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK);

        int fore = resolveForeColor(TextStyle.decodeForeColor(style), bold, palette);
        int back = resolveBackColor(TextStyle.decodeBackColor(style), palette);
        boolean cellInvert = reverseVideo
                ^ ((effect & TextStyle.CHARACTER_ATTRIBUTE_INVERSE) != 0)
                ^ repCell.insideSelection
                ^ (repCell.insideCursor && isBlockCursor);
        int textColor = cellInvert ? back : fore;

        if (dim) {
            int red = (0xFF & (textColor >> 16)) * 2 / 3;
            int green = (0xFF & (textColor >> 8)) * 2 / 3;
            int blue = (0xFF & textColor) * 2 / 3;
            textColor = 0xFF000000 | (red << 16) | (green << 8) | blue;
        }

        mTextPaint.setFakeBoldText(bold);
        mTextPaint.setUnderlineText(underline);
        mTextPaint.setTextSkewX(italic ? -0.35f : 0.f);
        mTextPaint.setStrikeThruText(strikeThrough);
        mTextPaint.setColor(textColor);

        boolean needSave = clip || needScale;
        if (needSave) canvas.save();
        if (clip) canvas.clipRect(clipL, clipT, clipR, clipB);
        float x = drawLeft;
        if (needScale) {
            canvas.scale(scaleX, 1.0f);
            x /= scaleX;
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            canvas.drawTextRun(text, 0, used, 0, used, x, baseline, true, mTextPaint);
        } else {
            canvas.drawText(text, 0, used, x, baseline, mTextPaint);
        }

        if (needSave) canvas.restore();
    }

    private static int resolveForeColor(int foreColor, boolean bold, int[] palette) {
        if ((foreColor & 0xff000000) != 0xff000000) {
            if (bold && foreColor >= 0 && foreColor < 8) foreColor += 8;
            return palette[foreColor];
        }
        return foreColor;
    }

    private static int resolveBackColor(int backColor, int[] palette) {
        if ((backColor & 0xff000000) != 0xff000000) {
            return palette[backColor];
        }
        return backColor;
    }

    private void drawTextRun(Canvas canvas, char[] text, int[] palette, float y,
                             int startColumn, int runWidthColumns,
                             int startCharIndex, int runWidthChars,
                             float mes, int cursor, int cursorStyle,
                             long textStyle, boolean reverseVideo, boolean isRtl) {
        int foreColor = TextStyle.decodeForeColor(textStyle);
        final int effect = TextStyle.decodeEffect(textStyle);
        int backColor = TextStyle.decodeBackColor(textStyle);
        final boolean bold = (effect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK)) != 0;
        final boolean underline = (effect & TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE) != 0;
        final boolean italic = (effect & TextStyle.CHARACTER_ATTRIBUTE_ITALIC) != 0;
        final boolean strikeThrough = (effect & TextStyle.CHARACTER_ATTRIBUTE_STRIKETHROUGH) != 0;
        final boolean dim = (effect & TextStyle.CHARACTER_ATTRIBUTE_DIM) != 0;

        foreColor = resolveForeColor(foreColor, bold, palette);
        backColor = resolveBackColor(backColor, palette);

        final boolean reverseVideoHere = reverseVideo ^ (effect & TextStyle.CHARACTER_ATTRIBUTE_INVERSE) != 0;
        if (reverseVideoHere) {
            int tmp = foreColor;
            foreColor = backColor;
            backColor = tmp;
        }

        float left = startColumn * mFontWidth;
        float right = left + runWidthColumns * mFontWidth;

        mes = mes / mFontWidth;
        boolean savedMatrix = false;
        if (Math.abs(mes - runWidthColumns) > 0.01 && mes > 0) {
            canvas.save();
            canvas.scale(runWidthColumns / mes, 1.f);
            left *= mes / runWidthColumns;
            right *= mes / runWidthColumns;
            savedMatrix = true;
        }

        if (backColor != palette[TextStyle.COLOR_INDEX_BACKGROUND]) {
            mTextPaint.setColor(backColor);
            canvas.drawRect(left, y - mFontLineSpacingAndAscent + mFontAscent, right, y, mTextPaint);
        }

        if (cursor != 0) {
            mTextPaint.setColor(cursor);
            float cursorHeight = mFontLineSpacingAndAscent - mFontAscent;
            if (cursorStyle == TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE) cursorHeight /= 4.f;
            else if (cursorStyle == TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR) right -= (((right - left) * 3) / 4.f);
            canvas.drawRect(left, y - cursorHeight, right, y, mTextPaint);
        }

        if ((effect & TextStyle.CHARACTER_ATTRIBUTE_INVISIBLE) == 0) {
            if (dim) {
                int red   = (0xFF & (foreColor >> 16)) * 2 / 3;
                int green = (0xFF & (foreColor >>  8)) * 2 / 3;
                int blue  = (0xFF &  foreColor)        * 2 / 3;
                foreColor = 0xFF000000 | (red << 16) | (green << 8) | blue;
            }

            mTextPaint.setFakeBoldText(bold);
            mTextPaint.setUnderlineText(underline);
            mTextPaint.setTextSkewX(italic ? -0.35f : 0.f);
            mTextPaint.setStrikeThruText(strikeThrough);
            mTextPaint.setColor(foreColor);

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                canvas.drawTextRun(text, startCharIndex, runWidthChars,
                        startCharIndex, runWidthChars,
                        left, y - mFontLineSpacingAndAscent, isRtl, mTextPaint);
            } else {
                canvas.drawText(text, startCharIndex, runWidthChars, left, y - mFontLineSpacingAndAscent, mTextPaint);
            }
        }

        if (savedMatrix) canvas.restore();
    }

    public float getFontWidth() {
        return mFontWidth;
    }

    public int getFontLineSpacing() {
        return mFontLineSpacing;
    }

    public int translateVisualToLogicalColumn(TerminalEmulator mEmulator, int visualCol, int row) {
        if (visualCol < 0) return 0;
        if (visualCol >= mEmulator.mColumns) return mEmulator.mColumns;

        TerminalBuffer screen = mEmulator.getScreen();
        if (row < -screen.getActiveTranscriptRows() || row >= mEmulator.mRows) return visualCol;

        int internalRow = screen.externalToInternalRow(row);
        if (internalRow < 0 || internalRow >= screen.getActiveRows()) return visualCol;

        TerminalRow rowObject = screen.allocateFullLineIfNecessary(internalRow);
        if (rowObject == null) return visualCol;

        byte inheritedDir = BidiLayout.resolveInheritedDirection(screen, row, mEmulator.mColumns);
        if (!rowObject.mMayContainRtlOrBidi && inheritedDir != BidiLayout.DIR_RTL) {
            return visualCol;
        }

        if (rowObject.mCachedBidiLayout instanceof BidiLayout) {
            BidiLayout cached = (BidiLayout) rowObject.mCachedBidiLayout;
            if (cached.inputParagraphDirection == inheritedDir && cached.visualToLogical.length == mEmulator.mColumns) {
                return cached.visualToLogical[visualCol];
            }
        }

        BidiLayout layout = BidiLayout.build(rowObject, mEmulator.mColumns, -1, false,
                -1, -1, -1, -1, row, false, inheritedDir);
        return layout.visualToLogical[visualCol];
    }

    public int translateLogicalToVisualColumn(TerminalEmulator mEmulator, int logicalCol, int row) {
        if (logicalCol < 0) return 0;
        if (logicalCol >= mEmulator.mColumns) return mEmulator.mColumns;

        TerminalBuffer screen = mEmulator.getScreen();
        if (row < -screen.getActiveTranscriptRows() || row >= mEmulator.mRows) return logicalCol;

        int internalRow = screen.externalToInternalRow(row);
        if (internalRow < 0 || internalRow >= screen.getActiveRows()) return logicalCol;

        TerminalRow rowObject = screen.allocateFullLineIfNecessary(internalRow);
        if (rowObject == null) return logicalCol;

        byte inheritedDir = BidiLayout.resolveInheritedDirection(screen, row, mEmulator.mColumns);
        if (!rowObject.mMayContainRtlOrBidi && inheritedDir != BidiLayout.DIR_RTL) {
            return logicalCol;
        }

        if (rowObject.mCachedBidiLayout instanceof BidiLayout) {
            BidiLayout cached = (BidiLayout) rowObject.mCachedBidiLayout;
            if (cached.inputParagraphDirection == inheritedDir && cached.logicalToVisual.length == mEmulator.mColumns) {
                return cached.logicalToVisual[logicalCol];
            }
        }

        BidiLayout layout = BidiLayout.build(rowObject, mEmulator.mColumns, -1, false,
                -1, -1, -1, -1, row, false, inheritedDir);
        return layout.logicalToVisual[logicalCol];
    }
}
