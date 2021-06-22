package com.termux.shared.file;

import android.content.Context;
import android.os.Build;
import android.system.Os;

import androidx.annotation.NonNull;

import com.google.common.io.RecursiveDeleteOption;
import com.termux.shared.R;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.file.filesystem.FileType;
import com.termux.shared.file.filesystem.FileTypes;
import com.termux.shared.data.DataUtils;
import com.termux.shared.logger.Logger;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.Closeable;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.Charset;
import java.nio.file.LinkOption;
import java.nio.file.StandardCopyOption;
import java.util.regex.Pattern;

public class FileUtils {

    private static final String LOG_TAG = "FileUtils";

    /**
     * Replace "$PREFIX/" or "~/" prefix with termux absolute paths.
     *
     * @param path The {@code path} to expand.
     * @return Returns the {@code expand path}.
     */
    public static String getExpandedTermuxPath(String path) {
        if (path != null && !path.isEmpty()) {
            path = path.replaceAll("^\\$PREFIX$", TermuxConstants.TERMUX_PREFIX_DIR_PATH);
            path = path.replaceAll("^\\$PREFIX/", TermuxConstants.TERMUX_PREFIX_DIR_PATH + "/");
            path = path.replaceAll("^~/$", TermuxConstants.TERMUX_HOME_DIR_PATH);
            path = path.replaceAll("^~/", TermuxConstants.TERMUX_HOME_DIR_PATH + "/");
        }

        return path;
    }

    /**
     * Replace termux absolute paths with "$PREFIX/" or "~/" prefix.
     *
     * @param path The {@code path} to unexpand.
     * @return Returns the {@code unexpand path}.
     */
    public static String getUnExpandedTermuxPath(String path) {
        if (path != null && !path.isEmpty()) {
            path = path.replaceAll("^" + Pattern.quote(TermuxConstants.TERMUX_PREFIX_DIR_PATH) + "/", "\\$PREFIX/");
            path = path.replaceAll("^" + Pattern.quote(TermuxConstants.TERMUX_HOME_DIR_PATH) + "/", "~/");
        }

        return path;
    }

    /**
     * If {@code expandPath} is enabled, then input path is first attempted to be expanded by calling
     * {@link #getExpandedTermuxPath(String)}.
     *
     * Then if path is already an absolute path, then it is used as is to get canonical path.
     * If path is not an absolute path and {code prefixForNonAbsolutePath} is not {@code null}, then
     * {code prefixForNonAbsolutePath} + "/" is prefixed before path before getting canonical path.
     * If path is not an absolute path and {code prefixForNonAbsolutePath} is {@code null}, then
     * "/" is prefixed before path before getting canonical path.
     *
     * If an exception is raised to get the canonical path, then absolute path is returned.
     *
     * @param path The {@code path} to convert.
     * @param prefixForNonAbsolutePath Optional prefix path to prefix before non-absolute paths. This
     *                                 can be set to {@code null} if non-absolute paths should
     *                                 be prefixed with "/". The call to {@link File#getCanonicalPath()}
     *                                 will automatically do this anyways.
     * @return Returns the {@code canonical path}.
     */
    public static String getCanonicalPath(String path, final String prefixForNonAbsolutePath, final boolean expandPath) {
        if (path == null) path = "";

        if (expandPath)
            path = getExpandedTermuxPath(path);

        String absolutePath;

        // If path is already an absolute path
        if (path.startsWith("/")) {
            absolutePath = path;
        } else {
            if (prefixForNonAbsolutePath != null)
                absolutePath = prefixForNonAbsolutePath + "/" + path;
            else
                absolutePath = "/" + path;
        }

        try {
            return new File(absolutePath).getCanonicalPath();
        } catch(Exception e) {
        }

        return absolutePath;
    }

    /**
     * Removes one or more forward slashes "//" with single slash "/"
     * Removes "./"
     * Removes trailing forward slash "/"
     *
     * @param path The {@code path} to convert.
     * @return Returns the {@code normalized path}.
     */
    public static String normalizePath(String path) {
        if (path == null) return null;

        path = path.replaceAll("/+", "/");
        path = path.replaceAll("\\./", "");

        if (path.endsWith("/")) {
            path = path.substring(0, path.length() - 1);
        }

        return path;
    }

    /**
     * Determines whether path is in {@code dirPath}. The {@code dirPath} is not canonicalized and
     * only normalized.
     *
     * @param path The {@code path} to check.
     * @param dirPath The {@code directory path} to check in.
     * @param ensureUnder If set to {@code true}, then it will be ensured that {@code path} is
     *                    under the directory and does not equal it.
     * @return Returns {@code true} if path in {@code dirPath}, otherwise returns {@code false}.
     */
    public static boolean isPathInDirPath(String path, final String dirPath, final boolean ensureUnder) {
        if (path == null || dirPath == null) return false;

        try {
            path = new File(path).getCanonicalPath();
        } catch(Exception e) {
            return false;
        }

        String normalizedDirPath = normalizePath(dirPath);

        if (ensureUnder)
            return !path.equals(normalizedDirPath) && path.startsWith(normalizedDirPath + "/");
        else
            return path.startsWith(normalizedDirPath + "/");
    }



    /**
     * Checks whether a regular file exists at {@code filePath}.
     *
     * @param filePath The {@code path} for regular file to check.
     * @param followLinks The {@code boolean} that decides if symlinks will be followed while
     *                       finding if file exists. Check {@link #getFileType(String, boolean)}
     *                       for details.
     * @return Returns {@code true} if regular file exists, otherwise {@code false}.
     */
    public static boolean regularFileExists(final String filePath, final boolean followLinks) {
        return getFileType(filePath, followLinks) == FileType.REGULAR;
    }

    /**
     * Checks whether a directory file exists at {@code filePath}.
     *
     * @param filePath The {@code path} for directory file to check.
     * @param followLinks The {@code boolean} that decides if symlinks will be followed while
     *                       finding if file exists. Check {@link #getFileType(String, boolean)}
     *                       for details.
     * @return Returns {@code true} if directory file exists, otherwise {@code false}.
     */
    public static boolean directoryFileExists(final String filePath, final boolean followLinks) {
        return getFileType(filePath, followLinks) == FileType.DIRECTORY;
    }

    /**
     * Checks whether a symlink file exists at {@code filePath}.
     *
     * @param filePath The {@code path} for symlink file to check.
     * @return Returns {@code true} if symlink file exists, otherwise {@code false}.
     */
    public static boolean symlinkFileExists(final String filePath) {
        return getFileType(filePath, false) == FileType.SYMLINK;
    }

    /**
     * Checks whether any file exists at {@code filePath}.
     *
     * @param filePath The {@code path} for file to check.
     * @param followLinks The {@code boolean} that decides if symlinks will be followed while
     *                       finding if file exists. Check {@link #getFileType(String, boolean)}
     *                       for details.
     * @return Returns {@code true} if file exists, otherwise {@code false}.
     */
    public static boolean fileExists(final String filePath, final boolean followLinks) {
        return getFileType(filePath, followLinks) != FileType.NO_EXIST;
    }

    /**
     * Checks the type of file that exists at {@code filePath}.
     *
     * This function is a wrapper for
     * {@link FileTypes#getFileType(String, boolean)}
     * 
     * @param filePath The {@code path} for file to check.
     * @param followLinks The {@code boolean} that decides if symlinks will be followed while
     *                       finding type. If set to {@code true}, then type of symlink target will
     *                       be returned if file at {@code filePath} is a symlink. If set to
     *                       {@code false}, then type of file at {@code filePath} itself will be
     *                       returned.
     * @return Returns the {@link FileType} of file.
     */
    public static FileType getFileType(final String filePath, final boolean followLinks) {
        return FileTypes.getFileType(filePath, followLinks);
    }



    /**
     * Validate the existence and permissions of regular file at path.
     *
     * If the {@code parentDirPath} is not {@code null}, then setting of missing permissions will
     * only be done if {@code path} is under {@code parentDirPath}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for the regular file. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to validate. Symlinks will not be followed.
     * @param parentDirPath The optional {@code parent directory path} to restrict operations to.
     *                      This can optionally be {@code null}. It is not canonicalized and only normalized.
     * @param permissionsToCheck The 3 character string that contains the "r", "w", "x" or "-" in-order.
     * @param setPermissions The {@code boolean} that decides if permissions are to be
     *                              automatically set defined by {@code permissionsToCheck}.
     * @param setMissingPermissionsOnly The {@code boolean} that decides if only missing permissions
     *                                  are to be set or if they should be overridden.
     * @param ignoreErrorsIfPathIsUnderParentDirPath The {@code boolean} that decides if permission
     *                                               errors are to be ignored if path is under
     *                                               {@code parentDirPath}.
     * @return Returns the {@code errmsg} if path is not a regular file, or validating permissions
     * failed, otherwise {@code null}.
     */
    public static String validateRegularFileExistenceAndPermissions(@NonNull final Context context, String label, final String filePath, final String parentDirPath,
                                                                    final String permissionsToCheck, final boolean setPermissions, final boolean setMissingPermissionsOnly,
                                                                    final boolean ignoreErrorsIfPathIsUnderParentDirPath) {
        label = (label == null ? "" : label + " ");
        if (filePath == null || filePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "regular file path", "validateRegularFileExistenceAndPermissions");

        try {
            FileType fileType = getFileType(filePath, false);

            // If file exists but not a regular file
            if (fileType != FileType.NO_EXIST && fileType != FileType.REGULAR) {
                return context.getString(R.string.error_non_regular_file_found, label + "file");
            }

            boolean isPathUnderParentDirPath = false;
            if (parentDirPath != null) {
                // The path can only be under parent directory path
                isPathUnderParentDirPath = isPathInDirPath(filePath, parentDirPath, true);
            }

            // If setPermissions is enabled and path is a regular file
            if (setPermissions && permissionsToCheck != null && fileType == FileType.REGULAR) {
                // If there is not parentDirPath restriction or path is under parentDirPath
                if (parentDirPath == null || (isPathUnderParentDirPath && getFileType(parentDirPath, false) == FileType.DIRECTORY)) {
                    if (setMissingPermissionsOnly)
                        setMissingFilePermissions(label + "file", filePath, permissionsToCheck);
                    else
                        setFilePermissions(label + "file", filePath, permissionsToCheck);
                }
            }

            // If path is not a regular file
            // Regular files cannot be automatically created so we do not ignore if missing
            if (fileType != FileType.REGULAR) {
                return context.getString(R.string.error_no_regular_file_found, label + "file");
            }

            // If there is not parentDirPath restriction or path is not under parentDirPath or
            // if permission errors must not be ignored for paths under parentDirPath
            if (parentDirPath == null || !isPathUnderParentDirPath || !ignoreErrorsIfPathIsUnderParentDirPath) {
                if (permissionsToCheck != null) {
                    // Check if permissions are missing
                    return checkMissingFilePermissions(context, label + "regular", filePath, permissionsToCheck, false);
                }
            }
        } catch (Exception e) {
            return context.getString(R.string.error_validate_file_existence_and_permissions_failed_with_exception, label + "file", filePath, e.getMessage());
        }

        return null;

    }

    /**
     * Validate the existence and permissions of directory file at path.
     *
     * If the {@code parentDirPath} is not {@code null}, then creation of missing directory and
     * setting of missing permissions will only be done if {@code path} is under
     * {@code parentDirPath} or equals {@code parentDirPath}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for the directory file. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to validate or create. Symlinks will not be followed.
     * @param parentDirPath The optional {@code parent directory path} to restrict operations to.
     *                      This can optionally be {@code null}. It is not canonicalized and only normalized.
     * @param createDirectoryIfMissing The {@code boolean} that decides if directory file
     *                                 should be created if its missing.
     * @param permissionsToCheck The 3 character string that contains the "r", "w", "x" or "-" in-order.
     * @param setPermissions The {@code boolean} that decides if permissions are to be
     *                              automatically set defined by {@code permissionsToCheck}.
     * @param setMissingPermissionsOnly The {@code boolean} that decides if only missing permissions
     *                                  are to be set or if they should be overridden.
     * @param ignoreErrorsIfPathIsInParentDirPath The {@code boolean} that decides if existence
     *                                  and permission errors are to be ignored if path is
     *                                  in {@code parentDirPath}.
     * @param ignoreIfNotExecutable The {@code boolean} that decides if missing executable permission
     *                              error is to be ignored. This allows making an attempt to set
     *                              executable permissions, but ignoring if it fails.
     * @return Returns the {@code errmsg} if path is not a directory file, failed to create it,
     * or validating permissions failed, otherwise {@code null}.
     */
    public static String validateDirectoryFileExistenceAndPermissions(@NonNull final Context context, String label, final String filePath, final String parentDirPath, final boolean createDirectoryIfMissing,
                                                                      final String permissionsToCheck, final boolean setPermissions, final boolean setMissingPermissionsOnly,
                                                                      final boolean ignoreErrorsIfPathIsInParentDirPath, final boolean ignoreIfNotExecutable) {
        label = (label == null ? "" : label + " ");
        if (filePath == null || filePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "directory file path", "validateDirectoryExistenceAndPermissions");

        try {
            File file = new File(filePath);
            FileType fileType = getFileType(filePath, false);

            // If file exists but not a directory file
            if (fileType != FileType.NO_EXIST && fileType != FileType.DIRECTORY) {
                return context.getString(R.string.error_non_directory_file_found, label + "directory");
            }

            boolean isPathInParentDirPath = false;
            if (parentDirPath != null) {
                // The path can be equal to parent directory path or under it
                isPathInParentDirPath = isPathInDirPath(filePath, parentDirPath, false);
            }

            if (createDirectoryIfMissing || setPermissions) {
                // If there is not parentDirPath restriction or path is in parentDirPath
                if (parentDirPath == null || (isPathInParentDirPath && getFileType(parentDirPath, false) == FileType.DIRECTORY)) {
                    // If createDirectoryIfMissing is enabled and no file exists at path, then create directory
                    if (createDirectoryIfMissing && fileType == FileType.NO_EXIST) {
                        Logger.logVerbose(LOG_TAG, "Creating " + label + "directory file at path \"" + filePath + "\"");
                        // Create directory and update fileType if successful, otherwise return with error
                        if (file.mkdirs())
                            fileType = getFileType(filePath, false);
                        else
                            return context.getString(R.string.error_creating_file_failed, label + "directory file", filePath);
                    }

                    // If setPermissions is enabled and path is a directory
                    if (setPermissions && permissionsToCheck != null && fileType == FileType.DIRECTORY) {
                        if (setMissingPermissionsOnly)
                            setMissingFilePermissions(label + "directory", filePath, permissionsToCheck);
                        else
                            setFilePermissions(label + "directory", filePath, permissionsToCheck);
                    }
                }
            }

            // If there is not parentDirPath restriction or path is not in parentDirPath or
            // if existence or permission errors must not be ignored for paths in parentDirPath
            if (parentDirPath == null || !isPathInParentDirPath || !ignoreErrorsIfPathIsInParentDirPath) {
                // If path is not a directory
                // Directories can be automatically created so we can ignore if missing with above check
                if (fileType != FileType.DIRECTORY) {
                    return context.getString(R.string.error_file_not_found_at_path, label + "directory", filePath);
                }

                if (permissionsToCheck != null) {
                    // Check if permissions are missing
                    return checkMissingFilePermissions(context, label + "directory", filePath, permissionsToCheck, ignoreIfNotExecutable);
                }
            }
        } catch (Exception e) {
            return context.getString(R.string.error_validate_directory_existence_and_permissions_failed_with_exception, label + "directory file", filePath, e.getMessage());
        }

        return null;
    }



    /**
     * Create a regular file at path.
     *
     * This function is a wrapper for
     * {@link #validateDirectoryFileExistenceAndPermissions(Context, String, String, String, boolean, String, boolean, boolean, boolean, boolean)}.
     *
     * @param context The {@link Context} to get error string.
     * @param filePath The {@code path} for regular file to create.
     * @return Returns the {@code errmsg} if path is not a regular file or failed to create it,
     * otherwise {@code null}.
     */
    public static String createRegularFile(@NonNull final Context context, final String filePath) {
        return createRegularFile(context, null, filePath);
    }

    /**
     * Create a regular file at path.
     *
     * This function is a wrapper for
     * {@link #validateDirectoryFileExistenceAndPermissions(Context, String, String, String, boolean, String, boolean, boolean, boolean, boolean)}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for the regular file. This can optionally be {@code null}.
     * @param filePath The {@code path} for regular file to create.
     * @return Returns the {@code errmsg} if path is not a regular file or failed to create it,
     * otherwise {@code null}.
     */
    public static String createRegularFile(@NonNull final Context context, final String label, final String filePath) {
        return createRegularFile(context, label, filePath,
            null, false, false);
    }

    /**
     * Create a regular file at path.
     *
     * This function is a wrapper for
     * {@link #validateRegularFileExistenceAndPermissions(Context, String, String, String, String, boolean, boolean, boolean)}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for the regular file. This can optionally be {@code null}.
     * @param filePath The {@code path} for regular file to create.
     * @param permissionsToCheck The 3 character string that contains the "r", "w", "x" or "-" in-order.
     * @param setPermissions The {@code boolean} that decides if permissions are to be
     *                              automatically set defined by {@code permissionsToCheck}.
     * @param setMissingPermissionsOnly The {@code boolean} that decides if only missing permissions
     *                                  are to be set or if they should be overridden.
     * @return Returns the {@code errmsg} if path is not a regular file, failed to create it,
     * or validating permissions failed, otherwise {@code null}.
     */
    public static String createRegularFile(@NonNull final Context context, String label, final String filePath,
                                           final String permissionsToCheck, final boolean setPermissions, final boolean setMissingPermissionsOnly) {
        label = (label == null ? "" : label + " ");
        if (filePath == null || filePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "file path", "createRegularFile");

        String errmsg;

        File file = new File(filePath);
        FileType fileType = getFileType(filePath, false);

        // If file exists but not a regular file
        if (fileType != FileType.NO_EXIST && fileType != FileType.REGULAR) {
            return context.getString(R.string.error_non_regular_file_found, label + "file");
        }

        // If regular file already exists
        if (fileType == FileType.REGULAR) {
            return null;
        }

        // Create the file parent directory
        errmsg = createParentDirectoryFile(context, label + "regular file parent", filePath);
        if (errmsg != null)
            return errmsg;

        try {
            Logger.logVerbose(LOG_TAG, "Creating " + label + "regular file at path \"" + filePath + "\"");

            if (!file.createNewFile())
                return context.getString(R.string.error_creating_file_failed, label + "regular file", filePath);
        } catch (Exception e) {
            return context.getString(R.string.error_creating_file_failed_with_exception, label + "regular file", filePath, e.getMessage());
        }

        return validateRegularFileExistenceAndPermissions(context, label, filePath,
            null,
            permissionsToCheck, setPermissions, setMissingPermissionsOnly,
            false);
    }



    /**
     * Create parent directory of file at path.
     *
     * This function is a wrapper for
     * {@link #validateDirectoryFileExistenceAndPermissions(Context, String, String, String, boolean, String, boolean, boolean, boolean, boolean)}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for the parent directory file. This can optionally be {@code null}.
     * @param filePath The {@code path} for file whose parent needs to be created.
     * @return Returns the {@code errmsg} if parent path is not a directory file or failed to create it,
     * otherwise {@code null}.
     */
    public static String createParentDirectoryFile(@NonNull final Context context, final String label, final String filePath) {
        if (filePath == null || filePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "file path", "createParentDirectoryFile");

        File file = new File(filePath);
        String fileParentPath = file.getParent();

        if (fileParentPath != null)
            return createDirectoryFile(context, label, fileParentPath,
            null, false, false);
        else
            return null;
    }

    /**
     * Create a directory file at path.
     *
     * This function is a wrapper for
     * {@link #validateDirectoryFileExistenceAndPermissions(Context, String, String, String, boolean, String, boolean, boolean, boolean, boolean)}.
     *
     * @param context The {@link Context} to get error string.
     * @param filePath The {@code path} for directory file to create.
     * @return Returns the {@code errmsg} if path is not a directory file or failed to create it,
     * otherwise {@code null}.
     */
    public static String createDirectoryFile(@NonNull final Context context, final String filePath) {
        return createDirectoryFile(context, null, filePath);
    }

    /**
     * Create a directory file at path.
     *
     * This function is a wrapper for
     * {@link #validateDirectoryFileExistenceAndPermissions(Context, String, String, String, boolean, String, boolean, boolean, boolean, boolean)}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for the directory file. This can optionally be {@code null}.
     * @param filePath The {@code path} for directory file to create.
     * @return Returns the {@code errmsg} if path is not a directory file or failed to create it,
     * otherwise {@code null}.
     */
    public static String createDirectoryFile(@NonNull final Context context, final String label, final String filePath) {
        return createDirectoryFile(context, label, filePath,
            null, false, false);
    }

    /**
     * Create a directory file at path.
     * 
     * This function is a wrapper for
     * {@link #validateDirectoryFileExistenceAndPermissions(Context, String, String, String, boolean, String, boolean, boolean, boolean, boolean)}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for the directory file. This can optionally be {@code null}.
     * @param filePath The {@code path} for directory file to create.
     * @param permissionsToCheck The 3 character string that contains the "r", "w", "x" or "-" in-order.
     * @param setPermissions The {@code boolean} that decides if permissions are to be
     *                              automatically set defined by {@code permissionsToCheck}.
     * @param setMissingPermissionsOnly The {@code boolean} that decides if only missing permissions
     *                                  are to be set or if they should be overridden.
     * @return Returns the {@code errmsg} if path is not a directory file, failed to create it,
     * or validating permissions failed, otherwise {@code null}.
     */
    public static String createDirectoryFile(@NonNull final Context context, final String label, final String filePath,
                                             final String permissionsToCheck, final boolean setPermissions, final boolean setMissingPermissionsOnly) {
        return validateDirectoryFileExistenceAndPermissions(context, label, filePath,
            null, true,
            permissionsToCheck, setPermissions, setMissingPermissionsOnly,
            false, false);
    }



    /**
     * Create a symlink file at path.
     *
     * This function is a wrapper for
     * {@link #createSymlinkFile(Context, String, String, String, boolean, boolean, boolean)}.
     *
     * Dangling symlinks will be allowed.
     * Symlink destination will be overwritten if it already exists but only if its a symlink.
     *
     * @param context The {@link Context} to get error string.
     * @param targetFilePath The {@code path} TO which the symlink file will be created.
     * @param destFilePath The {@code path} AT which the symlink file will be created.
     * @return Returns the {@code errmsg} if path is not a symlink file, failed to create it,
     * otherwise {@code null}.
     */
    public static String createSymlinkFile(@NonNull final Context context, final String targetFilePath, final String destFilePath) {
        return createSymlinkFile(context, null, targetFilePath, destFilePath,
            true, true, true);
    }

    /**
     * Create a symlink file at path.
     *
     * This function is a wrapper for
     * {@link #createSymlinkFile(Context, String, String, String, boolean, boolean, boolean)}.
     *
     * Dangling symlinks will be allowed.
     * Symlink destination will be overwritten if it already exists but only if its a symlink.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for the symlink file. This can optionally be {@code null}.
     * @param targetFilePath The {@code path} TO which the symlink file will be created.
     * @param destFilePath The {@code path} AT which the symlink file will be created.
     * @return Returns the {@code errmsg} if path is not a symlink file, failed to create it,
     * otherwise {@code null}.
     */
    public static String createSymlinkFile(@NonNull final Context context, String label, final String targetFilePath, final String destFilePath) {
        return createSymlinkFile(context, label, targetFilePath, destFilePath,
            true, true, true);
    }

    /**
     * Create a symlink file at path.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for the symlink file. This can optionally be {@code null}.
     * @param targetFilePath The {@code path} TO which the symlink file will be created.
     * @param destFilePath The {@code path} AT which the symlink file will be created.
     * @param allowDangling The {@code boolean} that decides if it should be considered an
     *                              error if source file doesn't exist.
     * @param overwrite The {@code boolean} that decides if destination file should be overwritten if
     *                  it already exists. If set to {@code true}, then destination file will be
     *                  deleted before symlink is created.
     * @param overwriteOnlyIfDestIsASymlink The {@code boolean} that decides if overwrite should
     *                                         only be done if destination file is also a symlink.
     * @return Returns the {@code errmsg} if path is not a symlink file, failed to create it,
     * or validating permissions failed, otherwise {@code null}.
     */
    public static String createSymlinkFile(@NonNull final Context context, String label, final String targetFilePath, final String destFilePath,
                                           final boolean allowDangling, final boolean overwrite, final boolean overwriteOnlyIfDestIsASymlink) {
        label = (label == null ? "" : label + " ");
        if (targetFilePath == null || targetFilePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "target file path", "createSymlinkFile");
        if (destFilePath == null || destFilePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "destination file path", "createSymlinkFile");

        String errmsg;

        try {
            File destFile = new File(destFilePath);

            String targetFileAbsolutePath = targetFilePath;
            // If target path is relative instead of absolute
            if (!targetFilePath.startsWith("/")) {
                String destFileParentPath = destFile.getParent();
                if (destFileParentPath != null)
                    targetFileAbsolutePath = destFileParentPath + "/" +  targetFilePath;
            }

            FileType targetFileType = getFileType(targetFileAbsolutePath, false);
            FileType destFileType = getFileType(destFilePath, false);

            // If target file does not exist
            if (targetFileType == FileType.NO_EXIST) {
                // If dangling symlink should not be allowed, then return with error
                if (!allowDangling)
                    return context.getString(R.string.error_file_not_found_at_path, label + "symlink target file", targetFileAbsolutePath);
            }

            // If destination exists
            if (destFileType != FileType.NO_EXIST) {
                // If destination must not be overwritten
                if (!overwrite) {
                    return null;
                }

                // If overwriteOnlyIfDestIsASymlink is enabled but destination file is not a symlink
                if (overwriteOnlyIfDestIsASymlink && destFileType != FileType.SYMLINK)
                    return context.getString(R.string.error_cannot_overwrite_a_non_symlink_file_type, label + " file", destFilePath, targetFilePath, destFileType.getName());

                // Delete the destination file
                errmsg = deleteFile(context, label + "symlink destination", destFilePath, true);
                if (errmsg != null)
                    return errmsg;
            } else {
                // Create the destination file parent directory
                errmsg = createParentDirectoryFile(context, label + "symlink destination file parent", destFilePath);
                if (errmsg != null)
                    return errmsg;
            }

            // create a symlink at destFilePath to targetFilePath
            Logger.logVerbose(LOG_TAG, "Creating " + label + "symlink file at path \"" + destFilePath + "\" to \"" + targetFilePath + "\"");
            Os.symlink(targetFilePath, destFilePath);
        } catch (Exception e) {
            return context.getString(R.string.error_creating_symlink_file_failed_with_exception, label + "symlink file", destFilePath, targetFilePath, e.getMessage());
        }

        return null;
    }



    /**
     * Copy a regular file from {@code sourceFilePath} to {@code destFilePath}.
     *
     * This function is a wrapper for
     * {@link #copyOrMoveFile(Context, String, String, String, boolean, boolean, int, boolean, boolean)}.
     *
     * If destination file already exists, then it will be overwritten, but only if its a regular
     * file, otherwise an error will be returned.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to copy. This can optionally be {@code null}.
     * @param srcFilePath The {@code source path} for file to copy.
     * @param destFilePath The {@code destination path} for file to copy.
     * @param ignoreNonExistentSrcFile The {@code boolean} that decides if it should be considered an
     *                              error if source file to copied doesn't exist.
     * @return Returns the {@code errmsg} if copy was not successful, otherwise {@code null}.
     */
    public static String copyRegularFile(@NonNull final Context context, final String label, final String srcFilePath, final String destFilePath, final boolean ignoreNonExistentSrcFile) {
        return copyOrMoveFile(context, label, srcFilePath, destFilePath,
            false, ignoreNonExistentSrcFile, FileType.REGULAR.getValue(),
            true, true);
    }

    /**
     * Move a regular file from {@code sourceFilePath} to {@code destFilePath}.
     *
     * This function is a wrapper for
     * {@link #copyOrMoveFile(Context, String, String, String, boolean, boolean, int, boolean, boolean)}.
     *
     * If destination file already exists, then it will be overwritten, but only if its a regular
     * file, otherwise an error will be returned.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to move. This can optionally be {@code null}.
     * @param srcFilePath The {@code source path} for file to move.
     * @param destFilePath The {@code destination path} for file to move.
     * @param ignoreNonExistentSrcFile The {@code boolean} that decides if it should be considered an
     *                              error if source file to moved doesn't exist.
     * @return Returns the {@code errmsg} if move was not successful, otherwise {@code null}.
     */
    public static String moveRegularFile(@NonNull final Context context, final String label, final String srcFilePath, final String destFilePath, final boolean ignoreNonExistentSrcFile) {
        return copyOrMoveFile(context, label, srcFilePath, destFilePath,
            true, ignoreNonExistentSrcFile, FileType.REGULAR.getValue(),
            true, true);
    }

    /**
     * Copy a directory file from {@code sourceFilePath} to {@code destFilePath}.
     *
     * This function is a wrapper for
     * {@link #copyOrMoveFile(Context, String, String, String, boolean, boolean, int, boolean, boolean)}.
     *
     * If destination file already exists, then it will be overwritten, but only if its a directory
     * file, otherwise an error will be returned.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to copy. This can optionally be {@code null}.
     * @param srcFilePath The {@code source path} for file to copy.
     * @param destFilePath The {@code destination path} for file to copy.
     * @param ignoreNonExistentSrcFile The {@code boolean} that decides if it should be considered an
     *                              error if source file to copied doesn't exist.
     * @return Returns the {@code errmsg} if copy was not successful, otherwise {@code null}.
     */
    public static String copyDirectoryFile(@NonNull final Context context, final String label, final String srcFilePath, final String destFilePath, final boolean ignoreNonExistentSrcFile) {
        return copyOrMoveFile(context, label, srcFilePath, destFilePath,
            false, ignoreNonExistentSrcFile, FileType.DIRECTORY.getValue(),
            true, true);
    }

    /**
     * Move a directory file from {@code sourceFilePath} to {@code destFilePath}.
     *
     * This function is a wrapper for
     * {@link #copyOrMoveFile(Context, String, String, String, boolean, boolean, int, boolean, boolean)}.
     *
     * If destination file already exists, then it will be overwritten, but only if its a directory
     * file, otherwise an error will be returned.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to move. This can optionally be {@code null}.
     * @param srcFilePath The {@code source path} for file to move.
     * @param destFilePath The {@code destination path} for file to move.
     * @param ignoreNonExistentSrcFile The {@code boolean} that decides if it should be considered an
     *                              error if source file to moved doesn't exist.
     * @return Returns the {@code errmsg} if move was not successful, otherwise {@code null}.
     */
    public static String moveDirectoryFile(@NonNull final Context context, final String label, final String srcFilePath, final String destFilePath, final boolean ignoreNonExistentSrcFile) {
        return copyOrMoveFile(context, label, srcFilePath, destFilePath,
        true, ignoreNonExistentSrcFile, FileType.DIRECTORY.getValue(),
            true, true);
    }

    /**
     * Copy a symlink file from {@code sourceFilePath} to {@code destFilePath}.
     *
     * This function is a wrapper for
     * {@link #copyOrMoveFile(Context, String, String, String, boolean, boolean, int, boolean, boolean)}.
     *
     * If destination file already exists, then it will be overwritten, but only if its a symlink
     * file, otherwise an error will be returned.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to copy. This can optionally be {@code null}.
     * @param srcFilePath The {@code source path} for file to copy.
     * @param destFilePath The {@code destination path} for file to copy.
     * @param ignoreNonExistentSrcFile The {@code boolean} that decides if it should be considered an
     *                              error if source file to copied doesn't exist.
     * @return Returns the {@code errmsg} if copy was not successful, otherwise {@code null}.
     */
    public static String copySymlinkFile(@NonNull final Context context, final String label, final String srcFilePath, final String destFilePath, final boolean ignoreNonExistentSrcFile) {
        return copyOrMoveFile(context, label, srcFilePath, destFilePath,
            false, ignoreNonExistentSrcFile, FileType.SYMLINK.getValue(),
            true, true);
    }

    /**
     * Move a symlink file from {@code sourceFilePath} to {@code destFilePath}.
     *
     * This function is a wrapper for
     * {@link #copyOrMoveFile(Context, String, String, String, boolean, boolean, int, boolean, boolean)}.
     *
     * If destination file already exists, then it will be overwritten, but only if its a symlink
     * file, otherwise an error will be returned.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to move. This can optionally be {@code null}.
     * @param srcFilePath The {@code source path} for file to move.
     * @param destFilePath The {@code destination path} for file to move.
     * @param ignoreNonExistentSrcFile The {@code boolean} that decides if it should be considered an
     *                              error if source file to moved doesn't exist.
     * @return Returns the {@code errmsg} if move was not successful, otherwise {@code null}.
     */
    public static String moveSymlinkFile(@NonNull final Context context, final String label, final String srcFilePath, final String destFilePath, final boolean ignoreNonExistentSrcFile) {
        return copyOrMoveFile(context, label, srcFilePath, destFilePath,
            true, ignoreNonExistentSrcFile, FileType.SYMLINK.getValue(),
            true, true);
    }

    /**
     * Copy a file from {@code sourceFilePath} to {@code destFilePath}.
     *
     * This function is a wrapper for
     * {@link #copyOrMoveFile(Context, String, String, String, boolean, boolean, int, boolean, boolean)}.
     *
     * If destination file already exists, then it will be overwritten, but only if its the same file
     * type as the source, otherwise an error will be returned.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to copy. This can optionally be {@code null}.
     * @param srcFilePath The {@code source path} for file to copy.
     * @param destFilePath The {@code destination path} for file to copy.
     * @param ignoreNonExistentSrcFile The {@code boolean} that decides if it should be considered an
     *                              error if source file to copied doesn't exist.
     * @return Returns the {@code errmsg} if copy was not successful, otherwise {@code null}.
     */
    public static String copyFile(@NonNull final Context context, final String label, final String srcFilePath, final String destFilePath, final boolean ignoreNonExistentSrcFile) {
        return copyOrMoveFile(context, label, srcFilePath, destFilePath,
            false, ignoreNonExistentSrcFile, FileTypes.FILE_TYPE_NORMAL_FLAGS,
            true, true);
    }

    /**
     * Move a file from {@code sourceFilePath} to {@code destFilePath}.
     *
     * This function is a wrapper for
     * {@link #copyOrMoveFile(Context, String, String, String, boolean, boolean, int, boolean, boolean)}.
     *
     * If destination file already exists, then it will be overwritten, but only if its the same file
     * type as the source, otherwise an error will be returned.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to move. This can optionally be {@code null}.
     * @param srcFilePath The {@code source path} for file to move.
     * @param destFilePath The {@code destination path} for file to move.
     * @param ignoreNonExistentSrcFile The {@code boolean} that decides if it should be considered an
     *                              error if source file to moved doesn't exist.
     * @return Returns the {@code errmsg} if move was not successful, otherwise {@code null}.
     */
    public static String moveFile(@NonNull final Context context, final String label, final String srcFilePath, final String destFilePath, final boolean ignoreNonExistentSrcFile) {
        return copyOrMoveFile(context, label, srcFilePath, destFilePath,
            true, ignoreNonExistentSrcFile, FileTypes.FILE_TYPE_NORMAL_FLAGS,
            true, true);
    }
    
    /**
     * Copy or move a file from {@code sourceFilePath} to {@code destFilePath}.
     *
     * The {@code sourceFilePath} and {@code destFilePath} must be the canonical path to the source
     * and destination since symlinks will not be followed.
     *
     * If the {@code sourceFilePath} or {@code destFilePath} is a canonical path to a directory,
     * then any symlink files found under the directory will be deleted, but not their targets when
     * deleting source after move and deleting destination before copy/move.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to copy or move. This can optionally be {@code null}.
     * @param srcFilePath The {@code source path} for file to copy or move.
     * @param destFilePath The {@code destination path} for file to copy or move.
     * @param moveFile The {@code boolean} that decides if source file needs to be copied or moved.
     *                 If set to {@code true}, then source file will be moved, otherwise it will be
     *                 copied.
     * @param ignoreNonExistentSrcFile The {@code boolean} that decides if it should be considered an
     *                              error if source file to copied or moved doesn't exist.
     * @param allowedFileTypeFlags The flags that are matched against the source file's {@link FileType}
     *                             to see if it should be copied/moved or not. This is a safety measure
     *                             to prevent accidental copy/move/delete of the wrong type of file,
     *                             like a directory instead of a regular file. You can pass
     *                             {@link FileTypes#FILE_TYPE_ANY_FLAGS} to allow copy/move of any file type.
     * @param overwrite The {@code boolean} that decides if destination file should be overwritten if
     *                  it already exists. If set to {@code true}, then destination file will be
     *                  deleted before source is copied or moved.
     * @param overwriteOnlyIfDestSameFileTypeAsSrc The {@code boolean} that decides if overwrite should
     *                                         only be done if destination file is also the same file
     *                                          type as the source file.
     * @return Returns the {@code errmsg} if copy or move was not successful, otherwise {@code null}.
     */
    public static String copyOrMoveFile(@NonNull final Context context, String label, final String srcFilePath, final String destFilePath,
                                        final boolean moveFile, final boolean ignoreNonExistentSrcFile, int allowedFileTypeFlags,
                                        final boolean overwrite, final boolean overwriteOnlyIfDestSameFileTypeAsSrc) {
        label = (label == null ? "" : label + " ");
        if (srcFilePath == null || srcFilePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "source file path", "copyOrMoveFile");
        if (destFilePath == null || destFilePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "destination file path", "copyOrMoveFile");

        String mode = (moveFile ? "Moving" : "Copying");
        String modePast = (moveFile ? "moved" : "copied");

        String errmsg;

        InputStream inputStream = null;
        OutputStream outputStream = null;
        
        try {
            Logger.logVerbose(LOG_TAG, mode + " " + label + "source file from \"" + srcFilePath + "\" to destination \"" + destFilePath + "\"");

            File srcFile = new File(srcFilePath);
            File destFile = new File(destFilePath);

            FileType srcFileType = getFileType(srcFilePath, false);
            FileType destFileType = getFileType(destFilePath, false);

            String srcFileCanonicalPath = srcFile.getCanonicalPath();
            String destFileCanonicalPath = destFile.getCanonicalPath();

            // If source file does not exist
            if (srcFileType == FileType.NO_EXIST) {
                // If copy or move is to be ignored if source file is not found
                if (ignoreNonExistentSrcFile)
                    return null;
                // Else return with error
                else
                    return context.getString(R.string.error_file_not_found_at_path, label + "source file", srcFilePath);
            }

            // If the file type of the source file does not exist in the allowedFileTypeFlags, then return with error
            if ((allowedFileTypeFlags & srcFileType.getValue()) <= 0)
                return context.getString(R.string.error_file_not_an_allowed_file_type, label + "source file meant to be " + modePast, srcFilePath, FileTypes.convertFileTypeFlagsToNamesString(allowedFileTypeFlags));

            // If source and destination file path are the same
            if (srcFileCanonicalPath.equals(destFileCanonicalPath))
                return context.getString(R.string.error_copying_or_moving_file_to_same_path, mode + " " + label + "source file", srcFilePath, destFilePath);

            // If destination exists
            if (destFileType != FileType.NO_EXIST) {
                // If destination must not be overwritten
                if (!overwrite) {
                    return null;
                }

                // If overwriteOnlyIfDestSameFileTypeAsSrc is enabled but destination file does not match source file type
                if (overwriteOnlyIfDestSameFileTypeAsSrc && destFileType != srcFileType)
                    return context.getString(R.string.error_cannot_overwrite_a_different_file_type, label + "source file", mode.toLowerCase(), srcFilePath, destFilePath, destFileType.getName(), srcFileType.getName());

                // Delete the destination file
                errmsg = deleteFile(context, label + "destination file", destFilePath, true);
                if (errmsg != null)
                    return errmsg;
            }


            // Copy or move source file to dest
            boolean copyFile = !moveFile;

            // If moveFile is true
            if (moveFile) {
                // We first try to rename source file to destination file to save a copy operation in case both source and destination are on the same filesystem
                Logger.logVerbose(LOG_TAG, "Attempting to rename source to destination.");

                // https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:libcore/ojluni/src/main/java/java/io/UnixFileSystem.java;l=358
                // https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:libcore/luni/src/main/java/android/system/Os.java;l=512
                // Uses File.getPath() to get the path of source and destination and not the canonical path
                if (!srcFile.renameTo(destFile)) {
                    // If destination directory is a subdirectory of the source directory
                    // Copying is still allowed by copyDirectory() by excluding destination directory files
                    if (srcFileType == FileType.DIRECTORY && destFileCanonicalPath.startsWith(srcFileCanonicalPath + File.separator))
                        return context.getString(R.string.error_cannot_move_directory_to_sub_directory_of_itself, label + "source directory", srcFilePath, destFilePath);

                    // If rename failed, then we copy
                    Logger.logVerbose(LOG_TAG, "Renaming " + label + "source file to destination failed, attempting to copy.");
                    copyFile = true;
                }
            }

            // If moveFile is false or renameTo failed while moving
            if (copyFile) {
                Logger.logVerbose(LOG_TAG, "Attempting to copy source to destination.");

                // Create the dest file parent directory
                errmsg = createParentDirectoryFile(context, label + "dest file parent", destFilePath);
                if (errmsg != null)
                    return errmsg;

                if (srcFileType == FileType.DIRECTORY) {
                    // Will give runtime exceptions on android < 8 due to missing classes like java.nio.file.Path if org.apache.commons.io version > 2.5
                    org.apache.commons.io.FileUtils.copyDirectory(srcFile, destFile, true);
                } else if (srcFileType == FileType.SYMLINK) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                        java.nio.file.Files.copy(srcFile.toPath(), destFile.toPath(), LinkOption.NOFOLLOW_LINKS, StandardCopyOption.REPLACE_EXISTING);
                    } else {
                        // read the target for the source file and create a symlink at dest
                        // source file metadata will be lost
                        errmsg = createSymlinkFile(context, label + "dest file", Os.readlink(srcFilePath), destFilePath);
                        if (errmsg != null)
                            return errmsg;
                    }
                } else {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                        java.nio.file.Files.copy(srcFile.toPath(), destFile.toPath(), LinkOption.NOFOLLOW_LINKS, StandardCopyOption.REPLACE_EXISTING);
                    } else {
                        // Will give runtime exceptions on android < 8 due to missing classes like java.nio.file.Path if org.apache.commons.io version > 2.5
                        org.apache.commons.io.FileUtils.copyFile(srcFile, destFile, true);
                    }
                }
            }

            // If source file had to be moved
            if (moveFile) {
                // Delete the source file since copying would have succeeded
                errmsg = deleteFile(context, label + "source file", srcFilePath, true);
                if (errmsg != null)
                    return errmsg;
            }

            Logger.logVerbose(LOG_TAG, mode + " successful.");
        }
        catch (Exception e) {
            return context.getString(R.string.error_copying_or_moving_file_failed_with_exception, mode + " " + label + "file", srcFilePath, destFilePath, e.getMessage());
        } finally {
            closeCloseable(inputStream);
            closeCloseable(outputStream);
        }

        return null;
    }



    /**
     * Delete regular file at path.
     *
     * This function is a wrapper for {@link #deleteFile(Context, String, String, boolean, int)}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to delete. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to delete.
     * @param ignoreNonExistentFile The {@code boolean} that decides if it should be considered an
     *                              error if file to deleted doesn't exist.
     * @return Returns the {@code errmsg} if deletion was not successful, otherwise {@code null}.
     */
    public static String deleteRegularFile(@NonNull final Context context, String label, final String filePath, final boolean ignoreNonExistentFile) {
        return deleteFile(context, label, filePath, ignoreNonExistentFile, FileType.REGULAR.getValue());
    }

    /**
     * Delete directory file at path.
     *
     * This function is a wrapper for {@link #deleteFile(Context, String, String, boolean, int)}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to delete. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to delete.
     * @param ignoreNonExistentFile The {@code boolean} that decides if it should be considered an
     *                              error if file to deleted doesn't exist.
     * @return Returns the {@code errmsg} if deletion was not successful, otherwise {@code null}.
     */
    public static String deleteDirectoryFile(@NonNull final Context context, String label, final String filePath, final boolean ignoreNonExistentFile) {
        return deleteFile(context, label, filePath, ignoreNonExistentFile, FileType.DIRECTORY.getValue());
    }

    /**
     * Delete symlink file at path.
     *
     * This function is a wrapper for {@link #deleteFile(Context, String, String, boolean, int)}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to delete. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to delete.
     * @param ignoreNonExistentFile The {@code boolean} that decides if it should be considered an
     *                              error if file to deleted doesn't exist.
     * @return Returns the {@code errmsg} if deletion was not successful, otherwise {@code null}.
     */
    public static String deleteSymlinkFile(@NonNull final Context context, String label, final String filePath, final boolean ignoreNonExistentFile) {
        return deleteFile(context, label, filePath, ignoreNonExistentFile, FileType.SYMLINK.getValue());
    }

    /**
     * Delete regular, directory or symlink file at path.
     *
     * This function is a wrapper for {@link #deleteFile(Context, String, String, boolean, int)}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to delete. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to delete.
     * @param ignoreNonExistentFile The {@code boolean} that decides if it should be considered an
     *                              error if file to deleted doesn't exist.
     * @return Returns the {@code errmsg} if deletion was not successful, otherwise {@code null}.
     */
    public static String deleteFile(@NonNull final Context context, String label, final String filePath, final boolean ignoreNonExistentFile) {
        return deleteFile(context, label, filePath, ignoreNonExistentFile, FileTypes.FILE_TYPE_NORMAL_FLAGS);
    }

    /**
     * Delete file at path.
     *
     * The {@code filePath} must be the canonical path to the file to be deleted since symlinks will
     * not be followed.
     * If the {@code filePath} is a canonical path to a directory, then any symlink files found under
     * the directory will be deleted, but not their targets.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to delete. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to delete.
     * @param ignoreNonExistentFile The {@code boolean} that decides if it should be considered an
     *                              error if file to deleted doesn't exist.
     * @param allowedFileTypeFlags The flags that are matched against the file's {@link FileType} to
     *                             see if it should be deleted or not. This is a safety measure to
     *                             prevent accidental deletion of the wrong type of file, like a
     *                             directory instead of a regular file. You can pass
     *                             {@link FileTypes#FILE_TYPE_ANY_FLAGS} to allow deletion of any file type.
     * @return Returns the {@code errmsg} if deletion was not successful, otherwise {@code null}.
     */
    public static String deleteFile(@NonNull final Context context, String label, final String filePath, final boolean ignoreNonExistentFile, int allowedFileTypeFlags) {
        label = (label == null ? "" : label + " ");
        if (filePath == null || filePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "file path", "deleteFile");
        
        try {
            Logger.logVerbose(LOG_TAG, "Deleting " + label + "file at path \"" + filePath + "\"");

            File file = new File(filePath);
            FileType fileType = getFileType(filePath, false);

            // If file does not exist
            if (fileType == FileType.NO_EXIST) {
                // If delete is to be ignored if file does not exist
                if (ignoreNonExistentFile)
                    return null;
                // Else return with error
                else
                    return context.getString(R.string.error_file_not_found_at_path, label + "file meant to be deleted", filePath);
            }

            // If the file type of the file does not exist in the allowedFileTypeFlags, then return with error
            if ((allowedFileTypeFlags & fileType.getValue()) <= 0)
                return context.getString(R.string.error_file_not_an_allowed_file_type, label + "file meant to be deleted", filePath, FileTypes.convertFileTypeFlagsToNamesString(allowedFileTypeFlags));

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    /* Try to use {@link SecureDirectoryStream} if available for safer directory
                    deletion, it should be available for android >= 8.0
                    * https://guava.dev/releases/24.1-jre/api/docs/com/google/common/io/MoreFiles.html#deleteRecursively-java.nio.file.Path-com.google.common.io.RecursiveDeleteOption...-
                    * https://github.com/google/guava/issues/365
                    * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:libcore/ojluni/src/main/java/sun/nio/fs/UnixSecureDirectoryStream.java
                    *
                    * MoreUtils is marked with the @Beta annotation so the API may be removed in
                    * future but has been there for a few years now
                    */
                //noinspection UnstableApiUsage
                com.google.common.io.MoreFiles.deleteRecursively(file.toPath(), RecursiveDeleteOption.ALLOW_INSECURE);
            } else {
                if (fileType == FileType.DIRECTORY) {
                    // deleteDirectory() instead of forceDelete() gets the files list first instead of walking directory tree, so seems safer
                    // Will give runtime exceptions on android < 8 due to missing classes like java.nio.file.Path if org.apache.commons.io version > 2.5
                    org.apache.commons.io.FileUtils.deleteDirectory(file);
                } else {
                    // Will give runtime exceptions on android < 8 due to missing classes like java.nio.file.Path if org.apache.commons.io version > 2.5
                    org.apache.commons.io.FileUtils.forceDelete(file);
                }
            }

            // If file still exists after deleting it
            fileType = getFileType(filePath, false);
            if (fileType != FileType.NO_EXIST)
                return context.getString(R.string.error_file_still_exists_after_deleting, label + "file meant to be deleted", filePath);
        }
        catch (Exception e) {
            return context.getString(R.string.error_deleting_file_failed_with_exception, label + "file", filePath, e.getMessage());
        }

        return null;
    }



    /**
     * Clear contents of directory at path without deleting the directory. If directory does not exist
     * it will be created automatically.
     *
     * This function is a wrapper for
     * {@link #clearDirectory(Context, String, String)}.
     *
     * @param context The {@link Context} to get error string.
     * @param filePath The {@code path} for directory to clear.
     * @return Returns the {@code errmsg} if clearing was not successful, otherwise {@code null}.
     */
    public static String clearDirectory(Context context, String filePath) {
        return clearDirectory(context, null, filePath);
    }

    /**
     * Clear contents of directory at path without deleting the directory. If directory does not exist
     * it will be created automatically.
     *
     * The {@code filePath} must be the canonical path to a directory since symlinks will not be followed.
     * Any symlink files found under the directory will be deleted, but not their targets.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for directory to clear. This can optionally be {@code null}.
     * @param filePath The {@code path} for directory to clear.
     * @return Returns the {@code errmsg} if clearing was not successful, otherwise {@code null}.
     */
    public static String clearDirectory(@NonNull final Context context, String label, final String filePath) {
        label = (label == null ? "" : label + " ");
        if (filePath == null || filePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "file path", "clearDirectory");

        String errmsg;

        try {
            Logger.logVerbose(LOG_TAG, "Clearing " + label + "directory at path \"" + filePath + "\"");

            File file = new File(filePath);
            FileType fileType = getFileType(filePath, false);

            // If file exists but not a directory file
            if (fileType != FileType.NO_EXIST && fileType != FileType.DIRECTORY) {
                return context.getString(R.string.error_non_directory_file_found, label + "directory");
            }

            // If directory exists, clear its contents
            if (fileType == FileType.DIRECTORY) {
                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    //noinspection UnstableApiUsage
                    com.google.common.io.MoreFiles.deleteDirectoryContents(file.toPath(), RecursiveDeleteOption.ALLOW_INSECURE);
                } else {
                    // Will give runtime exceptions on android < 8 due to missing classes like java.nio.file.Path if org.apache.commons.io version > 2.5
                    org.apache.commons.io.FileUtils.cleanDirectory(new File(filePath));
                }
            }
            // Else create it
            else {
                errmsg = createDirectoryFile(context, label, filePath);
                if (errmsg != null)
                    return errmsg;
            }
        } catch (Exception e) {
            return context.getString(R.string.error_clearing_directory_failed_with_exception, label + "directory", filePath, e.getMessage());
        }

        return null;
    }



    /**
     * Read a {@link String} from file at path with a specific {@link Charset} into {@code dataString}.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to read. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to read.
     * @param charset The {@link Charset} of the file. If this is {@code null},
     *      *                then default {@link Charset} will be used.
     * @param dataStringBuilder The {@code StringBuilder} to read data into.
     * @param ignoreNonExistentFile The {@code boolean} that decides if it should be considered an
     *                              error if file to read doesn't exist.
     * @return Returns the {@code errmsg} if reading was not successful, otherwise {@code null}.
     */
    public static String readStringFromFile(@NonNull final Context context, String label, final String filePath, Charset charset, @NonNull final StringBuilder dataStringBuilder, final boolean ignoreNonExistentFile) {
        label = (label == null ? "" : label + " ");
        if (filePath == null || filePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "file path", "readStringFromFile");

        Logger.logVerbose(LOG_TAG, "Reading string from " + label + "file at path \"" + filePath + "\"");

        String errmsg;

        FileType fileType = getFileType(filePath, false);

        // If file exists but not a regular file
        if (fileType != FileType.NO_EXIST && fileType != FileType.REGULAR) {
            return context.getString(R.string.error_non_regular_file_found, label + "file");
        }

        // If file does not exist
        if (fileType == FileType.NO_EXIST) {
            // If reading is to be ignored if file does not exist
            if (ignoreNonExistentFile)
                return null;
            // Else return with error
            else
                return context.getString(R.string.error_file_not_found_at_path, label + "file meant to be read", filePath);
        }

        if (charset == null) charset = Charset.defaultCharset();

        // Check if charset is supported
        errmsg = isCharsetSupported(context, charset);
        if (errmsg != null)
            return errmsg;

        FileInputStream fileInputStream = null;
        BufferedReader bufferedReader = null;
        try {
            // Read string from file
            fileInputStream = new FileInputStream(filePath);
            bufferedReader = new BufferedReader(new InputStreamReader(fileInputStream, charset));

            String receiveString;

            boolean firstLine = true;
            while ((receiveString = bufferedReader.readLine()) != null ) {
                if (!firstLine) dataStringBuilder.append("\n"); else firstLine = false;
                dataStringBuilder.append(receiveString);
            }

            Logger.logVerbose(LOG_TAG, Logger.getMultiLineLogStringEntry("String", DataUtils.getTruncatedCommandOutput(dataStringBuilder.toString(), Logger.LOGGER_ENTRY_SIZE_LIMIT_IN_BYTES, true, false, true), "-"));
        } catch (Exception e) {
            return context.getString(R.string.error_reading_string_to_file_failed_with_exception, label + "file", filePath, e.getMessage());
        } finally {
            closeCloseable(fileInputStream);
            closeCloseable(bufferedReader);
        }

        return null;
    }

    /**
     * Write the {@link String} {@code dataString} with a specific {@link Charset} to file at path.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for file to write. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to write.
     * @param charset The {@link Charset} of the {@code dataString}. If this is {@code null},
     *                then default {@link Charset} will be used.
     * @param append The {@code boolean} that decides if file should be appended to or not.
     * @return Returns the {@code errmsg} if writing was not successful, otherwise {@code null}.
     */
    public static String writeStringToFile(@NonNull final Context context, String label, final String filePath, Charset charset, final String dataString, final boolean append) {
        label = (label == null ? "" : label + " ");
        if (filePath == null || filePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "file path", "writeStringToFile");

        Logger.logVerbose(LOG_TAG, Logger.getMultiLineLogStringEntry("Writing string to " + label + "file at path \"" + filePath + "\"", DataUtils.getTruncatedCommandOutput(dataString, Logger.LOGGER_ENTRY_SIZE_LIMIT_IN_BYTES, true, false, true), "-"));

        String errmsg;

        FileType fileType = getFileType(filePath, false);

        // If file exists but not a regular file
        if (fileType != FileType.NO_EXIST && fileType != FileType.REGULAR) {
            return context.getString(R.string.error_non_regular_file_found, label + "file");
        }

        // Create the file parent directory
        errmsg = createParentDirectoryFile(context, label + "file parent", filePath);
        if (errmsg != null)
            return errmsg;

        if (charset == null) charset = Charset.defaultCharset();

        // Check if charset is supported
        errmsg = isCharsetSupported(context, charset);
        if (errmsg != null)
            return errmsg;

        FileOutputStream fileOutputStream = null;
        BufferedWriter bufferedWriter = null;
        try {
            // Write string to file
            fileOutputStream = new FileOutputStream(filePath, append);
            bufferedWriter = new BufferedWriter(new OutputStreamWriter(fileOutputStream, charset));

            bufferedWriter.write(dataString);
            bufferedWriter.flush();
        } catch (Exception e) {
            return context.getString(R.string.error_writing_string_to_file_failed_with_exception, label + "file", filePath, e.getMessage());
        } finally {
            closeCloseable(fileOutputStream);
            closeCloseable(bufferedWriter);
        }

        return null;
    }



    /**
     * Check if a specific {@link Charset} is supported.
     *
     * @param context The {@link Context} to get error string.
     * @param charset The {@link Charset} to check.
     * @return Returns the {@code errmsg} if charset is not supported or failed to check it, otherwise {@code null}.
     */
    public static String isCharsetSupported(@NonNull final Context context, final Charset charset) {
        if (charset == null) return context.getString(R.string.error_null_or_empty_parameter, "charset", "isCharsetSupported");

        try {
            if (!Charset.isSupported(charset.name())) {
                return context.getString(R.string.error_unsupported_charset, charset.name());
            }
        } catch (Exception e) {
            return context.getString(R.string.error_checking_if_charset_supported_failed, charset.name(), e.getMessage());
        }

        return null;
    }



    /**
     * Close a {@link Closeable} object if not {@code null} and ignore any exceptions raised.
     *
     * @param closeable The {@link Closeable} object to close.
     */
    public static void closeCloseable(final Closeable closeable) {
        if (closeable != null) {
            try {
                closeable.close();
            }
            catch (IOException e) {
                // ignore
            }
        }
    }



    /**
     * Set permissions for file at path. Existing permission outside the {@code permissionsToSet}
     * will be removed.
     *
     * @param filePath The {@code path} for file to set permissions to.
     * @param permissionsToSet The 3 character string that contains the "r", "w", "x" or "-" in-order.
     */
    public static void setFilePermissions(final String filePath, final String permissionsToSet) {
        setFilePermissions(null, filePath, permissionsToSet);
    }

    /**
     * Set permissions for file at path. Existing permission outside the {@code permissionsToSet}
     * will be removed.
     *
     * @param label The optional label for the file. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to set permissions to.
     * @param permissionsToSet The 3 character string that contains the "r", "w", "x" or "-" in-order.
     */
    public static void setFilePermissions(String label, final String filePath, final String permissionsToSet) {
        label = (label == null ? "" : label + " ");
        if (filePath == null || filePath.isEmpty()) return;

        if (!isValidPermissionString(permissionsToSet)) {
            Logger.logError(LOG_TAG, "Invalid permissionsToSet passed to setFilePermissions: \"" + permissionsToSet + "\"");
            return;
        }

        File file = new File(filePath);

        if (permissionsToSet.contains("r")) {
            if (!file.canRead()) {
                Logger.logVerbose(LOG_TAG, "Setting read permissions for " + label + "file at path \"" + filePath + "\"");
                file.setReadable(true);
            }
        } else {
            if (file.canRead()) {
                Logger.logVerbose(LOG_TAG, "Removing read permissions for " + label + "file at path \"" + filePath + "\"");
                file.setReadable(false);
            }
        }


        if (permissionsToSet.contains("w")) {
            if (!file.canWrite()) {
                Logger.logVerbose(LOG_TAG, "Setting write permissions for " + label + "file at path \"" + filePath + "\"");
                file.setWritable(true);
            }
        } else {
            if (file.canWrite()) {
                Logger.logVerbose(LOG_TAG, "Removing write permissions for " + label + "file at path \"" + filePath + "\"");
                file.setWritable(false);
            }
        }


        if (permissionsToSet.contains("x")) {
            if (!file.canExecute()) {
                Logger.logVerbose(LOG_TAG, "Setting execute permissions for " + label + "file at path \"" + filePath + "\"");
                file.setExecutable(true);
            }
        } else {
            if (file.canExecute()) {
                Logger.logVerbose(LOG_TAG, "Removing execute permissions for " + label + "file at path \"" + filePath + "\"");
                file.setExecutable(false);
            }
        }
    }



    /**
     * Set missing permissions for file at path. Existing permission outside the {@code permissionsToSet}
     * will not be removed.
     *
     * @param filePath The {@code path} for file to set permissions to.
     * @param permissionsToSet The 3 character string that contains the "r", "w", "x" or "-" in-order.
     */
    public static void setMissingFilePermissions(final String filePath, final String permissionsToSet) {
        setMissingFilePermissions(null, filePath, permissionsToSet);
    }

    /**
     * Set missing permissions for file at path. Existing permission outside the {@code permissionsToSet}
     * will not be removed.
     *
     * @param label The optional label for the file. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to set permissions to.
     * @param permissionsToSet The 3 character string that contains the "r", "w", "x" or "-" in-order.
     */
    public static void setMissingFilePermissions(String label, final String filePath, final String permissionsToSet) {
        label = (label == null ? "" : label + " ");
        if (filePath == null || filePath.isEmpty()) return;

        if (!isValidPermissionString(permissionsToSet)) {
            Logger.logError(LOG_TAG, "Invalid permissionsToSet passed to setMissingFilePermissions: \"" + permissionsToSet + "\"");
            return;
        }

        File file = new File(filePath);

        if (permissionsToSet.contains("r") && !file.canRead()) {
            Logger.logVerbose(LOG_TAG, "Setting missing read permissions for " + label + "file at path \"" + filePath + "\"");
            file.setReadable(true);
        }

        if (permissionsToSet.contains("w") && !file.canWrite()) {
            Logger.logVerbose(LOG_TAG, "Setting missing write permissions for " + label + "file at path \"" + filePath + "\"");
            file.setWritable(true);
        }

        if (permissionsToSet.contains("x") && !file.canExecute()) {
            Logger.logVerbose(LOG_TAG, "Setting missing execute permissions for " + label + "file at path \"" + filePath + "\"");
            file.setExecutable(true);
        }
    }



    /**
     * Checking missing permissions for file at path.
     *
     * @param context The {@link Context} to get error string.
     * @param filePath The {@code path} for file to check permissions for.
     * @param permissionsToCheck The 3 character string that contains the "r", "w", "x" or "-" in-order.
     * @param ignoreIfNotExecutable The {@code boolean} that decides if missing executable permission
     *                              error is to be ignored.
     * @return Returns the {@code errmsg} if validating permissions failed, otherwise {@code null}.
     */
    public static String checkMissingFilePermissions(@NonNull final Context context, final String filePath, final String permissionsToCheck, final boolean ignoreIfNotExecutable) {
        return checkMissingFilePermissions(context, null, filePath, permissionsToCheck, ignoreIfNotExecutable);
    }

    /**
     * Checking missing permissions for file at path.
     *
     * @param context The {@link Context} to get error string.
     * @param label The optional label for the file. This can optionally be {@code null}.
     * @param filePath The {@code path} for file to check permissions for.
     * @param permissionsToCheck The 3 character string that contains the "r", "w", "x" or "-" in-order.
     * @param ignoreIfNotExecutable The {@code boolean} that decides if missing executable permission
     *                              error is to be ignored.
     * @return Returns the {@code errmsg} if validating permissions failed, otherwise {@code null}.
     */
    public static String checkMissingFilePermissions(@NonNull final Context context, String label, final String filePath, final String permissionsToCheck, final boolean ignoreIfNotExecutable) {
        label = (label == null ? "" : label + " ");
        if (filePath == null || filePath.isEmpty()) return context.getString(R.string.error_null_or_empty_parameter, label + "file path", "checkMissingFilePermissions");

        if (!isValidPermissionString(permissionsToCheck)) {
            Logger.logError(LOG_TAG, "Invalid permissionsToCheck passed to checkMissingFilePermissions: \"" + permissionsToCheck + "\"");
            return context.getString(R.string.error_invalid_file_permissions_string_to_check);
        }

        File file = new File(filePath);

        // If file is not readable
        if (permissionsToCheck.contains("r") && !file.canRead()) {
            return context.getString(R.string.error_file_not_readable, label + "file");
        }

        // If file is not writable
        if (permissionsToCheck.contains("w") && !file.canWrite()) {
            return context.getString(R.string.error_file_not_writable, label + "file");
        }
        // If file is not executable
        // This canExecute() will give "avc: granted { execute }" warnings for target sdk 29
        else if (permissionsToCheck.contains("x") && !file.canExecute() && !ignoreIfNotExecutable) {
            return context.getString(R.string.error_file_not_executable, label + "file");
        }

        return null;
    }



    /**
     * Checks whether string exactly matches the 3 character permission string that
     * contains the "r", "w", "x" or "-" in-order.
     *
     * @param string The {@link String} to check.
     * @return Returns {@code true} if string exactly matches a permission string, otherwise {@code false}.
     */
    public static boolean isValidPermissionString(final String string) {
        if (string == null || string.isEmpty()) return false;
        return Pattern.compile("^([r-])[w-][x-]$", 0).matcher(string).matches();
    }
    
}
