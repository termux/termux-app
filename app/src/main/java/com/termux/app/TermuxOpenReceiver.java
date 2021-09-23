package com.termux.app;

import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.provider.MediaStore;
import android.webkit.MimeTypeMap;

import com.termux.app.utils.PluginUtils;
import com.termux.shared.data.IntentUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

import androidx.annotation.NonNull;

public class TermuxOpenReceiver extends BroadcastReceiver {

    private static final String LOG_TAG = "TermuxOpenReceiver";

    @Override
    public void onReceive(Context context, Intent intent) {
        final Uri data = intent.getData();
        if (data == null) {
            Logger.logError(LOG_TAG, "termux-open: Called without intent data");
            return;
        }

        Logger.logVerbose(LOG_TAG, "Intent Received:\n" + IntentUtils.getIntentString(intent));

        final String filePath = data.getPath();
        final String contentTypeExtra = intent.getStringExtra("content-type");
        final boolean useChooser = intent.getBooleanExtra("chooser", false);
        final String intentAction = intent.getAction() == null ? Intent.ACTION_VIEW : intent.getAction();
        switch (intentAction) {
            case Intent.ACTION_SEND:
            case Intent.ACTION_VIEW:
                // Ok.
                break;
            default:
                Logger.logError(LOG_TAG, "Invalid action '" + intentAction + "', using 'view'");
                break;
        }

        final boolean isExternalUrl = data.getScheme() != null && !data.getScheme().equals("file");
        if (isExternalUrl) {
            Intent urlIntent = new Intent(intentAction, data);
            if (intentAction.equals(Intent.ACTION_SEND)) {
                urlIntent.putExtra(Intent.EXTRA_TEXT, data.toString());
                urlIntent.setData(null);
            } else if (contentTypeExtra != null) {
                urlIntent.setDataAndType(data, contentTypeExtra);
            }
            urlIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            try {
                context.startActivity(urlIntent);
            } catch (ActivityNotFoundException e) {
                Logger.logError(LOG_TAG, "termux-open: No app handles the url " + data);
            }
            return;
        }

        final File fileToShare = new File(filePath);
        if (!(fileToShare.isFile() && fileToShare.canRead())) {
            Logger.logError(LOG_TAG, "termux-open: Not a readable file: '" + fileToShare.getAbsolutePath() + "'");
            return;
        }

        Intent sendIntent = new Intent();
        sendIntent.setAction(intentAction);
        sendIntent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_GRANT_READ_URI_PERMISSION);

        String contentTypeToUse;
        if (contentTypeExtra == null) {
            String fileName = fileToShare.getName();
            int lastDotIndex = fileName.lastIndexOf('.');
            String fileExtension = fileName.substring(lastDotIndex + 1);
            MimeTypeMap mimeTypes = MimeTypeMap.getSingleton();
            // Lower casing makes it work with e.g. "JPG":
            contentTypeToUse = mimeTypes.getMimeTypeFromExtension(fileExtension.toLowerCase());
            if (contentTypeToUse == null) contentTypeToUse = "application/octet-stream";
        } else {
            contentTypeToUse = contentTypeExtra;
        }

        Uri uriToShare = Uri.parse("content://" + TermuxConstants.TERMUX_FILE_SHARE_URI_AUTHORITY + fileToShare.getAbsolutePath());

        if (Intent.ACTION_SEND.equals(intentAction)) {
            sendIntent.putExtra(Intent.EXTRA_STREAM, uriToShare);
            sendIntent.setType(contentTypeToUse);
        } else {
            sendIntent.setDataAndType(uriToShare, contentTypeToUse);
        }

        if (useChooser) {
            sendIntent = Intent.createChooser(sendIntent, null).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        }

        try {
            context.startActivity(sendIntent);
        } catch (ActivityNotFoundException e) {
            Logger.logError(LOG_TAG, "termux-open: No app handles the url " + data);
        }
    }

    public static class ContentProvider extends android.content.ContentProvider {

        private static final String LOG_TAG = "TermuxContentProvider";

        @Override
        public boolean onCreate() {
            return true;
        }

        @Override
        public Cursor query(@NonNull Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
            File file = new File(uri.getPath());

            if (projection == null) {
                projection = new String[]{
                    MediaStore.MediaColumns.DISPLAY_NAME,
                    MediaStore.MediaColumns.SIZE,
                    MediaStore.MediaColumns._ID
                };
            }

            Object[] row = new Object[projection.length];
            for (int i = 0; i < projection.length; i++) {
                String column = projection[i];
                Object value;
                switch (column) {
                    case MediaStore.MediaColumns.DISPLAY_NAME:
                        value = file.getName();
                        break;
                    case MediaStore.MediaColumns.SIZE:
                        value = (int) file.length();
                        break;
                    case MediaStore.MediaColumns._ID:
                        value = 1;
                        break;
                    default:
                        value = null;
                }
                row[i] = value;
            }

            MatrixCursor cursor = new MatrixCursor(projection);
            cursor.addRow(row);
            return cursor;
        }

        @Override
        public String getType(@NonNull Uri uri) {
            return null;
        }

        @Override
        public Uri insert(@NonNull Uri uri, ContentValues values) {
            return null;
        }

        @Override
        public int delete(@NonNull Uri uri, String selection, String[] selectionArgs) {
            return 0;
        }

        @Override
        public int update(@NonNull Uri uri, ContentValues values, String selection, String[] selectionArgs) {
            return 0;
        }

        @Override
        public ParcelFileDescriptor openFile(@NonNull Uri uri, @NonNull String mode) throws FileNotFoundException {
            File file = new File(uri.getPath());
            try {
                String path = file.getCanonicalPath();
                Logger.logDebug(LOG_TAG, "Open file request received for \"" + path + "\" with mode \"" + mode + "\"");
                String storagePath = Environment.getExternalStorageDirectory().getCanonicalPath();
                // See https://support.google.com/faqs/answer/7496913:
                if (!(path.startsWith(TermuxConstants.TERMUX_FILES_DIR_PATH) || path.startsWith(storagePath))) {
                    throw new IllegalArgumentException("Invalid path: " + path);
                }

                // If "allow-external-apps" property to not set to "true", then throw exception
                String errmsg = PluginUtils.checkIfAllowExternalAppsPolicyIsViolated(getContext(), LOG_TAG);
                if (errmsg != null) {
                    throw new IllegalArgumentException(errmsg);
                }

                // Do not allow apps with RUN_COMMAND permission to modify termux apps properties files,
                // including allow-external-apps
                if (TermuxConstants.TERMUX_PROPERTIES_PRIMARY_FILE_PATH.equals(path) ||
                    TermuxConstants.TERMUX_PROPERTIES_SECONDARY_FILE_PATH.equals(path) ||
                    TermuxConstants.TERMUX_FLOAT_PROPERTIES_PRIMARY_FILE_PATH.equals(path) ||
                    TermuxConstants.TERMUX_FLOAT_PROPERTIES_SECONDARY_FILE_PATH.equals(path)) {
                    mode = "r";
                }

            } catch (IOException e) {
                throw new IllegalArgumentException(e);
            }

            return ParcelFileDescriptor.open(file, ParcelFileDescriptor.parseMode(mode));
        }
    }

}
