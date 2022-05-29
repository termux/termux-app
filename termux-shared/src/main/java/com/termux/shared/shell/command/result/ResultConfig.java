package com.termux.shared.shell.command.result;

import android.app.PendingIntent;
import android.util.Pair;

import androidx.annotation.NonNull;

import com.termux.shared.logger.Logger;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.models.ReportInfo;

import java.util.ArrayList;
import java.util.Formatter;
import java.util.List;

public class ResultConfig {

    /** Defines {@link PendingIntent} that should be sent with the result of the command. We cannot
     * implement {@link java.io.Serializable} because {@link PendingIntent} cannot be serialized. */
    public PendingIntent resultPendingIntent;
    /** The key with which to send result {@link android.os.Bundle} in {@link #resultPendingIntent}. */
    public String resultBundleKey;
    /** The key with which to send {@link ResultData#stdout} in {@link #resultPendingIntent}. */
    public String resultStdoutKey;
    /** The key with which to send {@link ResultData#stderr} in {@link #resultPendingIntent}. */
    public String resultStderrKey;
    /** The key with which to send {@link ResultData#exitCode} in {@link #resultPendingIntent}. */
    public String resultExitCodeKey;
    /** The key with which to send {@link ResultData#errorsList} errCode in {@link #resultPendingIntent}. */
    public String resultErrCodeKey;
    /** The key with which to send {@link ResultData#errorsList} errmsg in {@link #resultPendingIntent}. */
    public String resultErrmsgKey;
    /** The key with which to send original length of {@link ResultData#stdout} in {@link #resultPendingIntent}. */
    public String resultStdoutOriginalLengthKey;
    /** The key with which to send original length of {@link ResultData#stderr} in {@link #resultPendingIntent}. */
    public String resultStderrOriginalLengthKey;


    /** Defines the directory path in which to write the result of the command. */
    public String resultDirectoryPath;
    /** Defines the directory path under which {@link #resultDirectoryPath} can exist. */
    public String resultDirectoryAllowedParentPath;
    /** Defines whether the result should be written to a single file or multiple files
     * (err, error, stdout, stderr, exit_code) in {@link #resultDirectoryPath}. */
    public boolean resultSingleFile;
    /** Defines the basename of the result file that should be created in {@link #resultDirectoryPath}
     * if {@link #resultSingleFile} is {@code true}. */
    public String resultFileBasename;
    /** Defines the output {@link Formatter} format of the {@link #resultFileBasename} result file. */
    public String resultFileOutputFormat;
    /** Defines the error {@link Formatter} format of the {@link #resultFileBasename} result file. */
    public String resultFileErrorFormat;
    /** Defines the suffix of the result files that should be created in {@link #resultDirectoryPath}
     * if {@link #resultSingleFile} is {@code true}. */
    public String resultFilesSuffix;


    public ResultConfig() {
    }


    public boolean isCommandWithPendingResult() {
        return resultPendingIntent != null || resultDirectoryPath != null;
    }


    @NonNull
    @Override
    public String toString() {
        return getResultConfigLogString(this, true);
    }

    /**
     * Get a log friendly {@link String} for {@link ResultConfig} parameters.
     *
     * @param resultConfig The {@link ResultConfig} to convert.
     * @param ignoreNull Set to {@code true} if non-critical {@code null} values are to be ignored.
     * @return Returns the log friendly {@link String}.
     */
    public static String getResultConfigLogString(final ResultConfig resultConfig, boolean ignoreNull) {
        if (resultConfig == null) return "null";

        StringBuilder logString = new StringBuilder();

        logString.append("Result Pending: `").append(resultConfig.isCommandWithPendingResult()).append("`\n");

        if (resultConfig.resultPendingIntent != null) {
            logString.append(resultConfig.getResultPendingIntentVariablesLogString(ignoreNull));
            if (resultConfig.resultDirectoryPath != null)
                logString.append("\n");
        }

        if (resultConfig.resultDirectoryPath != null && !resultConfig.resultDirectoryPath.isEmpty())
            logString.append(resultConfig.getResultDirectoryVariablesLogString(ignoreNull));

        return logString.toString();
    }

    /**
     * Get log variables {@link List < Pair <String, Object>>} for {@link ResultConfig}.
     *
     * @return Returns the result pending intent variables in list {@link List<  Pair  <String, String>>}.
     */
    private List<Pair<String, String>> getResultPendingIntentVariableList() {
        List<Pair<String, String>> variableList = new ArrayList<Pair<String, String>>() {{
            add(Pair.create("Result Bundle Key", resultBundleKey));
            add(Pair.create("Result Stdout Key", resultStdoutKey));
            add(Pair.create("Result Stderr Key", resultStderrKey));
            add(Pair.create("Result Exit Code Key", resultExitCodeKey));
            add(Pair.create("Result Err Code Key", resultErrCodeKey));
            add(Pair.create("Result Error Key", resultErrmsgKey));
            add(Pair.create("Result Stdout Original Length Key", resultStdoutOriginalLengthKey));
            add(Pair.create("Result Stderr Original Length Key", resultStderrOriginalLengthKey));
        }};
        return variableList;
    }
    
    public String getResultPendingIntentVariablesLogString(boolean ignoreNull) {
        if (resultPendingIntent == null) return "Result PendingIntent Creator: -";

        StringBuilder resultPendingIntentVariablesString = new StringBuilder();

        resultPendingIntentVariablesString.append("Result PendingIntent Creator: `").append(resultPendingIntent.getCreatorPackage()).append("`");

        for (Pair<String, String> logVar: getResultPendingIntentVariableList()) {
            if (!ignoreNull || logVar.second != null) {
                resultPendingIntentVariablesString.append("\n").append(Logger.getSingleLineLogStringEntry(logVar.first, logVar.second, "-"));
            }
        }

        return resultPendingIntentVariablesString.toString();
    }

    /**
     * Get log variables {@link List < Pair <String, Object>>} for {@link ResultConfig}.
     *
     * @return Returns the result directory variables in list {@link List<  Pair  <String, Object>>}.
     */
    private static List<Pair<String, Object>> getResultDirectoryVariableList(final ResultConfig resultConfig) {
        List<Pair<String, Object>> variableList = new ArrayList<Pair<String, Object>>() {{
            add(Pair.create("Result Directory Path", resultConfig.resultDirectoryPath));
            add(Pair.create("Result Single File", resultConfig.resultSingleFile));
            add(Pair.create("Result File Basename", resultConfig.resultFileBasename));
            add(Pair.create("Result File Output Format", resultConfig.resultFileOutputFormat));
            add(Pair.create("Result File Error Format", resultConfig.resultFileErrorFormat));
            add(Pair.create("Result Files Suffix", resultConfig.resultFilesSuffix));
        }};
        return variableList;
    }

    public String getResultDirectoryVariablesLogString(boolean ignoreNull) {
        if (resultDirectoryPath == null) return "Result Directory Path: -";

        StringBuilder resultDirectoryVariablesString = new StringBuilder();

        for (Pair<String, Object> logVar: getResultDirectoryVariableList(this)) {
            switch(logVar.first) {
                case "Result Directory Path":
                    resultDirectoryVariablesString.append(Logger.getSingleLineLogStringEntry(logVar.first, logVar.second, "-"));
                    break;
                case "Result Single File":
                    resultDirectoryVariablesString.append("\n").append(Logger.getSingleLineLogStringEntry(logVar.first, logVar.second, "-"));
                    break;
                default:
                    if (!ignoreNull || logVar.second != null) {
                        resultDirectoryVariablesString.append("\n").append(Logger.getSingleLineLogStringEntry(logVar.first, logVar.second, "-"));
                    }
            }
        }

        return resultDirectoryVariablesString.toString();
    }

    /**
     * Get a markdown {@link String} for {@link ResultConfig}.
     *
     * @param resultConfig The {@link ResultConfig} to convert.
     * @return Returns the markdown {@link String}.
     */
    public static String getResultConfigMarkdownString(final ResultConfig resultConfig) {
        if (resultConfig == null) return "null";

        StringBuilder markdownString = new StringBuilder();

        if (resultConfig.resultPendingIntent != null)
            markdownString.append(MarkdownUtils.getSingleLineMarkdownStringEntry("Result PendingIntent Creator", resultConfig.resultPendingIntent.getCreatorPackage(), "-"));
        else
            markdownString.append("**Result PendingIntent Creator:** -  ");

        if (resultConfig.resultDirectoryPath != null) {
            for (Pair<String, Object> logVar: getResultDirectoryVariableList(resultConfig)) {
                markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry(logVar.first, logVar.second, "-"));
            }
        }

        return markdownString.toString();
    }

}
