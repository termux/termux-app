package com.termux.app.utils;

import android.content.Context;

import com.termux.R;
import com.termux.app.TermuxConstants;

import java.io.File;
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
        if(path != null && !path.isEmpty()) {
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
        if(path != null && !path.isEmpty()) {
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
    public static String getCanonicalPath(String path, String prefixForNonAbsolutePath, boolean expandPath) {
        if (path == null) path = "";

        if(expandPath)
            path = getExpandedTermuxPath(path);

        String absolutePath;

        // If path is already an absolute path
        if (path.startsWith("/") ) {
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
     * Determines whether path is in {@code dirPath}.
     *
     * @param path The {@code path} to check.
     * @param dirPath The {@code directory path} to check in.
     * @param ensureUnder If set to {@code true}, then it will be ensured that {@code path} is
     *                    under the directory and does not equal it.
     * @return Returns {@code true} if path in {@code dirPath}, otherwise returns {@code false}.
     */
    public static boolean isPathInDirPath(String path, String dirPath, boolean ensureUnder) {
        if (path == null || dirPath == null) return false;

        try {
            path = new File(path).getCanonicalPath();
        } catch(Exception e) {
            return false;
        }

        String normalizedDirPath = normalizePath(dirPath);

        if(ensureUnder)
            return !path.equals(normalizedDirPath) && path.startsWith(normalizedDirPath + "/");
        else
            return path.startsWith(normalizedDirPath + "/");
    }

    /**
     * Validate the existence and permissions of regular file at path.
     *
     * If the {@code parentDirPath} is not {@code null}, then setting of missing permissions will
     * only be done if {@code path} is under {@code parentDirPath}.
     *
     * @param context The {@link Context} to get error string.
     * @param path The {@code path} for file to validate.
     * @param parentDirPath The optional {@code parent directory path} to restrict operations to.
     *                      This can optionally be {@code null}.
     * @param permissionsToCheck The 3 character string that contains the "r", "w", "x" or "-" in-order.
     * @param setMissingPermissions The {@code boolean} that decides if missing permissions are to be
     *                              automatically set.
     * @param ignoreErrorsIfPathIsUnderParentDirPath The {@code boolean} that decides if permission
     *                                               errors are to be ignored if path is under
     *                                               {@code parentDirPath}.
     * @return Returns the {@code errmsg} if path is not a regular file, or validating permissions
     * failed, otherwise {@code null}.
     */
    public static String validateRegularFileExistenceAndPermissions(final Context context, final String path, final String parentDirPath, String permissionsToCheck, final boolean setMissingPermissions, final boolean ignoreErrorsIfPathIsUnderParentDirPath) {
        if (path == null || path.isEmpty()) return context.getString(R.string.null_or_empty_file);

        try {
            File file = new File(path);

            // If file exits but not a regular file
            if (file.exists() && !file.isFile()) {
                return context.getString(R.string.non_regular_file_found);
            }

            boolean isPathUnderParentDirPath = false;
            if (parentDirPath != null) {
                // The path can only be under parent directory path
                isPathUnderParentDirPath = isPathInDirPath(path, parentDirPath, true);
            }

            // If setMissingPermissions is enabled and path is a regular file
            if (setMissingPermissions && permissionsToCheck != null && file.isFile()) {
                // If there is not parentDirPath restriction or path is under parentDirPath
                if (parentDirPath == null || (isPathUnderParentDirPath && new File(parentDirPath).isDirectory())) {
                    setMissingFilePermissions(path, permissionsToCheck);
                }
            }

            // If path is not a regular file
            // Regular files cannot be automatically created so we do not ignore if missing
            if (!file.isFile()) {
                return context.getString(R.string.no_regular_file_found);
            }

            // If there is not parentDirPath restriction or path is not under parentDirPath or
            // if permission errors must not be ignored for paths under parentDirPath
            if (parentDirPath == null || !isPathUnderParentDirPath || !ignoreErrorsIfPathIsUnderParentDirPath) {
                if (permissionsToCheck != null) {
                    // Check if permissions are missing
                    return checkMissingFilePermissions(context, path, permissionsToCheck, "File", false);
                }
            }
        }
        // Some function calls may throw SecurityException, etc
        catch (Exception e) {
            return context.getString(R.string.validate_file_existence_and_permissions_failed_with_exception, path, e.getMessage());
        }

        return null;

    }

    /**
     * Validate the existence and permissions of directory at path.
     *
     * If the {@code parentDirPath} is not {@code null}, then creation of missing directory and
     * setting of missing permissions will only be done if {@code path} is under
     * {@code parentDirPath} or equals {@code parentDirPath}.
     *
     * @param context The {@link Context} to get error string.
     * @param path The {@code path} for file to validate.
     * @param parentDirPath The optional {@code parent directory path} to restrict operations to.
     *                      This can optionally be {@code null}.
     * @param permissionsToCheck The 3 character string that contains the "r", "w", "x" or "-" in-order.
     * @param createDirectoryIfMissing The {@code boolean} that decides if directory
     *                                 should be created if its missing.
     * @param setMissingPermissions The {@code boolean} that decides if missing permissions are to be
     *                              automatically set.
     * @param ignoreErrorsIfPathIsInParentDirPath The {@code boolean} that decides if existence
     *                                  and permission errors are to be ignored if path is
     *                                  in {@code parentDirPath}.
     * @param ignoreIfNotExecutable The {@code boolean} that decides if missing executable permission
     *                              error is to be ignored. This allows making an attempt to set
     *                              executable permissions, but ignoring if it fails.
     * @return Returns the {@code errmsg} if path is not a directory, or validating permissions
     * failed, otherwise {@code null}.
     */
    public static String validateDirectoryExistenceAndPermissions(final Context context, final String path, final String parentDirPath, String permissionsToCheck, final boolean createDirectoryIfMissing, final boolean setMissingPermissions, final boolean ignoreErrorsIfPathIsInParentDirPath, final boolean ignoreIfNotExecutable) {
        if (path == null || path.isEmpty()) return context.getString(R.string.null_or_empty_directory);

        try {
            File file = new File(path);

            // If file exits but not a directory file
            if (file.exists() && !file.isDirectory()) {
                return context.getString(R.string.non_directory_file_found);
            }

            boolean isPathInParentDirPath = false;
            if (parentDirPath != null) {
                // The path can be equal to parent directory path or under it
                isPathInParentDirPath = isPathInDirPath(path, parentDirPath, false);
            }

            if (createDirectoryIfMissing || setMissingPermissions) {
                // If there is not parentDirPath restriction or path is in parentDirPath
                if (parentDirPath == null || (isPathInParentDirPath && new File(parentDirPath).isDirectory())) {
                    // If createDirectoryIfMissing is enabled and no file exists at path, then create directory
                    if (createDirectoryIfMissing && !file.exists()) {
                        Logger.logVerbose(LOG_TAG, "Creating missing directory at path: \"" + path + "\"");
                        // If failed to create directory
                        if (!file.mkdirs()) {
                            return context.getString(R.string.creating_missing_directory_failed, path);
                        }
                    }

                    // If setMissingPermissions is enabled and path is a directory
                    if (setMissingPermissions && permissionsToCheck != null && file.isDirectory()) {
                        setMissingFilePermissions(path, permissionsToCheck);
                    }
                }
            }

            // If there is not parentDirPath restriction or path is not in parentDirPath or
            // if existence or permission errors must not be ignored for paths in parentDirPath
            if (parentDirPath == null || !isPathInParentDirPath || !ignoreErrorsIfPathIsInParentDirPath) {
                // If path is not a directory
                // Directories can be automatically created so we can ignore if missing with above check
                if (!file.isDirectory()) {
                    return context.getString(R.string.no_directory_found);
                }

                if (permissionsToCheck != null) {
                    // Check if permissions are missing
                    return checkMissingFilePermissions(context, path, permissionsToCheck, "Directory", ignoreIfNotExecutable);
                }
            }
        }
        // Some function calls may throw SecurityException, etc
        catch (Exception e) {
            return context.getString(R.string.validate_directory_existence_and_permissions_failed_with_exception, path, e.getMessage());
        }

        return null;
    }

    /**
     * Set missing permissions for file at path.
     *
     * @param path The {@code path} for file to set permissions to.
     * @param permissionsToSet The 3 character string that contains the "r", "w", "x" or "-" in-order.
     */
    public static void setMissingFilePermissions(String path, String permissionsToSet) {
        if (path == null || path.isEmpty()) return;

        if (!isValidPermissingString(permissionsToSet)) {
            Logger.logError(LOG_TAG, "Invalid permissionsToSet passed to setMissingFilePermissions: \"" + permissionsToSet + "\"");
            return;
        }

        File file = new File(path);

        if (permissionsToSet.contains("r") && !file.canRead()) {
            Logger.logVerbose(LOG_TAG, "Setting missing read permissions for file at path: \"" + path + "\"");
            file.setReadable(true);
        }

        if (permissionsToSet.contains("w") && !file.canWrite()) {
            Logger.logVerbose(LOG_TAG, "Setting missing write permissions for file at path: \"" + path + "\"");
            file.setWritable(true);
        }

        if (permissionsToSet.contains("x") && !file.canExecute()) {
            Logger.logVerbose(LOG_TAG, "Setting missing execute permissions for file at path: \"" + path + "\"");
            file.setExecutable(true);
        }
    }

    /**
     * Checking missing permissions for file at path.
     *
     * @param context The {@link Context} to get error string.
     * @param path The {@code path} for file to check permissions for.
     * @param permissionsToCheck The 3 character string that contains the "r", "w", "x" or "-" in-order.
     * @param fileType The label for the type of file to use for error string.
     * @param ignoreIfNotExecutable The {@code boolean} that decides if missing executable permission
     *                              error is to be ignored.
     * @return Returns the {@code errmsg} if validating permissions failed, otherwise {@code null}.
     */
    public static String checkMissingFilePermissions(Context context, String path, String permissionsToCheck, String fileType, boolean ignoreIfNotExecutable) {
        if (path == null || path.isEmpty()) return context.getString(R.string.null_or_empty_path);

        if (!isValidPermissingString(permissionsToCheck)) {
            Logger.logError(LOG_TAG, "Invalid permissionsToCheck passed to checkMissingFilePermissions: \"" + permissionsToCheck + "\"");
            return context.getString(R.string.invalid_file_permissions_string_to_check);
        }

        if (fileType == null || fileType.isEmpty()) fileType = "File";

        File file = new File(path);

        // If file is not readable
        if (permissionsToCheck.contains("r") && !file.canRead()) {
            return context.getString(R.string.file_not_readable, fileType);
        }

        // If file is not writable
        if (permissionsToCheck.contains("w") && !file.canWrite()) {
            return context.getString(R.string.file_not_writable, fileType);
        }
        // If file is not executable
        // This canExecute() will give "avc: granted { execute }" warnings for target sdk 29
        else if (permissionsToCheck.contains("x") && !file.canExecute() && !ignoreIfNotExecutable) {
            return context.getString(R.string.file_not_executable, fileType);
        }

        return null;
    }

    /**
     * Determines whether string exactly matches the 3 character permission string that
     * contains the "r", "w", "x" or "-" in-order.
     *
     * @param string The {@link String} to check.
     * @return Returns {@code true} if string exactly matches a permission string, otherwise {@code false}.
     */
    public static boolean isValidPermissingString(String string) {
        if (string == null || string.isEmpty()) return false;
        return Pattern.compile("^([r-])[w-][x-]$", 0).matcher(string).matches();
    }

}
