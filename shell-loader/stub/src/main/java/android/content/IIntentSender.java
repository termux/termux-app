package android.content;

import android.os.Bundle;
import android.os.IBinder;

/**
 * Stub - will be replaced by system at runtime
 */
public abstract class IIntentSender {
    abstract void send(int code, Intent intent, String resolvedType, IBinder whitelistToken,
                       IIntentReceiver finishedReceiver, String requiredPermission, Bundle options);
}
