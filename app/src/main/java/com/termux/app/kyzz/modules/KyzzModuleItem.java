package com.termux.app.kyzz.modules;

import java.io.Serializable;

public class KyzzModuleItem implements Serializable {
    private final String mName;
    private final String mDescription;
    private final String mCategory;
    private final String mPackageManager; // "pkg", "npm", "pip"
    private final String mInstallCommand;
    private boolean mIsInstalled;
    private String mVersion;

    public KyzzModuleItem(String name, String description, String category, String packageManager, String installCommand, boolean isInstalled, String version) {
        mName = name;
        mDescription = description;
        mCategory = category;
        mPackageManager = packageManager;
        mInstallCommand = installCommand;
        mIsInstalled = isInstalled;
        mVersion = version;
    }

    public String getName() {
        return mName;
    }

    public String getDescription() {
        return mDescription;
    }

    public String getCategory() {
        return mCategory;
    }

    public String getPackageManager() {
        return mPackageManager;
    }

    public String getInstallCommand() {
        return mInstallCommand;
    }

    public boolean isInstalled() {
        return mIsInstalled;
    }

    public void setInstalled(boolean installed) {
        mIsInstalled = installed;
    }

    public String getVersion() {
        return mVersion;
    }

    public void setVersion(String version) {
        mVersion = version;
    }
}
