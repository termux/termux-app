package com.termux.display.controller.xserver.events;

import com.termux.display.controller.xserver.Window;

import java.io.IOException;

public class ConfigureNotify extends Event {
    private final Window event;
    private final Window window;
    private final Window aboveSibling;
    private final short x;
    private final short y;
    private final short height;
    private final short width;
    private final short borderWidth;
    private final boolean overrideRedirect;

    public ConfigureNotify(Window event, Window window, Window aboveSibling, int x, int y, int width, int height, int borderWidth, boolean overrideRedirect) {
        super(22);
        this.event = event;
        this.window = window;
        this.aboveSibling = aboveSibling;
        this.x = (short)x;
        this.y = (short)y;
        this.width = (short)width;
        this.height = (short)height;
        this.borderWidth = (short)borderWidth;
        this.overrideRedirect = overrideRedirect;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {

    }
}
