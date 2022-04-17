package com.termux.shared.net.socket.local;

import com.termux.shared.errors.Errno;

public class LocalSocketErrno extends Errno {

    public static final String TYPE = "LocalSocket Error";


    /** Errors for {@link LocalSocketManager} (100-150) */
    public static final Errno ERRNO_START_LOCAL_SOCKET_LIB_LOAD_FAILED_WITH_EXCEPTION = new Errno(TYPE, 100, "Failed to load \"%1$s\" library.\nException: %2$s");

    /** Errors for {@link LocalServerSocket} (150-200) */
    public static final Errno ERRNO_SERVER_SOCKET_PATH_NULL_OR_EMPTY = new Errno(TYPE, 150, "The \"%1$s\" server socket path is null or empty.");
    public static final Errno ERRNO_SERVER_SOCKET_PATH_TOO_LONG = new Errno(TYPE, 151, "The \"%1$s\" server socket path \"%2$s\" is greater than 108 bytes.");
    public static final Errno ERRNO_SERVER_SOCKET_PATH_NOT_ABSOLUTE = new Errno(TYPE, 152, "The \"%1$s\" server socket path \"%2$s\" is not an absolute file path.");
    public static final Errno ERRNO_SERVER_SOCKET_BACKLOG_INVALID = new Errno(TYPE, 153, "The \"%1$s\" server socket backlog \"%2$s\" is not greater than 0.");
    public static final Errno ERRNO_CREATE_SERVER_SOCKET_FAILED = new Errno(TYPE, 154, "Create \"%1$s\" server socket failed.\n%2$s");
    public static final Errno ERRNO_SERVER_SOCKET_FD_INVALID = new Errno(TYPE, 155, "Invalid file descriptor \"%1$s\" returned when creating \"%2$s\" server socket.");
    public static final Errno ERRNO_ACCEPT_CLIENT_SOCKET_FAILED = new Errno(TYPE, 156, "Accepting client socket for \"%1$s\" server failed.\n%2$s");
    public static final Errno ERRNO_CLIENT_SOCKET_FD_INVALID = new Errno(TYPE, 157, "Invalid file descriptor \"%1$s\" returned when accept new client for \"%2$s\" server.");
    public static final Errno ERRNO_GET_CLIENT_SOCKET_PEER_UID_FAILED = new Errno(TYPE, 158, "Getting peer uid for client socket for \"%1$s\" server failed.\n%2$s");
    public static final Errno ERRNO_CLIENT_SOCKET_PEER_UID_INVALID = new Errno(TYPE, 158, "Invalid peer uid \"%1$s\" returned for new client for \"%2$s\" server.");
    public static final Errno ERRNO_CLIENT_SOCKET_PEER_UID_DISALLOWED = new Errno(TYPE, 160, "Disallowed peer %1$s tried to connect with \"%2$s\" server.");
    public static final Errno ERRNO_CLOSE_SERVER_SOCKET_FAILED_WITH_EXCEPTION = new Errno(TYPE, 161, "Close \"%1$s\" server socket failed.\nException: %2$s");
    public static final Errno ERRNO_CLIENT_SOCKET_LISTENER_FAILED_WITH_EXCEPTION = new Errno(TYPE, 162, "Exception in client socket listener for \"%1$s\" server.\nException: %2$s");

    /** Errors for {@link LocalClientSocket} (200-250) */
    public static final Errno ERRNO_SET_CLIENT_SOCKET_READ_TIMEOUT_FAILED = new Errno(TYPE, 200, "Set \"%1$s\" client socket read (SO_RCVTIMEO) timeout to \"%2$s\" failed.\n%3$s");
    public static final Errno ERRNO_SET_CLIENT_SOCKET_SEND_TIMEOUT_FAILED = new Errno(TYPE, 201, "Set \"%1$s\" client socket send (SO_SNDTIMEO) timeout \"%2$s\" failed.\n%3$s");
    public static final Errno ERRNO_READ_DATA_FROM_CLIENT_SOCKET_FAILED = new Errno(TYPE, 202, "Read data from \"%1$s\" client socket failed.\n%2$s");
    public static final Errno ERRNO_READ_DATA_FROM_INPUT_STREAM_OF_CLIENT_SOCKET_FAILED_WITH_EXCEPTION = new Errno(TYPE, 203, "Read data from \"%1$s\" client socket input stream failed.\n%2$s");
    public static final Errno ERRNO_SEND_DATA_TO_CLIENT_SOCKET_FAILED = new Errno(TYPE, 204, "Send data to \"%1$s\" client socket failed.\n%2$s");
    public static final Errno ERRNO_SEND_DATA_TO_OUTPUT_STREAM_OF_CLIENT_SOCKET_FAILED_WITH_EXCEPTION = new Errno(TYPE, 205, "Send data to \"%1$s\" client socket output stream failed.\n%2$s");
    public static final Errno ERRNO_CHECK_AVAILABLE_DATA_ON_CLIENT_SOCKET_FAILED = new Errno(TYPE, 206, "Check available data on \"%1$s\" client socket failed.\n%2$s");
    public static final Errno ERRNO_CLOSE_CLIENT_SOCKET_FAILED_WITH_EXCEPTION = new Errno(TYPE, 207, "Close \"%1$s\" client socket failed.\n%2$s");
    public static final Errno ERRNO_USING_CLIENT_SOCKET_WITH_INVALID_FD = new Errno(TYPE, 208, "Trying to use client socket with invalid file descriptor \"%1$s\" for \"%2$s\" server.");

    LocalSocketErrno(final String type, final int code, final String message) {
        super(type, code, message);
    }

}
