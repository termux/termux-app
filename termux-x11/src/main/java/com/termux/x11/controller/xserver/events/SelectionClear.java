package com.termux.x11.controller.xserver.events;

import com.termux.x11.controller.xserver.Window;

import java.io.IOException;

public class SelectionClear extends Event {
    private final int timestamp;
    private final Window owner;
    private final int selection;

    public SelectionClear(int timestamp, Window owner, int selection) {
        super(29);
        this.timestamp = timestamp;
        this.owner = owner;
        this.selection = selection;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {
    }
}
