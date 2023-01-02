package com.termux.app.event;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.data.IntentUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxUtils;
import com.termux.shared.termux.file.TermuxFileUtils;
import com.termux.shared.termux.shell.command.environment.TermuxShellEnvironment;
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
            case Intent.ACTION_PACKAGE_ADDED:
            case Intent.ACTION_PACKAGE_REMOVED:
            case Intent.ACTION_PACKAGE_REPLACED:
                onActionPackageUpdated(context, intent);
                break;
            default:
                Logger.logError(LOG_TAG, "Invalid action \"" + action + "\" passed to " + LOG_TAG);
        }
    }

    public synchronized void onActionBootCompleted(@NonNull Context context, @NonNull Intent intent) {
        TermuxShellManager.onActionBootCompleted(context, intent);
    }

    public synchronized void onActionPackageUpdated(@NonNull Context context, @NonNull Intent intent) {
        Uri data = intent.getData();
        if (data != null && TermuxUtils.isUriDataForTermuxPluginPackage(data)) {
            Logger.logDebug(LOG_TAG, intent.getAction().replaceAll("^android.intent.action.", "") +
                " event received for \"" + data.toString().replaceAll("^package:", "") + "\"");
            if (TermuxFileUtils.isTermuxFilesDirectoryAccessible(context, false, false) == null)
                TermuxShellEnvironment.writeEnvironmentToFile(context);
        }
    }



    /**
     * Register {@link SystemEventReceiver} to listen to {@link Intent#ACTION_PACKAGE_ADDED},
     * {@link Intent#ACTION_PACKAGE_REMOVED} and {@link Intent#ACTION_PACKAGE_REPLACED} broadcasts.
     * They must be registered dynamically and cannot be registered implicitly in
     * the AndroidManifest.xml due to Android 8+ restrictions.
     *
     *  https://developer.android.com/guide/components/broadcast-exceptions
     */
    public synchronized static void registerPackageUpdateEvents(@NonNull Context context) {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(Intent.ACTION_PACKAGE_ADDED);
        intentFilter.addAction(Intent.ACTION_PACKAGE_REMOVED);
        intentFilter.addAction(Intent.ACTION_PACKAGE_REPLACED);
        intentFilter.addDataScheme("package");
        context.registerReceiver(getInstance(), intentFilter);
    }

    public synchronized static void unregisterPackageUpdateEvents(@NonNull Context context) {
        context.unregisterReceiver(getInstance());
    }

}
