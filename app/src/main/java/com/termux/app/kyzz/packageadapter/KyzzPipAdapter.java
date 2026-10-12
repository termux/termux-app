package com.termux.app.kyzz.packageadapter;

import java.io.File;

public class KyzzPipAdapter implements KyzzPackageManagerAdapter {
    @Override
    public String getName() {
        return "pip";
    }

    @Override
    public boolean isAvailable(String prefixPath) {
        if (prefixPath == null) prefixPath = "/data/data/com.termux/files/usr";
        File pip = new File(prefixPath + "/bin/pip");
        File pip3 = new File(prefixPath + "/bin/pip3");
        return (pip.exists() && pip.canExecute()) || (pip3.exists() && pip3.canExecute());
    }

    @Override
    public String buildInstallCommand(String packageName) {
        return "pip install " + sanitize(packageName);
    }

    @Override
    public String buildRemoveCommand(String packageName) {
        return "pip uninstall -y " + sanitize(packageName);
    }

    @Override
    public String buildUpdateCommand(String packageName) {
        return "pip install --upgrade " + sanitize(packageName);
    }

    private String sanitize(String input) {
        if (input == null) return "";
        return input.replaceAll("[^a-zA-Z0-9_.-]", "");
    }
}
