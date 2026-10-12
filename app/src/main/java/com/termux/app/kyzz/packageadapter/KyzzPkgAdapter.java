package com.termux.app.kyzz.packageadapter;

import java.io.File;

public class KyzzPkgAdapter implements KyzzPackageManagerAdapter {
    @Override
    public String getName() {
        return "pkg";
    }

    @Override
    public boolean isAvailable(String prefixPath) {
        if (prefixPath == null) prefixPath = "/data/data/com.termux/files/usr";
        File pkg = new File(prefixPath + "/bin/pkg");
        File apt = new File(prefixPath + "/bin/apt");
        return (pkg.exists() && pkg.canExecute()) || (apt.exists() && apt.canExecute());
    }

    @Override
    public String buildInstallCommand(String packageName) {
        return "pkg install -y " + sanitize(packageName);
    }

    @Override
    public String buildRemoveCommand(String packageName) {
        return "pkg uninstall -y " + sanitize(packageName);
    }

    @Override
    public String buildUpdateCommand(String packageName) {
        return "pkg upgrade -y " + sanitize(packageName);
    }

    private String sanitize(String input) {
        if (input == null) return "";
        return input.replaceAll("[^a-zA-Z0-9_.-]", "");
    }
}
