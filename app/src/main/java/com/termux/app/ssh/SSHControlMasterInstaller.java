package com.termux.app.ssh;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.system.Os;

import com.termux.shared.errors.Error;
import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * SSH ControlMaster Installer - manages SSH connection multiplexing setup.
 *
 * Responsibilities:
 * 1. Install SSH wrapper script on first launch or openssh package update
 * 2. Rename original ssh binary to ssh-real
 * 3. Create ~/.ssh/control/ directory with proper permissions
 * 4. Provide API to check active SSH connections
 *
 * Design: Silent failure - installation errors don't block app startup.
 */
public class SSHControlMasterInstaller {

    private static final String LOG_TAG = "SSHControlMasterInstaller";

    /** Handler for polling mechanism */
    private static Handler sPollingHandler = null;

    /** Runnable for polling check */
    private static Runnable sPollingRunnable = null;

    /** Context reference for polling callbacks - set when starting watch */
    private static Context sApplicationContext = null;

    /** Polling interval in milliseconds */
    private static final long POLLING_INTERVAL_MS = 2000; // 2 seconds

    /** SSH binary path in Termux prefix */
    private static final String SSH_BINARY_PATH = TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + "/ssh";

    /** Renamed original SSH binary path */
    private static final String SSH_REAL_BINARY_PATH = TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + "/ssh-real";

    /** SSH wrapper script asset name */
    private static final String SSH_WRAPPER_ASSET_NAME = "termux-ssh-wrapper.sh";

    /** SSH wrapper script installation path */
    private static final String SSH_WRAPPER_PATH = TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + "/termux-ssh-wrapper.sh";

    /** SSH wrapper symlink path (acts as the new ssh command) */
    private static final String SSH_WRAPPER_SYMLINK_PATH = SSH_BINARY_PATH;

    /** Control socket directory */
    private static final File CONTROL_DIR = TermuxConstants.TERMUX_SSH_CONTROL_DIR;

    /** Marker file to track installation version */
    private static final String INSTALL_MARKER_PATH = TermuxConstants.TERMUX_HOME_DIR_PATH + "/.termux/ssh-control-installed";

    /** Current installation version (increment when wrapper script changes) */
    private static final int INSTALL_VERSION = 1;

    /**
     * Install SSH ControlMaster wrapper and setup control directory.
     * Called from TermuxApplication.onCreate() during app initialization.
     *
     * @param context Application context
     * @return true if installation succeeded or already installed, false on failure
     */
    public static boolean install(Context context) {
        try {
            // Check if openssh package is installed (ssh binary exists)
            File sshBinary = new File(SSH_BINARY_PATH);
            if (!sshBinary.exists()) {
                Logger.logInfo(LOG_TAG, "openssh not installed yet, skipping SSH ControlMaster setup");
                return true; // Not an error - openssh may be installed later
            }

            // Check if already installed with current version
            if (isAlreadyInstalled()) {
                Logger.logInfo(LOG_TAG, "SSH ControlMaster already installed with version " + INSTALL_VERSION);
                ensureControlDirectoryExists();
                return true;
            }

            Logger.logInfo(LOG_TAG, "Installing SSH ControlMaster wrapper...");

            // Step 1: Create control directory
            if (!ensureControlDirectoryExists()) {
                return false;
            }

            // Step 2: Rename original ssh to ssh-real if not already done
            if (!renameOriginalSSH()) {
                return false;
            }

            // Step 3: Install wrapper script from assets
            if (!installWrapperScript(context)) {
                return false;
            }

            // Step 4: Create symlink from ssh to wrapper
            if (!createWrapperSymlink()) {
                return false;
            }

            // Step 5: Mark installation complete
            markInstallationComplete();

            Logger.logInfo(LOG_TAG, "SSH ControlMaster installation complete");
            return true;

        } catch (Exception e) {
            Logger.logErrorExtended(LOG_TAG, "SSH ControlMaster installation failed: " + e.getMessage());
            return false;
        }
    }

    /**
     * Check if SSH ControlMaster is already installed with current version.
     * Validates both marker file AND symlink status to detect openssh updates.
     */
    private static boolean isAlreadyInstalled() {
        File marker = new File(INSTALL_MARKER_PATH);
        if (!marker.exists()) {
            return false;
        }

        // Validate marker version
        try {
            StringBuilder sb = new StringBuilder();
            Error error = FileUtils.readTextFromFile("SSH marker", marker.getAbsolutePath(),
                java.nio.charset.StandardCharsets.UTF_8, sb, false);
            if (error != null) {
                return false;
            }
            int version = Integer.parseInt(sb.toString().trim());
            if (version < INSTALL_VERSION) {
                return false;
            }
        } catch (Exception e) {
            return false;
        }

        // CRITICAL: Validate symlink status - openssh update can overwrite symlink with binary
        File sshBinary = new File(SSH_BINARY_PATH);
        if (!sshBinary.exists()) {
            return false;  // ssh binary doesn't exist at all
        }

        // Check if ssh is a symlink (not an ELF binary from openssh update)
        try {
            android.system.StructStat stat = Os.lstat(sshBinary.getAbsolutePath());
            int fileType = stat.st_mode & 0170000;  // S_IFMT in octal
            int symlinkType = 0120000;  // S_IFLNK in octal
            
            if (fileType != symlinkType) {
                Logger.logWarn(LOG_TAG, "ssh exists but is not a symlink (likely openssh update), triggering reinstall");
                return false;  // ssh is a regular file/executable, not our symlink
            }

            // Verify symlink points to the correct wrapper
            String linkTarget = Os.readlink(sshBinary.getAbsolutePath());
            if (!SSH_WRAPPER_PATH.equals(linkTarget)) {
                Logger.logWarn(LOG_TAG, "ssh symlink points to wrong target: " + linkTarget + ", triggering reinstall");
                return false;
            }
        } catch (Exception e) {
            Logger.logWarn(LOG_TAG, "Failed to validate ssh symlink status: " + e.getMessage());
            return false;
        }

        // All validations passed - wrapper is correctly installed
        return true;
    }

    /**
     * Mark installation as complete by writing version marker.
     */
    private static void markInstallationComplete() {
        try {
            File markerDir = new File(INSTALL_MARKER_PATH).getParentFile();
            if (markerDir != null && !markerDir.exists()) {
                FileUtils.createDirectoryFile(markerDir.getAbsolutePath());
            }
            Error error = FileUtils.writeTextToFile("SSH marker", INSTALL_MARKER_PATH,
                java.nio.charset.StandardCharsets.UTF_8, String.valueOf(INSTALL_VERSION), false);
            if (error != null) {
                Logger.logErrorExtended(LOG_TAG, "Failed to write installation marker: " + error);
            }
        } catch (Exception e) {
            Logger.logErrorExtended(LOG_TAG, "Failed to write installation marker: " + e.getMessage());
        }
    }

    /**
     * Create ~/.ssh/control/ directory with proper permissions (700).
     */
    private static boolean ensureControlDirectoryExists() {
        try {
            // Create ~/.ssh parent directory first
            File sshDir = new File(TermuxConstants.TERMUX_HOME_DIR_PATH, ".ssh");
            if (!sshDir.exists()) {
                Error error = FileUtils.createDirectoryFile(sshDir.getAbsolutePath());
                if (error != null) {
                    Logger.logErrorExtended(LOG_TAG, "Failed to create ~/.ssh directory: " + error);
                    return false;
                }
                Os.chmod(sshDir.getAbsolutePath(), 0700);
            }

            // Create control directory
            if (!CONTROL_DIR.exists()) {
                Error error = FileUtils.createDirectoryFile(CONTROL_DIR.getAbsolutePath());
                if (error != null) {
                    Logger.logErrorExtended(LOG_TAG, "Failed to create control directory: " + error);
                    return false;
                }
                Os.chmod(CONTROL_DIR.getAbsolutePath(), 0700);
            }

            return true;
        } catch (Exception e) {
            Logger.logErrorExtended(LOG_TAG, "Failed to setup control directory: " + e.getMessage());
            return false;
        }
    }

    /**
     * Rename original ssh binary to ssh-real.
     */
    private static boolean renameOriginalSSH() {
        try {
            File sshReal = new File(SSH_REAL_BINARY_PATH);
            File sshBinary = new File(SSH_BINARY_PATH);

            // If ssh-real already exists, we're done
            if (sshReal.exists()) {
                Logger.logDebug(LOG_TAG, "ssh-real already exists");
                return true;
            }

            // If ssh binary doesn't exist (openssh not installed), skip
            if (!sshBinary.exists()) {
                Logger.logDebug(LOG_TAG, "ssh binary not found, skipping rename");
                return true;
            }

            // Rename ssh to ssh-real
            Error error = FileUtils.moveFile("ssh binary", SSH_BINARY_PATH, SSH_REAL_BINARY_PATH, false);
            if (error != null) {
                Logger.logErrorExtended(LOG_TAG, "Failed to rename ssh to ssh-real: " + error);
                return false;
            }

            Logger.logInfo(LOG_TAG, "Renamed ssh binary to ssh-real");
            return true;
        } catch (Exception e) {
            Logger.logErrorExtended(LOG_TAG, "Failed to rename ssh binary: " + e.getMessage());
            return false;
        }
    }

    /**
     * Install wrapper script from assets to bin directory.
     */
    private static boolean installWrapperScript(Context context) {
        try {
            // Copy wrapper script from assets
            InputStream is = context.getAssets().open(SSH_WRAPPER_ASSET_NAME);
            File wrapperFile = new File(SSH_WRAPPER_PATH);

            // Write to bin directory
            FileOutputStream fos = new FileOutputStream(wrapperFile);
            byte[] buffer = new byte[4096];
            int bytesRead;
            while ((bytesRead = is.read(buffer)) != -1) {
                fos.write(buffer, 0, bytesRead);
            }
            fos.close();
            is.close();

            // Set executable permissions (755)
            Os.chmod(wrapperFile.getAbsolutePath(), 0755);

            Logger.logInfo(LOG_TAG, "Installed SSH wrapper script to " + SSH_WRAPPER_PATH);
            return true;
        } catch (IOException e) {
            Logger.logErrorExtended(LOG_TAG, "Failed to install wrapper script: " + e.getMessage());
            return false;
        } catch (Exception e) {
            Logger.logErrorExtended(LOG_TAG, "Failed to set wrapper permissions: " + e.getMessage());
            return false;
        }
    }

    /**
     * Create symlink from ssh to wrapper script.
     */
    private static boolean createWrapperSymlink() {
        try {
            File symlink = new File(SSH_WRAPPER_SYMLINK_PATH);

            // Remove existing symlink if it points elsewhere
            if (symlink.exists()) {
                Error error = FileUtils.deleteFile("ssh symlink", symlink.getAbsolutePath(), false);
                if (error != null) {
                    Logger.logErrorExtended(LOG_TAG, "Failed to remove existing ssh: " + error);
                    return false;
                }
            }

            // Create symlink
            Os.symlink(SSH_WRAPPER_PATH, SSH_WRAPPER_SYMLINK_PATH);

            Logger.logInfo(LOG_TAG, "Created symlink: ssh -> termux-ssh-wrapper.sh");
            return true;
        } catch (Exception e) {
            Logger.logErrorExtended(LOG_TAG, "Failed to create wrapper symlink: " + e.getMessage());
            return false;
        }
    }

    /**
     * Check if a control socket exists for a given host connection.
     * Socket naming pattern: %r@%h:%p (user@host:port)
     *
     * @param host Host name or IP
     * @param port Port number (default SSH port is 22)
     * @param user Username
     * @return true if control socket exists and is active
     */
    public static boolean checkControlMasterSocket(String host, int port, String user) {
        if (!CONTROL_DIR.exists()) {
            return false;
        }

        // Build socket path based on pattern: user@host:port
        String socketName = user + "@" + host + ":" + port;
        File socketFile = new File(CONTROL_DIR, socketName);

        return socketFile.exists() && socketFile.canRead() && socketFile.canWrite();
    }

    /**
     * Check if SSH ControlMaster is installed and functional.
     *
     * @return true if installation is complete
     */
    public static boolean isInstalled() {
        File sshReal = new File(SSH_REAL_BINARY_PATH);
        File wrapper = new File(SSH_WRAPPER_PATH);
        File sshLink = new File(SSH_BINARY_PATH);

        return sshReal.exists() && wrapper.exists() && sshLink.exists();
    }

    /**
     * Force reinstall SSH ControlMaster (used when openssh package is updated).
     *
     * @param context Application context
     * @return true if reinstall succeeded
     */
    public static boolean reinstall(Context context) {
        // Remove installation marker to trigger reinstall
        File marker = new File(INSTALL_MARKER_PATH);
        if (marker.exists()) {
            FileUtils.deleteFile("SSH marker", marker.getAbsolutePath(), false);
        }

        // Remove existing symlink
        File sshLink = new File(SSH_WRAPPER_SYMLINK_PATH);
        if (sshLink.exists()) {
            FileUtils.deleteFile("ssh symlink", sshLink.getAbsolutePath(), false);
        }

        return install(context);
    }

    /**
     * Get list of active SSH connections by scanning control socket directory.
     *
     * Scans ~/.ssh/control/ directory for Unix socket files, parses filenames
     * using pattern user@host:port, and returns list of SSHConnectionInfo objects.
     *
     * @return List of SSHConnectionInfo for active connections, empty list if none
     */
    public static java.util.List<SSHConnectionInfo> getActiveConnections() {
        java.util.List<SSHConnectionInfo> connections = new java.util.ArrayList<>();

        if (!CONTROL_DIR.exists()) {
            Logger.logDebug(LOG_TAG, "Control directory does not exist: " + CONTROL_DIR.getAbsolutePath());
            return connections;
        }

        File[] files = CONTROL_DIR.listFiles();
        if (files == null || files.length == 0) {
            Logger.logDebug(LOG_TAG, "No files found in control directory");
            return connections;
        }

        Logger.logDebug(LOG_TAG, "Scanning control directory, found " + files.length + " files");

        int parsedCount = 0;
        int skippedCount = 0;

        for (File file : files) {
            String filename = file.getName();
            String path = file.getAbsolutePath();

            // Check if file is a Unix socket (character device with specific mode on Android/Linux)
            // Unix sockets have file type indicator 's' in stat output
            // On Android, we can check using Os.stat() to get file mode
            boolean isSocket = isUnixSocket(file);

            if (!isSocket) {
                Logger.logDebug(LOG_TAG, "Skipping non-socket file: " + filename);
                skippedCount++;
                continue;
            }

            // Parse filename: user@host:port
            SSHConnectionInfo info = SSHConnectionInfo.parseFromFilename(filename, path);

            if (info == null) {
                Logger.logDebug(LOG_TAG, "Failed to parse socket filename: " + filename);
                skippedCount++;
                continue;
            }

            connections.add(info);
            parsedCount++;
            Logger.logDebug(LOG_TAG, "Parsed active connection: " + info.toString() + " from " + filename);
        }

        Logger.logDebug(LOG_TAG, "Scan complete: " + parsedCount + " connections found, " + skippedCount + " files skipped, returning " + connections.size() + " items");

        return connections;
    }

    /**
     * Check if a file is a Unix socket.
     *
     * Uses Os.stat() to check file mode for Unix socket type.
     * Unix sockets have S_IFSOCK (0xC000) in their mode bits.
     *
     * @param file File to check
     * @return true if file is a Unix socket, false otherwise
     */
    private static boolean isUnixSocket(File file) {
        if (!file.exists()) {
            return false;
        }

        try {
            // Os.stat() returns struct stat with st_mode field
            // S_IFSOCK = 0xC000 (49192 in decimal) - socket file type mask
            // Use OsConstants.S_IFSOCK when available
            android.system.StructStat stat = Os.stat(file.getAbsolutePath());
            int mode = stat.st_mode;

            // Check if file type is socket: (mode & S_IFMT) == S_IFSOCK
            // S_IFMT = 0xF000 (61440) - file type mask
            // S_IFSOCK = 0xC000 (49192) - socket type
            int fileType = mode & 0170000; // S_IFMT in octal
            int socketType = 0140000;     // S_IFSOCK in octal

            return fileType == socketType;
        } catch (Exception e) {
            Logger.logDebug(LOG_TAG, "Failed to stat file " + file.getName() + ": " + e.getMessage());
            return false;
        }
    }

    /**
     * Start polling-based monitoring for SSH binary creation.
     * If ssh binary already exists, install immediately and skip polling.
     * Otherwise, start periodic polling every 2 seconds to check for ssh binary.
     *
     * This replaces the unreliable FileObserver with a robust polling mechanism.
     *
     * @param context Application context (used for install() call when ssh is detected)
     */
    public static void startWatchingSSHBinary(Context context) {
        if (context == null) {
            Logger.logError(LOG_TAG, "Cannot start SSH binary watcher: null context");
            return;
        }

        sApplicationContext = context.getApplicationContext();

        // Check if ssh already exists - install immediately and skip polling
        File sshBinary = new File(SSH_BINARY_PATH);
        if (sshBinary.exists()) {
            Logger.logInfo(LOG_TAG, "SSH binary already exists, installing wrapper immediately");
            install(sApplicationContext);
            return;
        }

        // Stop any existing polling before starting new one
        stopWatchingSSHBinary();

        Logger.logInfo(LOG_TAG, "Starting polling for SSH binary creation at " + SSH_BINARY_PATH + 
            " (interval: " + POLLING_INTERVAL_MS + "ms)");

        // Create Handler on main looper
        sPollingHandler = new Handler(Looper.getMainLooper());

        // Create polling Runnable
        sPollingRunnable = new Runnable() {
            @Override
            public void run() {
                File ssh = new File(SSH_BINARY_PATH);
                
                if (ssh.exists()) {
                    Logger.logInfo(LOG_TAG, "Detected ssh binary creation via polling, triggering wrapper installation");
                    
                    // Stop polling before install to avoid recursion
                    stopWatchingSSHBinary();
                    
                    // Install wrapper
                    if (sApplicationContext != null) {
                        boolean success = install(sApplicationContext);
                        if (success) {
                            Logger.logInfo(LOG_TAG, "SSH ControlMaster wrapper installed successfully after ssh binary detection");
                        } else {
                            Logger.logError(LOG_TAG, "SSH ControlMaster wrapper installation failed after ssh binary detection");
                        }
                    }
                } else {
                    // ssh not found yet, schedule next poll
                    if (sPollingHandler != null && sPollingRunnable != null) {
                        sPollingHandler.postDelayed(sPollingRunnable, POLLING_INTERVAL_MS);
                    }
                }
            }
        };

        // Start polling immediately
        sPollingHandler.post(sPollingRunnable);
        Logger.logInfo(LOG_TAG, "Polling started, checking for ssh binary every " + POLLING_INTERVAL_MS + "ms");
    }

    /**
     * Stop polling for SSH binary creation and release resources.
     * Safe to call multiple times - checks if polling is active before stopping.
     */
    public static void stopWatchingSSHBinary() {
        if (sPollingHandler != null && sPollingRunnable != null) {
            Logger.logInfo(LOG_TAG, "Stopping SSH binary polling");
            sPollingHandler.removeCallbacks(sPollingRunnable);
        }
        sPollingHandler = null;
        sPollingRunnable = null;
    }

    /**
     * Check if polling is actively watching for SSH binary creation.
     *
     * @return true if polling is active, false otherwise
     */
    public static boolean isWatchingSSHBinary() {
        return sPollingHandler != null && sPollingRunnable != null;
    }
}