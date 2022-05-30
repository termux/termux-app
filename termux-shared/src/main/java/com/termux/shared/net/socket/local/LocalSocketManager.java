package com.termux.shared.net.socket.local;

import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.errors.Error;
import com.termux.shared.jni.models.JniResult;
import com.termux.shared.logger.Logger;

/**
 * Manager for an AF_UNIX/SOCK_STREAM local server.
 *
 * Usage:
 * 1. Implement the {@link ILocalSocketManager} that will receive call backs from the server including
 *    when client connects via {@link ILocalSocketManager#onClientAccepted(LocalSocketManager, LocalClientSocket)}.
 *    Optionally extend the {@link LocalSocketManagerClientBase} class that provides base implementation.
 * 2. Create a {@link LocalSocketRunConfig} instance with the run config of the server.
 * 3. Create a {@link LocalSocketManager} instance and call {@link #start()}.
 * 4. Stop server if needed with a call to {@link #stop()}.
 */
public class LocalSocketManager {

    public static final String LOG_TAG = "LocalSocketManager";

    /** The native JNI local socket library. */
    protected static String LOCAL_SOCKET_LIBRARY = "local-socket";

    /** Whether {@link #LOCAL_SOCKET_LIBRARY} has been loaded or not. */
    protected static boolean localSocketLibraryLoaded;

    /** The {@link Context} that may needed for various operations. */
    @NonNull protected final Context mContext;

    /** The {@link LocalSocketRunConfig} containing run config for the {@link LocalSocketManager}. */
    @NonNull protected final LocalSocketRunConfig mLocalSocketRunConfig;

    /** The {@link LocalServerSocket} for the {@link LocalSocketManager}. */
    @NonNull protected final LocalServerSocket mServerSocket;

    /** The {@link ILocalSocketManager} client for the {@link LocalSocketManager}. */
    @NonNull protected final ILocalSocketManager mLocalSocketManagerClient;

    /** The {@link Thread.UncaughtExceptionHandler} used for client thread started by {@link LocalSocketManager}. */
    @NonNull protected final Thread.UncaughtExceptionHandler mLocalSocketManagerClientThreadUEH;

    /** Whether the {@link LocalServerSocket} managed by {@link LocalSocketManager} in running or not. */
    protected boolean mIsRunning;


    /**
     * Create an new instance of {@link LocalSocketManager}.
     *
     * @param context The {@link #mContext} value.
     * @param localSocketRunConfig The {@link #mLocalSocketRunConfig} value.
     */
    public LocalSocketManager(@NonNull Context context, @NonNull LocalSocketRunConfig localSocketRunConfig) {
        mContext = context.getApplicationContext();
        mLocalSocketRunConfig = localSocketRunConfig;
        mServerSocket = new LocalServerSocket(this);
        mLocalSocketManagerClient = mLocalSocketRunConfig.getLocalSocketManagerClient();
        mLocalSocketManagerClientThreadUEH = getLocalSocketManagerClientThreadUEHOrDefault();
        mIsRunning = false;
    }

    /**
     * Create the {@link LocalServerSocket} and start listening for new {@link LocalClientSocket}.
     */
    public synchronized Error start() {
        Logger.logDebugExtended(LOG_TAG, "start\n" + mLocalSocketRunConfig);

        if (!localSocketLibraryLoaded) {
            try {
                Logger.logDebug(LOG_TAG, "Loading \"" + LOCAL_SOCKET_LIBRARY + "\" library");
                System.loadLibrary(LOCAL_SOCKET_LIBRARY);
                localSocketLibraryLoaded = true;
            } catch (Throwable t) {
                Error error = LocalSocketErrno.ERRNO_START_LOCAL_SOCKET_LIB_LOAD_FAILED_WITH_EXCEPTION.getError(t, LOCAL_SOCKET_LIBRARY,  t.getMessage());
                Logger.logErrorExtended(LOG_TAG, error.getErrorLogString());
                return error;
            }
        }

        mIsRunning = true;
        return mServerSocket.start();
    }

    /**
     * Stop the {@link LocalServerSocket} and stop listening for new {@link LocalClientSocket}.
     */
    public synchronized Error stop() {
        if (mIsRunning) {
            Logger.logDebugExtended(LOG_TAG, "stop\n" + mLocalSocketRunConfig);
            mIsRunning = false;
            return mServerSocket.stop();
        }
        return null;
    }




    /*
     Note: Exceptions thrown from JNI must be caught with Throwable class instead of Exception,
     otherwise exception will be sent to UncaughtExceptionHandler of the thread.
    */

    /**
     * Creates an AF_UNIX/SOCK_STREAM local server socket at {@code path}, with the specified backlog.
     *
     * @param serverTitle The server title used for logging and errors.
     * @param path The path at which to create the socket.
     *             For a filesystem socket, this must be an absolute path to the socket file.
     *             For an abstract namespace socket, the first byte must be a null `\0` character.
     *             Max allowed length is 108 bytes as per sun_path size (UNIX_PATH_MAX) on Linux.
     * @param backlog The maximum length to which the queue of pending connections for the socket
     *                may grow. This value may be ignored or may not have one-to-one mapping
     *                in kernel implementation. Value must be greater than 0.
     * @return Returns the {@link JniResult}. If server creation was successful, then
     * {@link JniResult#retval} will be 0 and {@link JniResult#intData} will contain the server socket
     * fd.
     */
    @Nullable
    public static JniResult createServerSocket(@NonNull String serverTitle, @NonNull byte[] path, int backlog) {
        try {
            return createServerSocketNative(serverTitle, path, backlog);
        } catch (Throwable t) {
            String message = "Exception in createServerSocketNative()";
            Logger.logStackTraceWithMessage(LOG_TAG, message, t);
            return new JniResult(message, t);
        }
    }

    /**
     * Closes the socket with fd.
     *
     * @param serverTitle The server title used for logging and errors.
     * @param fd The socket fd.
     * @return Returns the {@link JniResult}. If closing socket was successful, then
     * {@link JniResult#retval} will be 0.
     */
    @Nullable
    public static JniResult closeSocket(@NonNull String serverTitle, int fd) {
        try {
            return closeSocketNative(serverTitle, fd);
        } catch (Throwable t) {
            String message = "Exception in closeSocketNative()";
            Logger.logStackTraceWithMessage(LOG_TAG, message, t);
            return new JniResult(message, t);
        }
    }

    /**
     * Accepts a connection on the supplied server socket fd.
     *
     * @param serverTitle The server title used for logging and errors.
     * @param fd The server socket fd.
     * @return Returns the {@link JniResult}. If accepting socket was successful, then
     * {@link JniResult#retval} will be 0 and {@link JniResult#intData} will contain the client socket
     * fd.
     */
    @Nullable
    public static JniResult accept(@NonNull String serverTitle, int fd) {
        try {
            return acceptNative(serverTitle, fd);
        } catch (Throwable t) {
            String message = "Exception in acceptNative()";
            Logger.logStackTraceWithMessage(LOG_TAG, message, t);
            return new JniResult(message, t);
        }
    }

    /**
     * Attempts to read up to data buffer length bytes from file descriptor fd into the data buffer.
     * On success, the number of bytes read is returned (zero indicates end of file).
     * It is not an error if bytes read is smaller than the number of bytes requested; this may happen
     * for example because fewer bytes are actually available right now (maybe because we were close
     * to end-of-file, or because we are reading from a pipe), or because read() was interrupted by
     * a signal. On error, the {@link JniResult#errno} and {@link JniResult#errmsg} will be set.
     *
     * If while reading the deadline elapses but all the data has not been read, the call will fail.
     *
     * @param serverTitle The server title used for logging and errors.
     * @param fd The socket fd.
     * @param data The data buffer to read bytes into.
     * @param deadline The deadline milliseconds since epoch.
     * @return Returns the {@link JniResult}. If reading was successful, then {@link JniResult#retval}
     * will be 0 and {@link JniResult#intData} will contain the bytes read.
     */
    @Nullable
    public static JniResult read(@NonNull String serverTitle, int fd, @NonNull byte[] data, long deadline) {
        try {
            return readNative(serverTitle, fd, data, deadline);
        } catch (Throwable t) {
            String message = "Exception in readNative()";
            Logger.logStackTraceWithMessage(LOG_TAG, message, t);
            return new JniResult(message, t);
        }
    }

    /**
     * Attempts to send data buffer to the file descriptor. On error, the {@link JniResult#errno} and
     * {@link JniResult#errmsg} will be set.
     *
     * If while sending the deadline elapses but all the data has not been sent, the call will fail.
     *
     * @param serverTitle The server title used for logging and errors.
     * @param fd The socket fd.
     * @param data The data buffer containing bytes to send.
     * @param deadline The deadline milliseconds since epoch.
     * @return Returns the {@link JniResult}. If sending was successful, then {@link JniResult#retval}
     * will be 0.
     */
    @Nullable
    public static JniResult send(@NonNull String serverTitle, int fd, @NonNull byte[] data, long deadline) {
        try {
            return sendNative(serverTitle, fd, data, deadline);
        } catch (Throwable t) {
            String message = "Exception in sendNative()";
            Logger.logStackTraceWithMessage(LOG_TAG, message, t);
            return new JniResult(message, t);
        }
    }

    /**
     * Gets the number of bytes available to read on the socket.
     *
     * @param serverTitle The server title used for logging and errors.
     * @param fd The socket fd.
     * @return Returns the {@link JniResult}. If checking availability was successful, then
     * {@link JniResult#retval} will be 0 and {@link JniResult#intData} will contain the bytes available.
     */
    @Nullable
    public static JniResult available(@NonNull String serverTitle, int fd) {
        try {
            return availableNative(serverTitle, fd);
        } catch (Throwable t) {
            String message = "Exception in availableNative()";
            Logger.logStackTraceWithMessage(LOG_TAG, message, t);
            return new JniResult(message, t);
        }
    }

    /**
     * Set receiving (SO_RCVTIMEO) timeout in milliseconds for socket.
     *
     * @param serverTitle The server title used for logging and errors.
     * @param fd The socket fd.
     * @param timeout The timeout value in milliseconds.
     * @return Returns the {@link JniResult}. If setting timeout was successful, then
     * {@link JniResult#retval} will be 0.
     */
    @Nullable
    public static JniResult setSocketReadTimeout(@NonNull String serverTitle, int fd, int timeout) {
        try {
            return setSocketReadTimeoutNative(serverTitle, fd, timeout);
        } catch (Throwable t) {
            String message = "Exception in setSocketReadTimeoutNative()";
            Logger.logStackTraceWithMessage(LOG_TAG, message, t);
            return new JniResult(message, t);
        }
    }

    /**
     * Set sending (SO_SNDTIMEO) timeout in milliseconds for fd.
     *
     * @param serverTitle The server title used for logging and errors.
     * @param fd The socket fd.
     * @param timeout The timeout value in milliseconds.
     * @return Returns the {@link JniResult}. If setting timeout was successful, then
     * {@link JniResult#retval} will be 0.
     */
    @Nullable
    public static JniResult setSocketSendTimeout(@NonNull String serverTitle, int fd, int timeout) {
        try {
            return setSocketSendTimeoutNative(serverTitle, fd, timeout);
        } catch (Throwable t) {
            String message = "Exception in setSocketSendTimeoutNative()";
            Logger.logStackTraceWithMessage(LOG_TAG, message, t);
            return new JniResult(message, t);
        }
    }

    /**
     * Get the {@link PeerCred} for the socket.
     *
     * @param serverTitle The server title used for logging and errors.
     * @param fd The socket fd.
     * @param peerCred The {@link PeerCred} object that should be filled.
     * @return Returns the {@link JniResult}. If setting timeout was successful, then
     * {@link JniResult#retval} will be 0.
     */
    @Nullable
    public static JniResult getPeerCred(@NonNull String serverTitle, int fd, PeerCred peerCred) {
        try {
            return getPeerCredNative(serverTitle, fd, peerCred);
        } catch (Throwable t) {
            String message = "Exception in getPeerCredNative()";
            Logger.logStackTraceWithMessage(LOG_TAG, message, t);
            return new JniResult(message, t);
        }
    }



    /** Wrapper for {@link #onError(LocalClientSocket, Error)} for {@code null} {@link LocalClientSocket}. */
    public void onError(@NonNull Error error) {
        onError(null, error);
    }

    /** Wrapper to call {@link ILocalSocketManager#onError(LocalSocketManager, LocalClientSocket, Error)} in a new thread. */
    public void onError(@Nullable LocalClientSocket clientSocket, @NonNull Error error) {
        startLocalSocketManagerClientThread(() ->
            mLocalSocketManagerClient.onError(this, clientSocket, error));
    }

    /** Wrapper to call {@link ILocalSocketManager#onDisallowedClientConnected(LocalSocketManager, LocalClientSocket, Error)} in a new thread. */
    public void onDisallowedClientConnected(@NonNull LocalClientSocket clientSocket, @NonNull Error error) {
        startLocalSocketManagerClientThread(() ->
            mLocalSocketManagerClient.onDisallowedClientConnected(this, clientSocket, error));
    }

    /** Wrapper to call {@link ILocalSocketManager#onClientAccepted(LocalSocketManager, LocalClientSocket)} in a new thread. */
    public void onClientAccepted(@NonNull LocalClientSocket clientSocket) {
        startLocalSocketManagerClientThread(() ->
            mLocalSocketManagerClient.onClientAccepted(this, clientSocket));
    }

    /** All client accept logic must be run on separate threads so that incoming client acceptance is not blocked. */
    public void startLocalSocketManagerClientThread(@NonNull Runnable runnable) {
        Thread thread = new Thread(runnable);
        thread.setUncaughtExceptionHandler(getLocalSocketManagerClientThreadUEH());
        try {
            thread.start();
        } catch (Exception e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "LocalSocketManagerClientThread start failed", e);
        }
    }



    /** Get {@link #mContext}. */
    public Context getContext() {
        return mContext;
    }

    /** Get {@link #mLocalSocketRunConfig}. */
    public LocalSocketRunConfig getLocalSocketRunConfig() {
        return mLocalSocketRunConfig;
    }

    /** Get {@link #mLocalSocketManagerClient}. */
    public ILocalSocketManager getLocalSocketManagerClient() {
        return mLocalSocketManagerClient;
    }

    /** Get {@link #mServerSocket}. */
    public LocalServerSocket getServerSocket() {
        return mServerSocket;
    }

    /** Get {@link #mLocalSocketManagerClientThreadUEH}. */
    public Thread.UncaughtExceptionHandler getLocalSocketManagerClientThreadUEH() {
        return mLocalSocketManagerClientThreadUEH;
    }

    /**
     * Get {@link Thread.UncaughtExceptionHandler} returned by call to
     * {@link ILocalSocketManager#getLocalSocketManagerClientThreadUEH(LocalSocketManager)}
     * or the default handler that just logs the exception.
     */
    protected Thread.UncaughtExceptionHandler getLocalSocketManagerClientThreadUEHOrDefault() {
        Thread.UncaughtExceptionHandler uncaughtExceptionHandler =
            mLocalSocketManagerClient.getLocalSocketManagerClientThreadUEH(this);
        if (uncaughtExceptionHandler == null)
            uncaughtExceptionHandler = (t, e) ->
                Logger.logStackTraceWithMessage(LOG_TAG, "Uncaught exception for " + t + " in " + mLocalSocketRunConfig.getTitle() + " server", e);
        return uncaughtExceptionHandler;
    }

    /** Get {@link #mIsRunning}. */
    public boolean isRunning() {
        return mIsRunning;
    }



    /** Get an error log {@link String} for the {@link LocalSocketManager}. */
    public static String getErrorLogString(@NonNull Error error,
                                           @NonNull LocalSocketRunConfig localSocketRunConfig,
                                           @Nullable LocalClientSocket clientSocket) {
        StringBuilder logString = new StringBuilder();

        logString.append(localSocketRunConfig.getTitle()).append(" Socket Server Error:\n");
        logString.append(error.getErrorLogString());
        logString.append("\n\n\n");

        logString.append(localSocketRunConfig.getLogString());

        if (clientSocket != null) {
            logString.append("\n\n\n");
            logString.append(clientSocket.getLogString());
        }

        return logString.toString();
    }

    /** Get an error markdown {@link String} for the {@link LocalSocketManager}. */
    public static String getErrorMarkdownString(@NonNull Error error,
                                                @NonNull LocalSocketRunConfig localSocketRunConfig,
                                                @Nullable LocalClientSocket clientSocket) {
        StringBuilder markdownString = new StringBuilder();

        markdownString.append(error.getErrorMarkdownString());
        markdownString.append("\n##\n\n\n");

        markdownString.append(localSocketRunConfig.getMarkdownString());

        if (clientSocket != null) {
            markdownString.append("\n\n\n");
            markdownString.append(clientSocket.getMarkdownString());
        }

        return markdownString.toString();
    }





    @Nullable private static native JniResult createServerSocketNative(@NonNull String serverTitle, @NonNull byte[] path, int backlog);

    @Nullable private static native JniResult closeSocketNative(@NonNull String serverTitle, int fd);

    @Nullable private static native JniResult acceptNative(@NonNull String serverTitle, int fd);

    @Nullable private static native JniResult readNative(@NonNull String serverTitle, int fd, @NonNull byte[] data, long deadline);

    @Nullable private static native JniResult sendNative(@NonNull String serverTitle, int fd, @NonNull byte[] data, long deadline);

    @Nullable private static native JniResult availableNative(@NonNull String serverTitle, int fd);

    private static native JniResult setSocketReadTimeoutNative(@NonNull String serverTitle, int fd, int timeout);

    @Nullable private static native JniResult setSocketSendTimeoutNative(@NonNull String serverTitle, int fd, int timeout);

    @Nullable private static native JniResult getPeerCredNative(@NonNull String serverTitle, int fd, PeerCred peerCred);

}
