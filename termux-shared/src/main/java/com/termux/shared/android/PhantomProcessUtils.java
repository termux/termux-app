package com.termux.shared.android;

import android.Manifest;
import android.content.Context;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.logger.Logger;
import com.termux.shared.shell.command.environment.AndroidShellEnvironment;
import com.termux.shared.shell.command.ExecutionCommand;
import com.termux.shared.shell.command.runner.app.AppShell;

/**
 * Utils for phantom processes added in android 12.
 *
 * https://github.com/termux/termux-app/issues/2366
 * https://issuetracker.google.com/u/1/issues/205156966#comment28
 * https://cs.android.com/android/_/android/platform/frameworks/base/+/09dcdad5
 * https://github.com/agnostic-apollo/Android-Docs/tree/master/ocs/apps/processes/phantom-cached-and-empty-processes.md
 */
public class PhantomProcessUtils {

    private static final String LOG_TAG = "PhantomProcessUtils";

    /**
     * If feature flag set to false, then will disable trimming of phantom process and processes using
     * excessive CPU. Flag is available on Pixel Android 12L beta 3 and Android 13. Availability on
     * other devices will depend on if other vendors merged the 09dcdad5 commit or not in their releases
     * and if they actually want to support the flag. Check {@link FeatureFlagUtils} javadocs for
     * more details.
     */
    public static final String FEATURE_FLAG_SETTINGS_ENABLE_MONITOR_PHANTOM_PROCS = "settings_enable_monitor_phantom_procs";

    /**
     * Maximum number of allowed phantom processes. It is also used as the label for the currently
     * enforced ActivityManagerConstants MAX_PHANTOM_PROCESSES value in the `dumpsys activity settings`
     * output.
     *
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r4:frameworks/base/services/core/java/com/android/server/am/ActivityManagerConstants.java;l=574
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r4:frameworks/base/services/core/java/com/android/server/am/ActivityManagerConstants.java;l=172
     */
    public static final String KEY_MAX_PHANTOM_PROCESSES = "max_phantom_processes";

    /**
     * Whether or not syncs (bulk set operations) for DeviceConfig are disabled currently. The value
     * is boolean (1 or 0). The value '1' means that DeviceConfig#setProperties(DeviceConfig.Properties)
     * will return {@code false}.
     *
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r4:frameworks/base/core/java/android/provider/DeviceConfig.java
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r4:frameworks/base/packages/SettingsProvider/src/com/android/providers/settings/SettingsProvider.java;l=1186
     * https://cs.android.com/android/platform/superproject/+/android-12.0.0_r4:frameworks/base/packages/SettingsProvider/src/com/android/providers/settings/SettingsProvider.java;l=1142
     */
    public static final String SETTINGS_GLOBAL_DEVICE_CONFIG_SYNC_DISABLED = "device_config_sync_disabled";

    /**
     * Get {@link #FEATURE_FLAG_SETTINGS_ENABLE_MONITOR_PHANTOM_PROCS} feature flag value.
     *
     * @param context The {@link Context} for operations.
     * @return Returns {@link FeatureFlagUtils.FeatureFlagValue}.
     */
    @NonNull
    public static FeatureFlagUtils.FeatureFlagValue getFeatureFlagMonitorPhantomProcsValueString(@NonNull Context context) {
        return FeatureFlagUtils.getFeatureFlagValueString(context, FEATURE_FLAG_SETTINGS_ENABLE_MONITOR_PHANTOM_PROCS);
    }

    /**
     * Get currently enforced ActivityManagerConstants MAX_PHANTOM_PROCESSES value, defaults to 32.
     * Can be changed by modifying device config activity_manager namespace "max_phantom_processes" value.
     *
     * @param context The {@link Context} for operations.
     * @return Returns {@link Integer}.
     */
    @Nullable
    public static Integer getActivityManagerMaxPhantomProcesses(@NonNull Context context) {
        if (!PermissionUtils.checkPermissions(context, new String[]{Manifest.permission.DUMP, Manifest.permission.PACKAGE_USAGE_STATS})) {
            return null;
        }

        // Dumpsys logs the currently enforced MAX_PHANTOM_PROCESSES value and not the device config setting.
        String script = "/system/bin/dumpsys activity settings | /system/bin/grep -iE '^[\t ]+" + KEY_MAX_PHANTOM_PROCESSES + "=[0-9]+$' | /system/bin/cut -d = -f2";
        ExecutionCommand executionCommand = new ExecutionCommand(-1, "/system/bin/sh", null,
            script + "\n", "/", ExecutionCommand.Runner.APP_SHELL.getName(), true);
        executionCommand.commandLabel = " ActivityManager " + KEY_MAX_PHANTOM_PROCESSES + " Command";
        executionCommand.backgroundCustomLogLevel = Logger.LOG_LEVEL_OFF;
        AppShell appShell = AppShell.execute(context, executionCommand, null, new AndroidShellEnvironment(), null, true);
        boolean stderrSet = !executionCommand.resultData.stderr.toString().isEmpty();
        if (appShell == null || !executionCommand.isSuccessful() || executionCommand.resultData.exitCode != 0 || stderrSet) {
            Logger.logErrorExtended(LOG_TAG, executionCommand.toString());
            return null;
        }

        try {
            return Integer.parseInt(executionCommand.resultData.stdout.toString().trim());
        } catch (NumberFormatException e) {
            Logger.logStackTraceWithMessage(LOG_TAG, "The " + executionCommand.commandLabel + " did not return a valid integer", e);
            Logger.logErrorExtended(LOG_TAG, executionCommand.toString());
        }

        return null;
    }

    /**
     * Get {@link #SETTINGS_GLOBAL_DEVICE_CONFIG_SYNC_DISABLED} settings value.
     *
     * @param context The {@link Context} for operations.
     * @return Returns {@link Integer}.
     */
    @Nullable
    public static Integer getSettingsGlobalDeviceConfigSyncDisabled(@NonNull Context context) {
        return (Integer) SettingsProviderUtils.getSettingsValue(context, SettingsProviderUtils.SettingNamespace.GLOBAL,
            SettingsProviderUtils.SettingType.INT, SETTINGS_GLOBAL_DEVICE_CONFIG_SYNC_DISABLED, null);
    }

}
