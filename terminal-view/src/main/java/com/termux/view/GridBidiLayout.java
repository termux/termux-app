package com.termux.view;

import com.termux.terminal.WcWidth;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Display-only bidi islands inside an otherwise unmodified terminal grid. A field owns only its
 * original occupied cells, not the free space to the next field or the edge of the screen. Two
 * literal spaces, a tab, or a border delimit fields. Leading/trailing padding and familiar list
 * prefixes stay native. Only fields with a strong RTL character are shaped.
 *
 * <p>This is a bounded heuristic, not a reconstruction of arbitrary TUI widgets. In particular,
 * single-space-separated labels inside an RTL field participate in that field's bidi paragraph.
 * Style boundaries do not delimit fields. Original text, emulator cells and PTY bytes never change.
 */
public final class GridBidiLayout {
    private static final Pattern PREFIX = Pattern.compile(
        "^(?:(?:[-*+\\u2022\\u25cf\\u25e6\\u25b6\\u203a>]|[0-9]+[.)]|\\[[ xX]\\]|\\^[A-Za-z?]|M-[A-Za-z]) +)");

    public static final class Field {
        /** Half-open ORIGINAL terminal-cell allocation. Separators are excluded. */
        public final int startColumn, endColumn;
        /** Original row UTF-16 offsets (end excludes any virtual insertion blank). */
        public final int start, end;
        /** A logical copy, never glyph-reversed. Run/cell UTF-16 offsets are local to this array. */
        public final char[] text;
        /** Cell columns and placed x coordinates are LOCAL to the field. */
        public final BidiLineLayout layout;

        private Field(char[] source, int start, int end, int startColumn, int endColumn,
                      int cursorColumn, boolean virtualInsertion) {
            this.start = start;
            this.end = end;
            this.startColumn = startColumn;
            this.endColumn = endColumn;
            text = Arrays.copyOfRange(source, start, end + (virtualInsertion ? 1 : 0));
            if (virtualInsertion) text[text.length - 1] = ' ';
            int cursor = cursorColumn >= startColumn && cursorColumn < endColumn
                ? cursorColumn - startColumn : -1;
            layout = new BidiLineLayout(text, text.length, endColumn - startColumn, cursor,
                endColumn - startColumn - 1);
        }

        public void place(float cellWidth, float[] advances) {
            layout.place(cellWidth, (endColumn - startColumn) * cellWidth, advances);
        }
    }

    private static final class Cell {
        final int start, column, width, cp;
        int end;
        Cell(int start, int end, int column, int width, int cp) {
            this.start = start; this.end = end; this.column = column; this.width = width; this.cp = cp;
        }
        boolean space() { return cp == ' ' && end == start + 1; }
    }

    /** RTL fields in original left-to-right grid order; ASCII fields are deliberately absent. */
    public final List<Field> fields;
    private final int columns;
    private final int[] owner, ownerEnd;
    private final Field[] fieldByColumn;
    private float cellWidth;

    public GridBidiLayout(char[] text, int length, int columns, int cursorColumn) {
        if (text == null) throw new NullPointerException("text");
        if (length < 0 || length > text.length || columns < 0)
            throw new IllegalArgumentException("Invalid length or columns");
        this.columns = columns;
        owner = new int[columns];
        ownerEnd = new int[columns];
        for (int c = 0; c < columns; c++) { owner[c] = c; ownerEnd[c] = c + 1; }
        List<Cell> cells = new ArrayList<>();
        int column = 0;
        for (int i = 0; i < length; ) {
            int cp = Character.codePointAt(text, i, length);
            int next = i + Character.charCount(cp);
            // Real VT rows have expanded tabs. Define deterministic 8-cell tab stops for raw tests.
            int width = cp == '\t' ? Math.min(columns - column, 8 - column % 8) : WcWidth.width(cp);
            if (width > 0) {
                if (column + width > columns) break;
                Cell cell = new Cell(cells.isEmpty() ? 0 : i, next, column, width, cp);
                cells.add(cell);
                for (int c = column; c < column + width; c++) {
                    owner[c] = column; ownerEnd[c] = column + width;
                }
                column += width;
            } else if (!cells.isEmpty()) {
                // Combining marks/bidi controls remain attached to their owning base cell.
                cells.get(cells.size() - 1).end = next;
            }
            i = next;
        }
        List<Field> result = new ArrayList<>();
        int first = 0;
        for (int i = 0; i < cells.size(); ) {
            Cell cell = cells.get(i);
            if (border(cells, i) || cell.cp == '\t') {
                addField(result, text, length, cells, first, i, columns, cursorColumn);
                first = ++i;
            } else if (cell.space()) {
                int next = i + 1;
                while (next < cells.size() && cells.get(next).space()) next++;
                if (next - i >= 2) {
                    addField(result, text, length, cells, first, i, columns, cursorColumn);
                    first = next;
                }
                i = next;
            } else i++;
        }
        addField(result, text, length, cells, first, cells.size(), columns, cursorColumn);
        fields = Collections.unmodifiableList(result);
        fieldByColumn = new Field[columns];
        for (Field field : fields)
            Arrays.fill(fieldByColumn, field.startColumn, field.endColumn, field);
    }

    /** Used by the paragraph gate too, so an indented/bulleted primary-buffer row keeps its anchor. */
    public static boolean hasAnchoredPrefix(char[] text, int length) {
        return length > 0 && (text[0] == ' ' || text[0] == '\t' ||
            PREFIX.matcher(new String(text, 0, length)).find());
    }

    private static boolean border(List<Cell> cells, int i) {
        int cp = cells.get(i).cp;
        if ((cp >= 0x2500 && cp <= 0x259f) || cp == '|') return true;
        if (cp == '-' || cp == '=') {
            // Membership in a run of >= 3 needs only a local triplet, not a scan of the run.
            boolean before = i > 0 && cells.get(i - 1).cp == cp;
            boolean after = i + 1 < cells.size() && cells.get(i + 1).cp == cp;
            return (before && after) || (before && i > 1 && cells.get(i - 2).cp == cp) ||
                (after && i + 2 < cells.size() && cells.get(i + 2).cp == cp);
        }
        return cp == '+' && ((i > 0 && (cells.get(i - 1).cp == '-' || cells.get(i - 1).cp == '=')) ||
            (i + 1 < cells.size() && (cells.get(i + 1).cp == '-' || cells.get(i + 1).cp == '=')));
    }

    private static void addField(List<Field> out, char[] text, int length, List<Cell> cells, int first, int end,
                                 int columns, int cursor) {
        while (first < end && cells.get(first).space()) first++;
        while (end > first && cells.get(end - 1).space()) end--;
        if (first == end) return;
        int startIndex = cells.get(first).start;
        int endIndex = cells.get(end - 1).end;
        Matcher prefix = PREFIX.matcher(new String(text, startIndex, endIndex - startIndex));
        if (prefix.find()) {
            int prefixEnd = startIndex + prefix.end();
            while (first < end && cells.get(first).start < prefixEnd) first++;
        }
        if (first == end) return;
        // A single space can separate native TUI columns too (htop TIME+ then COMMAND).
        // Keep every complete leading token before the first RTL-bearing token native. Stop
        // at that token's START, not its first RTL code point: abcعربي remains one mixed token.
        // Once RTL begins, single-space-separated Latin paths/digits stay inside its paragraph.
        int tokenStart = first;
        boolean rtl = false;
        for (int i = first; i < end; i++) {
            Cell cell = cells.get(i);
            if (cell.space()) tokenStart = i + 1;
            else if (TerminalPresentationPolicy.hasStrongRtl(text, cell.start, cell.end)) {
                first = tokenStart;
                rtl = true;
                break;
            }
        }
        if (!rtl) return;
        Cell firstCell = cells.get(first), last = cells.get(end - 1);
        startIndex = firstCell.start;
        int endColumn = last.column + last.width;
        boolean virtual = false;
        // Only the immediate insertion blank joins the field. A cursor in a distant gap stays
        // native, and a full final row never invents a cell past the right edge. Blink is irrelevant.
        if (cursor == endColumn && endColumn < columns) {
            if (end < cells.size() && cells.get(end).column == endColumn && cells.get(end).space()) {
                endIndex = cells.get(end).end;
                endColumn++;
            } else if (end == cells.size() && endIndex == length) {
                virtual = true;
                endColumn++;
            }
        }
        out.add(new Field(text, startIndex, endIndex, firstCell.column, endColumn, cursor, virtual));
    }

    /** Place every field using shaped UTF-16 advances in field order. Never expands natural width. */
    public void place(float cellWidth, float[][] advances) {
        if (cellWidth < 0 || Float.isNaN(cellWidth) || Float.isInfinite(cellWidth) ||
            advances == null || advances.length != fields.size())
            throw new IllegalArgumentException("Invalid field metrics");
        this.cellWidth = cellWidth;
        for (int i = 0; i < fields.size(); i++) fields.get(i).place(cellWidth, advances[i]);
    }

    public Field fieldAtColumn(int column) {
        return column >= 0 && column < columns ? fieldByColumn[column] : null;
    }

    private int clamp(int column) { return Math.max(0, Math.min(columns - 1, column)); }

    public float leftOf(int column) {
        if (columns == 0) return 0;
        column = clamp(column);
        Field f = fieldAtColumn(column);
        return f == null ? owner[column] * cellWidth
            : f.startColumn * cellWidth + f.layout.leftOf(column - f.startColumn);
    }

    public float rightOf(int column) {
        if (columns == 0) return 0;
        column = clamp(column);
        Field f = fieldAtColumn(column);
        return f == null ? ownerEnd[column] * cellWidth
            : f.startColumn * cellWidth + f.layout.rightOf(column - f.startColumn);
    }

    public float boundary(int column, boolean trailing) {
        if (columns == 0) return 0;
        column = clamp(column);
        Field f = fieldAtColumn(column);
        return f == null ? (trailing ? rightOf(column) : leftOf(column))
            : f.startColumn * cellWidth + f.layout.boundary(column - f.startColumn, trailing);
    }

    /** Maps only within an allocated RTL field. Native separators/UI columns stay exact. */
    public int columnAt(float x) { return columnAt(x, true); }

    /** Mouse protocols address raw grid cells, including the second half of native wide cells. */
    public int mouseColumnAt(float x) { return columnAt(x, false); }

    private int columnAt(float x, boolean selection) {
        if (columns == 0) return -1;
        if (Float.isNaN(x)) x = 0;
        if (!selection) {
            int raw = cellWidth == 0 ? 0 : (int) (x / cellWidth);
            if (raw < 0 || raw >= columns) return raw;
        }
        x = Math.max(0, Math.min(columns * cellWidth, x));
        int column = cellWidth == 0 ? 0 : clamp((int) (x / cellWidth));
        Field f = fieldAtColumn(column);
        return f == null ? (selection ? owner[column] : column)
            : f.startColumn + f.layout.columnAt(x - f.startColumn * cellWidth);
    }
}
