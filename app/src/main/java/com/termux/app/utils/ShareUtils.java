package com.termux.app.utils;

import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.Intent;

import androidx.core.content.ContextCompat;

import com.termux.R;

public class ShareUtils {

    /**
     * Open the system app chooser that allows the user to select which app to send the intent.
     *
     * @param context The context for operations.
     * @param intent The intent that describes the choices that should be shown.
     * @param title The title for choose menu.
     */
    private static void openSystemAppChooser(final Context context, final Intent intent, final String title) {
        if(context == null) return;

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
        if(context == null) return;

        final Intent shareTextIntent = new Intent(Intent.ACTION_SEND);
        shareTextIntent.setType("text/plain");
        shareTextIntent.putExtra(Intent.EXTRA_SUBJECT, subject);
        shareTextIntent.putExtra(Intent.EXTRA_TEXT, text);

        openSystemAppChooser(context, shareTextIntent, context.getString(R.string.share_with));
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
        if(context == null) return;

        final ClipboardManager clipboardManager = ContextCompat.getSystemService(context, ClipboardManager.class);

        if (clipboardManager != null) {
            clipboardManager.setPrimaryClip(ClipData.newPlainText(null, text));
            if (toastString != null && !toastString.isEmpty())
                Logger.showToast(context, toastString, true);
        }
    }

}
