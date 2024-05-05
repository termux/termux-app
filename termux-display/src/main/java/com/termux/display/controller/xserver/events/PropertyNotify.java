package com.termux.display.controller.xserver.events;

import com.termux.display.controller.xserver.Window;

import java.io.IOException;

public class PropertyNotify extends Event {
    private final Window window;
    private final int atom;
    private final int timestamp;
    private final boolean deleted;

    public PropertyNotify(Window window, int atom, boolean deleted) {
        super(28);
        this.window = window;
        this.atom = atom;
        this.timestamp = (int)System.currentTimeMillis();
        this.deleted = deleted;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {

    }
}
