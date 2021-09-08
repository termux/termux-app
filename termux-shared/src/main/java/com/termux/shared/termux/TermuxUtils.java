package com.termux.shared.termux;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;

import androidx.annotation.NonNull;

import com.termux.shared.R;
import com.termux.shared.file.FileUtils;
import com.termux.shared.file.TermuxFileUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.models.ExecutionCommand;
import com.termux.shared.models.errors.Error;
import com.termux.shared.packages.PackageUtils;
import com.termux.shared.shell.TermuxShellEnvironmentClient;
import com.termux.shared.shell.TermuxTask;

import org.apache.commons.io.IOUtils;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.Charset;
import java.util.List;
import java.util.regex.Pattern;

public class TermuxUtils {

    private static final String LOG_TAG = "TermuxUtils";

    /**
     * Get the {@link Context} for {@link TermuxConstants#TERMUX_PACKAGE_NAME} package.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the package.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    public static Context getTermuxPackageContext(@NonNull Context context) {
        return PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_PACKAGE_NAME);
    }

    /**
     * Get the {@link Context} for {@link TermuxConstants#TERMUX_API_PACKAGE_NAME} package.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the package.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    public static Context getTermuxAPIPackageContext(@NonNull Context context) {
        return PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_API_PACKAGE_NAME);
    }

    /**
     * Get the {@link Context} for {@link TermuxConstants#TERMUX_BOOT_PACKAGE_NAME} package.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the package.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    public static Context getTermuxBootPackageContext(@NonNull Context context) {
        return PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_BOOT_PACKAGE_NAME);
    }

    /**
     * Get the {@link Context} for {@link TermuxConstants#TERMUX_FLOAT_PACKAGE_NAME} package.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the package.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    public static Context getTermuxFloatPackageContext(@NonNull Context context) {
        return PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_FLOAT_PACKAGE_NAME);
    }

    /**
     * Get the {@link Context} for {@link TermuxConstants#TERMUX_STYLING_PACKAGE_NAME} package.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the package.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    public static Context getTermuxStylingPackageContext(@NonNull Context context) {
        return PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_STYLING_PACKAGE_NAME);
    }

    /**
     * Get the {@link Context} for {@link TermuxConstants#TERMUX_TASKER_PACKAGE_NAME} package.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the package.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    public static Context getTermuxTaskerPackageContext(@NonNull Context context) {
        return PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_TASKER_PACKAGE_NAME);
    }

    /**
     * Get the {@link Context} for {@link TermuxConstants#TERMUX_WIDGET_PACKAGE_NAME} package.
     *
     * @param context The {@link Context} to use to get the {@link Context} of the package.
     * @return Returns the {@link Context}. This will {@code null} if an exception is raised.
     */
    public static Context getTermuxWidgetPackageContext(@NonNull Context context) {
        return PackageUtils.getContextForPackage(context, TermuxConstants.TERMUX_WIDGET_PACKAGE_NAME);
    }



    /**
     * Check if Termux app is installed and enabled. This can be used by external apps that don't
     * share `sharedUserId` with the Termux app.
     *
     * If your third-party app is targeting sdk `30` (android `11`), then it needs to add `com.termux`
     * package to the `queries` element or request `QUERY_ALL_PACKAGES` permission in its
     * `AndroidManifest.xml`. Otherwise it will get `PackageSetting{...... com.termux/......} BLOCKED`
     * errors in `logcat` and `RUN_COMMAND` won't work.
     * Check [package-visibility](https://developer.android.com/training/basics/intents/package-visibility#package-name),
     * `QUERY_ALL_PACKAGES` [googleplay policy](https://support.google.com/googleplay/android-developer/answer/10158779
     * and this [article](https://medium.com/androiddevelopers/working-with-package-visibility-dc252829de2d) for more info.
     *
     * {@code
     * <manifest
     *     <queries>
     *         <package android:name="com.termux" />
     *    </queries>
     * </manifest>
     * }
     *
     * @param context The context for operations.
     * @return Returns {@code errmsg} if {@link TermuxConstants#TERMUX_PACKAGE_NAME} is not installed
     * or disabled, otherwise {@code null}.
     */
    public static String isTermuxAppInstalled(@NonNull final Context context) {
        return PackageUtils.isAppInstalled(context, TermuxConstants.TERMUX_APP_NAME, TermuxConstants.TERMUX_PACKAGE_NAME);
    }

    /**
     * Check if Termux:API app is installed and enabled. This can be used by external apps that don't
     * share `sharedUserId` with the Termux:API app.
     *
     * @param context The context for operations.
     * @return Returns {@code errmsg} if {@link TermuxConstants#TERMUX_API_PACKAGE_NAME} is not installed
     * or disabled, otherwise {@code null}.
     */
    public static String isTermuxAPIAppInstalled(@NonNull final Context context) {
        return PackageUtils.isAppInstalled(context, TermuxConstants.TERMUX_API_APP_NAME, TermuxConstants.TERMUX_API_PACKAGE_NAME);
    }

    /**
     * Check if Termux app is installed and accessible. This can only be used by apps that share
     * `sharedUserId` with the Termux app.
     *
     * This is done by checking if first checking if app is installed and enabled and then if
     * {@code currentPackageContext} can be used to get the {@link Context} of the app with
     * {@link TermuxConstants#TERMUX_PACKAGE_NAME} and then if
     * {@link TermuxConstants#TERMUX_PREFIX_DIR_PATH} exists and has
     * {@link FileUtils#APP_WORKING_DIRECTORY_PERMISSIONS} permissions. The directory will not
     * be automatically created and neither the missing permissions automatically set.
     *
     * @param currentPackageContext The context of current package.
     * @return Returns {@code errmsg} if failed to get termux package {@link Context} or
     *         {@link TermuxConstants#TERMUX_PREFIX_DIR_PATH} is accessible, otherwise {@code null}.
     */
    public static String isTermuxAppAccessible(@NonNull final Context currentPackageContext) {
        String errmsg = isTermuxAppInstalled(currentPackageContext);
        if (errmsg == null) {
            Context termuxPackageContext = TermuxUtils.getTermuxPackageContext(currentPackageContext);
            // If failed to get Termux app package context
            if (termuxPackageContext == null)
                errmsg = currentPackageContext.getString(R.string.error_termux_app_package_context_not_accessible);

            if (errmsg == null) {
                // If TermuxConstants.TERMUX_PREFIX_DIR_PATH is not a directory or does not have required permissions
                Error error = TermuxFileUtils.isTermuxPrefixDirectoryAccessible(false, false);
                if (error != null)
                    errmsg = currentPackageContext.getString(R.string.error_termux_prefix_dir_path_not_accessible,
                        PackageUtils.getAppNameForPackage(currentPackageContext));
            }
        }

        if (errmsg != null)
            return errmsg + " " + currentPackageContext.getString(R.string.msg_termux_app_required_by_app,
                PackageUtils.getAppNameForPackage(currentPackageContext));
        else
            return null;
    }



    /**
     * Send the {@link TermuxConstants#BROADCAST_TERMUX_OPENED} broadcast to notify apps that Termux
     * app has been opened.
     *
     * @param context The Context to send the broadcast.
     */
    public static void sendTermuxOpenedBroadcast(@NonNull Context context) {
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
     * Get a markdown {@link String} for the apps info of all/any termux plugin apps installed.
     *
     * @param currentPackageContext The context of current package.
     * @return Returns the markdown {@link String}.
     */
    public static String getTermuxPluginAppsInfoMarkdownString(final Context currentPackageContext) {
        if (currentPackageContext == null) return "null";

        StringBuilder markdownString = new StringBuilder();

        List<String> termuxPluginAppPackageNamesList = TermuxConstants.TERMUX_PLUGIN_APP_PACKAGE_NAMES_LIST;

        if (termuxPluginAppPackageNamesList != null) {
            for (int i = 0; i < termuxPluginAppPackageNamesList.size(); i++) {
                String termuxPluginAppPackageName = termuxPluginAppPackageNamesList.get(i);
                Context termuxPluginAppContext = PackageUtils.getContextForPackage(currentPackageContext, termuxPluginAppPackageName);
                // If the package context for the plugin app is not null, then assume its installed and get its info
                if (termuxPluginAppContext != null) {
                    if (i != 0)
                        markdownString.append("\n\n");
                    markdownString.append(getAppInfoMarkdownString(termuxPluginAppContext, false));
                }
            }
        }

        if (markdownString.toString().isEmpty())
            return null;

        return markdownString.toString();
    }

    /**
     * Get a markdown {@link String} for the app info. If the {@code context} passed is different
     * from the {@link TermuxConstants#TERMUX_PACKAGE_NAME} package context, then this function
     * must have been called by a different package like a plugin, so we return info for both packages
     * if {@code returnTermuxPackageInfoToo} is {@code true}.
     *
     * @param currentPackageContext The context of current package.
     * @param returnTermuxPackageInfoToo If set to {@code true}, then will return info of the
     * {@link TermuxConstants#TERMUX_PACKAGE_NAME} package as well if its different from current package.
     * @return Returns the markdown {@link String}.
     */
    public static String getAppInfoMarkdownString(final Context currentPackageContext, final boolean returnTermuxPackageInfoToo) {
        if (currentPackageContext == null) return "null";

        StringBuilder markdownString = new StringBuilder();

        Context termuxPackageContext = getTermuxPackageContext(currentPackageContext);

        String termuxPackageName = null;
        String termuxAppName = null;
        if (termuxPackageContext != null) {
            termuxPackageName = PackageUtils.getPackageNameForPackage(termuxPackageContext);
            termuxAppName = PackageUtils.getAppNameForPackage(termuxPackageContext);
        }

        String currentPackageName = PackageUtils.getPackageNameForPackage(currentPackageContext);
        String currentAppName = PackageUtils.getAppNameForPackage(currentPackageContext);

        boolean isTermuxPackage = (termuxPackageName != null && termuxPackageName.equals(currentPackageName));


        if (returnTermuxPackageInfoToo && !isTermuxPackage)
            markdownString.append("## ").append(currentAppName).append(" App Info (Current)\n");
        else
            markdownString.append("## ").append(currentAppName).append(" App Info\n");
        markdownString.append(getAppInfoMarkdownStringInner(currentPackageContext));

        if (returnTermuxPackageInfoToo && termuxPackageContext != null && !isTermuxPackage) {
            markdownString.append("\n\n## ").append(termuxAppName).append(" App Info\n");
            markdownString.append(getAppInfoMarkdownStringInner(termuxPackageContext));
        }

        markdownString.append("\n##\n");

        return markdownString.toString();
    }

    /**
     * Get a markdown {@link String} for the app info for the package associated with the {@code context}.
     *
     * @param context The context for operations for the package.
     * @return Returns the markdown {@link String}.
     */
    public static String getAppInfoMarkdownStringInner(@NonNull final Context context) {
        StringBuilder markdownString = new StringBuilder();

        markdownString.append((AndroidUtils.getAppInfoMarkdownString(context)));

        Error error;
        error = TermuxFileUtils.isTermuxFilesDirectoryAccessible(context, true, true);
        if (error != null) {
            AndroidUtils.appendPropertyToMarkdown(markdownString, "TERMUX_FILES_DIR", TermuxConstants.TERMUX_FILES_DIR_PATH);
            AndroidUtils.appendPropertyToMarkdown(markdownString, "IS_TERMUX_FILES_DIR_ACCESSIBLE", "false - " + Error.getMinimalErrorString(error));
        }

        String signingCertificateSHA256Digest = PackageUtils.getSigningCertificateSHA256DigestForPackage(context);
        if (signingCertificateSHA256Digest != null) {
            AndroidUtils.appendPropertyToMarkdown(markdownString,"APK_RELEASE", getAPKRelease(signingCertificateSHA256Digest));
            AndroidUtils.appendPropertyToMarkdown(markdownString,"SIGNING_CERTIFICATE_SHA256_DIGEST", signingCertificateSHA256Digest);
        }

        return markdownString.toString();
    }

    /**
     * Get a markdown {@link String} for reporting an issue.
     *
     * @param context The context for operations.
     * @return Returns the markdown {@link String}.
     */
    public static String getReportIssueMarkdownString(@NonNull final Context context) {
        if (context == null) return "null";

        StringBuilder markdownString = new StringBuilder();

        markdownString.append("## Where To Report An Issue");

        markdownString.append("\n\n").append(context.getString(R.string.msg_report_issue, TermuxConstants.TERMUX_WIKI_URL)).append("\n");

        markdownString.append("\n\n### Email\n");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_SUPPORT_EMAIL_URL, TermuxConstants.TERMUX_SUPPORT_EMAIL_MAILTO_URL)).append("  ");

        markdownString.append("\n\n### Reddit\n");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_REDDIT_SUBREDDIT, TermuxConstants.TERMUX_REDDIT_SUBREDDIT_URL)).append("  ");

        markdownString.append("\n\n### Github Issues for Termux apps\n");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_APP_NAME, TermuxConstants.TERMUX_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_API_APP_NAME, TermuxConstants.TERMUX_API_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_BOOT_APP_NAME, TermuxConstants.TERMUX_BOOT_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_FLOAT_APP_NAME, TermuxConstants.TERMUX_FLOAT_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_STYLING_APP_NAME, TermuxConstants.TERMUX_STYLING_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_TASKER_APP_NAME, TermuxConstants.TERMUX_TASKER_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_WIDGET_APP_NAME, TermuxConstants.TERMUX_WIDGET_GITHUB_ISSUES_REPO_URL)).append("  ");

        markdownString.append("\n\n### Github Issues for Termux packages\n");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_PACKAGES_GITHUB_REPO_NAME, TermuxConstants.TERMUX_PACKAGES_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_GAME_PACKAGES_GITHUB_REPO_NAME, TermuxConstants.TERMUX_GAME_PACKAGES_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_SCIENCE_PACKAGES_GITHUB_REPO_NAME, TermuxConstants.TERMUX_SCIENCE_PACKAGES_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_ROOT_PACKAGES_GITHUB_REPO_NAME, TermuxConstants.TERMUX_ROOT_PACKAGES_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_UNSTABLE_PACKAGES_GITHUB_REPO_NAME, TermuxConstants.TERMUX_UNSTABLE_PACKAGES_GITHUB_ISSUES_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_X11_PACKAGES_GITHUB_REPO_NAME, TermuxConstants.TERMUX_X11_PACKAGES_GITHUB_ISSUES_REPO_URL)).append("  ");

        markdownString.append("\n##\n");

        return markdownString.toString();
    }

    /**
     * Get a markdown {@link String} for important links.
     *
     * @param context The context for operations.
     * @return Returns the markdown {@link String}.
     */
    public static String getImportantLinksMarkdownString(@NonNull final Context context) {
        if (context == null) return "null";

        StringBuilder markdownString = new StringBuilder();

        markdownString.append("## Important Links");

        markdownString.append("\n\n### Github\n");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_APP_NAME, TermuxConstants.TERMUX_GITHUB_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_API_APP_NAME, TermuxConstants.TERMUX_API_GITHUB_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_BOOT_APP_NAME, TermuxConstants.TERMUX_BOOT_GITHUB_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_FLOAT_APP_NAME, TermuxConstants.TERMUX_FLOAT_GITHUB_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_STYLING_APP_NAME, TermuxConstants.TERMUX_STYLING_GITHUB_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_TASKER_APP_NAME, TermuxConstants.TERMUX_TASKER_GITHUB_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_WIDGET_APP_NAME, TermuxConstants.TERMUX_WIDGET_GITHUB_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_PACKAGES_GITHUB_REPO_NAME, TermuxConstants.TERMUX_PACKAGES_GITHUB_REPO_URL)).append("  ");

        markdownString.append("\n\n### Email\n");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_SUPPORT_EMAIL_URL, TermuxConstants.TERMUX_SUPPORT_EMAIL_MAILTO_URL)).append("  ");

        markdownString.append("\n\n### Reddit\n");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_REDDIT_SUBREDDIT, TermuxConstants.TERMUX_REDDIT_SUBREDDIT_URL)).append("  ");

        markdownString.append("\n\n### Wiki\n");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_WIKI, TermuxConstants.TERMUX_WIKI_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_APP_NAME, TermuxConstants.TERMUX_GITHUB_WIKI_REPO_URL)).append("  ");
        markdownString.append("\n").append(MarkdownUtils.getLinkMarkdownString(TermuxConstants.TERMUX_PACKAGES_GITHUB_REPO_NAME, TermuxConstants.TERMUX_PACKAGES_GITHUB_WIKI_REPO_URL)).append("  ");

        markdownString.append("\n##\n");

        return markdownString.toString();
    }



    /**
     * Get a markdown {@link String} for APT info of the app.
     *
     * This will take a few seconds to run due to running {@code apt update} command.
     *
     * @param context The context for operations.
     * @return Returns the markdown {@link String}.
     */
    public static String geAPTInfoMarkdownString(@NonNull final Context context) {

        String aptInfoScript;
        InputStream inputStream = context.getResources().openRawResource(com.termux.shared.R.raw.apt_info_script);
        try {
            aptInfoScript = IOUtils.toString(inputStream, Charset.defaultCharset());
        } catch (IOException e) {
            Logger.logError(LOG_TAG, "Failed to get APT info script: " + e.getMessage());
            return null;
        }

        IOUtils.closeQuietly(inputStream);

        if (aptInfoScript == null || aptInfoScript.isEmpty()) {
            Logger.logError(LOG_TAG, "The APT info script is null or empty");
            return null;
        }

        aptInfoScript = aptInfoScript.replaceAll(Pattern.quote("@TERMUX_PREFIX@"), TermuxConstants.TERMUX_PREFIX_DIR_PATH);

        ExecutionCommand executionCommand = new ExecutionCommand(1, TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH + "/bash", null, aptInfoScript, null, true, false);
        executionCommand.commandLabel = "APT Info Command";
        executionCommand.backgroundCustomLogLevel = Logger.LOG_LEVEL_OFF;
        TermuxTask termuxTask = TermuxTask.execute(context, executionCommand, null, new TermuxShellEnvironmentClient(), true);
        if (termuxTask == null || !executionCommand.isSuccessful() || executionCommand.resultData.exitCode != 0) {
            Logger.logErrorExtended(LOG_TAG, executionCommand.toString());
            return null;
        }

        if (!executionCommand.resultData.stderr.toString().isEmpty())
            Logger.logErrorExtended(LOG_TAG, executionCommand.toString());

        StringBuilder markdownString = new StringBuilder();

        markdownString.append("## ").append(TermuxConstants.TERMUX_APP_NAME).append(" APT Info\n\n");
        markdownString.append(executionCommand.resultData.stdout.toString());
        markdownString.append("\n##\n");

        return markdownString.toString();
    }

    /**
     * Get a markdown {@link String} for info for termux debugging.
     *
     * @param context The context for operations.
     * @return Returns the markdown {@link String}.
     */
    public static String getTermuxDebugMarkdownString(@NonNull final Context context) {
        String statInfo = TermuxFileUtils.getTermuxFilesStatMarkdownString(context);
        String logcatInfo = getLogcatDumpMarkdownString(context);

        if (statInfo != null && logcatInfo != null)
            return statInfo + "\n\n" + logcatInfo;
        else if (statInfo != null)
            return statInfo;
        else
            return logcatInfo;

    }

    /**
     * Get a markdown {@link String} for logcat command dump.
     *
     * @param context The context for operations.
     * @return Returns the markdown {@link String}.
     */
    public static String getLogcatDumpMarkdownString(@NonNull final Context context) {
        // Build script
        // We need to prevent OutOfMemoryError since StreamGobbler StringBuilder + StringBuilder.toString()
        // may require lot of memory if dump is too large.
        // Putting a limit at 3000 lines. Assuming average 160 chars/line will result in 500KB usage
        // per object.
        // That many lines should be enough for debugging for recent issues anyways assuming termux
        // has not been granted READ_LOGS permission s.
        String logcatScript = "/system/bin/logcat -d -t 3000 2>&1";

        // Run script
        // Logging must be disabled for output of logcat command itself in StreamGobbler
        ExecutionCommand executionCommand = new ExecutionCommand(1, "/system/bin/sh", null, logcatScript + "\n", "/", true, true);
        executionCommand.commandLabel = "Logcat dump command";
        executionCommand.backgroundCustomLogLevel = Logger.LOG_LEVEL_OFF;
        TermuxTask termuxTask = TermuxTask.execute(context, executionCommand, null, new TermuxShellEnvironmentClient(), true);
        if (termuxTask == null || !executionCommand.isSuccessful()) {
            Logger.logErrorExtended(LOG_TAG, executionCommand.toString());
            return null;
        }

        // Build script output
        StringBuilder logcatOutput = new StringBuilder();
        logcatOutput.append("$ ").append(logcatScript);
        logcatOutput.append("\n").append(executionCommand.resultData.stdout.toString());

        boolean stderrSet = !executionCommand.resultData.stderr.toString().isEmpty();
        if (executionCommand.resultData.exitCode != 0 || stderrSet) {
            Logger.logErrorExtended(LOG_TAG, executionCommand.toString());
            if (stderrSet)
                logcatOutput.append("\n").append(executionCommand.resultData.stderr.toString());
            logcatOutput.append("\n").append("exit code: ").append(executionCommand.resultData.exitCode.toString());
        }

        // Build markdown output
        StringBuilder markdownString = new StringBuilder();
        markdownString.append("## Logcat Dump\n\n");
        markdownString.append("\n\n").append(MarkdownUtils.getMarkdownCodeForString(logcatOutput.toString(), true));
        markdownString.append("\n##\n");

        return markdownString.toString();
    }



    public static String getAPKRelease(String signingCertificateSHA256Digest) {
        if (signingCertificateSHA256Digest == null) return "null";

        switch (signingCertificateSHA256Digest.toUpperCase()) {
            case TermuxConstants.APK_RELEASE_FDROID_SIGNING_CERTIFICATE_SHA256_DIGEST:
                return TermuxConstants.APK_RELEASE_FDROID;
            case TermuxConstants.APK_RELEASE_GITHUB_SIGNING_CERTIFICATE_SHA256_DIGEST:
                return TermuxConstants.APK_RELEASE_GITHUB;
            case TermuxConstants.APK_RELEASE_GOOGLE_PLAYSTORE_SIGNING_CERTIFICATE_SHA256_DIGEST:
                return TermuxConstants.APK_RELEASE_GOOGLE_PLAYSTORE;
            default:
                return "Unknown";
        }
    }


    /**
     * Get a process id of the main app process of the {@link TermuxConstants#TERMUX_PACKAGE_NAME}
     * package.
     *
     * @param context The context for operations.
     * @return Returns the process if found and running, otherwise {@code null}.
     */
    public static String getTermuxAppPID(final Context context) {
        return PackageUtils.getPackagePID(context, TermuxConstants.TERMUX_PACKAGE_NAME);
    }

}
