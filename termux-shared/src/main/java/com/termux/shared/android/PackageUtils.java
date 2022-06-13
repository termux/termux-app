package com.termux.shared.android;

import android.app.ActivityManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.UserHandle;
import android.os.UserManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.RequiresApi;

import com.termux.shared.R;
import com.termux.shared.data.DataUtils;
import com.termux.shared.interact.MessageDialogUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.reflection.ReflectionUtils;

import java.lang.reflect.Field;
import java.security.MessageDigest;
import java.util.List;

public class PackageUtils {

    private static final String LOG_TAG = "PackageUtils";

    /**
     * Get the {@link Context} for the package name with {@link Context#CONTEXT_RESTRICTED} flags.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the {@code packageName}.
     * @param packageName The package name whose {@link Context} to get.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    @Nullable
    public static Context getContextForPackage(@NonNull final Context context, String packageName) {
       return getContextForPackage(context, packageName, Context.CONTEXT_RESTRICTED);
    }

    /**
     * Get the {@link Context} for the package name.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the {@code packageName}.
     * @param packageName The package name whose {@link Context} to get.
     * @param flags The flags for {@link Context} type.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    @Nullable
    public static Context getContextForPackage(@NonNull final Context context, String packageName, int flags) {
        try {
            return context.createPackageContext(packageName, flags);
        } catch (Exception e) {
            Logger.logVerbose(LOG_TAG, "Failed to get \"" + packageName + "\" package context with flags " + flags + ": " + e.getMessage());
            return null;
        }
    }

    /**
     * Get the {@link Context} for a package name.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the {@code packageName}.
     * @param packageName The package name whose {@link Context} to get.
     * @param exitAppOnError If {@code true} and failed to get package context, then a dialog will
     *                       be shown which when dismissed will exit the app.
     * @param helpUrl The help user to add to {@link R.string#error_get_package_context_failed_help_url_message}.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    @Nullable
    public static Context getContextForPackageOrExitApp(@NonNull Context context, String packageName,
                                                        final boolean exitAppOnError, @Nullable String helpUrl) {
        Context packageContext = getContextForPackage(context, packageName);

        if (packageContext == null && exitAppOnError) {
            String errorMessage = context.getString(R.string.error_get_package_context_failed_message,
                packageName);
            if (!DataUtils.isNullOrEmpty(helpUrl))
                errorMessage += "\n" + context.getString(R.string.error_get_package_context_failed_help_url_message, helpUrl);
            Logger.logError(LOG_TAG, errorMessage);
            MessageDialogUtils.exitAppWithErrorMessage(context,
                context.getString(R.string.error_get_package_context_failed_title),
                errorMessage);
        }

        return packageContext;
    }



    /**
     * Get the {@link PackageInfo} for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@link PackageInfo}. This will be {@code null} if an exception is raised.
     */
    public static PackageInfo getPackageInfoForPackage(@NonNull final Context context) {
        return getPackageInfoForPackage(context, context.getPackageName());
    }

    /**
     * Get the {@link PackageInfo} for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @param flags The flags to pass to {@link PackageManager#getPackageInfo(String, int)}.
     * @return Returns the {@link PackageInfo}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static PackageInfo getPackageInfoForPackage(@NonNull final Context context, final int flags) {
        return getPackageInfoForPackage(context, context.getPackageName(), flags);
    }

    /**
     * Get the {@link PackageInfo} for the package associated with the {@code packageName}.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the package.
     * @return Returns the {@link PackageInfo}. This will be {@code null} if an exception is raised.
     */
    public static PackageInfo getPackageInfoForPackage(@NonNull final Context context, @NonNull final String packageName) {
        return getPackageInfoForPackage(context, packageName, 0);
    }

    /**
     * Get the {@link PackageInfo} for the package associated with the {@code packageName}.
     *
     * Also check {@link #isAppInstalled(Context, String, String) if targetting targeting sdk
     * `30` (android `11`) since {@link PackageManager.NameNotFoundException} may be thrown.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the package.
     * @param flags The flags to pass to {@link PackageManager#getPackageInfo(String, int)}.
     * @return Returns the {@link PackageInfo}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static PackageInfo getPackageInfoForPackage(@NonNull final Context context, @NonNull final String packageName, final int flags) {
        try {
            return context.getPackageManager().getPackageInfo(packageName, flags);
        } catch (final Exception e) {
            return null;
        }
    }



    /**
     * Get the {@link ApplicationInfo} for the {@code packageName}.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the package.
     * @return Returns the {@link ApplicationInfo}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static ApplicationInfo getApplicationInfoForPackage(@NonNull final Context context, @NonNull final String packageName) {
        return getApplicationInfoForPackage(context, packageName, 0);
    }

    /**
     * Get the {@link ApplicationInfo} for the {@code packageName}.
     *
     * Also check {@link #isAppInstalled(Context, String, String) if targetting targeting sdk
     * `30` (android `11`) since {@link PackageManager.NameNotFoundException} may be thrown.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the package.
     * @param flags The flags to pass to {@link PackageManager#getApplicationInfo(String, int)}.
     * @return Returns the {@link ApplicationInfo}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static ApplicationInfo getApplicationInfoForPackage(@NonNull final Context context, @NonNull final String packageName, final int flags) {
        try {
            return context.getPackageManager().getApplicationInfo(packageName, flags);
        } catch (final Exception e) {
            return null;
        }
    }

    /**
     * Get the {@code privateFlags} {@link Field} of the {@link ApplicationInfo} class.
     *
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns the private flags or {@code null} if an exception was raised.
     */
    @Nullable
    public static Integer getApplicationInfoPrivateFlagsForPackage(@NonNull final ApplicationInfo applicationInfo) {
        ReflectionUtils.bypassHiddenAPIReflectionRestrictions();
        try {
            return (Integer) ReflectionUtils.invokeField(ApplicationInfo.class, "privateFlags", applicationInfo).value;
        } catch (Exception e) {
            // ClassCastException may be thrown
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to get privateFlags field value for ApplicationInfo class", e);
            return null;
        }
    }

    /**
     * Get the {@code seInfo} {@link Field} of the {@link ApplicationInfo} class.
     *
     * String retrieved from the seinfo tag found in selinux policy. This value can be set through
     * the mac_permissions.xml policy construct. This value is used for setting an SELinux security
     * context on the process as well as its data directory.
     *
     * https://cs.android.com/android/platform/superproject/+/android-7.1.0_r1:frameworks/base/core/java/android/content/pm/ApplicationInfo.java;l=609
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:frameworks/base/core/java/android/content/pm/ApplicationInfo.java;l=981
     * https://cs.android.com/android/platform/superproject/+/android-7.0.0_r1:frameworks/base/services/core/java/com/android/server/pm/SELinuxMMAC.java;l=282
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r32:frameworks/base/services/core/java/com/android/server/pm/SELinuxMMAC.java;l=375
     * https://cs.android.com/android/_/android/platform/frameworks/base/+/be0b8896d1bc385d4c8fb54c21929745935dcbea
     *
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns the selinux info or {@code null} if an exception was raised.
     */
    @Nullable
    public static String getApplicationInfoSeInfoForPackage(@NonNull final ApplicationInfo applicationInfo) {
        ReflectionUtils.bypassHiddenAPIReflectionRestrictions();
        try {
            return (String) ReflectionUtils.invokeField(ApplicationInfo.class, Build.VERSION.SDK_INT < Build.VERSION_CODES.O ? "seinfo" : "seInfo", applicationInfo).value;
        } catch (Exception e) {
            // ClassCastException may be thrown
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to get seInfo field value for ApplicationInfo class", e);
            return null;
        }
    }

    /**
     * Get the {@code seInfoUser} {@link Field} of the {@link ApplicationInfo} class.
     *
     * Also check {@link #getApplicationInfoSeInfoForPackage(ApplicationInfo)}.
     *
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns the selinux info user or {@code null} if an exception was raised.
     */
    @Nullable
    public static String getApplicationInfoSeInfoUserForPackage(@NonNull final ApplicationInfo applicationInfo) {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) return null;
        ReflectionUtils.bypassHiddenAPIReflectionRestrictions();
        try {
            return (String) ReflectionUtils.invokeField(ApplicationInfo.class, "seInfoUser", applicationInfo).value;
        } catch (Exception e) {
            // ClassCastException may be thrown
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to get seInfoUser field value for ApplicationInfo class", e);
            return null;
        }
    }

    /**
     * Get the {@code privateFlags} {@link Field} of the {@link ApplicationInfo} class.
     *
     * @param fieldName The name of the field to get.
     * @return Returns the field value or {@code null} if an exception was raised.
     */
    @Nullable
    public static Integer getApplicationInfoStaticIntFieldValue(@NonNull String fieldName) {
        ReflectionUtils.bypassHiddenAPIReflectionRestrictions();
        try {
            return (Integer) ReflectionUtils.invokeField(ApplicationInfo.class, fieldName, null).value;
        } catch (Exception e) {
            // ClassCastException may be thrown
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed to get \"" + fieldName + "\" field value for ApplicationInfo class", e);
            return null;
        }
    }

    /**
     * Check if the app associated with the {@code applicationInfo} has a specific flag set.
     *
     * @param flagToCheckName The name of the field for the flag to check.
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns {@code true} if app has flag is set, otherwise {@code false}. This will be
     * {@code null} if an exception is raised.
     */
    @Nullable
    public static Boolean isApplicationInfoPrivateFlagSetForPackage(@NonNull String flagToCheckName, @NonNull final ApplicationInfo applicationInfo) {
        Integer privateFlags = getApplicationInfoPrivateFlagsForPackage(applicationInfo);
        if (privateFlags == null) return null;

        Integer flagToCheck = getApplicationInfoStaticIntFieldValue(flagToCheckName);
        if (flagToCheck == null) return null;

        return ( 0 != ( privateFlags & flagToCheck ) );
    }





    /**
     * Get the app name for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code android:name} attribute.
     */
    public static String getAppNameForPackage(@NonNull final Context context) {
        return getAppNameForPackage(context, context.getApplicationInfo());
    }

    /**
     * Get the app name for the package associated with the {@code applicationInfo}.
     *
     * @param context The {@link Context} for operations.
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns the {@code android:name} attribute.
     */
    public static String getAppNameForPackage(@NonNull final Context context, @NonNull final ApplicationInfo applicationInfo) {
        return applicationInfo.loadLabel(context.getPackageManager()).toString();
    }



    /**
     * Get the package name for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the package name.
     */
    public static String getPackageNameForPackage(@NonNull final Context context) {
        return getPackageNameForPackage(context.getApplicationInfo());
    }

    /**
     * Get the package name for the package associated with the {@code applicationInfo}.
     *
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns the package name.
     */
    public static String getPackageNameForPackage(@NonNull final ApplicationInfo applicationInfo) {
        return applicationInfo.packageName;
    }



    /**
     * Get the uid for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the uid.
     */
    public static int getUidForPackage(@NonNull final Context context) {
        return getUidForPackage(context.getApplicationInfo());
    }

    /**
     * Get the uid for the package associated with the {@code applicationInfo}.
     *
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns the uid.
     */
    public static int getUidForPackage(@NonNull final ApplicationInfo applicationInfo) {
        return applicationInfo.uid;
    }



    /**
     * Get the {@code targetSdkVersion} for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code targetSdkVersion}.
     */
    public static int getTargetSDKForPackage(@NonNull final Context context) {
        return getTargetSDKForPackage(context.getApplicationInfo());
    }

    /**
     * Get the {@code targetSdkVersion} for the package associated with the {@code applicationInfo}.
     *
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns the {@code targetSdkVersion}.
     */
    public static int getTargetSDKForPackage(@NonNull final ApplicationInfo applicationInfo) {
        return applicationInfo.targetSdkVersion;
    }



    /**
     * Get the base apk path for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the base apk path.
     */
    public static String getBaseAPKPathForPackage(@NonNull final Context context) {
        return getBaseAPKPathForPackage(context.getApplicationInfo());
    }

    /**
     * Get the base apk path for the package associated with the {@code applicationInfo}.
     *
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns the base apk path.
     */
    public static String getBaseAPKPathForPackage(@NonNull final ApplicationInfo applicationInfo) {
        return applicationInfo.publicSourceDir;
    }



    /**
     * Check if the app associated with the {@code context} has {@link ApplicationInfo#FLAG_DEBUGGABLE}
     * set.
     *
     * @param context The {@link Context} for the package.
     * @return Returns {@code true} if app is debuggable, otherwise {@code false}.
     */
    public static boolean isAppForPackageADebuggableBuild(@NonNull final Context context) {
        return isAppForPackageADebuggableBuild(context.getApplicationInfo());
    }

    /**
     * Check if the app associated with the {@code applicationInfo} has {@link ApplicationInfo#FLAG_DEBUGGABLE}
     * set.
     *
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns {@code true} if app is debuggable, otherwise {@code false}.
     */
    public static boolean isAppForPackageADebuggableBuild(@NonNull final ApplicationInfo applicationInfo) {
        return ( 0 != ( applicationInfo.flags & ApplicationInfo.FLAG_DEBUGGABLE ) );
    }



    /**
     * Check if the app associated with the {@code context} has {@link ApplicationInfo#FLAG_EXTERNAL_STORAGE}
     * set.
     *
     * @param context The {@link Context} for the package.
     * @return Returns {@code true} if app is installed on external storage, otherwise {@code false}.
     */
    public static boolean isAppInstalledOnExternalStorage(@NonNull final Context context) {
        return isAppInstalledOnExternalStorage(context.getApplicationInfo());
    }

    /**
     * Check if the app associated with the {@code applicationInfo} has {@link ApplicationInfo#FLAG_EXTERNAL_STORAGE}
     * set.
     *
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns {@code true} if app is installed on external storage, otherwise {@code false}.
     */
    public static boolean isAppInstalledOnExternalStorage(@NonNull final ApplicationInfo applicationInfo) {
        return ( 0 != ( applicationInfo.flags & ApplicationInfo.FLAG_EXTERNAL_STORAGE ) );
    }



    /**
     * Check if the app associated with the {@code context} has
     * ApplicationInfo.PRIVATE_FLAG_REQUEST_LEGACY_EXTERNAL_STORAGE (requestLegacyExternalStorage)
     * set to {@code true} in app manifest.
     *
     * @param context The {@link Context} for the package.
     * @return Returns {@code true} if app has requested legacy external storage, otherwise
     * {@code false}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static Boolean hasRequestedLegacyExternalStorage(@NonNull final Context context) {
        return hasRequestedLegacyExternalStorage(context.getApplicationInfo());
    }

    /**
     * Check if the app associated with the {@code applicationInfo} has
     * ApplicationInfo.PRIVATE_FLAG_REQUEST_LEGACY_EXTERNAL_STORAGE (requestLegacyExternalStorage)
     * set to {@code true} in app manifest.
     *
     * @param applicationInfo The {@link ApplicationInfo} for the package.
     * @return Returns {@code true} if app has requested legacy external storage, otherwise
     * {@code false}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static Boolean hasRequestedLegacyExternalStorage(@NonNull final ApplicationInfo applicationInfo) {
        return isApplicationInfoPrivateFlagSetForPackage("PRIVATE_FLAG_REQUEST_LEGACY_EXTERNAL_STORAGE", applicationInfo);
    }



    /**
     * Get the {@code versionCode} for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code versionCode}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static Integer getVersionCodeForPackage(@NonNull final Context context) {
        return getVersionCodeForPackage(context, context.getPackageName());
    }

    /**
     * Get the {@code versionCode} for the {@code packageName}.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the package.
     * @return Returns the {@code versionCode}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static Integer getVersionCodeForPackage(@NonNull final Context context, @NonNull final String packageName) {
        return getVersionCodeForPackage(getPackageInfoForPackage(context, packageName));
    }

    /**
     * Get the {@code versionCode} for the {@code packageName}.
     *
     * @param packageInfo The {@link PackageInfo} for the package.
     * @return Returns the {@code versionCode}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static Integer getVersionCodeForPackage(@Nullable final PackageInfo packageInfo) {
        return packageInfo != null ? packageInfo.versionCode : null;
    }



    /**
     * Get the {@code versionName} for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code versionName}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static String getVersionNameForPackage(@NonNull final Context context) {
        return getVersionNameForPackage(context, context.getPackageName());
    }

    /**
     * Get the {@code versionName} for the {@code packageName}.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the package.
     * @return Returns the {@code versionName}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static String getVersionNameForPackage(@NonNull final Context context, @NonNull final String packageName) {
        return getVersionNameForPackage(getPackageInfoForPackage(context, packageName));
    }

    /**
     * Get the {@code versionName} for the {@code packageName}.
     *
     * @param packageInfo The {@link PackageInfo} for the package.
     * @return Returns the {@code versionName}. This will be {@code null} if an {@code packageInfo}
     * is {@code null}.
     */
    @Nullable
    public static String getVersionNameForPackage(@Nullable final PackageInfo packageInfo) {
        return packageInfo != null ? packageInfo.versionName : null;
    }



    /**
     * Get the {@code SHA-256 digest} of signing certificate for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code SHA-256 digest}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static String getSigningCertificateSHA256DigestForPackage(@NonNull final Context context) {
        return getSigningCertificateSHA256DigestForPackage(context, context.getPackageName());
    }

    /**
     * Get the {@code SHA-256 digest} of signing certificate for the {@code packageName}.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the package.
     * @return Returns the {@code SHA-256 digest}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static String getSigningCertificateSHA256DigestForPackage(@NonNull final Context context, @NonNull final String packageName) {
        try {
            /*
             * Todo: We may need AndroidManifest queries entries if package is installed but with a different signature on android 11
             * https://developer.android.com/training/package-visibility
             * Need a device that allows (manual) installation of apk with mismatched signature of
             * sharedUserId apps to test. Currently, if its done, PackageManager just doesn't load
             * the package and removes its apk automatically if its installed as a user app instead of system app
             * W/PackageManager: Failed to parse /path/to/com.termux.tasker.apk: Signature mismatch for shared user: SharedUserSetting{xxxxxxx com.termux/10xxx}
             */
            PackageInfo packageInfo = getPackageInfoForPackage(context, packageName, PackageManager.GET_SIGNATURES);
            if (packageInfo == null) return null;
            return DataUtils.bytesToHex(MessageDigest.getInstance("SHA-256").digest(packageInfo.signatures[0].toByteArray()));
        } catch (final Exception e) {
            return null;
        }
    }



    /**
     * Get the serial number for the user for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the serial number. This will be {@code null} if failed to get it.
     */
    @RequiresApi(api = Build.VERSION_CODES.N)
    @Nullable
    public static Long getUserIdForPackage(@NonNull Context context) {
        UserManager userManager = (UserManager) context.getSystemService(Context.USER_SERVICE);
        if (userManager == null) return null;
        return userManager.getSerialNumberForUser(UserHandle.getUserHandleForUid(getUidForPackage(context)));
    }

    /**
     * Check if the current user is the primary user. This is done by checking if the the serial
     * number for the current user equals 0.
     *
     * @param context The {@link Context} for operations.
     * @return Returns {@code true} if the current user is the primary user, otherwise [@code false}.
     */
    @RequiresApi(api = Build.VERSION_CODES.N)
    public static boolean isCurrentUserThePrimaryUser(@NonNull Context context) {
        Long userId = getUserIdForPackage(context);
        return userId != null && userId == 0;
    }

    /**
     * Get the profile owner package name for the current user.
     *
     * @param context The {@link Context} for operations.
     * @return Returns the profile owner package name. This will be {@code null} if failed to get it
     * or no profile owner for the current user.
     */
    @Nullable
    public static String getProfileOwnerPackageNameForUser(@NonNull Context context) {
        DevicePolicyManager devicePolicyManager = (DevicePolicyManager) context.getSystemService(Context.DEVICE_POLICY_SERVICE);
        if (devicePolicyManager == null) return null;
        List<ComponentName> activeAdmins = devicePolicyManager.getActiveAdmins();
        if (activeAdmins != null){
            for (ComponentName admin:activeAdmins){
                String packageName = admin.getPackageName();
                if(devicePolicyManager.isProfileOwnerApp(packageName))
                    return packageName;
            }
        }
        return null;
    }



    /**
     * Get the process id of the main app process of a package. This will work for sharedUserId. Note
     * that some apps have multiple processes for the app like with `android:process=":background"`
     * attribute in AndroidManifest.xml.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the process.
     * @return Returns the process if found and running, otherwise {@code null}.
     */
    @Nullable
    public static String getPackagePID(final Context context, String packageName) {
        ActivityManager activityManager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        if (activityManager != null) {
            List<ActivityManager.RunningAppProcessInfo> processInfos = activityManager.getRunningAppProcesses();
            if (processInfos != null) {
                ActivityManager.RunningAppProcessInfo processInfo;
                for (int i = 0; i < processInfos.size(); i++) {
                    processInfo = processInfos.get(i);
                    if (processInfo.processName.equals(packageName))
                        return String.valueOf(processInfo.pid);
                }
            }
        }
        return null;
    }



    /**
     * Check if app is installed and enabled. This can be used by external apps that don't
     * share `sharedUserId` with the an app.
     *
     * If your third-party app is targeting sdk `30` (android `11`), then it needs to add package
     * name to the `queries` element or request `QUERY_ALL_PACKAGES` permission in its
     * `AndroidManifest.xml`. Otherwise it will get `PackageSetting{...... package_name/......} BLOCKED`
     * errors in `logcat` and  {@link PackageManager.NameNotFoundException} may be thrown.
     * `RUN_COMMAND` intent won't work either.
     * Check [package-visibility](https://developer.android.com/training/basics/intents/package-visibility#package-name),
     * `QUERY_ALL_PACKAGES` [googleplay policy](https://support.google.com/googleplay/android-developer/answer/10158779
     * and this [article](https://medium.com/androiddevelopers/working-with-package-visibility-dc252829de2d) for more info.
     *
     * {@code
     * <manifest
     *     <queries>
     *         <package android:name="com.termux" />
     *    </queries>
     *
     *    <application
     *        ....
     *    </application>
     * </manifest>
     * }
     *
     * @param context The context for operations.
     * @param appName The name of the app.
     * @param packageName The package name of the package.
     * @return Returns {@code errmsg} if {@code packageName} is not installed or disabled, otherwise {@code null}.
     */
    public static String isAppInstalled(@NonNull final Context context, String appName, String packageName) {
        String errmsg = null;

        ApplicationInfo applicationInfo = getApplicationInfoForPackage(context, packageName);
        boolean isAppEnabled = (applicationInfo != null && applicationInfo.enabled);

        // If app is not installed or is disabled
        if (!isAppEnabled)
            errmsg = context.getString(R.string.error_app_not_installed_or_disabled_warning, appName, packageName);

        return errmsg;
    }


    /** Wrapper for {@link #setComponentState(Context, String, String, boolean, String, boolean, boolean)} with
     * {@code alwaysShowToast} {@code true}. */
    public static String setComponentState(@NonNull final Context context, @NonNull String packageName,
                                           @NonNull String className, boolean newState, String toastString,
                                           boolean showErrorMessage) {
        return setComponentState(context, packageName, className, newState, toastString, showErrorMessage, true);
    }

    /**
     * Enable or disable a {@link ComponentName} with a call to
     * {@link PackageManager#setComponentEnabledSetting(ComponentName, int, int)}.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the component.
     * @param className The {@link Class} name of the component.
     * @param newState If component should be enabled or disabled.
     * @param toastString If this is not {@code null} or empty, then a toast before setting state.
     * @param showErrorMessage If an error message toast should be shown.
     * @param alwaysShowToast If toast should always be shown even if current state matches new state.
     * @return Returns the errmsg if failed to set state, otherwise {@code null}.
     */
    @Nullable
    public static String setComponentState(@NonNull final Context context, @NonNull String packageName,
                                           @NonNull String className, boolean newState, String toastString,
                                           boolean alwaysShowToast, boolean showErrorMessage) {
        try {
            PackageManager packageManager = context.getPackageManager();
            if (packageManager != null) {
                if (toastString != null && alwaysShowToast) {
                    Logger.showToast(context, toastString, true);
                    toastString = null;
                }

                Boolean currentlyDisabled = PackageUtils.isComponentDisabled(context, packageName, className, false);
                if (currentlyDisabled == null)
                    throw new UnsupportedOperationException("Failed to find if component currently disabled");

                Boolean setState = null;
                if (newState && currentlyDisabled)
                    setState = true;
                else if (!newState && !currentlyDisabled)
                    setState = false;

                if (setState == null) return null;

                if (toastString != null) Logger.showToast(context, toastString, true);
                ComponentName componentName = new ComponentName(packageName, className);
                packageManager.setComponentEnabledSetting(componentName,
                    setState ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED : PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                    PackageManager.DONT_KILL_APP);
            }
            return null;
        } catch (final Exception e) {
            String errmsg = context.getString(
                newState ? R.string.error_enable_component_failed : R.string.error_disable_component_failed,
                packageName, className) + ": " + e.getMessage();
            if (showErrorMessage)
                Logger.showToast(context, errmsg, true);
            return errmsg;
        }
    }

    /**
     * Check if state of a {@link ComponentName} is {@link PackageManager#COMPONENT_ENABLED_STATE_DISABLED}
     * with a call to {@link PackageManager#getComponentEnabledSetting(ComponentName)}.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the component.
     * @param className The {@link Class} name of the component.
     * @param logErrorMessage If an error message should be logged.
     * @return Returns {@code true} if disabled, {@code false} if not and {@code null} if failed to
     * get the state.
     */
    public static Boolean isComponentDisabled(@NonNull final Context context, @NonNull String packageName,
                                              @NonNull String className, boolean logErrorMessage) {
        try {
            PackageManager packageManager = context.getPackageManager();
            if (packageManager != null) {
                ComponentName componentName = new ComponentName(packageName, className);
                // Will throw IllegalArgumentException: Unknown component: ComponentInfo{} if app
                // for context is not installed or component does not exist.
                return packageManager.getComponentEnabledSetting(componentName) == PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
            }
        } catch (final Exception e) {
            if (logErrorMessage)
                Logger.logStackTraceWithMessage(LOG_TAG, context.getString(R.string.error_get_component_state_failed, packageName, className), e);
        }

        return null;
    }

    /**
     * Check if an {@link android.app.Activity} {@link ComponentName} can be called by calling
     * {@link PackageManager#queryIntentActivities(Intent, int)}.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the component.
     * @param className The {@link Class} name of the component.
     * @param flags The flags to filter results.
     * @return Returns {@code true} if it exists, otherwise {@code false}.
     */
    public static boolean doesActivityComponentExist(@NonNull final Context context, @NonNull String packageName,
                                                     @NonNull String className, int flags) {
        try {
            PackageManager packageManager = context.getPackageManager();
            if (packageManager != null) {
                Intent intent = new Intent();
                intent.setClassName(packageName, className);
                return packageManager.queryIntentActivities(intent, flags).size() > 0;
            }
        } catch (final Exception e) {
            // ignore
        }

        return false;
    }

}
