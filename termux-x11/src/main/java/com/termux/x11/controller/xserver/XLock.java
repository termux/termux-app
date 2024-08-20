package com.termux.x11.controller.xserver;

public interface XLock extends AutoCloseable {
    @Override
    void close();
}
