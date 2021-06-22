package com.termux.shared.interact;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;

import androidx.core.content.ContextCompat;

import com.termux.shared.R;
import com.termux.shared.data.DataUtils;
import com.termux.shared.logger.Logger;

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
        context.startActivity(chooserIntent);
    }

    /**
     * Share text.
     *
     * @param context The context for operations.
     * @param subject The subject for sharing.
     * @param text The text to share.
     */
    public static void shareText(final Context context, final String subject, final String text) {
        if (context == null) return;

        final Intent shareTextIntent = new Intent(Intent.ACTION_SEND);
        shareTextIntent.setType("text/plain");
        shareTextIntent.putExtra(Intent.EXTRA_SUBJECT, subject);
        shareTextIntent.putExtra(Intent.EXTRA_TEXT, DataUtils.getTruncatedCommandOutput(text, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, false, false, false));

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
        if (context == null) return;

        final ClipboardManager clipboardManager = ContextCompat.getSystemService(context, ClipboardManager.class);

        if (clipboardManager != null) {
            clipboardManager.setPrimaryClip(ClipData.newPlainText(null, text));
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
        try {
            Uri uri = Uri.parse(url);
            Intent intent = new Intent(Intent.ACTION_VIEW, uri);
            context.startActivity(intent);
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to open the url \"" + url + "\"", e);
        }
    }

}
