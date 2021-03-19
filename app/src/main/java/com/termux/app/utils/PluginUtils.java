package com.termux.app.utils;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import com.termux.R;
import com.termux.app.TermuxConstants;
import com.termux.app.TermuxConstants.TERMUX_APP.TERMUX_SERVICE;
import com.termux.app.settings.properties.SharedProperties;
import com.termux.app.settings.properties.TermuxPropertyConstants;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class PluginUtils {

    /** Plugin variable for stdout value of termux command */
    public static final String PLUGIN_VARIABLE_STDOUT = "%stdout"; // Default: "%stdout"
    /** Plugin variable for stderr value of termux command */
    public static final String PLUGIN_VARIABLE_STDERR = "%stderr"; // Default: "%stderr"
    /** Plugin variable for exit code value of termux command */
    public static final String PLUGIN_VARIABLE_EXIT_CODE = "%result"; // Default: "%result"
    /** Plugin variable for err value of termux command */
    public static final String PLUGIN_VARIABLE_ERR = "%err"; // Default: "%err"
    /** Plugin variable for errmsg value of termux command */
    public static final String PLUGIN_VARIABLE_ERRMSG = "%errmsg"; // Default: "%errmsg"

    /** Intent {@code Parcelable} extra containing original intent received from plugin host app by FireReceiver */
    public static final String EXTRA_ORIGINAL_INTENT = "originalIntent"; // Default: "originalIntent"



    /** Required file permissions for the executable file of execute intent. Executable file must have read and execute permissions */
    public static final String PLUGIN_EXECUTABLE_FILE_PERMISSIONS = "r-x"; // Default: "r-x"
    /** Required file permissions for the working directory of execute intent. Working directory must have read and write permissions.
     * Execute permissions should be attempted to be set, but ignored if they are missing */
    public static final String PLUGIN_WORKING_DIRECTORY_PERMISSIONS = "rwx"; // Default: "rwx"



    /**
     * A regex to validate if a string matches a valid plugin host variable name with the percent sign "%" prefix.
     * Valid values: A string containing a percent sign character "%", followed by 1 alphanumeric character,
     * followed by 2 or more alphanumeric or underscore "_" characters but does not end with an underscore "_"
     */
    public static final String PLUGIN_HOST_VARIABLE_NAME_MATCH_EXPRESSION =  "%[a-zA-Z0-9][a-zA-Z0-9_]{2,}(?<!_)";



    private static final String LOG_TAG = "PluginUtils";

    /**
     * Send execution result of commands to the {@link PendingIntent} creator received by
     * execution service if {@code pendingIntent} is not {@code null}
     *
     * @param logLevel The log level to dump the result.
     * @param logTag The log tag to use for logging.
     * @param context The {@link Context} that will be used to send result intent to the PluginResultsService.
     * @param pendingIntent The {@link PendingIntent} sent by creator to the execution service.
     * @param stdout The value for {@link TERMUX_SERVICE#EXTRA_STDOUT} extra of {@link TERMUX_SERVICE#EXTRA_RESULT_BUNDLE} bundle of intent.
     * @param stderr The value for {@link TERMUX_SERVICE#EXTRA_STDERR} extra of {@link TERMUX_SERVICE#EXTRA_RESULT_BUNDLE} bundle of intent.
     * @param exitCode The value for {@link TERMUX_SERVICE#EXTRA_EXIT_CODE} extra of {@link TERMUX_SERVICE#EXTRA_RESULT_BUNDLE} bundle of intent.
     * @param errCode The value for {@link TERMUX_SERVICE#EXTRA_ERR} extra of {@link TERMUX_SERVICE#EXTRA_RESULT_BUNDLE} bundle of intent.
     * @param errmsg The value for {@link TERMUX_SERVICE#EXTRA_ERRMSG} extra of {@link TERMUX_SERVICE#EXTRA_RESULT_BUNDLE} bundle of intent.
     */
    public static void sendExecuteResultToResultsService(final int logLevel, final String logTag, final Context context, final PendingIntent pendingIntent, final String stdout, final String stderr, final String exitCode, final String errCode, final String errmsg) {
        String label;

        if(pendingIntent == null)
            label = "Execution Result";
        else
            label = "Sending execution result to " + pendingIntent.getCreatorPackage();

        Logger.logMesssage(logLevel, logTag, label + ":\n" +
            TERMUX_SERVICE.EXTRA_STDOUT + ":\n```\n" + stdout + "\n```\n" +
            TERMUX_SERVICE.EXTRA_STDERR + ":\n```\n" + stderr + "\n```\n" +
            TERMUX_SERVICE.EXTRA_EXIT_CODE + ": `" + exitCode + "`\n" +
            TERMUX_SERVICE.EXTRA_ERR + ": `" + errCode + "`\n" +
            TERMUX_SERVICE.EXTRA_ERRMSG + ": `" + errmsg + "`");

        // If pendingIntent is null, then just return
        if(pendingIntent == null) return;

        final Bundle resultBundle = new Bundle();

        resultBundle.putString(TERMUX_SERVICE.EXTRA_STDOUT, stdout);
        resultBundle.putString(TERMUX_SERVICE.EXTRA_STDERR, stderr);
        if (exitCode != null && !exitCode.isEmpty()) resultBundle.putInt(TERMUX_SERVICE.EXTRA_EXIT_CODE, Integer.parseInt(exitCode));
        if (errCode != null && !errCode.isEmpty()) resultBundle.putInt(TERMUX_SERVICE.EXTRA_ERR, Integer.parseInt(errCode));
        resultBundle.putString(TERMUX_SERVICE.EXTRA_ERRMSG, errmsg);

        Intent resultIntent = new Intent();
        resultIntent.putExtra(TERMUX_SERVICE.EXTRA_RESULT_BUNDLE, resultBundle);

        if(context != null) {
            try {
                pendingIntent.send(context, Activity.RESULT_OK, resultIntent);
            } catch (PendingIntent.CanceledException e) {
                // The caller doesn't want the result? That's fine, just ignore
            }
        }
    }

    /**
     * Check if {@link TermuxConstants#PROP_ALLOW_EXTERNAL_APPS} property is not set to "true".
     *
     * @param context The {@link Context} to get error string.
     * @return Returns the {@code errmsg} if policy is violated, otherwise {@code null}.
     */
    public static String checkIfRunCommandServiceAllowExternalAppsPolicyIsViolated(final Context context) {
        String errmsg = null;
        if (!SharedProperties.isPropertyValueTrue(context, TermuxPropertyConstants.getTermuxPropertiesFile(), TermuxConstants.PROP_ALLOW_EXTERNAL_APPS)) {
            errmsg = context.getString(R.string.run_command_service_allow_external_apps_ungranted_warning);
        }

        return errmsg;
    }

    /**
     * Send execution result of commands to the {@link PendingIntent} creator received by
     * execution service if {@code pendingIntent} is not {@code null}
     *
     * @param logLevel The log level to dump the result.
     * @param logTag The log tag to use for logging.
     * @param executable The executable received.
     * @param arguments_list The arguments list for executable received.
     * @param workingDirectory The working directory for the command received.
     * @param inBackground The command should be run in background.
     * @param additionalExtras The {@link HashMap} for additional extras received. The key will be
     *                         used as the label to log the value. The object will be converted
     *                        to {@link String} with a call to {@code value.toString()}.
     */
    public static void dumpExecutionIntentToLog(int logLevel, String logTag, String label, String executable, List<String> arguments_list, String workingDirectory, boolean inBackground, HashMap<String, Object> additionalExtras) {
        if (label == null) label = "Execution Intent";

        StringBuilder executionIntentDump = new StringBuilder();

        executionIntentDump.append(label).append(":\n");
        executionIntentDump.append("Executable: `").append(executable).append("`\n");
        executionIntentDump.append("Arguments:").append(getArgumentsStringForLog(arguments_list)).append("\n");
        executionIntentDump.append("Working Directory: `").append(workingDirectory).append("`\n");
        executionIntentDump.append("inBackground: `").append(inBackground).append("`");

        if(additionalExtras != null) {
            for (Map.Entry<String, Object> entry : additionalExtras.entrySet()) {
                executionIntentDump.append("\n").append(entry.getKey()).append(": `").append(entry.getValue()).append("`");
            }
        }

        Logger.logMesssage(logLevel, logTag, executionIntentDump.toString());
    }

    /**
     * Converts arguments list to log friendly format. If arguments are null or of size 0, then
     * nothing is returned. Otherwise following format is returned:
     *
     * ```
     * Arg 0: `value`
     * Arg 1: 'value`
     * ```
     *
     * @param arguments_list The arguments list.
     * @return Returns the formatted arguments list.
     */
    public static String getArgumentsStringForLog(List<String> arguments_list) {
        if (arguments_list==null || arguments_list.size() == 0) return "";

        StringBuilder arguments_list_string = new StringBuilder("\n```\n");
        for(int i = 0; i != arguments_list.size(); i++) {
            arguments_list_string.append("Arg ").append(i).append(": `").append(arguments_list.get(i)).append("`\n");
        }
        arguments_list_string.append("```");

        return arguments_list_string.toString();
    }
}
