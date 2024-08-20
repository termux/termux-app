package com.termux.x11.controller.xserver.events;

import com.termux.x11.controller.xserver.Window;

import java.io.IOException;

public class UnmapNotify extends Event {
    private final Window event;
    private final Window window;

    public UnmapNotify(Window event, Window window) {
        super(18);
        this.event = event;
        this.window = window;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {
    }
}
