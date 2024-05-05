package com.termux.display.controller.xserver.events;

import com.termux.display.controller.xserver.Bitmask;
import com.termux.display.controller.xserver.Window;

import java.io.IOException;

public class ConfigureRequest extends Event {
    private final Window parent;
    private final Window window;
    private final Window sibling;
    private final short x;
    private final short y;
    private final short width;
    private final short height;
    private final short borderWidth;
    private final Window.StackMode stackMode;
    private final Bitmask valueMask;

    public ConfigureRequest(Window parent, Window window, Window sibling, short x, short y, short width, short height, short borderWidth, Window.StackMode stackMode, Bitmask valueMask) {
        super(23);
        this.parent = parent;
        this.window = window;
        this.sibling = sibling;
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
        this.borderWidth = borderWidth;
        this.stackMode = stackMode != null ? stackMode : Window.StackMode.ABOVE;
        this.valueMask = valueMask;
    }

    @Override
    public void send(short sequenceNumber) throws IOException {

    }
}
