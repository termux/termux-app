package com.termux.display.controller.xserver;

public class Visual {
    public final int id;
    public final byte visualClass = 4;
    public final boolean displayable;
    public final byte depth;
    public final byte bitsPerRGBValue;
    public final short colormapEntries = 256;
    public final int blueMask;
    public final int greenMask;
    public final int redMask;

    public Visual(int id, boolean displayable, int depth, int bitsPerRGBValue, int redMask, int greenMask, int blueMask) {
        this.id = id;
        this.displayable = displayable;
        this.depth = (byte)depth;
        this.bitsPerRGBValue = (byte)bitsPerRGBValue;
        this.redMask = redMask;
        this.greenMask = greenMask;
        this.blueMask = blueMask;
    }
}
