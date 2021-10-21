package com.termux.shared.packages;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.PowerManager;
import android.provider.Settings;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.content.ContextCompat;

import com.google.common.base.Joiner;
import com.termux.shared.R;
import com.termux.shared.logger.Logger;
import com.termux.shared.models.errors.Error;
import com.termux.shared.models.errors.FunctionErrno;
import com.termux.shared.view.ActivityUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

public class PermissionUtils {

    public static final int REQUEST_GRANT_STORAGE_PERMISSION = 1000;

    public static final int REQUEST_DISABLE_BATTERY_OPTIMIZATIONS = 2000;
    public static final int REQUEST_GRANT_DISPLAY_OVER_OTHER_APPS_PERMISSION = 2001;

    private static final String LOG_TAG = "PermissionUtils";


    /**
     * Check if app has been granted the required permission.
     *
     * @param context The context for operations.
     * @param permission The {@link String} name for permission to check.
     * @return Returns {@code true} if permission is granted, otherwise {@code false}.
     */
    public static boolean checkPermission(@NonNull Context context, @NonNull String permission) {
        return checkPermissions(context, new String[]{permission});
    }

    /**
     * Check if app has been granted the required permissions.
     *
     * @param context The context for operations.
     * @param permissions The {@link String[]} names for permissions to check.
     * @return Returns {@code true} if permissions are granted, otherwise {@code false}.
     */
    public static boolean checkPermissions(@NonNull Context context, @NonNull String[] permissions) {
        // checkSelfPermission may return true for permissions not even requested
        List<String> permissionsNotRequested = getPermissionsNotRequested(context, permissions);
        if (permissionsNotRequested.size() > 0) {
            Logger.logError(LOG_TAG,
                context.getString(R.string.error_attempted_to_check_for_permissions_not_requested,
                    Joiner.on(", ").join(permissionsNotRequested)));
            return false;
        }

        int result;
        for (String permission : permissions) {
            result = ContextCompat.checkSelfPermission(context, permission);
            if (result != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }

        return true;
    }



    /**
     * Request user to grant required permissions to the app.
     *
     * @param context The context for operations. It must be an instance of {@link Activity} or
     * {@link AppCompatActivity}.
     * @param permission The {@link String} name for permission to request.
     * @param requestCode The request code to use while asking for permission. It must be `>=0` or
     *                    will fail silently and will log an exception.
     * @return Returns {@code true} if requesting the permission was successful, otherwise {@code false}.
     */
    public static boolean requestPermission(@NonNull Context context, @NonNull String permission,
                                            int requestCode) {
        return requestPermissions(context, new String[]{permission}, requestCode);
    }

    /**
     * Request user to grant required permissions to the app.
     *
     * On sdk 30 (android 11), Activity.onRequestPermissionsResult() will pass
     * {@link PackageManager#PERMISSION_DENIED} (-1) without asking the user for the permission
     * if user previously denied the permission prompt. On sdk 29 (android 10),
     * Activity.onRequestPermissionsResult() will pass {@link PackageManager#PERMISSION_DENIED} (-1)
     * without asking the user for the permission if user previously selected "Deny & don't ask again"
     * option in prompt. The user will have to manually enable permission in app info in Android
     * settings. If user grants and then denies in settings, then next time prompt will shown.
     *
     * @param context The context for operations. It must be an instance of {@link Activity} or
     * {@link AppCompatActivity}.
     * @param permissions The {@link String[]} names for permissions to request.
     * @param requestCode The request code to use while asking for permissions. It must be `>=0` or
     *                    will fail silently and will log an exception.
     * @return Returns {@code true} if requesting the permissions was successful, otherwise {@code false}.
     */
    public static boolean requestPermissions(@NonNull Context context, @NonNull String[] permissions,
                                             int requestCode) {
        List<String> permissionsNotRequested = getPermissionsNotRequested(context, permissions);
        if (permissionsNotRequested.size() > 0) {
            Logger.logErrorAndShowToast(context, LOG_TAG,
                context.getString(R.string.error_attempted_to_ask_for_permissions_not_requested,
                    Joiner.on(", ").join(permissionsNotRequested)));
            return false;
        }

        for (String permission : permissions) {
            int result = ContextCompat.checkSelfPermission(context, permission);
            // If at least one permission not granted
            if (result != PackageManager.PERMISSION_GRANTED) {
                Logger.logInfo(LOG_TAG, "Requesting Permissions: " + Arrays.toString(permissions));

                try {
                    if (context instanceof AppCompatActivity)
                        ((AppCompatActivity) context).requestPermissions(new String[]{permission}, requestCode);
                    else if (context instanceof Activity)
                        ((Activity) context).requestPermissions(new String[]{permission}, requestCode);
                    else {
                        Error.logErrorAndShowToast(context, LOG_TAG,
                            FunctionErrno.ERRNO_PARAMETER_NOT_INSTANCE_OF.getError("context", "requestPermissions", "Activity or AppCompatActivity"));
                        return false;
                    }
                } catch (Exception e) {
                    String errmsg = context.getString(R.string.error_failed_to_request_permissions, requestCode, Arrays.toString(permissions));
                    Logger.logStackTraceWithMessage(LOG_TAG, errmsg, e);
                    Logger.showToast(context, errmsg + "\n" + e.getMessage(), true);
                    return false;
                }
            }
        }

        return true;
    }




    /**
     * Check if app has requested the required permission in the manifest.
     *
     * @param context The context for operations.
     * @param permission The {@link String} name for permission to check.
     * @return Returns {@code true} if permission has been requested, otherwise {@code false}.
     */
    public static boolean isPermissionRequested(@NonNull Context context, @NonNull String permission) {
        return getPermissionsNotRequested(context, new String[]{permission}).size() == 0;
    }

    /**
     * Check if app has requested the required permissions or not in the manifest.
     *
     * @param context The context for operations.
     * @param permissions The {@link String[]} names for permissions to check.
     * @return Returns {@link List<String>} of permissions that have not been requested. It will have
     * size 0 if all permissions have been requested.
     */
    @NonNull
    public static List<String> getPermissionsNotRequested(@NonNull Context context, @NonNull String[] permissions) {
        List<String> permissionsNotRequested = new ArrayList<>();
        Collections.addAll(permissionsNotRequested, permissions);

        PackageInfo packageInfo = PackageUtils.getPackageInfoForPackage(context, PackageManager.GET_PERMISSIONS);
        if (packageInfo == null) {
            return permissionsNotRequested;
        }

        // If no permissions are requested, then nothing to check
        if (packageInfo.requestedPermissions == null || packageInfo.requestedPermissions.length == 0)
            return permissionsNotRequested;

        List<String> requestedPermissionsList = Arrays.asList(packageInfo.requestedPermissions);
        for (String permission : permissions) {
            if (requestedPermissionsList.contains(permission)) {
                permissionsNotRequested.remove(permission);
            }
        }

        return permissionsNotRequested;
    }





    /**
     * Check if {@link Manifest.permission#SYSTEM_ALERT_WINDOW} permission has been granted.
     *
     * @param context The context for operations.
     * @return Returns {@code true} if permission is granted, otherwise {@code false}.
     */
    public static boolean checkDisplayOverOtherAppsPermission(@NonNull Context context) {
        return Settings.canDrawOverlays(context);
    }

    /**
     * Request user to grant {@link Manifest.permission#SYSTEM_ALERT_WINDOW} permission to the app.
     *
     * @param context The context for operations. It must be an instance of {@link Activity} or
     * {@link AppCompatActivity}.
     * @param requestCode The request code to use while asking for permission. It must be `>=0` or
     *                    will fail silently and will log an exception.
     * @return Returns {@code true} if requesting the permission was successful, otherwise {@code false}.
     */
    public static boolean requestDisplayOverOtherAppsPermission(@NonNull Context context, int requestCode) {
        Intent intent = new Intent(Settings.ACTION_MANAGE_OVERLAY_PERMISSION);
        intent.setData(Uri.parse("package:" + context.getPackageName()));
        return ActivityUtils.startActivityForResult(context, requestCode, intent);
    }

    /**
     * Check if running on sdk 29 (android 10) or higher and {@link Manifest.permission#SYSTEM_ALERT_WINDOW}
     * permission has been granted or not.
     *
     * @param context The context for operations.
     * @param logResults If it should be logged that permission has been granted or not.
     * @return Returns {@code true} if permission is granted, otherwise {@code false}.
     */
    public static boolean validateDisplayOverOtherAppsPermissionForPostAndroid10(@NonNull Context context,
                                                                                 boolean logResults) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) return true;

        if (!checkDisplayOverOtherAppsPermission(context)) {
            if (logResults)
                Logger.logWarn(LOG_TAG, context.getPackageName() + " does not have Display over other apps (SYSTEM_ALERT_WINDOW) permission");
            return false;
        } else {
            if (logResults)
                Logger.logDebug(LOG_TAG, context.getPackageName() + " already has Display over other apps (SYSTEM_ALERT_WINDOW) permission");
            return true;
        }
    }





    /**
     * Check if {@link Manifest.permission#REQUEST_IGNORE_BATTERY_OPTIMIZATIONS} permission has been
     * granted.
     *
     * @param context The context for operations.
     * @return Returns {@code true} if permission is granted, otherwise {@code false}.
     */
    public static boolean checkIfBatteryOptimizationsDisabled(@NonNull Context context) {
        PowerManager powerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
        return powerManager.isIgnoringBatteryOptimizations(context.getPackageName());
    }

    /**
     * Request user to grant {@link Manifest.permission#REQUEST_IGNORE_BATTERY_OPTIMIZATIONS}
     * permission to the app.
     *
     * @param context The context for operations. It must be an instance of {@link Activity} or
     * {@link AppCompatActivity}.
     * @param requestCode The request code to use while asking for permission. It must be `>=0` or
     *                    will fail silently and will log an exception.
     * @return Returns {@code true} if requesting the permission was successful, otherwise {@code false}.
     */
    @SuppressLint("BatteryLife")
    public static boolean requestDisableBatteryOptimizations(@NonNull Context context, int requestCode) {
        Intent intent = new Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS);
        intent.setData(Uri.parse("package:" + context.getPackageName()));
        return ActivityUtils.startActivityForResult(context, requestCode, intent);
    }

}
