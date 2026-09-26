package com.termux.app.ssh;

/**
 * Data class representing a file or directory on a remote SSH server.
 *
 * Stores file metadata parsed from ls -la command output.
 * Used by RemoteFileLister to build directory listings.
 */
public class RemoteFile {

    /** File type enumeration */
    public enum FileType {
        DIRECTORY,
        FILE,
        SYMLINK,
        OTHER
    }

    /** File or directory name */
    private final String name;

    /** Full path on remote server */
    private final String path;

    /** File type (directory, file, symlink, etc.) */
    private final FileType type;

    /** File size in bytes (0 for directories) */
    private final long size;

    /** Modification time string (raw format from ls -la) */
    private final String modifyTime;

    /** Permission string (e.g., "rwxr-xr-x") */
    private final String permissions;

    /** File owner username */
    private final String owner;

    /** File group name */
    private final String group;

    /** Symlink target (null if not a symlink) */
    private final String symlinkTarget;

    /** Whether symlink target is a directory (only meaningful for symlinks) */
    private final boolean symlinkTargetIsDirectory;

    /**
     * Create a RemoteFile with all metadata.
     *
     * @param name File name
     * @param path Full path on remote server
     * @param type File type
     * @param size File size in bytes
     * @param modifyTime Modification time string
     * @param permissions Permission string
     * @param owner Owner username
     * @param group Group name
     * @param symlinkTarget Symlink target (null if not symlink)
     */
    public RemoteFile(String name, String path, FileType type, long size,
                      String modifyTime, String permissions, String owner,
                      String group, String symlinkTarget) {
        this(name, path, type, size, modifyTime, permissions, owner, group, symlinkTarget, false);
    }

    /**
     * Create a RemoteFile with all metadata including symlink target type.
     *
     * @param name File name
     * @param path Full path on remote server
     * @param type File type
     * @param size File size in bytes
     * @param modifyTime Modification time string
     * @param permissions Permission string
     * @param owner Owner username
     * @param group Group name
     * @param symlinkTarget Symlink target (null if not symlink)
     * @param symlinkTargetIsDirectory Whether symlink target is a directory
     */
    public RemoteFile(String name, String path, FileType type, long size,
                      String modifyTime, String permissions, String owner,
                      String group, String symlinkTarget, boolean symlinkTargetIsDirectory) {
        this.name = name;
        this.path = path;
        this.type = type;
        this.size = size;
        this.modifyTime = modifyTime;
        this.permissions = permissions;
        this.owner = owner;
        this.group = group;
        this.symlinkTarget = symlinkTarget;
        this.symlinkTargetIsDirectory = symlinkTargetIsDirectory;
    }

    /**
     * Get file name.
     *
     * @return File or directory name
     */
    public String getName() {
        return name;
    }

    /**
     * Get full path on remote server.
     *
     * @return Full path
     */
    public String getPath() {
        return path;
    }

    /**
     * Get file type.
     *
     * @return FileType enum value
     */
    public FileType getType() {
        return type;
    }

    /**
     * Check if this is a directory.
     *
     * @return true if directory
     */
    public boolean isDirectory() {
        return type == FileType.DIRECTORY;
    }

    /**
     * Check if this is a directory or a symlink pointing to a directory.
     *
     * Use this method when deciding whether to navigate into an item.
     *
     * @return true if directory or symlink-to-directory
     */
    public boolean isDirectoryOrSymlinkToDirectory() {
        return type == FileType.DIRECTORY ||
               (type == FileType.SYMLINK && symlinkTargetIsDirectory);
    }

    /**
     * Check if this is a regular file.
     *
     * @return true if file
     */
    public boolean isFile() {
        return type == FileType.FILE;
    }

    /**
     * Check if this is a symbolic link.
     *
     * @return true if symlink
     */
    public boolean isSymlink() {
        return type == FileType.SYMLINK;
    }

    /**
     * Get file size in bytes.
     *
     * @return Size in bytes (0 for directories)
     */
    public long getSize() {
        return size;
    }

    /**
     * Get human-readable size string (e.g., "1.5 KB", "2.3 MB").
     *
     * @return Formatted size string
     */
    public String getSizeFormatted() {
        if (size < 0) {
            return "-";
        }
        if (size < 1024) {
            return size + " B";
        }
        if (size < 1024 * 1024) {
            return String.format("%.1f KB", size / 1024.0);
        }
        if (size < 1024 * 1024 * 1024) {
            return String.format("%.1f MB", size / (1024.0 * 1024));
        }
        return String.format("%.1f GB", size / (1024.0 * 1024 * 1024));
    }

    /**
     * Get modification time string.
     *
     * @return Raw modification time from ls -la
     */
    public String getModifyTime() {
        return modifyTime;
    }

    /**
     * Get permission string.
     *
     * @return Permission string (e.g., "rwxr-xr-x")
     */
    public String getPermissions() {
        return permissions;
    }

    /**
     * Get owner username.
     *
     * @return Owner name
     */
    public String getOwner() {
        return owner;
    }

    /**
     * Get group name.
     *
     * @return Group name
     */
    public String getGroup() {
        return group;
    }

    /**
     * Get symlink target.
     *
     * @return Target path for symlinks, null otherwise
     */
    public String getSymlinkTarget() {
        return symlinkTarget;
    }

    /**
     * Check if symlink target is a directory.
     *
     * Only meaningful for symlinks. Returns true if this symlink points
     * to a directory, false otherwise.
     *
     * @return true if symlink target is a directory
     */
    public boolean isSymlinkTargetDirectory() {
        return symlinkTargetIsDirectory;
    }

    /**
     * Get file type from permission string first character.
     *
     * @param permissionChar First character of ls -la permission field
     * @return Corresponding FileType
     */
    public static FileType getTypeFromPermissionChar(char permissionChar) {
        switch (permissionChar) {
            case 'd':
                return FileType.DIRECTORY;
            case '-':
                return FileType.FILE;
            case 'l':
                return FileType.SYMLINK;
            default:
                return FileType.OTHER;
        }
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(type == FileType.DIRECTORY ? "d" : type == FileType.SYMLINK ? "l" : "-");
        sb.append(" ").append(name);
        if (isSymlink() && symlinkTarget != null) {
            sb.append(" -> ").append(symlinkTarget);
        }
        sb.append(" (").append(getSizeFormatted()).append(")");
        return sb.toString();
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) return true;
        if (obj == null || getClass() != obj.getClass()) return false;

        RemoteFile other = (RemoteFile) obj;
        return path.equals(other.path);
    }

    @Override
    public int hashCode() {
        return path.hashCode();
    }
}