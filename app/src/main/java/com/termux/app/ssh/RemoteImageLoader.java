package com.termux.app.ssh;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Base64;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.runner.app.AppShell;
import com.termux.shared.termux.shell.command.environment.TermuxShellEnvironment;

import java.io.File;
import java.nio.charset.StandardCharsets;

/**
 * Service class for loading remote image files via SSH ControlMaster.
 *
 * Executes base64 command on remote server to retrieve image data,
 * decodes it to Bitmap with dimension pre-checking to prevent OOM.
 *
 * Design rationale:
 * - Uses base64 encoding for binary-safe transfer
 * - Pre-checks dimensions using BitmapFactory.Options.inJustDecodeBounds
 * - Downsamples large images (>4096x4096) to prevent memory exhaustion
 * - ImageLoadResult encapsulates success/failure with detailed info
 */
public class RemoteImageLoader {

    private static final String LOG_TAG = "RemoteImageLoader";

    /** SSH binary path (the wrapper that uses ControlMaster) */
    private static final String SSH_BINARY = "/data/data/com.termux/files/usr/bin/ssh";

    /** Default connect timeout in seconds */
    private static final int DEFAULT_CONNECT_TIMEOUT = 30;

    /** Maximum image dimension (width or height) before downsampling */
    private static final int MAX_DIMENSION = 4096;

    /** Maximum file size supported (20MB encoded = ~15MB raw image) */
    private static final long MAX_FILE_SIZE = 20 * 1024 * 1024;

    /**
     * Result of an image load operation.
     */
    public static class ImageLoadResult {
        /** Whether the load succeeded */
        public final boolean success;

        /** Decoded bitmap (null if failed or too large with warning) */
        @Nullable
        public final Bitmap bitmap;

        /** Error message if load failed */
        @Nullable
        public final String errorMessage;

        /** Exit code from SSH command (null if command failed to start) */
        @Nullable
        public final Integer exitCode;

        /** Image width in pixels (0 if failed) */
        public final int width;

        /** Image height in pixels (0 if failed) */
        public final int height;

        /** Whether image was too large and downsampling was applied */
        public final boolean wasDownsampled;

        /** Original file size in bytes */
        public final long fileSize;

        /** Warning message if image was too large but still loaded */
        @Nullable
        public final String warning;

        private ImageLoadResult(boolean success, @Nullable Bitmap bitmap,
                                @Nullable String errorMessage, @Nullable Integer exitCode,
                                int width, int height, boolean wasDownsampled,
                                long fileSize, @Nullable String warning) {
            this.success = success;
            this.bitmap = bitmap;
            this.errorMessage = errorMessage;
            this.exitCode = exitCode;
            this.width = width;
            this.height = height;
            this.wasDownsampled = wasDownsampled;
            this.fileSize = fileSize;
            this.warning = warning;
        }

        /**
         * Create a successful load result.
         */
        @NonNull
        public static ImageLoadResult success(@NonNull Bitmap bitmap, int width, int height,
                                               long fileSize, boolean wasDownsampled,
                                               @Nullable String warning) {
            return new ImageLoadResult(true, bitmap, null, 0,
                                       width, height, wasDownsampled, fileSize, warning);
        }

        /**
         * Create a failed load result.
         */
        @NonNull
        public static ImageLoadResult failure(@NonNull String errorMessage,
                                               @Nullable Integer exitCode, long fileSize) {
            return new ImageLoadResult(false, null, errorMessage, exitCode,
                                       0, 0, false, fileSize, null);
        }

        /**
         * Create a result for dimension check failure (too large, not loaded).
         */
        @NonNull
        public static ImageLoadResult tooLarge(int width, int height, long fileSize,
                                                @NonNull String warning) {
            return new ImageLoadResult(false, null, warning, null,
                                       width, height, false, fileSize, warning);
        }

        @NonNull
        @Override
        public String toString() {
            return "ImageLoadResult{success=" + success +
                   ", width=" + width +
                   ", height=" + height +
                   ", wasDownsampled=" + wasDownsampled +
                   ", fileSize=" + fileSize +
                   ", errorMessage='" + truncate(errorMessage, 100) + "'}";
        }

        @NonNull
        private static String truncate(@Nullable String str, int maxLen) {
            if (str == null) return "(null)";
            if (str.length() <= maxLen) return str;
            return str.substring(0, maxLen) + "...";
        }
    }

    /**
     * Load a remote image file via SSH.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Path to image file on remote server
     * @return ImageLoadResult containing bitmap or error info
     */
    @NonNull
    public static ImageLoadResult loadImage(@NonNull Context context,
                                             @NonNull SSHConnectionInfo connection,
                                             @NonNull String remotePath) {
        Logger.logDebug(LOG_TAG, "Load started: " + connection.toString() +
                       " remotePath=" + remotePath);

        // Pre-validation: check SSH socket exists
        if (!checkSocketExists(connection.getSocketPath())) {
            String errorMsg = "SSH connection not available: control socket not found";
            Logger.logError(LOG_TAG, errorMsg);
            return ImageLoadResult.failure(errorMsg, null, 0);
        }

        // Pre-validation: check if file is an image by extension
        if (!ImageFileType.isImageFile(remotePath)) {
            String errorMsg = "File is not a supported image format";
            Logger.logError(LOG_TAG, errorMsg);
            return ImageLoadResult.failure(errorMsg, null, 0);
        }

        // Get file size first for validation
        long fileSize = getFileSize(context, connection, remotePath);
        if (fileSize < 0) {
            String errorMsg = "Failed to get remote file size: file may not exist or inaccessible";
            Logger.logError(LOG_TAG, errorMsg);
            return ImageLoadResult.failure(errorMsg, null, 0);
        }

        Logger.logDebug(LOG_TAG, "Remote file size: " + fileSize + " bytes");

        // Check file size limit
        if (fileSize > MAX_FILE_SIZE) {
            String errorMsg = "File too large: " + formatFileSize(fileSize) +
                              " exceeds limit of " + formatFileSize(MAX_FILE_SIZE);
            Logger.logError(LOG_TAG, errorMsg);
            return ImageLoadResult.failure(errorMsg, null, fileSize);
        }

        // Empty file case
        if (fileSize == 0) {
            String errorMsg = "Remote file is empty (0 bytes)";
            Logger.logError(LOG_TAG, errorMsg);
            return ImageLoadResult.failure(errorMsg, null, 0);
        }

        // Execute SSH command: base64 'escaped_remote_path'
        String escapedPath = RemoteFileOperator.escapePath(remotePath);
        String remoteCommand = "base64 " + escapedPath;

        String[] commandArgs = buildSSHCommand(connection, remoteCommand, DEFAULT_CONNECT_TIMEOUT);
        String commandString = joinCommand(commandArgs);

        Logger.logDebug(LOG_TAG, "Executing SSH command: " + commandString);

        // Create execution command
        ExecutionCommand executionCommand = new ExecutionCommand(
            0,                          // id
            SSH_BINARY,                 // executable
            commandArgs,                // arguments
            null,                       // stdin (no input needed)
            "/",                        // workingDirectory
            "ssh-image-load",           // runner
            false                       // isFailsafe
        );

        // Execute synchronously
        AppShell appShell = AppShell.execute(
            context,
            executionCommand,
            null,                       // callback (null for sync)
            new TermuxShellEnvironment(),
            null,                       // additionalEnvironment
            true                        // isSynchronous
        );

        if (appShell == null) {
            String errorMsg = "Failed to start SSH command execution";
            Logger.logError(LOG_TAG, errorMsg);
            return ImageLoadResult.failure(errorMsg, null, fileSize);
        }

        // Extract results
        Integer exitCode = executionCommand.resultData.exitCode;
        String stdout = executionCommand.resultData.stdout.toString();
        String stderr = executionCommand.resultData.stderr.toString();

        Logger.logDebug(LOG_TAG, "SSH command completed: exitCode=" + exitCode +
                       " stdoutLen=" + stdout.length() + " stderrLen=" + stderr.length());

        if (exitCode != null && exitCode != 0) {
            String errorMsg = parseLoadError(stderr, exitCode, remotePath);
            Logger.logError(LOG_TAG, "Load failed: " + errorMsg);
            return ImageLoadResult.failure(errorMsg, exitCode, fileSize);
        }

        // Decode base64 to byte array
        byte[] imageData;
        try {
            imageData = Base64.decode(stdout, Base64.NO_WRAP);
            Logger.logDebug(LOG_TAG, "Base64 decoded: " + stdout.length() +
                           " chars -> " + imageData.length + " bytes");
        } catch (IllegalArgumentException e) {
            String errorMsg = "Base64 decode failed: corrupted data";
            Logger.logError(LOG_TAG, errorMsg + ": " + e.getMessage());
            return ImageLoadResult.failure(errorMsg, exitCode, fileSize);
        }

        // Pre-check dimensions without allocating full bitmap
        BitmapFactory.Options boundsOptions = new BitmapFactory.Options();
        boundsOptions.inJustDecodeBounds = true;
        BitmapFactory.decodeByteArray(imageData, 0, imageData.length, boundsOptions);

        int width = boundsOptions.outWidth;
        int height = boundsOptions.outHeight;
        String mimeType = boundsOptions.outMimeType;

        Logger.logDebug(LOG_TAG, "Image dimensions: " + width + "x" + height +
                       " mimeType=" + mimeType);

        // Check if dimensions were detected
        if (width <= 0 || height <= 0) {
            String errorMsg = "Failed to detect image dimensions: may be corrupted or unsupported format";
            Logger.logError(LOG_TAG, errorMsg);
            return ImageLoadResult.failure(errorMsg, exitCode, fileSize);
        }

        // Calculate downsample factor if image is too large
        int sampleSize = calculateSampleSize(width, height, MAX_DIMENSION);
        boolean needsDownsample = sampleSize > 1;

        String warning = null;
        if (needsDownsample) {
            warning = "Image too large (" + width + "x" + height + "), downsampling by " +
                       sampleSize + "x for display";
            Logger.logDebug(LOG_TAG, warning);
        }

        // Decode actual bitmap with optional downsampling
        BitmapFactory.Options decodeOptions = new BitmapFactory.Options();
        decodeOptions.inSampleSize = sampleSize;
        decodeOptions.inPreferredConfig = Bitmap.Config.RGB_565; // Save memory for large images

        Bitmap bitmap = BitmapFactory.decodeByteArray(imageData, 0, imageData.length, decodeOptions);

        if (bitmap == null) {
            String errorMsg = "Failed to decode image bitmap";
            Logger.logError(LOG_TAG, errorMsg);
            return ImageLoadResult.failure(errorMsg, exitCode, fileSize);
        }

        // Get actual dimensions of decoded bitmap (may differ if downsampling)
        int actualWidth = bitmap.getWidth();
        int actualHeight = bitmap.getHeight();

        Logger.logDebug(LOG_TAG, "Load succeeded: " + actualWidth + "x" + actualHeight +
                       " bitmap, " + bitmap.getByteCount() + " bytes in memory");

        return ImageLoadResult.success(bitmap, actualWidth, actualHeight,
                                        fileSize, needsDownsample, warning);
    }

    /**
     * Get the size of a remote file.
     *
     * @param context Android context
     * @param connection SSH connection info
     * @param remotePath Path on remote server
     * @return File size in bytes, or -1 if error/not found
     */
    public static long getFileSize(@NonNull Context context,
                                   @NonNull SSHConnectionInfo connection,
                                   @NonNull String remotePath) {
        Logger.logDebug(LOG_TAG, "Getting file size: " + remotePath);

        // Pre-validation: check SSH socket exists
        if (!checkSocketExists(connection.getSocketPath())) {
            Logger.logError(LOG_TAG, "SSH connection not available");
            return -1;
        }

        // Execute: stat -c %s 'escaped_path'
        String escapedPath = RemoteFileOperator.escapePath(remotePath);
        String remoteCommand = "stat -c %s " + escapedPath + " 2>/dev/null || echo -1";

        String[] commandArgs = buildSSHCommand(connection, remoteCommand, 10);

        ExecutionCommand executionCommand = new ExecutionCommand(
            0,
            SSH_BINARY,
            commandArgs,
            null,
            "/",
            "ssh-stat",
            false
        );

        AppShell appShell = AppShell.execute(
            context,
            executionCommand,
            null,
            new TermuxShellEnvironment(),
            null,
            true
        );

        if (appShell == null) {
            Logger.logError(LOG_TAG, "Failed to execute stat command");
            return -1;
        }

        Integer exitCode = executionCommand.resultData.exitCode;
        String stdout = executionCommand.resultData.stdout.toString().trim();

        Logger.logDebug(LOG_TAG, "Stat result: exitCode=" + exitCode + " stdout=" + stdout);

        try {
            long size = Long.parseLong(stdout);
            return size >= 0 ? size : -1;
        } catch (NumberFormatException e) {
            Logger.logError(LOG_TAG, "Failed to parse file size: " + stdout);
            return -1;
        }
    }

    /**
     * Calculate sample size for downsampling.
     *
     * @param width Original width
     * @param height Original height
     * @param maxDimension Maximum allowed dimension
     * @return Sample size (power of 2, 1 = no downsampling)
     */
    private static int calculateSampleSize(int width, int height, int maxDimension) {
        if (width <= maxDimension && height <= maxDimension) {
            return 1;
        }

        // Calculate the smallest power-of-2 sample size that brings dimensions under limit
        int sampleSize = 1;
        int maxOriginal = Math.max(width, height);

        while (maxOriginal / sampleSize > maxDimension) {
            sampleSize *= 2;
        }

        return sampleSize;
    }

    /**
     * Check if SSH control socket exists.
     *
     * @param socketPath Path to control socket
     * @return true if socket file exists
     */
    private static boolean checkSocketExists(@NonNull String socketPath) {
        File socketFile = new File(socketPath);
        return socketFile.exists();
    }

    /**
     * Build SSH command arguments for remote command execution.
     *
     * @param connection SSH connection info
     * @param remoteCommand Command to execute on remote server
     * @param timeoutSeconds Connect timeout in seconds
     * @return Command arguments array
     */
    @NonNull
    private static String[] buildSSHCommand(@NonNull SSHConnectionInfo connection,
                                             @NonNull String remoteCommand,
                                             int timeoutSeconds) {
        return new String[]{
            "-S", connection.getSocketPath(),
            "-o", "BatchMode=yes",
            "-o", "ConnectTimeout=" + timeoutSeconds,
            connection.getUser() + "@" + connection.getHost(),
            remoteCommand
        };
    }

    /**
     * Parse load-specific error messages.
     *
     * @param stderr Raw stderr output
     * @param exitCode Exit code
     * @param remotePath Remote path for context
     * @return User-friendly error message
     */
    @NonNull
    private static String parseLoadError(@NonNull String stderr, int exitCode,
                                          @NonNull String remotePath) {
        if (stderr.isEmpty()) {
            return "Load failed with exit code " + exitCode;
        }

        // Common patterns for load errors
        if (stderr.contains("No such file or directory") || stderr.contains("cannot stat")) {
            return "Remote file not found";
        }
        if (stderr.contains("Permission denied")) {
            return "Permission denied reading remote file";
        }
        if (stderr.contains("Connection refused") || stderr.contains("Connection timed out")) {
            return "SSH connection timeout";
        }
        if (stderr.contains("base64")) {
            return "Remote base64 command failed";
        }

        return truncateForLog(stderr);
    }

    /**
     * Format file size for human-readable display.
     *
     * @param bytes Size in bytes
     * @return Formatted string (e.g., "1.5 MB")
     */
    @NonNull
    private static String formatFileSize(long bytes) {
        if (bytes < 1024) {
            return bytes + " B";
        }
        double kb = bytes / 1024.0;
        if (kb < 1024) {
            return String.format("%.1f KB", kb);
        }
        double mb = kb / 1024.0;
        return String.format("%.1f MB", mb);
    }

    /**
     * Join command arguments into a single string for logging.
     *
     * @param args Command arguments
     * @return Joined command string
     */
    @NonNull
    private static String joinCommand(@NonNull String[] args) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < args.length; i++) {
            if (i > 0) sb.append(" ");
            sb.append(args[i]);
        }
        return sb.toString();
    }

    /**
     * Truncate string for logging (avoid huge log output).
     *
     * @param str Input string
     * @return Truncated string (max 200 chars)
     */
    @NonNull
    private static String truncateForLog(@Nullable String str) {
        if (str == null) return "(null)";
        if (str.length() <= 200) return str;
        return str.substring(0, 200) + "...";
    }
}