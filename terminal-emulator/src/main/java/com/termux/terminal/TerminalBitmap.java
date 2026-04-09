package com.termux.terminal;

import android.graphics.Bitmap;
import android.graphics.BitmapFactory;

/**
 * A terminal bitmap for images.
 */
public class TerminalBitmap {

    public static final String LOG_TAG = "TerminalBitmap";


    protected final TerminalSessionClient mClient;

    protected int mBitmapNum;
    protected Bitmap mBitmap;
    
    protected int mCellWidth;
    protected int mCellHeight;

    protected int mScrollLines;

    protected int[] mCursorDelta;


    protected TerminalBitmap(TerminalSessionClient client, int bitmapNum, Bitmap bitmap,
                             int cellWidth, int cellHeight,
                             int scrollLines, int[] cursorDelta) {
        mClient = client;

        mBitmapNum = bitmapNum;
        mBitmap = bitmap;

        mCellWidth = cellWidth;
        mCellHeight = cellHeight;

        mScrollLines = scrollLines;
        mCursorDelta = cursorDelta;
    }



    /** Build a {@link TerminalBitmap} from a {@link TerminalSixel}. */
    public static TerminalBitmap build(TerminalBuffer terminalBuffer, int bitmapNum, TerminalSixel terminalSixel,
                                       int x, int y, int cellWidth, int cellHeight) {
        try {
            Bitmap bitmap = terminalSixel.getBitmap();
            bitmap = resizeBitmapConstrained(LOG_TAG, "sixel", terminalBuffer.getClient(), bitmap,
                terminalSixel.getWidth(), terminalSixel.getHeight(), cellWidth, cellHeight,
                terminalBuffer.mColumns - x);
            if (bitmap == null) {
                Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                    "Create terminal bitmap " + bitmapNum + " from terminal sixel failed");
                return null;
            }

            return buildOrThrow(terminalBuffer, bitmapNum, bitmap,
                x, y, cellWidth, cellHeight);
        } catch (Throwable t) {
            if (t instanceof OutOfMemoryError) System.gc();
            Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                "Create terminal bitmap " + bitmapNum + " from terminal sixel failed: " + t.getMessage());
            return null;
        }
    }


    /** Build a {@link TerminalBitmap} from an image `byte[]`. */
    public static TerminalBitmap build(TerminalBuffer terminalBuffer, int bitmapNum, byte[] image,
                                       int x, int y, int cellWidth, int cellHeight,
                                       int width, int height, boolean shouldPreserveAspectRatio) {
        try {
            Bitmap newBitmap;
            int imageHeight;
            int imageWidth;
            int newWidth = width;
            int newHeight = height;

            if (image == null) {
                Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                    "Create terminal bitmap " + bitmapNum + " from image byte array failed:" +
                        " Image data not set");
                return null;
            }

            if (height > 0 || width > 0) {
                // Get image dimensions without creating a bitmap.
                BitmapFactory.Options options = new BitmapFactory.Options();
                options.inJustDecodeBounds = true;
                try {
                    BitmapFactory.decodeByteArray(image, 0, image.length, options);
                } catch (Throwable t) {
                    if (t instanceof OutOfMemoryError) System.gc();
                    Logger.logWarn(terminalBuffer.getClient(), LOG_TAG,
                        "Decode bitmap failed while creating" +
                            " terminal bitmap " + bitmapNum + " from image byte array: " + t.getMessage());
                }


                imageHeight = options.outHeight;
                imageWidth = options.outWidth;
                if (shouldPreserveAspectRatio) {
                    double wFactor = 9999.0;
                    double hFactor = 9999.0;
                    if (width > 0) {
                        wFactor = (double) width / imageWidth;
                    }
                    if (height > 0) {
                        hFactor = (double) height / imageHeight;
                    }
                    double factor = Math.min(wFactor, hFactor);
                    newWidth = (int) (factor * imageWidth);
                    newHeight = (int) (factor * imageHeight);
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


                // Create bitmap from image.
                try {
                    if (scaleFactor > 1) {
                        // Subsample the original image to get a smaller image to save memory.
                        BitmapFactory.Options scaleOptions = new BitmapFactory.Options();
                        scaleOptions.inSampleSize = scaleFactor;
                        newBitmap = BitmapFactory.decodeByteArray(image, 0, image.length, scaleOptions);
                    } else {
                        newBitmap = BitmapFactory.decodeByteArray(image, 0, image.length);
                    }
                } catch (Throwable t) {
                    if (t instanceof OutOfMemoryError) System.gc();
                    Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                        "Create terminal bitmap " + bitmapNum + " from image byte array failed:" +
                            " Decode scaled bitmap for scale factor " + scaleFactor + " failed: " + t.getMessage());
                    return null;
                }
                if (newBitmap == null) {
                    Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                        "Create terminal bitmap " + bitmapNum + " from image byte array failed:" +
                            " Decoded scaled bitmap not set for scale factor " + scaleFactor);
                    return null;
                }


                // Crop the bitmap if it exceeds terminal bounds.
                int maxWidth = (terminalBuffer.mColumns - x) * cellWidth;
                if (newWidth > maxWidth) {
                    int cropWidth = newBitmap.getWidth() * maxWidth / newWidth;
                    try {
                        newBitmap = Bitmap.createBitmap(newBitmap, 0, 0, cropWidth, newBitmap.getHeight());
                        newWidth = maxWidth;
                    } catch (Throwable t) {
                        if (t instanceof OutOfMemoryError) {
                            // This is just a memory optimization. If it fails,
                            // continue (and probably fail later).
                            System.gc();
                        }

                    }
                }


                // Create final scaled bitmap.
                try {
                    newBitmap = Bitmap.createScaledBitmap(newBitmap, newWidth, newHeight, true);
                } catch (Throwable t) {
                    if (t instanceof OutOfMemoryError) System.gc();
                    Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                        "Create terminal bitmap " + bitmapNum + " from image byte array failed:" +
                            " Create scaled bitmap failed: " + t.getMessage());
                    return null;
                }
            } else {
                // Create bitmap from image.
                try {
                    newBitmap = BitmapFactory.decodeByteArray(image, 0, image.length);
                } catch (Throwable t) {
                    if (t instanceof OutOfMemoryError) System.gc();
                    Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                        "Create terminal bitmap " + bitmapNum + " from image byte array failed:" +
                            " Create full bitmap failed: " + t.getMessage());
                    return null;
                }
            }

            if (newBitmap == null) {
                Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                    "Create terminal bitmap " + bitmapNum + " from image byte array failed: New bitmap not set");
                return null;
            }


            newBitmap = resizeBitmapConstrained(LOG_TAG, "image byte array", terminalBuffer.getClient(), newBitmap,
                newBitmap.getWidth(), newBitmap.getHeight(), cellWidth, cellHeight,
                terminalBuffer.mColumns - x);
            TerminalBitmap terminalBitmap = build(terminalBuffer, bitmapNum, newBitmap, x, y, cellWidth, cellHeight);
            if (terminalBitmap == null) {
                return terminalBitmap;
            }

            terminalBitmap.setCursorDelta(new int[] {
                terminalBitmap.getScrollLines(),
                (terminalBitmap.getBitmap().getWidth() + cellWidth - 1) / cellWidth});

            return terminalBitmap;
        } catch (Throwable t) {
            if (t instanceof OutOfMemoryError) System.gc();
            Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                "Create terminal bitmap " + bitmapNum + " from image byte array failed: " + t.getMessage());
            return null;
        }
    }


    /** Build a {@link TerminalBitmap} from a {@link Bitmap}. */
    public static TerminalBitmap build(TerminalBuffer terminalBuffer, int bitmapNum, Bitmap bitmap,
                                       int x, int y, int cellWidth, int cellHeight) {
        try {
            return buildOrThrow(terminalBuffer, bitmapNum, bitmap, x, y, cellWidth, cellHeight);
        } catch (Throwable t) {
            if (t instanceof OutOfMemoryError) System.gc();
            Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                "Create terminal bitmap " + bitmapNum + " from bitmap failed: " + t.getMessage());
            return null;
        }
    }

    /** Build a {@link TerminalBitmap} from a {@link Bitmap}. */
    public static TerminalBitmap buildOrThrow(TerminalBuffer terminalBuffer, int bitmapNum, Bitmap bitmap,
                                              int x, int y, int cellWidth, int cellHeight) throws Throwable {
        if (bitmap == null) {
            throw new IllegalArgumentException("Cannot create terminal bitmap from an unset bitmap");
        }

        int bitmapWidth = bitmap.getWidth();
        int bitmapHeight = bitmap.getHeight();
        int width = Math.min(terminalBuffer.mColumns - x, (bitmapWidth + cellWidth - 1) / cellWidth);
        int height = (bitmapHeight + cellHeight - 1) / cellHeight;
        int s = 0;

        for (int i = 0; i < height; i++) {
            if (y + i - s == terminalBuffer.mScreenRows) {
                terminalBuffer.scrollDownOneLine(0, terminalBuffer.mScreenRows, TextStyle.NORMAL);
                s++;
            }
            for (int j = 0; j < width ; j++) {
                terminalBuffer.setChar(x + j, y + i - s, '+', TextStyle.encodeTerminalBitmap(bitmapNum, j, i));
            }
        }

        if (width * cellWidth < bitmapWidth) {
            bitmap = Bitmap.createBitmap(bitmap, 0, 0, width * cellWidth, bitmapHeight);
        }

        int scrollLines =  height - s;

        return new TerminalBitmap(terminalBuffer.getClient(), bitmapNum, bitmap,
            cellWidth, cellHeight, scrollLines, null);
    }



    public TerminalSessionClient getClient() {
        return mClient;
    }


    public int getBitmapNum() {
        return mBitmapNum;
    }

    public Bitmap getBitmap() {
        return mBitmap;
    }


    public int getCellWidth() {
        return mCellWidth;
    }

    public int getCellHeight() {
        return mCellHeight;
    }


    public int getScrollLines() {
        return mScrollLines;
    }


    public int[] getCursorDelta() {
        return mCursorDelta;
    }

    public void setCursorDelta(int[] cursorDelta) {
        mCursorDelta = cursorDelta;
    }





    public static Bitmap resizeBitmap(String logTag, String label, TerminalSessionClient client, Bitmap bitmap,
                                      int bitmapWidth, int bitmapHeight) {
        
        Bitmap newBitmap;
        try {
            int[] pixels = new int[bitmap.getAllocationByteCount()];

            bitmap.getPixels(pixels, 0, bitmap.getWidth(), 0, 0, bitmap.getWidth(), bitmap.getHeight());

            newBitmap = Bitmap.createBitmap(bitmapWidth, bitmapHeight, Bitmap.Config.ARGB_8888);

            int newWidth = Math.min(bitmap.getWidth(), bitmapWidth);
            int newHeight = Math.min(bitmap.getHeight(), bitmapHeight);
            newBitmap.setPixels(pixels, 0, bitmap.getWidth(), 0, 0, newWidth, newHeight);
            return newBitmap;
        } catch (Throwable t) {
            if (t instanceof OutOfMemoryError) System.gc();
            Logger.logError(client, logTag, "Resize " + label + " bitmap to" +
                " width " + bitmapWidth + " and height " + bitmapHeight + " failed: " + t.getMessage());
            return null;
        }
    }

    public static Bitmap resizeBitmapConstrained(String logTag, String label, TerminalSessionClient client, Bitmap bitmap,
                                                 int bitmapWidth, int bitmapHeight,
                                                 int cellWidth, int cellHeight, int columns) {
        // Width and height must be multiples of the cell width and height.
        // Bitmap should not extend beyond screen width.
        Bitmap originalBitmap = bitmap;
        if (bitmapWidth > cellWidth * columns || (bitmapWidth % cellWidth) != 0 || (bitmapHeight % cellHeight) != 0) {
            int newBitmapWidth = Math.min(cellWidth * columns, ((bitmapWidth - 1) / cellWidth) * cellWidth + cellWidth);
            int newBitmapHeight = ((bitmapHeight - 1) / cellHeight) * cellHeight + cellHeight;
            bitmap = resizeBitmap(logTag, label, client, originalBitmap, newBitmapWidth, newBitmapHeight);
            // Only a minor display glitch if resize failed.
            return bitmap != null ? bitmap : originalBitmap;
        } else {
            return originalBitmap;
        }
    }

}
