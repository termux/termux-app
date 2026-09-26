package com.termux.app.ssh;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.runner.app.AppShell;
import com.termux.shared.termux.shell.command.environment.TermuxShellEnvironment;

/**
 * Service class for remote file operations via SSH ControlMaster.
 *
 * Supports copy, move, delete, rename, and mkdir operations on remote files
 * using existing SSH connection multiplexing.
 */
public class RemoteFileOperator {

    private static final String LOG_TAG = "RemoteFileOperator";

    /** SSH binary path (the wrapper that uses ControlMaster) */
    private static final String SSH_BINARY = "/data/data/com.termux/files/usr/bin/ssh";

    /**
     * Result of a remote file operation.
     */
    public static class OperationResult {
        /** Whether the operation succeeded (exit code 0) */
        public final boolean success;

        /** Exit code from the SSH command */
        @Nullable
        public final Integer exitCode;

        /** stdout from the command */
        @NonNull
        public final String stdout;

        /** stderr from the command (error messages) */
        @NonNull
        public final String stderr;

        /** Parsed error message if operation failed */
        @Nullable
        public final String errorMessage;

        OperationResult(boolean success, @Nullable Integer exitCode,
                        @NonNull String stdout, @NonNull String stderr,
                        @Nullable String errorMessage) {
            this.success = success;
            this.exitCode = exitCode;
            this.stdout = stdout;
            this.stderr = stderr;
            this.errorMessage = errorMessage;
        }

        @NonNull
        @Override
        public String toString() {
            return "OperationResult{success=" + success +
                   ", exitCode=" + exitCode +
                   ", stderr='" + truncate(stderr, 100) + "'}";
        }

        @NonNull
        private static String truncate(@NonNull String str, int maxLen) {
            if (str.length() <= maxLen) return str;
            return str.substring(0, maxLen) + "...";
        }
    }

    /**
     * Copy a file or directory on the remote server.
     *
     * @param context Android context
     * @param connection SSH connection info
     * @param sourcePath Source file/directory path
     * @param destPath Destination path
     * @param recursive If true, copy directories recursively (-r flag)
     * @return Operation result
     */
    @NonNull
    public static OperationResult copy(@NonNull Context context,
                                        @NonNull SSHConnectionInfo connection,
                                        @NonNull String sourcePath,
                                        @NonNull String destPath,
                                        boolean recursive) {
        String command = recursive
            ? "cp -r " + escapePath(sourcePath) + " " + escapePath(destPath)
            : "cp " + escapePath(sourcePath) + " " + escapePath(destPath);

        Logger.logDebug(LOG_TAG, "Copy operation: " + connection.toString() +
                       " src=" + sourcePath + " dst=" + destPath + " recursive=" + recursive);

        return executeCommand(context, connection, command, "copy");
    }

    /**
     * Move a file or directory on the remote server.
     *
     * @param context Android context
     * @param connection SSH connection info
     * @param sourcePath Source file/directory path
     * @param destPath Destination path
     * @return Operation result
     */
    @NonNull
    public static OperationResult move(@NonNull Context context,
                                        @NonNull SSHConnectionInfo connection,
                                        @NonNull String sourcePath,
                                        @NonNull String destPath) {
        String command = "mv " + escapePath(sourcePath) + " " + escapePath(destPath);

        Logger.logDebug(LOG_TAG, "Move operation: " + connection.toString() +
                       " src=" + sourcePath + " dst=" + destPath);

        return executeCommand(context, connection, command, "move");
    }

    /**
     * Delete a file or directory on the remote server.
     *
     * @param context Android context
     * @param connection SSH connection info
     * @param path Path to delete
     * @param recursive If true, delete directories recursively (-r flag)
     * @param force If true, force deletion without prompts (-f flag)
     * @return Operation result
     */
    @NonNull
    public static OperationResult delete(@NonNull Context context,
                                          @NonNull SSHConnectionInfo connection,
                                          @NonNull String path,
                                          boolean recursive,
                                          boolean force) {
        StringBuilder cmdBuilder = new StringBuilder("rm");
        if (force) cmdBuilder.append(" -f");
        if (recursive) cmdBuilder.append(" -r");
        cmdBuilder.append(" ").append(escapePath(path));

        String command = cmdBuilder.toString();

        Logger.logDebug(LOG_TAG, "Delete operation: " + connection.toString() +
                       " path=" + path + " recursive=" + recursive + " force=" + force);

        return executeCommand(context, connection, command, "delete");
    }

    /**
     * Rename a file or directory on the remote server.
     *
     * This is a convenience method that uses mv command internally.
     *
     * @param context Android context
     * @param connection SSH connection info
     * @param path Current path
     * @param newName New name (just the name, not full path)
     * @return Operation result
     */
    @NonNull
    public static OperationResult rename(@NonNull Context context,
                                          @NonNull SSHConnectionInfo connection,
                                          @NonNull String path,
                                          @NonNull String newName) {
        // Build new path: dirname of current path + new name
        String parentDir = getParentDirectory(path);
        String newPath = parentDir + "/" + newName;

        Logger.logDebug(LOG_TAG, "Rename operation: " + connection.toString() +
                       " path=" + path + " newName=" + newName + " newPath=" + newPath);

        return move(context, connection, path, newPath);
    }

    /**
     * Create a directory on the remote server.
     *
     * @param context Android context
     * @param connection SSH connection info
     * @param path Directory path to create
     * @param createParents If true, create parent directories as needed (-p flag)
     * @return Operation result
     */
    @NonNull
    public static OperationResult mkdir(@NonNull Context context,
                                         @NonNull SSHConnectionInfo connection,
                                         @NonNull String path,
                                         boolean createParents) {
        String command = createParents
            ? "mkdir -p " + escapePath(path)
            : "mkdir " + escapePath(path);

        Logger.logDebug(LOG_TAG, "Mkdir operation: " + connection.toString() +
                       " path=" + path + " createParents=" + createParents);

        return executeCommand(context, connection, command, "mkdir");
    }

    /**
     * Execute an SSH command via ControlMaster and return result.
     *
     * @param context Android context
     * @param connection SSH connection info
     * @param remoteCommand Command to execute on remote server
     * @param operationLabel Label for logging (e.g., "copy", "delete")
     * @return OperationResult with success status and output
     */
    @NonNull
    private static OperationResult executeCommand(@NonNull Context context,
                                                   @NonNull SSHConnectionInfo connection,
                                                   @NonNull String remoteCommand,
                                                   @NonNull String operationLabel) {
        // Build SSH command arguments
        String[] commandArgs = buildSSHCommand(connection, remoteCommand);
        String commandString = joinCommand(commandArgs);

        Logger.logDebug(LOG_TAG, "Executing SSH command for " + operationLabel + ": " + commandString);

        // Create execution command
        ExecutionCommand executionCommand = new ExecutionCommand(
            0,                          // id
            SSH_BINARY,                 // executable
            commandArgs,                // arguments
            null,                       // stdin
            "/",                        // workingDirectory (local)
            "ssh-" + operationLabel,    // runner
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
            Logger.logError(LOG_TAG, "Failed to execute SSH command: AppShell returned null");
            Logger.logDebug(LOG_TAG, "Command failed to start - possible SSH binary not found or connection issue");
            return new OperationResult(false, null, "", "",
                "Failed to start SSH command execution");
        }

        // Extract results
        Integer exitCode = executionCommand.resultData.exitCode;
        String stdout = executionCommand.resultData.stdout.toString();
        String stderr = executionCommand.resultData.stderr.toString();

        Logger.logDebug(LOG_TAG, "SSH command completed: exitCode=" + exitCode +
                       " stdoutLen=" + stdout.length() + " stderrLen=" + stderr.length());

        if (exitCode != null && exitCode != 0) {
            Logger.logError(LOG_TAG, "SSH command failed with exit code " + exitCode);
            Logger.logDebug(LOG_TAG, "stderr: " + truncateForLog(stderr));
            Logger.logDebug(LOG_TAG, "stdout: " + truncateForLog(stdout));

            String errorMessage = parseErrorMessage(stderr, exitCode);
            return new OperationResult(false, exitCode, stdout, stderr, errorMessage);
        }

        Logger.logDebug(LOG_TAG, operationLabel + " operation succeeded");
        return new OperationResult(true, exitCode, stdout, stderr, null);
    }

    /**
     * Build SSH command arguments for remote file operation.
     *
     * @param connection SSH connection info
     * @param remoteCommand Command to execute on remote server
     * @return Command arguments array
     */
    @NonNull
    private static String[] buildSSHCommand(@NonNull SSHConnectionInfo connection,
                                             @NonNull String remoteCommand) {
        return new String[]{
            "-S", connection.getSocketPath(),
            "-o", "BatchMode=yes",
            "-o", "ConnectTimeout=10",
            connection.getUser() + "@" + connection.getHost(),
            remoteCommand
        };
    }

    /**
     * Escape path for SSH remote command using single quotes.
     *
     * Handles paths with spaces, special characters, and single quotes.
     *
     * @param path Raw path string
     * @return Escaped path safe for SSH command
     */
    @NonNull
    public static String escapePath(@NonNull String path) {
        // Use single quotes for SSH remote command, escape existing single quotes
        // Replace ' with '\'' (end quote, escaped quote, start quote)
        if (path.contains("'")) {
            return "'" + path.replace("'", "'\\''") + "'";
        }
        return "'" + path + "'";
    }

    /**
     * Get parent directory of a path.
     *
     * @param path Full path
     * @return Parent directory path (or "/" if path is root)
     */
    @NonNull
    private static String getParentDirectory(@NonNull String path) {
        // Normalize: remove trailing slash unless path is just "/"
        String normalized = path.endsWith("/") && path.length() > 1
            ? path.substring(0, path.length() - 1)
            : path;

        int lastSlash = normalized.lastIndexOf('/');
        if (lastSlash <= 0) {
            return "/";
        }
        return normalized.substring(0, lastSlash);
    }

    /**
     * Parse error message from stderr for user-friendly display.
     *
     * @param stderr Raw stderr output
     * @param exitCode Exit code
     * @return User-friendly error message
     */
    @NonNull
    private static String parseErrorMessage(@NonNull String stderr, int exitCode) {
        if (stderr.isEmpty()) {
            return "Operation failed with exit code " + exitCode;
        }

        // Common SSH/SCP error patterns
        if (stderr.contains("No such file or directory")) {
            return "File or directory not found";
        }
        if (stderr.contains("Permission denied")) {
            return "Permission denied";
        }
        if (stderr.contains("cannot create directory")) {
            return "Cannot create directory: " + extractPathFromError(stderr);
        }
        if (stderr.contains("cannot remove")) {
            return "Cannot remove: " + extractPathFromError(stderr);
        }
        if (stderr.contains("cannot stat")) {
            return "File not found: " + extractPathFromError(stderr);
        }
        if (stderr.contains("is a directory")) {
            return "Operation requires recursive flag for directories";
        }
        if (stderr.contains("not a directory")) {
            return "Target path is not a directory";
        }
        if (stderr.contains("Connection refused") || stderr.contains("Connection timed out")) {
            return "SSH connection failed";
        }

        // Return truncated stderr as fallback
        return truncateForLog(stderr);
    }

    /**
     * Extract file path from common error messages.
     *
     * @param error Error message string
     * @return Extracted path or empty string
     */
    @NonNull
    private static String extractPathFromError(@NonNull String error) {
        // Pattern: "cannot create directory 'path'" or similar
        int start = error.indexOf('\'');
        if (start >= 0) {
            int end = error.indexOf("'", start + 1);
            if (end > start) {
                return error.substring(start + 1, end);
            }
        }
        return "";
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