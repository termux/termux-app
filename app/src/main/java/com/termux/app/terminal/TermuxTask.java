package com.termux.app.terminal;

import androidx.annotation.NonNull;

import com.termux.R;
import com.termux.app.TermuxConstants;
import com.termux.app.TermuxService;
import com.termux.app.shell.StreamGobbler;
import com.termux.app.utils.Logger;
import com.termux.app.utils.PluginUtils;
import com.termux.app.utils.ShellUtils;
import com.termux.app.models.ExecutionCommand;
import com.termux.app.models.ExecutionCommand.ExecutionState;

import java.io.File;
import java.io.IOException;

/**
 * A class that maintains info for background Termux tasks.
 * It also provides a way to link each {@link Process} with the {@link ExecutionCommand}
 * that started it.
 */
public final class TermuxTask {

    private final Process mProcess;
    private final ExecutionCommand mExecutionCommand;

    private static final String LOG_TAG = "TermuxTask";

    private TermuxTask(Process process, ExecutionCommand executionCommand) {
        this.mProcess = process;
        this.mExecutionCommand = executionCommand;
    }

    public static TermuxTask create(@NonNull final TermuxService service, @NonNull ExecutionCommand executionCommand) {
        if (executionCommand.workingDirectory == null || executionCommand.workingDirectory.isEmpty()) executionCommand.workingDirectory = TermuxConstants.TERMUX_HOME_DIR_PATH;

        String[] env = ShellUtils.buildEnvironment(service, false, executionCommand.workingDirectory);

        final String[] commandArray = ShellUtils.setupProcessArgs(executionCommand.executable, executionCommand.arguments);
        // final String commandDescription = Arrays.toString(commandArray);

        if(!executionCommand.setState(ExecutionState.EXECUTING))
            return null;

        Logger.logDebug(LOG_TAG, executionCommand.toString());

        String taskName = ShellUtils.getExecutableBasename(executionCommand.executable);

        if(executionCommand.commandLabel == null)
            executionCommand.commandLabel = taskName;

        // Exec the process
        final Process process;
        try {
            process = Runtime.getRuntime().exec(commandArray, env, new File(executionCommand.workingDirectory));
        } catch (IOException e) {
            executionCommand.setStateFailed(ExecutionCommand.RESULT_CODE_FAILED, service.getString(R.string.error_failed_to_execute_termux_task_command, executionCommand.getCommandIdAndLabelLogString()), e);
            TermuxSession.processTermuxSessionResult(service, null, executionCommand);
            return null;
        }

        final int pid = ShellUtils.getPid(process);

        Logger.logDebug(LOG_TAG, "Running \"" + executionCommand.commandLabel + "\" background task with pid " + pid);

        final TermuxTask termuxTask = new TermuxTask(process, executionCommand);

        StringBuilder stdout = new StringBuilder();
        StringBuilder stderr = new StringBuilder();

        new Thread() {
            @Override
            public void run() {
                try {
                    // setup stdout and stderr gobblers
                    StreamGobbler STDOUT = new StreamGobbler(pid + "-stdout", process.getInputStream(), stdout);
                    StreamGobbler STDERR = new StreamGobbler(pid + "-stderr", process.getErrorStream(), stderr);

                    // start gobbling
                    STDOUT.start();
                    STDERR.start();

                    // wait for our process to finish, while we gobble away in the
                    // background
                    int exitCode = process.waitFor();

                    // make sure our threads are done gobbling
                    // and the process is destroyed - while the latter shouldn't be
                    // needed in theory, and may even produce warnings, in "normal" Java
                    // they are required for guaranteed cleanup of resources, so lets be
                    // safe and do this on Android as well
                    STDOUT.join();
                    STDERR.join();
                    process.destroy();


                    // Process result
                    if (exitCode == 0)
                        Logger.logDebug(LOG_TAG, "The \"" + executionCommand.commandLabel + "\" background task with pid " + pid + " exited normally");
                    else
                        Logger.logDebug(LOG_TAG, "The \"" + executionCommand.commandLabel + "\" background task with pid " + pid + " exited with code: " + exitCode);

                    executionCommand.stdout = stdout.toString();
                    executionCommand.stderr = stderr.toString();
                    executionCommand.exitCode = exitCode;

                    if(!executionCommand.setState(ExecutionState.EXECUTED))
                        return;

                    TermuxTask.processTermuxTaskResult(service, termuxTask, null);

                } catch (IllegalThreadStateException | InterruptedException e) {
                    // TODO: Should either of these be handled or returned?
                }
            }
        }.start();

        return termuxTask;
    }

    public static void processTermuxTaskResult(@NonNull final TermuxService service, final TermuxTask termuxTask, ExecutionCommand executionCommand) {
        if(termuxTask != null)
            executionCommand = termuxTask.mExecutionCommand;

        if(executionCommand == null) return;

        PluginUtils.processPluginExecutionCommandResult(service.getApplicationContext(), LOG_TAG, executionCommand);

        if(termuxTask != null)
            service.onTermuxTaskExited(termuxTask);
    }

    public Process getTerminalSession() {
        return mProcess;
    }

    public ExecutionCommand getExecutionCommand() {
        return mExecutionCommand;
    }

}
