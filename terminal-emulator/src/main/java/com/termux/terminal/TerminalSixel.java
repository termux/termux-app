package com.termux.terminal;

import android.graphics.Bitmap;

/**
 * A terminal sixel received via `DCS q s..s ST` or `DCS P1; P2; P3; q s..s ST`.
 *
 * **See Also:**
 * - `TerminalEmulator.ESC_DCS__SIXEL`
 * - https://vt100.net/docs/vt3xx-gp/chapter14.html
 * - https://en.wikipedia.org/wiki/Sixel
 * - https://www.digiater.nl/openvms/decus/vax90b1/krypton-nasa/all-about-sixels.text
 */
public class TerminalSixel {

    public static final String LOG_TAG = "TerminalSixel";



    public static final int[] SIXEL__INITIAL_COLOR_MAP = {
        0xFF000000, 0xFF3333CC, 0xFFCC2323, 0xFF33CC33, 0xFFCC33CC, 0xFF33CCCC, 0xFFCCCC33, 0xFF777777,
        0xFF444444, 0xFF565699, 0xFF994444, 0xFF569956, 0xFF995699, 0xFF569999, 0xFF999956, 0xFFCCCCCC
    };

    /**
     * A sixel is a group of six pixels in a vertical column.
     */
    public static final int SIXEL__LINE_LEN = 6;

    /**
     * The amount of extra dimension added when resizing a sixel when receiving more image data.
     */
    public static final int SIXEL__BITMAP_RESIZE_EXTRA_DIMENSION = 100;

    /**
     * The max value for the sixel Graphics Repeat Introducer.
     *
     * The value `8192` can support 8K images, see also {@link TerminalBitmap#MAX_BITMAP_SIZE}.
     *
     * Each repeat creates a new sixel line of `1x6` pixels, where `6` is the {@link #SIXEL__LINE_LEN}.
     *
     *  - https://vt100.net/docs/vt3xx-gp/chapter14.html#S14.3.1
     */
    public static final int SIXEL__MAX_REPEAT = 8192;



    protected final TerminalSessionClient mClient;

    protected Bitmap mBitmap;

    protected int mWidth;
    protected int mHeight;

    protected int mCurX;
    protected int mCurY;

    protected final int[] mColorMap;
    protected int mColor;



    protected TerminalSixel(TerminalSessionClient client, Bitmap bitmap) {
        mClient = client;

        mBitmap = bitmap;

        mWidth = 0;
        mHeight = 0;

        mCurX = 0;
        mCurY = 0;

        mColorMap = new int[256];
        System.arraycopy(SIXEL__INITIAL_COLOR_MAP, 0, mColorMap, 0, 16);
        mColor = mColorMap[0];
    }



    public static TerminalSixel build(TerminalSessionClient client, int bitmapWidth, int bitmapHeight) {
        try {

            Bitmap bitmap = Bitmap.createBitmap(bitmapWidth, bitmapHeight, Bitmap.Config.ARGB_8888);
            bitmap.eraseColor(0);

            return new TerminalSixel(client, bitmap);
        } catch (Throwable t) {
            if (t instanceof OutOfMemoryError) System.gc();
            Logger.logError(client, LOG_TAG, "Create sixel bitmap for" +
                " width " + bitmapWidth + " with height " + bitmapHeight + " failed: " + t.getMessage());
            return null;
        }
    }



    public TerminalSessionClient getClient() {
        return mClient;
    }


    public Bitmap getBitmap() {
        return mBitmap;
    }


    public int getWidth() {
        return mWidth;
    }

    public int getHeight() {
        return mHeight;
    }


    public int getCurX() {
        return mCurX;
    }

    public int getCurY() {
        return mCurY;
    }


    public int[] getColorMap() {
        return mColorMap;
    }

    public int getColor() {
        return mColor;
    }



    public boolean readData(int codePoint, int repeat) {
        if (mBitmap == null) {
            return false;
        }

        if (repeat > TerminalSixel.SIXEL__MAX_REPEAT) {
            Logger.logError(mClient, LOG_TAG, "The sixel repeat value " + repeat + " is greater than max repeat value " + TerminalSixel.SIXEL__MAX_REPEAT);
            return false;
        }

        // Graphics Carriage Return `$`.
        // > The $ (2/4) character indicates the end of the sixel line. The active position returns
        // > to the left page border of the same sixel line. You can use this character to overprint lines.
        // - https://vt100.net/docs/vt3xx-gp/chapter14.html#S14.3.4
        if (codePoint == '$') {
            mCurX = 0;
            return true;
        }

        // Graphics New Line `-`.
        // > The - (2/13) character indicates the end of a sixel line. The active position moves to
        // > the left margin of the next sixel line.
        // - https://vt100.net/docs/vt3xx-gp/chapter14.html#S14.3.5
        if (codePoint == '-') {
            mCurX = 0;
            mCurY += SIXEL__LINE_LEN;
            return true;
        }

        if (mBitmap.getWidth() < mCurX + repeat) {
            int newBitmapWidth = mCurX + repeat + SIXEL__BITMAP_RESIZE_EXTRA_DIMENSION;

            if (newBitmapWidth < 0) {
                Logger.logError(mClient, LOG_TAG, "The new sixel bitmap width overflowed: " +
                    mCurX + " (cursor x) + " + repeat + " (repeat) + " + SIXEL__BITMAP_RESIZE_EXTRA_DIMENSION);
                return false;
            }

            mBitmap = TerminalBitmap.resizeBitmap(LOG_TAG, "sixel", mClient, mBitmap, newBitmapWidth, mBitmap.getHeight());
            if (mBitmap == null) {
                return false;
            }
        }

        if (mBitmap.getHeight() < mCurY + SIXEL__LINE_LEN) {
            // Very unlikely to resize both at the same time.
            int newBitmapHeight = mCurY + SIXEL__BITMAP_RESIZE_EXTRA_DIMENSION;

            if (newBitmapHeight < 0) {
                Logger.logError(mClient, LOG_TAG, "The new sixel bitmap height overflowed: " +
                    mCurY + " (cursor y) + " + SIXEL__BITMAP_RESIZE_EXTRA_DIMENSION);
                return false;
            }

            mBitmap = TerminalBitmap.resizeBitmap(LOG_TAG, "sixel", mClient, mBitmap, mBitmap.getWidth(), newBitmapHeight);
            if (mBitmap == null) {
                return false;
            }
        }

        if (mCurX + repeat > mBitmap.getWidth()) {
            repeat = mBitmap.getWidth() - mCurX;
        }

        if (mCurY + SIXEL__LINE_LEN > mBitmap.getHeight()) {
            Logger.logError(mClient, LOG_TAG, "The sixel curson y position " +
                mCurY + SIXEL__LINE_LEN + " is greater than bitmap height " + mBitmap.getHeight());
            return false;
        }

        // Sixel data characters are in the range of `?` (0x3F) to `~` (0x7E).
        // > Each sixel data character represents six vertical pixels of data. Each sixel data character
        // > represents a binary value equal to the character code value minus hex 3F.
        // - https://vt100.net/docs/vt3xx-gp/chapter14.html#S14.2.1
        if (repeat > 0 && codePoint >= '?' && codePoint <= '~') {
            int b = codePoint - '?';
            if (mCurY + SIXEL__LINE_LEN > mHeight) {
                mHeight = mCurY + SIXEL__LINE_LEN;
            }

            while (repeat-- > 0) {
                for (int i = 0; i < SIXEL__LINE_LEN; i++) {
                    if ((b & (1 << i)) != 0) {
                        mBitmap.setPixel(mCurX, mCurY + i, mColor);
                    }
                }

                mCurX += 1;
                if (mCurX > mWidth) {
                    mWidth = mCurX;
                }
            }
        }

        return true;
    }

    public boolean resize(int sixelWidth, int sixelHeight) {
        if (mBitmap == null) {
            return false;
        }

        if (sixelWidth < 1 || sixelHeight < 1)
            return false;

        int bitmapWidth = mBitmap.getWidth();
        int newBitmapWidth = Math.max(sixelWidth, bitmapWidth);

        int bitmapHeight = mBitmap.getHeight();
        int newBitmapHeight = Math.max(sixelHeight, bitmapHeight);

        if (bitmapWidth < newBitmapWidth || bitmapHeight < newBitmapHeight) {
            mBitmap = TerminalBitmap.resizeBitmap(LOG_TAG, "sixel", mClient, mBitmap, newBitmapWidth, newBitmapHeight);
            return mBitmap != null;
        }

        return true;
    }

    public void setColor(int color) {
        if (color >= 0 && color < mColorMap.length) {
            mColor = mColorMap[color];
        }
    }

    public void setRGBColor(int color, int r, int g, int b) {
        if (color >= 0 && color < mColorMap.length) {
            int red = Math.min(255, r * 255/100);
            int green = Math.min(255, g * 255/100);
            int blue = Math.min(255, b * 255/100);
            mColor = 0xff000000 + (red << 16) + (green << 8) + blue;
            mColorMap[color] = mColor;
        }
    }

}
