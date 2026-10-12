package com.termux.app.kyzz.sessions;

import java.io.Serializable;

public class KyzzSessionInfo implements Serializable {
    private final String mHandle;
    private String mName;
    private final String mCwd;
    private final boolean mIsActive;

    public KyzzSessionInfo(String handle, String name, String cwd, boolean isActive) {
        mHandle = handle;
        mName = name;
        mCwd = cwd;
        mIsActive = isActive;
    }

    public String getHandle() {
        return mHandle;
    }

    public String getName() {
        return mName;
    }

    public void setName(String name) {
        mName = name;
    }

    public String getCwd() {
        return mCwd;
    }

    public boolean isActive() {
        return mIsActive;
    }
}
