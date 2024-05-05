package com.termux.display.controller.xserver;

public interface XLock extends AutoCloseable {
    @Override
    void close();
}
