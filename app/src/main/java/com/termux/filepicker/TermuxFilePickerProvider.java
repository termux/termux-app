package com.termux.filepicker;


import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.support.annotation.NonNull;
import android.webkit.MimeTypeMap;

import java.io.File;
import java.io.FileNotFoundException;

/** Provider of files content uris picked from {@link com.termux.filepicker.TermuxFilePickerActivity}. */
public class TermuxFilePickerProvider extends ContentProvider {
    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor query(@NonNull Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        return null;
    }

    @Override
    public String getType(@NonNull Uri uri) {
		String contentType = null;
		String path = uri.getPath();
		int lastDotIndex = path.lastIndexOf('.');
		String possibleFileExtension = path.substring(lastDotIndex + 1, path.length());
		if (possibleFileExtension.contains("/")) {
			// The dot was in the path, so not a file extension.
		} else {
			MimeTypeMap mimeTypes = MimeTypeMap.getSingleton();
			// Lower casing makes it work with e.g. "JPG":
			contentType = mimeTypes.getMimeTypeFromExtension(possibleFileExtension.toLowerCase());
		}

		if (contentType == null) contentType = "application/octet-stream";
		return contentType;
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
        return ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY);
    }
}
