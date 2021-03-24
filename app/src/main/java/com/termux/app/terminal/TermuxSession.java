package com.termux.app.terminal;

import com.termux.app.TermuxConstants;
import com.termux.app.utils.Logger;
import com.termux.app.utils.ShellUtils;
import com.termux.models.ExecutionCommand;
import com.termux.terminal.TerminalSession;

import java.io.File;

/**
 * A class that maintains info for foreground Termux sessions.
 * It also provides a way to link each {@link TerminalSession} with the {@link ExecutionCommand}
 * that started it.
 */
public class TermuxSession {

    private final TerminalSession mTerminalSession;
    private final ExecutionCommand mExecutionCommand;

    private static final String LOG_TAG = "TermuxSession";

    private TermuxSession(TerminalSession terminalSession, ExecutionCommand executionCommand) {
        this.mTerminalSession = terminalSession;
        this.mExecutionCommand = executionCommand;
    }

    public static TermuxSession create(ExecutionCommand executionCommand, TermuxSessionClientBase termuxSessionClient, String sessionName) {
        TermuxConstants.TERMUX_HOME_DIR.mkdirs();

        if (executionCommand.workingDirectory == null || executionCommand.workingDirectory.isEmpty()) executionCommand.workingDirectory = TermuxConstants.TERMUX_HOME_DIR_PATH;

        String[] environment = ShellUtils.buildEnvironment(executionCommand.isFailsafe, executionCommand.workingDirectory);
        boolean isLoginShell = false;

        if (executionCommand.executable == null) {
            if (!executionCommand.isFailsafe) {
                for (String shellBinary : new String[]{"login", "bash", "zsh"}) {
                    File shellFile = new File(TermuxConstants.TERMUX_BIN_PREFIX_DIR_PATH, shellBinary);
                    if (shellFile.canExecute()) {
                        executionCommand.executable = shellFile.getAbsolutePath();
                        break;
                    }
                }
            }

            if (executionCommand.executable == null) {
                // Fall back to system shell as last resort:
                executionCommand.executable = "/system/bin/sh";
            }
            isLoginShell = true;
        }

        String[] processArgs = ShellUtils.setupProcessArgs(executionCommand.executable, executionCommand.arguments);

        executionCommand.executable = processArgs[0];
        String processName = (isLoginShell ? "-" : "") + ShellUtils.getExecutableBasename(executionCommand.executable);

        String[] arguments = new String[processArgs.length];
        arguments[0] = processName;
        if (processArgs.length > 1) System.arraycopy(processArgs, 1, arguments, 1, processArgs.length - 1);

        executionCommand.arguments = arguments;

        if(!executionCommand.setState(ExecutionCommand.ExecutionState.EXECUTING))
            return null;

        Logger.logDebug(LOG_TAG, executionCommand.toString());

        TerminalSession terminalSession = new TerminalSession(executionCommand.executable, executionCommand.workingDirectory, executionCommand.arguments, environment, termuxSessionClient);

        if (sessionName != null) {
            terminalSession.mSessionName = sessionName;
        }

        return new TermuxSession(terminalSession, executionCommand);
    }

    public TerminalSession getTerminalSession() {
        return mTerminalSession;
    }

    public ExecutionCommand getExecutionCommand() {
        return mExecutionCommand;
    }

}
