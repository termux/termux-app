package com.termux.shared.packages;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;

import androidx.core.content.ContextCompat;

import com.termux.shared.R;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.logger.Logger;
import com.termux.shared.settings.preferences.TermuxAppSharedPreferences;

import java.util.Arrays;

public class PermissionUtils {

    public static final int ACTION_MANAGE_OVERLAY_PERMISSION_REQUEST_CODE = 0;

    private static final String LOG_TAG = "PermissionUtils";

    public static boolean checkPermissions(Context context, String[] permissions) {
        int result;

        for (String p:permissions) {
            result = ContextCompat.checkSelfPermission(context,p);
            if (result != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

    public static void askPermissions(Activity context, String[] permissions) {
        if (context == null || permissions == null) return;

        int result;
        Logger.showToast(context, context.getString(R.string.message_sudo_please_grant_permissions), true);
        try {Thread.sleep(1000);} catch (InterruptedException e) {e.printStackTrace();}

        for (String permission:permissions) {
            result = ContextCompat.checkSelfPermission(context, permission);
            if (result != PackageManager.PERMISSION_GRANTED) {
                Logger.logDebug(LOG_TAG, "Requesting Permissions: " + Arrays.toString(permissions));
                context.requestPermissions(new String[]{permission}, 0);
            }
        }
    }



    public static boolean checkDisplayOverOtherAppsPermission(Context context) {
        boolean permissionGranted;

        permissionGranted = Settings.canDrawOverlays(context);
        if (!permissionGranted) {
            Logger.logWarn(LOG_TAG, TermuxConstants.TERMUX_APP_NAME + " App does not have Display over other apps (SYSTEM_ALERT_WINDOW) permission");
            return false;
        } else {
            Logger.logDebug(LOG_TAG, TermuxConstants.TERMUX_APP_NAME + " App already has Display over other apps (SYSTEM_ALERT_WINDOW) permission");
            return true;
        }
    }

    public static void askDisplayOverOtherAppsPermission(Activity context) {
        Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION, Uri.parse("package:" + context.getPackageName()));
        context.startActivityForResult(intent, ACTION_MANAGE_OVERLAY_PERMISSION_REQUEST_CODE);
    }

    public static boolean validateDisplayOverOtherAppsPermissionForPostAndroid10(Context context) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) return true;
        
        if (!PermissionUtils.checkDisplayOverOtherAppsPermission(context)) {
            TermuxAppSharedPreferences preferences = TermuxAppSharedPreferences.build(context);
            if (preferences == null) return false;

            if (preferences.arePluginErrorNotificationsEnabled())
                Logger.showToast(context, context.getString(R.string.error_display_over_other_apps_permission_not_granted), true);
            return false;
        } else {
            return true;
        }
    }

}
