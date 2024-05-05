package com.termux.display.controller.xserver.events;

import com.termux.display.controller.xserver.Window;

import java.io.IOException;

public class DestroyNotify extends Event {
    private final Window event;
    private final Window window;

    public DestroyNotify(Window event, Window window) {
        super(17);
        this.event = event;
        this.window = window;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {

    }
}
