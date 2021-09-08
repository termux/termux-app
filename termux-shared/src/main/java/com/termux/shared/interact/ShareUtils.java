package com.termux.shared.interact;

import android.Manifest;
import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Environment;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

import com.termux.shared.R;
import com.termux.shared.data.DataUtils;
import com.termux.shared.data.IntentUtils;
import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.models.errors.Error;
import com.termux.shared.packages.PermissionUtils;

import java.nio.charset.Charset;

public class ShareUtils {

    private static final String LOG_TAG = "ShareUtils";

    /**
     * Open the system app chooser that allows the user to select which app to send the intent.
     *
     * @param context The context for operations.
     * @param intent The intent that describes the choices that should be shown.
     * @param title The title for choose menu.
     */
    private static void openSystemAppChooser(final Context context, final Intent intent, final String title) {
        if (context == null) return;

        final Intent chooserIntent = new Intent(Intent.ACTION_CHOOSER);
        chooserIntent.putExtra(Intent.EXTRA_INTENT, intent);
        chooserIntent.putExtra(Intent.EXTRA_TITLE, title);
        chooserIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        try {
            context.startActivity(chooserIntent);
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to open system chooser for:\n" + IntentUtils.getIntentString(chooserIntent), e);
        }
    }

    /**
     * Share text.
     *
     * @param context The context for operations.
     * @param subject The subject for sharing.
     * @param text The text to share.
     */
    public static void shareText(final Context context, final String subject, final String text) {
        if (context == null || text == null) return;

        final Intent shareTextIntent = new Intent(Intent.ACTION_SEND);
        shareTextIntent.setType("text/plain");
        shareTextIntent.putExtra(Intent.EXTRA_SUBJECT, subject);
        shareTextIntent.putExtra(Intent.EXTRA_TEXT, DataUtils.getTruncatedCommandOutput(text, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, true, false, false));

        openSystemAppChooser(context, shareTextIntent, context.getString(R.string.title_share_with));
    }

    /**
     * Copy the text to clipboard.
     *
     * @param context The context for operations.
     * @param text The text to copy.
     * @param toastString If this is not {@code null} or empty, then a toast is shown if copying to
     *                    clipboard is successful.
     */
    public static void copyTextToClipboard(final Context context, final String text, final String toastString) {
        if (context == null || text == null) return;

        final ClipboardManager clipboardManager = ContextCompat.getSystemService(context, ClipboardManager.class);

        if (clipboardManager != null) {
            clipboardManager.setPrimaryClip(ClipData.newPlainText(null, DataUtils.getTruncatedCommandOutput(text, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, true, false, false)));
            if (toastString != null && !toastString.isEmpty())
                Logger.showToast(context, toastString, true);
        }
    }

    /**
     * Open a url.
     *
     * @param context The context for operations.
     * @param url The url to open.
     */
    public static void openURL(final Context context, final String url) {
        if (context == null || url == null || url.isEmpty()) return;
        Uri uri = Uri.parse(url);
        Intent intent = new Intent(Intent.ACTION_VIEW, uri);
        try {
            context.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            // If no activity found to handle intent, show system chooser
            openSystemAppChooser(context, intent, context.getString(R.string.title_open_url_with));
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to open url \"" + url + "\"", e);
        }
    }

    /**
     * Save a file at the path.
     *
     * If if path is under {@link Environment#getExternalStorageDirectory()}
     * or `/sdcard` and storage permission is missing, it will be requested if {@code context} is an
     * instance of {@link Activity} or {@link AppCompatActivity} and {@code storagePermissionRequestCode}
     * is `>=0` and the function will automatically return. The caller should call this function again
     * if user granted the permission.
     *
     * @param context The context for operations.
     * @param label The label for file.
     * @param filePath The path to save the file.
     * @param text The text to write to file.
     * @param showToast If set to {@code true}, then a toast is shown if saving to file is successful.
     * @param storagePermissionRequestCode The request code to use while asking for permission.
     */
    public static void saveTextToFile(final Context context, final String label, final String filePath, final String text, final boolean showToast, final int storagePermissionRequestCode) {
        if (context == null || filePath == null || filePath.isEmpty() || text == null) return;

        // If path is under primary external storage directory, then check for missing permissions.
        if ((FileUtils.isPathInDirPath(filePath, Environment.getExternalStorageDirectory().getAbsolutePath(), true) ||
            FileUtils.isPathInDirPath(filePath, "/sdcard", true)) &&
            !PermissionUtils.checkPermission(context, Manifest.permission.WRITE_EXTERNAL_STORAGE)) {
            Logger.logErrorAndShowToast(context, LOG_TAG, context.getString(R.string.msg_storage_permission_not_granted));

            if (storagePermissionRequestCode >= 0) {
                if (context instanceof AppCompatActivity)
                    PermissionUtils.requestPermission(((AppCompatActivity) context), Manifest.permission.WRITE_EXTERNAL_STORAGE, storagePermissionRequestCode);
                else if (context instanceof Activity)
                    PermissionUtils.requestPermission(((Activity) context), Manifest.permission.WRITE_EXTERNAL_STORAGE, storagePermissionRequestCode);
            }

            return;
        }

        Error error = FileUtils.writeStringToFile(label, filePath,
            Charset.defaultCharset(), text, false);
        if (error != null) {
            Logger.logErrorExtended(LOG_TAG, error.toString());
            Logger.showToast(context, Error.getMinimalErrorString(error), true);
        } else {
            if (showToast)
                Logger.showToast(context, context.getString(R.string.msg_file_saved_successfully, label, filePath), true);
        }
    }

}
