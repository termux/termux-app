package com.termux.app.ssh;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.runner.app.AppShell;
import com.termux.shared.termux.shell.command.environment.TermuxShellEnvironment;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Service class for listing remote files via SSH ControlMaster.
 *
 * Uses existing SSH connection multiplexing to execute ls -la commands
 * on remote servers without requiring password re-entry.
 * Parses ls -la output and builds RemoteFile object lists.
 */
public class RemoteFileLister {

    private static final String LOG_TAG = "RemoteFileLister";

    /** SSH binary path (the wrapper that uses ControlMaster) */
    private static final String SSH_BINARY = "/data/data/com.termux/files/usr/bin/ssh";

    /** Regex pattern for parsing ls -la output lines
     * Format: permissions links owner group size date time name [-> target]
     * Example: drwxr-xr-x  2 user group 4096 Jan 15 10:30 dirname
     * Example: lrwxrwxrwx  1 user group   10 Jan 15 10:30 linkname -> target
     * Group indices: 1=permissions, 2=links, 3=owner, 4=group, 5=size, 6=date, 7=time, 8=name
     */
    private static final Pattern LS_LINE_PATTERN = Pattern.compile(
        "^([bcdlsp-][rwxst-]{9})\\s+" +      // group 1: permissions (10 chars)
        "(\\d+)\\s+" +                        // group 2: links
        "(\\S+)\\s+" +                        // group 3: owner
        "(\\S+)\\s+" +                        // group 4: group
        "(\\d+)\\s+" +                        // group 5: size
        "(\\w{3}\\s+\\d{1,2})\\s+" +          // group 6: date part (Jan 15)
        "(\\d{1,2}:\\d{2}|\\d{4})\\s+" +      // group 7: time or year (10:30 or 2024)
        "(.+)$"                               // group 8: name (may contain symlink target)
    );

    /** Pattern for splitting symlink name and target */
    private static final Pattern SYMLINK_PATTERN = Pattern.compile("^(.+?)\\s+->\\s+(.+)$");

    /** Pattern for parsing ls -laL output with symlink target type indicator */
    private static final Pattern LS_SYMLINK_TYPE_PATTERN = Pattern.compile(
        "^(.+?)\\s+->\\s+(.+?)/$"  // trailing / indicates directory target
    );

    /**
     * List files in a remote directory via SSH ControlMaster.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Path on remote server to list
     * @return List of RemoteFile objects, empty list on error
     */
    @NonNull
    public static List<RemoteFile> listDirectory(@NonNull Context context,
                                                  @NonNull SSHConnectionInfo connection,
                                                  @NonNull String remotePath) {
        return listDirectory(context, connection, remotePath, null);
    }

    /**
     * List files in a remote directory via SSH ControlMaster with callback.
     *
     * Executes ssh command through control socket and parses ls -la output.
     * Synchronous execution - blocks until command completes.
     *
     * @param context Android context
     * @param connection SSH connection info with control socket
     * @param remotePath Path on remote server to list
     * @param callback Optional callback for async execution (null for sync)
     * @return List of RemoteFile objects, empty list on error
     */
    @NonNull
    public static List<RemoteFile> listDirectory(@NonNull Context context,
                                                  @NonNull SSHConnectionInfo connection,
                                                  @NonNull String remotePath,
                                                  @Nullable AppShell.AppShellClient callback) {
        List<RemoteFile> files = new ArrayList<>();

        // Build SSH command: ssh -S socketPath host "ls -la path"
        String[] commandArgs = buildSSHCommand(connection, remotePath);
        String commandString = joinCommand(commandArgs);

        Logger.logDebug(LOG_TAG, "Listing remote directory: " + connection.toString() + ":" + remotePath);
        Logger.logDebug(LOG_TAG, "SSH command: " + commandString);

        // Create execution command
        ExecutionCommand executionCommand = new ExecutionCommand(
            0,                          // id
            SSH_BINARY,                 // executable
            commandArgs,                // arguments (null means use executable directly)
            null,                       // stdin
            "/",                        // workingDirectory (local)
            "ssh-ls",                   // runner
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
            return files;
        }

        // For synchronous execution, process results now
        if (isSynchronous) {
            files = parseLSOutput(executionCommand.resultData.stdout.toString(),
                                  executionCommand.resultData.stderr.toString(),
                                  executionCommand.resultData.exitCode,
                                  remotePath);
        }

        return files;
    }

    /**
     * Build SSH command arguments for listing directory.
     *
     * Uses control socket for connection multiplexing.
     * Uses -F flag to append type indicators (/ for directories, @ for symlinks).
     *
     * IMPORTANT: For symlink-to-directory paths, we must append a trailing slash
     * to ensure ls lists the directory contents, not the symlink itself.
     * Without trailing slash: "ls -laF /path/to/symlink" returns symlink info
     * With trailing slash: "ls -laF /path/to/symlink/" lists directory contents
     *
     * @param connection SSH connection info
     * @param remotePath Path to list on remote server
     * @return Command arguments array
     */
    @NonNull
    private static String[] buildSSHCommand(@NonNull SSHConnectionInfo connection,
                                             @NonNull String remotePath) {
        // SSH command: ssh -S socketPath user@host "ls -laF 'path/'"
        // Use -S to specify control socket
        // Use -o BatchMode=yes to prevent password prompts (should use existing connection)
        // Use -F to append type indicators (/ for dir, @ for symlink, * for executable)
        // Quote remote path for shell safety

        // Ensure path ends with / to follow symlinks to directories
        // This is critical: "ls -laF symlink" returns symlink info,
        // but "ls -laF symlink/" lists the target directory contents
        String pathWithSlash = remotePath;
        if (!remotePath.equals("/") && !remotePath.endsWith("/")) {
            pathWithSlash = remotePath + "/";
        }
        String escapedPath = escapePathForSSH(pathWithSlash);

        return new String[]{
            "-S", connection.getSocketPath(),
            "-o", "BatchMode=yes",
            "-o", "ConnectTimeout=10",
            connection.getUser() + "@" + connection.getHost(),
            "ls -laF " + escapedPath
        };
    }

    /**
     * Escape path for SSH remote command.
     *
     * Handles paths with spaces, special characters, etc.
     *
     * @param path Raw path string
     * @return Escaped path safe for SSH command
     */
    @NonNull
    private static String escapePathForSSH(@NonNull String path) {
        // Use single quotes for SSH remote command, escape existing single quotes
        // Replace ' with '\'' (end quote, escaped quote, start quote)
        if (path.contains("'")) {
            return "'" + path.replace("'", "'\\''") + "'";
        }
        return "'" + path + "'";
    }

    /**
     * Parse ls -la output and build RemoteFile list.
     *
     * @param stdout Command stdout (ls -la output)
     * @param stderr Command stderr (error messages)
     * @param exitCode Command exit code
     * @param basePath Remote path that was listed
     * @return List of RemoteFile objects
     */
    @NonNull
    private static List<RemoteFile> parseLSOutput(@NonNull String stdout,
                                                   @NonNull String stderr,
                                                   @Nullable Integer exitCode,
                                                   @NonNull String basePath) {
        List<RemoteFile> files = new ArrayList<>();

        // Check exit code
        if (exitCode == null) {
            Logger.logError(LOG_TAG, "SSH command exit code is null (process may have failed)");
            Logger.logDebug(LOG_TAG, "stderr: " + truncateForLog(stderr));
            return files;
        }

        if (exitCode != 0) {
            Logger.logError(LOG_TAG, "SSH command failed with exit code " + exitCode);
            Logger.logDebug(LOG_TAG, "stderr: " + truncateForLog(stderr));
            return files;
        }

        Logger.logDebug(LOG_TAG, "SSH command succeeded, parsing output...");
        Logger.logDebug(LOG_TAG, "stdout length: " + stdout.length() + " chars");

        // Split output into lines
        String[] lines = stdout.split("\n");
        int parsedCount = 0;
        int skippedCount = 0;

        for (String line : lines) {
            line = line.trim();

            // Skip empty lines and "total X" header line
            if (line.isEmpty() || line.startsWith("total ")) {
                skippedCount++;
                continue;
            }

            // Parse line using regex
            RemoteFile file = parseLSLine(line, basePath);

            if (file != null) {
                // Skip . and .. entries (user can navigate via parent dir button)
                if (!file.getName().equals(".") && !file.getName().equals("..")) {
                    files.add(file);
                    parsedCount++;
                    Logger.logVerbose(LOG_TAG, "Parsed: " + file.toString());
                } else {
                    skippedCount++;
                }
            } else {
                skippedCount++;
                Logger.logDebug(LOG_TAG, "Failed to parse line: " + truncateForLog(line));
            }
        }

        Logger.logDebug(LOG_TAG, "Parse complete: " + parsedCount + " files parsed, " +
                       skippedCount + " lines skipped, " + files.size() + " files returned");

        return files;
    }

    /**
     * Parse a single ls -laF output line into RemoteFile.
     *
     * ls -laF appends type indicators:
     * - / for directories
     * - @ for symbolic links
     * - * for executables
     *
     * For symlinks, we also check if the target ends with / to determine
     * if the symlink points to a directory.
     *
     * @param line Raw ls -laF line
     * @param basePath Directory path that was listed
     * @return RemoteFile object, or null if parsing failed
     */
    @Nullable
    private static RemoteFile parseLSLine(@NonNull String line, @NonNull String basePath) {
        Matcher matcher = LS_LINE_PATTERN.matcher(line);

        if (!matcher.matches()) {
            return null;
        }

        try {
            // Extract fields from regex groups
            String permissions = matcher.group(1);
            int links = Integer.parseInt(matcher.group(2));
            String owner = matcher.group(3);
            String group = matcher.group(4);
            long size = Long.parseLong(matcher.group(5));
            String modifyTime = matcher.group(6) + " " + matcher.group(7);
            String nameField = matcher.group(8);

            // Determine file type from first character of permissions
            char typeChar = permissions.charAt(0);
            RemoteFile.FileType type = RemoteFile.getTypeFromPermissionChar(typeChar);

            // Handle type indicators from ls -laF
            // - Directories have trailing /
            // - Symlinks have trailing @
            // - Executables have trailing *
            String name = nameField;
            String symlinkTarget = null;
            boolean symlinkTargetIsDirectory = false;

            // Check for symlink type indicator (@)
            if (name.endsWith("@")) {
                name = name.substring(0, name.length() - 1);
            }

            // Handle symlinks: parse "name -> target"
            if (type == RemoteFile.FileType.SYMLINK) {
                Matcher symlinkMatcher = SYMLINK_PATTERN.matcher(name);
                if (symlinkMatcher.matches()) {
                    name = symlinkMatcher.group(1);
                    symlinkTarget = symlinkMatcher.group(2);

                    // Remove @ from name if present (ls -laF adds it)
                    if (name.endsWith("@")) {
                        name = name.substring(0, name.length() - 1);
                    }

                    // Check if symlink target ends with / (indicates directory)
                    if (symlinkTarget.endsWith("/")) {
                        symlinkTarget = symlinkTarget.substring(0, symlinkTarget.length() - 1);
                        symlinkTargetIsDirectory = true;
                    }
                } else {
                    // Handle abnormal symlink output: if name contains no " -> " but appears
                    // to be an absolute path, it may be the target path displayed directly.
                    // This can happen with certain ls implementations or corrupted output.
                    // Extract the last path segment as the actual link name.
                    if (name.startsWith("/")) {
                        // This looks like an absolute path, not a link name
                        // Use it as the symlink target and extract name from it
                        symlinkTarget = name;
                        // Check if target ends with / (directory indicator)
                        if (symlinkTarget.endsWith("/")) {
                            symlinkTarget = symlinkTarget.substring(0, symlinkTarget.length() - 1);
                            symlinkTargetIsDirectory = true;
                        }
                        // Extract the last segment as the link name
                        // First, remove trailing / for extraction
                        String pathForExtraction = name;
                        if (pathForExtraction.endsWith("/")) {
                            pathForExtraction = pathForExtraction.substring(0, pathForExtraction.length() - 1);
                        }
                        int lastSlash = pathForExtraction.lastIndexOf('/');
                        if (lastSlash >= 0 && lastSlash < pathForExtraction.length() - 1) {
                            name = pathForExtraction.substring(lastSlash + 1);
                            // Remove any trailing @ or / from extracted name
                            if (name.endsWith("@") || name.endsWith("/")) {
                                name = name.substring(0, name.length() - 1);
                            }
                        }
                        Logger.logDebug(LOG_TAG, "Parsed abnormal symlink: name=" + name +
                            ", target=" + symlinkTarget + ", from field: " + nameField);
                    }
                    // If name still has @ suffix after all processing, remove it
                    if (name.endsWith("@")) {
                        name = name.substring(0, name.length() - 1);
                    }
                }
            } else if (type == RemoteFile.FileType.DIRECTORY) {
                // Remove trailing / from directory names (ls -laF adds it)
                if (name.endsWith("/")) {
                    name = name.substring(0, name.length() - 1);
                }
            }

            // Build full path (normalize basePath to not end with /)
            String normalizedBase = basePath.endsWith("/") && basePath.length() > 1
                ? basePath.substring(0, basePath.length() - 1)
                : basePath;
            String fullPath = normalizedBase + "/" + name;

            return new RemoteFile(
                name,
                fullPath,
                type,
                size,
                modifyTime,
                permissions.substring(1), // Remove type char, keep rwx string
                owner,
                group,
                symlinkTarget,
                symlinkTargetIsDirectory
            );

        } catch (Exception e) {
            Logger.logError(LOG_TAG, "Exception parsing ls line: " + e.getMessage());
            return null;
        }
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