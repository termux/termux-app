package com.termux.terminal;

import android.graphics.Color;

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
    static int parse(String c) {
        try {
            int skipInitial, skipBetween;
            if (c.charAt(0) == '#') {
                // #RGB, #RRGGBB, #RRRGGGBBB or #RRRRGGGGBBBB. Most significant bits.
                skipInitial = 1;
                skipBetween = 0;
            } else if (c.startsWith("rgb:")) {
                // rgb:<red>/<green>/<blue> where <red>, <green>, <blue> := h | hh | hhh | hhhh. Scaled.
                skipInitial = 4;
                skipBetween = 1;
            } else {
                return 0;
            }
            int charsForColors = c.length() - skipInitial - 2 * skipBetween;
            if (charsForColors % 3 != 0) return 0; // Unequal lengths.
            int componentLength = charsForColors / 3;
            double mult = 255 / (Math.pow(2, componentLength * 4) - 1);

            int currentPosition = skipInitial;
            String rString = c.substring(currentPosition, currentPosition + componentLength);
            currentPosition += componentLength + skipBetween;
            String gString = c.substring(currentPosition, currentPosition + componentLength);
            currentPosition += componentLength + skipBetween;
            String bString = c.substring(currentPosition, currentPosition + componentLength);

            int r = (int) (Integer.parseInt(rString, 16) * mult);
            int g = (int) (Integer.parseInt(gString, 16) * mult);
            int b = (int) (Integer.parseInt(bString, 16) * mult);
            return 0xFF << 24 | r << 16 | g << 8 | b;
        } catch (NumberFormatException | IndexOutOfBoundsException e) {
            return 0;
        }
    }

    /** Try parse a color from a text parameter and into a specified index. */
    public void tryParseColor(int intoIndex, String textParameter) {
        int c = parse(textParameter);
        if (c != 0) mCurrentColors[intoIndex] = c;
    }

    /**
     * Get the perceived brightness of the color based on its RGB components.
     *
     * https://www.nbdtech.com/Blog/archive/2008/04/27/Calculating-the-Perceived-Brightness-of-a-Color.aspx
     * http://alienryderflex.com/hsp.html
     *
     * @param color The color code int.
     * @return Returns value between 0-255.
     */
    public static int getPerceivedBrightnessOfColor(int color) {
        return (int)
            Math.floor(Math.sqrt(
                Math.pow(Color.red(color), 2) * 0.241 +
                    Math.pow(Color.green(color), 2) * 0.691 +
                    Math.pow(Color.blue(color), 2) * 0.068
            ));
    }

}
