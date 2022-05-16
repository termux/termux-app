package com.termux.terminal;

/** Current terminal colors (if different from default). */
public final class TerminalColors {

    /** Static data - a bit ugly but ok for now. */
    public static final TerminalColorScheme COLOR_SCHEME = new TerminalColorScheme();

    /**
     * The current terminal colors, which are normally set from the color theme, but may be set dynamically with the OSC
     * 4 control sequence.
     */
    public final int[] mCurrentColors = new int[TextStyle.NUM_INDEXED_COLORS];

    /** Create a new instance with default colors from the theme. */
    public TerminalColors() {
        reset();
    }

    /** Reset a particular indexed color with the default color from the color theme. */
    public void reset(int index) {
        mCurrentColors[index] = COLOR_SCHEME.mDefaultColors[index];
    }

    /** Reset all indexed colors with the default color from the color theme. */
    public void reset() {
        System.arraycopy(COLOR_SCHEME.mDefaultColors, 0, mCurrentColors, 0, TextStyle.NUM_INDEXED_COLORS);
    }

    /**
     * Parse color according to http://manpages.ubuntu.com/manpages/intrepid/man3/XQueryColor.3.html
     * <p/>
     * Highest bit is set if successful, so return value is 0xFF${R}${G}${B}. Return 0 if failed.
     */
    static int parse(String colorString) {
        int skipInitial, skipBetween;
        if (colorString.charAt(0) == '#') {
            // #RGB, #RRGGBB, #RRRGGGBBB or #RRRRGGGGBBBB. Most significant bits.
            skipInitial = 1;
            skipBetween = 0;
        } else if (colorString.startsWith("rgb:")) {
            // rgb:<red>/<green>/<blue> where <red>, <green>, <blue> := h | hh | hhh | hhhh. Scaled.
            skipInitial = 4;
            skipBetween = 1;
        } else {
            throw new IllegalArgumentException("Wrong Prefix Format: '" + colorString + "'");
        }

        int componentLength = getComponentLength(colorString, skipInitial, skipBetween);

        return parseRGB(colorString, skipInitial, skipBetween, componentLength);
    }

    private static int getComponentLength(String colorString, int skipInitial, int skipBetween) {
        int charsForColors = colorString.length() - skipInitial - 2 * skipBetween;
        if (charsForColors % 3 != 0) {
            throw new IllegalArgumentException("Unequal Length: '" + colorString + "'");
        }
        return charsForColors / 3;
    }

    private static int parseRGB(String colorString, int skipInitial, int skipBetween, int componentLength) {
        double mult = 255 / (Math.pow(2, componentLength * 4) - 1);
        try {
            int currentPosition = skipInitial;
            String rString = colorString.substring(currentPosition, currentPosition + componentLength);
            currentPosition += componentLength + skipBetween;
            String gString = colorString.substring(currentPosition, currentPosition + componentLength);
            currentPosition += componentLength + skipBetween;
            String bString = colorString.substring(currentPosition, currentPosition + componentLength);

            int r = (int) (Integer.parseInt(rString, 16) * mult);
            int g = (int) (Integer.parseInt(gString, 16) * mult);
            int b = (int) (Integer.parseInt(bString, 16) * mult);
            return 0xFF << 24 | r << 16 | g << 8 | b;
        } catch (NumberFormatException e) {
            throw new IllegalArgumentException("NumberFormatException: '" + colorString + "'");
        } catch (IndexOutOfBoundsException e) {
            throw new IllegalArgumentException("IndexOutOfBoundsException: '" + colorString + "'");
        }
    }

    /** Try parse a color from a text parameter and into a specified index. */
    public void tryParseColor(int intoIndex, String textParameter) {
        try {
            mCurrentColors[intoIndex] = parse(textParameter);
        } catch (IllegalArgumentException e) {
            // Ignore.
        }
    }

}
