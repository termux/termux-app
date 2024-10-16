package com.termux.x11.controller.xserver;

public class ScreenInfo {
    public final short width;
    public final short height;

    public ScreenInfo(int width, int height) {
        this.width = (short)width;
        this.height = (short)height;
    }
}
