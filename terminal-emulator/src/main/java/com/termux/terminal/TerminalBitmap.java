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

import java.util.HashSet;
import java.util.Properties;
import java.util.Set;

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

    /**
     * Whether this bitmap is a placement of a kitty graphics image, in which case
     * {@link #mKittyImageId} and {@link #mKittyPlacementId} define which placement it is of which
     * image, although both may be unset if the client did not pass the `i` and `p` keys.
     *
     * The association is stored in the {@link TerminalBitmap} itself instead of a separate registry
     * so that it does not need to be kept in sync when bitmaps are removed by
     * {@link TerminalBuffer#doTerminalBitmapsGC(int)} and
     * {@link TerminalBuffer#removeScrolledOutTerminalBitmaps(int)}, since bitmap numbers are reused
     * for new bitmaps.
     */
    protected boolean mIsKittyImage;

    /** The kitty graphics image id this bitmap is a placement of, if {@link #mIsKittyImage}. */
    protected long mKittyImageId = KittyImage.IMAGE_ID__NONE;

    /** The kitty graphics placement id of this bitmap, if {@link #mIsKittyImage}. */
    protected long mKittyPlacementId = KittyImage.PLACEMENT_ID__NONE;


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
                if (imageWidth < 1 || imageHeight < 1) {
                    // The image dimensions are not known if the image data could not be decoded.
                    // Bailing out early is required since a `0` dimension would otherwise result in
                    // a division by `0` while calculating the factors below and in the `scaleFactor`
                    // loop below never terminating.
                    Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                        "Create terminal bitmap " + bitmapNum + " from image byte array failed:" +
                            " Decoded bitmap bounds " + imageWidth + "x" + imageHeight + " are not valid");
                    return null;
                }

                int[] newSize = getScaledImageSize(imageWidth, imageHeight, width, height, shouldPreserveAspectRatio);
                newWidth = newSize[0];
                newHeight = newSize[1];

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


    /**
     * Build a {@link TerminalBitmap} for a placement of a kitty graphics image.
     *
     * The kitty graphics protocol has its own image formats and allows a source rectangle of the
     * image to be displayed instead of the entire image, so
     * {@link #build(TerminalBuffer, int, byte[], int, int, int, int, int, int, boolean)} used for
     * iTerm images cannot be used. The source rectangle must be cropped before the image is scaled
     * to the cells it is to be displayed in, otherwise the wrong part of the image would be scaled.
     *
     * Note that unlike the iTerm image path, a `PNG` is decoded at its full size instead of being
     * subsampled while decoding, since the source rectangle coordinates are in the coordinate space
     * of the full size image. The size of the bitmap that would be created is checked against
     * {@link #MAX_BITMAP_SIZE} before decoding instead.
     *
     * @param format The kitty graphics image format, check {@link KittyImage#FORMAT__PNG} and
     *               {@link KittyImage#isRawFormat()} for more info.
     * @param image The image data, which is the raw pixel data for the raw formats, in which case
     *              its length must be `pixelWidth * pixelHeight * <bytes per pixel>`.
     * @param pixelWidth The width in pixels of the image data, required for the raw formats.
     * @param pixelHeight The height in pixels of the image data, required for the raw formats.
     * @param sourceX The x coordinate in pixels of the source rectangle of the image to display.
     * @param sourceY The y coordinate in pixels of the source rectangle of the image to display.
     * @param sourceWidth The width in pixels of the source rectangle, or a value `< 1` for the rest
     *                    of the image after `sourceX`.
     * @param sourceHeight The height in pixels of the source rectangle, or a value `< 1` for the
     *                     rest of the image after `sourceY`.
     */
    public static TerminalBitmap buildForKittyImage(TerminalBuffer terminalBuffer, int bitmapNum,
                                                    int format, byte[] image,
                                                    int pixelWidth, int pixelHeight,
                                                    int sourceX, int sourceY,
                                                    int sourceWidth, int sourceHeight,
                                                    int x, int y, int cellWidth, int cellHeight,
                                                    int width, int height, boolean shouldPreserveAspectRatio) {
        try {
            if (image == null) {
                Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                    "Create terminal bitmap " + bitmapNum + " for kitty image failed: Image data not set");
                return null;
            }

            // A `PNG` can only be subsampled while decoding it if the entire image is displayed,
            // since the source rectangle coordinates are in the coordinate space of the full size
            // image, so subsampling would crop the wrong region of a smaller image.
            boolean hasSourceRectangle = sourceX > 0 || sourceY > 0 || sourceWidth > 0 || sourceHeight > 0;

            Bitmap newBitmap;
            if (format == KittyImage.FORMAT__PNG) {
                newBitmap = createBitmapFromPng(terminalBuffer.getClient(), bitmapNum, image,
                    !hasSourceRectangle, width, height, shouldPreserveAspectRatio);
            } else {
                newBitmap = createBitmapFromRawPixels(terminalBuffer.getClient(), bitmapNum, image,
                    KittyImage.getBytesPerPixel(format), pixelWidth, pixelHeight);
            }

            if (newBitmap == null) {
                Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                    "Create terminal bitmap " + bitmapNum + " for kitty image failed: New bitmap not set");
                return null;
            }

            newBitmap = cropBitmapToSourceRectangle(terminalBuffer.getClient(), bitmapNum, newBitmap,
                sourceX, sourceY, sourceWidth, sourceHeight);
            if (newBitmap == null) {
                return null;
            }

            int bitmapWidth = newBitmap.getWidth();
            int bitmapHeight = newBitmap.getHeight();
            if (bitmapWidth < 1 || bitmapHeight < 1) {
                Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                    "Create terminal bitmap " + bitmapNum + " for kitty image failed:" +
                        " The bitmap dimensions " + bitmapWidth + "x" + bitmapHeight + " are not valid");
                return null;
            }

            if (height > 0 || width > 0) {
                int[] newSize = getScaledImageSize(bitmapWidth, bitmapHeight, width, height, shouldPreserveAspectRatio);
                if (newSize[0] < 1 || newSize[1] < 1) {
                    Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                        "Create terminal bitmap " + bitmapNum + " for kitty image failed:" +
                            " The scaled size " + newSize[0] + "x" + newSize[1] + " is not valid");
                    return null;
                }

                if (newSize[0] != bitmapWidth || newSize[1] != bitmapHeight) {
                    try {
                        newBitmap = Bitmap.createScaledBitmap(newBitmap, newSize[0], newSize[1], true);
                    } catch (Throwable t) {
                        if (t instanceof OutOfMemoryError) System.gc();
                        Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                            "Create terminal bitmap " + bitmapNum + " for kitty image failed:" +
                                " Create scaled bitmap failed: " + t.getMessage());
                        return null;
                    }

                    if (newBitmap == null) {
                        Logger.logError(terminalBuffer.getClient(), LOG_TAG,
                            "Create terminal bitmap " + bitmapNum + " for kitty image failed: Scaled bitmap not set");
                        return null;
                    }
                }
            }

            newBitmap = resizeBitmapConstrained(LOG_TAG, "kitty image", terminalBuffer.getClient(), newBitmap,
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
                "Create terminal bitmap " + bitmapNum + " for kitty image failed: " + t.getMessage());
            return null;
        }
    }

    /**
     * Create a {@link Bitmap} from `PNG` image data of a kitty graphics image.
     *
     * @param canSubsample Whether the image may be subsampled while decoding it to save memory,
     *                     which requires the entire image to be displayed, check
     *                     {@link #buildForKittyImage(TerminalBuffer, int, int, byte[], int, int, int, int, int, int, int, int, int, int, int, int, boolean)}
     *                     for more info.
     * @param width The width in pixels the image is to be displayed in, or a value `< 1` if it is to
     *              be displayed at its own size, in which case it cannot be subsampled.
     * @param height The height in pixels the image is to be displayed in, or a value `< 1` if it is
     *               to be displayed at its own size, in which case it cannot be subsampled.
     */
    private static Bitmap createBitmapFromPng(TerminalSessionClient client, int bitmapNum, byte[] image,
                                              boolean canSubsample, int width, int height,
                                              boolean shouldPreserveAspectRatio) {
        try {
            // Get image dimensions without creating a bitmap so that an image that is too large to
            // be drawn is not decoded at all.
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            BitmapFactory.decodeByteArray(image, 0, image.length, options);

            int imageWidth = options.outWidth;
            int imageHeight = options.outHeight;

            if (imageWidth > 0 && imageHeight > 0) {
                if (!isBitmapSizeValid(client, bitmapNum, imageWidth, imageHeight)) {
                    return null;
                }

                if (canSubsample && (width > 0 || height > 0)) {
                    int[] newSize = getScaledImageSize(imageWidth, imageHeight, width, height, shouldPreserveAspectRatio);
                    if (newSize[0] > 0 && newSize[1] > 0) {
                        int scaleFactor = 1;
                        while (imageHeight >= 2 * newSize[1] * scaleFactor && imageWidth >= 2 * newSize[0] * scaleFactor) {
                            scaleFactor = scaleFactor * 2;
                        }

                        if (scaleFactor > 1) {
                            // Subsample the original image to get a smaller image to save memory.
                            BitmapFactory.Options scaleOptions = new BitmapFactory.Options();
                            scaleOptions.inSampleSize = scaleFactor;
                            return BitmapFactory.decodeByteArray(image, 0, image.length, scaleOptions);
                        }
                    }
                }
            }

            return BitmapFactory.decodeByteArray(image, 0, image.length);
        } catch (Throwable t) {
            if (t instanceof OutOfMemoryError) System.gc();
            Logger.logError(client, LOG_TAG, "Create bitmap for" +
                " terminal bitmap " + bitmapNum + " from png image data failed: " + t.getMessage());
            return null;
        }
    }

    /**
     * Crop a {@link Bitmap} to the source rectangle of a kitty graphics placement.
     *
     * @return Returns the cropped bitmap, the original bitmap if the source rectangle covers the
     * entire bitmap, or `null` if the source rectangle is not within the bounds of the bitmap.
     */
    private static Bitmap cropBitmapToSourceRectangle(TerminalSessionClient client, int bitmapNum, Bitmap bitmap,
                                                      int sourceX, int sourceY, int sourceWidth, int sourceHeight) {
        int bitmapWidth = bitmap.getWidth();
        int bitmapHeight = bitmap.getHeight();

        // The width and height default to the rest of the image after the x and y offset.
        int width = (sourceWidth > 0) ? sourceWidth : bitmapWidth - sourceX;
        int height = (sourceHeight > 0) ? sourceHeight : bitmapHeight - sourceY;

        if (sourceX == 0 && sourceY == 0 && width == bitmapWidth && height == bitmapHeight) {
            return bitmap;
        }

        if (sourceX < 0 || sourceY < 0 || width < 1 || height < 1 ||
            sourceX + width > bitmapWidth || sourceY + height > bitmapHeight) {
            Logger.logError(client, LOG_TAG, "Crop bitmap for" +
                " terminal bitmap " + bitmapNum + " failed: The source rectangle" +
                " " + sourceX + "," + sourceY + " " + width + "x" + height +
                " is not within the bitmap dimensions " + bitmapWidth + "x" + bitmapHeight);
            return null;
        }

        try {
            return Bitmap.createBitmap(bitmap, sourceX, sourceY, width, height);
        } catch (Throwable t) {
            if (t instanceof OutOfMemoryError) System.gc();
            Logger.logError(client, LOG_TAG, "Crop bitmap for" +
                " terminal bitmap " + bitmapNum + " to source rectangle" +
                " " + sourceX + "," + sourceY + " " + width + "x" + height + " failed: " + t.getMessage());
            return null;
        }
    }

    /** Whether a bitmap of the dimensions can be drawn as per {@link #MAX_BITMAP_SIZE}. */
    private static boolean isBitmapSizeValid(TerminalSessionClient client, int bitmapNum, int bitmapWidth, int bitmapHeight) {
        long bitmapSize = (long) bitmapWidth * bitmapHeight * 4;
        if (bitmapSize > MAX_BITMAP_SIZE) {
            Logger.logError(client, LOG_TAG,
                "The bitmap for terminal bitmap " + bitmapNum + " with" +
                    " dimensions " + bitmapWidth + "x" + bitmapHeight +
                    " has size " + bitmapSize + " greater than max bitmap size " + MAX_BITMAP_SIZE);
            return false;
        }
        return true;
    }

    /**
     * Create an {@link Bitmap.Config#ARGB_8888} {@link Bitmap} from raw `RGB` or `RGBA` pixel data
     * of a kitty graphics image. The alpha channel is expected to not be premultiplied.
     */
    private static Bitmap createBitmapFromRawPixels(TerminalSessionClient client, int bitmapNum, byte[] pixels,
                                                    int bytesPerPixel, int pixelWidth, int pixelHeight) {
        if (bytesPerPixel < 3 || pixelWidth < 1 || pixelHeight < 1 ||
            pixels.length != pixelWidth * pixelHeight * bytesPerPixel) {
            Logger.logError(client, LOG_TAG, "Create bitmap for" +
                " terminal bitmap " + bitmapNum + " from raw pixels failed:" +
                " Pixel data of length " + pixels.length +
                " does not match dimensions " + pixelWidth + "x" + pixelHeight +
                " with " + bytesPerPixel + " bytes per pixel");
            return null;
        }

        if (!isBitmapSizeValid(client, bitmapNum, pixelWidth, pixelHeight)) {
            return null;
        }

        try {
            int[] colors = new int[pixelWidth * pixelHeight];
            for (int i = 0; i < colors.length; i++) {
                int offset = i * bytesPerPixel;
                int red = pixels[offset] & 0xff;
                int green = pixels[offset + 1] & 0xff;
                int blue = pixels[offset + 2] & 0xff;
                int alpha = (bytesPerPixel > 3) ? (pixels[offset + 3] & 0xff) : 0xff;
                colors[i] = (alpha << 24) | (red << 16) | (green << 8) | blue;
            }

            return Bitmap.createBitmap(colors, pixelWidth, pixelHeight, Bitmap.Config.ARGB_8888);
        } catch (Throwable t) {
            if (t instanceof OutOfMemoryError) System.gc();
            Logger.logError(client, LOG_TAG, "Create bitmap for" +
                " terminal bitmap " + bitmapNum + " from raw pixels with" +
                " dimensions " + pixelWidth + "x" + pixelHeight + " failed: " + t.getMessage());
            return null;
        }
    }

    /**
     * Get the size to scale an image to so that it is displayed in the requested `width` and
     * `height` in pixels, where a value `<= 0` means that the respective dimension of the image
     * itself should be used.
     *
     * @return Returns an array with the new width as the first value and the new height as the
     * second value.
     */
    private static int[] getScaledImageSize(int imageWidth, int imageHeight,
                                            int width, int height, boolean shouldPreserveAspectRatio) {
        int newWidth = width;
        int newHeight = height;

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

        return new int[] {newWidth, newHeight};
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

        // The bitmaps of the cells that are overwritten by this bitmap may no longer be referenced by
        // any cell afterwards, in which case they must be released, otherwise a client that displays
        // a new image at the same position for every frame would keep all of them in memory.
        Set<Integer> overwrittenBitmapNums = null;

        for (int i = 0; i < height; i++) {
            if (y + i - s == terminalBuffer.mScreenRows) {
                terminalBuffer.scrollDownOneLine(0, terminalBuffer.mScreenRows, TextStyle.NORMAL);
                s++;
            }
            for (int j = 0; j < width ; j++) {
                int row = y + i - s;

                int overwrittenBitmapNum = TextStyle.getTerminalBitmapNum(terminalBuffer.getStyleAt(row, x + j));
                if (overwrittenBitmapNum >= TerminalBuffer.TERMINAL_BITMAP__NUM_START && overwrittenBitmapNum != bitmapNum) {
                    if (overwrittenBitmapNums == null) overwrittenBitmapNums = new HashSet<>();
                    overwrittenBitmapNums.add(overwrittenBitmapNum);
                }

                terminalBuffer.setChar(x + j, row, '+', TextStyle.encodeTerminalBitmap(bitmapNum, j, i));
            }
        }

        terminalBuffer.releaseUnreferencedTerminalBitmaps(overwrittenBitmapNums);

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


    public boolean isKittyImage() {
        return mIsKittyImage;
    }

    public long getKittyImageId() {
        return mKittyImageId;
    }

    public long getKittyPlacementId() {
        return mKittyPlacementId;
    }

    /** Mark this bitmap as a placement of a kitty graphics image. */
    public void setKittyImage(long kittyImageId, long kittyPlacementId) {
        mIsKittyImage = true;
        mKittyImageId = kittyImageId;
        mKittyPlacementId = kittyPlacementId;
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
