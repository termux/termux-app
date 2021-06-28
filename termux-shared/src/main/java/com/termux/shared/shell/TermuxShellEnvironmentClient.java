package com.termux.shared.shell;

import android.content.Context;

import androidx.annotation.NonNull;

public class TermuxShellEnvironmentClient implements ShellEnvironmentClient {

    @NonNull
    @Override
    public String getDefaultWorkingDirectoryPath() {
        return TermuxShellUtils.getDefaultWorkingDirectoryPath();
    }

    @NonNull
    @Override
    public String getDefaultBinPath() {
        return TermuxShellUtils.getDefaultBinPath();
    }

    @NonNull
    @Override
    public String[] buildEnvironment(Context currentPackageContext, boolean isFailSafe, String workingDirectory) {
        return TermuxShellUtils.buildEnvironment(currentPackageContext, isFailSafe, workingDirectory);
    }

    @NonNull
    @Override
    public String[] setupProcessArgs(@NonNull String fileToExecute, String[] arguments) {
        return TermuxShellUtils.setupProcessArgs(fileToExecute, arguments);
    }

}
