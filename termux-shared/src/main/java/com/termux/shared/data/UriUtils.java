package com.termux.shared.data;

import android.content.ContentResolver;
import android.net.Uri;

import androidx.annotation.NonNull;

public class UriUtils {

    /**
     * Get the full file path from a {@link Uri}.
     *
     *  If the {@link Uri} was created from file path with {@link Uri#parse(String)}, like "am"
     *  command "-d" option does, and the path contained a "#", then anything after it would become
     *  the fragment and {@link Uri#getPath()} will only return the path before it, which would be
     *  invalid. The fragment must be manually appended to the path to get the full path.
     *
     *  If the {@link Uri} was created with {@link Uri.Builder} and path was set
     *  with {@link Uri.Builder#path(String)}, then "#" will automatically be encoded to "%23"
     *  and separate fragment will not exist.
     *
     * @param uri The {@link Uri} to get basename from.
     * @return Returns the file path if found, otherwise {@code null}.
     */
    public static String getUriFilePath(Uri uri) {
        if (uri == null) return null;
        String path = uri.getPath();
        if (DataUtils.isNullOrEmpty(path)) return null;
        String fragment = uri.getFragment();
        return path + (DataUtils.isNullOrEmpty(fragment) ? "" : "#" + fragment);
    }

    /**
     * Get {@link ContentResolver#SCHEME_FILE} {@link Uri} for path.
     *
     * @param path The path for the {@link Uri}.
     * @return Returns the {@link Uri}.
     */
    public static Uri getFileUri(@NonNull String path) {
        return new Uri.Builder().scheme(ContentResolver.SCHEME_FILE).path(path).build();
    }

    /**
     * Get {@link ContentResolver#SCHEME_FILE} {@link Uri} for path.
     *
     * @param authority The authority for the {@link Uri}.
     * @param path The path for the {@link Uri}.
     * @return Returns the {@link Uri}.
     */
    public static Uri getFileUri(@NonNull String authority, @NonNull String path) {
        return new Uri.Builder().scheme(ContentResolver.SCHEME_FILE).authority(authority).path(path).build();
    }

    /**
     * Get {@link ContentResolver#SCHEME_CONTENT} {@link Uri} for path.
     *
     * @param path The path for the {@link Uri}.
     * @return Returns the {@link Uri}.
     */
    public static Uri getContentUri(@NonNull String path) {
        return new Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).path(path).build();
    }

    /**
     * Get {@link ContentResolver#SCHEME_CONTENT} {@link Uri} for path.
     *
     * @param authority The authority for the {@link Uri}.
     * @param path The path for the {@link Uri}.
     * @return Returns the {@link Uri}.
     */
    public static Uri getContentUri(@NonNull String authority, @NonNull String path) {
        return new Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(authority).path(path).build();
    }

}
