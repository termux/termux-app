package com.termux.display.controller.xserver.events;

import com.termux.display.controller.xserver.Window;

import java.io.IOException;

public class CreateNotify extends Event {
    private final Window parent;
    private final Window window;

    public CreateNotify(Window parent, Window window) {
        super(16);
        this.parent = parent;
        this.window = window;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {
    }
}
