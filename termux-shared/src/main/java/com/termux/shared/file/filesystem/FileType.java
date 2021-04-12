package com.termux.shared.file.filesystem;

/** The {@link Enum} that defines file types. */
public enum FileType {

    NO_EXIST("no exist", 0),    // 0000000
    REGULAR("regular", 1),      // 0000001
    DIRECTORY("directory", 2),  // 0000010
    SYMLINK("symlink", 4),      // 0000100
    CHARACTER("character", 8),  // 0001000
    FIFO("fifo", 16),           // 0010000
    BLOCK("block", 32),         // 0100000
    UNKNOWN("unknown", 64);     // 1000000

    private final String name;
    private final int value;

    FileType(final String name, final int value) {
        this.name = name;
        this.value = value;
    }

    public String getName() {
        return name;
    }

    public int getValue() {
        return value;
    }

}
