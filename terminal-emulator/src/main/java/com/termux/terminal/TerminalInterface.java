package com.termux.terminal;

public interface TerminalInterface {

    void initializeEmulator(int columns, int rows);

    void updateSize(int columns, int rows);

    String getTitle();

    boolean isRunning();

    void write(byte[] data, int offset, int count);

    void writeCodePoint(boolean prependEscape, int codePoint);

    TerminalEmulator getEmulator();

    void reset();

    void finishIfRunning();

}



