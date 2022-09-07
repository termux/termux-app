package com.termux.terminal;

import android.graphics.Bitmap;

/**
 * A circular buffer of {@link TerminalRow}:s which keeps notes about what is visible on a logical screen and the scroll
 * history.
 * <p>
 * See {@link #externalToInternalRow(int)} for how to map from logical screen rows to array indices.
 */
public final class WorkingTerminalBitmap {
    final private int sixelInitialColorMap[] = {0xFF000000, 0xFF3333CC, 0xFFCC2323, 0xFF33CC33, 0xFFCC33CC, 0xFF33CCCC, 0xFFCCCC33, 0xFF777777,
                                                0xFF444444, 0xFF565699, 0xFF994444, 0xFF569956, 0xFF995699, 0xFF569999, 0xFF999956, 0xFFCCCCCC};
    private int[] colorMap;
    private int curX;
    private int curY;
    private int color;
    public int width;
    public int height;
    public Bitmap bitmap;
    private static final String LOG_TAG = "WorkingTerminalBitmap";

    public WorkingTerminalBitmap(int w, int h) {
        try {
            bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        } catch (OutOfMemoryError e) {
            Logger.logWarn(null, LOG_TAG, "Out of memory - sixel ignored");
            bitmap = null;
        }
        bitmap.eraseColor(0);
        width = 0;
        height = 0;
        curX = 0;
        curY = 0;
        colorMap = new int[256];
        for (int i=0; i<16; i++) {
            colorMap[i] = sixelInitialColorMap[i];
        }
        color = colorMap[0];
    }

    public void sixelChar(int c, int rep) {
        if (bitmap == null) {
            return;
        }
        if (c == '$') {
            curX = 0;
            return;
        }
        if (c == '-') {
            curX = 0;
            curY += 6;
            return;
        }
        if (bitmap.getWidth() < curX + rep) {
            try {
                bitmap = TerminalBitmap.resizeBitmap(bitmap, curX + rep + 100, bitmap.getHeight());
            } catch(OutOfMemoryError e) {
                Logger.logWarn(null, LOG_TAG, "Out of memory - sixel truncated");
            }
        }
        if (bitmap.getHeight() < curY + 6) {
            // Very unlikely to resize both at the same time
            try {
                bitmap = TerminalBitmap.resizeBitmap(bitmap, bitmap.getWidth(), curY + 100);
            } catch(OutOfMemoryError e) {
                Logger.logWarn(null, LOG_TAG, "Out of memory - sixel truncated");
            }
        }
        if (curX + rep > bitmap.getWidth()) {
            rep = bitmap.getWidth() - curX;
        }
        if ( curY + 6 > bitmap.getHeight()) {
            return;
        }
        if (rep > 0 && c >= '?' && c <= '~') {
            int b = c - '?';
            if (curY + 6 > height) {
                height = curY + 6;
            }
            while (rep-- > 0) {
                for (int i = 0 ; i < 6 ; i++) {
                    if ((b & (1<<i)) != 0) {
                        bitmap.setPixel(curX, curY + i, color);
                    }
                }
                curX += 1;
                if (curX > width) {
                    width = curX;
                }
            }
        }
    }

    public void sixelSetColor(int col) {
        if (col >= 0 && col < 256) {
            color = colorMap[col];
        }
    }

    public void sixelSetColor(int col, int r, int g, int b) {
        if (col >= 0 && col < 256) {
            int red = Math.min(255, r*255/100);
            int green = Math.min(255, g*255/100);
            int blue = Math.min(255, b*255/100);
            color = 0xff000000 + (red << 16) + (green << 8) + blue;
            colorMap[col] = color;
        }
    }
}
