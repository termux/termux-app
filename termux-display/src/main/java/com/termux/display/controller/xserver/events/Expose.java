package com.termux.display.controller.xserver.events;

import com.termux.display.controller.xserver.Window;

import java.io.IOException;

public class Expose extends Event {
    private final Window window;
    private final short width;
    private final short height;
    private final short x;
    private final short y;

    public Expose(Window window) {
        super(12);
        this.window = window;
        this.y = 0;
        this.x = 0;
        this.width = window.getWidth();
        this.height = window.getHeight();
    }

    @Override
    public void send(short sequenceNumber) throws IOException {

    }
}
