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
 * Renderer of a {@link TerminalEmulator} into a {@link Canvas}.
 * <p/>
 * Saves font metrics, so needs to be recreated each time the typeface or font size changes.
 */
public final class TerminalRenderer {
    int mTextSize;
    private final Paint mTextPaint = new Paint();
    /* These arrays are indexed by style e.g mTypefaceByStyle[TypefaceStyle.ITALIC]; */
    private final Typeface[] mTypefaceByStyle = new Typeface[4];
    /** The width of a single mono spaced character obtained by {@link Paint#measureText(String)} on a single 'X'. */
    private final float[] mFontWidthByStyle = new float[4];
    /** The {@link Paint#getFontSpacing()}. See http://www.fampennings.nl/maarten/android/08numgrid/font.png */
    private final int[] mFontLineSpacingByStyle = new int[4];
    /** The {@link Paint#ascent()}. See http://www.fampennings.nl/maarten/android/08numgrid/font.png */
    private final int[] mFontAscentByStyle = new int[4];
    /** The {@link #mFontLineSpacingByStyle} + {@link #mFontAscentByStyle}. */
    private final int[] mFontLineSpacingAndAscentByStyle = new int[4];
    private final float[] asciiMeasures = new float[127];

    public TerminalRenderer(int textSize) {
        mTextSize = textSize;
        mTextPaint.setTextSize(textSize);
    }

    public void setTextSize(int textSize) {
        mTextSize = textSize;
        mTextPaint.setTextSize(textSize);

        for (int i = 0; i < 4; i++) {
            setTypeface(mTypefaceByStyle[i], TypefaceStyle.values()[i]);
        }
    }

    public void setTypeface(Typeface typeface, TypefaceStyle style) {
        int idx = style.ordinal();

        mTypefaceByStyle[idx] = typeface;

        mTextPaint.setTypeface(typeface);
        mTextPaint.setAntiAlias(true);

        mFontLineSpacingByStyle[idx] = (int) Math.ceil(mTextPaint.getFontSpacing());
        mFontAscentByStyle[idx] = (int) Math.ceil(mTextPaint.ascent());
        mFontLineSpacingAndAscentByStyle[idx] = mFontLineSpacingByStyle[idx] + mFontAscentByStyle[idx];
        mFontWidthByStyle[idx] = mTextPaint.measureText("X");

        if (style == TypefaceStyle.NORMAL) {
            StringBuilder sb = new StringBuilder(" ");
            for (int i = 0; i < asciiMeasures.length; i++) {
                sb.setCharAt(0, (char) i);
                asciiMeasures[i] = mTextPaint.measureText(sb, 0, 1);
            }
        }
    }

    /** Render the terminal to a canvas with at a specified row scroll, and an optional rectangular selection. */
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

        int fontLineSpacingAndAscent = getFontLineSpacingAndAscent();
        int fontLineSpacing = getFontLineSpacing();
        float fontWidth = getFontWidth();

        float heightOffset = fontLineSpacingAndAscent;
        for (int row = topRow; row < endRow; row++) {
            heightOffset += fontLineSpacing;

            final int cursorX = (row == cursorRow && cursorVisible) ? cursorCol : -1;
            int selx1 = -1, selx2 = -1;
            if (row >= selectionY1 && row <= selectionY2) {
                if (row == selectionY1) selx1 = selectionX1;
                selx2 = (row == selectionY2) ? selectionX2 : mEmulator.mColumns;
            }

            TerminalRow lineObject = screen.allocateFullLineIfNecessary(screen.externalToInternalRow(row));
            final char[] line = lineObject.mText;
            final int charsUsedInLine = lineObject.getSpaceUsed();

            long lastRunStyle = 0;
            boolean lastRunInsideCursor = false;
            boolean lastRunInsideSelection = false;
            int lastRunStartColumn = -1;
            int lastRunStartIndex = 0;
            boolean lastRunFontWidthMismatch = false;
            int currentCharIndex = 0;
            float measuredWidthForRun = 0.f;

            for (int column = 0; column < columns; ) {
                final char charAtIndex = line[currentCharIndex];
                final boolean charIsHighsurrogate = Character.isHighSurrogate(charAtIndex);
                final int charsForCodePoint = charIsHighsurrogate ? 2 : 1;
                final int codePoint = charIsHighsurrogate ? Character.toCodePoint(charAtIndex, line[currentCharIndex + 1]) : charAtIndex;
                final int codePointWcWidth = WcWidth.width(codePoint);
                final boolean insideCursor = (cursorX == column || (codePointWcWidth == 2 && cursorX == column + 1));
                final boolean insideSelection = column >= selx1 && column <= selx2;
                final long style = lineObject.getStyle(column);

                // Check if the measured text width for this code point is not the same as that expected by wcwidth().
                // This could happen for some fonts which are not truly monospace, or for more exotic characters such as
                // smileys which android font renders as wide.
                // If this is detected, we draw this code point scaled to match what wcwidth() expects.
                final float measuredCodePointWidth = (codePoint < asciiMeasures.length) ? asciiMeasures[codePoint] : mTextPaint.measureText(line,
                    currentCharIndex, charsForCodePoint);
                final boolean fontWidthMismatch = Math.abs(measuredCodePointWidth / fontWidth - codePointWcWidth) > 0.01;

                if (style != lastRunStyle || insideCursor != lastRunInsideCursor || insideSelection != lastRunInsideSelection || fontWidthMismatch || lastRunFontWidthMismatch) {
                    if (column == 0) {
                        // Skip first column as there is nothing to draw, just record the current style.
                    } else {
                        final int columnWidthSinceLastRun = column - lastRunStartColumn;
                        final int charsSinceLastRun = currentCharIndex - lastRunStartIndex;
                        int cursorColor = lastRunInsideCursor ? mEmulator.mColors.mCurrentColors[TextStyle.COLOR_INDEX_CURSOR] : 0;
                        boolean invertCursorTextColor = false;
                        if (lastRunInsideCursor && cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK) {
                            invertCursorTextColor = true;
                        }
                        drawTextRun(canvas, line, palette, heightOffset, lastRunStartColumn, columnWidthSinceLastRun,
                            lastRunStartIndex, charsSinceLastRun, measuredWidthForRun,
                            cursorColor, cursorShape, lastRunStyle, reverseVideo || invertCursorTextColor || lastRunInsideSelection);
                    }
                    measuredWidthForRun = 0.f;
                    lastRunStyle = style;
                    lastRunInsideCursor = insideCursor;
                    lastRunInsideSelection = insideSelection;
                    lastRunStartColumn = column;
                    lastRunStartIndex = currentCharIndex;
                    lastRunFontWidthMismatch = fontWidthMismatch;
                }
                measuredWidthForRun += measuredCodePointWidth;
                column += codePointWcWidth;
                currentCharIndex += charsForCodePoint;
                while (currentCharIndex < charsUsedInLine && WcWidth.width(line, currentCharIndex) <= 0) {
                    // Eat combining chars so that they are treated as part of the last non-combining code point,
                    // instead of e.g. being considered inside the cursor in the next run.
                    currentCharIndex += Character.isHighSurrogate(line[currentCharIndex]) ? 2 : 1;
                }
            }

            final int columnWidthSinceLastRun = columns - lastRunStartColumn;
            final int charsSinceLastRun = currentCharIndex - lastRunStartIndex;
            int cursorColor = lastRunInsideCursor ? mEmulator.mColors.mCurrentColors[TextStyle.COLOR_INDEX_CURSOR] : 0;
            boolean invertCursorTextColor = false;
            if (lastRunInsideCursor && cursorShape == TerminalEmulator.TERMINAL_CURSOR_STYLE_BLOCK) {
                invertCursorTextColor = true;
            }
            drawTextRun(canvas, line, palette, heightOffset, lastRunStartColumn, columnWidthSinceLastRun, lastRunStartIndex, charsSinceLastRun,
                measuredWidthForRun, cursorColor, cursorShape, lastRunStyle, reverseVideo || invertCursorTextColor || lastRunInsideSelection);
        }
    }

    /**
     * Prepare the {@link #mTextPaint} for drawing the specified style.
     * If there is no Typeface supplied for the style, we will use a
     * fallback and return it.
     **/
    private TypefaceStyle prepareStyleOrFallback(TypefaceStyle desiredStyle) {
        TypefaceStyle fallbackStyle = desiredStyle;
        switch (desiredStyle) {
            case NORMAL:
                mTextPaint.setTextSkewX(0.f);
                mTextPaint.setFakeBoldText(false);
                break;
            case ITALIC:
                if (mTypefaceByStyle[desiredStyle.ordinal()] == null) {
                    fallbackStyle = TypefaceStyle.NORMAL;
                    mTextPaint.setTextSkewX(-0.35f);
                } else {
                    mTextPaint.setTextSkewX(0.f);
                }
                mTextPaint.setFakeBoldText(false);
                break;
            case BOLD:
                if (mTypefaceByStyle[desiredStyle.ordinal()] == null) {
                    fallbackStyle = TypefaceStyle.NORMAL;
                    mTextPaint.setFakeBoldText(true);
                } else {
                    mTextPaint.setFakeBoldText(false);
                }
                mTextPaint.setTextSkewX(0.f);
                break;
            case BOLD_ITALIC:
                if (mTypefaceByStyle[desiredStyle.ordinal()] == null) {
                    // italic has priority over bold here since fake bold
                    // is not as bad as skewed text which may be to big
                    // for the cell
                    if (mTypefaceByStyle[TypefaceStyle.ITALIC.ordinal()] != null) {
                        fallbackStyle = TypefaceStyle.ITALIC;
                        mTextPaint.setFakeBoldText(true);
                        mTextPaint.setTextSkewX(0.f);
                    } else if (mTypefaceByStyle[TypefaceStyle.BOLD.ordinal()] != null) {
                        fallbackStyle = TypefaceStyle.BOLD;
                        mTextPaint.setFakeBoldText(false);
                        mTextPaint.setTextSkewX(-0.35f);
                    } else {
                        fallbackStyle = TypefaceStyle.NORMAL;
                        mTextPaint.setFakeBoldText(true);
                        mTextPaint.setTextSkewX(-0.35f);
                    }
                } else {
                    mTextPaint.setFakeBoldText(false);
                    mTextPaint.setTextSkewX(0.f);
                }
                break;
        }

        mTextPaint.setTypeface(mTypefaceByStyle[fallbackStyle.ordinal()]);

        return fallbackStyle;
    }

    private void drawTextRun(Canvas canvas, char[] text, int[] palette, float y, int startColumn, int runWidthColumns,
                             int startCharIndex, int runWidthChars, float mes, int cursor, int cursorStyle,
                             long textStyle, boolean reverseVideo) {
        int foreColor = TextStyle.decodeForeColor(textStyle);
        final int effect = TextStyle.decodeEffect(textStyle);
        int backColor = TextStyle.decodeBackColor(textStyle);
        final boolean bold = (effect & (TextStyle.CHARACTER_ATTRIBUTE_BOLD | TextStyle.CHARACTER_ATTRIBUTE_BLINK)) != 0;
        final boolean underline = (effect & TextStyle.CHARACTER_ATTRIBUTE_UNDERLINE) != 0;
        final boolean italic = (effect & TextStyle.CHARACTER_ATTRIBUTE_ITALIC) != 0;
        final boolean strikeThrough = (effect & TextStyle.CHARACTER_ATTRIBUTE_STRIKETHROUGH) != 0;
        final boolean dim = (effect & TextStyle.CHARACTER_ATTRIBUTE_DIM) != 0;

        TypefaceStyle style = TypefaceStyle.NORMAL;
        if (italic && bold) {
            style = prepareStyleOrFallback(TypefaceStyle.BOLD_ITALIC);
        } else if (italic) {
            style = prepareStyleOrFallback(TypefaceStyle.ITALIC);
        } else if (bold) {
            style = prepareStyleOrFallback(TypefaceStyle.BOLD);
        } else {
            prepareStyleOrFallback(style);
        }

        float fontWidth = getFontWidth(style);
        int fontAscent = getFontAscent(style);
        int fontLineSpacingAndAscent = getFontLineSpacingAndAscent(style);

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

        float left = startColumn * fontWidth;
        float right = left + runWidthColumns * fontWidth;

        mes = mes / fontWidth;
        boolean savedMatrix = false;
        if (Math.abs(mes - runWidthColumns) > 0.01) {
            canvas.save();
            canvas.scale(runWidthColumns / mes, 1.f);
            left *= mes / runWidthColumns;
            right *= mes / runWidthColumns;
            savedMatrix = true;
        }

        if (backColor != palette[TextStyle.COLOR_INDEX_BACKGROUND]) {
            // Only draw non-default background.
            mTextPaint.setColor(backColor);
            canvas.drawRect(left, y - fontLineSpacingAndAscent + fontAscent, right, y, mTextPaint);
        }

        if (cursor != 0) {
            mTextPaint.setColor(cursor);
            float cursorHeight = fontLineSpacingAndAscent - fontAscent;
            if (cursorStyle == TerminalEmulator.TERMINAL_CURSOR_STYLE_UNDERLINE) cursorHeight /= 4.;
            else if (cursorStyle == TerminalEmulator.TERMINAL_CURSOR_STYLE_BAR) right -= ((right - left) * 3) / 4.;
            canvas.drawRect(left, y - cursorHeight, right, y, mTextPaint);
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

            mTextPaint.setUnderlineText(underline);
            mTextPaint.setStrikeThruText(strikeThrough);
            mTextPaint.setColor(foreColor);

            // The text alignment is the default Paint.Align.LEFT.
            canvas.drawTextRun(text, startCharIndex, runWidthChars, startCharIndex, runWidthChars, left, y - fontLineSpacingAndAscent, false, mTextPaint);
        }

        if (savedMatrix) canvas.restore();
    }

    public float getFontWidth(TypefaceStyle style) {
        return mFontWidthByStyle[style.ordinal()];
    }

    public int getFontAscent(TypefaceStyle style) {
        return mFontAscentByStyle[style.ordinal()];
    }
    public int getFontLineSpacing(TypefaceStyle style) {
        return mFontLineSpacingByStyle[style.ordinal()];
    }

    public int getFontLineSpacingAndAscent(TypefaceStyle style) {
        return mFontLineSpacingAndAscentByStyle[style.ordinal()];
    }

    public float getFontWidth() {
        return mFontWidthByStyle[TypefaceStyle.NORMAL.ordinal()];
    }
    public int getFontAscent() {
        return mFontAscentByStyle[TypefaceStyle.NORMAL.ordinal()];
    }
    public int getFontLineSpacing() {
        return mFontLineSpacingByStyle[TypefaceStyle.NORMAL.ordinal()];
    }
    public int getFontLineSpacingAndAscent() {
        return mFontLineSpacingAndAscentByStyle[TypefaceStyle.NORMAL.ordinal()];
    }

    public enum TypefaceStyle {
        NORMAL,
        ITALIC,
        BOLD,
        BOLD_ITALIC,
    }
}
