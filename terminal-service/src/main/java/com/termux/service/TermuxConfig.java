package com.termux.service;


import android.annotation.SuppressLint;

public final class TermuxConfig {
    /**
     * Note that this is a symlink on the Android M preview.
     */
    @SuppressLint("SdCardPath")
    public static String FILES_PATH = "/data/data/com.termux/files";
    public static String PREFIX_PATH = FILES_PATH + "/usr";
    public static String HOME_PATH = FILES_PATH + "/home";
}
