package com.termux.app.style;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.Point;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Looper;

import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.contract.ActivityResultContract;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AlertDialog;
import androidx.viewpager.widget.ViewPager;

import com.termux.R;
import com.termux.app.TermuxActivity;
import com.termux.shared.activity.ActivityUtils;
import com.termux.shared.data.DataUtils;
import com.termux.shared.errors.Error;
import com.termux.shared.file.FileUtils;
import com.termux.shared.image.ImageUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.termux.extrakeys.ExtraKeysView;
import com.termux.shared.termux.settings.preferences.TermuxAppSharedPreferences;
import com.termux.shared.theme.ThemeUtils;
import com.termux.shared.view.ViewUtils;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TextStyle;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class TermuxBackgroundManager {

    /** Require {@link TermuxActivity} to perform operations. */
    private final TermuxActivity mActivity;

    /** A launcher for start the process of executing an {@link ActivityResultContract}. */
    private final ActivityResultLauncher<String> mActivityResultLauncher;

    /** Termux app shared preferences manager. */
    private final TermuxAppSharedPreferences mPreferences;

    /** ExecutorService to execute task in background. */
    private final ExecutorService executor;

    /** Handler allows to send and process {@link  android.os.Message Message}. */
    private final Handler handler;


    private static final String LOG_TAG = "TermuxBackgroundManager";

    public TermuxBackgroundManager(TermuxActivity activity) {
        this.mActivity = activity;
        this.mPreferences = activity.getPreferences();
        this.mActivityResultLauncher = registerActivityResultLauncher();
        this.executor = Executors.newSingleThreadExecutor();
        this.handler = new Handler(Looper.getMainLooper());
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
                    executor.execute(() -> {
                        Bitmap bitmap = ImageUtils.getBitmap(mActivity, uri);

                        if (bitmap == null) {
                            Logger.logErrorAndShowToast(mActivity, LOG_TAG, mActivity.getString(R.string.error_background_image_loading_from_gallery_failed));
                            return;
                        }

                        ImageUtils.compressAndSaveBitmap(bitmap, TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH);
                        boolean success = generateImageFiles(mActivity, bitmap);

                        if (success) {
                            notifyBackgroundUpdated(true);

                            Logger.logInfo(LOG_TAG, "Image received successfully from the gallary.");
                            Logger.logDebug(LOG_TAG, "Storing background original image to " + TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH);
                            Logger.logDebug(LOG_TAG, "Storing background portrait image to " + TermuxConstants.TERMUX_BACKGROUND_IMAGE_PORTRAIT_PATH);
                            Logger.logDebug(LOG_TAG, "Storing background landscape image to " + TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH);

                        } else {
                            Logger.logErrorAndShowToast(mActivity, LOG_TAG, mActivity.getString(R.string.error_background_image_loading_from_gallery_failed));
                        }
                    });

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
    public static boolean isImageFilesExist(@NonNull Context context, boolean shouldGenerate) {
        boolean isLandscape = (ViewUtils.getDisplayOrientation(context) == Configuration.ORIENTATION_LANDSCAPE);
        Point size = ViewUtils.getDisplaySize(context, true);

        String imagePath1 = isLandscape ? TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH : TermuxConstants.TERMUX_BACKGROUND_IMAGE_PORTRAIT_PATH;

        String imagePath2 = isLandscape ? TermuxConstants.TERMUX_BACKGROUND_IMAGE_PORTRAIT_PATH : TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH;

        boolean exist = ImageUtils.isImageOptimized(imagePath1, size)
            && ImageUtils.isImageOptimized(imagePath2, DataUtils.swap(size));

        if (!exist && shouldGenerate && ImageUtils.isImage(TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH)) {
            Bitmap bitmap = ImageUtils.getBitmap(TermuxConstants.TERMUX_BACKGROUND_IMAGE_PATH);
            return generateImageFiles(context, bitmap);
        }

        return exist;
    }



    /**
     * Enable background image loading. If the image already exist then ask for restore otherwise pick from gallery.
     */
    public void setBackgroundImage() {
        if (!mPreferences.isBackgroundImageEnabled() && isImageFilesExist(mActivity, true)) {
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
            FileUtils.deleteDirectoryFile(null, TermuxConstants.TERMUX_BACKGROUND_DIR_PATH, true);
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
     * Generate background image files using original image. {@link Context}
     * passed to this method must be of an {@link Activity} to determine the size
     * of display.
     *
     * @param context The context require for the operations.
     * @param bitmap Image bitmap to save as background.
     * @return Returns whether the images generated successfully.
     */
    public static boolean generateImageFiles(@NonNull Context context, Bitmap bitmap) {

        if (bitmap == null || !(context instanceof Activity)) {
            return false;
        }

        Point size = ViewUtils.getDisplaySize(context, true);
        boolean isLandscape = ViewUtils.getDisplayOrientation(context) == Configuration.ORIENTATION_LANDSCAPE;
        Error error;

        if (isLandscape) {
            error = ImageUtils.saveForDisplayResolution(bitmap, size, TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH, TermuxConstants.TERMUX_BACKGROUND_IMAGE_PORTRAIT_PATH);

        } else {
            error = ImageUtils.saveForDisplayResolution(bitmap, DataUtils.swap(size), TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH, TermuxConstants.TERMUX_BACKGROUND_IMAGE_PORTRAIT_PATH);
        }

        return error == null;
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

        updateToolbarBackground();
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
        String imagePath = isLandscape ? TermuxConstants.TERMUX_BACKGROUND_IMAGE_LANDSCAPE_PATH : TermuxConstants.TERMUX_BACKGROUND_IMAGE_PORTRAIT_PATH;

        try {
            // Performing on main Thread may cause ANR and lag.
            executor.execute(() -> {
                if (isImageFilesExist(mActivity, true)) {
                    Drawable drawable = ImageUtils.getDrawable(imagePath);
                    ImageUtils.addOverlay(drawable, mActivity.getProperties().getBackgroundOverlayColor());

                    handler.post(() -> mActivity.getWindow().getDecorView().setBackground(drawable));
                } else {
                    Logger.logErrorAndShowToast(mActivity, LOG_TAG, mActivity.getString(R.string.error_background_image_loading_failed));

                    // Image files are unable to load so set background to solid color and notify update.
                    handler.post(this::updateBackgroundColor);
                    notifyBackgroundUpdated(false);
                }
            });

        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to load image", e);
            Logger.showToast(mActivity, mActivity.getString(R.string.error_background_image_loading_failed), true);

            // Since loading of image is failed, Set background to solid color.
            updateBackgroundColor();
            notifyBackgroundUpdated(false);
        }
    }

    /**
     * Set backgroudn of the ExtraKey toolbar and buttons.
     * Must be called when background preference is changed.
     */
    public void updateToolbarBackground() {
        ViewPager viewPager = mActivity.getTerminalToolbarViewPager();
        ExtraKeysView extraKeysView = mActivity.getExtraKeysView();

        if (viewPager == null || extraKeysView == null) {
            return;
        }

        if (mPreferences.isBackgroundImageEnabled()) {
            // Set overlay background to ToolbarViewPager and make button transparent.
            mActivity.getTerminalToolbarViewPager().setBackgroundColor(mActivity.getProperties().getBackgroundOverlayColor());
            extraKeysView.setButtonBackgroundColor(Color.TRANSPARENT);

        } else {
            // Use default background color of ToolbarViewPager and button.
            viewPager.setBackgroundColor(Color.BLACK);
            extraKeysView.setButtonBackgroundColor(ThemeUtils.getSystemAttrColor(mActivity, ExtraKeysView.ATTR_BUTTON_BACKGROUND_COLOR, ExtraKeysView.DEFAULT_BUTTON_BACKGROUND_COLOR));
        }
    }



    /**
     * Notify that the background is changed. New background can be image or solid color.
     *
     * @param isImage The {@code boolean} indicates that new background is image or not.
     */
    public void notifyBackgroundUpdated(boolean isImage) {
        mPreferences.setBackgroundImageEnabled(isImage);
        TermuxActivity.updateTermuxActivityStyling(mActivity, false);
    }

}
