package com.termux.app;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.Intent;
import android.os.Bundle;

import com.termux.app.utils.Logger;
import com.termux.app.utils.ShellUtils;
import com.termux.models.ExecutionCommand;
import com.termux.models.ExecutionCommand.ExecutionState;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;

/**
 * A background job launched by Termux.
 */
public final class BackgroundJob {

    Process mProcess;

    private static final String LOG_TAG = "BackgroundJob";

    public BackgroundJob(String executable, final String[] arguments, String workingDirectory, final TermuxService service){
        this(new ExecutionCommand(TermuxService.getNextExecutionId(), executable, arguments, workingDirectory, false, false), service);
    }

    public BackgroundJob(ExecutionCommand executionCommand, final TermuxService service) {
        String[] env = ShellUtils.buildEnvironment(false, executionCommand.workingDirectory);

        if (executionCommand.workingDirectory == null || executionCommand.workingDirectory.isEmpty())
            executionCommand.workingDirectory = TermuxConstants.TERMUX_HOME_DIR_PATH;

        final String[] commandArray = ShellUtils.setupProcessArgs(executionCommand.executable, executionCommand.arguments);
        final String commandDescription = Arrays.toString(commandArray);

        if(!executionCommand.setState(ExecutionState.EXECUTING))
            return;

        Process process;
        try {
            process = Runtime.getRuntime().exec(commandArray, env, new File(executionCommand.workingDirectory));
        } catch (IOException e) {
            mProcess = null;
            // TODO: Visible error message?
            Logger.logStackTraceWithMessage(LOG_TAG, "Failed running background job: " + commandDescription, e);
            return;
        }

        mProcess = process;
        final int pid = ShellUtils.getPid(mProcess);
        final Bundle result = new Bundle();
        final StringBuilder outResult = new StringBuilder();
        final StringBuilder errResult = new StringBuilder();

        Thread errThread = new Thread() {
            @Override
            public void run() {
                InputStream stderr = mProcess.getErrorStream();
                BufferedReader reader = new BufferedReader(new InputStreamReader(stderr, StandardCharsets.UTF_8));
                String line;
                try {
                    // FIXME: Long lines.
                    while ((line = reader.readLine()) != null) {
                        errResult.append(line).append('\n');
                        Logger.logDebug(LOG_TAG, "[" + pid + "] stderr: " + line);
                    }
                } catch (IOException e) {
                    // Ignore.
                }
            }
        };
        errThread.start();

        new Thread() {
            @Override
            public void run() {
                Logger.logDebug(LOG_TAG, "[" + pid + "] starting: " + commandDescription);
                InputStream stdout = mProcess.getInputStream();
                BufferedReader reader = new BufferedReader(new InputStreamReader(stdout, StandardCharsets.UTF_8));

                String line;
                try {
                    // FIXME: Long lines.
                    while ((line = reader.readLine()) != null) {
                        Logger.logDebug(LOG_TAG, "[" + pid + "] stdout: " + line);
                        outResult.append(line).append('\n');
                    }
                } catch (IOException e) {
                    Logger.logStackTraceWithMessage(LOG_TAG, "Error reading output", e);
                }

                try {
                    int exitCode = mProcess.waitFor();
                    service.onBackgroundJobExited(BackgroundJob.this);
                    if (exitCode == 0) {
                        Logger.logDebug(LOG_TAG, "[" + pid + "] exited normally");
                    } else {
                        Logger.logDebug(LOG_TAG, "[" + pid + "] exited with code: " + exitCode);
                    }

                    result.putString("stdout", outResult.toString());
                    result.putInt("exitCode", exitCode);

                    errThread.join();
                    result.putString("stderr", errResult.toString());

                    if(!executionCommand.setState(ExecutionState.EXECUTED))
                        return;

                    Intent data = new Intent();
                    data.putExtra("result", result);

                    if(executionCommand.pluginPendingIntent != null) {
                        try {
                            executionCommand.pluginPendingIntent.send(service.getApplicationContext(), Activity.RESULT_OK, data);
                        } catch (PendingIntent.CanceledException e) {
                            // The caller doesn't want the result? That's fine, just ignore
                        }
                    }

                    executionCommand.setState(ExecutionState.SUCCESS);
                } catch (InterruptedException e) {
                    // Ignore
                }
            }
        }.start();
    }

}
