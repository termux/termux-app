package com.termux.shared.shell;

import android.content.Context;

import androidx.annotation.NonNull;

public interface ShellEnvironmentClient {

    /**
     * Get the default working directory path for the environment in case the path that was passed
     * was {@code null} or empty.
     *
     * @return Should return the default working directory path.
     */
    @NonNull
    String getDefaultWorkingDirectoryPath();

    /**
     * Get the default "/bin" path, likely $PREFIX/bin.
     *
     * @return Should return the "/bin" path.
     */
    @NonNull
    String getDefaultBinPath();

    /**
     * Build the shell environment to be used for commands.
     *
     * @param currentPackageContext The {@link Context} for the current package.
     * @param isFailSafe If running a failsafe session.
     * @param workingDirectory The working directory for the environment.
     * @return Should return the build environment.
     */
    @NonNull
    String[] buildEnvironment(Context currentPackageContext, boolean isFailSafe, String workingDirectory);

    /**
     * Setup process arguments for the file to execute, like interpreter, etc.
     *
     * @param fileToExecute The file to execute.
     * @param arguments     The arguments to pass to the executable.
     * @return Should return the final process arguments.
     */
    @NonNull
    String[] setupProcessArgs(@NonNull String fileToExecute, String[] arguments);

}
