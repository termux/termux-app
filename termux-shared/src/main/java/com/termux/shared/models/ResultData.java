package com.termux.shared.models;

import androidx.annotation.NonNull;

import com.termux.shared.data.DataUtils;
import com.termux.shared.logger.Logger;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.models.errors.Errno;
import com.termux.shared.models.errors.Error;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class ResultData implements Serializable {

    /** The stdout of command. */
    public final StringBuilder stdout = new StringBuilder();
    /** The stderr of command. */
    public final StringBuilder stderr = new StringBuilder();
    /** The exit code of command. */
    public Integer exitCode;

    /** The internal errors list of command. */
    public List<Error> errorsList =  new ArrayList<>();


    public ResultData() {
    }


    public void clearStdout() {
        stdout.setLength(0);
    }

    public StringBuilder prependStdout(String message) {
        return stdout.insert(0, message);
    }

    public StringBuilder prependStdoutLn(String message) {
        return stdout.insert(0, message + "\n");
    }

    public StringBuilder appendStdout(String message) {
        return stdout.append(message);
    }

    public StringBuilder appendStdoutLn(String message) {
        return stdout.append(message).append("\n");
    }


    public void clearStderr() {
        stderr.setLength(0);
    }

    public StringBuilder prependStderr(String message) {
        return stderr.insert(0, message);
    }

    public StringBuilder prependStderrLn(String message) {
        return stderr.insert(0, message + "\n");
    }

    public StringBuilder appendStderr(String message) {
        return stderr.append(message);
    }

    public StringBuilder appendStderrLn(String message) {
        return stderr.append(message).append("\n");
    }


    public synchronized boolean setStateFailed(@NonNull Error error) {
        return setStateFailed(error.getType(), error.getCode(), error.getMessage(), null);
    }

    public synchronized boolean setStateFailed(@NonNull Error error, Throwable throwable) {
        return setStateFailed(error.getType(), error.getCode(), error.getMessage(), Collections.singletonList(throwable));
    }
    public synchronized boolean setStateFailed(@NonNull Error error, List<Throwable> throwablesList) {
        return setStateFailed(error.getType(), error.getCode(), error.getMessage(), throwablesList);
    }

    public synchronized boolean setStateFailed(int code, String message) {
        return setStateFailed(null, code, message, null);
    }

    public synchronized boolean setStateFailed(int code, String message, Throwable throwable) {
        return setStateFailed(null, code, message, Collections.singletonList(throwable));
    }

    public synchronized boolean setStateFailed(int code, String message, List<Throwable> throwablesList) {
        return setStateFailed(null, code, message, throwablesList);
    }

    public synchronized boolean setStateFailed(String type, int code, String message, List<Throwable> throwablesList) {
        if (errorsList == null)
            errorsList =  new ArrayList<>();

        Error error = new Error();
        errorsList.add(error);

        return error.setStateFailed(type, code, message, throwablesList);
    }

    public boolean isStateFailed() {
        if (errorsList != null) {
            for (Error error : errorsList)
                if (error.isStateFailed())
                    return true;
        }

        return false;
    }

    public int getErrCode() {
        if (errorsList != null && errorsList.size() > 0)
            return errorsList.get(errorsList.size() - 1).getCode();
        else
            return Errno.ERRNO_SUCCESS.getCode();
    }


    @NonNull
    @Override
    public String toString() {
        return getResultDataLogString(this, true);
    }

    /**
     * Get a log friendly {@link String} for {@link ResultData} parameters.
     *
     * @param resultData The {@link ResultData} to convert.
     * @param logStdoutAndStderr Set to {@code true} if {@link #stdout} and {@link #stderr} should be logged.
     * @return Returns the log friendly {@link String}.
     */
    public static String getResultDataLogString(final ResultData resultData, boolean logStdoutAndStderr) {
        if (resultData == null) return "null";

        StringBuilder logString = new StringBuilder();

        if (logStdoutAndStderr) {
            logString.append("\n").append(resultData.getStdoutLogString());
            logString.append("\n").append(resultData.getStderrLogString());
        }
        logString.append("\n").append(resultData.getExitCodeLogString());

        logString.append("\n\n").append(getErrorsListLogString(resultData));

        return logString.toString();
    }



    public String getStdoutLogString() {
        if (stdout.toString().isEmpty())
            return Logger.getSingleLineLogStringEntry("Stdout", null, "-");
        else
            return Logger.getMultiLineLogStringEntry("Stdout", DataUtils.getTruncatedCommandOutput(stdout.toString(), Logger.LOGGER_ENTRY_MAX_SAFE_PAYLOAD / 5, false, false, true), "-");
    }

    public String getStderrLogString() {
        if (stderr.toString().isEmpty())
            return Logger.getSingleLineLogStringEntry("Stderr", null, "-");
        else
            return Logger.getMultiLineLogStringEntry("Stderr", DataUtils.getTruncatedCommandOutput(stderr.toString(), Logger.LOGGER_ENTRY_MAX_SAFE_PAYLOAD / 5, false, false, true), "-");
    }

    public String getExitCodeLogString() {
        return Logger.getSingleLineLogStringEntry("Exit Code", exitCode, "-");
    }

    public static String getErrorsListLogString(final ResultData resultData) {
        if (resultData == null) return "null";

        StringBuilder logString = new StringBuilder();

        if (resultData.errorsList != null) {
            for (Error error : resultData.errorsList) {
                if (error.isStateFailed()) {
                    if (!logString.toString().isEmpty())
                        logString.append("\n");
                    logString.append(Error.getErrorLogString(error));
                }
            }
        }

        return logString.toString();
    }

    /**
     * Get a markdown {@link String} for {@link ResultData}.
     *
     * @param resultData The {@link ResultData} to convert.
     * @return Returns the markdown {@link String}.
     */
    public static String getResultDataMarkdownString(final ResultData resultData) {
        if (resultData == null) return "null";

        StringBuilder markdownString = new StringBuilder();

        if (resultData.stdout.toString().isEmpty())
            markdownString.append(MarkdownUtils.getSingleLineMarkdownStringEntry("Stdout", null, "-"));
        else
            markdownString.append(MarkdownUtils.getMultiLineMarkdownStringEntry("Stdout", resultData.stdout.toString(), "-"));

        if (resultData.stderr.toString().isEmpty())
            markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Stderr", null, "-"));
        else
            markdownString.append("\n").append(MarkdownUtils.getMultiLineMarkdownStringEntry("Stderr", resultData.stderr.toString(), "-"));

        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Exit Code", resultData.exitCode, "-"));

        markdownString.append("\n\n").append(getErrorsListMarkdownString(resultData));


        return markdownString.toString();
    }

    public static String getErrorsListMarkdownString(final ResultData resultData) {
        if (resultData == null) return "null";

        StringBuilder markdownString = new StringBuilder();

        if (resultData.errorsList != null) {
            for (Error error : resultData.errorsList) {
                if (error.isStateFailed()) {
                    if (!markdownString.toString().isEmpty())
                        markdownString.append("\n");
                    markdownString.append(Error.getErrorMarkdownString(error));
                }
            }
        }

        return markdownString.toString();
    }

    public static String getErrorsListMinimalString(final ResultData resultData) {
        if (resultData == null) return "null";

        StringBuilder minimalString = new StringBuilder();

        if (resultData.errorsList != null) {
            for (Error error : resultData.errorsList) {
                if (error.isStateFailed()) {
                    if (!minimalString.toString().isEmpty())
                        minimalString.append("\n");
                    minimalString.append(Error.getMinimalErrorString(error));
                }
            }
        }

        return minimalString.toString();
    }

}
