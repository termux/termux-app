package com.termux.app.utils;

import android.annotation.SuppressLint;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ResolveInfo;
import android.os.Build;

import androidx.annotation.NonNull;

import com.google.common.base.Joiner;
import com.termux.BuildConfig;

import com.termux.app.TermuxConstants;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.List;
import java.util.Properties;
import java.util.TimeZone;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

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

    /**
     * Get a markdown {@link String} for the device details.
     *
     * @param context The context for operations.
     * @return Returns the markdown {@link String}.
     */
    public static String getDeviceDetailsMarkdownString(final Context context) {
        if (context == null) return "null";

        // Some properties cannot be read with {@link System#getProperty(String)} but can be read
        // directly by running getprop command
        Properties systemProperties = getSystemProperties();

        StringBuilder markdownString = new StringBuilder();

        markdownString.append("#### Software\n");
        appendPropertyMarkdown(markdownString,
            TermuxConstants.TERMUX_APP_NAME.toUpperCase() + "_VERSION", getTermuxAppVersionName() + "(" + getTermuxAppVersionCode() + ")");
        appendPropertyMarkdown(markdownString,TermuxConstants.TERMUX_APP_NAME.toUpperCase() + "_DEBUG_BUILD", isTermuxAppDebugBuild());
        appendPropertyMarkdown(markdownString,"OS_VERSION", getSystemPropertyWithAndroidAPI("os.version"));
        appendPropertyMarkdown(markdownString, "SDK_INT", Build.VERSION.SDK_INT);
        // If its a release version
        if ("REL".equals(Build.VERSION.CODENAME))
            appendPropertyMarkdown(markdownString, "RELEASE", Build.VERSION.RELEASE);
        else
            appendPropertyMarkdown(markdownString, "CODENAME", Build.VERSION.CODENAME);
        appendPropertyMarkdown(markdownString, "INCREMENTAL", Build.VERSION.INCREMENTAL);
        appendPropertyMarkdownIfSet(markdownString, "SECURITY_PATCH", systemProperties.getProperty("ro.build.version.security_patch"));
        appendPropertyMarkdownIfSet(markdownString, "IS_DEBUGGABLE", systemProperties.getProperty("ro.debuggable"));
        appendPropertyMarkdownIfSet(markdownString, "IS_EMULATOR", systemProperties.getProperty("ro.boot.qemu"));
        appendPropertyMarkdownIfSet(markdownString, "IS_TREBLE_ENABLED", systemProperties.getProperty("ro.treble.enabled"));
        appendPropertyMarkdown(markdownString, "TYPE", Build.TYPE);
        appendPropertyMarkdown(markdownString, "TAGS", Build.TAGS);

        markdownString.append("\n\n#### Hardware\n");
        appendPropertyMarkdown(markdownString, "MANUFACTURER", Build.MANUFACTURER);
        appendPropertyMarkdown(markdownString, "BRAND", Build.BRAND);
        appendPropertyMarkdown(markdownString, "MODEL", Build.MODEL);
        appendPropertyMarkdown(markdownString, "PRODUCT", Build.PRODUCT);
        appendPropertyMarkdown(markdownString, "DISPLAY", Build.DISPLAY);
        appendPropertyMarkdown(markdownString, "ID", Build.ID);
        appendPropertyMarkdown(markdownString, "BOARD", Build.BOARD);
        appendPropertyMarkdown(markdownString, "HARDWARE", Build.HARDWARE);
        appendPropertyMarkdown(markdownString, "DEVICE", Build.DEVICE);
        appendPropertyMarkdown(markdownString, "SUPPORTED_ABIS", Joiner.on(", ").skipNulls().join(Build.SUPPORTED_ABIS));

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
                    if(key != null && value != null && !key.isEmpty() && !value.isEmpty())
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

    private static String getSystemPropertyWithAndroidAPI(@NonNull String property) {
        try {
            return System.getProperty(property);
        } catch (Exception e) {
            Logger.logVerbose("Failed to get system property \"" + property + "\":" + e.getMessage());
            return null;
        }
    }

    private static void appendPropertyMarkdownIfSet(StringBuilder markdownString, String label, Object value) {
        if(value == null) return;
        if(value instanceof String && (((String) value).isEmpty()) || "REL".equals(value)) return;
        markdownString.append("\n").append(getPropertyMarkdown(label, value));
    }

    private static void appendPropertyMarkdown(StringBuilder markdownString, String label, Object value) {
        markdownString.append("\n").append(getPropertyMarkdown(label, value));
    }

    private static String getPropertyMarkdown(String label, Object value) {
        return MarkdownUtils.getSingleLineMarkdownStringEntry(label, value, "-");
    }



    public static int getTermuxAppVersionCode() {
        return BuildConfig.VERSION_CODE;
    }

    public static String getTermuxAppVersionName() {
        return BuildConfig.VERSION_NAME;
    }

    public static boolean isTermuxAppDebugBuild() {
        return BuildConfig.DEBUG;
    }

    public static String getCurrentTimeStamp() {
        @SuppressLint("SimpleDateFormat")
        final SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd HH:mm");
        df.setTimeZone(TimeZone.getTimeZone("UTC"));
        return df.format(new Date());
    }


}
