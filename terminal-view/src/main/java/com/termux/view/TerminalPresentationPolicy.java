package com.termux.view;

/** Conservative display policy. A VT stream does not identify all full-screen or columnar apps. */
public final class TerminalPresentationPolicy {
    private TerminalPresentationPolicy() {}

    /** Stable paragraph-level logical navigation; never invert again at an embedded Latin run. */
    public static boolean rtlArrows(char[] text, int length) {
        boolean latin = false;
        for (int i = 0; i < length; ) {
            int cp = Character.codePointAt(text, i, length);
            byte dir = Character.getDirectionality(cp);
            if (dir == Character.DIRECTIONALITY_RIGHT_TO_LEFT ||
                dir == Character.DIRECTIONALITY_RIGHT_TO_LEFT_ARABIC) return true;
            if (dir == Character.DIRECTIONALITY_LEFT_TO_RIGHT) latin = true;
            i += Character.charCount(cp);
        }
        return !latin;
    }

    /** Native-grid owning cell [start, end). Combining marks never stop the search. */
    public static int[] cellRange(char[] text, int length, int target) {
        target = Math.max(0, target);
        int column = 0;
        for (int i = 0; i < length; ) {
            int cp = Character.codePointAt(text, i, length);
            int width = com.termux.terminal.WcWidth.width(cp);
            if (width > 0) {
                if (target < column + width) return new int[]{column, column + width};
                column += width;
            }
            i += Character.charCount(cp);
        }
        return new int[]{target, target + 1};
    }

    public static boolean flowAllowed(boolean enabled, boolean alternate, boolean mouse, boolean applicationCursor) {
        return enabled && !alternate && !mouse && !applicationCursor;
    }

    /** Rendering is allowed in TUIs too; only whole-row flow/navigation has the VT-mode gate. */
    public static boolean renderingAllowed(int sdkInt, boolean enabled) {
        return sdkInt >= 23 && enabled;
    }

    public static boolean hasStrongRtl(char[] text, int start, int end) {
        for (int i = start; i < end; ) {
            int cp = Character.codePointAt(text, i, end);
            byte dir = Character.getDirectionality(cp);
            if (dir == Character.DIRECTIONALITY_RIGHT_TO_LEFT ||
                dir == Character.DIRECTIONALITY_RIGHT_TO_LEFT_ARABIC) return true;
            i += Character.charCount(cp);
        }
        return false;
    }

    /** Android's directional-run drawing/measurement requires API 23. Kept pure for tests. */
    public static boolean flowAllowed(int sdkInt, boolean enabled, boolean alternate, boolean mouse,
                                      boolean applicationCursor) {
        return sdkInt >= 23 && flowAllowed(enabled, alternate, mouse, applicationCursor);
    }

    /**
     * Do not pack apparent columns, indentation, box drawing or ASCII table borders.
     * This is deliberately a heuristic, not command-name detection. The user can always disable
     * flow for unknown TUIs (including tools that draw on the primary buffer).
     */
    public static boolean looksLikeGrid(char[] text, int length) {
        int end = length;
        while (end > 0 && text[end - 1] == ' ') end--;
        if (GridBidiLayout.hasAnchoredPrefix(text, end)) return true;
        int spaces = 0;
        for (int i = 0; i < end; i++) {
            char c = text[i];
            if ((c >= '\u2500' && c <= '\u259f') || c == '\t' || c == '|') return true;
            if (c == ' ') {
                if (++spaces >= 2) return true;
            } else {
                spaces = 0;
            }
            if (i + 2 < end && ((c == '-' && text[i + 1] == '-' && text[i + 2] == '-') ||
                (c == '=' && text[i + 1] == '=' && text[i + 2] == '='))) return true;
        }
        return false;
    }
}
