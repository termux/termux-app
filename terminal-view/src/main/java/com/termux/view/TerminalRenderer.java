package com.termux.view;

import android.annotation.TargetApi;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Typeface;
import android.os.Build;

import com.termux.terminal.TerminalBuffer;
import com.termux.terminal.TerminalEmulator;
import com.termux.terminal.TerminalRow;
import com.termux.terminal.TextStyle;

/**
 * Opt-in display-only bidi. The VT buffer, cursor reports and PTY text stay logical.
 * With the feature disabled, every paint operation uses the upstream native renderer.
 * Opted-in TUIs keep their grid and shape bounded RTL fields, not the whole screen.
 * All paths use native cell metrics, so toggling shaping never resizes the terminal.
 */
public final class TerminalRenderer {
    final int mTextSize;
    final Typeface mTypeface;
    private final Paint mTextPaint = new Paint();
    private final NativeTerminalRenderer mNativeRenderer;
    final float mFontWidth;
    final int mFontLineSpacing;
    private final int mFontAscent;
    final int mFontLineSpacingAndAscent;
    private boolean mRtlTextShapingEnabled;
    private BidiLineLayout.Run mDrawingFlowRun;

    public TerminalRenderer(int textSize, Typeface typeface) {
        this(textSize, typeface, false);
    }

    public TerminalRenderer(int textSize, Typeface typeface, boolean enabled) {
        mNativeRenderer = new NativeTerminalRenderer(textSize, typeface);
        mTextSize = textSize;
        mTypeface = typeface;
        mRtlTextShapingEnabled = enabled;
        mTextPaint.setTypeface(typeface);
        mTextPaint.setAntiAlias(true);
        mTextPaint.setTextSize(textSize);
        // Keep official cell geometry in BOTH modes (including the native baseline rounding).
        mFontLineSpacing = mNativeRenderer.mFontLineSpacing;
        mFontLineSpacingAndAscent = mNativeRenderer.mFontLineSpacingAndAscent;
        mFontAscent = mFontLineSpacingAndAscent - mFontLineSpacing;
        mFontWidth = mNativeRenderer.mFontWidth;
    }

    public void setRtlTextShapingEnabled(boolean enabled) { mRtlTextShapingEnabled = enabled; }
    public boolean isRtlTextShapingEnabled() { return mRtlTextShapingEnabled; }

    /** API 23+ rendering gate, independent of alternate-screen/mouse/application-key modes. */
    public boolean isRtlRenderingEnabled(TerminalEmulator emulator) {
        return emulator != null && TerminalPresentationPolicy.renderingAllowed(Build.VERSION.SDK_INT,
            mRtlTextShapingEnabled);
    }

    public boolean isRtlFlowEnabled(TerminalEmulator emulator) {
        return emulator != null && TerminalPresentationPolicy.flowAllowed(Build.VERSION.SDK_INT, mRtlTextShapingEnabled,
            emulator.isAlternateBufferActive(), emulator.isMouseTrackingActive(),
            emulator.isCursorKeysApplicationMode());
    }

    private TerminalRow rowAt(TerminalEmulator emulator, int row) {
        TerminalBuffer screen = emulator.getScreen();
        row = Math.max(-screen.getActiveTranscriptRows(), Math.min(emulator.mRows - 1, row));
        return screen.allocateFullLineIfNecessary(screen.externalToInternalRow(row));
    }

    public boolean isRtlFlowRow(TerminalEmulator emulator, int row) {
        if (!isRtlFlowEnabled(emulator)) return false;
        TerminalRow line = rowAt(emulator, row);
        return TerminalPresentationPolicy.hasStrongRtl(line.mText, 0, line.getSpaceUsed()) &&
            !TerminalPresentationPolicy.looksLikeGrid(line.mText, line.getSpaceUsed());
    }

    /** True for a shaped paragraph OR bounded RTL fields. ASCII-only rows remain native. */
    public boolean isRtlShapedRow(TerminalEmulator emulator, int row) {
        if (!isRtlRenderingEnabled(emulator)) return false;
        TerminalRow line = rowAt(emulator, row);
        return TerminalPresentationPolicy.hasStrongRtl(line.mText, 0, line.getSpaceUsed());
    }

    private void resetMetricsPaint() {
        mTextPaint.setTextScaleX(1f);
        mTextPaint.setFakeBoldText(false);
        mTextPaint.setTextSkewX(0f);
        mTextPaint.setUnderlineText(false);
        mTextPaint.setStrikeThruText(false);
    }

    // All shaping callers are gated by isRtlRenderingEnabled (API 23+).
    @TargetApi(Build.VERSION_CODES.M)
    private BidiLineLayout layout(TerminalEmulator emulator, int row, TerminalRow line) {
        // Keep cursor cell even during blink-off: blinking must never move or re-shape text.
        int lastRequired = row == emulator.getCursorRow() ? emulator.getCursorCol() : -1;
        // Backgrounds/erased colored cells remain part of the display, not trimmed as whitespace.
        for (int c = 0; c < emulator.mColumns; c++) {
            long style = line.getStyle(c);
            if (TextStyle.decodeBackColor(style) != TextStyle.COLOR_INDEX_BACKGROUND ||
                (TextStyle.decodeEffect(style) & (TextStyle.CHARACTER_ATTRIBUTE_INVERSE |
                    TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE | TextStyle.CHARACTER_ATTRIBUTE_STRIKETHROUGH)) != 0)
                lastRequired = Math.max(lastRequired, c);
        }
        int cursorColumn = row == emulator.getCursorRow() ? emulator.getCursorCol() : -1;
        BidiLineLayout result = new BidiLineLayout(line.mText, line.getSpaceUsed(), emulator.mColumns,
            cursorColumn, lastRequired);
        float[] advances = new float[result.textLength];
        resetMetricsPaint();
        for (BidiLineLayout.Run run : result.runs) {
            mTextPaint.getTextRunAdvances(line.mText, run.start, run.end - run.start,
                run.start, run.end - run.start, run.rtl, advances, run.start);
        }
        result.place(mFontWidth, emulator.mColumns * mFontWidth, advances);
        return result;
    }

    public boolean isRtlCursor(TerminalEmulator emulator) {
        if (!isRtlFlowEnabled(emulator)) return false;
        TerminalRow line = rowAt(emulator, emulator.getCursorRow());
        // Paragraph-logical arrows stay consistent across Arabic/Latin tokens and empty input.
        // Grid/TUI arrows, Home/End and modifiers always retain native VT key semantics.
        return !TerminalPresentationPolicy.looksLikeGrid(line.mText, line.getSpaceUsed()) &&
            TerminalPresentationPolicy.rtlArrows(line.mText, line.getSpaceUsed());
    }

    @TargetApi(Build.VERSION_CODES.M)
    private GridBidiLayout gridLayout(TerminalEmulator emulator, int row, TerminalRow line) {
        int cursor = row == emulator.getCursorRow() ? emulator.getCursorCol() : -1;
        GridBidiLayout result = new GridBidiLayout(line.mText, line.getSpaceUsed(), emulator.mColumns, cursor);
        float[][] advances = new float[result.fields.size()][];
        resetMetricsPaint();
        for (int i = 0; i < result.fields.size(); i++) {
            GridBidiLayout.Field field = result.fields.get(i);
            advances[i] = new float[field.layout.textLength];
            for (BidiLineLayout.Run run : field.layout.runs) {
                mTextPaint.getTextRunAdvances(field.text, run.start, run.end - run.start,
                    run.start, run.end - run.start, run.rtl, advances[i], run.start);
            }
        }
        result.place(mFontWidth, advances);
        return result;
    }

    public int logicalColumnAt(TerminalEmulator emulator, int row, float x) {
        TerminalRow line = rowAt(emulator, row);
        if (!isRtlShapedRow(emulator, row)) {
            int target = Math.max(0, Math.min(emulator.mColumns - 1, (int) (x / mFontWidth)));
            return TerminalPresentationPolicy.cellRange(line.mText, line.getSpaceUsed(), target)[0];
        }
        return isRtlFlowRow(emulator, row) ? layout(emulator, row, line).columnAt(x)
            : gridLayout(emulator, row, line).columnAt(x);
    }

    /** Mouse reports are raw VT cells outside actual shaped allocations, not selection owners. */
    public int mouseColumnAt(TerminalEmulator emulator, int row, float x) {
        if (!isRtlShapedRow(emulator, row)) return (int) (x / mFontWidth);
        TerminalRow line = rowAt(emulator, row);
        return isRtlFlowRow(emulator, row) ? layout(emulator, row, line).columnAt(x)
            : gridLayout(emulator, row, line).mouseColumnAt(x);
    }

    public float logicalBoundaryX(TerminalEmulator emulator, int row, int column, boolean trailing) {
        column = Math.max(0, Math.min(emulator.mColumns - 1, column));
        TerminalRow line = rowAt(emulator, row);
        if (!isRtlShapedRow(emulator, row)) {
            int[] range = TerminalPresentationPolicy.cellRange(line.mText, line.getSpaceUsed(), column);
            return range[trailing ? 1 : 0] * mFontWidth;
        }
        return isRtlFlowRow(emulator, row) ? layout(emulator, row, line).boundary(column, trailing)
            : gridLayout(emulator, row, line).boundary(column, trailing);
    }

    /** Visual bounds of an inclusive logical selection, using a single shared row layout. */
    public float[] selectionBoundsX(TerminalEmulator emulator, int row, int first, int last) {
        first = Math.max(0, Math.min(emulator.mColumns - 1, first));
        last = Math.max(first, Math.min(emulator.mColumns - 1, last));
        if (!isRtlShapedRow(emulator, row)) return new float[]{first * mFontWidth, (last + 1) * mFontWidth};
        TerminalRow line = rowAt(emulator, row);
        boolean flow = isRtlFlowRow(emulator, row);
        BidiLineLayout positions = flow ? layout(emulator, row, line) : null;
        GridBidiLayout grid = flow ? null : gridLayout(emulator, row, line);
        float left = emulator.mColumns * mFontWidth, right = 0;
        for (int c = first; c <= last; c++) {
            left = Math.min(left, flow ? positions.leftOf(c) : grid.leftOf(c));
            right = Math.max(right, flow ? positions.rightOf(c) : grid.rightOf(c));
        }
        return new float[]{left, right};
    }

    public void render(TerminalEmulator emulator, Canvas canvas, int topRow,
                       int selectionY1, int selectionY2, int selectionX1, int selectionX2) {
        if (!isRtlRenderingEnabled(emulator)) {
            mNativeRenderer.render(emulator, canvas, topRow, selectionY1, selectionY2, selectionX1, selectionX2);
            return;
        }
        // The opted-in frame is always cleared, including empty/ASCII frames after RTL deletion.
        // Native only paints non-default backgrounds; without this moving cursors leave trails.
        final int[] palette = emulator.mColors.mCurrentColors;
        canvas.drawColor(palette[emulator.isReverseVideo()
            ? TextStyle.COLOR_INDEX_FOREGROUND : TextStyle.COLOR_INDEX_BACKGROUND]);
        // Compute replacement allocations BEFORE native rasterization: erasing cell rectangles
        // afterwards cannot remove source glyph overhang without also destroying native anchors.
        BidiLineLayout[] flowRows = new BidiLineLayout[emulator.mRows];
        GridBidiLayout[] gridRows = new GridBidiLayout[emulator.mRows];
        boolean[][] omittedCells = new boolean[emulator.mRows][];
        for (int i = 0; i < emulator.mRows; i++) {
            int row = topRow + i;
            if (!isRtlShapedRow(emulator, row)) continue;
            TerminalRow line = rowAt(emulator, row);
            boolean[] omitted = omittedCells[i] = new boolean[emulator.mColumns];
            if (isRtlFlowRow(emulator, row)) {
                flowRows[i] = layout(emulator, row, line);
                java.util.Arrays.fill(omitted, true);
            } else {
                gridRows[i] = gridLayout(emulator, row, line);
                for (GridBidiLayout.Field field : gridRows[i].fields)
                    java.util.Arrays.fill(omitted, field.startColumn, field.endColumn, true);
            }
        }
        // Keep upstream rendering for ALL grid anchors, styles, English and untranslated UI.
        mNativeRenderer.renderRows(emulator, canvas, topRow, emulator.mRows,
            selectionY1, selectionY2, selectionX1, selectionX2, omittedCells);
        for (int i = 0; i < emulator.mRows; i++) {
            int row = topRow + i;
            if (omittedCells[i] == null) continue;
            float y = mFontLineSpacingAndAscent + (i + 1) * mFontLineSpacing;
            int cursor = row == emulator.getCursorRow() && emulator.shouldCursorBeVisible() ? emulator.getCursorCol() : -1;
            int sx1 = -1, sx2 = -1;
            if (row >= selectionY1 && row <= selectionY2) {
                sx1 = row == selectionY1 ? selectionX1 : 0;
                sx2 = row == selectionY2 ? selectionX2 : emulator.mColumns - 1;
            }
            TerminalRow line = rowAt(emulator, row);
            if (flowRows[i] != null) {
                canvas.save();
                canvas.clipRect(0, y - mFontLineSpacing, emulator.mColumns * mFontWidth, y);
                clearRegion(emulator, canvas, 0, emulator.mColumns * mFontWidth, y);
                drawFlowRow(emulator, canvas, line, line.mText, flowRows[i],
                    0, y, cursor, sx1, sx2);
                canvas.restore();
            } else {
                for (GridBidiLayout.Field field : gridRows[i].fields) {
                    float left = field.startColumn * mFontWidth, right = field.endColumn * mFontWidth;
                    canvas.save();
                    canvas.clipRect(left, y - mFontLineSpacing, right, y);
                    // Native already painted the original backgrounds without source text,
                    // cursor or selection. Do not erase neighboring native glyph overhang here.
                    canvas.translate(left, 0);
                    drawFlowRow(emulator, canvas, line, field.text, field.layout, field.startColumn,
                        y, cursor, sx1, sx2);
                    canvas.restore();
                }
            }
        }
    }

    private void clearRegion(TerminalEmulator emulator, Canvas canvas, float left, float right, float y) {
        mTextPaint.setColor(emulator.mColors.mCurrentColors[emulator.isReverseVideo()
            ? TextStyle.COLOR_INDEX_FOREGROUND : TextStyle.COLOR_INDEX_BACKGROUND]);
        canvas.drawRect(left, y - mFontLineSpacing, right, y, mTextPaint);
    }

    private void drawFlowRow(TerminalEmulator emulator, Canvas canvas, TerminalRow line, char[] text,
                             BidiLineLayout layout, int columnOffset, float y, int cursor, int sx1, int sx2) {
        int[] palette = emulator.mColors.mCurrentColors;
        int cursorStyle = emulator.getCursorStyle();
        for (BidiLineLayout.Run run : layout.runs) {
            // Coalesce cells with identical paint state. Cursor and selection change clipping only;
            // EVERY paint pass shapes the complete directional run (including lam-alef/context).
            int first = 0;
            while (first < run.cells.size()) {
                BidiLineLayout.Cell cell = run.cells.get(first);
                int column = cell.column + columnOffset;
                long style = line.getStyle(column);
                boolean selected = column <= sx2 && column + cell.width - 1 >= sx1 && sx1 >= 0;
                boolean insideCursor = cursor >= column && cursor < column + cell.width;
                float left = layout.leftOf(cell.column), right = layout.rightOf(cell.column);
                int next = first + 1;
                while (next < run.cells.size()) {
                    BidiLineLayout.Cell other = run.cells.get(next);
                    int otherColumn = other.column + columnOffset;
                    boolean otherSelected = otherColumn <= sx2 && otherColumn + other.width - 1 >= sx1 && sx1 >= 0;
                    boolean otherCursor = cursor >= otherColumn && cursor < otherColumn + other.width;
                    if (line.getStyle(otherColumn) != style || selected != otherSelected || insideCursor != otherCursor) break;
                    left = Math.min(left, layout.leftOf(other.column));
                    right = Math.max(right, layout.rightOf(other.column));
                    next++;
                }
                canvas.save();
                canvas.clipRect(left, y - mFontLineSpacing, right, y);
                mDrawingFlowRun = run;
                try {
                    // BAR is drawn separately at the logical leading edge (right edge in RTL).
                    int color = insideCursor && cursorStyle != TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR
                        ? palette[TextStyle.COLOR_INDEX_CURSOR] : 0;
                    drawRun(canvas, text, palette, y, left, right, run.start, run.end - run.start,
                        run.rtl, color, cursorStyle, style, emulator.isReverseVideo() || selected ||
                            (insideCursor && cursorStyle == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK));
                    if (insideCursor && cursorStyle == TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR) {
                        mTextPaint.setColor(palette[TextStyle.COLOR_INDEX_CURSOR]);
                        float width = Math.max(1f, Math.min(mFontWidth / 4, right - left));
                        float barLeft = run.rtl ? right - width : left;
                        canvas.drawRect(barLeft, y - mFontLineSpacing, barLeft + width, y, mTextPaint);
                    }
                } finally {
                    mDrawingFlowRun = null;
                    mTextPaint.setTextScaleX(1f);
                    canvas.restore();
                }
                first = next;
            }
        }
    }

    /**
     * Draw a run of text, its background and the cursor between the {@code left} and {@code right} x coordinates.
     *
     * @param rtl Whether the run is laid out from right to left, in which case the last logical char is drawn at {@code left}.
     */
    @TargetApi(Build.VERSION_CODES.M)
    private void drawRun(Canvas canvas, char[] text, int[] palette, float y, float left, float right,
                         int startCharIndex, int runWidthChars, boolean rtl, int cursor, int cursorStyle,
                         long textStyle, boolean reverseVideo) {
        int foreColor = TextStyle.decodeForeColor(textStyle);
        final int effect = TextStyle.decodeEffect(textStyle);
        int backColor = TextStyle.decodeBackColor(textStyle);
        final boolean bold = (effect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK)) != 0;
        final boolean underline = (effect & TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE) != 0;
        final boolean italic = (effect & TextStyle.CHARACTER_ATTRIBUTE_ITALIC) != 0;
        final boolean strikeThrough = (effect & TextStyle.CHARACTER_ATTRIBUTE_STRIKETHROUGH) != 0;
        final boolean dim = (effect & TextStyle.CHARACTER_ATTRIBUTE_DIM) != 0;

        if ((foreColor & 0xff000000) != 0xff000000) {
            // Let bold have bright colors if applicable (one of the first 8):
            if (bold && foreColor >= 0 && foreColor < 8) foreColor += 8;
            foreColor = palette[foreColor];
        }

        if ((backColor & 0xff000000) != 0xff000000) {
            backColor = palette[backColor];
        }

        // Reverse video here if _one and only one_ of the reverse flags are set:
        final boolean reverseVideoHere = reverseVideo ^ (effect & (TextStyle.CHARACTER_ATTRIBUTE_INVERSE)) != 0;
        if (reverseVideoHere) {
            int tmp = foreColor;
            foreColor = backColor;
            backColor = tmp;
        }

        // Native pixels were cleared; repaint even default color beneath each shaped slice.
        mTextPaint.setColor(backColor);
        canvas.drawRect(left, y - mFontLineSpacingAndAscent + mFontAscent, right, y, mTextPaint);

        if (cursor != 0) {
            mTextPaint.setColor(cursor);
            float cursorHeight = mFontLineSpacingAndAscent - mFontAscent;
            if (cursorStyle == TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE) cursorHeight /= 4.;
            float cursorRight = right;
            if (cursorStyle == TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR) cursorRight = left + (right - left) / 4;
            canvas.drawRect(left, y - cursorHeight, cursorRight, y, mTextPaint);
        }

        if ((effect & TextStyle.CHARACTER_ATTRIBUTE_INVISIBLE) == 0) {
            if (dim) {
                int red = (0xFF & (foreColor >> 16));
                int green = (0xFF & (foreColor >> 8));
                int blue = (0xFF & foreColor);
                // Dim color handling used by libvte which in turn took it from xterm
                // (https://bug735245.bugzilla-attachments.gnome.org/attachment.cgi?id=284267):
                red = red * 2 / 3;
                green = green * 2 / 3;
                blue = blue * 2 / 3;
                foreColor = 0xFF000000 + (red << 16) + (green << 8) + blue;
            }

            mTextPaint.setFakeBoldText(bold);
            mTextPaint.setUnderlineText(underline);
            mTextPaint.setTextSkewX(italic ? -0.35f : 0.f);
            mTextPaint.setStrikeThruText(strikeThrough);
            mTextPaint.setColor(foreColor);

            // Every clipped paint pass uses the FULL run as its shaping context. Never split
            // lam-alef/joined Arabic at a cursor, selection or color boundary.
            BidiLineLayout.Run run = mDrawingFlowRun;
            mTextPaint.setTextScaleX(run.scale);
            canvas.drawTextRun(text, run.start, run.end - run.start, run.start, run.end - run.start,
                run.left, y - mFontLineSpacingAndAscent, run.rtl, mTextPaint);
            mTextPaint.setTextScaleX(1f);
        }
    }

    public float getFontWidth() {
        return mFontWidth;
    }

    public int getFontLineSpacing() {
        return mFontLineSpacing;
    }

}
