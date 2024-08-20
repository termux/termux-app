package com.termux.x11.controller.xserver.events;

import com.termux.x11.controller.xserver.Window;

import java.io.IOException;

public class MapRequest extends Event {
    private final Window parent;
    private final Window window;

    public MapRequest(Window parent, Window window) {
        super(20);
        this.parent = parent;
        this.window = window;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {
    }
}
