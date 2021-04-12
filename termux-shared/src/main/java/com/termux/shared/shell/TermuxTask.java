package com.termux.shared.shell;

import android.content.Context;
import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;

import androidx.annotation.NonNull;

import com.termux.shared.R;
import com.termux.shared.models.ExecutionCommand;
import com.termux.shared.termux.TermuxConstants;
import com.termux.shared.logger.Logger;
import com.termux.shared.models.ExecutionCommand.ExecutionState;

import java.io.DataOutputStream;
import java.io.File;
import java.io.IOException;
import java.nio.charset.StandardCharsets;

/**
 * A class that maintains info for background Termux tasks run with {@link Runtime#exec(String[], String[], File)}.
 * It also provides a way to link each {@link Process} with the {@link ExecutionCommand}
 * that started it.
 */
public final class TermuxTask {

    private final Process mProcess;
    private final ExecutionCommand mExecutionCommand;
    private final TermuxTaskClient mTermuxTaskClient;

    private final StringBuilder mStdout = new StringBuilder();
    private final StringBuilder mStderr = new StringBuilder();

    private static final String LOG_TAG = "TermuxTask";

    private TermuxTask(@NonNull final Process process, @NonNull final ExecutionCommand executionCommand,
                       final TermuxTaskClient termuxTaskClient) {
        this.mProcess = process;
        this.mExecutionCommand = executionCommand;
        this.mTermuxTaskClient = termuxTaskClient;
    }

    /**
     * Start execution of an {@link ExecutionCommand} with {@link Runtime#exec(String[], String[], File)}.
     *
     * The {@link ExecutionCommand#executable}, must be set.
     * The  {@link ExecutionCommand#commandLabel}, {@link ExecutionCommand#arguments} and
     * {@link ExecutionCommand#workingDirectory} may optionally be set.
     *
     * @param context The {@link Context} for operations.
     * @param executionCommand The {@link ExecutionCommand} containing the information for execution command.
     * @param termuxTaskClient The {@link TermuxTaskClient} interface implementation.
     *                           The {@link TermuxTaskClient#onTermuxTaskExited(TermuxTask)} will
     *                           be called regardless of {@code isSynchronous} value but not if
     *                           {@code null} is returned by this method. This can
     *                           optionally be {@code null}.
     * @param isSynchronous If set to {@code true}, then the command will be executed in the
     *                      caller thread and results returned synchronously in the {@link ExecutionCommand}
     *                      sub object of the {@link TermuxTask} returned.
     *                      If set to {@code false}, then a new thread is started run the commands
     *                      asynchronously in the background and control is returned to the caller thread.
     * @return Returns the {@link TermuxTask}. This will be {@code null} if failed to start the execution command.
     */
    public static TermuxTask execute(@NonNull final Context context, @NonNull ExecutionCommand executionCommand,
                                     final TermuxTaskClient termuxTaskClient, final boolean isSynchronous) {
        if (executionCommand.workingDirectory == null || executionCommand.workingDirectory.isEmpty()) executionCommand.workingDirectory = TermuxConstants.TERMUX_HOME_DIR_PATH;

        String[] env = ShellUtils.buildEnvironment(context, false, executionCommand.workingDirectory);

        final String[] commandArray = ShellUtils.setupProcessArgs(executionCommand.executable, executionCommand.arguments);

        if (!executionCommand.setState(ExecutionState.EXECUTING))
            return null;

        Logger.logDebug(LOG_TAG, executionCommand.toString());

        String taskName = ShellUtils.getExecutableBasename(executionCommand.executable);

        if (executionCommand.commandLabel == null)
            executionCommand.commandLabel = taskName;

        // Exec the process
        final Process process;
        try {
            process = Runtime.getRuntime().exec(commandArray, env, new File(executionCommand.workingDirectory));
        } catch (IOException e) {
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, context.getString(R.string.error_failed_to_execute_termux_task_command, executionCommand.getCommandIdAndLabelLogString()), e);
            TermuxTask.processTermuxTaskResult(null, executionCommand);
            return null;
        }

        final TermuxTask termuxTask = new TermuxTask(process, executionCommand, termuxTaskClient);

        if (isSynchronous) {
            try {
                termuxTask.executeInner(context);
            } catch (IllegalThreadStateException | InterruptedException e) {
                // TODO: Should either of these be handled or returned?
            }
        } else {
            new Thread() {
                @Override
                public void run() {
                    try {
                        termuxTask.executeInner(context);
                    } catch (IllegalThreadStateException | InterruptedException e) {
                        // TODO: Should either of these be handled or returned?
                    }
                }
            }.start();
        }

        return termuxTask;
    }

    /**
     * Sets up stdout and stderr readers for the {@link #mProcess} and waits for the process to end.
     *
     * If the processes finishes, then sets {@link ExecutionCommand#stdout}, {@link ExecutionCommand#stderr}
     * and {@link ExecutionCommand#exitCode} for the {@link #mExecutionCommand} of the {@code termuxTask}
     * and then calls {@link #processTermuxTaskResult(TermuxTask, ExecutionCommand) to process the result}.
     *
     * @param context The {@link Context} for operations.
     */
    private void executeInner(@NonNull final Context context) throws IllegalThreadStateException, InterruptedException {
        final int pid = ShellUtils.getPid(mProcess);

        Logger.logDebug(LOG_TAG, "Running \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask with pid " + pid);

        mExecutionCommand.stdout = null;
        mExecutionCommand.stderr = null;
        mExecutionCommand.exitCode = null;


        // setup stdin, and stdout and stderr gobblers
        DataOutputStream STDIN = new DataOutputStream(mProcess.getOutputStream());
        StreamGobbler STDOUT = new StreamGobbler(pid + "-stdout", mProcess.getInputStream(), mStdout);
        StreamGobbler STDERR = new StreamGobbler(pid + "-stderr", mProcess.getErrorStream(), mStderr);

        // start gobbling
        STDOUT.start();
        STDERR.start();

        if (mExecutionCommand.stdin != null && !mExecutionCommand.stdin.isEmpty()) {
            try {
                STDIN.write((mExecutionCommand.stdin + "\n").getBytes(StandardCharsets.UTF_8));
                STDIN.flush();
                STDIN.close();
                //STDIN.write("exit\n".getBytes(StandardCharsets.UTF_8));
                //STDIN.flush();
            } catch(IOException e) {
                if (e.getMessage().contains("EPIPE") || e.getMessage().contains("Stream closed")) {
                    // Method most horrid to catch broken pipe, in which case we
                    // do nothing. The command is not a shell, the shell closed
                    // STDIN, the script already contained the exit command, etc.
                    // these cases we want the output instead of returning null.
                } else {
                    // other issues we don't know how to handle, leads to
                    // returning null
                    mExecutionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, context.getString(R.string.error_exception_received_while_executing_termux_task_command, mExecutionCommand.getCommandIdAndLabelLogString(), e.getMessage()), e);
                    mExecutionCommand.stdout = mStdout.toString();
                    mExecutionCommand.stderr = mStderr.toString();
                    mExecutionCommand.exitCode = -1;
                    TermuxTask.processTermuxTaskResult(this, null);
                    kill();
                    return;
                }
            }
        }

        // wait for our process to finish, while we gobble away in the background
        int exitCode = mProcess.waitFor();

        // make sure our threads are done gobbling
        // and the process is destroyed - while the latter shouldn't be
        // needed in theory, and may even produce warnings, in "normal" Java
        // they are required for guaranteed cleanup of resources, so lets be
        // safe and do this on Android as well
        try {
            STDIN.close();
        } catch (IOException e) {
            // might be closed already
        }
        STDOUT.join();
        STDERR.join();
        mProcess.destroy();

        // Process result
        if (exitCode == 0)
            Logger.logDebug(LOG_TAG, "The \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask with pid " + pid + " exited normally");
        else
            Logger.logDebug(LOG_TAG, "The \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask with pid " + pid + " exited with code: " + exitCode);

        // If the execution command has already failed, like SIGKILL was sent, then don't continue
        if (mExecutionCommand.isStateFailed()) {
            Logger.logDebug(LOG_TAG, "Ignoring setting \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask state to ExecutionState.EXECUTED and processing results since it has already failed");
            return;
        }

        mExecutionCommand.stdout = mStdout.toString();
        mExecutionCommand.stderr = mStderr.toString();
        mExecutionCommand.exitCode = exitCode;

        if (!mExecutionCommand.setState(ExecutionState.EXECUTED))
            return;

        TermuxTask.processTermuxTaskResult(this, null);
    }

    /**
     * Kill this {@link TermuxTask} by sending a {@link OsConstants#SIGILL} to its {@link #mProcess}
     * if its still executing.
     *
     * @param context The {@link Context} for operations.
     * @param processResult If set to {@code true}, then the {@link #processTermuxTaskResult(TermuxTask, ExecutionCommand)}
     *                      will be called to process the failure.
     */
    public void killIfExecuting(@NonNull final Context context, boolean processResult) {
        // If execution command has already finished executing, then no need to process results or send SIGKILL
        if (mExecutionCommand.hasExecuted()) {
            Logger.logDebug(LOG_TAG, "Ignoring sending SIGKILL to \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask since it has already finished executing");
            return;
        }

        Logger.logDebug(LOG_TAG, "Send SIGKILL to \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask");

        if (mExecutionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, context.getString(R.string.error_sending_sigkill_to_process), null)) {
            if (processResult) {
                // Get whatever output has been set till now in case its needed
                mExecutionCommand.stdout = mStdout.toString();
                mExecutionCommand.stderr = mStderr.toString();
                mExecutionCommand.exitCode = 137; // SIGKILL

                TermuxTask.processTermuxTaskResult(this, null);
            }
        }

        if (mExecutionCommand.isExecuting()) {
            kill();
        }
    }

    /**
     * Kill this {@link TermuxTask} by sending a {@link OsConstants#SIGILL} to its {@link #mProcess}.
     */
    public void kill() {
        int pid = ShellUtils.getPid(mProcess);
        try {
            // Send SIGKILL to process
            Os.kill(pid, OsConstants.SIGKILL);
        } catch (ErrnoException e) {
            Logger.logWarn(LOG_TAG, "Failed to send SIGKILL to \"" + mExecutionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask with pid " + pid + ": " + e.getMessage());
        }
    }

    /**
     * Process the results of {@link TermuxTask} or {@link ExecutionCommand}.
     *
     * Only one of {@code termuxTask} and {@code executionCommand} must be set.
     *
     * If the {@code termuxTask} and its {@link #mTermuxTaskClient} are not {@code null},
     * then the {@link TermuxTaskClient#onTermuxTaskExited(TermuxTask)} callback will be called.
     *
     * @param termuxTask The {@link TermuxTask}, which should be set if
     *                  {@link #execute(Context, ExecutionCommand, TermuxTaskClient, boolean)}
     *                   successfully started the process.
     * @param executionCommand The {@link ExecutionCommand}, which should be set if
     *                          {@link #execute(Context, ExecutionCommand, TermuxTaskClient, boolean)}
     *                          failed to start the process.
     */
    private static void processTermuxTaskResult(final TermuxTask termuxTask, ExecutionCommand executionCommand) {
        if (termuxTask != null)
            executionCommand = termuxTask.mExecutionCommand;

        if (executionCommand == null) return;

        if (executionCommand.shouldNotProcessResults()) {
            Logger.logDebug(LOG_TAG, "Ignoring duplicate call to process \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask result");
            return;
        }

        Logger.logDebug(LOG_TAG, "Processing \"" + executionCommand.getCommandIdAndLabelLogString() + "\" TermuxTask result");

        if (termuxTask != null && termuxTask.mTermuxTaskClient != null) {
            termuxTask.mTermuxTaskClient.onTermuxTaskExited(termuxTask);
        } else {
            // If a callback is not set and execution command didn't fail, then we set success state now
            // Otherwise, the callback host can set it himself when its done with the termuxTask
            if (!executionCommand.isStateFailed())
                executionCommand.setState(ExecutionCommand.ExecutionState.SUCCESS);
        }
    }

    public Process getProcess() {
        return mProcess;
    }

    public ExecutionCommand getExecutionCommand() {
        return mExecutionCommand;
    }



    public interface TermuxTaskClient {

        /**
         * Callback function for when {@link TermuxTask} exits.
         *
         * @param termuxTask The {@link TermuxTask} that exited.
         */
        void onTermuxTaskExited(TermuxTask termuxTask);

    }

}
