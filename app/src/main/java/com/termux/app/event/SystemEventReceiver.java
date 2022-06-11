package com.termux.app.event;

import android.content.BroadcastReceiver;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.data.IntentUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.shell.TermuxShellManager;

public class SystemEventReceiver extends BroadcastReceiver {

    private static SystemEventReceiver mInstance;

    private static final String LOG_TAG = "SystemEventReceiver";

    public static synchronized SystemEventReceiver getInstance() {
        if (mInstance == null) {
            mInstance = new SystemEventReceiver();
        }
        return mInstance;
    }

    @Override
    public void onReceive(@NonNull Context context, @Nullable Intent intent) {
        if (intent == null) return;
        Logger.logDebug(LOG_TAG, "Intent Received:\n" + IntentUtils.getIntentString(intent));

        String action = intent.getAction();
        if (action == null) return;

        switch (action) {
            case Intent.ACTION_BOOT_COMPLETED:
                onActionBootCompleted(context, intent);
                break;
            default:
                Logger.logError(LOG_TAG, "Invalid action \"" + action + "\" passed to " + LOG_TAG);
        }
    }

    public synchronized void onActionBootCompleted(@NonNull Context context, @NonNull Intent intent) {
        TermuxShellManager.onActionBootCompleted(context, intent);
    }

}
