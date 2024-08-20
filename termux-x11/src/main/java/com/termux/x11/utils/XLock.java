package com.termux.x11.utils;

public interface XLock extends AutoCloseable {
    @Override
    void close();
}
