package com.termux.shared.file.filesystem;

import android.system.ErrnoException;
import android.system.Os;

import java.io.File;
import java.io.FileDescriptor;
import java.io.IOException;

public class NativeDispatcher {

    public static void stat(String filePath, FileAttributes fileAttributes) throws IOException {
        validateFileExistence(filePath);

        try {
            fileAttributes.loadFromStructStat(Os.stat(filePath));
        } catch (ErrnoException e) {
            throw new IOException("Failed to run Os.stat() on file at path \"" + filePath + "\": " + e.getMessage());
        }
    }

    public static void lstat(String filePath, FileAttributes fileAttributes) throws IOException {
        validateFileExistence(filePath);

        try {
            fileAttributes.loadFromStructStat(Os.lstat(filePath));
        } catch (ErrnoException e) {
            throw new IOException("Failed to run Os.lstat() on file at path \"" + filePath + "\": " + e.getMessage());
        }
    }

    public static void fstat(FileDescriptor fileDescriptor, FileAttributes fileAttributes) throws IOException {
        validateFileDescriptor(fileDescriptor);

        try {
            fileAttributes.loadFromStructStat(Os.fstat(fileDescriptor));
        } catch (ErrnoException e) {
            throw new IOException("Failed to run Os.fstat() on file descriptor \"" + fileDescriptor.toString() + "\": " + e.getMessage());
        }
    }

    public static void validateFileExistence(String filePath) throws IOException {
        if (filePath == null || filePath.isEmpty()) throw new IOException("The path is null or empty");

        File file = new File(filePath);

        //if (!file.exists())
        //    throw new IOException("No such file or directory: \"" + filePath + "\"");
    }

    public static void validateFileDescriptor(FileDescriptor fileDescriptor) throws IOException {
        if (fileDescriptor == null) throw new IOException("The file descriptor is null");

        if (!fileDescriptor.valid())
            throw new IOException("No such file descriptor: \"" + fileDescriptor.toString() + "\"");
    }

}
