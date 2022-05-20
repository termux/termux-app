package com.termux.terminal;

public interface ResizeBuffer {
    void resize(int newColumns, int newRows, int newTotalRows, int[] cursor, long currentStyle, boolean isAltScreen) ;
}
