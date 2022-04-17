package com.termux.shared.net.socket.local;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.errors.Error;

/**
 * The interface for the {@link LocalSocketManager} for callbacks to manager client/server starter.
 */
public interface ILocalSocketManager {

    /**
     * This should return the {@link Thread.UncaughtExceptionHandler} that should be used for the
     * client socket listener and client logic runner threads started for other interface methods.
     *
     * @param localSocketManager The {@link LocalSocketManager} for the server.
     * @return Should return {@link Thread.UncaughtExceptionHandler} or {@code null}, if default
     * handler should be used which just logs the exception.
     */
    @Nullable
    Thread.UncaughtExceptionHandler getLocalSocketManagerClientThreadUEH(
        @NonNull LocalSocketManager localSocketManager);

    /**
     * This is called if any error is raised by {@link LocalSocketManager}, {@link LocalServerSocket}
     * or {@link LocalClientSocket}. The server will automatically close the client socket
     * with a call to {@link LocalClientSocket#closeClientSocket(boolean)} if the error occurred due
     * to the client.
     *
     * The {@link LocalClientSocket#getPeerCred()} can be used to get the {@link PeerCred} object
     * containing info for the connected client/peer.
     *
     * @param localSocketManager The {@link LocalSocketManager} for the server.
     * @param clientSocket The {@link LocalClientSocket} that connected. This will be {@code null}
     *                     if error is not for a {@link LocalClientSocket}.
     * @param error The {@link Error} auto generated that can be used for logging purposes.
     */
    void onError(@NonNull LocalSocketManager localSocketManager,
                 @Nullable LocalClientSocket clientSocket, @NonNull Error error);

    /**
     * This is called if a {@link LocalServerSocket} connects to the server which **does not** have
     * the server app's user id or root user id. The server will automatically close the client socket
     * with a call to {@link LocalClientSocket#closeClientSocket(boolean)}.
     *
     * The {@link LocalClientSocket#getPeerCred()} can be used to get the {@link PeerCred} object
     * containing info for the connected client/peer.
     *
     * @param localSocketManager The {@link LocalSocketManager} for the server.
     * @param clientSocket The {@link LocalClientSocket} that connected.
     * @param error The {@link Error} auto generated that can be used for logging purposes.
     */
    void onDisallowedClientConnected(@NonNull LocalSocketManager localSocketManager,
                 @NonNull LocalClientSocket clientSocket, @NonNull Error error);

    /**
     * This is called if a {@link LocalServerSocket} connects to the server which has the
     * the server app's user id or root user id. It is the responsibility of the interface
     * implementation to close the client socket with a call to
     * {@link LocalClientSocket#closeClientSocket(boolean)} once its done processing.
     *
     * The {@link LocalClientSocket#getPeerCred()} can be used to get the {@link PeerCred} object
     * containing info for the connected client/peer.
     *
     * @param localSocketManager The {@link LocalSocketManager} for the server.
     * @param clientSocket The {@link LocalClientSocket} that connected.
     */
    void onClientAccepted(@NonNull LocalSocketManager localSocketManager,
                          @NonNull LocalClientSocket clientSocket);

}
