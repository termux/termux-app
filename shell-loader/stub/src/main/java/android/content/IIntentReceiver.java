package android.content;

import android.os.Bundle;

/**
 * Stub - will be replaced by system at runtime
 */
public interface IIntentReceiver {
    public static abstract class Stub implements IIntentReceiver {
        public abstract void performReceive(Intent intent, int resultCode, String data, Bundle extras,
                                            boolean ordered, boolean sticky, int sendingUser);
    }
} 
