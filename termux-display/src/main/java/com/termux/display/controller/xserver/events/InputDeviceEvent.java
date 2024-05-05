package com.termux.display.controller.xserver.events;

import com.termux.display.controller.xserver.Bitmask;
import com.termux.display.controller.xserver.Window;

import java.io.IOException;

public class InputDeviceEvent extends Event {
    private final byte detail;
    private final int timestamp;
    private final Window root;
    private final Window event;
    private final Window child;
    private final short eventX;
    private final short eventY;
    private final short rootX;
    private final short rootY;
    private final Bitmask state;

    public InputDeviceEvent(int code, byte detail, Window root, Window event, Window child, short rootX, short rootY, short eventX, short eventY, Bitmask state) {
        super(code);
        this.detail = detail;
        this.timestamp = (int)System.currentTimeMillis();
        this.root = root;
        this.event = event;
        this.child = child;
        this.rootX = rootX;
        this.rootY = rootY;
        this.eventX = eventX;
        this.eventY = eventY;
        this.state = state;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {
    }
}
