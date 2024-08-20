package com.termux.x11.controller.xserver.events;

import com.termux.x11.controller.xserver.Window;

import java.io.IOException;

public class MapNotify extends Event {
    private final Window event;
    private final Window window;

    public MapNotify(Window event, Window window) {
        super(19);
        this.event = event;
        this.window = window;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {
    }
}
