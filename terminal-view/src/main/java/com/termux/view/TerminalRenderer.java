package com.termux.view;

import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.PorterDuff;
import android.graphics.Typeface;

import com.termux.terminal.TerminalBuffer;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalRow;
import com.termux.terminal.TextStyle;
import com.termux.terminal.WcWidth;

/**
 * Renders a {@link TerminalEmulator} into a {@link Canvas}.
 *
 * <p>Caches font metrics; must be recreated whenever the typeface or font size changes.
 *
 * <p>Rendering proceeds row by row. Within each row, cells with identical style, directionality,
 * and width-fit are batched into a single <em>run</em> and drawn in one {@link Canvas#drawTextRun}
 * call. RTL runs are reordered into logical order before drawing so that the platform shaping
 * engine can apply correct cursive joining.
 *
 * <p>Width measurement uses a pre-computed lookup table ({@link #asciiMeasures}) for plain ASCII
 * cells to avoid calling {@link Paint#measureText} on every cell every frame. The expensive
 * measurement path is taken only for non-ASCII, RTL, or combining-character cells.
 */
public final class TerminalRenderer {

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

    /**
     * Pre-computed advance widths for the first 127 ASCII codepoints.
     * Indexed directly by codepoint value; avoids per-cell {@link Paint#measureText} overhead
     * for the common case of standard terminal output.
     */
    private final float[] asciiMeasures = new float[127];

    /**
     * Translucent highlight used to mark selected cells inside RTL runs. RTL text cannot be
     * rendered with the inverse-video trick used for LTR runs because splitting a shaped run to
     * invert part of it breaks cursive letter joining, so selection is drawn as a background
     * overlay instead.
     */
    private static final int RTL_SELECTION_HIGHLIGHT_COLOR = 0x6680B0FF;

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

        if (reverseVideo)
            canvas.drawColor(palette[TextStyle.COLOR_INDEX_FOREGROUND], PorterDuff.Mode.SRC);

        float heightOffset = mFontLineSpacingAndAscent;
        for (int row = topRow; row < endRow; row++) {
            heightOffset += mFontLineSpacing;

            TerminalRow lineObject = screen.allocateFullLineIfNecessary(screen.externalToInternalRow(row));
            BidiLayout layout = BidiLayout.build(lineObject, columns, cursorCol,
                    row == cursorRow && cursorVisible, selectionY1, selectionY2, selectionX1, selectionX2, row, true);
            BidiLayout.LogicalCell[] visualCells = layout.visualCells;

            long lastRunStyle = 0;
            boolean lastRunInsideCursor = false;
            boolean lastRunInsideSelection = false;
            boolean lastRunIsRtl = false;
            int lastRunStartColumn = -1;
            boolean lastRunFontWidthMismatch = false;

            for (int vCol = 0; vCol < columns; ) {
                BidiLayout.LogicalCell cell = visualCells[vCol];
                if (cell.displayWidth == 0 && cell.codePoint == 0) {
                    vCol++;
                    continue;
                }

                final boolean insideCursor = cell.insideCursor;
                final boolean insideSelection = cell.insideSelection;
                final long style = cell.style;
                final int codePoint = cell.codePoint;
                final int codePointWcWidth = cell.displayWidth;
                final boolean isRtl = cell.isRtl;

                // Measure the advance width of this cell. ASCII cells without combining characters
                // use the pre-computed lookup table; all other cells call measureText.
                final float measuredCodePointWidth;
                if (codePoint == 0) {
                    measuredCodePointWidth = 0f;
                } else if (codePoint < asciiMeasures.length && cell.combiningChars == null) {
                    measuredCodePointWidth = asciiMeasures[codePoint];
                } else {
                    int cap = Character.charCount(codePoint)
                            + (cell.combiningChars != null ? cell.combiningCount * 2 : 0);
                    char[] buf = new char[cap];
                    int len = Character.toChars(codePoint, buf, 0);
                    if (cell.combiningChars != null) {
                        for (int i = 0; i < cell.combiningCount; i++)
                            len += Character.toChars(cell.combiningChars[i], buf, len);
                    }
                    measuredCodePointWidth = mTextPaint.measureText(buf, 0, len);
                }

                final boolean fontWidthMismatch = !isRtl && codePointWcWidth > 0
                        && Math.abs(measuredCodePointWidth / mFontWidth - codePointWcWidth) > 0.01;

                // Split runs on style, directionality or font width changes. RTL runs are never
                // split at the cursor or selection boundaries: splitting a shaped RTL word (e.g.
                // Persian) breaks cursive letter joining, so for RTL runs the cursor and selection
                // are drawn as overlays on top of the finished run instead.
                final boolean rtlRun = isRtl && lastRunIsRtl;
                final boolean styleOrDirectionChanged = style != lastRunStyle
                        || isRtl != lastRunIsRtl
                        || fontWidthMismatch
                        || lastRunFontWidthMismatch;
                final boolean cellStateChanged = !rtlRun
                        && (insideCursor != lastRunInsideCursor
                        || insideSelection != lastRunInsideSelection);

                if (styleOrDirectionChanged || cellStateChanged) {
                    if (vCol != 0) {
                        flushRun(canvas, mEmulator, visualCells, palette, heightOffset,
                                lastRunStartColumn, vCol, lastRunStyle,
                                lastRunInsideCursor, lastRunInsideSelection,
                                lastRunIsRtl, lastRunFontWidthMismatch,
                                cursorShape, reverseVideo);
                    }
                    lastRunStyle = style;
                    lastRunInsideCursor = insideCursor;
                    lastRunInsideSelection = insideSelection;
                    lastRunIsRtl = isRtl;
                    lastRunStartColumn = vCol;
                    lastRunFontWidthMismatch = fontWidthMismatch;
                }

                vCol += codePointWcWidth;
            }

            if (columns > lastRunStartColumn) {
                flushRun(canvas, mEmulator, visualCells, palette, heightOffset,
                        lastRunStartColumn, columns, lastRunStyle,
                        lastRunInsideCursor, lastRunInsideSelection,
                        lastRunIsRtl, lastRunFontWidthMismatch,
                        cursorShape, reverseVideo);
            }
        }
    }

    /**
     * Collects cells in the column range [{@code startCol}, {@code endCol}), builds a UTF-16
     * character buffer, measures the run width, and delegates to {@link #drawTextRun}.
     *
     * <p>RTL runs are insertion-sorted into logical (left-to-right) column order before encoding
     * so that {@link Canvas#drawTextRun} receives characters in the order the shaping engine
     * expects for correct bidirectional cursive rendering.
     *
     * <p>The character buffer is sized precisely by summing {@link Character#charCount} over every
     * base codepoint and each of its combining characters, so supplementary-plane codepoints that
     * require a surrogate pair are always accommodated without overflow.
     */
    private void flushRun(Canvas canvas,
                          TerminalEmulator emulator,
                          BidiLayout.LogicalCell[] visualCells,
                          int[] palette,
                          float heightOffset,
                          int startCol, int endCol,
                          long style,
                          boolean insideCursor, boolean insideSelection,
                          boolean isRtl, boolean fontWidthMismatch,
                          int cursorShape, boolean reverseVideo) {

        final int runColumns = endCol - startCol;

        // For RTL runs the cursor cell is located on the visual row (it can be anywhere inside the
        // run since RTL runs are not split at the cursor), so it is resolved here and drawn as an
        // overlay on top of the finished run in drawRtlCursorAndSelectionOverlays().
        final int cursorColor;
        final boolean invertCursorTextColor;
        if (isRtl) {
            int cursorVisualCol = -1;
            for (int c = startCol; c < endCol; c++) {
                if (visualCells[c].insideCursor) {
                    cursorVisualCol = c;
                    break;
                }
            }
            cursorColor = cursorVisualCol >= 0
                    ? emulator.mColors.mCurrentColors[TextStyle.COLOR_INDEX_CURSOR] : 0;
            invertCursorTextColor = false;
        } else {
            cursorColor = insideCursor
                    ? emulator.mColors.mCurrentColors[TextStyle.COLOR_INDEX_CURSOR] : 0;
            invertCursorTextColor =
                    insideCursor && cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK;
        }

        // Collect non-empty cells.
        int count = 0;
        BidiLayout.LogicalCell[] cells = new BidiLayout.LogicalCell[runColumns];
        for (int c = startCol; c < endCol; c++) {
            BidiLayout.LogicalCell rc = visualCells[c];
            if (rc.displayWidth == 0 && rc.codePoint == 0) continue;
            cells[count++] = rc;
        }

        // Reorder RTL cells into logical column order for the shaping engine.
        if (isRtl) {
            for (int i = 1; i < count; i++) {
                BidiLayout.LogicalCell key = cells[i];
                int j = i - 1;
                while (j >= 0 && cells[j].originalColumn > key.originalColumn) {
                    cells[j + 1] = cells[j];
                    j--;
                }
                cells[j + 1] = key;
            }
        }

        // Compute exact buffer capacity: each codepoint (base or combining) may need 2 chars.
        int capacity = 0;
        for (int i = 0; i < count; i++) {
            BidiLayout.LogicalCell rc = cells[i];
            if (rc.codePoint == 0) {
                capacity += 1;
            } else {
                capacity += Character.charCount(rc.codePoint);
                if (rc.combiningChars != null) {
                    for (int k = 0; k < rc.combiningCount; k++)
                        capacity += Character.charCount(rc.combiningChars[k]);
                }
            }
        }

        char[] runBuffer = new char[capacity];
        int used = 0;
        for (int i = 0; i < count; i++) {
            BidiLayout.LogicalCell rc = cells[i];
            if (rc.codePoint != 0) {
                used += Character.toChars(rc.codePoint, runBuffer, used);
                if (rc.combiningChars != null) {
                    for (int k = 0; k < rc.combiningCount; k++)
                        used += Character.toChars(rc.combiningChars[k], runBuffer, used);
                }
            } else if (rc.displayWidth > 0) {
                runBuffer[used++] = ' ';
            }
        }

        // Switch to the system default typeface for RTL runs to enable native cursive shaping.
        final Typeface originalTypeface = mTextPaint.getTypeface();
        if (isRtl) mTextPaint.setTypeface(Typeface.DEFAULT);

        // Determine the run's rendered advance width.
        final float measuredWidth;
        if (isRtl) {
            measuredWidth = mTextPaint.measureText(runBuffer, 0, used);
        } else if (fontWidthMismatch) {
            float total = 0f;
            for (int i = 0; i < count; i++) {
                BidiLayout.LogicalCell rc = cells[i];
                if (rc.codePoint != 0) {
                    int cap = Character.charCount(rc.codePoint)
                            + (rc.combiningChars != null ? rc.combiningCount * 2 : 0);
                    char[] t = new char[cap];
                    int tl = Character.toChars(rc.codePoint, t, 0);
                    if (rc.combiningChars != null) {
                        for (int k = 0; k < rc.combiningCount; k++)
                            tl += Character.toChars(rc.combiningChars[k], t, tl);
                    }
                    total += mTextPaint.measureText(t, 0, tl);
                } else {
                    total += mFontWidth;
                }
            }
            measuredWidth = total;
        } else {
            measuredWidth = runColumns * mFontWidth;
        }

        drawTextRun(canvas, runBuffer, palette, heightOffset,
                startCol, runColumns, 0, used, measuredWidth,
                isRtl ? 0 : cursorColor, cursorShape, style,
                reverseVideo || (isRtl ? false : invertCursorTextColor || insideSelection), isRtl);

        if (isRtl) {
            mTextPaint.setTypeface(originalTypeface);
            drawRtlCursorAndSelectionOverlays(canvas, visualCells, startCol, endCol, palette,
                    heightOffset, cursorShape, cursorColor, reverseVideo);
        }
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

        if ((foreColor & 0xff000000) != 0xff000000) {
            // Bold text in the first 8 palette entries maps to the bright variant.
            if (bold && foreColor >= 0 && foreColor < 8) foreColor += 8;
            foreColor = palette[foreColor];
        }
        if ((backColor & 0xff000000) != 0xff000000) {
            backColor = palette[backColor];
        }

        // Reverse video is active when exactly one of the global and per-cell flags is set.
        final boolean reverseVideoHere = reverseVideo ^ (effect & TextStyle.CHARACTER_ATTRIBUTE_INVERSE) != 0;
        if (reverseVideoHere) {
            int tmp = foreColor;
            foreColor = backColor;
            backColor = tmp;
        }

        float left = startColumn * mFontWidth;
        float right = left + runWidthColumns * mFontWidth;

        // Scale the canvas horizontally if the font's advance differs from the cell grid width,
        // keeping the text centred within its allocated columns.
        mes = mes / mFontWidth;
        boolean savedMatrix = false;
        if (Math.abs(mes - runWidthColumns) > 0.01) {
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
            if (cursorStyle == TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE) cursorHeight /= 4.;
            else if (cursorStyle == TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR) right -= ((right - left) * 3) / 4.;
            canvas.drawRect(left, y - cursorHeight, right, y, mTextPaint);
        }

        if ((effect & TextStyle.CHARACTER_ATTRIBUTE_INVISIBLE) == 0) {
            if (dim) {
                // Dim colour algorithm from libvte / xterm:
                // https://bug735245.bugzilla-attachments.gnome.org/attachment.cgi?id=284267
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

            canvas.drawTextRun(text, startCharIndex, runWidthChars,
                    startCharIndex, runWidthChars,
                    left, y - mFontLineSpacingAndAscent, isRtl, mTextPaint);
        }

        if (savedMatrix) canvas.restore();
    }

    /**
     * Draws the selection highlight and caret as overlays for an RTL run. The run itself was
     * already drawn as a single shaped unit (preserving cursive joining); the overlays are drawn
     * on top in unscaled, absolute screen coordinates so they always line up with the run's cell
     * grid.
     */
    private void drawRtlCursorAndSelectionOverlays(Canvas canvas,
                                                   BidiLayout.LogicalCell[] visualCells,
                                                   int startCol, int endCol,
                                                   int[] palette, float heightOffset,
                                                   int cursorShape, int cursorColor,
                                                   boolean reverseVideo) {

        // Selection: a translucent highlight rect over the selected cells.
        int selStart = -1;
        for (int v = startCol; v <= endCol; v++) {
            boolean selected = v < endCol && visualCells[v].insideSelection;
            if (selected && selStart < 0) {
                selStart = v;
            } else if (!selected && selStart >= 0) {
                drawCellRect(canvas, selStart, v, heightOffset, RTL_SELECTION_HIGHLIGHT_COLOR);
                selStart = -1;
            }
        }

        // Caret: drawn at the exact cell that holds the cursor.
        for (int v = startCol; v < endCol; v++) {
            BidiLayout.LogicalCell rc = visualCells[v];
            if (!rc.insideCursor) continue;

            float left = v * mFontWidth;
            int widthCells = rc.displayWidth > 0 ? rc.displayWidth : 1;
            float right = left + widthCells * mFontWidth;

            float cursorHeight = mFontLineSpacingAndAscent - mFontAscent;
            if (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE) cursorHeight /= 4.;
            else if (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR) right -= ((right - left) * 3) / 4.;

            mTextPaint.setColor(cursorColor);
            canvas.drawRect(left, heightOffset - cursorHeight, right, heightOffset, mTextPaint);

            if (cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK && rc.codePoint != 0) {
                // Redraw the caret cell's character in the cell background colour so the block
                // caret looks like an inverted character, as with LTR text.
                int backColor = TextStyle.decodeBackColor(rc.style);
                if ((backColor & 0xff000000) != 0xff000000) backColor = palette[backColor];
                if (reverseVideo) backColor = cursorColor;

                char[] buf = buildCharBuffer(rc);
                final Typeface originalTypeface = mTextPaint.getTypeface();
                mTextPaint.setTypeface(Typeface.DEFAULT);
                mTextPaint.setFakeBoldText(false);
                mTextPaint.setUnderlineText(false);
                mTextPaint.setTextSkewX(0.f);
                mTextPaint.setStrikeThruText(false);
                mTextPaint.setColor(backColor);
                canvas.drawText(buf, 0, buf.length, left, heightOffset - mFontLineSpacingAndAscent, mTextPaint);
                mTextPaint.setTypeface(originalTypeface);
            }
            break;
        }
    }

    private void drawCellRect(Canvas canvas, int startCol, int endCol, float rowBottom, int color) {
        float left = startCol * mFontWidth;
        float right = endCol * mFontWidth;
        float top = rowBottom - mFontLineSpacingAndAscent + mFontAscent;
        mTextPaint.setColor(color);
        canvas.drawRect(left, top, right, rowBottom, mTextPaint);
    }

    private static char[] buildCharBuffer(BidiLayout.LogicalCell rc) {
        int capacity = Character.charCount(rc.codePoint);
        if (rc.combiningChars != null) capacity += rc.combiningCount * 2;
        char[] buf = new char[capacity];
        int len = Character.toChars(rc.codePoint, buf, 0);
        if (rc.combiningChars != null) {
            for (int k = 0; k < rc.combiningCount; k++)
                len += Character.toChars(rc.combiningChars[k], buf, len);
        }
        return buf;
    }

    public float getFontWidth() {
        return mFontWidth;
    }

    public int getFontLineSpacing() {
        return mFontLineSpacing;
    }

    public int translateVisualToLogicalColumn(TerminalEmulator mEmulator, int visualCol, int row) {
        if (visualCol < 0) return 0;
        if (visualCol >= mEmulator.mColumns) return mEmulator.mColumns - 1;

        TerminalBuffer screen = mEmulator.getScreen();
        if (row < -screen.getActiveTranscriptRows() || row >= mEmulator.mRows) return visualCol;

        int internalRow = screen.externalToInternalRow(row);
        if (internalRow < 0 || internalRow >= screen.getActiveRows()) return visualCol;

        TerminalRow rowObject = screen.allocateFullLineIfNecessary(internalRow);
        if (rowObject == null) return visualCol;

        BidiLayout layout = BidiLayout.build(rowObject, mEmulator.mColumns, -1, false, -1, -1, -1, -1, -1, false);
        return layout.visualToLogical[visualCol];
    }

    public int translateLogicalToVisualColumn(TerminalEmulator mEmulator, int logicalCol, int row) {
        if (logicalCol < 0) return 0;
        if (logicalCol >= mEmulator.mColumns) return mEmulator.mColumns - 1;

        TerminalBuffer screen = mEmulator.getScreen();
        if (row < -screen.getActiveTranscriptRows() || row >= mEmulator.mRows) return logicalCol;

        int internalRow = screen.externalToInternalRow(row);
        if (internalRow < 0 || internalRow >= screen.getActiveRows()) return logicalCol;

        TerminalRow rowObject = screen.allocateFullLineIfNecessary(internalRow);
        if (rowObject == null) return logicalCol;

        BidiLayout layout = BidiLayout.build(rowObject, mEmulator.mColumns, -1, false, -1, -1, -1, -1, -1, false);
        return layout.logicalToVisual[logicalCol];
    }
}
