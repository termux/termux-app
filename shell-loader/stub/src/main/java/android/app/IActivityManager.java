package android.app;

import android.content.IIntentSender;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;

public interface IActivityManager {
    public IIntentSender getIntentSender(int type,
                                         String packageName, IBinder token, String resultWho,
                                         int requestCode, Intent[] intents, String[] resolvedTypes,
                                         int flags, Bundle bOptions, int userId);
}
