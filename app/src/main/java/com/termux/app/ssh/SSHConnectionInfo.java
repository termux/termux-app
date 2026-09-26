package com.termux.app.ssh;

import java.io.Serializable;

/**
 * Data class representing an active SSH connection via ControlMaster.
 *
 * Stores connection details parsed from control socket filename.
 * Socket naming pattern: user@host:port
 *
 * Implements Serializable for passing via Intent extras.
 */
public class SSHConnectionInfo implements Serializable {

    private static final long serialVersionUID = 1L;

    /** Username for SSH connection */
    private final String user;

    /** Host name or IP address */
    private final String host;

    /** Port number (default SSH port is 22) */
    private final int port;

    /** Path to the control socket file */
    private final String socketPath;

    /**
     * Create SSHConnectionInfo from parsed socket filename.
     *
     * @param user Username
     * @param host Host name or IP
     * @param port Port number
     * @param socketPath Full path to control socket
     */
    public SSHConnectionInfo(String user, String host, int port, String socketPath) {
        this.user = user;
        this.host = host;
        this.port = port;
        this.socketPath = socketPath;
    }

    /**
     * Get username.
     *
     * @return Username for SSH connection
     */
    public String getUser() {
        return user;
    }

    /**
     * Get host name.
     *
     * @return Host name or IP address
     */
    public String getHost() {
        return host;
    }

    /**
     * Get port number.
     *
     * @return Port number
     */
    public int getPort() {
        return port;
    }

    /**
     * Get control socket path.
     *
     * @return Full path to control socket file
     */
    public String getSocketPath() {
        return socketPath;
    }

    /**
     * Format connection info as user@host:port string.
     * This matches the SSH ControlMaster socket naming convention.
     *
     * @return Formatted connection string
     */
    @Override
    public String toString() {
        return user + "@" + host + ":" + port;
    }

    /**
     * Parse socket filename to create SSHConnectionInfo.
     * Expected format: user@host:port
     *
     * @param socketFilename Socket filename without path
     * @param socketPath Full path to socket file
     * @return SSHConnectionInfo if parsing succeeds, null if format is invalid
     */
    public static SSHConnectionInfo parseFromFilename(String socketFilename, String socketPath) {
        if (socketFilename == null || socketFilename.isEmpty()) {
            return null;
        }

        // Format: user@host:port
        // Find @ separator for user
        int atIndex = socketFilename.indexOf('@');
        if (atIndex <= 0) {
            return null; // No @ or @ at start (empty user)
        }

        // Find : separator for port
        int colonIndex = socketFilename.lastIndexOf(':');
        if (colonIndex <= atIndex + 1) {
            return null; // No : after @ or : immediately after @ (empty host)
        }

        try {
            String user = socketFilename.substring(0, atIndex);
            String host = socketFilename.substring(atIndex + 1, colonIndex);
            String portStr = socketFilename.substring(colonIndex + 1);

            // Validate that port is a number
            int port = Integer.parseInt(portStr);

            // Basic validation: port should be valid TCP port range
            if (port < 1 || port > 65535) {
                return null;
            }

            // Validate user and host are not empty
            if (user.isEmpty() || host.isEmpty()) {
                return null;
            }

            return new SSHConnectionInfo(user, host, port, socketPath);
        } catch (NumberFormatException e) {
            return null; // Port is not a valid integer
        }
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) return true;
        if (obj == null || getClass() != obj.getClass()) return false;

        SSHConnectionInfo other = (SSHConnectionInfo) obj;
        return port == other.port &&
               user.equals(other.user) &&
               host.equals(other.host);
    }

    @Override
    public int hashCode() {
        int result = user.hashCode();
        result = 31 * result + host.hashCode();
        result = 31 * result + port;
        return result;
    }
}