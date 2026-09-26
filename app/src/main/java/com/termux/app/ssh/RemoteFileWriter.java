package com.termux.app.ssh;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.runner.app.AppShell;
import com.termux.shared.termux.shell.command.environment.TermuxShellEnvironment;

import java.util.Base64;

/**
 * Service class for writing remote files via SSH ControlMaster.
 *
 * Uses existing SSH connection multiplexing to write files
 * on remote servers without requiring password re-entry.
 * Uses base64 encoding for binary-safe file transfer.
 */
public class RemoteFileWriter {

    private static final String LOG_TAG = "RemoteFileWriter";

    /** SSH binary path (the wrapper that uses ControlMaster) */
    private static final String SSH_BINARY = "/data/data/com.termux/files/usr/bin/ssh";

    /**
     * Result data class for file write operations.
     *
     * Encapsulates exit code and error message.
     */
    public static class WriteResult {
        /** Exit code from SSH command (0 = success, non-zero = error) */
        private final int exitCode;

        /** Error message from stderr (null on success) */
        private final String errorMessage;

        /**
         * Create a WriteResult.
         *
         * @param exitCode SSH command exit code
         * @param errorMessage Error message (null on success)
         */
        public WriteResult(int exitCode, @Nullable String errorMessage) {
            this.exitCode = exitCode;
            this.errorMessage = errorMessage;
        }

        /**
         * Check if write operation succeeded.
         *
         * @return true if exit code is 0
         */
        public boolean isSuccess() {
            return exitCode == 0;
        }

        /**
         * Get exit code.
         *
         * @return SSH command exit code
         */
        public int getExitCode() {
            return exitCode;
        }

        /**
         * Get error message.
         *
         * @return Error message from stderr, null on success
         */
        @Nullable
        public String getErrorMessage() {
            return errorMessage;
        }

        @NonNull
        @Override
        public String toString() {
            if (isSuccess()) {
                return "WriteResult[success]";
            }
            return "WriteResult[error, exitCode=" + exitCode +
                   ", error=" + truncateForLog(errorMessage) + "]";
        }
    }

    /**
     * Write content to a remote file via SSH ControlMaster.
     *
     * Uses base64 encoding to ensure binary-safe transfer.
     * Content is encoded locally, sent via SSH, and decoded on remote.
     * Synchronous execution - blocks until command completes.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Path to file on remote server
     * @param content Content to write (string, will be encoded as UTF-8)
     * @return WriteResult containing exit code and error message
     */
    @NonNull
    public static WriteResult writeFile(@NonNull Context context,
                                          @NonNull SSHConnectionInfo connection,
                                          @NonNull String remotePath,
                                          @NonNull String content) {
        return writeFile(context, connection, remotePath, content, null);
    }

    /**
     * Write content to a remote file via SSH ControlMaster with callback.
     *
     * Uses base64 encoding to ensure binary-safe transfer.
     * Content is encoded locally, sent via SSH, and decoded on remote.
     * For async execution, callback receives result via AppShellClient interface.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Path to file on remote server
     * @param content Content to write (string, will be encoded as UTF-8)
     * @param callback Optional callback for async execution (null for sync)
     * @return WriteResult containing exit code and error message
     */
    @NonNull
    public static WriteResult writeFile(@NonNull Context context,
                                          @NonNull SSHConnectionInfo connection,
                                          @NonNull String remotePath,
                                          @NonNull String content,
                                          @Nullable AppShell.AppShellClient callback) {
        // Encode content as base64 for binary-safe transfer
        byte[] contentBytes = content.getBytes(java.nio.charset.StandardCharsets.UTF_8);
        String base64Content = Base64.getEncoder().encodeToString(contentBytes);

        Logger.logDebug(LOG_TAG, "Writing remote file: " + connection.toString() + ":" + remotePath);
        Logger.logDebug(LOG_TAG, "Content size: " + contentBytes.length + " bytes, base64 size: " +
                       base64Content.length() + " chars");

        // Build SSH command with stdin containing base64 data
        String[] commandArgs = buildSSHCommand(connection, remotePath);
        String commandString = joinCommand(commandArgs);

        Logger.logDebug(LOG_TAG, "SSH command: " + commandString);

        // Create execution command with stdin containing base64-encoded content
        ExecutionCommand executionCommand = new ExecutionCommand(
            0,                          // id
            SSH_BINARY,                 // executable
            commandArgs,                // arguments
            base64Content,              // stdin - base64 encoded content
            "/",                        // workingDirectory (local)
            "ssh-write",                // runner
            false                       // isFailsafe
        );

        // Execute synchronously if no callback
        boolean isSynchronous = (callback == null);

        AppShell appShell = AppShell.execute(
            context,
            executionCommand,
            callback,
            new TermuxShellEnvironment(),
            null,                       // additionalEnvironment
            isSynchronous
        );

        if (appShell == null) {
            Logger.logError(LOG_TAG, "Failed to execute SSH command: AppShell returned null");
            Logger.logDebug(LOG_TAG, "Command failed to start - possible SSH binary not found or connection issue");
            return new WriteResult(-1, "Failed to start SSH command");
        }

        // For synchronous execution, process results now
        if (isSynchronous) {
            String stderr = executionCommand.resultData.stderr.toString();
            Integer exitCode = executionCommand.resultData.exitCode;

            int exit = (exitCode != null) ? exitCode : -1;

            Logger.logDebug(LOG_TAG, "SSH command completed: exitCode=" + exit +
                           ", stderr=" + truncateForLog(stderr));

            if (exit == 0) {
                return new WriteResult(0, null);
            } else {
                return new WriteResult(exit, stderr);
            }
        }

        // For async execution, return placeholder result
        // Actual result will be delivered via callback
        return new WriteResult(-1, "Async execution pending");
    }

    /**
     * Write binary content to a remote file via SSH ControlMaster.
     *
     * Uses base64 encoding to ensure binary-safe transfer.
     * Binary data is encoded locally, sent via SSH, and decoded on remote.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Path to file on remote server
     * @param content Binary content to write
     * @return WriteResult containing exit code and error message
     */
    @NonNull
    public static WriteResult writeBinaryFile(@NonNull Context context,
                                               @NonNull SSHConnectionInfo connection,
                                               @NonNull String remotePath,
                                               @NonNull byte[] content) {
        return writeBinaryFile(context, connection, remotePath, content, null);
    }

    /**
     * Write binary content to a remote file via SSH ControlMaster with callback.
     *
     * Uses base64 encoding to ensure binary-safe transfer.
     * Binary data is encoded locally, sent via SSH, and decoded on remote.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Path to file on remote server
     * @param content Binary content to write
     * @param callback Optional callback for async execution (null for sync)
     * @return WriteResult containing exit code and error message
     */
    @NonNull
    public static WriteResult writeBinaryFile(@NonNull Context context,
                                               @NonNull SSHConnectionInfo connection,
                                               @NonNull String remotePath,
                                               @NonNull byte[] content,
                                               @Nullable AppShell.AppShellClient callback) {
        // Encode content as base64 for binary-safe transfer
        String base64Content = Base64.getEncoder().encodeToString(content);

        Logger.logDebug(LOG_TAG, "Writing binary remote file: " + connection.toString() + ":" + remotePath);
        Logger.logDebug(LOG_TAG, "Content size: " + content.length + " bytes, base64 size: " +
                       base64Content.length() + " chars");

        // Build SSH command with stdin containing base64 data
        String[] commandArgs = buildSSHCommand(connection, remotePath);
        String commandString = joinCommand(commandArgs);

        Logger.logDebug(LOG_TAG, "SSH command: " + commandString);

        // Create execution command with stdin containing base64-encoded content
        ExecutionCommand executionCommand = new ExecutionCommand(
            0,                          // id
            SSH_BINARY,                 // executable
            commandArgs,                // arguments
            base64Content,              // stdin - base64 encoded content
            "/",                        // workingDirectory (local)
            "ssh-write-bin",            // runner
            false                       // isFailsafe
        );

        // Execute synchronously if no callback
        boolean isSynchronous = (callback == null);

        AppShell appShell = AppShell.execute(
            context,
            executionCommand,
            callback,
            new TermuxShellEnvironment(),
            null,                       // additionalEnvironment
            isSynchronous
        );

        if (appShell == null) {
            Logger.logError(LOG_TAG, "Failed to execute SSH command: AppShell returned null");
            Logger.logDebug(LOG_TAG, "Command failed to start - possible SSH binary not found or connection issue");
            return new WriteResult(-1, "Failed to start SSH command");
        }

        // For synchronous execution, process results now
        if (isSynchronous) {
            String stderr = executionCommand.resultData.stderr.toString();
            Integer exitCode = executionCommand.resultData.exitCode;

            int exit = (exitCode != null) ? exitCode : -1;

            Logger.logDebug(LOG_TAG, "SSH command completed: exitCode=" + exit +
                           ", stderr=" + truncateForLog(stderr));

            if (exit == 0) {
                return new WriteResult(0, null);
            } else {
                return new WriteResult(exit, stderr);
            }
        }

        // For async execution, return placeholder result
        return new WriteResult(-1, "Async execution pending");
    }

    /**
     * Build SSH command arguments for writing file.
     *
     * Uses control socket for connection multiplexing.
     * Reads base64 content from stdin and decodes to file.
     *
     * @param connection SSH connection info
     * @param remotePath Path to file on remote server
     * @return Command arguments array
     */
    @NonNull
    private static String[] buildSSHCommand(@NonNull SSHConnectionInfo connection,
                                             @NonNull String remotePath) {
        // SSH command: ssh -S socketPath user@host "base64 -d > 'path'"
        // stdin contains base64-encoded content
        // Use -S to specify control socket
        // Use -o BatchMode=yes to prevent password prompts (should use existing connection)
        // Quote remote path for shell safety
        String escapedPath = escapePathForSSH(remotePath);

        return new String[]{
            "-S", connection.getSocketPath(),
            "-o", "BatchMode=yes",
            "-o", "ConnectTimeout=10",
            connection.getUser() + "@" + connection.getHost(),
            "base64 -d > " + escapedPath
        };
    }

    /**
     * Escape path for SSH remote command.
     *
     * Handles paths with spaces, special characters, and single quotes.
     * Uses single quote escaping: 'path' with embedded quotes escaped as '\''
     *
     * @param path Raw path string
     * @return Escaped path safe for SSH command
     */
    @NonNull
    public static String escapePathForSSH(@NonNull String path) {
        // Use single quotes for SSH remote command, escape existing single quotes
        // Replace ' with '\'' (end quote, escaped quote, start quote)
        if (path.contains("'")) {
            return "'" + path.replace("'", "'\\''") + "'";
        }
        return "'" + path + "'";
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
    public static String truncateForLog(@Nullable String str) {
        if (str == null) return "(null)";
        if (str.length() <= 200) return str;
        return str.substring(0, 200) + "...";
    }
}