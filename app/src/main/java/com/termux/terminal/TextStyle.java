package com.termux.terminal;

/**
 * Encodes effects, foreground and background colors into a 32 bit integer, which are stored for each cell in a terminal
 * row in {@link TerminalRow#mStyle}.
 * <p/>
 * The foreground and background colors take 9 bits each, leaving (32-9-9)=14 bits for effect flags. Using 9 for now
 * (the different CHARACTER_ATTRIBUTE_* bits).
 */
public final class TextStyle {

    public final static int CHARACTER_ATTRIBUTE_BOLD = 1;
    public final static int CHARACTER_ATTRIBUTE_ITALIC = 1 << 1;
    public final static int CHARACTER_ATTRIBUTE_UNDERLINE = 1 << 2;
    public final static int CHARACTER_ATTRIBUTE_BLINK = 1 << 3;
    public final static int CHARACTER_ATTRIBUTE_INVERSE = 1 << 4;
    public final static int CHARACTER_ATTRIBUTE_INVISIBLE = 1 << 5;
    public final static int CHARACTER_ATTRIBUTE_STRIKETHROUGH = 1 << 6;
    /**
     * The selective erase control functions (DECSED and DECSEL) can only erase characters defined as erasable.
     * <p/>
     * This bit is set if DECSCA (Select Character Protection Attribute) has been used to define the characters that
     * come after it as erasable from the screen.
     */
    public final static int CHARACTER_ATTRIBUTE_PROTECTED = 1 << 7;
    /** Dim colors. Also known as faint or half intensity. */
    public final static int CHARACTER_ATTRIBUTE_DIM = 1 << 8;

    public final static int COLOR_INDEX_FOREGROUND = 256;
    public final static int COLOR_INDEX_BACKGROUND = 257;
    public final static int COLOR_INDEX_CURSOR = 258;

    /** The 256 standard color entries and the three special (foreground, background and cursor) ones. */
    public final static int NUM_INDEXED_COLORS = 259;

    /** Normal foreground and background colors and no effects. */
    final static int NORMAL = encode(COLOR_INDEX_FOREGROUND, COLOR_INDEX_BACKGROUND, 0);

    static int encode(int foreColor, int backColor, int effect) {
        return ((effect & 0b111111111) << 18) | ((foreColor & 0b111111111) << 9) | (backColor & 0b111111111);
    }

    public static int decodeForeColor(int encodedColor) {
        return (encodedColor >> 9) & 0b111111111;
    }

    public static int decodeBackColor(int encodedColor) {
        return encodedColor & 0b111111111;
    }

    public static int decodeEffect(int encodedColor) {
        return (encodedColor >> 18) & 0b111111111;
    }

}
