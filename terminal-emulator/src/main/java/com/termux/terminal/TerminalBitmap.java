package com.termux.terminal;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Rect;

import android.os.SystemClock;

/**
 * A circular buffer of {@link TerminalRow}:s which keeps notes about what is visible on a logical screen and the scroll
 * history.
 * <p>
 * See {@link #externalToInternalRow(int)} for how to map from logical screen rows to array indices.
 */
public class TerminalBitmap {
    public Bitmap bitmap;
    public int cellWidth;
    public int cellHeight;
    public int scrollLines;
    public int[] cursorDelta;
    private static final String LOG_TAG = "TerminalBitmap";


    public TerminalBitmap(int num, WorkingTerminalBitmap sixel, int Y, int X, int cellW, int cellH, TerminalBuffer screen) {
        Bitmap bm = sixel.bitmap;
        bm = resizeBitmapConstraints(bm, sixel.width, sixel.height, cellW, cellH, screen.mColumns - X);
        addBitmap(num, bm, Y, X, cellW, cellH, screen);
    }

    public TerminalBitmap(int num, byte[] image, int Y, int X, int cellW, int cellH, int width, int height, boolean aspect, TerminalBuffer screen) {
        Bitmap bm = null;
        int imageHeight;
        int imageWidth;
        int newWidth = width;
        int newHeight = height;
        if (height > 0 || width > 0) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            try {
                BitmapFactory.decodeByteArray(image, 0, image.length, options);
            } catch (Exception e) {
                Logger.logWarn(null, LOG_TAG, "Cannot decode image");
            }
            imageHeight = options.outHeight;
            imageWidth = options.outWidth;
            if (aspect) {
                double wFactor = 9999.0;
                double hFactor = 9999.0;
                if (width > 0) {
                    wFactor = (double)width / imageWidth;
                }
                if (height > 0) {
                    hFactor = (double)height / imageHeight;
                }
                double factor = Math.min(wFactor, hFactor);
                newWidth = (int)(factor * imageWidth);
                newHeight = (int)(factor * imageHeight);
            } else {
                if (height <= 0) {
                    newHeight = imageHeight;
                }
                if (width <= 0) {
                    newWidth = imageWidth;
                }
            }
            int scaleFactor = 1;
            while (imageHeight >= 2 * newHeight * scaleFactor && imageWidth >= 2 * newWidth * scaleFactor) {
                scaleFactor = scaleFactor * 2;
            }
            BitmapFactory.Options scaleOptions = new BitmapFactory.Options();
            scaleOptions.inSampleSize = scaleFactor;
            try {
                bm = BitmapFactory.decodeByteArray(image, 0, image.length, scaleOptions);
            } catch (Exception e) {
                Logger.logWarn(null, LOG_TAG, "Out of memory, cannot decode image");
                bitmap = null;
                return;
            }
            if (bm == null) {
                Logger.logWarn(null, LOG_TAG, "Could not decode image");
                bitmap = null;
                return;
            }
            int maxWidth = (screen.mColumns - X) * cellW;
            if (newWidth > maxWidth) {
                int cropWidth = bm.getWidth() * maxWidth / newWidth;
                try {
                    bm = Bitmap.createBitmap(bm, 0, 0, cropWidth, bm.getHeight());
                    newWidth = maxWidth;
                } catch(OutOfMemoryError e) {
                    // This is just a memory optimization. If it fails,
                    // continue (and probably fail later).
                }
            }
            try {
                bm = Bitmap.createScaledBitmap(bm, newWidth, newHeight, true);
            } catch(OutOfMemoryError e) {
                Logger.logWarn(null, LOG_TAG, "Out of memory, cannot rescale image");
                bm = null;
            }
        } else {
            try {
                bm = BitmapFactory.decodeByteArray(image, 0, image.length);
            } catch (Exception e) {
                Logger.logWarn(null, LOG_TAG, "Out of memory, cannot decode image");
            }
        }

        if (bm == null) {
            Logger.logWarn(null, LOG_TAG, "Cannot decode image");
            bitmap = null;
            return;
        }

        bm = resizeBitmapConstraints(bm, bm.getWidth(), bm.getHeight(), cellW, cellH, screen.mColumns - X);
        addBitmap(num, bm, Y, X, cellW, cellH, screen);
        cursorDelta  = new int[] {scrollLines, (bitmap.getWidth() + cellW - 1) / cellW};
    }

    private void addBitmap(int num, Bitmap bm, int Y, int X, int cellW, int cellH, TerminalBuffer screen) {
        if (bm == null) {
            bitmap = null;
            return;
        }
        int width = bm.getWidth();
        int height = bm.getHeight();
        cellWidth = cellW;
        cellHeight = cellH;
        int w = Math.min(screen.mColumns - X, (width + cellW - 1) / cellW);
        int h = (height + cellH - 1) / cellH;
        int s = 0;
        for (int i=0; i<h; i++) {
            if (Y+i-s == screen.mScreenRows) {
                screen.scrollDownOneLine(0, screen.mScreenRows, TextStyle.NORMAL);
                s++;
            }
            for (int j=0; j<w ; j++) {
                screen.setChar(X+j, Y+i-s, '+', TextStyle.encodeBitmap(num, j, i));
            }
        }
        if (w * cellW < width) {
            try {
                bm = Bitmap.createBitmap(bm, 0, 0, w * cellW, height);
            } catch(OutOfMemoryError e) {
                // Image cannot be cropped to only visible part due to out of memory.
                // This causes memory waste.
            } 
        }
        bitmap = bm;
        scrollLines =  h - s;
    }
        
    static public Bitmap resizeBitmap(Bitmap bm, int w, int h) {
        int[] pixels = new int[bm.getAllocationByteCount()];
        bm.getPixels(pixels, 0, bm.getWidth(), 0, 0, bm.getWidth(), bm.getHeight());
        Bitmap newbm;
        try {
            newbm = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        } catch(OutOfMemoryError e) {
            // Only a minor display glitch in this case
            return bm;
        }
        int newWidth = Math.min(bm.getWidth(), w);
        int newHeight = Math.min(bm.getHeight(), h);
        newbm.setPixels(pixels, 0, bm.getWidth(), 0, 0, newWidth, newHeight);
        return newbm;
    }

    static public Bitmap resizeBitmapConstraints(Bitmap bm, int w, int h, int cellW, int cellH, int Columns) {
        // Width and height must be multiples of the cell width and height
        // Bitmap should not extend beyonf screen width
        if (w > cellW * Columns || (w % cellW) != 0 || (h % cellH) != 0) {
            int newW = Math.min(cellW * Columns, ((w - 1) / cellW) * cellW + cellW);
            int newH = ((h - 1) / cellH) * cellH + cellH;
            try {
                bm = resizeBitmap(bm, newW, newH);
            } catch(OutOfMemoryError e) {
                // Only a minor display glitch in this case
            }
        }
        return bm;
    }
}
