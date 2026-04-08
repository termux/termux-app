package com.termux.app.ssh;

import android.content.Context;
import android.util.Base64;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.runner.app.AppShell;
import com.termux.shared.termux.shell.command.environment.TermuxShellEnvironment;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;

/**
 * Service class for bidirectional file transfer via SSH ControlMaster.
 *
 * Supports uploading files from Android to remote server and downloading
 * files from remote server to Android using base64 encoding for binary safety.
 *
 * Design rationale:
 * - Uses base64 encoding to ensure binary-safe transfer (images, archives, etc.)
 * - Executes SSH remote commands instead of relying on scp binary
 *   (SAF provides content URIs, not file paths that scp can use)
 * - ProgressCallback reports real-time transfer progress
 * - TransferResult encapsulates success/failure status with detailed error messages
 */
public class RemoteFileTransfer {

    private static final String LOG_TAG = "RemoteFileTransfer";

    /** SSH binary path (the wrapper that uses ControlMaster) */
    private static final String SSH_BINARY = "/data/data/com.termux/files/usr/bin/ssh";

    /** Default connect timeout in seconds */
    private static final int DEFAULT_CONNECT_TIMEOUT = 30;

    /** Chunk size for reading/writing data (4KB) */
    private static final int CHUNK_SIZE = 4096;

    /** Chunk size for streaming transfer (1MB) - prevents OOM for large files */
    private static final int TRANSFER_CHUNK_SIZE = 1024 * 1024;

    /** Maximum file size supported (50MB) - larger files may cause memory pressure */
    private static final long MAX_FILE_SIZE = 50 * 1024 * 1024;

    /**
     * Result of a file transfer operation.
     */
    public static class TransferResult {
        /** Whether the transfer succeeded */
        public final boolean success;

        /** Number of bytes transferred */
        public final long bytesTransferred;

        /** Total bytes expected (may differ from actual if error occurred) */
        public final long totalBytes;

        /** Error message if transfer failed */
        @Nullable
        public final String errorMessage;

        /** Exit code from SSH command (null if command failed to start) */
        @Nullable
        public final Integer exitCode;

        TransferResult(boolean success, long bytesTransferred, long totalBytes,
                       @Nullable String errorMessage, @Nullable Integer exitCode) {
            this.success = success;
            this.bytesTransferred = bytesTransferred;
            this.totalBytes = totalBytes;
            this.errorMessage = errorMessage;
            this.exitCode = exitCode;
        }

        /**
         * Create a successful transfer result.
         */
        @NonNull
        public static TransferResult success(long bytesTransferred, long totalBytes) {
            return new TransferResult(true, bytesTransferred, totalBytes, null, 0);
        }

        /**
         * Create a failed transfer result.
         */
        @NonNull
        public static TransferResult failure(@NonNull String errorMessage,
                                              @Nullable Integer exitCode,
                                              long bytesTransferred,
                                              long totalBytes) {
            return new TransferResult(false, bytesTransferred, totalBytes, errorMessage, exitCode);
        }

        @NonNull
        @Override
        public String toString() {
            return "TransferResult{success=" + success +
                   ", bytesTransferred=" + bytesTransferred +
                   ", totalBytes=" + totalBytes +
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
     * Callback interface for transfer progress notifications.
     */
    public interface ProgressCallback {
        /**
         * Called periodically during transfer to report progress.
         *
         * @param bytesTransferred Number of bytes transferred so far
         * @param totalBytes Total bytes to transfer
         */
        void onProgress(long bytesTransferred, long totalBytes);

        /**
         * Called when transfer completes (success or failure).
         *
         * @param result Transfer result containing success status and details
         */
        void onComplete(@NonNull TransferResult result);
    }

    /**
     * Upload a file from Android to remote server via SSH.
     *
     * Reads data from InputStream, encodes to base64, and sends to remote
     * server where it's decoded and written to the target path.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param inputStream InputStream containing file data (from SAF content URI)
     * @param fileName File name for remote path (used for logging)
     * @param fileSize Total size of file in bytes (for progress reporting)
     * @param remotePath Destination path on remote server
     * @param callback Progress callback (may be null)
     * @return TransferResult indicating success or failure
     */
    @NonNull
    public static TransferResult upload(@NonNull Context context,
                                         @NonNull SSHConnectionInfo connection,
                                         @NonNull InputStream inputStream,
                                         @NonNull String fileName,
                                         long fileSize,
                                         @NonNull String remotePath,
                                         @Nullable ProgressCallback callback) {
        Logger.logDebug(LOG_TAG, "Upload started: " + connection.toString() +
                       " fileName=" + fileName + " fileSize=" + fileSize +
                       " remotePath=" + remotePath);

        // Pre-validation: check file size limit
        if (fileSize > MAX_FILE_SIZE) {
            String errorMsg = "File too large: " + formatFileSize(fileSize) +
                              " exceeds limit of " + formatFileSize(MAX_FILE_SIZE);
            Logger.logError(LOG_TAG, errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, null, 0, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Pre-validation: check SSH socket exists
        if (!checkSocketExists(connection.getSocketPath())) {
            String errorMsg = "SSH connection not available: control socket not found";
            Logger.logError(LOG_TAG, errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, null, 0, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Read and encode file data
        byte[] encodedData;
        long bytesRead = 0;
        try {
            encodedData = readAndEncodeBase64(inputStream, fileSize, callback);
            bytesRead = fileSize; // If encoding succeeded, we read all bytes
            Logger.logDebug(LOG_TAG, "Base64 encoding complete: " + encodedData.length +
                           " bytes encoded from " + fileSize + " original bytes");
        } catch (IOException e) {
            String errorMsg = "Failed to read local file: " + e.getMessage();
            Logger.logError(LOG_TAG, errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, null, bytesRead, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Execute SSH command: base64 -d > 'escaped_remote_path'
        String escapedPath = RemoteFileOperator.escapePath(remotePath);
        String remoteCommand = "base64 -d > " + escapedPath;

        String[] commandArgs = buildSSHCommand(connection, remoteCommand, DEFAULT_CONNECT_TIMEOUT);
        String commandString = joinCommand(commandArgs);

        Logger.logDebug(LOG_TAG, "Executing SSH upload command: " + commandString);

        // Create execution command with stdin data
        ExecutionCommand executionCommand = new ExecutionCommand(
            0,                          // id
            SSH_BINARY,                 // executable
            commandArgs,                // arguments
            new String(encodedData, StandardCharsets.UTF_8), // stdin (base64 data)
            "/",                        // workingDirectory
            "ssh-upload",               // runner
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
            TransferResult result = TransferResult.failure(errorMsg, null, bytesRead, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Extract results
        Integer exitCode = executionCommand.resultData.exitCode;
        String stdout = executionCommand.resultData.stdout.toString();
        String stderr = executionCommand.resultData.stderr.toString();

        Logger.logDebug(LOG_TAG, "Upload SSH command completed: exitCode=" + exitCode +
                       " stderrLen=" + stderr.length());

        if (exitCode != null && exitCode != 0) {
            String errorMsg = parseUploadError(stderr, exitCode, remotePath);
            Logger.logError(LOG_TAG, "Upload failed: " + errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, exitCode, bytesRead, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        Logger.logDebug(LOG_TAG, "Upload succeeded: " + bytesRead + " bytes transferred to " + remotePath);
        TransferResult result = TransferResult.success(bytesRead, fileSize);
        if (callback != null) callback.onComplete(result);
        return result;
    }

    /**
     * Download a file from remote server to Android via SSH.
     *
     * Executes base64 encoding on remote server, retrieves encoded content,
     * decodes it, and writes to OutputStream.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Source path on remote server
     * @param outputStream OutputStream to write file data (from SAF content URI)
     * @param callback Progress callback (may be null)
     * @return TransferResult indicating success or failure
     */
    @NonNull
    public static TransferResult download(@NonNull Context context,
                                           @NonNull SSHConnectionInfo connection,
                                           @NonNull String remotePath,
                                           @NonNull OutputStream outputStream,
                                           @Nullable ProgressCallback callback) {
        Logger.logDebug(LOG_TAG, "Download started: " + connection.toString() +
                       " remotePath=" + remotePath);

        // Pre-validation: check SSH socket exists
        if (!checkSocketExists(connection.getSocketPath())) {
            String errorMsg = "SSH connection not available: control socket not found";
            Logger.logError(LOG_TAG, errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, null, 0, 0);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Get file size first
        long fileSize = getFileSize(context, connection, remotePath);
        if (fileSize < 0) {
            String errorMsg = "Failed to get remote file size: file may not exist or inaccessible";
            Logger.logError(LOG_TAG, errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, null, 0, 0);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        Logger.logDebug(LOG_TAG, "Remote file size: " + fileSize + " bytes");

        // Check file size limit
        if (fileSize > MAX_FILE_SIZE) {
            String errorMsg = "File too large: " + formatFileSize(fileSize) +
                              " exceeds limit of " + formatFileSize(MAX_FILE_SIZE);
            Logger.logError(LOG_TAG, errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, null, 0, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Empty file case: just create empty file locally
        if (fileSize == 0) {
            Logger.logDebug(LOG_TAG, "Downloading empty file (0 bytes)");
            TransferResult result = TransferResult.success(0, 0);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Execute SSH command: base64 'escaped_remote_path'
        String escapedPath = RemoteFileOperator.escapePath(remotePath);
        String remoteCommand = "base64 " + escapedPath;

        String[] commandArgs = buildSSHCommand(connection, remoteCommand, DEFAULT_CONNECT_TIMEOUT);
        String commandString = joinCommand(commandArgs);

        Logger.logDebug(LOG_TAG, "Executing SSH download command: " + commandString);

        // Create execution command
        ExecutionCommand executionCommand = new ExecutionCommand(
            0,                          // id
            SSH_BINARY,                 // executable
            commandArgs,                // arguments
            null,                       // stdin (no input needed)
            "/",                        // workingDirectory
            "ssh-download",             // runner
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
            TransferResult result = TransferResult.failure(errorMsg, null, 0, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Extract results
        Integer exitCode = executionCommand.resultData.exitCode;
        String stdout = executionCommand.resultData.stdout.toString();
        String stderr = executionCommand.resultData.stderr.toString();

        Logger.logDebug(LOG_TAG, "Download SSH command completed: exitCode=" + exitCode +
                       " stdoutLen=" + stdout.length() + " stderrLen=" + stderr.length());

        if (exitCode != null && exitCode != 0) {
            String errorMsg = parseDownloadError(stderr, exitCode, remotePath);
            Logger.logError(LOG_TAG, "Download failed: " + errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, exitCode, 0, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Decode base64 and write to output stream
        long bytesWritten = 0;
        try {
            bytesWritten = decodeAndWrite(stdout, outputStream, fileSize, callback);
            Logger.logDebug(LOG_TAG, "Base64 decode complete: " + bytesWritten + " bytes written");
        } catch (IOException e) {
            String errorMsg = "Failed to write downloaded data: " + e.getMessage();
            Logger.logError(LOG_TAG, errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, exitCode, bytesWritten, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        } catch (IllegalArgumentException e) {
            String errorMsg = "Remote file corrupted: base64 decode failed";
            Logger.logError(LOG_TAG, errorMsg + ": " + e.getMessage());
            TransferResult result = TransferResult.failure(errorMsg, exitCode, bytesWritten, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        Logger.logDebug(LOG_TAG, "Download succeeded: " + bytesWritten + " bytes from " + remotePath);
        TransferResult result = TransferResult.success(bytesWritten, fileSize);
        if (callback != null) callback.onComplete(result);
        return result;
    }

    /**
     * Upload a file from Android to remote server using chunked streaming.
     *
     * Transfers files in 1MB chunks to prevent OOM for large files.
     * Each chunk is read from InputStream, base64 encoded, and sent via SSH
     * where dd writes it to the target path at the correct offset.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param inputStream InputStream containing file data (from SAF content URI)
     * @param fileName File name for remote path (used for logging)
     * @param fileSize Total size of file in bytes (for progress reporting)
     * @param remotePath Destination path on remote server
     * @param callback Progress callback (may be null)
     * @return TransferResult indicating success or failure
     */
    @NonNull
    public static TransferResult uploadChunked(@NonNull Context context,
                                                @NonNull SSHConnectionInfo connection,
                                                @NonNull InputStream inputStream,
                                                @NonNull String fileName,
                                                long fileSize,
                                                @NonNull String remotePath,
                                                @Nullable ProgressCallback callback) {
        Logger.logDebug(LOG_TAG, "Chunked upload started: " + connection.toString() +
                       " fileName=" + fileName + " fileSize=" + fileSize +
                       " remotePath=" + remotePath);

        // Pre-validation: check SSH socket exists
        if (!checkSocketExists(connection.getSocketPath())) {
            String errorMsg = "SSH connection not available: control socket not found";
            Logger.logError(LOG_TAG, errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, null, 0, fileSize);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Empty file case: just create empty file on remote
        if (fileSize == 0) {
            Logger.logDebug(LOG_TAG, "Uploading empty file (0 bytes)");
            // Create empty file via touch
            String escapedPath = RemoteFileOperator.escapePath(remotePath);
            String remoteCommand = "touch " + escapedPath + " || true";

            String[] commandArgs = buildSSHCommand(connection, remoteCommand, DEFAULT_CONNECT_TIMEOUT);
            ExecutionCommand executionCommand = new ExecutionCommand(
                0, SSH_BINARY, commandArgs, null, "/", "ssh-upload-empty", false
            );

            AppShell appShell = AppShell.execute(
                context, executionCommand, null, new TermuxShellEnvironment(), null, true
            );

            if (appShell == null || (executionCommand.resultData.exitCode != null &&
                                      executionCommand.resultData.exitCode != 0)) {
                String errorMsg = "Failed to create empty remote file";
                Logger.logError(LOG_TAG, errorMsg);
                TransferResult result = TransferResult.failure(errorMsg, 
                    executionCommand.resultData.exitCode, 0, 0);
                if (callback != null) callback.onComplete(result);
                return result;
            }

            TransferResult result = TransferResult.success(0, 0);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        Logger.logDebug(LOG_TAG, "Remote file size: " + fileSize + " bytes (" +
                       formatFileSize(fileSize) + ")");

        // Calculate total chunks
        int totalChunks = (int) (fileSize / TRANSFER_CHUNK_SIZE);
        if (fileSize % TRANSFER_CHUNK_SIZE != 0) {
            totalChunks++;
        }

        Logger.logDebug(LOG_TAG, "Uploading in " + totalChunks + " chunks of " +
                       TRANSFER_CHUNK_SIZE + " bytes each");

        // Report initial progress
        if (callback != null) {
            callback.onProgress(0, fileSize);
        }

        // Upload each chunk
        long bytesUploaded = 0;
        String escapedPath = RemoteFileOperator.escapePath(remotePath);
        byte[] chunkBuffer = new byte[TRANSFER_CHUNK_SIZE];

        for (int chunkIndex = 0; chunkIndex < totalChunks; chunkIndex++) {
            // Calculate chunk size for this iteration
            int expectedChunkSize = (int) Math.min(TRANSFER_CHUNK_SIZE, fileSize - bytesUploaded);

            Logger.logDebug(LOG_TAG, "Uploading chunk " + (chunkIndex + 1) + "/" + totalChunks +
                           " offset=" + bytesUploaded + " expectedSize=" + expectedChunkSize);

            // Read chunk from InputStream
            int bytesRead;
            try {
                bytesRead = readFully(inputStream, chunkBuffer, expectedChunkSize);
                if (bytesRead != expectedChunkSize) {
                    String errorMsg = "Unexpected read: got " + bytesRead + " bytes, expected " + expectedChunkSize;
                    Logger.logError(LOG_TAG, errorMsg);
                    TransferResult result = TransferResult.failure(errorMsg, null, bytesUploaded, fileSize);
                    if (callback != null) callback.onComplete(result);
                    return result;
                }
            } catch (IOException e) {
                String errorMsg = "Failed to read chunk " + (chunkIndex + 1) + ": " + e.getMessage();
                Logger.logError(LOG_TAG, errorMsg);
                TransferResult result = TransferResult.failure(errorMsg, null, bytesUploaded, fileSize);
                if (callback != null) callback.onComplete(result);
                return result;
            }

            // Encode chunk to base64
            byte[] encodedChunk = Base64.encode(chunkBuffer, 0, bytesRead, Base64.NO_WRAP);

            Logger.logDebug(LOG_TAG, "Chunk " + (chunkIndex + 1) + " encoded: " + bytesRead +
                           " bytes -> " + encodedChunk.length + " base64 bytes");

            // Build SSH command: base64 -d | dd of=file bs=1M seek=N conv=notrunc
            // For first chunk, we don't need seek; subsequent chunks use seek to append
            String remoteCommand;
            if (chunkIndex == 0) {
                // First chunk: create file
                remoteCommand = "base64 -d | dd of=" + escapedPath + " bs=" + TRANSFER_CHUNK_SIZE +
                               " count=1 2>/dev/null";
            } else {
                // Subsequent chunks: append at correct offset
                remoteCommand = "base64 -d | dd of=" + escapedPath + " bs=" + TRANSFER_CHUNK_SIZE +
                               " seek=" + chunkIndex + " conv=notrunc 2>/dev/null";
            }

            String[] commandArgs = buildSSHCommand(connection, remoteCommand, DEFAULT_CONNECT_TIMEOUT);
            String commandString = joinCommand(commandArgs);

            Logger.logDebug(LOG_TAG, "Executing chunk upload command: " + commandString);

            // Create execution command with stdin (base64 encoded chunk)
            ExecutionCommand executionCommand = new ExecutionCommand(
                0,
                SSH_BINARY,
                commandArgs,
                new String(encodedChunk, StandardCharsets.UTF_8), // stdin
                "/",
                "ssh-upload-chunk-" + chunkIndex,
                false
            );

            // Execute synchronously
            AppShell appShell = AppShell.execute(
                context,
                executionCommand,
                null,
                new TermuxShellEnvironment(),
                null,
                true
            );

            if (appShell == null) {
                String errorMsg = "Failed to start SSH command for chunk " + (chunkIndex + 1);
                Logger.logError(LOG_TAG, errorMsg);
                TransferResult result = TransferResult.failure(errorMsg, null, bytesUploaded, fileSize);
                if (callback != null) callback.onComplete(result);
                return result;
            }

            // Extract results
            Integer exitCode = executionCommand.resultData.exitCode;
            String stderr = executionCommand.resultData.stderr.toString();

            Logger.logDebug(LOG_TAG, "Chunk " + (chunkIndex + 1) + " SSH command completed: exitCode=" +
                           exitCode);

            if (exitCode != null && exitCode != 0) {
                String errorMsg = parseUploadError(stderr, exitCode, remotePath) +
                                  " (chunk " + (chunkIndex + 1) + ")";
                Logger.logError(LOG_TAG, "Chunk upload failed: " + errorMsg);
                TransferResult result = TransferResult.failure(errorMsg, exitCode, bytesUploaded, fileSize);
                if (callback != null) callback.onComplete(result);
                return result;
            }

            bytesUploaded += bytesRead;

            Logger.logDebug(LOG_TAG, "Chunk " + (chunkIndex + 1) + " uploaded successfully, " +
                           "total uploaded: " + bytesUploaded + " bytes");

            // Report progress
            if (callback != null) {
                callback.onProgress(bytesUploaded, fileSize);
            }
        }

        Logger.logDebug(LOG_TAG, "Chunked upload succeeded: " + bytesUploaded + " bytes to " + remotePath);
        TransferResult result = TransferResult.success(bytesUploaded, fileSize);
        if (callback != null) callback.onComplete(result);
        return result;
    }

    /**
     * Read exactly the specified number of bytes from InputStream.
     *
     * @param inputStream Input stream to read from
     * @param buffer Buffer to read into
     * @param bytesToRead Number of bytes to read
     * @return Number of bytes actually read
     */
    private static int readFully(@NonNull InputStream inputStream,
                                  @NonNull byte[] buffer,
                                  int bytesToRead) throws IOException {
        int totalRead = 0;
        while (totalRead < bytesToRead) {
            int read = inputStream.read(buffer, totalRead, bytesToRead - totalRead);
            if (read == -1) {
                break; // EOF reached early
            }
            totalRead += read;
        }
        return totalRead;
    }

    /**
     * Download a file from remote server using chunked streaming.
     *
     * Transfers files in 1MB chunks to prevent OOM for large files.
     * Each chunk is fetched using dd | base64, decoded, and written to OutputStream.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Source path on remote server
     * @param outputStream OutputStream to write file data (from SAF content URI)
     * @param callback Progress callback (may be null)
     * @return TransferResult indicating success or failure
     */
    @NonNull
    public static TransferResult downloadChunked(@NonNull Context context,
                                                  @NonNull SSHConnectionInfo connection,
                                                  @NonNull String remotePath,
                                                  @NonNull OutputStream outputStream,
                                                  @Nullable ProgressCallback callback) {
        Logger.logDebug(LOG_TAG, "Chunked download started: " + connection.toString() +
                       " remotePath=" + remotePath);

        // Pre-validation: check SSH socket exists
        if (!checkSocketExists(connection.getSocketPath())) {
            String errorMsg = "SSH connection not available: control socket not found";
            Logger.logError(LOG_TAG, errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, null, 0, 0);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Get file size first
        long fileSize = getFileSize(context, connection, remotePath);
        if (fileSize < 0) {
            String errorMsg = "Failed to get remote file size: file may not exist or inaccessible";
            Logger.logError(LOG_TAG, errorMsg);
            TransferResult result = TransferResult.failure(errorMsg, null, 0, 0);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        Logger.logDebug(LOG_TAG, "Remote file size: " + fileSize + " bytes (" +
                       formatFileSize(fileSize) + ")");

        // Empty file case: just create empty file locally
        if (fileSize == 0) {
            Logger.logDebug(LOG_TAG, "Downloading empty file (0 bytes)");
            TransferResult result = TransferResult.success(0, 0);
            if (callback != null) callback.onComplete(result);
            return result;
        }

        // Calculate total chunks
        int totalChunks = (int) (fileSize / TRANSFER_CHUNK_SIZE);
        if (fileSize % TRANSFER_CHUNK_SIZE != 0) {
            totalChunks++;
        }

        Logger.logDebug(LOG_TAG, "Downloading in " + totalChunks + " chunks of " +
                       TRANSFER_CHUNK_SIZE + " bytes each");

        // Report initial progress
        if (callback != null) {
            callback.onProgress(0, fileSize);
        }

        // Download each chunk
        long bytesWritten = 0;
        String escapedPath = RemoteFileOperator.escapePath(remotePath);

        for (int chunkIndex = 0; chunkIndex < totalChunks; chunkIndex++) {
            long skipBytes = chunkIndex * TRANSFER_CHUNK_SIZE;
            long chunkSize = Math.min(TRANSFER_CHUNK_SIZE, fileSize - skipBytes);

            Logger.logDebug(LOG_TAG, "Downloading chunk " + (chunkIndex + 1) + "/" + totalChunks +
                           " skip=" + skipBytes + " size=" + chunkSize);

            // Execute SSH command: dd if=file bs=1M skip=N count=1 | base64
            // Note: dd skip uses block count, so skip=N means skip N blocks of bs size
            String remoteCommand = "dd if=" + escapedPath + " bs=" + TRANSFER_CHUNK_SIZE +
                                   " skip=" + chunkIndex + " count=1 2>/dev/null | base64";

            String[] commandArgs = buildSSHCommand(connection, remoteCommand, DEFAULT_CONNECT_TIMEOUT);
            String commandString = joinCommand(commandArgs);

            Logger.logDebug(LOG_TAG, "Executing chunk download command: " + commandString);

            // Create execution command
            ExecutionCommand executionCommand = new ExecutionCommand(
                0,
                SSH_BINARY,
                commandArgs,
                null,
                "/",
                "ssh-download-chunk-" + chunkIndex,
                false
            );

            // Execute synchronously
            AppShell appShell = AppShell.execute(
                context,
                executionCommand,
                null,
                new TermuxShellEnvironment(),
                null,
                true
            );

            if (appShell == null) {
                String errorMsg = "Failed to start SSH command for chunk " + (chunkIndex + 1);
                Logger.logError(LOG_TAG, errorMsg);
                TransferResult result = TransferResult.failure(errorMsg, null, bytesWritten, fileSize);
                if (callback != null) callback.onComplete(result);
                return result;
            }

            // Extract results
            Integer exitCode = executionCommand.resultData.exitCode;
            String stdout = executionCommand.resultData.stdout.toString();
            String stderr = executionCommand.resultData.stderr.toString();

            Logger.logDebug(LOG_TAG, "Chunk " + (chunkIndex + 1) + " SSH command completed: exitCode=" +
                           exitCode + " stdoutLen=" + stdout.length());

            if (exitCode != null && exitCode != 0) {
                String errorMsg = parseDownloadError(stderr, exitCode, remotePath) +
                                  " (chunk " + (chunkIndex + 1) + ")";
                Logger.logError(LOG_TAG, "Chunk download failed: " + errorMsg);
                TransferResult result = TransferResult.failure(errorMsg, exitCode, bytesWritten, fileSize);
                if (callback != null) callback.onComplete(result);
                return result;
            }

            // Decode chunk and write to output stream
            try {
                byte[] decodedChunk = Base64.decode(stdout, Base64.NO_WRAP);
                outputStream.write(decodedChunk);
                bytesWritten += decodedChunk.length;

                Logger.logDebug(LOG_TAG, "Chunk " + (chunkIndex + 1) + " decoded: " +
                               decodedChunk.length + " bytes, total written: " + bytesWritten);

                // Report progress
                if (callback != null) {
                    callback.onProgress(bytesWritten, fileSize);
                }
            } catch (IllegalArgumentException e) {
                String errorMsg = "Base64 decode failed for chunk " + (chunkIndex + 1) + ": " + e.getMessage();
                Logger.logError(LOG_TAG, errorMsg);
                TransferResult result = TransferResult.failure(errorMsg, exitCode, bytesWritten, fileSize);
                if (callback != null) callback.onComplete(result);
                return result;
            } catch (IOException e) {
                String errorMsg = "Write failed for chunk " + (chunkIndex + 1) + ": " + e.getMessage();
                Logger.logError(LOG_TAG, errorMsg);
                TransferResult result = TransferResult.failure(errorMsg, exitCode, bytesWritten, fileSize);
                if (callback != null) callback.onComplete(result);
                return result;
            }
        }

        // Flush output stream
        try {
            outputStream.flush();
        } catch (IOException e) {
            Logger.logError(LOG_TAG, "Failed to flush output stream: " + e.getMessage());
        }

        Logger.logDebug(LOG_TAG, "Chunked download succeeded: " + bytesWritten + " bytes from " + remotePath);
        TransferResult result = TransferResult.success(bytesWritten, fileSize);
        if (callback != null) callback.onComplete(result);
        return result;
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
            Logger.logError(LOG_TAG, "SSH connection not available: control socket not found");
            return -1;
        }

        // Execute: stat -c %s 'escaped_path'
        String escapedPath = RemoteFileOperator.escapePath(remotePath);
        String remoteCommand = "stat -c %s " + escapedPath + " 2>/dev/null || echo -1";

        String[] commandArgs = buildSSHCommand(connection, remoteCommand, 10);
        String commandString = joinCommand(commandArgs);

        Logger.logDebug(LOG_TAG, "Executing stat command: " + commandString);

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

        Logger.logDebug(LOG_TAG, "Stat command result: exitCode=" + exitCode + " stdout=" + stdout);

        try {
            long size = Long.parseLong(stdout);
            return size >= 0 ? size : -1;
        } catch (NumberFormatException e) {
            Logger.logError(LOG_TAG, "Failed to parse file size: " + stdout);
            return -1;
        }
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
     * Read InputStream and encode to base64 with progress reporting.
     *
     * @param inputStream Input stream to read
     * @param fileSize Total file size for progress calculation
     * @param callback Progress callback (may be null)
     * @return Base64 encoded byte array
     */
    @NonNull
    private static byte[] readAndEncodeBase64(@NonNull InputStream inputStream,
                                               long fileSize,
                                               @Nullable ProgressCallback callback) throws IOException {
        ByteArrayOutputStream buffer = new ByteArrayOutputStream();
        byte[] chunk = new byte[CHUNK_SIZE];
        long totalRead = 0;
        int bytesRead;

        // Report initial progress
        if (callback != null) {
            callback.onProgress(0, fileSize);
        }

        while ((bytesRead = inputStream.read(chunk)) != -1) {
            buffer.write(chunk, 0, bytesRead);
            totalRead += bytesRead;

            // Report progress after each chunk
            if (callback != null && fileSize > 0) {
                callback.onProgress(totalRead, fileSize);
            }
        }

        // Encode complete data to base64
        byte[] rawBytes = buffer.toByteArray();
        byte[] encoded = Base64.encode(rawBytes, Base64.NO_WRAP);

        Logger.logDebug(LOG_TAG, "Encoded " + rawBytes.length + " bytes to " +
                       encoded.length + " base64 bytes (ratio: " +
                       String.format("%.2f", (double) encoded.length / rawBytes.length) + ")");

        return encoded;
    }

    /**
     * Decode base64 stdout and write to OutputStream with progress reporting.
     *
     * @param base64Data Base64 encoded string from SSH stdout
     * @param outputStream Output stream to write decoded data
     * @param fileSize Expected file size for progress
     * @param callback Progress callback (may be null)
     * @return Number of bytes written
     */
    private static long decodeAndWrite(@NonNull String base64Data,
                                        @NonNull OutputStream outputStream,
                                        long fileSize,
                                        @Nullable ProgressCallback callback) throws IOException, IllegalArgumentException {
        // Report initial progress
        if (callback != null) {
            callback.onProgress(0, fileSize);
        }

        // Decode base64 data
        byte[] decoded = Base64.decode(base64Data, Base64.NO_WRAP);

        // Write decoded data in chunks for progress reporting
        int offset = 0;
        long totalWritten = 0;
        int chunkWriteSize = CHUNK_SIZE;

        while (offset < decoded.length) {
            int writeLen = Math.min(chunkWriteSize, decoded.length - offset);
            outputStream.write(decoded, offset, writeLen);
            offset += writeLen;
            totalWritten += writeLen;

            // Report progress after each chunk
            if (callback != null && fileSize > 0) {
                callback.onProgress(totalWritten, fileSize);
            }
        }

        outputStream.flush();

        Logger.logDebug(LOG_TAG, "Decoded " + base64Data.length() + " base64 chars to " +
                       totalWritten + " bytes");

        return totalWritten;
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
     * Parse upload-specific error messages.
     *
     * @param stderr Raw stderr output
     * @param exitCode Exit code
     * @param remotePath Remote path for context
     * @return User-friendly error message
     */
    @NonNull
    private static String parseUploadError(@NonNull String stderr, int exitCode,
                                            @NonNull String remotePath) {
        if (stderr.isEmpty()) {
            return "Upload failed with exit code " + exitCode;
        }

        // Common patterns for upload errors
        if (stderr.contains("No such file or directory")) {
            // Could be parent directory doesn't exist
            return "Remote directory does not exist";
        }
        if (stderr.contains("Permission denied")) {
            return "Permission denied writing to remote path";
        }
        if (stderr.contains("cannot create")) {
            return "Cannot create file: permission denied or path invalid";
        }
        if (stderr.contains("disk quota exceeded") || stderr.contains("No space left")) {
            return "Remote disk full or quota exceeded";
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
     * Parse download-specific error messages.
     *
     * @param stderr Raw stderr output
     * @param exitCode Exit code
     * @param remotePath Remote path for context
     * @return User-friendly error message
     */
    @NonNull
    private static String parseDownloadError(@NonNull String stderr, int exitCode,
                                              @NonNull String remotePath) {
        if (stderr.isEmpty()) {
            return "Download failed with exit code " + exitCode;
        }

        // Common patterns for download errors
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
        if (mb < 1024) {
            return String.format("%.1f MB", mb);
        }
        double gb = mb / 1024.0;
        return String.format("%.1f GB", gb);
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