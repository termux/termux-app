package com.termux.shared.models;

import android.content.Intent;
import android.net.Uri;

import androidx.annotation.NonNull;

import com.termux.shared.data.IntentUtils;
import com.termux.shared.models.errors.Error;
import com.termux.shared.logger.Logger;
import com.termux.shared.markdown.MarkdownUtils;
import com.termux.shared.data.DataUtils;

import java.util.Collections;
import java.util.List;

public class ExecutionCommand {

    /*
    The {@link ExecutionState#SUCCESS} and {@link ExecutionState#FAILED} is defined based on
    successful execution of command without any internal errors or exceptions being raised.
    The shell command {@link #exitCode} being non-zero **does not** mean that execution command failed.
    Only the {@link #errCode} being non-zero means that execution command failed from the Termux app
    perspective.
    */

    /** The {@link Enum} that defines {@link ExecutionCommand} state. */
    public enum ExecutionState {

        PRE_EXECUTION("Pre-Execution", 0),
        EXECUTING("Executing", 1),
        EXECUTED("Executed", 2),
        SUCCESS("Success", 3),
        FAILED("Failed", 4);

        private final String name;
        private final int value;

        ExecutionState(final String name, final int value) {
            this.name = name;
            this.value = value;
        }

        public String getName() {
            return name;
        }

        public int getValue() {
            return value;
        }

    }


    /** The optional unique id for the {@link ExecutionCommand}. */
    public Integer id;


    /** The current state of the {@link ExecutionCommand}. */
    private ExecutionState currentState = ExecutionState.PRE_EXECUTION;
    /** The previous state of the {@link ExecutionCommand}. */
    private ExecutionState previousState = ExecutionState.PRE_EXECUTION;


    /** The executable for the {@link ExecutionCommand}. */
    public String executable;
    /** The executable Uri for the {@link ExecutionCommand}. */
    public Uri executableUri;
    /** The executable arguments array for the {@link ExecutionCommand}. */
    public String[] arguments;
    /** The stdin string for the {@link ExecutionCommand}. */
    public String stdin;
    /** The current working directory for the {@link ExecutionCommand}. */
    public String workingDirectory;


    /** The terminal transcript rows for the {@link ExecutionCommand}. */
    public Integer terminalTranscriptRows;


    /** If the {@link ExecutionCommand} is a background or a foreground terminal session command. */
    public boolean inBackground;
    /** If the {@link ExecutionCommand} is meant to start a failsafe terminal session. */
    public boolean isFailsafe;

    /**
     * The {@link ExecutionCommand} custom log level for background {@link com.termux.shared.shell.TermuxTask}
     * commands. By default, @link com.termux.shared.shell.StreamGobbler} only logs stdout and
     * stderr if {@link Logger} `CURRENT_LOG_LEVEL` is >= {@link Logger#LOG_LEVEL_VERBOSE} and
     * {@link com.termux.shared.shell.TermuxTask} only logs stdin if `CURRENT_LOG_LEVEL` is >=
     * {@link Logger#LOG_LEVEL_DEBUG}.
     */
    public Integer backgroundCustomLogLevel;

    /** The session action of foreground commands. */
    public String sessionAction;


    /** The command label for the {@link ExecutionCommand}. */
    public String commandLabel;
    /** The markdown text for the command description for the {@link ExecutionCommand}. */
    public String commandDescription;
    /** The markdown text for the help of command for the {@link ExecutionCommand}. This can be used
     * to provide useful info to the user if an internal error is raised. */
    public String commandHelp;


    /** Defines the markdown text for the help of the Termux plugin API that was used to start the
     * {@link ExecutionCommand}. This can be used to provide useful info to the user if an internal
     * error is raised. */
    public String pluginAPIHelp;


    /** Defines the {@link Intent} received which started the command. */
    public Intent commandIntent;

    /** Defines if {@link ExecutionCommand} was started because of an external plugin request
     * like with an intent or from within Termux app itself. */
    public boolean isPluginExecutionCommand;

    /** Defines the {@link ResultConfig} for the {@link ExecutionCommand} containing information
     * on how to handle the result. */
    public final ResultConfig resultConfig = new ResultConfig();

    /** Defines the {@link ResultData} for the {@link ExecutionCommand} containing information
     * of the result. */
    public final ResultData resultData = new ResultData();


    /** Defines if processing results already called for this {@link ExecutionCommand}. */
    public boolean processingResultsAlreadyCalled;

    private static final String LOG_TAG = "ExecutionCommand";


    public ExecutionCommand() {
    }

    public ExecutionCommand(Integer id) {
        this.id = id;
    }

    public ExecutionCommand(Integer id, String executable, String[] arguments, String stdin, String workingDirectory, boolean inBackground, boolean isFailsafe) {
        this.id = id;
        this.executable = executable;
        this.arguments = arguments;
        this.stdin = stdin;
        this.workingDirectory = workingDirectory;
        this.inBackground = inBackground;
        this.isFailsafe = isFailsafe;
    }


    public boolean isPluginExecutionCommandWithPendingResult() {
        return isPluginExecutionCommand && resultConfig.isCommandWithPendingResult();
    }


    public synchronized boolean setState(ExecutionState newState) {
        // The state transition cannot go back or change if already at {@link ExecutionState#SUCCESS}
        if (newState.getValue() < currentState.getValue() || currentState == ExecutionState.SUCCESS) {
            Logger.logError(LOG_TAG, "Invalid "+ getCommandIdAndLabelLogString() + " state transition from \"" + currentState.getName() + "\" to " +  "\"" + newState.getName() + "\"");
            return false;
        }

        // The {@link ExecutionState#FAILED} can be set again, like to add more errors, but we don't update
        // {@link #previousState} with the {@link #currentState} value if its at {@link ExecutionState#FAILED} to
        // preserve the last valid state
        if (currentState != ExecutionState.FAILED)
            previousState = currentState;

        currentState = newState;
        return  true;
    }

    public synchronized boolean hasExecuted() {
        return currentState.getValue() >= ExecutionState.EXECUTED.getValue();
    }

    public synchronized boolean isExecuting() {
        return currentState == ExecutionState.EXECUTING;
    }

    public synchronized boolean isSuccessful() {
        return currentState == ExecutionState.SUCCESS;
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
        if (!this.resultData.setStateFailed(type, code, message, throwablesList)) {
            Logger.logWarn(LOG_TAG, "setStateFailed for "  + getCommandIdAndLabelLogString() + " resultData encountered an error.");
        }

        return setState(ExecutionState.FAILED);
    }

    public synchronized boolean shouldNotProcessResults() {
        if (processingResultsAlreadyCalled) {
            return true;
        } else {
            processingResultsAlreadyCalled = true;
            return false;
        }
    }

    public synchronized boolean isStateFailed() {
        if (currentState != ExecutionState.FAILED)
            return false;

        if (!resultData.isStateFailed()) {
            Logger.logWarn(LOG_TAG, "The "  + getCommandIdAndLabelLogString() + " has an invalid errCode value set in errors list while having ExecutionState.FAILED state.\n" + resultData.errorsList);
            return false;
        } else {
            return true;
        }
    }


    @NonNull
    @Override
    public String toString() {
        if (!hasExecuted())
            return getExecutionInputLogString(this, true, true);
        else {
            return getExecutionOutputLogString(this, true, true, true);
        }
    }

    /**
     * Get a log friendly {@link String} for {@link ExecutionCommand} execution input parameters.
     *
     * @param executionCommand The {@link ExecutionCommand} to convert.
     * @param ignoreNull Set to {@code true} if non-critical {@code null} values are to be ignored.
     * @param logStdin Set to {@code true} if {@link #stdin} should be logged.
     * @return Returns the log friendly {@link String}.
     */
    public static String getExecutionInputLogString(final ExecutionCommand executionCommand, boolean ignoreNull, boolean logStdin) {
        if (executionCommand == null) return "null";

        StringBuilder logString = new StringBuilder();

        logString.append(executionCommand.getCommandIdAndLabelLogString()).append(":");

        if (executionCommand.previousState != ExecutionState.PRE_EXECUTION)
            logString.append("\n").append(executionCommand.getPreviousStateLogString());
        logString.append("\n").append(executionCommand.getCurrentStateLogString());

        logString.append("\n").append(executionCommand.getExecutableLogString());
        logString.append("\n").append(executionCommand.getArgumentsLogString());
        logString.append("\n").append(executionCommand.getWorkingDirectoryLogString());
        logString.append("\n").append(executionCommand.getInBackgroundLogString());
        logString.append("\n").append(executionCommand.getIsFailsafeLogString());

        if (executionCommand.inBackground) {
            if (logStdin && (!ignoreNull || !DataUtils.isNullOrEmpty(executionCommand.stdin)))
                logString.append("\n").append(executionCommand.getStdinLogString());

            if (!ignoreNull || executionCommand.backgroundCustomLogLevel != null)
                logString.append("\n").append(executionCommand.getBackgroundCustomLogLevelLogString());
        }

        if (!ignoreNull || executionCommand.sessionAction != null)
            logString.append("\n").append(executionCommand.getSessionActionLogString());

        if (!ignoreNull || executionCommand.commandIntent != null)
            logString.append("\n").append(executionCommand.getCommandIntentLogString());

        logString.append("\n").append(executionCommand.getIsPluginExecutionCommandLogString());
        if (executionCommand.isPluginExecutionCommand)
            logString.append("\n").append(ResultConfig.getResultConfigLogString(executionCommand.resultConfig, ignoreNull));

        return logString.toString();
    }

    /**
     * Get a log friendly {@link String} for {@link ExecutionCommand} execution output parameters.
     *
     * @param executionCommand The {@link ExecutionCommand} to convert.
     * @param ignoreNull Set to {@code true} if non-critical {@code null} values are to be ignored.
     * @param logResultData Set to {@code true} if {@link #resultData} should be logged.
     * @param logStdoutAndStderr Set to {@code true} if {@link ResultData#stdout} and {@link ResultData#stderr} should be logged.
     * @return Returns the log friendly {@link String}.
     */
    public static String getExecutionOutputLogString(final ExecutionCommand executionCommand, boolean ignoreNull, boolean logResultData, boolean logStdoutAndStderr) {
        if (executionCommand == null) return "null";

        StringBuilder logString = new StringBuilder();

        logString.append(executionCommand.getCommandIdAndLabelLogString()).append(":");

        logString.append("\n").append(executionCommand.getPreviousStateLogString());
        logString.append("\n").append(executionCommand.getCurrentStateLogString());

        if (logResultData)
            logString.append("\n").append(ResultData.getResultDataLogString(executionCommand.resultData, logStdoutAndStderr));

        return logString.toString();
    }

    /**
     * Get a log friendly {@link String} for {@link ExecutionCommand} with more details.
     *
     * @param executionCommand The {@link ExecutionCommand} to convert.
     * @return Returns the log friendly {@link String}.
     */
    public static String getDetailedLogString(final ExecutionCommand executionCommand) {
        if (executionCommand == null) return "null";

        StringBuilder logString = new StringBuilder();

        logString.append(getExecutionInputLogString(executionCommand, false, true));
        logString.append(getExecutionOutputLogString(executionCommand, false, true, true));

        logString.append("\n").append(executionCommand.getCommandDescriptionLogString());
        logString.append("\n").append(executionCommand.getCommandHelpLogString());
        logString.append("\n").append(executionCommand.getPluginAPIHelpLogString());

        return logString.toString();
    }

    /**
     * Get a markdown {@link String} for {@link ExecutionCommand}.
     *
     * @param executionCommand The {@link ExecutionCommand} to convert.
     * @return Returns the markdown {@link String}.
     */
    public static String getExecutionCommandMarkdownString(final ExecutionCommand executionCommand) {
        if (executionCommand == null) return "null";

        if (executionCommand.commandLabel == null) executionCommand.commandLabel = "Execution Command";

        StringBuilder markdownString = new StringBuilder();

        markdownString.append("## ").append(executionCommand.commandLabel).append("\n");


        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Previous State", executionCommand.previousState.getName(), "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Current State", executionCommand.currentState.getName(), "-"));

        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Executable", executionCommand.executable, "-"));
        markdownString.append("\n").append(getArgumentsMarkdownString(executionCommand.arguments));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Working Directory", executionCommand.workingDirectory, "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("inBackground", executionCommand.inBackground, "-"));
        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("isFailsafe", executionCommand.isFailsafe, "-"));

        if (executionCommand.inBackground) {
            if (!DataUtils.isNullOrEmpty(executionCommand.stdin))
                markdownString.append("\n").append(MarkdownUtils.getMultiLineMarkdownStringEntry("Stdin", executionCommand.stdin, "-"));
            if (executionCommand.backgroundCustomLogLevel != null)
                markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Background Custom Log Level", executionCommand.backgroundCustomLogLevel, "-"));
        }

        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("Session Action", executionCommand.sessionAction, "-"));


        markdownString.append("\n").append(MarkdownUtils.getSingleLineMarkdownStringEntry("isPluginExecutionCommand", executionCommand.isPluginExecutionCommand, "-"));

        markdownString.append("\n\n").append(ResultConfig.getResultConfigMarkdownString(executionCommand.resultConfig));

        markdownString.append("\n\n").append(ResultData.getResultDataMarkdownString(executionCommand.resultData));

        if (executionCommand.commandDescription != null || executionCommand.commandHelp != null) {
            if (executionCommand.commandDescription != null)
                markdownString.append("\n\n### Command Description\n\n").append(executionCommand.commandDescription).append("\n");
            if (executionCommand.commandHelp != null)
                markdownString.append("\n\n### Command Help\n\n").append(executionCommand.commandHelp).append("\n");
            markdownString.append("\n##\n");
        }

        if (executionCommand.pluginAPIHelp != null) {
            markdownString.append("\n\n### Plugin API Help\n\n").append(executionCommand.pluginAPIHelp);
            markdownString.append("\n##\n");
        }

        return markdownString.toString();
    }


    public String getIdLogString() {
        if (id != null)
            return "(" + id + ") ";
        else
            return "";
    }

    public String getCurrentStateLogString() {
        return "Current State: `" + currentState.getName() + "`";
    }

    public String getPreviousStateLogString() {
        return "Previous State: `" + previousState.getName() + "`";
    }

    public String getCommandLabelLogString() {
        if (commandLabel != null && !commandLabel.isEmpty())
            return commandLabel;
        else
            return "Execution Command";
    }

    public String getCommandIdAndLabelLogString() {
        return getIdLogString() + getCommandLabelLogString();
    }

    public String getExecutableLogString() {
        return "Executable: `" + executable + "`";
    }

    public String getArgumentsLogString() {
        return getArgumentsLogString(arguments);
    }

    public String getWorkingDirectoryLogString() {
        return "Working Directory: `" + workingDirectory + "`";
    }

    public String getInBackgroundLogString() {
        return "inBackground: `" + inBackground + "`";
    }

    public String getIsFailsafeLogString() {
        return "isFailsafe: `" + isFailsafe + "`";
    }

    public String getStdinLogString() {
        if (DataUtils.isNullOrEmpty(stdin))
            return "Stdin: -";
        else
            return Logger.getMultiLineLogStringEntry("Stdin", stdin, "-");
    }

    public String getBackgroundCustomLogLevelLogString() {
        return "Background Custom Log Level: `" + backgroundCustomLogLevel + "`";
    }

    public String getSessionActionLogString() {
        return Logger.getSingleLineLogStringEntry("Session Action", sessionAction, "-");
    }

    public String getCommandDescriptionLogString() {
        return Logger.getSingleLineLogStringEntry("Command Description", commandDescription, "-");
    }

    public String getCommandHelpLogString() {
        return Logger.getSingleLineLogStringEntry("Command Help", commandHelp, "-");
    }

    public String getPluginAPIHelpLogString() {
        return Logger.getSingleLineLogStringEntry("Plugin API Help", pluginAPIHelp, "-");
    }

    public String getCommandIntentLogString() {
        if (commandIntent == null)
            return "Command Intent: -";
        else
            return Logger.getMultiLineLogStringEntry("Command Intent", IntentUtils.getIntentString(commandIntent), "-");
    }

    public String getIsPluginExecutionCommandLogString() {
        return "isPluginExecutionCommand: `" + isPluginExecutionCommand + "`";
    }


    /**
     * Get a log friendly {@link String} for {@link List<String>} argumentsArray.
     * If argumentsArray are null or of size 0, then `Arguments: -` is returned. Otherwise
     * following format is returned:
     *
     * Arguments:
     * ```
     * Arg 1: `value`
     * Arg 2: 'value`
     * ```
     *
     * @param argumentsArray The {@link String[]} argumentsArray to convert.
     * @return Returns the log friendly {@link String}.
     */
    public static String getArgumentsLogString(final String[] argumentsArray) {
        StringBuilder argumentsString = new StringBuilder("Arguments:");

        if (argumentsArray != null && argumentsArray.length != 0) {
            argumentsString.append("\n```\n");
            for (int i = 0; i != argumentsArray.length; i++) {
                argumentsString.append(Logger.getSingleLineLogStringEntry("Arg " + (i + 1),
                    DataUtils.getTruncatedCommandOutput(argumentsArray[i], Logger.LOGGER_ENTRY_MAX_SAFE_PAYLOAD / 5, true, false, true),
                    "-")).append("\n");
            }
            argumentsString.append("```");
        } else{
            argumentsString.append(" -");
        }

        return argumentsString.toString();
    }

    /**
     * Get a markdown {@link String} for {@link String[]} argumentsArray.
     * If argumentsArray are null or of size 0, then `**Arguments:** -` is returned. Otherwise
     * following format is returned:
     *
     * **Arguments:**
     *
     * **Arg 1:**
     * ```
     * value
     * ```
     * **Arg 2:**
     * ```
     * value
     *```
     *
     * @param argumentsArray The {@link String[]} argumentsArray to convert.
     * @return Returns the markdown {@link String}.
     */
    public static String getArgumentsMarkdownString(final String[] argumentsArray) {
        StringBuilder argumentsString = new StringBuilder("**Arguments:**");

        if (argumentsArray != null && argumentsArray.length != 0) {
            argumentsString.append("\n");
            for (int i = 0; i != argumentsArray.length; i++) {
                argumentsString.append(MarkdownUtils.getMultiLineMarkdownStringEntry("Arg " + (i + 1), argumentsArray[i], "-")).append("\n");
            }
        } else{
            argumentsString.append(" -  ");
        }

        return argumentsString.toString();
    }

}
