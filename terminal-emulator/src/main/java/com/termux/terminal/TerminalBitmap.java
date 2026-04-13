package com.termux.terminal;

import android.app.WallpaperManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RecordingCanvas;
import android.graphics.Rect;
import android.graphics.RectF;
import android.os.Build;

import java.util.Properties;

/**
 * A terminal bitmap for images.
 */
public class TerminalBitmap {

    public static final String LOG_TAG = "TerminalBitmap";

    private static int initMaxBitmapSize() {
        // Synced with `RecordingCanvas.MAX_BITMAP_SIZE`.
        // - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:frameworks/base/graphics/java/android/graphics/RecordingCanvas.java;l=42-50
        int defaultSize =
                Build.VERSION.SDK_INT >= Build.VERSION_CODES.VANILLA_ICE_CREAM ?
                    150 * 1024 * 1024 : // 150 MB
                    100 * 1024 * 1024;  // 100 MB

        Properties systemProperties = AndroidUtils.getSystemProperties(LOG_TAG);
        String maxTextureSizeString = systemProperties.getProperty("ro.hwui.max_texture_allocation_size");

        if (maxTextureSizeString == null) return defaultSize;

        try {
            int maxTextureSize = Integer.parseInt(maxTextureSizeString);
            return maxTextureSize > 0 ? maxTextureSize : defaultSize;
        }
        catch (Exception e) {
            return defaultSize;
        }
    }

    /**
     * The max size of a Terminal {@link Bitmap} for its pixels. The limit is defined as per how
     * `RecordingCanvas.MAX_BITMAP_SIZE` value is defined, check below for details. The value should
     * normally be between `100-200MB` depending on device and Android version.
     *
     * Each pixel is stored on 4 bytes for a {@link Bitmap.Config#ARGB_8888} bitmap color config.
     * The bitmap will have following memory usage for its respective resolution (`width x height x 4`).
     * - 1280x720 (HD): 3,686,400 bytes/3.6MB.
     * - 1920x1080 (FHD): 8,294,400 bytes/8MB.
     * - 2560x1440 (QHD): 14,745,600 bytes/14.7MB.
     * - 3840x2160 (4K UHD): 33,177,600 bytes/33MB.
     * - 7680x4320 (8K UHD): 132,710,400 bytes/132MB.
     * .
     * - https://en.wikipedia.org/wiki/Display_resolution_standards#High-definition
     *
     * The terminal uses {@link Canvas#drawBitmap(Bitmap, Rect, RectF, Paint)} to draw the bitmap
     * when `TerminalRenderer.render()` is called.
     *
     * The {@link Canvas} class defines `Canvas.MAXIMUM_BITMAP_SIZE` for the maximum dimension
     * for a bitmap which is returned by {@link Canvas#getMaximumBitmapWidth()} and
     * {@link Canvas#getMaximumBitmapHeight()}. It is hardcoded with the value `32766` as defined by
     * Skia (2D graphics library), which technically has the limit `32767` as it requires supporting
     * math on 16-bit buffers.
     * - https://cs.android.com/android/_/android/platform/frameworks/base/+/f61970fc79e9c5cf340fa942597628242361864a
     * - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:frameworks/base/graphics/java/android/graphics/Canvas.java;l=76-78
     * - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:external/skia/src/shaders/SkImageShader.cpp;l=254-267
     *
     * The {@link RecordingCanvas} class defines `RecordingCanvas.MAX_BITMAP_SIZE` for the
     * maximum size (not dimension) for a bitmap, which is checked by
     * `RecordingCanvas.throwIfCannotDraw()` when `BaseRecordingCanvas.drawBitmap()` is called.
     * The `RecordingCanvas` is a specialized implementation of the `Canvas` class that is designed
     * to record draw commands for deferred rendering instead of executing draw commands instantly.
     * By recording draw commands, they can be cached so that complex views can be efficiently
     * re-drawn without recalculating them again for every frame. The caching part is similar to
     * how a terminal behaves, where it stores all the bitmaps for rendering depending on scroll
     * position. So both `RecordingCanvas` and a terminal require similar limits on bitmap
     * sizes considering memory consumption limits of apps, and multiple bitmaps being loaded
     * instead of a single one like for wallpapers, hence why `TerminalBitmap.MAX_BITMAP_SIZE` is
     * synced with `RecordingCanvas`.
     * The `RecordingCanvas.MAX_BITMAP_SIZE` is set from `ro.hwui.max_texture_allocation_size`
     * system property if set for Android `>= 12`, otherwise `150MB` (`100MB` for Android `10-14`).
     * The values `>= 150MB` are enough to support `7680x4320` (8K UHD) bitmaps.
     * Some devices like larger xiaomi devices have `ro.hwui.max_texture_allocation_size` set to `209715200` (`200MB`).
     * - https://cs.android.com/android/_/android/platform/frameworks/base/+/e4d011201cea40d46cb2b2eef401db8fddc5c9c6
     * - https://cs.android.com/android/_/android/platform/frameworks/base/+/0e717a9d06ded980908649393bd73e46ffafcd54
     * - https://cs.android.com/android/_/android/platform/frameworks/base/+/97396260ed06cc9d1834d4d8e4e649a3ef09f1f3
     * - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:frameworks/base/graphics/java/android/graphics/RecordingCanvas.java;l=42-50
     *
     * The Android wallpaper manager service also checks if dimensions of cropped wallpaper exceeds
     * max texture size that the GPU can support, otherwise it will cause System UI to keep crashing
     * because it can not initialize EGL with an appropriate surface. The `GLHelper.getMaxTextureSize()`
     * returns the max texture size, which is defined by `sys.max_texture_size` system property if set,
     * otherwise by value for `GL_MAX_TEXTURE_SIZE`. The `sys.max_texture_size` defines the maximum
     * width or height of a texture, not total size. Its value can be low like `2048` or high like
     * `16384` for 16K support.
     * - https://cs.android.com/android/_/android/platform/frameworks/base/+/32c6a7c691b0d91085c1ed13fe6f1c473c94b4c8
     * - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:frameworks/base/services/core/java/com/android/server/wallpaper/WallpaperCropper.java;l=461
     * - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:frameworks/base/services/core/java/com/android/server/wallpaper/GLHelper.java;l=145
     * - https://developer.android.com/reference/android/opengl/GLES10#GL_MAX_TEXTURE_SIZE
     *
     * The {@link WallpaperManager#getDesiredMinimumWidth()} and {@link WallpaperManager#getDesiredMinimumHeight()}
     * can also be called to get minimum suggested width and height of the wallpaper that an app
     * should use when setting the wallpaper. This normally is equal to the width and height of the
     * current device display, but the width can be higher than display width if the homescreen is
     * scrollable horizontally with multiple pages, in which case the width returned is equal to
     * entire workspace width. The launcher apps can provide Android their desired width and height
     * dimensions depending on the homescreen pages config by calling
     * {@link WallpaperManager#suggestDesiredDimensions(int, int)}, which also ensures that values
     * passed are scaled down to `sys.max_texture_size` system property if its set.
     * - https://cs.android.com/android/_/android/platform/frameworks/base/+/289c273ec49462c7bfdbf6238e9016936da7307c
     * - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:frameworks/base/core/java/android/app/WallpaperManager.java;l=2737-2794
     * - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:frameworks/base/services/core/java/com/android/server/wallpaper/WallpaperManagerService.java;l=2330-2366
     * - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:frameworks/base/services/core/java/com/android/server/wallpaper/WallpaperDisplayHelper.java;l=108-115
     * - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:frameworks/base/core/java/android/view/Display.java;l=1052-1063
     *
     * If an app specifies `largeHeap=true` in its `AndroidManifest.xml`, then it can be allocated
     * larger heap memory to load larger bitmaps maps instead of resulting in an OOM. The Termux app
     * does not have it enabled, and hence is more likely to have OOMs when loading larger bitmaps.
     * - https://developer.android.com/guide/topics/manifest/application-element#largeHeap
     * - https://developer.android.com/topic/performance/memory
     */
    public static final int MAX_BITMAP_SIZE = initMaxBitmapSize();



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
            int newBitmapSize = bitmapWidth * bitmapHeight * 4;
            if (newBitmapSize < 0 || newBitmapSize > MAX_BITMAP_SIZE) {
                Logger.logError(client, logTag, "The new " + label + " bitmap after resize with" +
                    " width " + bitmapWidth + " and height " + bitmapHeight +
                    " has size " + newBitmapSize + " greater than max bitmap size " + MAX_BITMAP_SIZE);
                return null;
            }

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
