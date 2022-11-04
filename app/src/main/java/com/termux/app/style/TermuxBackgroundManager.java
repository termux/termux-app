package com.termux.app.style;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Point;
import android.graphics.drawable.Drawable;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContract;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;

import com.termux.R;
import com.termux.app.TermuxActivity;
import com.termux.shared.activity.ActivityUtils;
import com.termux.shared.data.DataUtils;
import com.termux.shared.errors.Error;
import com.termux.shared.file.FileUtils;
import com.termux.shared.image.ImageUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.view.ViewUtils;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TextStyle;

public class TermuxBackgroundManager {

    /** Require {@link TermuxActivity} to perform operations. */
    private final TermuxActivity mActivity;

    /** A launcher for start the process of executing an {@link ActivityResultContract}. */
    private final ActivityResultLauncher<String> mActivityResultLauncher;

    /** Termux app shared preferences manager. */
    private final TermuxAppSharedPreferences mPreferences;


    private static final String LOG_TAG = "TermuxBackgroundManager";

    public TermuxBackgroundManager(TermuxActivity activity) {
        this.mActivity = activity;
        this.mPreferences = activity.getPreferences();
        this.mActivityResultLauncher = registerActivityResultLauncher();
    }

    /**
     * Registers for activity result launcher. It's safe to call before fragment
     * or activity is created.
     *
     * @return A launcher for executing an {@link ActivityResultContract}.
     */
    private ActivityResultLauncher<String> registerActivityResultLauncher() {
        return mActivity.registerForActivityResult(new ActivityResultContracts.GetContent(), uri -> {

            if (uri != null) {
                try {
                    new Thread(() -> {
                        Bitmap bitmap = ImageUtils.getBitmap(mActivity, uri);

                        if (bitmap == null) {
                            Logger.logErrorAndShowToast(mActivity, LOG_TAG, mActivity.getString(R.string.error_background_image_loading_from_gallery_failed));
                            return;
                        }

                        Point size = ViewUtils.getDisplaySize(mActivity, false);
                        boolean isLandscape = ViewUtils.getDisplayOrientation(mActivity) == Configuration.ORIENTATION_LANDSCAPE;
                        Error error;

                        if (isLandscape) {
                            error = ImageUtils.saveForDisplayResolution(bitmap, size, TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH, TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH);

                        } else {
                            error = ImageUtils.saveForDisplayResolution(bitmap, DataUtils.swap(size), TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH, TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH);
                        }

                        if (error != null) {
                            Logger.logErrorAndShowToast(mActivity, LOG_TAG, mActivity.getString(R.string.error_background_image_loading_from_gallery_failed));
                            return;
                        }

                        notifyBackgroundUpdated(true);

                        Logger.logInfo(LOG_TAG, "Image received successfully from the gallary.");
                        Logger.logDebug(LOG_TAG, "Storing background portrait image to " + TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH);
                        Logger.logDebug(LOG_TAG, "Storing background landscape image to " + TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH);
                    }).start();

                } catch (Exception e) {
                    Logger.logStackTraceWithMessage(LOG_TAG, "Failed to load image", e);
                    Logger.showToast(mActivity, mActivity.getString(R.string.error_background_image_loading_from_gallery_failed), true);
                }
            }
        });
    }

    /**
     * Check whether the optimized background image for {@link Configuration#ORIENTATION_PORTRAIT
     * Portrait} and {@link Configuration#ORIENTATION_LANDSCAPE Landscape} display view exist.
     *
     * @param context The context for operation.
     * @return Returns whether the optimized background image exist or not.
     */
    public static boolean isImageFilesExist(@NonNull Context context) {
        boolean isLandscape = (ViewUtils.getDisplayOrientation(context) == Configuration.ORIENTATION_LANDSCAPE);
        Point size = ViewUtils.getDisplaySize(context, false);

        String imagePath1 = isLandscape ? TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH : TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH;

        String imagePath2 = isLandscape ? TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH : TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH;

        return ImageUtils.isImageOptimized(imagePath1, size) && ImageUtils.isImageOptimized(imagePath2, DataUtils.swap(size));
    }



    /**
     * Enable background image loading. If the image already exist then ask for restore otherwise pick from gallery.
     */
    public void setBackgroundImage() {
        if (!mPreferences.isBackgroundImageEnabled() && isImageFilesExist(mActivity)) {
            restoreBackgroundImages();

        } else {
            pickImageFromGallery();
        }
    }

    /**
     * Disable background image loading and notify about the changes.
     * If image files are not deleted then it can be used to restore
     * when resetting background to image.
     *
     * @param deleteFiles The {@code boolean} that decides if it should delete the image files.
     */
    public void removeBackgroundImage(boolean deleteFiles) {
        if (deleteFiles) {
            FileUtils.deleteRegularFile(null, TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH, true);
            FileUtils.deleteRegularFile(null, TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH, true);
        }

        notifyBackgroundUpdated(false);
    }

    /** {@link ActivityResultLauncher#launch(Object) Launch} Activity for result to pick image from gallery. */
    private void pickImageFromGallery() {
        ActivityUtils.startActivityForResult(mActivity, mActivityResultLauncher, ImageUtils.ANY_IMAGE_TYPE);
    }

    /**
     * If the background images exist then ask user whether to restore them or not.
     * If denied pick from gallery.
     */
    private void restoreBackgroundImages() {
        AlertDialog.Builder b = new AlertDialog.Builder(mActivity);

        b.setMessage(R.string.title_restore_background_image);
        b.setPositiveButton(R.string.action_yes, (dialog, id) -> {
            notifyBackgroundUpdated(true);
        });

        b.setNegativeButton(R.string.action_no, ((dialog, id) -> {
            pickImageFromGallery();
        }));

        b.show();
    }


    /**
     * Updates background to image or solid color. If forced then load again even if
     * the background is already set. Forced update is require when the display orientation
     * is changed.
     *
     * @param forced Force background update task.
     */
    public void updateBackground(boolean forced) {
        if (!mActivity.isVisible()) return;

        if (mActivity.getPreferences().isBackgroundImageEnabled()) {

            Drawable drawable = mActivity.getWindow().getDecorView().getBackground();

            // If it's not forced update and background is already drawn,
            // then avoid reloading of image.
            if (!forced && ImageUtils.isBitmapDrawable(drawable)) {
                return;
            }

            updateBackgroundImage();
        } else {
            updateBackgroundColor();
        }
    }

    /**
     * Set background to color.
     */
    public void updateBackgroundColor() {
        if (!mActivity.isVisible()) return;

        TerminalSession session = mActivity.getCurrentSession();

        if (session != null && session.getEmulator() != null) {
            mActivity.getWindow().getDecorView().setBackgroundColor(session.getEmulator().mColors.mCurrentColors[TextStyle.COLOR_INDEX_BACKGROUND]);
        }
    }

    /**
     * Set background to image corresponding to display orientation.
     */
    public void updateBackgroundImage() {
        boolean isLandscape = ViewUtils.getDisplayOrientation(mActivity) == Configuration.ORIENTATION_LANDSCAPE;

        String imagePath = isLandscape ? TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH : TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH;

        if (isImageFilesExist(mActivity)) {
            try {
                Drawable drawable = ImageUtils.getDrawable(imagePath);
                ImageUtils.addOverlay(drawable, mActivity.getProperties().getBackgroundOverlayColor());
                mActivity.getWindow().getDecorView().setBackground(drawable);

            } catch (Exception e) {
                Logger.logStackTraceWithMessage(LOG_TAG, "Failed to load image", e);
                Logger.showToast(mActivity, mActivity.getString(R.string.error_background_image_loading_from_gallery_failed), true);

                // Since loading of image is failed, Set background to solid color.
                updateBackgroundColor();
            }

        } else {
            Logger.logErrorAndShowToast(mActivity, LOG_TAG, mActivity.getString(R.string.error_background_image_loading_failed));

            // Image files are unable to load so set background to solid color and notify update.
            updateBackgroundColor();
            notifyBackgroundUpdated(false);
        }
    }



    /**
     * Notify that the background is changed. New background can be image or solid color.
     *
     * @param isImage The {@code boolean} indicates that new background is image or not.
     */
    public void notifyBackgroundUpdated(boolean isImage) {
        mPreferences.setBackgroundImageEnabled(isImage);
        TermuxActivity.updateTermuxActivityStyling(mActivity, true);
    }

}
