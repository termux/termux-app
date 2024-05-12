package com.termux.display.utils;

public interface XLock extends AutoCloseable {
    @Override
    void close();
}
