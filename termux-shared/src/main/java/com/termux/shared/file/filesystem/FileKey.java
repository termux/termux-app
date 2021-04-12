/*
 * Copyright (c) 2008, 2009, Oracle and/or its affiliates. All rights reserved.
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

/**
 * Container for device/inode to uniquely identify file.
 * https://cs.android.com/android/platform/superproject/+/android-11.0.0_r3:libcore/ojluni/src/main/java/sun/nio/fs/UnixFileKey.java
 */

public class FileKey {
    private final long st_dev;
    private final long st_ino;

    FileKey(long st_dev, long st_ino) {
        this.st_dev = st_dev;
        this.st_ino = st_ino;
    }

    @Override
    public int hashCode() {
        return (int)(st_dev ^ (st_dev >>> 32)) +
            (int)(st_ino ^ (st_ino >>> 32));
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == this)
            return true;
        if (!(obj instanceof FileKey))
            return false;
        FileKey other = (FileKey)obj;
        return (this.st_dev == other.st_dev) && (this.st_ino == other.st_ino);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("(dev=")
            .append(Long.toHexString(st_dev))
            .append(",ino=")
            .append(st_ino)
            .append(')');
        return sb.toString();
    }
}
