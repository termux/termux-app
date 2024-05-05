package com.termux.display.controller.xserver;

public class PixmapFormat {
    public final byte depth;
    public final byte bitsPerPixel;
    public final byte scanlinePad;

    public PixmapFormat(int depth, int bitsPerPixel, int scanlinePad) {
        this.depth = (byte)depth;
        this.bitsPerPixel = (byte)bitsPerPixel;
        this.scanlinePad = (byte)scanlinePad;
    }
}
