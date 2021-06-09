package com.termux.shared.shell;

import android.content.Context;
import android.system.OsConstants;

import androidx.annotation.NonNull;

import com.termux.shared.R;
import com.termux.shared.models.ExecutionCommand;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.logger.Logger;
import com.termux.terminal.TerminalSession;
import com.termux.terminal.TerminalSessionClient;

import java.io.File;

/**
 * A class that maintains info for foreground Termux sessions.
 * It also provides a way to link each {@link TerminalSession} with the {@link ExecutionCommand}
 * that started it.
 */
public class TermuxSession {

    private final TerminalSession mTerminalSession;
    private final ExecutionCommand mExecutionCommand;
    private final TermuxSessionClient mTermuxSessionClient;
    private final boolean mSetStdoutOnExit;

    private static final String LOG_TAG = "TermuxSession";

    private TermuxSession(@NonNull final TerminalSession terminalSession, @NonNull final ExecutionCommand executionCommand,
                          final TermuxSessionClient termuxSessionClient, final boolean setStdoutOnExit) {
        this.mTerminalSession = terminalSession;
        this.mExecutionCommand = executionCommand;
        this.mTermuxSessionClient = termuxSessionClient;
        this.mSetStdoutOnExit = setStdoutOnExit;
    }

    /**
     * Start execution of an {@link ExecutionCommand} with {@link Runtime#exec(String[], String[], File)}.
     *
     * The {@link ExecutionCommand#executable}, must be set, {@link ExecutionCommand#commandLabel},
     * {@link ExecutionCommand#arguments} and {@link ExecutionCommand#workingDirectory} may optionally
     * be set.
     *
     * If {@link ExecutionCommand#executable} is {@code null}, then a default shell is automatically
     * chosen.
     *
     * @param context The {@link Context} for operations.
     * @param executionCommand The {@link ExecutionCommand} containing the information for execution command.
     * @param terminalSessionClient The {@link TerminalSessionClient} interface implementation.
     * @param termuxSessionClient The {@link TermuxSessionClient} interface implementation.
     * @param sessionName The optional {@link TerminalSession} name.
     * @param setStdoutOnExit If set to {@code true}, then the {@link ExecutionCommand#stdout}
     *                        available in the {@link TermuxSessionClient#onTermuxSessionExited(TermuxSession)}
     *                        callback will be set to the {@link TerminalSession} transcript. The session
     *                        transcript will contain both stdout and stderr combined, basically
     *                        anything sent to the the pseudo terminal /dev/pts, including PS1 prefixes.
     *                        Set this to {@code true} only if the session transcript is required,
     *                        since this requires extra processing to get it.
     * @return Returns the {@link TermuxSession}. This will be {@code null} if failed to start the execution command.
     */
    public static TermuxSession execute(@NonNull final Context context, @NonNull ExecutionCommand executionCommand,
                                        @NonNull final TerminalSessionClient terminalSessionClient, final TermuxSessionClient termuxSessionClient,
                                        final String sessionName, final boolean setStdoutOnExit) {
        if (executionCommand.workingDirectory == null || executionCommand.workingDirectory.isEmpty()) executionCommand.workingDirectory = TermuxConstants.TERMUX_HOME_DIR_PATH;

        String[] environment = ShellUtils.buildEnvironment(context, executionCommand.isFailsafe, executionCommand.workingDirectory);

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

        if (executionCommand.commandLabel == null)
            executionCommand.commandLabel = processName;

        if (!executionCommand.setState(ExecutionCommand.ExecutionState.EXECUTING)) {
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, context.getString(R.string.error_failed_to_execute_termux_session_command, executionCommand.getCommandIdAndLabelLogString()), null);
            TermuxSession.processTermuxSessionResult(null, executionCommand);
            return null;
        }

        Logger.logDebug(LOG_TAG, executionCommand.toString());

        Logger.logDebug(LOG_TAG, "Running \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession");
        TerminalSession terminalSession = new TerminalSession(executionCommand.executable, executionCommand.workingDirectory, executionCommand.arguments, environment, executionCommand.terminalTranscriptRows, terminalSessionClient);

        if (sessionName != null) {
            terminalSession.mSessionName = sessionName;
        }

        return new TermuxSession(terminalSession, executionCommand, termuxSessionClient, setStdoutOnExit);
    }

    /**
     * Signal that this {@link TermuxSession} has finished.  This should be called when
     * {@link TerminalSessionClient#onSessionFinished(TerminalSession)} callback is received by the caller.
     *
     * If the processes has finished, then sets {@link ExecutionCommand#stdout}, {@link ExecutionCommand#stderr}
     * and {@link ExecutionCommand#exitCode} for the {@link #mExecutionCommand} of the {@code termuxTask}
     * and then calls {@link #processTermuxSessionResult(TermuxSession, ExecutionCommand)} to process the result}.
     *
     */
    public void finish() {
        // If process is still running, then ignore the call
        if (mTerminalSession.isRunning()) return;

        int exitCode = mTerminalSession.getExitStatus();

        if (exitCode == 0)
            Logger.logDebug(LOG_TAG, "The \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession with exited normally");
        else
            Logger.logDebug(LOG_TAG, "The \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession with exited with code: " + exitCode);

        // If the execution command has already failed, like SIGKILL was sent, then don't continue
        if (mExecutionCommand.isStateFailed()) {
            Logger.logDebug(LOG_TAG, "Ignoring setting \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession state to ExecutionState.EXECUTED and processing results since it has already failed");
            return;
        }

        if (this.mSetStdoutOnExit)
            mExecutionCommand.stdout = ShellUtils.getTerminalSessionTranscriptText(mTerminalSession, true, false);
        else
            mExecutionCommand.stdout = null;

        mExecutionCommand.stderr = null;
        mExecutionCommand.exitCode = exitCode;

        if (!mExecutionCommand.setState(ExecutionCommand.ExecutionState.EXECUTED))
            return;

        TermuxSession.processTermuxSessionResult(this, null);
    }

    /**
     * Kill this {@link TermuxSession} by sending a {@link OsConstants#SIGILL} to its {@link #mTerminalSession}
     * if its still executing.
     *
     * We process the results even if
     *
     * @param context The {@link Context} for operations.
     * @param processResult If set to {@code true}, then the {@link #processTermuxSessionResult(TermuxSession, ExecutionCommand)}
     *                      will be called to process the failure.
     */
    public void killIfExecuting(@NonNull final Context context, boolean processResult) {
        // If execution command has already finished executing, then no need to process results or send SIGKILL
        if (mExecutionCommand.hasExecuted()) {
            Logger.logDebug(LOG_TAG, "Ignoring sending SIGKILL to \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession since it has already finished executing");
            return;
        }

        Logger.logDebug(LOG_TAG, "Send SIGKILL to \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession");
        if (mExecutionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, context.getString(R.string.error_sending_sigkill_to_process), null)) {
            if (processResult) {
                // Get whatever output has been set till now in case its needed
                if (this.mSetStdoutOnExit)
                    mExecutionCommand.stdout = ShellUtils.getTerminalSessionTranscriptText(mTerminalSession, true, false);
                else
                    mExecutionCommand.stdout = null;

                mExecutionCommand.stderr = null;
                mExecutionCommand.exitCode = 137; // SIGKILL

                TermuxSession.processTermuxSessionResult(this, null);
            }
        }

        // Send SIGKILL to process
        mTerminalSession.finishIfRunning();
    }

    /**
     * Process the results of {@link TermuxSession} or {@link ExecutionCommand}.
     *
     * Only one of {@code termuxSession} and {@code executionCommand} must be set.
     *
     * If the {@code termuxSession} and its {@link #mTermuxSessionClient} are not {@code null},
     * then the {@link TermuxSession.TermuxSessionClient#onTermuxSessionExited(TermuxSession)}
     * callback will be called.
     *
     * @param termuxSession The {@link TermuxSession}, which should be set if
     *                  {@link #execute(Context, ExecutionCommand, TerminalSessionClient, TermuxSessionClient, String, boolean)} 
     *                   successfully started the process.
     * @param executionCommand The {@link ExecutionCommand}, which should be set if
     *                          {@link #execute(Context, ExecutionCommand, TerminalSessionClient, TermuxSessionClient, String, boolean)}
     *                          failed to start the process.
     */
    private static void processTermuxSessionResult(final TermuxSession termuxSession, ExecutionCommand executionCommand) {
        if (termuxSession != null)
            executionCommand = termuxSession.mExecutionCommand;

        if (executionCommand == null) return;

        if (executionCommand.shouldNotProcessResults()) {
            Logger.logDebug(LOG_TAG, "Ignoring duplicate call to process \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession result");
            return;
        }

        Logger.logDebug(LOG_TAG, "Processing \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession result");

        if (termuxSession != null && termuxSession.mTermuxSessionClient != null) {
            termuxSession.mTermuxSessionClient.onTermuxSessionExited(termuxSession);
        } else {
            // If a callback is not set and execution command didn't fail, then we set success state now
            // Otherwise, the callback host can set it himself when its done with the termuxSession
            if (!executionCommand.isStateFailed())
                executionCommand.setState(ExecutionCommand.ExecutionState.SUCCESS);
        }
    }

    public TerminalSession getTerminalSession() {
        return mTerminalSession;
    }

    public ExecutionCommand getExecutionCommand() {
        return mExecutionCommand;
    }



    public interface TermuxSessionClient {

        /**
         * Callback function for when {@link TermuxSession} exits.
         *
         * @param termuxSession The {@link TermuxSession} that exited.
         */
        void onTermuxSessionExited(TermuxSession termuxSession);

    }

}
