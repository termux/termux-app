package com.termux.view;

import com.termux.terminal.WcWidth;

import java.text.Bidi;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/**
 * A display-only, RTL-paragraph layout of a logical terminal row. Offsets always refer to the
 * caller's original UTF-16 array; neither that array nor the order of characters is changed.
 * Android shaping belongs to the renderer, not this class. Call {@link #place} before querying
 * pixel positions, and again whenever the font, advances or viewport changes.
 *
 * <p>Terminal cells, rather than Unicode grapheme clusters, are authoritative: a positive
 * {@link WcWidth} starts a cell and following zero-width characters belong to that cell. Thus
 * supplementary characters stay intact and combining marks never acquire their own spacing.
 * Emoji ZWJ sequences retain the emulator's cell accounting, not grapheme-cluster accounting.
 * Explicit bidi controls inside a cell stay with their base; its base's resolved level determines
 * the direction of the entire cell (we must not split a terminal cell at a bidi control).
 */
public final class BidiLineLayout {

    public static final class Cell {
        public final int start;
        public final int end;
        public final int column;
        public final int width;

        private Cell(int start, int end, int column, int width) {
            this.start = start;
            this.end = end;
            this.column = column;
            this.width = width;
        }
    }

    public static final class Run {
        public final int start;
        public final int end;
        public final boolean rtl;
        public final int columnCount;
        /** Original logical order, including in RTL runs. */
        public final List<Cell> cells;
        /** Viewport-local extents; draw the shaped run at left with horizontal scale scale. */
        public float left;
        public float right;
        public float scale = 1f;

        private Run(List<Cell> cells, int level) {
            this.cells = Collections.unmodifiableList(new ArrayList<>(cells));
            start = cells.get(0).start;
            end = cells.get(cells.size() - 1).end;
            rtl = (level & 1) != 0;
            int count = 0;
            for (Cell cell : cells) count += cell.width;
            columnCount = count;
        }
    }

    /** Trimmed UTF-16 length, including marks attached to the final retained cell. */
    public final int textLength;
    /** Directional runs in visual left-to-right order. */
    public final List<Run> runs;
    /** Viewport-local hit extents indexed by logical terminal column; wide halves duplicate. */
    public final float[] cellLeft;
    public final float[] cellRight;

    private final List<Cell> logicalCells;
    private final boolean[] cellRtl;
    private final int[] owner;
    private final int columns;
    private final int contentColumns;
    private float placedWidth;

    /**
     * Trims only trailing U+0020 cells. A space carrying a combining mark is not trimmed.
     * cursorColumn < 0 means no cursor; other cursor columns are clamped to the row. Source
     * cells through the cursor are retained. A short/empty source is not padded or rewritten:
     * columns without source text are virtual blanks (see {@link #place}). Text beyond columns
     * is ignored; a wide cell that does not fit is omitted whole, never cut through a surrogate.
     */
    public BidiLineLayout(char[] text, int length, int columns, int cursorColumn) {
        this(text, length, columns, cursorColumn, cursorColumn);
    }

    /** Retain styled blanks without confusing their final column with the insertion cursor. */
    public BidiLineLayout(char[] text, int length, int columns, int cursorColumn, int lastRequiredColumn) {
        if (text == null) throw new NullPointerException("text");
        if (length < 0 || length > text.length || columns < 0)
            throw new IllegalArgumentException("Invalid length or columns");
        this.columns = columns;
        cellLeft = new float[columns];
        cellRight = new float[columns];
        cellRtl = new boolean[columns];
        owner = new int[columns];
        Arrays.fill(cellRtl, true);
        Arrays.fill(owner, -1);
        int cursor = cursorColumn < 0 || columns == 0 ? -1 : Math.min(cursorColumn, columns - 1);

        List<Cell> cells = new ArrayList<>();
        int start = 0, end = 0, column = 0, width = 0;
        for (int i = 0; i < length && columns > 0; ) {
            int codePoint = Character.codePointAt(text, i, length);
            int next = i + Character.charCount(codePoint);
            int w = WcWidth.width(codePoint);
            if (w > 0) {
                if (width > 0) {
                    cells.add(new Cell(start, i, column, width));
                    column += width;
                    start = i;
                    width = 0;
                }
                if (w > columns - column) break;
                width = w;
            }
            end = next;
            i = next;
        }
        if (end > start) cells.add(new Cell(start, end, column, width));

        int keep = 0;
        for (int i = 0; i < cells.size(); i++) {
            Cell cell = cells.get(i);
            boolean nonSpace = false;
            for (int j = cell.start; j < cell.end; j++) {
                if (text[j] != ' ') {
                    nonSpace = true;
                    break;
                }
            }
            if (nonSpace || cell.column <= Math.max(cursor, lastRequiredColumn)) keep = i + 1;
        }
        logicalCells = Collections.unmodifiableList(new ArrayList<>(cells.subList(0, keep)));
        textLength = keep == 0 ? 0 : logicalCells.get(keep - 1).end;
        contentColumns = keep == 0 ? 0 : logicalCells.get(keep - 1).column
            + logicalCells.get(keep - 1).width;

        List<Run> logicalRuns = new ArrayList<>();
        List<Byte> levels = new ArrayList<>();
        if (textLength > 0) {
            Bidi bidi = new Bidi(new String(text, 0, textLength), Bidi.DIRECTION_RIGHT_TO_LEFT);
            int[] resolved = new int[logicalCells.size()];
            for (int i = 0; i < resolved.length; i++) resolved[i] = levelOf(bidi, text, logicalCells.get(i));
            // The empty insertion cell is not authored paragraph whitespace. At the end of an
            // LTR token, keep it at that token's trailing edge rather than moving it to the
            // paragraph's left edge. Authored intervening spaces retain normal UAX #9 behavior.
            if (resolved.length > 1) {
                int last = resolved.length - 1;
                Cell insertion = logicalCells.get(last);
                Cell previous = logicalCells.get(last - 1);
                if (insertion.column == cursor && insertion.end == insertion.start + 1 &&
                    text[insertion.start] == ' ' && text[previous.start] != ' ' &&
                    (resolved[last - 1] & 1) == 0) resolved[last] = resolved[last - 1];
            }
            int first = 0, level = resolved[0];
            for (int i = 1; i <= logicalCells.size(); i++) {
                int nextLevel = i == logicalCells.size() ? -1 : resolved[i];
                if (nextLevel != level) {
                    Run run = new Run(logicalCells.subList(first, i), level);
                    logicalRuns.add(run);
                    levels.add((byte) level);
                    for (Cell cell : run.cells) {
                        for (int c = cell.column; c < cell.column + cell.width; c++) {
                            cellRtl[c] = run.rtl;
                            owner[c] = cell.column;
                        }
                    }
                    first = i;
                    level = nextLevel;
                }
            }
        }
        Run[] visual = logicalRuns.toArray(new Run[0]);
        byte[] runLevels = new byte[levels.size()];
        for (int i = 0; i < runLevels.length; i++) runLevels[i] = levels.get(i);
        if (visual.length > 0) Bidi.reorderVisually(runLevels, 0, visual, 0, visual.length);
        runs = Collections.unmodifiableList(Arrays.asList(visual));
    }

    private static int levelOf(Bidi bidi, char[] text, Cell cell) {
        // Leading orphan marks attach to the first base, but still have no columns of their own.
        for (int i = cell.start; i < cell.end; ) {
            int codePoint = Character.codePointAt(text, i, cell.end);
            if (WcWidth.width(codePoint) > 0) return bidi.getLevelAt(i);
            i += Character.charCount(codePoint);
        }
        return bidi.getLevelAt(cell.start);
    }

    /**
     * Places supplied shaped UTF-16 advances. Each run must be measured using its original
     * logical substring, its rtl flag and full directional shaping context. Advances from every
     * run must be written at original UTF-16 offsets. Only a uniform whole-line shrink is used;
     * natural Arabic text is never expanded to terminal-grid width. Invalid/negative individual
     * advances are treated as zero. cellWidth and availableWidth must be finite and nonnegative.
     *
     * <p>Zero-advance base cells following a measured base split that cluster's advance in
     * proportion to terminal width; initial zero bases split the next measured cluster. This is
     * a hit-testing approximation (including lam-alef), not a claim about font ligature carets.
     * Attached marks have no separate hit region. If an entire run measures zero, its base cells
     * share a bounded one-cell hit region at its anchor, without changing text width or scale.
     *
     * <p>Unused logical columns are virtual RTL blanks, one cellWidth * scale step leftward from
     * the content's left edge, with both edges clamped to [0, availableWidth]. On a completely
     * empty row, column zero is the rightmost cell. Offscreen virtual blanks collapse at zero.
     * They do not affect shaping/line width; columnAt ignores them while real base cells exist.
     */
    public void place(float cellWidth, float availableWidth, float[] advances) {
        if (!finiteNonnegative(cellWidth) || !finiteNonnegative(availableWidth))
            throw new IllegalArgumentException("Invalid viewport or cell width");
        if (advances == null || advances.length < textLength)
            throw new IllegalArgumentException("Advances must cover textLength");

        placedWidth = availableWidth;
        double[][] widths = new double[runs.size()][];
        double[] naturalWidths = new double[runs.size()];
        double total = 0;
        for (int r = 0; r < runs.size(); r++) {
            Run run = runs.get(r);
            double[] values = new double[run.cells.size()];
            for (int i = 0; i < values.length; i++) {
                Cell cell = run.cells.get(i);
                for (int j = cell.start; j < cell.end; j++) {
                    if (finiteNonnegative(advances[j])) values[i] += advances[j];
                }
                naturalWidths[r] += values[i];
            }
            widths[r] = values;
            total += naturalWidths[r];
        }
        double scale = total > availableWidth && total > 0 ? availableWidth / total : 1;
        double contentLeft = Math.max(0, availableWidth - total * scale);
        double step = cellWidth * scale;
        for (int c = 0; c < columns; c++) {
            int blank = Math.max(0, c - contentColumns);
            cellRight[c] = bounded(contentLeft - blank * step, availableWidth);
            cellLeft[c] = bounded(contentLeft - (blank + 1.0) * step, availableWidth);
        }

        double x = contentLeft;
        for (int r = 0; r < runs.size(); r++) {
            Run run = runs.get(r);
            double natural = naturalWidths[r];
            run.scale = (float) scale;
            run.left = bounded(x, availableWidth);
            run.right = bounded(x + natural * scale, availableWidth);
            double[] values = widths[r];
            splitZeroBaseAdvances(run.cells, values);
            double pen = run.rtl ? x + natural * scale : x;
            for (int i = 0; i < run.cells.size(); i++) {
                Cell cell = run.cells.get(i);
                double distance = values[i] * scale;
                double left = run.rtl ? pen - distance : pen;
                double right = run.rtl ? pen : pen + distance;
                if (natural == 0 && cell.width > 0) {
                    // Invisible glyphs get a cursor target, not artificial inter-glyph spacing.
                    right = Math.min(availableWidth, Math.max(step, pen));
                    left = Math.max(0, right - step);
                }
                for (int c = cell.column; c < cell.column + cell.width; c++) {
                    cellLeft[c] = bounded(left, availableWidth);
                    cellRight[c] = bounded(right, availableWidth);
                }
                pen += run.rtl ? -distance : distance;
            }
            x += natural * scale;
        }
    }

    private static void splitZeroBaseAdvances(List<Cell> cells, double[] values) {
        // A cell already includes all its combining marks and surrogate units. Only base cells
        // participate here; there is never an allocation per UTF-16 character or per mark.
        int begin = 0;
        while (begin < values.length) {
            int measured = begin;
            while (measured < values.length && values[measured] == 0) measured++;
            if (measured == values.length) return; // Entire run has no measured glyph.
            int end = measured + 1;
            while (end < values.length && values[end] == 0) end++;
            int weight = 0;
            for (int i = begin; i < end; i++) weight += cells.get(i).width;
            double advance = values[measured];
            if (weight > 0) {
                for (int i = begin; i < end; i++)
                    values[i] = advance * cells.get(i).width / weight;
            }
            begin = end;
        }
    }

    private static boolean finiteNonnegative(float value) {
        return value >= 0 && !Float.isInfinite(value) && !Float.isNaN(value);
    }

    private static float bounded(double value, float availableWidth) {
        return (float) Math.max(0, Math.min(availableWidth, value));
    }

    private int clampColumn(int column) {
        return Math.max(0, Math.min(columns - 1, column));
    }

    public float leftOf(int logicalColumn) {
        return columns == 0 ? 0 : cellLeft[clampColumn(logicalColumn)];
    }

    public float rightOf(int logicalColumn) {
        return columns == 0 ? 0 : cellRight[clampColumn(logicalColumn)];
    }

    /** Leading or trailing edge of the cell itself, not of the next logical column. */
    public float boundary(int logicalColumn, boolean trailing) {
        return rtlAt(logicalColumn) != trailing ? rightOf(logicalColumn) : leftOf(logicalColumn);
    }

    public boolean rtlAt(int logicalColumn) {
        return columns == 0 || cellRtl[clampColumn(logicalColumn)];
    }

    /**
     * Nearest real base cell, returning its first logical column for both halves of a wide cell.
     * Virtual blanks are candidates only if no real base exists; zero-column rows return -1.
     * Equal-distance ties choose the smaller logical column. NaN is treated as pixel zero.
     */
    public int columnAt(float pixelX) {
        if (columns == 0) return -1;
        if (Float.isNaN(pixelX)) pixelX = 0;
        pixelX = Math.max(0, Math.min(placedWidth, pixelX));
        int best = -1;
        double bestDistance = Double.POSITIVE_INFINITY;
        boolean hasContent = contentColumns > 0;
        for (int c = 0; c < columns; c++) {
            if (hasContent && owner[c] != c) continue;
            double distance = pixelX < cellLeft[c] ? (double) cellLeft[c] - pixelX
                : pixelX > cellRight[c] ? (double) pixelX - cellRight[c] : 0;
            if (best == -1 || distance < bestDistance) {
                best = c;
                bestDistance = distance;
            }
        }
        return best;
    }
}
