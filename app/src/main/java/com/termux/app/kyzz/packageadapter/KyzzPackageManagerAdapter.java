package com.termux.app.kyzz.packageadapter;

public interface KyzzPackageManagerAdapter {
    String getName();
    boolean isAvailable(String prefixPath);
    String buildInstallCommand(String packageName);
    String buildRemoveCommand(String packageName);
    String buildUpdateCommand(String packageName);
}
