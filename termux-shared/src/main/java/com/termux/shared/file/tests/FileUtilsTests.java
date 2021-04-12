package com.termux.shared.file.tests;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;

import java.io.File;
import java.nio.charset.Charset;

public class FileUtilsTests {

    private static final String LOG_TAG = "FileUtilsTests";

    /**
     * Run basic tests for {@link FileUtils} class.
     *
     * Move tests need to be written, specially for failures.
     *
     * The log level must be set to verbose.
     *
     * Run at app startup like in an activity
     * FileUtilsTests.runTests(this, TermuxConstants.TERMUX_HOME_DIR_PATH + "/FileUtilsTests");
     *
     * @param context The {@link Context} for operations.
     */
    public static void runTests(@NonNull final Context context, @NonNull final String testRootDirectoryPath) {
        try {
            Logger.logInfo(LOG_TAG, "Running tests");
            Logger.logInfo(LOG_TAG, "testRootDirectoryPath: \"" + testRootDirectoryPath + "\"");

            String fileUtilsTestsDirectoryCanonicalPath = FileUtils.getCanonicalPath(testRootDirectoryPath, null, false);
            assertEqual("FileUtilsTests directory path is not a canonical path", testRootDirectoryPath, fileUtilsTestsDirectoryCanonicalPath);

            runTestsInner(context, testRootDirectoryPath);
            Logger.logInfo(LOG_TAG, "All tests successful");
        } catch (Exception e) {
            Logger.logErrorAndShowToast(context, LOG_TAG, e.getMessage());
        }
    }

    private static void runTestsInner(@NonNull final Context context, @NonNull final String testRootDirectoryPath) throws Exception {
        String errmsg;
        String label;
        String path;

        /*
         * - dir1
         *  - sub_dir1
         *  - sub_reg1
         *  - sub_sym1 (absolute symlink to dir2)
         *  - sub_sym2 (copy of sub_sym1 for symlink to dir2)
         *  - sub_sym3 (relative symlink to dir4)
         * - dir2
         *  - sub_reg1
         *  - sub_reg2 (copy of dir2/sub_reg1)
         * - dir3 (copy of dir1)
         * - dir4 (moved from dir3)
         */

        String dir1_label = "dir1";
        String dir1_path = testRootDirectoryPath + "/dir1";

        String dir1__sub_dir1_label = "dir1/sub_dir1";
        String dir1__sub_dir1_path = dir1_path + "/sub_dir1";

        String dir1__sub_reg1_label = "dir1/sub_reg1";
        String dir1__sub_reg1_path = dir1_path + "/sub_reg1";

        String dir1__sub_sym1_label = "dir1/sub_sym1";
        String dir1__sub_sym1_path = dir1_path + "/sub_sym1";

        String dir1__sub_sym2_label = "dir1/sub_sym2";
        String dir1__sub_sym2_path = dir1_path + "/sub_sym2";

        String dir1__sub_sym3_label = "dir1/sub_sym3";
        String dir1__sub_sym3_path = dir1_path + "/sub_sym3";


        String dir2_label = "dir2";
        String dir2_path = testRootDirectoryPath + "/dir2";

        String dir2__sub_reg1_label = "dir2/sub_reg1";
        String dir2__sub_reg1_path = dir2_path + "/sub_reg1";

        String dir2__sub_reg2_label = "dir2/sub_reg2";
        String dir2__sub_reg2_path = dir2_path + "/sub_reg2";


        String dir3_label = "dir3";
        String dir3_path = testRootDirectoryPath + "/dir3";

        String dir4_label = "dir4";
        String dir4_path = testRootDirectoryPath + "/dir4";





        // Create or clear test root directory file
        label = "testRootDirectoryPath";
        errmsg = FileUtils.clearDirectory(context, label, testRootDirectoryPath);
        assertEqual("Failed to create " + label + " directory file", null, errmsg);

        if (!FileUtils.directoryFileExists(testRootDirectoryPath, false))
            throwException("The " + label + " directory file does not exist as expected after creation");


        // Create dir1 directory file
        errmsg = FileUtils.createDirectoryFile(context, dir1_label, dir1_path);
        assertEqual("Failed to create " + dir1_label + " directory file", null, errmsg);

        // Create dir2 directory file
        errmsg = FileUtils.createDirectoryFile(context, dir2_label, dir2_path);
        assertEqual("Failed to create " + dir2_label + " directory file", null, errmsg);





        // Create dir1/sub_dir1 directory file
        label = dir1__sub_dir1_label; path = dir1__sub_dir1_path;
        errmsg = FileUtils.createDirectoryFile(context, label, path);
        assertEqual("Failed to create " + label + " directory file", null, errmsg);
        if (!FileUtils.directoryFileExists(path, false))
            throwException("The " + label + " directory file does not exist as expected after creation");

        // Create dir1/sub_reg1 regular file
        label = dir1__sub_reg1_label; path = dir1__sub_reg1_path;
        errmsg = FileUtils.createRegularFile(context, label, path);
        assertEqual("Failed to create " + label + " regular file", null, errmsg);
        if (!FileUtils.regularFileExists(path, false))
            throwException("The " + label + " regular file does not exist as expected after creation");

        // Create dir1/sub_sym1 -> dir2 absolute symlink file
        label = dir1__sub_sym1_label; path = dir1__sub_sym1_path;
        errmsg = FileUtils.createSymlinkFile(context, label, dir2_path, path);
        assertEqual("Failed to create " + label + " symlink file", null, errmsg);
        if (!FileUtils.symlinkFileExists(path))
            throwException("The " + label + " symlink file does not exist as expected after creation");

        // Copy dir1/sub_sym1 symlink file to dir1/sub_sym2
        label = dir1__sub_sym2_label; path = dir1__sub_sym2_path;
        errmsg = FileUtils.copySymlinkFile(context, label, dir1__sub_sym1_path, path, false);
        assertEqual("Failed to copy " + dir1__sub_sym1_label + " symlink file to " + label, null, errmsg);
        if (!FileUtils.symlinkFileExists(path))
            throwException("The " + label + " symlink file does not exist as expected after copying it from " + dir1__sub_sym1_label);
        if (!new File(path).getCanonicalPath().equals(dir2_path))
            throwException("The " + label + " symlink file does not point to " + dir2_label);





        // Write "line1" to dir2/sub_reg1 regular file
        label = dir2__sub_reg1_label; path = dir2__sub_reg1_path;
        errmsg = FileUtils.writeStringToFile(context, label, path, Charset.defaultCharset(), "line1", false);
        assertEqual("Failed to write string to " + label + " file with append mode false", null, errmsg);
        if (!FileUtils.regularFileExists(path, false))
            throwException("The " + label + " file does not exist as expected after writing to it with append mode false");

        // Write "line2" to dir2/sub_reg1 regular file
        errmsg = FileUtils.writeStringToFile(context, label, path, Charset.defaultCharset(), "\nline2", true);
        assertEqual("Failed to write string to " + label + " file with append mode true", null, errmsg);

        // Read dir2/sub_reg1 regular file
        StringBuilder dataStringBuilder = new StringBuilder();
        errmsg = FileUtils.readStringFromFile(context, label, path, Charset.defaultCharset(), dataStringBuilder, false);
        assertEqual("Failed to read from " + label + " file", null, errmsg);
        assertEqual("The data read from " + label + " file in not as expected", "line1\nline2", dataStringBuilder.toString());

        // Copy dir2/sub_reg1 regular file to dir2/sub_reg2 file
        label = dir2__sub_reg2_label; path = dir2__sub_reg2_path;
        errmsg = FileUtils.copyRegularFile(context, label, dir2__sub_reg1_path, path, false);
        assertEqual("Failed to copy " + dir2__sub_reg1_label + " regular file to " + label, null, errmsg);
        if (!FileUtils.regularFileExists(path, false))
            throwException("The " + label + " regular file does not exist as expected after copying it from " + dir2__sub_reg1_label);





        // Copy dir1 directory file to dir3
        label = dir3_label; path = dir3_path;
        errmsg = FileUtils.copyDirectoryFile(context, label, dir2_path, path, false);
        assertEqual("Failed to copy " + dir2_label + " directory file to " + label, null, errmsg);
        if (!FileUtils.directoryFileExists(path, false))
            throwException("The " + label + " directory file does not exist as expected after copying it from " + dir2_label);

        // Copy dir1 directory file to dir3 again to test overwrite
        label = dir3_label; path = dir3_path;
        errmsg = FileUtils.copyDirectoryFile(context, label, dir2_path, path, false);
        assertEqual("Failed to copy " + dir2_label + " directory file to " + label, null, errmsg);
        if (!FileUtils.directoryFileExists(path, false))
            throwException("The " + label + " directory file does not exist as expected after copying it from " + dir2_label);

        // Move dir3 directory file to dir4
        label = dir4_label; path = dir4_path;
        errmsg = FileUtils.moveDirectoryFile(context, label, dir3_path, path, false);
        assertEqual("Failed to move " + dir3_label + " directory file to " + label, null, errmsg);
        if (!FileUtils.directoryFileExists(path, false))
            throwException("The " + label + " directory file does not exist as expected after copying it from " + dir3_label);





        // Create dir1/sub_sym3 -> dir4 relative symlink file
        label = dir1__sub_sym3_label; path = dir1__sub_sym3_path;
        errmsg = FileUtils.createSymlinkFile(context, label, "../dir4", path);
        assertEqual("Failed to create " + label + " symlink file", null, errmsg);
        if (!FileUtils.symlinkFileExists(path))
            throwException("The " + label + " symlink file does not exist as expected after creation");

        // Create dir1/sub_sym3 -> dirX relative dangling symlink file
        // This is to ensure that symlinkFileExists returns true if a symlink file exists but is dangling
        label = dir1__sub_sym3_label; path = dir1__sub_sym3_path;
        errmsg = FileUtils.createSymlinkFile(context, label, "../dirX", path);
        assertEqual("Failed to create " + label + " symlink file", null, errmsg);
        if (!FileUtils.symlinkFileExists(path))
            throwException("The " + label + " dangling symlink file does not exist as expected after creation");





        // Delete dir1/sub_sym2 symlink file
        label = dir1__sub_sym2_label; path = dir1__sub_sym2_path;
        errmsg = FileUtils.deleteSymlinkFile(context, label, path, false);
        assertEqual("Failed to delete " + label + " symlink file", null, errmsg);
        if (FileUtils.fileExists(path, false))
            throwException("The " + label + " symlink file still exist after deletion");

        // Check if dir2 directory file still exists after deletion of dir1/sub_sym2 since it was a symlink to dir2
        // When deleting a symlink file, its target must not be deleted
        label = dir2_label; path = dir2_path;
        if (!FileUtils.directoryFileExists(path, false))
            throwException("The " + label + " directory file has unexpectedly been deleted after deletion of " + dir1__sub_sym2_label);





        // Delete dir1 directory file
        label = dir1_label; path = dir1_path;
        errmsg = FileUtils.deleteDirectoryFile(context, label, path, false);
        assertEqual("Failed to delete " + label + " directory file", null, errmsg);
        if (FileUtils.fileExists(path, false))
            throwException("The " + label + " directory file still exist after deletion");


        // Check if dir2 directory file and dir2/sub_reg1 regular file still exist after deletion of
        // dir1 since there was a dir1/sub_sym1 symlink to dir2 in it
        // When deleting a directory, any targets of symlinks must not be deleted when deleting symlink files
        label = dir2_label; path = dir2_path;
        if (!FileUtils.directoryFileExists(path, false))
            throwException("The " + label + " directory file has unexpectedly been deleted after deletion of " + dir1_label);
        label = dir2__sub_reg1_label; path = dir2__sub_reg1_path;
        if (!FileUtils.fileExists(path, false))
            throwException("The " + label + " regular file has unexpectedly been deleted after deletion of " + dir1_label);





        // Delete dir2/sub_reg1 regular file
        label = dir2__sub_reg1_label; path = dir2__sub_reg1_path;
        errmsg = FileUtils.deleteRegularFile(context, label, path, false);
        assertEqual("Failed to delete " + label + " regular file", null, errmsg);
        if (FileUtils.fileExists(path, false))
            throwException("The " + label + " regular file still exist after deletion");

        FileUtils.getFileType("/dev/ptmx", false);
        FileUtils.getFileType("/dev/null", false);
    }

    public static void assertEqual(@NonNull final String message, final String expected, final String actual) throws Exception {
        if (!equalsRegardingNull(expected, actual))
            throwException(message + "\nexpected: \"" + expected + "\"\nactual: \"" + actual + "\"");
    }

    private static boolean equalsRegardingNull(final String expected, final String actual) {
        if (expected == null) {
            return actual == null;
        }

        return isEquals(expected, actual);
    }

    private static boolean isEquals(String expected, String actual) {
        return expected.equals(actual);
    }

    public static void throwException(@NonNull final String message) throws Exception {
            throw new Exception(message);
    }

}
