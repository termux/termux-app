package com.termux.shared.shell;

import android.content.Context;
import android.system.OsConstants;

import androidx.annotation.NonNull;

import com.termux.shared.R;
import com.termux.shared.models.ExecutionCommand;
import com.termux.shared.models.ResultData;
import com.termux.shared.models.errors.Errno;
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
     * @param shellEnvironmentClient The {@link ShellEnvironmentClient} interface implementation.
     * @param sessionName The optional {@link TerminalSession} name.
     * @param setStdoutOnExit If set to {@code true}, then the {@link ResultData#stdout}
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
                                        @NonNull final ShellEnvironmentClient shellEnvironmentClient,
                                        final String sessionName, final boolean setStdoutOnExit) {
        if (executionCommand.workingDirectory == null || executionCommand.workingDirectory.isEmpty())
            executionCommand.workingDirectory = shellEnvironmentClient.getDefaultWorkingDirectoryPath();
        if (executionCommand.workingDirectory.isEmpty())
            executionCommand.workingDirectory = "/";

        String[] environment = shellEnvironmentClient.buildEnvironment(context, executionCommand.isFailsafe, executionCommand.workingDirectory);

        String defaultBinPath = shellEnvironmentClient.getDefaultBinPath();
        if (defaultBinPath.isEmpty())
            defaultBinPath = "/system/bin";

        boolean isLoginShell = false;
        if (executionCommand.executable == null) {
            if (!executionCommand.isFailsafe) {
                for (String shellBinary : new String[]{"login", "bash", "zsh"}) {
                    File shellFile = new File(defaultBinPath, shellBinary);
                    if (shellFile.canExecute()) {
                        executionCommand.executable = shellFile.getAbsolutePath();
                        break;
                    }
                }
            }

            if (executionCommand.executable == null) {
                // Fall back to system shell as last resort:
                // Do not start a login shell since ~/.profile may cause startup failure if its invalid.
                // /system/bin/sh is provided by mksh (not toybox) and does load .mkshrc but for android its set
                // to /system/etc/mkshrc even though its default is ~/.mkshrc.
                // So /system/etc/mkshrc must still be valid for failsafe session to start properly.
                // https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:external/mksh/src/main.c;l=663
                // https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:external/mksh/src/main.c;l=41
                // https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:external/mksh/Android.bp;l=114
                executionCommand.executable = "/system/bin/sh";
            } else {
                isLoginShell = true;
            }

        }

        String[] processArgs = shellEnvironmentClient.setupProcessArgs(executionCommand.executable, executionCommand.arguments);

        executionCommand.executable = processArgs[0];
        String processName = (isLoginShell ? "-" : "") + ShellUtils.getExecutableBasename(executionCommand.executable);

        String[] arguments = new String[processArgs.length];
        arguments[0] = processName;
        if (processArgs.length > 1) System.arraycopy(processArgs, 1, arguments, 1, processArgs.length - 1);

        executionCommand.arguments = arguments;

        if (executionCommand.commandLabel == null)
            executionCommand.commandLabel = processName;

        if (!executionCommand.setState(ExecutionCommand.ExecutionState.EXECUTING)) {
            executionCommand.setStateFailed(Errno.ERRNO_FAILED.getCode(), context.getString(R.string.error_failed_to_execute_termux_session_command, executionCommand.getCommandIdAndLabelLogString()));
            TermuxSession.processTermuxSessionResult(null, executionCommand);
            return null;
        }

        Logger.logDebugExtended(LOG_TAG, executionCommand.toString());

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
     * If the processes has finished, then sets {@link ResultData#stdout}, {@link ResultData#stderr}
     * and {@link ResultData#exitCode} for the {@link #mExecutionCommand} of the {@code termuxTask}
     * and then calls {@link #processTermuxSessionResult(TermuxSession, ExecutionCommand)} to process the result}.
     *
     */
    public void finish() {
        // If process is still running, then ignore the call
        if (mTerminalSession.isRunning()) return;

        int exitCode = mTerminalSession.getExitStatus();

        if (exitCode == 0)
            Logger.logDebug(LOG_TAG, "The \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession exited normally");
        else
            Logger.logDebug(LOG_TAG, "The \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession exited with code: " + exitCode);

        // If the execution command has already failed, like SIGKILL was sent, then don't continue
        if (mExecutionCommand.isStateFailed()) {
            Logger.logDebug(LOG_TAG, "Ignoring setting \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxSession state to ExecutionState.EXECUTED and processing results since it has already failed");
            return;
        }

        mExecutionCommand.resultData.exitCode = exitCode;

        if (this.mSetStdoutOnExit)
            mExecutionCommand.resultData.stdout.append(ShellUtils.getTerminalSessionTranscriptText(mTerminalSession, true, false));

        if (!mExecutionCommand.setState(ExecutionCommand.ExecutionState.EXECUTED))
            return;

        TermuxSession.processTermuxSessionResult(this, null);
    }

    /**
     * Kill this {@link TermuxSession} by sending a {@link OsConstants#SIGILL} to its {@link #mTerminalSession}
     * if its still executing.
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
        if (mExecutionCommand.setStateFailed(Errno.ERRNO_FAILED.getCode(), context.getString(R.string.error_sending_sigkill_to_process))) {
            if (processResult) {
                mExecutionCommand.resultData.exitCode = 137; // SIGKILL

                // Get whatever output has been set till now in case its needed
                if (this.mSetStdoutOnExit)
                    mExecutionCommand.resultData.stdout.append(ShellUtils.getTerminalSessionTranscriptText(mTerminalSession, true, false));

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
     *                  {@link #execute(Context, ExecutionCommand, TerminalSessionClient, TermuxSessionClient, ShellEnvironmentClient, String, boolean)}
     *                   successfully started the process.
     * @param executionCommand The {@link ExecutionCommand}, which should be set if
     *                          {@link #execute(Context, ExecutionCommand, TerminalSessionClient, TermuxSessionClient, ShellEnvironmentClient, String, boolean)}
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
