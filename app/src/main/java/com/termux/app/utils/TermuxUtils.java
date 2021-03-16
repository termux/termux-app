package com.termux.app.utils;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ResolveInfo;

import com.termux.app.TermuxConstants;

import java.util.List;

public class TermuxUtils {

    public static Context getTermuxPackageContext(Context context) {
        try {
            return context.createPackageContext(TermuxConstants.TERMUX_PACKAGE_NAME, Context.CONTEXT_RESTRICTED);
        } catch (Exception e) {
            Logger.logStackTraceWithMessage("Failed to get \"" + TermuxConstants.TERMUX_PACKAGE_NAME + "\" package context. Force using current context.", e);
            Logger.logError("Force using current context");
            return context;
        }
    }

    /**
     * Send the {@link TermuxConstants#BROADCAST_TERMUX_OPENED} broadcast to notify apps that Termux
     * app has been opened.
     *
     * @param context The Context to send the broadcast.
     */
    public static void sendTermuxOpenedBroadcast(Context context) {
        Intent broadcast = new Intent(TermuxConstants.BROADCAST_TERMUX_OPENED);
        List<ResolveInfo> matches = context.getPackageManager().queryBroadcastReceivers(broadcast, 0);

        // send broadcast to registered Termux receivers
        // this technique is needed to work around broadcast changes that Oreo introduced
        for (ResolveInfo info : matches) {
            Intent explicitBroadcast = new Intent(broadcast);
            ComponentName cname = new ComponentName(info.activityInfo.applicationInfo.packageName,
                info.activityInfo.name);
            explicitBroadcast.setComponent(cname);
            context.sendBroadcast(explicitBroadcast);
        }
    }

}
