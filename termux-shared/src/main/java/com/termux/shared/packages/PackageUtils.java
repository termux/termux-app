package com.termux.shared.packages;

import android.app.ActivityManager;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.UserManager;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.R;
import com.termux.shared.data.DataUtils;
import com.termux.shared.interact.MessageDialogUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.termux.TermuxConstants;

import java.security.MessageDigest;
import java.util.List;

public class PackageUtils {

    private static final String LOG_TAG = "PackageUtils";

    /**
     * Get the {@link Context} for the package name.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the {@code packageName}.
     * @param packageName The package name whose {@link Context} to get.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    @Nullable
    public static Context getContextForPackage(@NonNull final Context context, String packageName) {
        try {
            return context.createPackageContext(packageName, Context.CONTEXT_RESTRICTED);
        } catch (Exception e) {
            Logger.logVerbose(LOG_TAG, "Failed to get \"" + packageName + "\" package context: " + e.getMessage());
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
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    @Nullable
    public static Context getContextForPackageOrExitApp(@NonNull Context context, String packageName, final boolean exitAppOnError) {
        Context packageContext = getContextForPackage(context, packageName);

        if (packageContext == null && exitAppOnError) {
            String errorMessage = context.getString(R.string.error_get_package_context_failed_message,
                packageName, TermuxConstants.TERMUX_GITHUB_REPO_URL);
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
            return getPackageInfoForPackage(context, 0);
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
        try {
            return context.getPackageManager().getPackageInfo(context.getPackageName(), flags);
        } catch (final Exception e) {
            return null;
        }
    }

    /**
     * Get the app name for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code android:name} attribute.
     */
    public static String getAppNameForPackage(@NonNull final Context context) {
        return context.getApplicationInfo().loadLabel(context.getPackageManager()).toString();
    }

    /**
     * Get the package name for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the package name.
     */
    public static String getPackageNameForPackage(@NonNull final Context context) {
        return context.getApplicationInfo().packageName;
    }

    /**
     * Get the {@code targetSdkVersion} for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code targetSdkVersion}.
     */
    public static int getTargetSDKForPackage(@NonNull final Context context) {
        return context.getApplicationInfo().targetSdkVersion;
    }

    /**
     * Check if the app associated with the {@code context} has {@link ApplicationInfo#FLAG_DEBUGGABLE}
     * set.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code versionName}. This will be {@code null} if an exception is raised.
     */
    public static Boolean isAppForPackageADebuggableBuild(@NonNull final Context context) {
        return ( 0 != ( context.getApplicationInfo().flags & ApplicationInfo.FLAG_DEBUGGABLE ) );
    }

    /**
     * Check if the app associated with the {@code context} has {@link ApplicationInfo#FLAG_EXTERNAL_STORAGE}
     * set.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code versionName}. This will be {@code null} if an exception is raised.
     */
    public static Boolean isAppInstalledOnExternalStorage(@NonNull final Context context) {
        return ( 0 != ( context.getApplicationInfo().flags & ApplicationInfo.FLAG_EXTERNAL_STORAGE ) );
    }

    /**
     * Get the {@code versionCode} for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code versionCode}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static Integer getVersionCodeForPackage(@NonNull final Context context) {
        try {
            return getPackageInfoForPackage(context).versionCode;
        } catch (final Exception e) {
            return null;
        }
    }

    /**
     * Get the {@code versionName} for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code versionName}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static String getVersionNameForPackage(@NonNull final Context context) {
        try {
            return getPackageInfoForPackage(context).versionName;
        } catch (final Exception e) {
            return null;
        }
    }

    /**
     * Get the {@code SHA-256 digest} of signing certificate for the package associated with the {@code context}.
     *
     * @param context The {@link Context} for the package.
     * @return Returns the {@code SHA-256 digest}. This will be {@code null} if an exception is raised.
     */
    @Nullable
    public static String getSigningCertificateSHA256DigestForPackage(@NonNull final Context context) {
        try {
            /*
            * Todo: We may need AndroidManifest queries entries if package is installed but with a different signature on android 11
            * https://developer.android.com/training/package-visibility
            * Need a device that allows (manual) installation of apk with mismatched signature of
            * sharedUserId apps to test. Currently, if its done, PackageManager just doesn't load
            * the package and removes its apk automatically if its installed as a user app instead of system app
            * W/PackageManager: Failed to parse /path/to/com.termux.tasker.apk: Signature mismatch for shared user: SharedUserSetting{xxxxxxx com.termux/10xxx}
            */
            PackageInfo packageInfo = getPackageInfoForPackage(context, PackageManager.GET_SIGNATURES);
            if (packageInfo == null) return null;
            return DataUtils.bytesToHex(MessageDigest.getInstance("SHA-256").digest(packageInfo.signatures[0].toByteArray()));
        } catch (final Exception e) {
            return null;
        }
    }



    /**
     * Get the serial number for the current user.
     *
     * @param context The {@link Context} for operations.
     * @return Returns the serial number. This will be {@code null} if failed to get it.
     */
    @Nullable
    public static Long getSerialNumberForCurrentUser(@NonNull Context context) {
        UserManager userManager = (UserManager) context.getSystemService(Context.USER_SERVICE);
        if (userManager == null) return null;
        return userManager.getSerialNumberForUser(android.os.Process.myUserHandle());
    }

    /**
     * Check if the current user is the primary user. This is done by checking if the the serial
     * number for the current user equals 0.
     *
     * @param context The {@link Context} for operations.
     * @return Returns {@code true} if the current user is the primary user, otherwise [@code false}.
     */
    public static boolean isCurrentUserThePrimaryUser(@NonNull Context context) {
        Long userId = getSerialNumberForCurrentUser(context);
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
     * errors in `logcat` and `RUN_COMMAND` won't work.
     * Check [package-visibility](https://developer.android.com/training/basics/intents/package-visibility#package-name),
     * `QUERY_ALL_PACKAGES` [googleplay policy](https://support.google.com/googleplay/android-developer/answer/10158779
     * and this [article](https://medium.com/androiddevelopers/working-with-package-visibility-dc252829de2d) for more info.
     *
     * {@code
     * <manifest
     *     <queries>
     *         <package android:name="package_name" />
     *    </queries>
     * </manifest>
     * }
     *
     * @param context The context for operations.
     * @return Returns {@code errmsg} if {@code packageName} is not installed or disabled, otherwise {@code null}.
     */
    public static String isAppInstalled(@NonNull final Context context, String appName, String packageName) {
        String errmsg = null;

        PackageManager packageManager = context.getPackageManager();

        ApplicationInfo applicationInfo;
        try {
            applicationInfo = packageManager.getApplicationInfo(packageName, 0);
        } catch (final PackageManager.NameNotFoundException e) {
            applicationInfo = null;
        }
        boolean isAppEnabled = (applicationInfo != null && applicationInfo.enabled);

        // If app is not installed or is disabled
        if (!isAppEnabled)
            errmsg = context.getString(R.string.error_app_not_installed_or_disabled_warning, appName, packageName);

        return errmsg;
    }



    /**
     * Enable or disable a {@link ComponentName} with a call to
     * {@link PackageManager#setComponentEnabledSetting(ComponentName, int, int)}.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the component.
     * @param className The {@link Class} name of the component.
     * @param state If component should be enabled or disabled.
     * @param toastString If this is not {@code null} or empty, then a toast before setting state.
     * @param showErrorMessage If an error message toast should be shown.
     * @return Returns the errmsg if failed to set state, otherwise {@code null}.
     */
    @Nullable
    public static String setComponentState(@NonNull final Context context, @NonNull String packageName,
                                         @NonNull String className, boolean state, String toastString,
                                         boolean showErrorMessage) {
        try {
            PackageManager packageManager = context.getPackageManager();
            if (packageManager != null) {
                ComponentName componentName = new ComponentName(packageName, className);
                if (toastString != null) Logger.showToast(context, toastString, true);
                packageManager.setComponentEnabledSetting(componentName,
                    state ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED : PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                    PackageManager.DONT_KILL_APP);
            }
            return null;
        } catch (final Exception e) {
            String errmsg = context.getString(
                state ? R.string.error_enable_component_failed : R.string.error_disable_component_failed,
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
