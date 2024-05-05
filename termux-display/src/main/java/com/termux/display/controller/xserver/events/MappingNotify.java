package com.termux.display.controller.xserver.events;

import java.io.IOException;

public class MappingNotify extends Event {
    public enum Request {MODIFIER, KEYBOARD, POINTER}
    private final Request request;
    private final byte firstKeycode;
    private final byte count;

    public MappingNotify(Request request, byte firstKeycode, int count) {
        super(34);
        this.request = request;
        this.firstKeycode = firstKeycode;
        this.count = (byte)count;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {
    }
}
