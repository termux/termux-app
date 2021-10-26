package com.termux.shared.shell.command.result;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;

import com.termux.shared.R;
import com.termux.shared.data.DataUtils;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.errors.Error;
import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.errors.FunctionErrno;
import com.termux.shared.android.AndroidUtils;
import com.termux.shared.shell.command.ShellCommandConstants.RESULT_SENDER;

public class ResultSender {

    private static final String LOG_TAG = "ResultSender";

    /**
     * Send result stored in {@link ResultConfig} to command caller via
     * {@link ResultConfig#resultPendingIntent} and/or by writing it to files in
     * {@link ResultConfig#resultDirectoryPath}. If both are not {@code null}, then result will be
     * sent via both.
     *
     * @param context The {@link Context} for operations.
     * @param logTag The log tag to use for logging.
     * @param label The label for the command.
     * @param resultConfig The {@link ResultConfig} object containing information on how to send the result.
     * @param resultData The {@link ResultData} object containing result data.
     * @param logStdoutAndStderr Set to {@code true} if {@link ResultData#stdout} and {@link ResultData#stderr}
     *                           should be logged.
     * @return Returns the {@link Error} if failed to send the result, otherwise {@code null}.
     */
    public static Error sendCommandResultData(Context context, String logTag, String label, ResultConfig resultConfig, ResultData resultData, boolean logStdoutAndStderr) {
        if (context == null || resultConfig == null || resultData == null)
            return FunctionErrno.ERRNO_NULL_OR_EMPTY_PARAMETERS.getError("context, resultConfig or resultData", "sendCommandResultData");

        Error error;

        if (resultConfig.resultPendingIntent != null) {
            error = sendCommandResultDataWithPendingIntent(context, logTag, label, resultConfig, resultData, logStdoutAndStderr);
            if (error != null || resultConfig.resultDirectoryPath == null)
                return error;
        }

        if (resultConfig.resultDirectoryPath != null) {
            return sendCommandResultDataToDirectory(context, logTag, label, resultConfig, resultData, logStdoutAndStderr);
        } else {
            return FunctionErrno.ERRNO_UNSET_PARAMETERS.getError("resultConfig.resultPendingIntent or resultConfig.resultDirectoryPath", "sendCommandResultData");
        }
    }

    /**
     * Send result stored in {@link ResultConfig} to command caller via {@link ResultConfig#resultPendingIntent}.
     *
     * @param context The {@link Context} for operations.
     * @param logTag The log tag to use for logging.
     * @param label The label for the command.
     * @param resultConfig The {@link ResultConfig} object containing information on how to send the result.
     * @param resultData The {@link ResultData} object containing result data.
     * @param logStdoutAndStderr Set to {@code true} if {@link ResultData#stdout} and {@link ResultData#stderr}
     *                           should be logged.
     * @return Returns the {@link Error} if failed to send the result, otherwise {@code null}.
     */
    public static Error sendCommandResultDataWithPendingIntent(Context context, String logTag, String label, ResultConfig resultConfig, ResultData resultData, boolean logStdoutAndStderr) {
        if (context == null || resultConfig == null || resultData == null || resultConfig.resultPendingIntent == null || resultConfig.resultBundleKey == null)
            return FunctionErrno.ERRNO_NULL_OR_EMPTY_PARAMETER.getError("context, resultConfig, resultData, resultConfig.resultPendingIntent or resultConfig.resultBundleKey", "sendCommandResultDataWithPendingIntent");

        logTag = DataUtils.getDefaultIfNull(logTag, LOG_TAG);

        Logger.logDebugExtended(logTag, "Sending result for command \"" + label + "\":\n" + resultConfig.toString() + "\n" + ResultData.getResultDataLogString(resultData, logStdoutAndStderr));

        String resultDataStdout = resultData.stdout.toString();
        String resultDataStderr = resultData.stderr.toString();

        String truncatedStdout = null;
        String truncatedStderr = null;

        String stdoutOriginalLength = String.valueOf(resultDataStdout.length());
        String stderrOriginalLength = String.valueOf(resultDataStderr.length());

        // Truncate stdout and stdout to max TRANSACTION_SIZE_LIMIT_IN_BYTES
        if (resultDataStderr.isEmpty()) {
            truncatedStdout = DataUtils.getTruncatedCommandOutput(resultDataStdout, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, false, false, false);
        } else if (resultDataStdout.isEmpty()) {
            truncatedStderr = DataUtils.getTruncatedCommandOutput(resultDataStderr, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES, false, false, false);
        } else {
            truncatedStdout = DataUtils.getTruncatedCommandOutput(resultDataStdout, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES / 2, false, false, false);
            truncatedStderr = DataUtils.getTruncatedCommandOutput(resultDataStderr, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES / 2, false, false, false);
        }

        if (truncatedStdout != null && truncatedStdout.length() < resultDataStdout.length()) {
            Logger.logWarn(logTag, "The result for command \"" + label + "\" stdout length truncated from " + stdoutOriginalLength + " to " + truncatedStdout.length());
            resultDataStdout = truncatedStdout;
        }

        if (truncatedStderr != null && truncatedStderr.length() < resultDataStderr.length()) {
            Logger.logWarn(logTag, "The result for command \"" + label + "\" stderr length truncated from " + stderrOriginalLength + " to " + truncatedStderr.length());
            resultDataStderr = truncatedStderr;
        }

        String resultDataErrmsg = null;
        if (resultData.isStateFailed()) {
            resultDataErrmsg = ResultData.getErrorsListLogString(resultData);
            if (resultDataErrmsg.isEmpty()) resultDataErrmsg = null;
        }

        String errmsgOriginalLength = (resultDataErrmsg == null) ? null : String.valueOf(resultDataErrmsg.length());

        // Truncate error to max TRANSACTION_SIZE_LIMIT_IN_BYTES / 4
        // trim from end to preserve start of stacktraces
        String truncatedErrmsg = DataUtils.getTruncatedCommandOutput(resultDataErrmsg, DataUtils.TRANSACTION_SIZE_LIMIT_IN_BYTES / 4, true, false, false);
        if (truncatedErrmsg != null && truncatedErrmsg.length() < resultDataErrmsg.length()) {
            Logger.logWarn(logTag, "The result for command \"" + label + "\" error length truncated from " + errmsgOriginalLength + " to " + truncatedErrmsg.length());
            resultDataErrmsg = truncatedErrmsg;
        }


        final Bundle resultBundle = new Bundle();
        resultBundle.putString(resultConfig.resultStdoutKey, resultDataStdout);
        resultBundle.putString(resultConfig.resultStdoutOriginalLengthKey, stdoutOriginalLength);
        resultBundle.putString(resultConfig.resultStderrKey, resultDataStderr);
        resultBundle.putString(resultConfig.resultStderrOriginalLengthKey, stderrOriginalLength);
        if (resultData.exitCode != null)
            resultBundle.putInt(resultConfig.resultExitCodeKey, resultData.exitCode);
        resultBundle.putInt(resultConfig.resultErrCodeKey, resultData.getErrCode());
        resultBundle.putString(resultConfig.resultErrmsgKey, resultDataErrmsg);

        Intent resultIntent = new Intent();
        resultIntent.putExtra(resultConfig.resultBundleKey, resultBundle);

        try {
            resultConfig.resultPendingIntent.send(context, Activity.RESULT_OK, resultIntent);
        } catch (PendingIntent.CanceledException e) {
            // The caller doesn't want the result? That's fine, just ignore
            Logger.logDebug(logTag, "The command \"" + label + "\" creator " + resultConfig.resultPendingIntent.getCreatorPackage() + " does not want the results anymore");
        }

        return null;

    }

    /**
     * Send result stored in {@link ResultConfig} to command caller by writing it to files in
     * {@link ResultConfig#resultDirectoryPath}.
     *
     * @param context The {@link Context} for operations.
     * @param logTag The log tag to use for logging.
     * @param label The label for the command.
     * @param resultConfig The {@link ResultConfig} object containing information on how to send the result.
     * @param resultData The {@link ResultData} object containing result data.
     * @param logStdoutAndStderr Set to {@code true} if {@link ResultData#stdout} and {@link ResultData#stderr}
     *                           should be logged.
     * @return Returns the {@link Error} if failed to send the result, otherwise {@code null}.
     */
    public static Error sendCommandResultDataToDirectory(Context context, String logTag, String label, ResultConfig resultConfig, ResultData resultData, boolean logStdoutAndStderr) {
        if (context == null || resultConfig == null || resultData == null || DataUtils.isNullOrEmpty(resultConfig.resultDirectoryPath))
            return FunctionErrno.ERRNO_NULL_OR_EMPTY_PARAMETER.getError("context, resultConfig, resultData or resultConfig.resultDirectoryPath", "sendCommandResultDataToDirectory");

        logTag = DataUtils.getDefaultIfNull(logTag, LOG_TAG);

        Error error;

        String resultDataStdout = resultData.stdout.toString();
        String resultDataStderr = resultData.stderr.toString();

        String resultDataExitCode = "";
        if (resultData.exitCode != null)
            resultDataExitCode = String.valueOf(resultData.exitCode);

        String resultDataErrmsg = null;
        if (resultData.isStateFailed()) {
            resultDataErrmsg = ResultData.getErrorsListLogString(resultData);
        }
        resultDataErrmsg = DataUtils.getDefaultIfNull(resultDataErrmsg, "");

        resultConfig.resultDirectoryPath = FileUtils.getCanonicalPath(resultConfig.resultDirectoryPath, null);

        Logger.logDebugExtended(logTag, "Writing result for command \"" + label + "\":\n" + resultConfig.toString() + "\n" + ResultData.getResultDataLogString(resultData, logStdoutAndStderr));

        // If resultDirectoryPath is not a directory, or is not readable or writable, then just return
        // Creation of missing directory and setting of read, write and execute permissions are
        // only done if resultDirectoryPath is under resultDirectoryAllowedParentPath.
        // We try to set execute permissions, but ignore if they are missing, since only read and write
        // permissions are required for working directories.
        error = FileUtils.validateDirectoryFileExistenceAndPermissions("result", resultConfig.resultDirectoryPath,
            resultConfig.resultDirectoryAllowedParentPath, true,
            FileUtils.APP_WORKING_DIRECTORY_PERMISSIONS, true, true,
            true, true);
        if (error != null) {
            error.appendMessage("\n" + context.getString(R.string.msg_directory_absolute_path, "Result", resultConfig.resultDirectoryPath));
            return error;
        }

        if (resultConfig.resultSingleFile) {
            // If resultFileBasename is null, empty or contains forward slashes "/"
            if (DataUtils.isNullOrEmpty(resultConfig.resultFileBasename) ||
                    resultConfig.resultFileBasename.contains("/")) {
                error = ResultSenderErrno.ERROR_RESULT_FILE_BASENAME_NULL_OR_INVALID.getError(resultConfig.resultFileBasename);
                return error;
            }

            String error_or_output;

            if (resultData.isStateFailed()) {
                try {
                    if (DataUtils.isNullOrEmpty(resultConfig.resultFileErrorFormat)) {
                        error_or_output = String.format(RESULT_SENDER.FORMAT_FAILED_ERR__ERRMSG__STDOUT__STDERR__EXIT_CODE,
                            MarkdownUtils.getMarkdownCodeForString(String.valueOf(resultData.getErrCode()), false),
                            MarkdownUtils.getMarkdownCodeForString(resultDataErrmsg, true),
                            MarkdownUtils.getMarkdownCodeForString(resultDataStdout, true),
                            MarkdownUtils.getMarkdownCodeForString(resultDataStderr, true),
                            MarkdownUtils.getMarkdownCodeForString(resultDataExitCode, false));
                    } else {
                        error_or_output = String.format(resultConfig.resultFileErrorFormat,
                            resultData.getErrCode(), resultDataErrmsg, resultDataStdout, resultDataStderr, resultDataExitCode);
                    }
                } catch (Exception e) {
                    error = ResultSenderErrno.ERROR_FORMAT_RESULT_ERROR_FAILED_WITH_EXCEPTION.getError(e.getMessage());
                    return error;
                }
            } else {
                try {
                    if (DataUtils.isNullOrEmpty(resultConfig.resultFileOutputFormat)) {
                        if (resultDataStderr.isEmpty() && resultDataExitCode.equals("0"))
                            error_or_output = String.format(RESULT_SENDER.FORMAT_SUCCESS_STDOUT, resultDataStdout);
                        else if (resultDataStderr.isEmpty())
                            error_or_output = String.format(RESULT_SENDER.FORMAT_SUCCESS_STDOUT__EXIT_CODE,
                                resultDataStdout,
                                MarkdownUtils.getMarkdownCodeForString(resultDataExitCode, false));
                        else
                            error_or_output = String.format(RESULT_SENDER.FORMAT_SUCCESS_STDOUT__STDERR__EXIT_CODE,
                                MarkdownUtils.getMarkdownCodeForString(resultDataStdout, true),
                                MarkdownUtils.getMarkdownCodeForString(resultDataStderr, true),
                                MarkdownUtils.getMarkdownCodeForString(resultDataExitCode, false));
                    } else {
                        error_or_output = String.format(resultConfig.resultFileOutputFormat,
                            resultDataStdout, resultDataStderr, resultDataExitCode);
                    }
                } catch (Exception e) {
                    error = ResultSenderErrno.ERROR_FORMAT_RESULT_OUTPUT_FAILED_WITH_EXCEPTION.getError(e.getMessage());
                    return error;
                }
            }

            // Write error or output to temp file
            // Check errCode file creation below for explanation for why temp file is used
            String temp_filename = resultConfig.resultFileBasename + "-" + AndroidUtils.getCurrentMilliSecondLocalTimeStamp();
            error = FileUtils.writeTextToFile(temp_filename, resultConfig.resultDirectoryPath + "/" + temp_filename,
                null, error_or_output, false);
            if (error != null) {
                return error;
            }

            // Move error or output temp file to final destination
            error = FileUtils.moveRegularFile("error or output temp file", resultConfig.resultDirectoryPath + "/" + temp_filename,
                resultConfig.resultDirectoryPath + "/" + resultConfig.resultFileBasename, false);
            if (error != null) {
                return error;
            }
        } else {
            String filename;

            // Default to no suffix, useful if user expects result in an empty directory, like created with mktemp
            if (resultConfig.resultFilesSuffix == null)
                resultConfig.resultFilesSuffix = "";

            // If resultFilesSuffix contains forward slashes "/"
            if (resultConfig.resultFilesSuffix.contains("/")) {
                error = ResultSenderErrno.ERROR_RESULT_FILES_SUFFIX_INVALID.getError(resultConfig.resultFilesSuffix);
                return error;
            }

            // Write result to result files under resultDirectoryPath

            // Write stdout to file
            if (!resultDataStdout.isEmpty()) {
                filename = RESULT_SENDER.RESULT_FILE_STDOUT_PREFIX + resultConfig.resultFilesSuffix;
                error = FileUtils.writeTextToFile(filename, resultConfig.resultDirectoryPath + "/" + filename,
                    null, resultDataStdout, false);
                if (error != null) {
                    return error;
                }
            }

            // Write stderr to file
            if (!resultDataStderr.isEmpty()) {
                filename = RESULT_SENDER.RESULT_FILE_STDERR_PREFIX + resultConfig.resultFilesSuffix;
                error = FileUtils.writeTextToFile(filename, resultConfig.resultDirectoryPath + "/" + filename,
                    null, resultDataStderr, false);
                if (error != null) {
                    return error;
                }
            }

            // Write exitCode to file
            if (!resultDataExitCode.isEmpty()) {
                filename = RESULT_SENDER.RESULT_FILE_EXIT_CODE_PREFIX + resultConfig.resultFilesSuffix;
                error = FileUtils.writeTextToFile(filename, resultConfig.resultDirectoryPath + "/" + filename,
                    null, resultDataExitCode, false);
                if (error != null) {
                    return error;
                }
            }

            // Write errmsg to file
            if (resultData.isStateFailed() && !resultDataErrmsg.isEmpty()) {
                filename = RESULT_SENDER.RESULT_FILE_ERRMSG_PREFIX + resultConfig.resultFilesSuffix;
                error = FileUtils.writeTextToFile(filename, resultConfig.resultDirectoryPath + "/" + filename,
                    null, resultDataErrmsg, false);
                if (error != null) {
                    return error;
                }
            }

            // Write errCode to file
            // This must be created after writing to other result files has already finished since
            // caller should wait for this file to be created to be notified that the command has
            // finished and should then start reading from the rest of the result files if they exist.
            // Since there may be a delay between creation of errCode file and writing to it or flushing
            // to disk, we create a temp file first and then move it to the final destination, since
            // caller may otherwise read from an empty file in some cases.

            // Write errCode to temp file
            String temp_filename = RESULT_SENDER.RESULT_FILE_ERR_PREFIX + "-" + AndroidUtils.getCurrentMilliSecondLocalTimeStamp();
            if (!resultConfig.resultFilesSuffix.isEmpty()) temp_filename = temp_filename + "-" + resultConfig.resultFilesSuffix;
            error = FileUtils.writeTextToFile(temp_filename, resultConfig.resultDirectoryPath + "/" + temp_filename,
                null, String.valueOf(resultData.getErrCode()), false);
            if (error != null) {
                return error;
            }

            // Move errCode temp file to final destination
            filename = RESULT_SENDER.RESULT_FILE_ERR_PREFIX + resultConfig.resultFilesSuffix;
            error = FileUtils.moveRegularFile(RESULT_SENDER.RESULT_FILE_ERR_PREFIX + " temp file", resultConfig.resultDirectoryPath + "/" + temp_filename,
                resultConfig.resultDirectoryPath + "/" + filename, false);
            if (error != null) {
                return error;
            }
        }

        return null;
    }

}
