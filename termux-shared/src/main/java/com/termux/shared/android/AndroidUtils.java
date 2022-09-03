package com.termux.shared.android;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.os.Build;

import androidx.annotation.NonNull;

import com.google.common.base.Joiner;
import com.termux.shared.R;
import com.termux.shared.data.DataUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.markdown.MarkdownUtils;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;
import java.util.TimeZone;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class AndroidUtils {

    /**
     * Get a markdown {@link String} for the app info for the package associated with the {@code context}.
     * This will contain additional info about the app in addition to the one returned by
     * {@link #getAppInfoMarkdownString(Context, String)}, which will be got via the {@code context}
     * object.
     *
     * @param context The context for operations for the package.
     * @return Returns the markdown {@link String}.
     */
    public static String getAppInfoMarkdownString(@NonNull final Context context) {
        StringBuilder markdownString = new StringBuilder();

        String appInfo = getAppInfoMarkdownString(context, context.getPackageName());
        if (appInfo == null)
            return markdownString.toString();
        else
            markdownString.append(appInfo);

        String filesDir = context.getFilesDir().getAbsolutePath();
        if (!filesDir.equals("/data/user/0/" + context.getPackageName() + "/files") &&
            !filesDir.equals("/data/data/" + context.getPackageName() + "/files"))
            AndroidUtils.appendPropertyToMarkdown(markdownString,"FILES_DIR", filesDir);


        if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            Long userId = PackageUtils.getUserIdForPackage(context);
            if (userId == null || userId != 0)
                AndroidUtils.appendPropertyToMarkdown(markdownString, "USER_ID", userId);
        }

        AndroidUtils.appendPropertyToMarkdownIfSet(markdownString,"PROFILE_OWNER", PackageUtils.getProfileOwnerPackageNameForUser(context));

        return markdownString.toString();
    }

    /**
     * Get a markdown {@link String} for the app info for the {@code packageName}.
     *
     * @param context The {@link Context} for operations.
     * @param packageName The package name of the package.
     * @return Returns the markdown {@link String}.
     */
    public static String getAppInfoMarkdownString(@NonNull final Context context, @NonNull final String packageName) {
        PackageInfo packageInfo = PackageUtils.getPackageInfoForPackage(context, packageName);
        if (packageInfo == null) return null;
        ApplicationInfo applicationInfo = PackageUtils.getApplicationInfoForPackage(context, packageName);
        if (applicationInfo == null) return null;

        StringBuilder markdownString = new StringBuilder();

        AndroidUtils.appendPropertyToMarkdown(markdownString,"APP_NAME", PackageUtils.getAppNameForPackage(context, applicationInfo));
        AndroidUtils.appendPropertyToMarkdown(markdownString,"PACKAGE_NAME", PackageUtils.getPackageNameForPackage(applicationInfo));
        AndroidUtils.appendPropertyToMarkdown(markdownString,"VERSION_NAME", PackageUtils.getVersionNameForPackage(packageInfo));
        AndroidUtils.appendPropertyToMarkdown(markdownString,"VERSION_CODE", PackageUtils.getVersionCodeForPackage(packageInfo));
        AndroidUtils.appendPropertyToMarkdown(markdownString,"UID", PackageUtils.getUidForPackage(applicationInfo));
        AndroidUtils.appendPropertyToMarkdown(markdownString,"TARGET_SDK", PackageUtils.getTargetSDKForPackage(applicationInfo));
        AndroidUtils.appendPropertyToMarkdown(markdownString,"IS_DEBUGGABLE_BUILD", PackageUtils.isAppForPackageADebuggableBuild(applicationInfo));

        if (PackageUtils.isAppInstalledOnExternalStorage(applicationInfo)) {
            AndroidUtils.appendPropertyToMarkdown(markdownString,"APK_PATH", PackageUtils.getBaseAPKPathForPackage(applicationInfo));
            AndroidUtils.appendPropertyToMarkdown(markdownString,"IS_INSTALLED_ON_EXTERNAL_STORAGE", true);
        }

        AndroidUtils.appendPropertyToMarkdown(markdownString,"SE_PROCESS_CONTEXT", SELinuxUtils.getContext());
        AndroidUtils.appendPropertyToMarkdown(markdownString,"SE_FILE_CONTEXT", SELinuxUtils.getFileContext(context.getFilesDir().getAbsolutePath()));

        String seInfoUser = PackageUtils.getApplicationInfoSeInfoUserForPackage(applicationInfo);
        AndroidUtils.appendPropertyToMarkdown(markdownString,"SE_INFO", PackageUtils.getApplicationInfoSeInfoForPackage(applicationInfo) +
            (DataUtils.isNullOrEmpty(seInfoUser) ? "" : seInfoUser));

        return markdownString.toString();
    }

    public static String getDeviceInfoMarkdownString(@NonNull final Context context) {
        return getDeviceInfoMarkdownString(context, false);
    }

    /**
     * Get a markdown {@link String} for the device info.
     *
     * @param context The context for operations.
     * @param addPhantomProcessesInfo If phantom processes info should be added on Android >= 12.
     * @return Returns the markdown {@link String}.
     */
    public static String getDeviceInfoMarkdownString(@NonNull final Context context, boolean addPhantomProcessesInfo) {
        // Some properties cannot be read with {@link System#getProperty(String)} but can be read
        // directly by running getprop command
        Properties systemProperties = getSystemProperties();

        StringBuilder markdownString = new StringBuilder();

        markdownString.append("## Device Info");

        markdownString.append("\n\n### Software\n");
        appendPropertyToMarkdown(markdownString,"OS_VERSION", getSystemPropertyWithAndroidAPI("os.version"));
        appendPropertyToMarkdown(markdownString, "SDK_INT", Build.VERSION.SDK_INT);
        // If its a release version
        if ("REL".equals(Build.VERSION.CODENAME))
            appendPropertyToMarkdown(markdownString, "RELEASE", Build.VERSION.RELEASE);
        else
            appendPropertyToMarkdown(markdownString, "CODENAME", Build.VERSION.CODENAME);
        appendPropertyToMarkdown(markdownString, "ID", Build.ID);
        appendPropertyToMarkdown(markdownString, "DISPLAY", Build.DISPLAY);
        appendPropertyToMarkdown(markdownString, "INCREMENTAL", Build.VERSION.INCREMENTAL);
        appendPropertyToMarkdownIfSet(markdownString, "SECURITY_PATCH", systemProperties.getProperty("ro.build.version.security_patch"));
        appendPropertyToMarkdownIfSet(markdownString, "IS_DEBUGGABLE", systemProperties.getProperty("ro.debuggable"));
        appendPropertyToMarkdownIfSet(markdownString, "IS_EMULATOR", systemProperties.getProperty("ro.boot.qemu"));
        appendPropertyToMarkdownIfSet(markdownString, "IS_TREBLE_ENABLED", systemProperties.getProperty("ro.treble.enabled"));
        appendPropertyToMarkdown(markdownString, "TYPE", Build.TYPE);
        appendPropertyToMarkdown(markdownString, "TAGS", Build.TAGS);

        // If on Android >= 12
        if (Build.VERSION.SDK_INT > Build.VERSION_CODES.R) {
            Integer maxPhantomProcesses = PhantomProcessUtils.getActivityManagerMaxPhantomProcesses(context);
            if (maxPhantomProcesses != null)
                appendPropertyToMarkdown(markdownString, "MAX_PHANTOM_PROCESSES", maxPhantomProcesses);
            else
                appendLiteralPropertyToMarkdown(markdownString, "MAX_PHANTOM_PROCESSES", "- (*" + context.getString(R.string.msg_requires_dump_and_package_usage_stats_permissions) + "*)");

            appendPropertyToMarkdown(markdownString, "MONITOR_PHANTOM_PROCS", PhantomProcessUtils.getFeatureFlagMonitorPhantomProcsValueString(context).getName());
            appendPropertyToMarkdown(markdownString, "DEVICE_CONFIG_SYNC_DISABLED", PhantomProcessUtils.getSettingsGlobalDeviceConfigSyncDisabled(context));
        }

        markdownString.append("\n\n### Hardware\n");
        appendPropertyToMarkdown(markdownString, "MANUFACTURER", Build.MANUFACTURER);
        appendPropertyToMarkdown(markdownString, "BRAND", Build.BRAND);
        appendPropertyToMarkdown(markdownString, "MODEL", Build.MODEL);
        appendPropertyToMarkdown(markdownString, "PRODUCT", Build.PRODUCT);
        appendPropertyToMarkdown(markdownString, "BOARD", Build.BOARD);
        appendPropertyToMarkdown(markdownString, "HARDWARE", Build.HARDWARE);
        appendPropertyToMarkdown(markdownString, "DEVICE", Build.DEVICE);
        appendPropertyToMarkdown(markdownString, "SUPPORTED_ABIS", Joiner.on(", ").skipNulls().join(Build.SUPPORTED_ABIS));

        markdownString.append("\n##\n");

        return markdownString.toString();
    }



    public static Properties getSystemProperties() {
        Properties systemProperties = new Properties();

        // getprop commands returns values in the format `[key]: [value]`
        // Regex matches string starting with a literal `[`,
        // followed by one or more characters that do not match a closing square bracket as the key,
        // followed by a literal `]: [`,
        // followed by one or more characters as the value,
        // followed by string ending with literal `]`
        // multiline values will be ignored
        Pattern propertiesPattern = Pattern.compile("^\\[([^]]+)]: \\[(.+)]$");

        try {
            Process process = new ProcessBuilder()
                .command("/system/bin/getprop")
                .redirectErrorStream(true)
                .start();

            InputStream inputStream = process.getInputStream();
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(inputStream));
            String line, key, value;

            while ((line = bufferedReader.readLine()) != null) {
                Matcher matcher = propertiesPattern.matcher(line);
                if (matcher.matches()) {
                    key = matcher.group(1);
                    value = matcher.group(2);
                    if (key != null && value != null && !key.isEmpty() && !value.isEmpty())
                        systemProperties.put(key, value);
                }
            }

            bufferedReader.close();
            process.destroy();

        } catch (IOException e) {
            Logger.logStackTraceWithMessage("Failed to get run \"/system/bin/getprop\" to get system properties.", e);
        }

        //for (String key : systemProperties.stringPropertyNames()) {
        //    Logger.logVerbose(key + ": " +  systemProperties.get(key));
        //}

        return systemProperties;
    }

    public static String getSystemPropertyWithAndroidAPI(@NonNull String property) {
        try {
            return System.getProperty(property);
        } catch (Exception e) {
            Logger.logVerbose("Failed to get system property \"" + property + "\":" + e.getMessage());
            return null;
        }
    }

    public static void appendPropertyToMarkdownIfSet(StringBuilder markdownString, String label, Object value) {
        if (value == null) return;
        if (value instanceof String && (((String) value).isEmpty()) || "REL".equals(value)) return;
        markdownString.append("\n").append(getPropertyMarkdown(label, value));
    }

    public static void appendPropertyToMarkdown(StringBuilder markdownString, String label, Object value) {
        markdownString.append("\n").append(getPropertyMarkdown(label, value));
    }

    public static String getPropertyMarkdown(String label, Object value) {
        return MarkdownUtils.getSingleLineMarkdownStringEntry(label, value, "-");
    }

    public static void appendLiteralPropertyToMarkdown(StringBuilder markdownString, String label, Object value) {
        markdownString.append("\n").append(getLiteralPropertyMarkdown(label, value));
    }

    public static String getLiteralPropertyMarkdown(String label, Object value) {
        return MarkdownUtils.getLiteralSingleLineMarkdownStringEntry(label, value, "-");
    }



    public static String getCurrentTimeStamp() {
        @SuppressLint("SimpleDateFormat")
        final SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss z");
        df.setTimeZone(TimeZone.getTimeZone("UTC"));
        return df.format(new Date());
    }

    public static String getCurrentMilliSecondUTCTimeStamp() {
        @SuppressLint("SimpleDateFormat")
        final SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS z");
        df.setTimeZone(TimeZone.getTimeZone("UTC"));
        return df.format(new Date());
    }

    public static String getCurrentMilliSecondLocalTimeStamp() {
        @SuppressLint("SimpleDateFormat")
        final SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd_HH.mm.ss.SSS");
        df.setTimeZone(TimeZone.getDefault());
        return df.format(new Date());
    }

}
