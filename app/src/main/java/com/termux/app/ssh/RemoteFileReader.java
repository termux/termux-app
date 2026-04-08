package com.termux.app.ssh;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.runner.app.AppShell;
import com.termux.shared.termux.shell.command.environment.TermuxShellEnvironment;

/**
 * Service class for reading remote files via SSH ControlMaster.
 *
 * Uses existing SSH connection multiplexing to execute cat commands
 * on remote servers without requiring password re-entry.
 * Returns file content with exit code and error information.
 */
public class RemoteFileReader {

    private static final String LOG_TAG = "RemoteFileReader";

    /** SSH binary path (the wrapper that uses ControlMaster) */
    private static final String SSH_BINARY = "/data/data/com.termux/files/usr/bin/ssh";

    /**
     * Result data class for file read operations.
     *
     * Encapsulates exit code, file content, and error message.
     */
    public static class ReadResult {
        /** Exit code from SSH command (0 = success, non-zero = error) */
        private final int exitCode;

        /** File content (null on error or if file is empty) */
        private final String content;

        /** Error message from stderr (null on success) */
        private final String errorMessage;

        /**
         * Create a ReadResult.
         *
         * @param exitCode SSH command exit code
         * @param content File content (may be null or empty)
         * @param errorMessage Error message (null on success)
         */
        public ReadResult(int exitCode, @Nullable String content, @Nullable String errorMessage) {
            this.exitCode = exitCode;
            this.content = content;
            this.errorMessage = errorMessage;
        }

        /**
         * Check if read operation succeeded.
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
         * Get file content.
         *
         * @return File content string, may be null or empty
         */
        @Nullable
        public String getContent() {
            return content;
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
                return "ReadResult[success, contentLength=" +
                       (content != null ? content.length() : 0) + "]";
            }
            return "ReadResult[error, exitCode=" + exitCode +
                   ", error=" + truncateForLog(errorMessage) + "]";
        }
    }

    /**
     * Read a remote file via SSH ControlMaster.
     *
     * Executes ssh command through control socket and returns file content.
     * Synchronous execution - blocks until command completes.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Path to file on remote server
     * @return ReadResult containing exit code, content, and error message
     */
    @NonNull
    public static ReadResult readFile(@NonNull Context context,
                                       @NonNull SSHConnectionInfo connection,
                                       @NonNull String remotePath) {
        return readFile(context, connection, remotePath, null);
    }

    /**
     * Read a remote file via SSH ControlMaster with callback.
     *
     * Executes ssh command through control socket and returns file content.
     * For async execution, callback receives result via AppShellClient interface.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Path to file on remote server
     * @param callback Optional callback for async execution (null for sync)
     * @return ReadResult containing exit code, content, and error message
     */
    @NonNull
    public static ReadResult readFile(@NonNull Context context,
                                       @NonNull SSHConnectionInfo connection,
                                       @NonNull String remotePath,
                                       @Nullable AppShell.AppShellClient callback) {
        // Build SSH command: ssh -S socketPath user@host "cat 'path'"
        String[] commandArgs = buildSSHCommand(connection, remotePath);
        String commandString = joinCommand(commandArgs);

        Logger.logDebug(LOG_TAG, "Reading remote file: " + connection.toString() + ":" + remotePath);
        Logger.logDebug(LOG_TAG, "SSH command: " + commandString);

        // Create execution command
        ExecutionCommand executionCommand = new ExecutionCommand(
            0,                          // id
            SSH_BINARY,                 // executable
            commandArgs,                // arguments
            null,                       // stdin
            "/",                        // workingDirectory (local)
            "ssh-cat",                  // runner
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
            return new ReadResult(-1, null, "Failed to start SSH command");
        }

        // For synchronous execution, process results now
        if (isSynchronous) {
            String stdout = executionCommand.resultData.stdout.toString();
            String stderr = executionCommand.resultData.stderr.toString();
            Integer exitCode = executionCommand.resultData.exitCode;

            int exit = (exitCode != null) ? exitCode : -1;

            Logger.logDebug(LOG_TAG, "SSH command completed: exitCode=" + exit +
                           ", stdout=" + stdout.length() + " chars, stderr=" +
                           truncateForLog(stderr));

            if (exit == 0) {
                return new ReadResult(0, stdout, null);
            } else {
                return new ReadResult(exit, null, stderr);
            }
        }

        // For async execution, return placeholder result
        // Actual result will be delivered via callback
        return new ReadResult(-1, null, "Async execution pending");
    }

    /**
     * Build SSH command arguments for reading file.
     *
     * Uses control socket for connection multiplexing.
     *
     * @param connection SSH connection info
     * @param remotePath Path to file on remote server
     * @return Command arguments array
     */
    @NonNull
    private static String[] buildSSHCommand(@NonNull SSHConnectionInfo connection,
                                             @NonNull String remotePath) {
        // SSH command: ssh -S socketPath user@host "cat 'path'"
        // Use -S to specify control socket
        // Use -o BatchMode=yes to prevent password prompts (should use existing connection)
        // Quote remote path for shell safety
        String escapedPath = escapePathForSSH(remotePath);

        return new String[]{
            "-S", connection.getSocketPath(),
            "-o", "BatchMode=yes",
            "-o", "ConnectTimeout=10",
            connection.getUser() + "@" + connection.getHost(),
            "cat " + escapedPath
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