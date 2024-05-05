package com.termux.display.controller.xserver.events;

import com.termux.display.controller.xserver.Window;

import java.io.IOException;

public class ResizeRequest extends Event {
    private final Window window;
    private final short width;
    private final short height;

    public ResizeRequest(Window window, short width, short height) {
        super(25);
        this.window = window;
        this.width = width;
        this.height = height;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {

    }
}
