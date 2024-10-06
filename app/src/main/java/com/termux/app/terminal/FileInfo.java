package com.termux.app.terminal;

public class FileInfo {
    private final String name;
    private final String path;
    private final boolean isDirectory;

    public FileInfo(String name, String path, boolean isDirectory) {
        this.name = name;
        this.path = path;
        this.isDirectory = isDirectory;
    }

    public String getName() {
        return name;
    }

    public String getPath() {
        return path;
    }

    public boolean isDirectory() {
        return isDirectory;
    }
}

