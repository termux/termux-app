package com.termux.app.kyzz.packageadapter;

import java.io.File;

public class KyzzNpmAdapter implements KyzzPackageManagerAdapter {
    @Override
    public String getName() {
        return "npm";
    }

    @Override
    public boolean isAvailable(String prefixPath) {
        if (prefixPath == null) prefixPath = "/data/data/com.termux/files/usr";
        File npm = new File(prefixPath + "/bin/npm");
        return npm.exists() && npm.canExecute();
    }

    @Override
    public String buildInstallCommand(String packageName) {
        return "npm install -g " + sanitize(packageName);
    }

    @Override
    public String buildRemoveCommand(String packageName) {
        return "npm uninstall -g " + sanitize(packageName);
    }

    @Override
    public String buildUpdateCommand(String packageName) {
        return "npm update -g " + sanitize(packageName);
    }

    private String sanitize(String input) {
        if (input == null) return "";
        return input.replaceAll("[^a-zA-Z0-9_.-@/]", "");
    }
}
