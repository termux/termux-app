package com.termux.shared.shell.command.environment;

import android.content.Context;

import androidx.annotation.NonNull;

import com.termux.shared.shell.command.ExecutionCommand;

import java.util.HashMap;

/**
 * Environment for {@link ExecutionCommand}.
 */
public class ShellCommandShellEnvironment {

    /** Environment variable prefix for the {@link ExecutionCommand}. */
    public static final String SHELL_CMD_ENV_PREFIX = "SHELL_CMD__";

    /** Environment variable for the {@link ExecutionCommand.Runner} name. */
    public static final String ENV_SHELL_CMD__RUNNER_NAME = SHELL_CMD_ENV_PREFIX + "RUNNER_NAME";

    /** Environment variable for the package name running the {@link ExecutionCommand}. */
    public static final String ENV_SHELL_CMD__PACKAGE_NAME = SHELL_CMD_ENV_PREFIX + "PACKAGE_NAME";

    /** Environment variable for the {@link ExecutionCommand#id}/TermuxShellManager.SHELL_ID name.
     * This will be common for all runners. */
    public static final String ENV_SHELL_CMD__SHELL_ID = SHELL_CMD_ENV_PREFIX + "SHELL_ID";

    /** Environment variable for the {@link ExecutionCommand#shellName} name. */
    public static final String ENV_SHELL_CMD__SHELL_NAME = SHELL_CMD_ENV_PREFIX + "SHELL_NAME";
    
    /** Environment variable for the {@link ExecutionCommand.Runner#APP_SHELL} number since boot. */
    public static final String ENV_SHELL_CMD__APP_SHELL_NUMBER_SINCE_BOOT = SHELL_CMD_ENV_PREFIX + "APP_SHELL_NUMBER_SINCE_BOOT";

    /** Environment variable for the {{@link ExecutionCommand.Runner#TERMINAL_SESSION} number since boot. */
    public static final String ENV_SHELL_CMD__TERMINAL_SESSION_NUMBER_SINCE_BOOT = SHELL_CMD_ENV_PREFIX + "TERMINAL_SESSION_NUMBER_SINCE_BOOT";

    /** Environment variable for the {@link ExecutionCommand.Runner#APP_SHELL} number since app start. */
    public static final String ENV_SHELL_CMD__APP_SHELL_NUMBER_SINCE_APP_START = SHELL_CMD_ENV_PREFIX + "APP_SHELL_NUMBER_SINCE_APP_START";

    /** Environment variable for the {@link ExecutionCommand.Runner#TERMINAL_SESSION} number since app start. */
    public static final String ENV_SHELL_CMD__TERMINAL_SESSION_NUMBER_SINCE_APP_START = SHELL_CMD_ENV_PREFIX + "TERMINAL_SESSION_NUMBER_SINCE_APP_START";


    /** Get shell environment containing info for {@link ExecutionCommand}. */
    @NonNull
    public HashMap<String, String> getEnvironment(@NonNull Context currentPackageContext,
                                                  @NonNull ExecutionCommand executionCommand) {
        HashMap<String, String> environment = new HashMap<>();

        ExecutionCommand.Runner runner = ExecutionCommand.Runner.runnerOf(executionCommand.runner);
        if (runner == null) return environment;

        ShellEnvironmentUtils.putToEnvIfSet(environment, ENV_SHELL_CMD__RUNNER_NAME, runner.getName());
        ShellEnvironmentUtils.putToEnvIfSet(environment, ENV_SHELL_CMD__PACKAGE_NAME, currentPackageContext.getPackageName());
        ShellEnvironmentUtils.putToEnvIfSet(environment, ENV_SHELL_CMD__SHELL_ID, String.valueOf(executionCommand.id));
        ShellEnvironmentUtils.putToEnvIfSet(environment, ENV_SHELL_CMD__SHELL_NAME, executionCommand.shellName);

        return environment;
    }

}
