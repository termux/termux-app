package com.termux.display;

public class ScreenInfo {
    public short width;
    public short height;

    public ScreenInfo(String value) {
        String[] parts = value.split("x");
        width = Short.parseShort(parts[0]);
        height = Short.parseShort(parts[1]);
    }

    public ScreenInfo(int width, int height) {
        this.width = (short)width;
        this.height = (short)height;
    }

    public short getWidthInMillimeters() {
        return (short)(width / 10);
    }

    public short getHeightInMillimeters() {
        return (short)(height / 10);
    }

    @Override
    public String toString() {
        return width+"x"+height;
    }
}
