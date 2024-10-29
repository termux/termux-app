package com.termux.shared.image;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.graphics.BitmapFactory;
import android.graphics.BlendMode;
import android.graphics.BlendModeColorFilter;
import android.graphics.ImageDecoder;
import android.graphics.Point;
import android.graphics.PorterDuff;
import android.graphics.PorterDuffColorFilter;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.Build;
import android.provider.MediaStore;

import com.termux.shared.errors.Error;
import com.termux.shared.file.FileUtils;
import com.termux.shared.file.FileUtilsErrno;
import com.termux.shared.logger.Logger;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;

public final class ImageUtils {

    /**
     * Request code that can be used to distinguish Activity result.
     */
    public static final int REQUEST_CODE_IMAGE = 100;

    /**
     * Compression quality used to compress image. The value is interpreted differently depending on the {@link CompressFormat CompressFormat}.
     */
    public static final int COMPRESS_QUALITY = 80;

    /**
     * Tolerance for diffrence in original image required optimized image.
     */
    public static final int OPTIMALITY_TOLERANCE = 50;

    public static final String IMAGE_TYPE = "image";

    public static final String ANY_IMAGE_TYPE = IMAGE_TYPE + "/*";


    private static final String LOG_TAG = "ImageUtils";

    /**
     * Don't let anyone instantiate this class.
     */
    private ImageUtils() {
    }

    /**
     * Get an {@link Bitmap} image from the {@link Uri}.
     *
     * @param context The context for the operations.
     * @param uri     The uri from where image content will be loaded.
     * @return Bitmap containing the image, or return {@code null} if failed to
     * load bitmap content.
     */
    public static Bitmap getBitmap(final Context context, Uri uri) {
        Bitmap bitmap = null;

        try {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                bitmap = ImageDecoder.decodeBitmap(ImageDecoder.createSource(context.getContentResolver(), uri));
            } else {
                bitmap = MediaStore.Images.Media.getBitmap(context.getContentResolver(), uri);
            }
            bitmap = MediaStore.Images.Media.getBitmap(context.getContentResolver(), uri);
        } catch (IOException e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to load bitmap from " + uri, e);
        }
        return bitmap;
    }

    /**
     * Generate image bitmap from the path.
     *
     * @param path The path for image file
     * @return Bitmap generated from image path, if fails to
     * to generate returns {@code null}
     */
    public static Bitmap getBitmap(String path) {
        return BitmapFactory.decodeFile(path);
    }


    /**
     * Creates an centered and resized {@link Bitmap} according to given size.
     *
     * @param bitmap Original bitmap source to resize.
     * @param point  Target size containing width and height to resize.
     * @return Returns the resized bitmap for given size.
     */
    public static Bitmap resizeBitmap(Bitmap bitmap, Point point) {
        return ThumbnailUtils.extractThumbnail(bitmap, point.x, point.y);
    }

    /**
     * Wrapper for {@link #compressAndSaveBitmap(Bitmap, int, String)} with `{@link #COMPRESS_QUALITY} `quality`.
     */
    public static Error compressAndSaveBitmap(Bitmap bitmap, String path) {
        return compressAndSaveBitmap(bitmap, COMPRESS_QUALITY, path);
    }

    /**
     * Wrapper for {@link #compressAndSaveBitmap(Bitmap, CompressFormat, int, String)} with `{@link Bitmap.CompressFormat#JPEG}` `format`.
     */
    public static Error compressAndSaveBitmap(Bitmap bitmap, int quality, String path) {
        return compressAndSaveBitmap(bitmap, Bitmap.CompressFormat.JPEG, quality, path);
    }

    /**
     * Compress the given bitmap image file for given format and quality.
     *
     * @param bitmap  Original source bitmap.
     * @param foramt  The format for image compression.
     * @param quality Hint to the compressor, 0-100. The value is interpreted differently
     *               depending on the {@link Bitmap.CompressFormat}.
     * @param path    The path to store compressed bitmap.
     * @return Returns the {@code error} if compression and save operation was not successful,
     * otherwise {@code null}.
     */
    public static Error compressAndSaveBitmap(Bitmap bitmap, Bitmap.CompressFormat foramt, int quality, String path) {
        FileUtils.deleteRegularFile(null, path, true);
        Error error = FileUtils.createRegularFile(path);

        if (error != null)
            return error;

        try (FileOutputStream out = new FileOutputStream(path)) {
            bitmap.compress(foramt, quality, out);
        } catch (Exception e) {
            FileUtils.deleteRegularFile(null, path, true);
            error = FileUtilsErrno.ERRNO_CREATING_FILE_FAILED_WITH_EXCEPTION.getError(e, e.getMessage());
        }

        return error;
    }


    /**
     * Wrapper for {@link #getDrawable(String)} with `file.getAbsolutePath()` `path` of file.
     */
    public static Drawable getDrawable(File file) {
        String path = file.getAbsolutePath();

        return getDrawable(path);
    }

    /**
     * Create {@link BitmapDrawable} from specified file path.
     *
     * @param path The path file to load image bitmap drawable.
     * @return Drawable created from image file path.
     */
    public static Drawable getDrawable(String path) {
        return BitmapDrawable.createFromPath(path);
    }

    /**
     * Add an overlay color/tint on image with {@link BlendMode#MULTIPLY}.
     *
     * @param drawable The source image bitmap drawable.
     * @param color    Overlay color for image.
     */
    public static void addOverlay(Drawable drawable, int color) {

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            drawable.setColorFilter(new BlendModeColorFilter(color, BlendMode.DARKEN));
        } else {
            drawable.setColorFilter(new PorterDuffColorFilter(color, PorterDuff.Mode.DARKEN));
        }
    }


    /**
     * Wrapper for {@link #isImageOptimized(String, Point, int)} with `{@link #OPTIMALITY_TOLERANCE}` `tolerance`.
     */
    public static boolean isImageOptimized(String path, Point point) {
        return isImageOptimized(path, point, OPTIMALITY_TOLERANCE);
    }

    /**
     * Wrapper for {@link #isImageOptimized(String, int, int, int)} with `width` and `height` obtained from {@link Point}.
     */
    public static boolean isImageOptimized(String path, Point point, int tolerance) {
        return isImageOptimized(path, point.x, point.y, tolerance);
    }

    /**
     * Check whether the image file present at file location is optimized corresponding
     * to given width and height. It can tolorent error upto given value.
     *
     * @param path      The file path of image.
     * @param width     The required width of image file.
     * @param height    The required width of image file.
     * @param tolerance The tolerance value upto which diffrence is ignored.
     * @return Returns whether the given image is optimized or not.
     */
    public static boolean isImageOptimized(String path, int width, int height, int tolerance) {

        if (!FileUtils.regularFileExists(path, false)) {
            Logger.logInfo(LOG_TAG, "Image file " + path + " does not exist.");
            return false;
        }

        BitmapFactory.Options opt = new BitmapFactory.Options();
        opt.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path, opt);

        int imgWidth = opt.outWidth;
        int imgHeight = opt.outHeight;

        return Math.abs(imgWidth - width) <= tolerance && Math.abs(imgHeight - height) <= tolerance;
    }

    public static boolean isImage(String path) {
        if (!FileUtils.regularFileExists(path, false)) {
            Logger.logInfo(LOG_TAG, "Image file " + path + " does not exist.");
            return false;
        }

        BitmapFactory.Options opt = new BitmapFactory.Options();
        opt.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(path, opt);

        return opt.outWidth != -1 && opt.outHeight != -1;
    }

    /**
     * Resize bitmap for {@link Configuration#ORIENTATION_PORTRAIT Portrait} and {@link
     * Configuration#ORIENTATION_LANDSCAPE Landscape} display view.
     * Also Compress the image bitmap before saving it to given path.
     *
     * @param bitmap The original bitmap image to resize and store.
     * @param point  Display resolution containing width and height.
     * @param path1  The path for storing image with width point.x and height point.y
     * @param path2  The path for storing image with width point.y and height point.x
     * @return Returns the {@code error} if save operation was not successful,
     * otherwise {@code null}.
     */
    public static Error saveForDisplayResolution(Bitmap bitmap, Point point, String path1, String path2) {

        Error error;
        Bitmap bitmap1 = resizeBitmap(bitmap, point);
        error = compressAndSaveBitmap(bitmap1, path1);

        if (error != null) {
            return error;
        }

        Bitmap bitmap2 = resizeBitmap(bitmap, new Point(point.y, point.x));
        error = compressAndSaveBitmap(bitmap2, path2);

        return error;
    }

    /**
     * Check for the given {@link Drawable} whether it is instance of {@link
     * BitmapDrawable} or not.
     *
     * @param drawable The drawable to check.
     * @return Retruns whether drawable is bitmap drawable.
     */
    public static boolean isBitmapDrawable(Drawable drawable) {
        return drawable instanceof BitmapDrawable;
    }

}
