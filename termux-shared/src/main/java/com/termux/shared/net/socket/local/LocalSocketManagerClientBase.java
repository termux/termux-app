package com.termux.shared.net.socket.local;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.errors.Error;
import com.termux.shared.logger.Logger;

/** Base helper implementation for {@link ILocalSocketManager}. */
public abstract class LocalSocketManagerClientBase implements ILocalSocketManager {

    @Nullable
    @Override
    public Thread.UncaughtExceptionHandler getLocalSocketManagerClientThreadUEH(
        @NonNull LocalSocketManager localSocketManager) {
        return null;
    }

    @Override
    public void onError(@NonNull LocalSocketManager localSocketManager,
                        @Nullable LocalClientSocket clientSocket, @NonNull Error error) {
        // Only log if log level is debug or higher since PeerCred.cmdline may contain private info
        Logger.logErrorPrivate(getLogTag(), "onError");
        Logger.logErrorPrivateExtended(getLogTag(), LocalSocketManager.getErrorLogString(error,
            localSocketManager.getLocalSocketRunConfig(), clientSocket));
    }

    @Override
    public void onDisallowedClientConnected(@NonNull LocalSocketManager localSocketManager,
                                            @NonNull LocalClientSocket clientSocket, @NonNull Error error) {
        Logger.logWarn(getLogTag(), "onDisallowedClientConnected");
        Logger.logWarnExtended(getLogTag(), LocalSocketManager.getErrorLogString(error,
            localSocketManager.getLocalSocketRunConfig(), clientSocket));
    }

    @Override
    public void onClientAccepted(@NonNull LocalSocketManager localSocketManager,
                                 @NonNull LocalClientSocket clientSocket) {
        // Just close socket and let child class handle any required communication
        clientSocket.closeClientSocket(true);
    }



    protected abstract String getLogTag();

}
