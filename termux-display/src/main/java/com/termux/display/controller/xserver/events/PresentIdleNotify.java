package com.termux.display.controller.xserver.events;

import com.termux.display.controller.xserver.Pixmap;
import com.termux.display.controller.xserver.Window;

import java.io.IOException;

public class PresentIdleNotify extends Event {
    private final int eventId;
    private final Window window;
    private final Pixmap pixmap;
    private final int serial;
    private final int idleFence;

    public PresentIdleNotify(int eventId, Window window, Pixmap pixmap, int serial, int idleFence) {
        super(35);
        this.eventId = eventId;
        this.window = window;
        this.serial = serial;
        this.pixmap = pixmap;
        this.idleFence = idleFence;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {

    }

    public static short getEventType() {
        return 2;
    }

    public static int getEventMask() {
        return 1<<getEventType();
    }
}
