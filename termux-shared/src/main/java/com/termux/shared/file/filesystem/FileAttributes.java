/*
 * Copyright (c) 2008, 2013, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

package com.termux.shared.file.filesystem;

import android.os.Build;
import android.system.StructStat;

import androidx.annotation.NonNull;

import java.io.File;
import java.io.FileDescriptor;
import java.io.IOException;
import java.util.concurrent.TimeUnit;
import java.util.Set;
import java.util.HashSet;

/**
 * Unix implementation of PosixFileAttributes.
 * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:libcore/ojluni/src/main/java/sun/nio/fs/UnixFileAttributes.java
 */

public class FileAttributes {
    private String filePath;
    private FileDescriptor fileDescriptor;

    private int st_mode;
    private long st_ino;
    private long st_dev;
    private long st_rdev;
    private long st_nlink;
    private int st_uid;
    private int st_gid;
    private long st_size;
    private long st_blksize;
    private long st_blocks;
    private long st_atime_sec;
    private long st_atime_nsec;
    private long st_mtime_sec;
    private long st_mtime_nsec;
    private long st_ctime_sec;
    private long st_ctime_nsec;

    // created lazily
    private volatile String owner;
    private volatile String group;
    private volatile FileKey key;

    private FileAttributes(String filePath) {
        this.filePath = filePath;
    }

    private FileAttributes(FileDescriptor fileDescriptor) {
        this.fileDescriptor = fileDescriptor;
    }

    // get the FileAttributes for a given file
    public static FileAttributes get(String filePath, boolean followLinks) throws IOException {
        FileAttributes fileAttributes;

        if (filePath == null || filePath.isEmpty())
            fileAttributes = new FileAttributes((String) null);
        else
            fileAttributes = new FileAttributes(new File(filePath).getAbsolutePath());

        if (followLinks) {
            NativeDispatcher.stat(filePath, fileAttributes);
        } else {
            NativeDispatcher.lstat(filePath, fileAttributes);
        }

        // Logger.logDebug(fileAttributes.toString());

        return fileAttributes;
    }

    // get the FileAttributes for an open file
    public static FileAttributes get(FileDescriptor fileDescriptor) throws IOException {
        FileAttributes fileAttributes = new FileAttributes(fileDescriptor);
        NativeDispatcher.fstat(fileDescriptor, fileAttributes);
        return fileAttributes;
    }

    public String file() {
        if (filePath != null)
            return filePath;
        else if (fileDescriptor != null)
            return fileDescriptor.toString();
        else
            return null;
    }

    // package-private
    public boolean isSameFile(FileAttributes attrs) {
        return ((st_ino == attrs.st_ino) && (st_dev == attrs.st_dev));
    }

    // package-private
    public int mode() {
        return st_mode;
    }

    public long blksize() {
        return st_blksize;
    }

    public long blocks() {
        return st_blocks;
    }

    public long ino() {
        return st_ino;
    }

    public long dev() {
        return st_dev;
    }

    public long rdev() {
        return st_rdev;
    }

    public long nlink() {
        return st_nlink;
    }

    public int uid() {
        return st_uid;
    }

    public int gid() {
        return st_gid;
    }

    private static FileTime toFileTime(long sec, long nsec) {
        if (nsec == 0) {
            return FileTime.from(sec, TimeUnit.SECONDS);
        } else {
            // truncate to microseconds to avoid overflow with timestamps
            // way out into the future. We can re-visit this if FileTime
            // is updated to define a from(secs,nsecs) method.
            long micro = sec * 1000000L + nsec / 1000L;
            return FileTime.from(micro, TimeUnit.MICROSECONDS);
        }
    }

    public FileTime lastAccessTime() {
        return toFileTime(st_atime_sec, st_atime_nsec);
    }

    public FileTime lastModifiedTime() {
        return toFileTime(st_mtime_sec, st_mtime_nsec);
    }

    public FileTime lastChangeTime() {
        return toFileTime(st_ctime_sec, st_ctime_nsec);
    }

    public FileTime creationTime() {
        return lastModifiedTime();
    }

    public boolean isRegularFile() {
        return ((st_mode & UnixConstants.S_IFMT) == UnixConstants.S_IFREG);
    }

    public boolean isDirectory() {
        return ((st_mode & UnixConstants.S_IFMT) == UnixConstants.S_IFDIR);
    }

    public boolean isSymbolicLink() {
        return ((st_mode & UnixConstants.S_IFMT) == UnixConstants.S_IFLNK);
    }

    public boolean isCharacter() {
        return ((st_mode & UnixConstants.S_IFMT) == UnixConstants.S_IFCHR);
    }

    public boolean isFifo() {
        return ((st_mode & UnixConstants.S_IFMT) == UnixConstants.S_IFIFO);
    }

    public boolean isSocket() {
        return ((st_mode & UnixConstants.S_IFMT) == UnixConstants.S_IFSOCK);
    }

    public boolean isBlock() {
        return ((st_mode & UnixConstants.S_IFMT) == UnixConstants.S_IFBLK);
    }

    public boolean isOther() {
        int type = st_mode & UnixConstants.S_IFMT;
        return (type != UnixConstants.S_IFREG &&
            type != UnixConstants.S_IFDIR &&
            type != UnixConstants.S_IFLNK);
    }

    public boolean isDevice() {
        int type = st_mode & UnixConstants.S_IFMT;
        return (type == UnixConstants.S_IFCHR ||
            type == UnixConstants.S_IFBLK ||
            type == UnixConstants.S_IFIFO);
    }

    public long size() {
        return st_size;
    }

    public FileKey fileKey() {
        if (key == null) {
            synchronized (this) {
                if (key == null) {
                    key = new FileKey(st_dev, st_ino);
                }
            }
        }
        return key;
    }

    public String owner() {
        if (owner == null) {
            synchronized (this) {
                if (owner == null) {
                    owner = Integer.toString(st_uid);
                }
            }
        }
        return owner;
    }

    public String group() {
        if (group == null) {
            synchronized (this) {
                if (group == null) {
                    group = Integer.toString(st_gid);
                }
            }
        }
        return group;
    }

    public Set<FilePermission> permissions() {
        int bits = (st_mode & UnixConstants.S_IAMB);
        HashSet<FilePermission> perms = new HashSet<>();

        if ((bits & UnixConstants.S_IRUSR) > 0)
            perms.add(FilePermission.OWNER_READ);
        if ((bits & UnixConstants.S_IWUSR) > 0)
            perms.add(FilePermission.OWNER_WRITE);
        if ((bits & UnixConstants.S_IXUSR) > 0)
            perms.add(FilePermission.OWNER_EXECUTE);

        if ((bits & UnixConstants.S_IRGRP) > 0)
            perms.add(FilePermission.GROUP_READ);
        if ((bits & UnixConstants.S_IWGRP) > 0)
            perms.add(FilePermission.GROUP_WRITE);
        if ((bits & UnixConstants.S_IXGRP) > 0)
            perms.add(FilePermission.GROUP_EXECUTE);

        if ((bits & UnixConstants.S_IROTH) > 0)
            perms.add(FilePermission.OTHERS_READ);
        if ((bits & UnixConstants.S_IWOTH) > 0)
            perms.add(FilePermission.OTHERS_WRITE);
        if ((bits & UnixConstants.S_IXOTH) > 0)
            perms.add(FilePermission.OTHERS_EXECUTE);

        return perms;
    }

    public void loadFromStructStat(StructStat structStat) {
        this.st_mode = structStat.st_mode;
        this.st_ino = structStat.st_ino;
        this.st_dev = structStat.st_dev;
        this.st_rdev = structStat.st_rdev;
        this.st_nlink = structStat.st_nlink;
        this.st_uid = structStat.st_uid;
        this.st_gid = structStat.st_gid;
        this.st_size = structStat.st_size;
        this.st_blksize = structStat.st_blksize;
        this.st_blocks = structStat.st_blocks;

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O_MR1) {
            this.st_atime_sec = structStat.st_atim.tv_sec;
            this.st_atime_nsec = structStat.st_atim.tv_nsec;
            this.st_mtime_sec = structStat.st_mtim.tv_sec;
            this.st_mtime_nsec = structStat.st_mtim.tv_nsec;
            this.st_ctime_sec = structStat.st_ctim.tv_sec;
            this.st_ctime_nsec = structStat.st_ctim.tv_nsec;
        } else {
            this.st_atime_sec = structStat.st_atime;
            this.st_atime_nsec = 0;
            this.st_mtime_sec = structStat.st_mtime;
            this.st_mtime_nsec = 0;
            this.st_ctime_sec = structStat.st_ctime;
            this.st_ctime_nsec = 0;
        }
    }

    public String getFileString() {
        return "File: `" + file() + "`";
    }

    public String getTypeString() {
        return "Type: `" + FileTypes.getFileType(this).getName() + "`";
    }

    public String getSizeString() {
        return "Size: `" + size() + "`";
    }

    public String getBlocksString() {
        return "Blocks: `" + blocks() + "`";
    }

    public String getIOBlockString() {
        return "IO Block: `" + blksize() + "`";
    }

    public String getDeviceString() {
        return "Device: `" + Long.toHexString(st_dev) + "`";
    }

    public String getInodeString() {
        return "Inode: `" + st_ino + "`";
    }

    public String getLinksString() {
        return "Links: `" + nlink() + "`";
    }

    public String getDeviceTypeString() {
        return "Device Type: `" + rdev() + "`";
    }

    public String getOwnerString() {
        return "Owner: `" + owner() + "`";
    }

    public String getGroupString() {
        return "Group: `" + group() + "`";
    }

    public String getPermissionString() {
        return "Permissions: `" + FilePermissions.toString(permissions()) + "`";
    }

    public String getAccessTimeString() {
        return "Access Time: `" + lastAccessTime() + "`";
    }

    public String getModifiedTimeString() {
        return "Modified Time: `" + lastModifiedTime() + "`";
    }

    public String getChangeTimeString() {
        return "Change Time: `" + lastChangeTime() + "`";
    }

    @NonNull
    @Override
    public String toString() {
        return getFileAttributesLogString(this);
    }

    public static String getFileAttributesLogString(final FileAttributes fileAttributes) {
        if (fileAttributes == null) return "null";

        StringBuilder logString = new StringBuilder();

        logString.append(fileAttributes.getFileString());

        logString.append("\n").append(fileAttributes.getTypeString());

        logString.append("\n").append(fileAttributes.getSizeString());
        logString.append("\n").append(fileAttributes.getBlocksString());
        logString.append("\n").append(fileAttributes.getIOBlockString());

        logString.append("\n").append(fileAttributes.getDeviceString());
        logString.append("\n").append(fileAttributes.getInodeString());
        logString.append("\n").append(fileAttributes.getLinksString());

        if (fileAttributes.isBlock() || fileAttributes.isCharacter())
            logString.append("\n").append(fileAttributes.getDeviceTypeString());

        logString.append("\n").append(fileAttributes.getOwnerString());
        logString.append("\n").append(fileAttributes.getGroupString());
        logString.append("\n").append(fileAttributes.getPermissionString());

        logString.append("\n").append(fileAttributes.getAccessTimeString());
        logString.append("\n").append(fileAttributes.getModifiedTimeString());
        logString.append("\n").append(fileAttributes.getChangeTimeString());

        return logString.toString();
    }

}
