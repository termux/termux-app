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
 * zero-allocation scrolling and frame throughput. Main-buffer BiDi prose uses continuous
 * HarfBuzz-measured flow at natural undistorted stroke proportions ({@code scaleX <= 1.0f}),
 * left-anchored at the viewport origin with inherited wrapped-line BiDi ordering. Alternate-buffer TUIs, box-drawing
 * frames, Sixel bitmaps, and columnar tables preserve strict grid alignment.
 */
public final class TerminalRenderer {

    /** Never stretch RTL glyphs horizontally past natural 1.0x stroke proportions. */
    static final float MAX_RTL_HORIZONTAL_STRETCH = 1.0f;
    /** Proportional scale factor for RTL scripts matching monospace Latin visual weight cleanly. */
    static final float RTL_TEXT_SIZE_SCALE = 1.10f;
    /** Compact fallback factor for legacy callers; delimiters shape proportionally via Typeface.DEFAULT. */
    static final float RTL_DELIMITER_ADVANCE_FACTOR = 0.86f;

    static final int ALIGN_LEFT = -1;
    static final int ALIGN_CENTER = 0;
    static final int ALIGN_RIGHT = 1;

    private static final int SEG_RTL = 0;
    private static final int SEG_DELIM = 1;
    private static final int SEG_SPACE = 2;
    private static final int SEG_LTR = 3;
    private static final int SEG_BITMAP = 4;

    final int mTextSize;
    final Typeface mTypeface;
    private final Paint mTextPaint;
    final boolean mUseApi23TextRun;

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
    private int[] mSegKind = new int[32];
    private int[] mSegVisStart = new int[32];
    private int[] mSegVisEnd = new int[32];
    private int[] mSegCharStart = new int[32];
    private int[] mSegCharEnd = new int[32];
    private float[] mSegNatWidth = new float[32];
    private float[] mSegLeft = new float[32];

    public TerminalRenderer(int textSize, Typeface typeface) {
        this(textSize, typeface, null, Build.VERSION.SDK_INT >= Build.VERSION_CODES.M);
    }

    /** Package-private constructor allowing JVM unit tests to supply custom {@link Paint} metrics and API level. */
    TerminalRenderer(int textSize, Typeface typeface, Paint textPaint, boolean useApi23TextRun) {
        mTextPaint = (textPaint != null) ? textPaint : new Paint();
        mUseApi23TextRun = useApi23TextRun;
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
        return MAX_RTL_HORIZONTAL_STRETCH;
    }

    /** Computes horizontal X scale factor for an RTL phrase, compressing overflow without stretching glyphs > 1.0x. */
    static float computeRtlRunScaleX(float measuredWidth, int runColumns, float fontWidth) {
        if (measuredWidth <= 0f || runColumns <= 0 || fontWidth <= 0f) return 1.0f;
        float slotWidth = runColumns * fontWidth;
        float rawScale = slotWidth / measuredWidth;
        return (rawScale < 1.0f) ? Math.max(0.1f, rawScale) : 1.0f;
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

    /** Caps rightward shift for centered phrases so long RTL text never opens big leading gaps. */
    static float computeBalancedRtlLeftX(int startCol, int runColumns, float fontWidth,
                                         float measuredWidth, float scaleX, int alignment) {
        float raw = computeRtlRunLeftX(startCol, runColumns, fontWidth, measuredWidth, scaleX, alignment);
        if (alignment == ALIGN_CENTER) {
            float maxShift = startCol * fontWidth + fontWidth * 0.45f;
            if (raw > maxShift) return maxShift;
        }
        return raw;
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
        if (textOnLeft && textOnRight) {
            boolean flushLeft = startCol > 0 && !isPlainSpaceCell(visualCells[startCol - 1]);
            boolean flushRight = endCol < columns && !isPlainSpaceCell(visualCells[endCol]);
            if (flushLeft && !flushRight) return ALIGN_LEFT;
            if (flushRight && !flushLeft) return ALIGN_RIGHT;
            return ALIGN_CENTER;
        }
        if (textOnRight && startCol == 0) {
            return (endCol < columns && !isPlainSpaceCell(visualCells[endCol])) ? ALIGN_RIGHT : ALIGN_LEFT;
        }
        if (textOnRight && startCol > 0 && !textOnLeft) return ALIGN_RIGHT;
        return ALIGN_LEFT;
    }

    /**
     * Resolves whether a single space at {@code vCol} bridges two adjacent shaped RTL words
     * (or an RTL-hugging paired delimiter) into one continuous phrase while leaving multi-space
     * indentation/tables on the grid.
     */
    static boolean canBridgeSpaceIntoRtlRun(BidiLayout.LogicalCell[] visualCells, int columns,
                                            int vCol, BidiLayout.LogicalCell lastRtlCell) {
        if (lastRtlCell == null || vCol + 1 >= columns) return false;
        BidiLayout.LogicalCell curr = visualCells[vCol];
        if (curr.hasBitmap || curr.codePoint != ' ' || curr.combiningCount != 0 || curr.displayWidth != 1) {
            return false;
        }
        BidiLayout.LogicalCell next = visualCells[vCol + 1];
        boolean nextIsRtlMember = isShapedRtlCell(next) || canJoinRtlAdjacentDelimiter(visualCells, columns, vCol + 1);
        if (!nextIsRtlMember) return false;
        return Math.abs(curr.originalColumn - lastRtlCell.originalColumn) == 1
                && Math.abs(next.originalColumn - curr.originalColumn) == 1;
    }

    /** Returns true for paired ASCII delimiters that should stay crisp in monospace with UAX #9 mirroring. */
    static boolean isMirroredAsciiDelimiter(int codePoint) {
        return codePoint == '(' || codePoint == ')'
                || codePoint == '[' || codePoint == ']'
                || codePoint == '{' || codePoint == '}'
                || codePoint == '<' || codePoint == '>';
    }

    static boolean isMirroredAsciiDelimiterCell(BidiLayout.LogicalCell cell) {
        return cell != null && !cell.hasBitmap && cell.displayWidth == 1
                && cell.combiningCount == 0 && isMirroredAsciiDelimiter(cell.codePoint);
    }

    /**
     * Returns true if a paired ASCII delimiter at {@code vCol} visually hugs a shaped RTL cell
     * directly (or through adjacent delimiters) so it can travel tightly with the RTL phrase
     * without leaving artificial monospace gaps next to parentheses/brackets.
     */
    static boolean canJoinRtlAdjacentDelimiter(BidiLayout.LogicalCell[] visualCells, int columns, int vCol) {
        if (vCol < 0 || vCol >= columns || !isMirroredAsciiDelimiterCell(visualCells[vCol])) {
            return false;
        }
        for (int i = vCol - 1; i >= 0; i--) {
            BidiLayout.LogicalCell c = visualCells[i];
            if (isShapedRtlCell(c)) return true;
            if (!isMirroredAsciiDelimiterCell(c)) break;
        }
        for (int i = vCol + 1; i < columns; i++) {
            BidiLayout.LogicalCell c = visualCells[i];
            if (isShapedRtlCell(c)) return true;
            if (!isMirroredAsciiDelimiterCell(c)) break;
        }
        return false;
    }

    /** Returns the UAX #9 mirrored counterpart for an ASCII paired delimiter. */
    static int getMirroredAsciiChar(int codePoint) {
        switch (codePoint) {
            case '(': return ')';
            case ')': return '(';
            case '[': return ']';
            case ']': return '[';
            case '{': return '}';
            case '}': return '{';
            case '<': return '>';
            case '>': return '<';
            default: return codePoint;
        }
    }

    /** Returns true for invisible BiDi format control characters that must not emit glyphs. */
    static boolean isInvisibleBidiFormatChar(int codePoint) {
        return codePoint == 0x061C
                || codePoint == 0x200E || codePoint == 0x200F
                || (codePoint >= 0x202A && codePoint <= 0x202E)
                || (codePoint >= 0x2066 && codePoint <= 0x2069);
    }

    /**
     * Resolves whether a cell should use the shaped RTL rendering path. Spaces without combining
     * marks, width-2 glyphs, and ASCII paired delimiters remain on the monospace grid path so
     * inter-word alignment and bracket symmetry hold across mixed text.
     */
    static boolean isShapedRtlCell(BidiLayout.LogicalCell cell) {
        if (!cell.isRtl || cell.hasBitmap || cell.displayWidth != 1) return false;
        if (cell.codePoint == ' ' && cell.combiningCount == 0) return false;
        if (cell.combiningCount == 0 && isMirroredAsciiDelimiter(cell.codePoint)) return false;
        return !isInvisibleBidiFormatChar(cell.codePoint);
    }

    /** Returns true if {@code layout} must use strict fixed-column grid placement instead of continuous flow. */
    static boolean usesStrictGrid(BidiLayout layout, TerminalEmulator emulator) {
        boolean altBuffer = (emulator != null && emulator.isAlternateBufferActive());
        return usesStrictGrid(layout, altBuffer);
    }

    static boolean usesStrictGrid(BidiLayout layout, boolean altBuffer) {
        if (altBuffer || layout == null) return true;
        return layout.hasBoxBorder || layout.hasBitmap || layout.hasColumnarMultiSpace;
    }

    /** Keeps terminal rows left-anchored so CLI outputs and prompts never jump to the far right edge. */
    static boolean shouldRightAlignBidi(BidiLayout layout, boolean altBuffer) {
        return false;
    }

    /** Returns true if a paired ASCII delimiter hugs RTL text and should shape proportionally alongside it. */
    static boolean isDelimInRtlFlow(BidiLayout.LogicalCell[] visualCells, int activeLen, int vCol) {
        return canJoinRtlAdjacentDelimiter(visualCells, activeLen, vCol);
    }

    private static boolean isRtlOrDelimFlowCell(BidiLayout.LogicalCell[] visualCells, int activeLen, int vCol) {
        if (vCol < 0 || vCol >= activeLen) return false;
        return isShapedRtlCell(visualCells[vCol]) || isDelimInRtlFlow(visualCells, activeLen, vCol);
    }

    /** Bridges single spaces between adjacent shaped RTL cells into a single HarfBuzz phrase. */
    private static boolean canBridgeRtlFlowSpace(BidiLayout.LogicalCell[] visualCells, int activeLen,
                                                 int vCol, BidiLayout.LogicalCell lastRtlCell) {
        if (lastRtlCell == null || vCol + 1 >= activeLen) return false;
        BidiLayout.LogicalCell curr = visualCells[vCol];
        if (curr.hasBitmap || curr.codePoint != ' ' || curr.combiningCount != 0 || curr.displayWidth != 1) {
            return false;
        }
        BidiLayout.LogicalCell next = visualCells[vCol + 1];
        if (!isShapedRtlCell(next)) return false;
        return Math.abs(curr.originalColumn - lastRtlCell.originalColumn) == 1
                && Math.abs(next.originalColumn - curr.originalColumn) == 1;
    }

    /** Advances from {@code startCol} across one shaped RTL phrase (including bridged internal spaces). */
    private static int findRtlFlowSegmentEnd(BidiLayout.LogicalCell[] visualCells, int activeLen, int startCol) {
        BidiLayout.LogicalCell lastRtl = visualCells[startCol];
        int v = startCol + 1;
        while (v < activeLen) {
            BidiLayout.LogicalCell cur = visualCells[v];
            if (isShapedRtlCell(cur)) {
                if (lastRtl != null && Math.abs(cur.originalColumn - lastRtl.originalColumn) > 2) break;
                lastRtl = cur;
                v++;
            } else if (canBridgeRtlFlowSpace(visualCells, activeLen, v, lastRtl)) {
                lastRtl = visualCells[v + 1];
                v += 2;
            } else {
                break;
            }
        }
        return v;
    }

    /**
     * Classifies an unbridged plain space at {@code vCol}: returns {@code 2} if both nearest non-space
     * active neighbors (or the single existing side) are RTL, {@code 1} if mixed RTL/LTR boundary,
     * and {@code 0} if purely between LTR neighbors.
     */
    private static int classifyFlowSpace(BidiLayout.LogicalCell[] visualCells, int promptEnd, int activeLen, int vCol) {
        int leftScript = -1; // -1 none/prompt, 0 LTR, 1 RTL
        for (int i = vCol - 1; i >= promptEnd; i--) {
            BidiLayout.LogicalCell c = visualCells[i];
            if (c.displayWidth == 0 && c.codePoint == 0) continue;
            if (!isPlainSpaceCell(c)) {
                leftScript = isRtlOrDelimFlowCell(visualCells, activeLen, i) ? 1 : 0;
                break;
            }
        }
        if (leftScript < 0 && promptEnd > 0) {
            leftScript = 0; // Prompt prefix is left-anchored LTR boundary.
        }

        int rightScript = -1;
        for (int i = vCol + 1; i < activeLen; i++) {
            BidiLayout.LogicalCell c = visualCells[i];
            if (c.displayWidth == 0 && c.codePoint == 0) continue;
            if (!isPlainSpaceCell(c)) {
                rightScript = isRtlOrDelimFlowCell(visualCells, activeLen, i) ? 1 : 0;
                break;
            }
        }

        if (leftScript < 0 && rightScript < 0) return 0;
        if (leftScript == 1 && (rightScript == 1 || rightScript < 0)) return 2;
        if (rightScript == 1 && leftScript < 0) return 2;
        if (leftScript == 1 || rightScript == 1) return 1;
        return 0;
    }

    private void ensureRunCellCapacity(int needed) {
        if (mRunCells.length >= needed) return;
        int newCap = Math.max(needed, mRunCells.length * 2);
        mRunCells = new BidiLayout.LogicalCell[newCap];
        mCellCharStart = new int[newCap];
        mCellCharEnd = new int[newCap];
        mVisualToRunCell = new int[newCap];
        mVisCellLeft = new float[newCap];
        mVisCellRight = new float[newCap];
    }

    private void ensureRunCharCapacity(int needed) {
        if (mRunCharBuffer.length >= needed) return;
        mRunCharBuffer = new char[Math.max(needed, mRunCharBuffer.length * 2)];
    }

    private void applyPaintStyleForMeasurement(long cellStyle) {
        int effect = TextStyle.decodeEffect(cellStyle);
        mTextPaint.setFakeBoldText((effect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK)) != 0);
        mTextPaint.setTextSkewX((effect & TextStyle.CHARACTER_ATTRIBUTE_ITALIC) != 0 ? -0.35f : 0.f);
        mTextPaint.setUnderlineText(false);
        mTextPaint.setStrikeThruText(false);
    }

    private float measureFlowLtrCell(BidiLayout.LogicalCell cell) {
        if (cell.combiningCount == 0 && cell.codePoint < asciiMeasures.length && !isInvisibleBidiFormatChar(cell.codePoint)) {
            int effect = TextStyle.decodeEffect(cell.style);
            boolean boldItalic = (effect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK | TextStyle.CHARACTER_ATTRIBUTE_ITALIC)) != 0;
            if (!boldItalic) {
                return asciiMeasures[cell.codePoint];
            }
        }
        applyPaintStyleForMeasurement(cell.style);
        int tl = encodeCellToBuffer(cell, mCellMeasureBuffer, 0);
        float w = (tl > 0) ? mTextPaint.measureText(mCellMeasureBuffer, 0, tl) : 0f;
        return (w > 0f) ? w : (Math.max(1, cell.displayWidth) * mFontWidth);
    }

    private float encodeAndMeasureRtlFlowSpan(BidiLayout.LogicalCell[] visualCells, int sStart, int sEnd) {
        int sCount = 0;
        int neededChars = 0;
        for (int v = sStart; v < sEnd; v++) {
            BidiLayout.LogicalCell c = visualCells[v];
            if (c.displayWidth == 0 && c.codePoint == 0) continue;
            mRunCells[sCount++] = c;
            if (c.codePoint != 0) neededChars += Character.charCount(c.codePoint);
            if (c.combiningChars != null) {
                for (int k = 0; k < c.combiningCount; k++) {
                    neededChars += Character.charCount(c.combiningChars[k]);
                }
            }
        }
        if (sCount == 0) return 0f;

        ensureRunCharCapacity(neededChars);
        sortRunCellsLogical(sCount);

        int used = 0;
        for (int r = 0; r < sCount; r++) {
            BidiLayout.LogicalCell rc = mRunCells[r];
            int cStart = used;
            used += encodeCellToBuffer(rc, mRunCharBuffer, used);
            int cEnd = used;
            for (int v = sStart; v < sEnd; v++) {
                if (visualCells[v] == rc) {
                    mCellCharStart[v] = cStart;
                    mCellCharEnd[v] = cEnd;
                    break;
                }
            }
        }
        if (used == 0) return 0f;

        applyPaintStyleForMeasurement(visualCells[sStart].style);
        float segNatW = mTextPaint.measureText(mRunCharBuffer, 0, used);
        boolean useRunAdvance = (mUseApi23TextRun && segNatW > 0f);
        int span = sEnd - sStart;
        for (int v = sStart; v < sEnd; v++) {
            float gLeft;
            float gRight;
            if (useRunAdvance) {
                int cStart = mCellCharStart[v];
                int cEnd = mCellCharEnd[v];
                float advStart = (cStart <= 0) ? 0f
                        : mTextPaint.getRunAdvance(mRunCharBuffer, 0, used, 0, used, true, cStart);
                float advEnd = (cEnd >= used) ? segNatW
                        : mTextPaint.getRunAdvance(mRunCharBuffer, 0, used, 0, used, true, cEnd);
                float loAdv = Math.min(advStart, advEnd);
                float hiAdv = Math.max(advStart, advEnd);
                gLeft = Math.max(0f, segNatW - hiAdv);
                gRight = Math.min(segNatW, segNatW - loAdv);
            } else {
                float rel = v - sStart;
                gLeft = segNatW * (rel / span);
                gRight = segNatW * ((rel + 1) / span);
            }
            mVisCellLeft[v] = gLeft;
            mVisCellRight[v] = gRight;
        }
        return segNatW;
    }

    /** Returns cached pixel geometry for {@code layout}, computing and caching on first query or font change. */
    BidiLayout.PixelGeometry ensureRowPixelGeometry(BidiLayout layout, int columns, boolean altBuffer) {
        BidiLayout.PixelGeometry cached = layout.mPixelGeometry;
        if (cached != null && cached.matches(mTextSize, mTypeface, mFontWidth, altBuffer)
                && cached.visLeft.length == columns) {
            return cached;
        }
        BidiLayout.PixelGeometry computed = computeRowPixelGeometry(layout, columns, altBuffer);
        layout.mPixelGeometry = computed;
        return computed;
    }

    private BidiLayout.PixelGeometry computeRowPixelGeometry(BidiLayout layout, int columns, boolean altBuffer) {
        final float[] visLeft = new float[columns];
        final float[] visRight = new float[columns];
        final float viewportW = columns * mFontWidth;

        if (usesStrictGrid(layout, altBuffer)) {
            for (int v = 0; v < columns; v++) {
                visLeft[v] = v * mFontWidth;
                visRight[v] = (v + 1) * mFontWidth;
            }
            return new BidiLayout.PixelGeometry(visLeft, visRight, false, false,
                    0f, viewportW, 1.0f, mFontWidth,
                    mTextSize, mTypeface, mFontWidth, altBuffer);
        }

        final BidiLayout.LogicalCell[] visualCells = layout.visualCells;
        final int activeLen = Math.min(columns, Math.max(0, layout.trimmedActiveLength));
        final int promptEnd = Math.max(0, Math.min(layout.promptPrefixLength, activeLen));
        final boolean rightAligned = shouldRightAlignBidi(layout, altBuffer);

        ensureRunCellCapacity(columns);
        ensureSegCapacity(columns);

        final Typeface origTypeface = mTextPaint.getTypeface();
        final float origTextSize = mTextPaint.getTextSize();
        final boolean origFakeBold = mTextPaint.isFakeBoldText();
        final float origSkewX = mTextPaint.getTextSkewX();

        float rtlSpaceW = mFontWidth * 0.44f;
        int segCount = 0;
        float totalPayloadW = 0f;

        try {
            mTextPaint.setTypeface(Typeface.DEFAULT);
            mTextPaint.setTextSize(mTextSize * RTL_TEXT_SIZE_SCALE);
            mTextPaint.setFakeBoldText(false);
            mTextPaint.setTextSkewX(0f);
            float measuredSpace = mTextPaint.measureText(" ");
            if (measuredSpace > 0f) rtlSpaceW = measuredSpace;
            final float interScriptSpaceW = Math.min(mFontWidth * 0.65f, Math.max(rtlSpaceW, mFontWidth * 0.48f));

            for (int v = 0; v < promptEnd; v++) {
                visLeft[v] = v * mFontWidth;
                visRight[v] = (v + 1) * mFontWidth;
            }

            int vCol = promptEnd;
            while (vCol < activeLen) {
                BidiLayout.LogicalCell cell = visualCells[vCol];
                if (cell.hasBitmap) {
                    int wCols = Math.max(1, cell.displayWidth);
                    int sEnd = Math.min(activeLen, vCol + wCols);
                    float bw = (sEnd - vCol) * mFontWidth;
                    for (int k = vCol; k < sEnd; k++) {
                        int rel = k - vCol;
                        mVisCellLeft[k] = rel * mFontWidth;
                        mVisCellRight[k] = (rel + 1) * mFontWidth;
                    }
                    mSegKind[segCount] = SEG_BITMAP;
                    mSegVisStart[segCount] = vCol;
                    mSegVisEnd[segCount] = sEnd;
                    mSegNatWidth[segCount] = bw;
                    totalPayloadW += bw;
                    segCount++;
                    vCol = sEnd;
                } else if (cell.displayWidth == 0 && cell.codePoint == 0) {
                    mVisCellLeft[vCol] = 0f;
                    mVisCellRight[vCol] = 0f;
                    vCol++;
                } else if (isShapedRtlCell(cell)) {
                    int sStart = vCol;
                    int sEnd = findRtlFlowSegmentEnd(visualCells, activeLen, sStart);
                    mTextPaint.setTypeface(Typeface.DEFAULT);
                    mTextPaint.setTextSize(mTextSize * RTL_TEXT_SIZE_SCALE);
                    float segW = encodeAndMeasureRtlFlowSpan(visualCells, sStart, sEnd);
                    mSegKind[segCount] = SEG_RTL;
                    mSegVisStart[segCount] = sStart;
                    mSegVisEnd[segCount] = sEnd;
                    mSegNatWidth[segCount] = segW;
                    totalPayloadW += segW;
                    segCount++;
                    vCol = sEnd;
                } else if (isDelimInRtlFlow(visualCells, activeLen, vCol)) {
                    mTextPaint.setTypeface(Typeface.DEFAULT);
                    mTextPaint.setTextSize(mTextSize * RTL_TEXT_SIZE_SCALE);
                    applyPaintStyleForMeasurement(cell.style);
                    int dispCp = cell.isRtl ? getMirroredAsciiChar(cell.codePoint) : cell.codePoint;
                    mCellMeasureBuffer[0] = (char) dispCp;
                    float dw = mTextPaint.measureText(mCellMeasureBuffer, 0, 1);
                    if (dw <= 0f) dw = mFontWidth * 0.44f;
                    mVisCellLeft[vCol] = 0f;
                    mVisCellRight[vCol] = dw;
                    mSegKind[segCount] = SEG_DELIM;
                    mSegVisStart[segCount] = vCol;
                    mSegVisEnd[segCount] = vCol + 1;
                    mSegNatWidth[segCount] = dw;
                    totalPayloadW += dw;
                    segCount++;
                    vCol++;
                } else if (isPlainSpaceCell(cell) && classifyFlowSpace(visualCells, promptEnd, activeLen, vCol) > 0) {
                    int cls = classifyFlowSpace(visualCells, promptEnd, activeLen, vCol);
                    float sw = (cls == 2) ? rtlSpaceW : interScriptSpaceW;
                    mVisCellLeft[vCol] = 0f;
                    mVisCellRight[vCol] = sw;
                    mSegKind[segCount] = SEG_SPACE;
                    mSegVisStart[segCount] = vCol;
                    mSegVisEnd[segCount] = vCol + 1;
                    mSegNatWidth[segCount] = sw;
                    totalPayloadW += sw;
                    segCount++;
                    vCol++;
                } else {
                    mTextPaint.setTypeface(mTypeface);
                    mTextPaint.setTextSize(mTextSize);
                    int sStart = vCol;
                    float ltrOffset = 0f;
                    while (vCol < activeLen) {
                        BidiLayout.LogicalCell cur = visualCells[vCol];
                        if (cur.hasBitmap || isShapedRtlCell(cur) || isDelimInRtlFlow(visualCells, activeLen, vCol)) {
                            break;
                        }
                        if (isPlainSpaceCell(cur) && classifyFlowSpace(visualCells, promptEnd, activeLen, vCol) > 0) {
                            break;
                        }
                        if (cur.displayWidth == 0 && cur.codePoint == 0) {
                            mVisCellLeft[vCol] = ltrOffset;
                            mVisCellRight[vCol] = ltrOffset;
                            vCol++;
                            continue;
                        }
                        float cw = measureFlowLtrCell(cur);
                        mVisCellLeft[vCol] = ltrOffset;
                        if (cur.displayWidth == 2 && vCol + 1 < activeLen
                                && visualCells[vCol + 1].displayWidth == 0 && visualCells[vCol + 1].codePoint == 0) {
                            float mid = ltrOffset + cw * 0.5f;
                            mVisCellRight[vCol] = mid;
                            mVisCellLeft[vCol + 1] = mid;
                            mVisCellRight[vCol + 1] = ltrOffset + cw;
                            ltrOffset += cw;
                            vCol += 2;
                        } else {
                            mVisCellRight[vCol] = ltrOffset + cw;
                            ltrOffset += cw;
                            vCol++;
                        }
                    }
                    int sEnd = vCol;
                    mSegKind[segCount] = SEG_LTR;
                    mSegVisStart[segCount] = sStart;
                    mSegVisEnd[segCount] = sEnd;
                    mSegNatWidth[segCount] = ltrOffset;
                    totalPayloadW += ltrOffset;
                    segCount++;
                }
            }
        } finally {
            mTextPaint.setTypeface(origTypeface);
            mTextPaint.setTextSize(origTextSize);
            mTextPaint.setFakeBoldText(origFakeBold);
            mTextPaint.setTextSkewX(origSkewX);
        }

        final float fixedPrefixW = promptEnd * mFontWidth;
        final float availPayloadW = Math.max(mFontWidth, viewportW - fixedPrefixW);
        final float flowScaleX = (totalPayloadW > availPayloadW && totalPayloadW > 0f)
                ? Math.max(0.25f, availPayloadW / totalPayloadW)
                : 1.0f;
        final float drawnPayloadW = totalPayloadW * flowScaleX;
        final float startX = rightAligned ? Math.max(0f, viewportW - drawnPayloadW) : fixedPrefixW;

        float segCurX = startX;
        for (int s = 0; s < segCount; s++) {
            int sStart = mSegVisStart[s];
            int sEnd = mSegVisEnd[s];
            float segDrawnW = mSegNatWidth[s] * flowScaleX;
            for (int v = sStart; v < sEnd; v++) {
                visLeft[v] = segCurX + mVisCellLeft[v] * flowScaleX;
                visRight[v] = segCurX + mVisCellRight[v] * flowScaleX;
            }
            if (sEnd > sStart) {
                visRight[sEnd - 1] = segCurX + segDrawnW;
            }
            segCurX += segDrawnW;
        }

        final float contentLeft = rightAligned ? startX : 0f;
        final float contentRight = rightAligned
                ? Math.min(viewportW, startX + drawnPayloadW)
                : Math.min(viewportW, fixedPrefixW + drawnPayloadW);
        final boolean rtlTail = rightAligned
                || (activeLen > promptEnd && isRtlOrDelimFlowCell(visualCells, activeLen, activeLen - 1));
        final float trailingSpaceW = rtlTail ? rtlSpaceW : mFontWidth;

        if (rightAligned) {
            float rx = contentLeft;
            for (int v = activeLen; v < columns; v++) {
                float leftEdge = Math.max(0f, rx - trailingSpaceW);
                visRight[v] = rx;
                visLeft[v] = leftEdge;
                rx = leftEdge;
            }
        } else {
            float lx = contentRight;
            for (int v = activeLen; v < columns; v++) {
                float rightEdge = Math.min(viewportW, lx + trailingSpaceW);
                visLeft[v] = lx;
                visRight[v] = rightEdge;
                lx = rightEdge;
            }
        }

        return new BidiLayout.PixelGeometry(visLeft, visRight, true, rightAligned,
                contentLeft, contentRight, flowScaleX, rtlSpaceW,
                mTextSize, mTypeface, mFontWidth, altBuffer);
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
        final boolean altBuffer = mEmulator.isAlternateBufferActive();

        if (reverseVideo) {
            canvas.drawColor(palette[TextStyle.COLOR_INDEX_FOREGROUND], PorterDuff.Mode.SRC);
        }

        int inheritedCtx = BidiLayout.resolveInheritedContext(screen, topRow, columns);
        byte paragraphDir = (byte) (inheritedCtx & BidiLayout.INHERITED_DIR_MASK);
        boolean paragraphStartsWithPrompt = (inheritedCtx & BidiLayout.INHERITED_PROMPT_FLAG) != 0;
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
                boolean wrapped = screen.getLineWrap(row);
                paragraphDir = wrapped ? BidiLayout.DIR_LTR : BidiLayout.DIR_DEFAULT;
                paragraphStartsWithPrompt = wrapped && paragraphStartsWithPrompt;
                continue;
            }

            if (!lineObject.mMayContainRtlOrBidi && paragraphDir != BidiLayout.DIR_RTL) {
                renderPureLtrRow(lineObject, screen, canvas, palette, columns,
                        cursorX, selx1, selx2, cursorShape, reverseVideo, heightOffset);
                boolean wrapped = screen.getLineWrap(row);
                paragraphDir = wrapped ? BidiLayout.DIR_LTR : BidiLayout.DIR_DEFAULT;
                if (!wrapped) paragraphStartsWithPrompt = false;
                continue;
            }

            BidiLayout layout = BidiLayout.build(lineObject, columns, cursorCol, row == cursorRow && cursorVisible,
                    selectionY1, selectionY2, selectionX1, selectionX2, row, true,
                    paragraphDir, paragraphStartsWithPrompt);

            if (layout.isIdentity && !shouldRightAlignBidi(layout, altBuffer)) {
                renderPureLtrRow(lineObject, screen, canvas, palette, columns,
                        cursorX, selx1, selx2, cursorShape, reverseVideo, heightOffset);
            } else {
                renderBidiRow(layout, screen, canvas, palette, columns,
                        cursorShape, reverseVideo, heightOffset, altBuffer);
            }

            boolean wrapped = screen.getLineWrap(row);
            paragraphDir = wrapped ? layout.resolvedBaseDirection : BidiLayout.DIR_DEFAULT;
            paragraphStartsWithPrompt = wrapped && layout.paragraphStartsWithPrompt;
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

    /** Renders a BiDi-reordered row using continuous Web/Facebook flow in main buffer or strict grid in TUIs. */
    private void renderBidiRow(BidiLayout layout, TerminalBuffer screen, Canvas canvas, int[] palette,
                               int columns, int cursorShape, boolean reverseVideo, float heightOffset,
                               boolean altBuffer) {
        if (!usesStrictGrid(layout, altBuffer)) {
            BidiLayout.PixelGeometry geo = ensureRowPixelGeometry(layout, columns, false);
            renderBidiRowContinuous(layout, geo, screen, canvas, palette, columns,
                    cursorShape, reverseVideo, heightOffset);
            return;
        }
        renderBidiRowStrict(layout, screen, canvas, palette, columns, cursorShape, reverseVideo, heightOffset);
    }

    private void renderBidiRowContinuous(BidiLayout layout, BidiLayout.PixelGeometry geo,
                                         TerminalBuffer screen, Canvas canvas, int[] palette,
                                         int columns, int cursorShape, boolean reverseVideo, float heightOffset) {
        final BidiLayout.LogicalCell[] visualCells = layout.visualCells;
        final int activeLen = Math.min(columns, Math.max(0, layout.trimmedActiveLength));
        final int promptEnd = Math.max(0, Math.min(layout.promptPrefixLength, activeLen));

        final float top = heightOffset - mFontLineSpacingAndAscent + mFontAscent;
        final float bottom = heightOffset;
        final float baseline = heightOffset - mFontLineSpacingAndAscent;
        final float clipTop = top - mFontLineSpacing;
        final float clipBottom = bottom + mFontLineSpacing;
        final float viewportW = columns * mFontWidth;
        final boolean isBlockCursor = (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK);

        // Pass 1: Backgrounds, selections, and cursors read straight from unified PixelGeometry.
        drawContinuousOverlays(canvas, visualCells, geo, activeLen, columns, palette,
                cursorShape, reverseVideo, top, bottom, viewportW);

        if (activeLen <= 0) return;

        final Typeface origTypeface = mTextPaint.getTypeface();
        final float origTextSize = mTextPaint.getTextSize();
        final boolean origFakeBold = mTextPaint.isFakeBoldText();
        final float origSkewX = mTextPaint.getTextSkewX();

        try {
            // Draw left-anchored monospace prompt prefix cleanly.
            int vCol = 0;
            if (promptEnd > 0) {
                mTextPaint.setTypeface(mTypeface);
                mTextPaint.setTextSize(mTextSize);
                while (vCol < promptEnd) {
                    BidiLayout.LogicalCell pCell = visualCells[vCol];
                    if (pCell.hasBitmap) {
                        int spanEnd = Math.min(promptEnd, vCol + Math.max(1, pCell.displayWidth));
                        drawSixelBitmapRect(screen.getSixelBitmap(pCell.style), screen.getSixelRect(pCell.style),
                                canvas, geo.visLeft[vCol], geo.visRight[spanEnd - 1], heightOffset);
                        vCol = spanEnd;
                        continue;
                    }
                    if (pCell.displayWidth == 0 && pCell.codePoint == 0) {
                        vCol++;
                        continue;
                    }
                    boolean startInv = pCell.insideSelection ^ (pCell.insideCursor && isBlockCursor);
                    int subEnd = vCol + 1;
                    while (subEnd < promptEnd) {
                        BidiLayout.LogicalCell next = visualCells[subEnd];
                        if (next.hasBitmap) break;
                        if (next.displayWidth == 0 && next.codePoint == 0) {
                            subEnd++;
                            continue;
                        }
                        boolean nextInv = next.insideSelection ^ (next.insideCursor && isBlockCursor);
                        if (next.style != pCell.style || nextInv != startInv) break;
                        subEnd++;
                    }
                    drawContinuousLtrRun(canvas, visualCells, geo, vCol, subEnd, palette,
                            cursorShape, reverseVideo, baseline);
                    vCol = subEnd;
                }
            }

            // Pass 2: Draw payload segments in continuous flow using the shared classification helpers.
            vCol = promptEnd;
            while (vCol < activeLen) {
                BidiLayout.LogicalCell cell = visualCells[vCol];
                if (cell.hasBitmap) {
                    int spanEnd = Math.min(activeLen, vCol + Math.max(1, cell.displayWidth));
                    drawSixelBitmapRect(screen.getSixelBitmap(cell.style), screen.getSixelRect(cell.style),
                            canvas, geo.visLeft[vCol], geo.visRight[spanEnd - 1], heightOffset);
                    vCol = spanEnd;
                } else if (cell.displayWidth == 0 && cell.codePoint == 0) {
                    vCol++;
                } else if (isShapedRtlCell(cell)) {
                    int sStart = vCol;
                    int sEnd = findRtlFlowSegmentEnd(visualCells, activeLen, sStart);
                    mTextPaint.setTypeface(Typeface.DEFAULT);
                    mTextPaint.setTextSize(mTextSize * RTL_TEXT_SIZE_SCALE);
                    drawContinuousRtlSpan(canvas, visualCells, geo, sStart, sEnd, promptEnd, activeLen,
                            palette, cursorShape, reverseVideo, baseline, clipTop, clipBottom);
                    vCol = sEnd;
                } else if (isDelimInRtlFlow(visualCells, activeLen, vCol)) {
                    mTextPaint.setTypeface(Typeface.DEFAULT);
                    mTextPaint.setTextSize(mTextSize * RTL_TEXT_SIZE_SCALE);
                    drawContinuousDelim(canvas, cell, geo, vCol, palette, cursorShape, reverseVideo, baseline);
                    vCol++;
                } else if (isPlainSpaceCell(cell) && classifyFlowSpace(visualCells, promptEnd, activeLen, vCol) > 0) {
                    vCol++;
                } else {
                    mTextPaint.setTypeface(mTypeface);
                    mTextPaint.setTextSize(mTextSize);
                    int sStart = vCol;
                    while (vCol < activeLen) {
                        BidiLayout.LogicalCell cur = visualCells[vCol];
                        if (cur.hasBitmap || isShapedRtlCell(cur) || isDelimInRtlFlow(visualCells, activeLen, vCol)) {
                            break;
                        }
                        if (isPlainSpaceCell(cur) && classifyFlowSpace(visualCells, promptEnd, activeLen, vCol) > 0) {
                            break;
                        }
                        vCol++;
                    }
                    int sEnd = vCol;
                    int subStart = sStart;
                    while (subStart < sEnd) {
                        BidiLayout.LogicalCell sc = visualCells[subStart];
                        if (sc.displayWidth == 0 && sc.codePoint == 0) {
                            subStart++;
                            continue;
                        }
                        boolean startInv = sc.insideSelection ^ (sc.insideCursor && isBlockCursor);
                        int subEnd = subStart + 1;
                        while (subEnd < sEnd) {
                            BidiLayout.LogicalCell next = visualCells[subEnd];
                            if (next.displayWidth == 0 && next.codePoint == 0) {
                                subEnd++;
                                continue;
                            }
                            boolean nextInv = next.insideSelection ^ (next.insideCursor && isBlockCursor);
                            if (next.style != sc.style || nextInv != startInv) break;
                            subEnd++;
                        }
                        drawContinuousLtrRun(canvas, visualCells, geo, subStart, subEnd, palette,
                                cursorShape, reverseVideo, baseline);
                        subStart = subEnd;
                    }
                }
            }
        } finally {
            mTextPaint.setTypeface(origTypeface);
            mTextPaint.setTextSize(origTextSize);
            mTextPaint.setFakeBoldText(origFakeBold);
            mTextPaint.setTextSkewX(origSkewX);
            mTextPaint.setUnderlineText(false);
            mTextPaint.setStrikeThruText(false);
        }
    }

    private void drawContinuousOverlays(Canvas canvas, BidiLayout.LogicalCell[] visualCells,
                                        BidiLayout.PixelGeometry geo, int activeLen, int columns,
                                        int[] palette, int cursorShape, boolean reverseVideo,
                                        float top, float bottom, float viewportW) {
        final int defaultBg = palette[TextStyle.COLOR_INDEX_BACKGROUND];
        final int cursorColor = palette[TextStyle.COLOR_INDEX_CURSOR];
        final boolean isBlockCursor = (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK);

        boolean hasBatch = false;
        int batchColor = defaultBg;
        float batchLeft = 0f;
        float batchRight = 0f;

        for (int v = 0; v < columns; v++) {
            BidiLayout.LogicalCell cell = visualCells[v];
            if (cell.displayWidth == 0 && cell.codePoint == 0) continue;

            float cellLeft = geo.visLeft[v];
            float cellRight = geo.visRight[v];
            if (cell.displayWidth == 2 && v + 1 < columns
                    && visualCells[v + 1].displayWidth == 0 && visualCells[v + 1].codePoint == 0) {
                cellRight = Math.max(cellRight, geo.visRight[v + 1]);
            }

            long style = cell.style;
            int effect = TextStyle.decodeEffect(style);
            boolean bold = (effect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK)) != 0;
            int fore = resolveForeColor(TextStyle.decodeForeColor(style), bold, palette);
            int back = resolveBackColor(TextStyle.decodeBackColor(style), palette);
            boolean cellInvert = reverseVideo
                    ^ ((effect & TextStyle.CHARACTER_ATTRIBUTE_INVERSE) != 0)
                    ^ cell.insideSelection
                    ^ (cell.insideCursor && isBlockCursor);
            int resolvedBg = cellInvert ? fore : back;

            if (resolvedBg != defaultBg) {
                if (hasBatch && resolvedBg == batchColor
                        && (Math.abs(cellLeft - batchRight) < 0.05f || Math.abs(batchLeft - cellRight) < 0.05f)) {
                    batchLeft = Math.min(batchLeft, cellLeft);
                    batchRight = Math.max(batchRight, cellRight);
                } else {
                    if (hasBatch) {
                        mTextPaint.setColor(batchColor);
                        canvas.drawRect(batchLeft, top, batchRight, bottom, mTextPaint);
                    }
                    hasBatch = true;
                    batchColor = resolvedBg;
                    batchLeft = cellLeft;
                    batchRight = cellRight;
                }
            } else if (hasBatch) {
                mTextPaint.setColor(batchColor);
                canvas.drawRect(batchLeft, top, batchRight, bottom, mTextPaint);
                hasBatch = false;
            }
        }
        if (hasBatch) {
            mTextPaint.setColor(batchColor);
            canvas.drawRect(batchLeft, top, batchRight, bottom, mTextPaint);
        }

        // Draw cursor indicator cleanly over cell bounds.
        for (int v = 0; v < columns; v++) {
            BidiLayout.LogicalCell cell = visualCells[v];
            if (cell.displayWidth == 0 && cell.codePoint == 0) continue;
            boolean hasWideCont = (cell.displayWidth == 2 && v + 1 < columns
                    && visualCells[v + 1].displayWidth == 0 && visualCells[v + 1].codePoint == 0);
            boolean wideCursor = hasWideCont && visualCells[v + 1].insideCursor;
            if (!cell.insideCursor && !wideCursor) continue;

            float cLeft = geo.visLeft[v];
            float cRight = hasWideCont ? Math.max(geo.visRight[v], geo.visRight[v + 1]) : geo.visRight[v];
            mTextPaint.setColor(cursorColor);
            float cursorHeight = mFontLineSpacingAndAscent - mFontAscent;

            if (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR) {
                float barW = Math.max(2f, mFontWidth / 4.f);
                boolean isTrailingEol = (v >= activeLen);
                boolean anchorRight = isTrailingEol ? geo.rightAligned : cell.isRtl;
                float barL;
                float barR;
                if (anchorRight) {
                    barR = Math.max(barW, Math.min(viewportW, cRight));
                    barL = barR - barW;
                } else {
                    barL = Math.max(0f, Math.min(viewportW - barW, cLeft));
                    barR = barL + barW;
                }
                canvas.drawRect(barL, bottom - cursorHeight, barR, bottom, mTextPaint);
            } else {
                float drawL = cLeft;
                float drawR = cRight;
                if (v >= activeLen || drawR - drawL < 2f) {
                    float minW = Math.max(drawR - drawL, mFontWidth * 0.45f);
                    if (geo.rightAligned && v >= activeLen) {
                        drawR = Math.max(minW, Math.min(viewportW, drawR));
                        drawL = drawR - minW;
                    } else {
                        drawL = Math.max(0f, Math.min(viewportW - minW, drawL));
                        drawR = drawL + minW;
                    }
                }
                float cTop = (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE)
                        ? (bottom - cursorHeight / 4.f)
                        : (bottom - cursorHeight);
                canvas.drawRect(drawL, cTop, drawR, bottom, mTextPaint);
            }
        }
    }

    private void drawContinuousDelim(Canvas canvas, BidiLayout.LogicalCell cell, BidiLayout.PixelGeometry geo,
                                     int vCol, int[] palette, int cursorShape, boolean reverseVideo, float baseline) {
        if (!configurePaintForCell(cell, palette, cursorShape, reverseVideo)) return;
        int dispCp = cell.isRtl ? getMirroredAsciiChar(cell.codePoint) : cell.codePoint;
        mCellMeasureBuffer[0] = (char) dispCp;
        float gx = geo.visLeft[vCol];
        boolean needScale = Math.abs(geo.scaleX - 1.0f) > 0.005f;
        if (needScale) {
            canvas.save();
            canvas.scale(geo.scaleX, 1.0f);
            canvas.drawText(mCellMeasureBuffer, 0, 1, gx / geo.scaleX, baseline, mTextPaint);
            canvas.restore();
        } else {
            canvas.drawText(mCellMeasureBuffer, 0, 1, gx, baseline, mTextPaint);
        }
    }

    private void drawContinuousLtrRun(Canvas canvas, BidiLayout.LogicalCell[] visualCells,
                                      BidiLayout.PixelGeometry geo, int startCol, int endCol,
                                      int[] palette, int cursorShape, boolean reverseVideo, float baseline) {
        BidiLayout.LogicalCell repCell = null;
        int neededChars = 0;
        for (int v = startCol; v < endCol; v++) {
            BidiLayout.LogicalCell c = visualCells[v];
            if (c.displayWidth == 0 && c.codePoint == 0) continue;
            if (repCell == null) repCell = c;
            if (c.codePoint != 0) neededChars += Character.charCount(c.codePoint);
            if (c.combiningChars != null) {
                for (int k = 0; k < c.combiningCount; k++) {
                    neededChars += Character.charCount(c.combiningChars[k]);
                }
            }
        }
        if (repCell == null || neededChars == 0) return;
        if (!configurePaintForCell(repCell, palette, cursorShape, reverseVideo)) return;

        ensureRunCharCapacity(neededChars);
        int used = 0;
        float measuredW = 0f;
        for (int v = startCol; v < endCol; v++) {
            BidiLayout.LogicalCell c = visualCells[v];
            if (c.displayWidth == 0 && c.codePoint == 0) continue;
            used += encodeCellToBuffer(c, mRunCharBuffer, used);
            measuredW += measureFlowLtrCell(c);
        }
        if (used == 0) return;

        float slotLeft = geo.visLeft[startCol];
        float slotRight = geo.visRight[endCol - 1];
        float slotW = slotRight - slotLeft;
        boolean saved = false;
        float drawX = slotLeft;
        if (measuredW > 0f && slotW > 0f && Math.abs(slotW / measuredW - 1.0f) > 0.005f) {
            float sx = slotW / measuredW;
            canvas.save();
            canvas.scale(sx, 1.0f);
            drawX = slotLeft / sx;
            saved = true;
        }
        if (mUseApi23TextRun) {
            canvas.drawTextRun(mRunCharBuffer, 0, used, 0, used, drawX, baseline, false, mTextPaint);
        } else {
            canvas.drawText(mRunCharBuffer, 0, used, drawX, baseline, mTextPaint);
        }
        if (saved) canvas.restore();
    }

    private void drawContinuousRtlSpan(Canvas canvas, BidiLayout.LogicalCell[] visualCells,
                                       BidiLayout.PixelGeometry geo, int sStart, int sEnd,
                                       int promptEnd, int activeLen, int[] palette,
                                       int cursorShape, boolean reverseVideo,
                                       float baseline, float clipTop, float clipBottom) {
        int sCount = 0;
        int neededChars = 0;
        for (int v = sStart; v < sEnd; v++) {
            BidiLayout.LogicalCell c = visualCells[v];
            if (c.displayWidth == 0 && c.codePoint == 0) continue;
            mRunCells[sCount++] = c;
            if (c.codePoint != 0) neededChars += Character.charCount(c.codePoint);
            if (c.combiningChars != null) {
                for (int k = 0; k < c.combiningCount; k++) {
                    neededChars += Character.charCount(c.combiningChars[k]);
                }
            }
        }
        if (sCount == 0 || neededChars == 0) return;

        ensureRunCharCapacity(neededChars);
        sortRunCellsLogical(sCount);
        int used = 0;
        for (int r = 0; r < sCount; r++) {
            used += encodeCellToBuffer(mRunCells[r], mRunCharBuffer, used);
        }
        if (used == 0) return;

        float scaleX = geo.scaleX;
        boolean needScale = Math.abs(scaleX - 1.0f) > 0.005f;
        float drawLeft = geo.visLeft[sStart];
        final boolean isBlockCursor = (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK);

        if (isSpanUniform(visualCells, sStart, sEnd, isBlockCursor)) {
            drawRtlSegment(canvas, visualCells[sStart], palette, mRunCharBuffer, 0, used,
                    scaleX, drawLeft, needScale, baseline, cursorShape, reverseVideo,
                    false, 0f, 0f, 0f, 0f);
            return;
        }

        int subStart = sStart;
        while (subStart < sEnd) {
            BidiLayout.LogicalCell startCell = visualCells[subStart];
            boolean startInv = startCell.insideSelection ^ (startCell.insideCursor && isBlockCursor);
            int subEnd = subStart + 1;
            while (subEnd < sEnd) {
                BidiLayout.LogicalCell next = visualCells[subEnd];
                boolean nextInv = next.insideSelection ^ (next.insideCursor && isBlockCursor);
                if (next.style != startCell.style || nextInv != startInv) break;
                subEnd++;
            }
            float padL = (subStart == sStart)
                    ? ((sStart > promptEnd && isDelimInRtlFlow(visualCells, activeLen, sStart - 1)) ? mFontWidth * 0.08f : mFontWidth * 0.35f)
                    : 0f;
            float padR = (subEnd == sEnd)
                    ? ((sEnd < activeLen && isDelimInRtlFlow(visualCells, activeLen, sEnd)) ? mFontWidth * 0.08f : mFontWidth * 0.35f)
                    : 0f;
            float clipL = geo.visLeft[subStart] - padL;
            float clipR = geo.visRight[subEnd - 1] + padR;
            drawRtlSegment(canvas, startCell, palette, mRunCharBuffer, 0, used,
                    scaleX, drawLeft, needScale, baseline, cursorShape, reverseVideo,
                    true, clipL, clipTop, clipR, clipBottom);
            subStart = subEnd;
        }
    }

    private void drawSixelBitmapRect(Bitmap bitmap, Rect srcRect, Canvas canvas,
                                     float left, float right, float heightOffset) {
        if (bitmap == null || bitmap.isRecycled()) return;
        float top = heightOffset - mFontLineSpacing;
        RectF dst = new RectF(left, top, right, heightOffset);
        canvas.drawBitmap(bitmap, srcRect, dst, null);
    }

    /** Strict grid fallback for alternate buffer, box-drawing tables, Sixel bitmaps, and columnar multi-space layouts. */
    private void renderBidiRowStrict(BidiLayout layout, TerminalBuffer screen, Canvas canvas, int[] palette,
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
                    || canJoinRtlAdjacentDelimiter(visualCells, columns, vCol)
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
                final int measureCp = (cell.isRtl && cell.combiningCount == 0 && isMirroredAsciiDelimiter(codePoint))
                        ? getMirroredAsciiChar(codePoint) : codePoint;
                final float measuredCodePointWidth;
                if (measureCp < asciiMeasures.length && cell.combiningCount == 0) {
                    measuredCodePointWidth = asciiMeasures[measureCp];
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
                // Keep connected RTL words and hugging paired delimiters unbroken across color/selection/cursor;
                // only split non-adjacent logical RTL slices.
                if (lastRtlCell != null && (isMirroredAsciiDelimiterCell(cell) || isMirroredAsciiDelimiterCell(lastRtlCell))) {
                    splitRun = isMirroredAsciiDelimiterCell(cell) && isMirroredAsciiDelimiterCell(lastRtlCell)
                            && Math.abs(cell.originalColumn - lastRtlCell.originalColumn) > 2;
                } else {
                    splitRun = lastRtlCell != null && Math.abs(cell.originalColumn - lastRtlCell.originalColumn) > 2;
                }
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
            int cp = cell.codePoint;
            if (isInvisibleBidiFormatChar(cp)) {
                cp = ' ';
            } else if (cell.isRtl && cell.combiningCount == 0 && isMirroredAsciiDelimiter(cp)) {
                cp = getMirroredAsciiChar(cp);
            }
            written += Character.toChars(cp, dst, offset + written);
        }
        if (cell.combiningChars != null) {
            for (int k = 0; k < cell.combiningCount; k++) {
                int comb = cell.combiningChars[k];
                if (!isInvisibleBidiFormatChar(comb)) {
                    written += Character.toChars(comb, dst, offset + written);
                }
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

        boolean hasJoinedDelimiter = false;
        for (int i = 0; i < count; i++) {
            if (isMirroredAsciiDelimiterCell(mRunCells[i])) {
                hasJoinedDelimiter = true;
                break;
            }
        }

        final Typeface originalTypeface = mTextPaint.getTypeface();
        final float originalTextSize = mTextPaint.getTextSize();
        mTextPaint.setTypeface(Typeface.DEFAULT);
        if (originalTextSize > 0f) {
            mTextPaint.setTextSize(originalTextSize * RTL_TEXT_SIZE_SCALE);
        }
        try {
            if (hasJoinedDelimiter) {
                flushMixedDelimRtlRun(canvas, visualCells, palette, heightOffset,
                        startCol, endCol, runColumns, cursorShape, reverseVideo,
                        originalTypeface, originalTextSize);
                return;
            }

            sortRunCellsLogical(count);

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

            final float top = heightOffset - mFontLineSpacingAndAscent + mFontAscent;
            final float bottom = heightOffset;
            final float clipTop = top - mFontLineSpacing;
            final float clipBottom = bottom + mFontLineSpacing;
            final int defaultBg = palette[TextStyle.COLOR_INDEX_BACKGROUND];
            final boolean isBlockCursor = (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK);

            boolean uniformText = true;
            boolean hasOverlay = reverseVideo;
            final long firstStyle = visualCells[startCol].style;
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

            int firstEffect = TextStyle.decodeEffect(firstStyle);
            mTextPaint.setFakeBoldText((firstEffect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK)) != 0);
            mTextPaint.setTextSkewX((firstEffect & TextStyle.CHARACTER_ATTRIBUTE_ITALIC) != 0 ? -0.35f : 0.f);
            final float measuredWidth = mTextPaint.measureText(mRunCharBuffer, 0, used);

            final float scaleX = computeRtlRunScaleX(measuredWidth, runColumns, mFontWidth);
            final int alignment = resolveRtlAlignment(visualCells, startCol, endCol, visualCells.length);
            final float drawLeft = computeBalancedRtlLeftX(startCol, runColumns, mFontWidth, measuredWidth, scaleX, alignment);
            final float drawnWidth = measuredWidth * scaleX;
            final float slotLeft = startCol * mFontWidth;
            final float slotRight = endCol * mFontWidth;
            final boolean needScale = Math.abs(scaleX - 1.0f) > 0.005f;
            final float baseline = heightOffset - mFontLineSpacingAndAscent;

            if (hasOverlay || !uniformText) {
                computeVisualCellBounds(0, runColumns, 0, used, measuredWidth, scaleX, drawLeft, true);
            }
            if (hasOverlay) {
                drawRtlOverlays(canvas, visualCells, palette, startCol, endCol, runColumns,
                        cursorShape, reverseVideo, top, bottom, slotLeft, slotRight);
            }

            drawRtlSpanSegments(canvas, visualCells, palette, startCol, endCol, startCol, endCol,
                    mRunCharBuffer, 0, used, scaleX, drawLeft, drawnWidth, slotLeft, slotRight,
                    needScale, baseline, clipTop, clipBottom, cursorShape, reverseVideo, uniformText);
        } finally {
            mTextPaint.setTypeface(originalTypeface);
            mTextPaint.setTextSize(originalTextSize);
        }
    }

    private void flushMixedDelimRtlRun(Canvas canvas, BidiLayout.LogicalCell[] visualCells,
                                       int[] palette, float heightOffset,
                                       int startCol, int endCol, int runColumns,
                                       int cursorShape, boolean reverseVideo,
                                       Typeface originalTypeface, float originalTextSize) {
        ensureSegCapacity(runColumns);
        int segCount = 0;
        int used = 0;
        float fixedDelimSum = 0f;
        float natRtlSum = 0f;
        float rawSpaceW = mTextPaint.measureText(" ");
        float spaceNatW = (rawSpaceW > 0f) ? rawSpaceW : (mFontWidth * 0.45f);

        int rel = 0;
        while (rel < runColumns) {
            BidiLayout.LogicalCell cell = visualCells[startCol + rel];
            if (isMirroredAsciiDelimiterCell(cell)) {
                int dispCp = cell.isRtl ? getMirroredAsciiChar(cell.codePoint) : cell.codePoint;
                mCellMeasureBuffer[0] = (char) dispCp;
                mTextPaint.setFakeBoldText(false);
                mTextPaint.setTextSkewX(0f);
                float rawDw = mTextPaint.measureText(mCellMeasureBuffer, 0, 1);
                float slotW = (rawDw > 0f) ? Math.min(rawDw, mFontWidth) : (mFontWidth * RTL_DELIMITER_ADVANCE_FACTOR);
                mSegKind[segCount] = SEG_DELIM;
                mSegVisStart[segCount] = rel;
                mSegVisEnd[segCount] = rel + 1;
                mSegCharStart[segCount] = 0;
                mSegCharEnd[segCount] = 0;
                mSegNatWidth[segCount] = slotW;
                fixedDelimSum += slotW;
                segCount++;
                rel++;
            } else if (isPlainSpaceCell(cell)
                    && ((rel > 0 && isMirroredAsciiDelimiterCell(visualCells[startCol + rel - 1]))
                    || (rel + 1 < runColumns && isMirroredAsciiDelimiterCell(visualCells[startCol + rel + 1])))) {
                mSegKind[segCount] = SEG_SPACE;
                mSegVisStart[segCount] = rel;
                mSegVisEnd[segCount] = rel + 1;
                mSegCharStart[segCount] = 0;
                mSegCharEnd[segCount] = 0;
                mSegNatWidth[segCount] = spaceNatW;
                natRtlSum += spaceNatW;
                segCount++;
                rel++;
            } else {
                int sStart = rel;
                int sCount = 0;
                while (rel < runColumns) {
                    BidiLayout.LogicalCell cur = visualCells[startCol + rel];
                    if (isMirroredAsciiDelimiterCell(cur)) break;
                    if (isPlainSpaceCell(cur) && rel + 1 < runColumns
                            && isMirroredAsciiDelimiterCell(visualCells[startCol + rel + 1])) {
                        break;
                    }
                    if (!(cur.displayWidth == 0 && cur.codePoint == 0)) {
                        mRunCells[sCount++] = cur;
                    }
                    rel++;
                }
                int sEnd = rel;
                if (sCount == 0) continue;
                sortRunCellsLogical(sCount);

                int bufStart = used;
                for (int r = 0; r < sCount; r++) {
                    BidiLayout.LogicalCell rc = mRunCells[r];
                    int cStart = used;
                    used += encodeCellToBuffer(rc, mRunCharBuffer, used);
                    int cEnd = used;
                    for (int v = sStart; v < sEnd; v++) {
                        if (visualCells[startCol + v] == rc) {
                            mCellCharStart[v] = cStart;
                            mCellCharEnd[v] = cEnd;
                            break;
                        }
                    }
                }
                int bufLen = used - bufStart;
                int segEffect = TextStyle.decodeEffect(visualCells[startCol + sStart].style);
                mTextPaint.setFakeBoldText((segEffect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK)) != 0);
                mTextPaint.setTextSkewX((segEffect & TextStyle.CHARACTER_ATTRIBUTE_ITALIC) != 0 ? -0.35f : 0.f);
                float segNatW = (bufLen > 0) ? mTextPaint.measureText(mRunCharBuffer, bufStart, bufLen) : 0f;

                mSegKind[segCount] = SEG_RTL;
                mSegVisStart[segCount] = sStart;
                mSegVisEnd[segCount] = sEnd;
                mSegCharStart[segCount] = bufStart;
                mSegCharEnd[segCount] = used;
                mSegNatWidth[segCount] = segNatW;
                natRtlSum += segNatW;
                segCount++;
            }
        }
        if (segCount == 0) return;

        float slotWidth = runColumns * mFontWidth;
        float availRtlWidth = Math.max(0f, slotWidth - fixedDelimSum);
        float scaleX = 1.0f;
        if (natRtlSum > 0f) {
            float rawScale = availRtlWidth / natRtlSum;
            scaleX = (rawScale < 1.0f) ? Math.max(0.1f, rawScale) : 1.0f;
        }
        float totalDrawnW = fixedDelimSum + natRtlSum * scaleX;
        int alignment = resolveRtlAlignment(visualCells, startCol, endCol, visualCells.length);
        float drawLeft = computeBalancedRtlLeftX(startCol, runColumns, mFontWidth, totalDrawnW, 1.0f, alignment);

        final float top = heightOffset - mFontLineSpacingAndAscent + mFontAscent;
        final float bottom = heightOffset;
        final float clipTop = top - mFontLineSpacing;
        final float clipBottom = bottom + mFontLineSpacing;
        final float slotLeft = startCol * mFontWidth;
        final float slotRight = endCol * mFontWidth;
        final float baseline = heightOffset - mFontLineSpacingAndAscent;
        final int defaultBg = palette[TextStyle.COLOR_INDEX_BACKGROUND];
        final boolean isBlockCursor = (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK);

        boolean hasOverlay = reverseVideo;
        for (int c = startCol; c < endCol; c++) {
            BidiLayout.LogicalCell cell = visualCells[c];
            if (cell.insideCursor || cell.insideSelection) hasOverlay = true;
            int back = resolveBackColor(TextStyle.decodeBackColor(cell.style), palette);
            if (back != defaultBg || (TextStyle.decodeEffect(cell.style) & TextStyle.CHARACTER_ATTRIBUTE_INVERSE) != 0) {
                hasOverlay = true;
            }
        }

        float curX = drawLeft;
        for (int s = 0; s < segCount; s++) {
            mSegLeft[s] = curX;
            int kind = mSegKind[s];
            int sStart = mSegVisStart[s];
            int sEnd = mSegVisEnd[s];
            float segW = (kind == SEG_DELIM) ? mSegNatWidth[s] : (mSegNatWidth[s] * scaleX);
            if (kind == SEG_DELIM || kind == SEG_SPACE) {
                mVisCellLeft[sStart] = curX;
                mVisCellRight[sStart] = curX + segW;
            } else {
                boolean segUniform = isSpanUniform(visualCells, startCol + sStart, startCol + sEnd, isBlockCursor);
                if (hasOverlay || !segUniform) {
                    computeVisualCellBounds(sStart, sEnd, mSegCharStart[s], mSegCharEnd[s],
                            mSegNatWidth[s], scaleX, curX, false);
                }
            }
            curX += segW;
        }

        if (hasOverlay) {
            drawRtlOverlays(canvas, visualCells, palette, startCol, endCol, runColumns,
                    cursorShape, reverseVideo, top, bottom, slotLeft, slotRight);
        }

        boolean needRtlScale = Math.abs(scaleX - 1.0f) > 0.005f;
        for (int s = 0; s < segCount; s++) {
            int kind = mSegKind[s];
            if (kind == SEG_SPACE) continue;
            int sStart = mSegVisStart[s];
            int sEnd = mSegVisEnd[s];
            if (kind == SEG_DELIM) {
                BidiLayout.LogicalCell dCell = visualCells[startCol + sStart];
                if (configurePaintForCell(dCell, palette, cursorShape, reverseVideo)) {
                    int dispCp = dCell.isRtl ? getMirroredAsciiChar(dCell.codePoint) : dCell.codePoint;
                    mCellMeasureBuffer[0] = (char) dispCp;
                    canvas.drawText(mCellMeasureBuffer, 0, 1, mSegLeft[s], baseline, mTextPaint);
                }
            } else {
                int segStartCol = startCol + sStart;
                int segEndCol = startCol + sEnd;
                boolean segUniform = isSpanUniform(visualCells, segStartCol, segEndCol, isBlockCursor);
                float segDrawnW = mSegNatWidth[s] * scaleX;
                drawRtlSpanSegments(canvas, visualCells, palette, startCol, endCol, segStartCol, segEndCol,
                        mRunCharBuffer, mSegCharStart[s], mSegCharEnd[s] - mSegCharStart[s],
                        scaleX, mSegLeft[s], segDrawnW, mSegLeft[s], mSegLeft[s] + segDrawnW,
                        needRtlScale, baseline, clipTop, clipBottom, cursorShape, reverseVideo, segUniform);
            }
        }
    }

    private void ensureSegCapacity(int cap) {
        if (mSegKind.length >= cap) return;
        int newCap = Math.max(cap, mSegKind.length * 2);
        mSegKind = new int[newCap];
        mSegVisStart = new int[newCap];
        mSegVisEnd = new int[newCap];
        mSegCharStart = new int[newCap];
        mSegCharEnd = new int[newCap];
        mSegNatWidth = new float[newCap];
        mSegLeft = new float[newCap];
    }

    private void sortRunCellsLogical(int count) {
        if (count <= 1) return;
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

    private static boolean isSpanUniform(BidiLayout.LogicalCell[] visualCells, int segStartCol, int segEndCol,
                                         boolean isBlockCursor) {
        BidiLayout.LogicalCell first = visualCells[segStartCol];
        long firstStyle = first.style;
        boolean refInv = first.insideSelection ^ (first.insideCursor && isBlockCursor);
        for (int c = segStartCol + 1; c < segEndCol; c++) {
            BidiLayout.LogicalCell cell = visualCells[c];
            boolean cellInv = cell.insideSelection ^ (cell.insideCursor && isBlockCursor);
            if (cell.style != firstStyle || cellInv != refInv) return false;
        }
        return true;
    }

    private void drawRtlOverlays(Canvas canvas, BidiLayout.LogicalCell[] visualCells, int[] palette,
                                 int startCol, int endCol, int runColumns, int cursorShape,
                                 boolean reverseVideo, float top, float bottom,
                                 float slotLeft, float slotRight) {
        final int totalColumns = visualCells.length;
        final int defaultBg = palette[TextStyle.COLOR_INDEX_BACKGROUND];
        final int cursorColor = palette[TextStyle.COLOR_INDEX_CURSOR];
        final boolean isBlockCursor = (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK);

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
                float cRight = (gRight - gLeft < 2f) ? Math.max(gRight, gLeft + mFontWidth * 0.45f) : gRight;
                if (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE) {
                    canvas.drawRect(gLeft, bottom - cursorHeight / 4.f, cRight, bottom, mTextPaint);
                } else if (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR) {
                    canvas.drawRect(gLeft, bottom - cursorHeight, gLeft + mFontWidth / 4.f, bottom, mTextPaint);
                } else {
                    canvas.drawRect(gLeft, bottom - cursorHeight, cRight, bottom, mTextPaint);
                }
            }
        }
    }

    private void drawRtlSpanSegments(Canvas canvas, BidiLayout.LogicalCell[] visualCells, int[] palette,
                                     int runStartCol, int runEndCol, int segStartCol, int segEndCol,
                                     char[] text, int bufOffset, int bufLen,
                                     float scaleX, float drawLeft, float drawnWidth,
                                     float slotLeft, float slotRight, boolean needScale,
                                     float baseline, float clipTop, float clipBottom,
                                     int cursorShape, boolean reverseVideo, boolean uniform) {
        if (bufLen <= 0) return;
        if (uniform) {
            drawRtlSegment(canvas, visualCells[segStartCol], palette, text, bufOffset, bufLen,
                    scaleX, drawLeft, needScale, baseline, cursorShape, reverseVideo, false,
                    0f, 0f, 0f, 0f);
            return;
        }
        final boolean isBlockCursor = (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK);
        int subStart = segStartCol;
        while (subStart < segEndCol) {
            BidiLayout.LogicalCell startCell = visualCells[subStart];
            boolean startInv = startCell.insideSelection ^ (startCell.insideCursor && isBlockCursor);
            int subEnd = subStart + 1;
            while (subEnd < segEndCol) {
                BidiLayout.LogicalCell next = visualCells[subEnd];
                boolean nextInv = next.insideSelection ^ (next.insideCursor && isBlockCursor);
                if (next.style != startCell.style || nextInv != startInv) break;
                subEnd++;
            }
            float clipL = (subStart == segStartCol)
                    ? Math.min(drawLeft, slotLeft) - ((segStartCol == runStartCol) ? mFontWidth : mFontWidth * 0.15f)
                    : mVisCellLeft[subStart - runStartCol];
            float clipR = (subEnd == segEndCol)
                    ? Math.max(drawLeft + drawnWidth, slotRight) + ((segEndCol == runEndCol) ? mFontWidth : mFontWidth * 0.15f)
                    : mVisCellRight[subEnd - 1 - runStartCol];
            drawRtlSegment(canvas, startCell, palette, text, bufOffset, bufLen,
                    scaleX, drawLeft, needScale, baseline, cursorShape, reverseVideo, true,
                    clipL, clipTop, clipR, clipBottom);
            subStart = subEnd;
        }
    }

    private void computeVisualCellBounds(int vStart, int vEnd, int bufStart, int bufEnd,
                                         float measuredWidth, float scaleX, float segLeft,
                                         boolean mapViaRunCell) {
        boolean useRunAdvance = (mUseApi23TextRun && measuredWidth > 0f);
        int span = vEnd - vStart;
        float drawnWidth = measuredWidth * scaleX;
        for (int i = vStart; i < vEnd; i++) {
            float gLeft;
            float gRight;
            int idx = mapViaRunCell ? mVisualToRunCell[i] : i;
            if (useRunAdvance && idx >= 0) {
                int cStart = mCellCharStart[idx];
                int cEnd = mCellCharEnd[idx];
                float advStart = (cStart <= bufStart) ? 0f
                        : mTextPaint.getRunAdvance(mRunCharBuffer, bufStart, bufEnd, bufStart, bufEnd, true, cStart);
                float advEnd = (cEnd >= bufEnd) ? measuredWidth
                        : mTextPaint.getRunAdvance(mRunCharBuffer, bufStart, bufEnd, bufStart, bufEnd, true, cEnd);
                float loAdv = Math.min(advStart, advEnd);
                float hiAdv = Math.max(advStart, advEnd);
                gLeft = segLeft + Math.max(0f, measuredWidth - hiAdv) * scaleX;
                gRight = segLeft + Math.min(measuredWidth, measuredWidth - loAdv) * scaleX;
            } else {
                float rel = i - vStart;
                gLeft = segLeft + drawnWidth * (rel / span);
                gRight = segLeft + drawnWidth * ((rel + 1) / span);
            }
            mVisCellLeft[i] = gLeft;
            mVisCellRight[i] = gRight;
        }
    }

    private boolean configurePaintForCell(BidiLayout.LogicalCell repCell, int[] palette,
                                          int cursorShape, boolean reverseVideo) {
        long style = repCell.style;
        int effect = TextStyle.decodeEffect(style);
        if ((effect & TextStyle.CHARACTER_ATTRIBUTE_INVISIBLE) != 0) return false;

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
        return true;
    }

    private void drawRtlSegment(Canvas canvas, BidiLayout.LogicalCell repCell, int[] palette,
                                char[] text, int offset, int used, float scaleX, float drawLeft,
                                boolean needScale, float baseline, int cursorShape, boolean reverseVideo,
                                boolean clip, float clipL, float clipT, float clipR, float clipB) {
        if (!configurePaintForCell(repCell, palette, cursorShape, reverseVideo)) return;

        boolean needSave = clip || needScale;
        if (needSave) canvas.save();
        if (clip) canvas.clipRect(clipL, clipT, clipR, clipB);
        float x = drawLeft;
        if (needScale) {
            canvas.scale(scaleX, 1.0f);
            x /= scaleX;
        }

        if (mUseApi23TextRun) {
            canvas.drawTextRun(text, offset, used, offset, used, x, baseline, true, mTextPaint);
        } else {
            canvas.drawText(text, offset, used, x, baseline, mTextPaint);
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

            if (mUseApi23TextRun) {
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

    private BidiLayout resolveRowLayoutForQuery(TerminalEmulator mEmulator, int row) {
        if (mEmulator == null || mEmulator.mColumns <= 0) return null;
        TerminalBuffer screen = mEmulator.getScreen();
        if (screen == null || row < -screen.getActiveTranscriptRows() || row >= mEmulator.mRows) return null;

        int internalRow = screen.externalToInternalRow(row);
        if (internalRow < 0 || internalRow >= screen.getActiveRows()) return null;

        TerminalRow rowObject = screen.allocateFullLineIfNecessary(internalRow);
        if (rowObject == null) return null;

        int inheritedCtx = BidiLayout.resolveInheritedContext(screen, row, mEmulator.mColumns);
        byte inheritedDir = (byte) (inheritedCtx & BidiLayout.INHERITED_DIR_MASK);
        boolean inheritedPrompt = (inheritedCtx & BidiLayout.INHERITED_PROMPT_FLAG) != 0;
        if (!rowObject.mMayContainRtlOrBidi && inheritedDir != BidiLayout.DIR_RTL) {
            return null;
        }

        if (rowObject.mCachedBidiLayout instanceof BidiLayout) {
            BidiLayout cached = (BidiLayout) rowObject.mCachedBidiLayout;
            if (cached.inputParagraphDirection == inheritedDir
                    && cached.inputParagraphStartsWithPrompt == inheritedPrompt
                    && cached.visualToLogical.length == mEmulator.mColumns) {
                return cached;
            }
        }

        return BidiLayout.build(rowObject, mEmulator.mColumns, -1, false,
                -1, -1, -1, -1, row, false, inheritedDir, inheritedPrompt);
    }

    public int translateVisualToLogicalColumn(TerminalEmulator mEmulator, int visualCol, int row) {
        if (visualCol < 0) return 0;
        if (mEmulator == null || visualCol >= mEmulator.mColumns) {
            return (mEmulator != null) ? mEmulator.mColumns : Math.max(0, visualCol);
        }
        BidiLayout layout = resolveRowLayoutForQuery(mEmulator, row);
        return (layout != null) ? layout.visualToLogical[visualCol] : visualCol;
    }

    public int translateLogicalToVisualColumn(TerminalEmulator mEmulator, int logicalCol, int row) {
        if (logicalCol < 0) return 0;
        if (mEmulator == null || logicalCol >= mEmulator.mColumns) {
            return (mEmulator != null) ? mEmulator.mColumns : Math.max(0, logicalCol);
        }
        BidiLayout layout = resolveRowLayoutForQuery(mEmulator, row);
        return (layout != null) ? layout.logicalToVisual[logicalCol] : logicalCol;
    }

    public int translateXToVisualColumn(TerminalEmulator mEmulator, float x, int row) {
        return translateXToVisualColumn(mEmulator, x, row, false);
    }

    public int translateXToVisualColumn(TerminalEmulator mEmulator, float x, int row, boolean clampToActive) {
        if (mEmulator == null || mEmulator.mColumns <= 0) return 0;
        int columns = mEmulator.mColumns;
        BidiLayout layout = resolveRowLayoutForQuery(mEmulator, row);
        if (layout == null) {
            int col = (mFontWidth > 0f) ? (int) (x / mFontWidth) : 0;
            return Math.max(0, Math.min(columns - 1, col));
        }
        BidiLayout.PixelGeometry geo = ensureRowPixelGeometry(layout, columns, mEmulator.isAlternateBufferActive());
        return geo.findVisualColAtX(x, clampToActive, layout.trimmedActiveLength);
    }

    public int translateXToLogicalColumn(TerminalEmulator mEmulator, float x, int row) {
        if (mEmulator == null || mEmulator.mColumns <= 0) return 0;
        int columns = mEmulator.mColumns;
        BidiLayout layout = resolveRowLayoutForQuery(mEmulator, row);
        if (layout == null) {
            int col = (mFontWidth > 0f) ? (int) (x / mFontWidth) : 0;
            return Math.max(0, Math.min(columns - 1, col));
        }
        BidiLayout.PixelGeometry geo = ensureRowPixelGeometry(layout, columns, mEmulator.isAlternateBufferActive());
        int vCol = geo.findVisualColAtX(x, false, layout.trimmedActiveLength);
        return layout.visualToLogical[vCol];
    }

    public float getVisualColumnLeftX(TerminalEmulator mEmulator, int visualCol, int row) {
        if (mEmulator == null || mEmulator.mColumns <= 0 || visualCol < 0) return 0f;
        int columns = mEmulator.mColumns;
        if (visualCol >= columns) return columns * mFontWidth;
        BidiLayout layout = resolveRowLayoutForQuery(mEmulator, row);
        if (layout == null) return visualCol * mFontWidth;
        BidiLayout.PixelGeometry geo = ensureRowPixelGeometry(layout, columns, mEmulator.isAlternateBufferActive());
        return geo.visLeft[visualCol];
    }

    public float getVisualColumnRightX(TerminalEmulator mEmulator, int visualCol, int row) {
        if (mEmulator == null || mEmulator.mColumns <= 0 || visualCol < 0) return 0f;
        int columns = mEmulator.mColumns;
        if (visualCol >= columns) return columns * mFontWidth;
        BidiLayout layout = resolveRowLayoutForQuery(mEmulator, row);
        if (layout == null) return (visualCol + 1) * mFontWidth;
        BidiLayout.PixelGeometry geo = ensureRowPixelGeometry(layout, columns, mEmulator.isAlternateBufferActive());
        return geo.visRight[visualCol];
    }

    public float getLogicalColumnLeftX(TerminalEmulator mEmulator, int logicalCol, int row) {
        if (mEmulator == null || mEmulator.mColumns <= 0 || logicalCol < 0) return 0f;
        int columns = mEmulator.mColumns;
        if (logicalCol >= columns) return columns * mFontWidth;
        BidiLayout layout = resolveRowLayoutForQuery(mEmulator, row);
        if (layout == null) return logicalCol * mFontWidth;
        BidiLayout.PixelGeometry geo = ensureRowPixelGeometry(layout, columns, mEmulator.isAlternateBufferActive());
        int vCol = layout.logicalToVisual[logicalCol];
        return geo.visLeft[vCol];
    }
}
