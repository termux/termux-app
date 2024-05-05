package com.termux.display.controller.xserver.events;

import java.io.IOException;

public abstract class Event {
    public static final int KEY_PRESS = 1<<0;
    public static final int KEY_RELEASE = 1<<1;
    public static final int BUTTON_PRESS = 1<<2;
    public static final int BUTTON_RELEASE = 1<<3;
    public static final int ENTER_WINDOW = 1<<4;
    public static final int LEAVE_WINDOW = 1<<5;
    public static final int POINTER_MOTION = 1<<6;
    public static final int POINTER_MOTION_HINT = 1<<7;
    public static final int BUTTON1_MOTION = 1<<8;
    public static final int BUTTON2_MOTION = 1<<9;
    public static final int BUTTON3_MOTION = 1<<10;
    public static final int BUTTON4_MOTION = 1<<11;
    public static final int BUTTON5_MOTION = 1<<12;
    public static final int BUTTON_MOTION = 1<<13;
    public static final int KEYMAP_STATE = 1<<14;
    public static final int EXPOSURE = 1<<15;
    public static final int VISIBILITY_CHANGE = 1<<16;
    public static final int STRUCTURE_NOTIFY = 1<<17;
    public static final int RESIZE_REDIRECT = 1<<18;
    public static final int SUBSTRUCTURE_NOTIFY = 1<<19;
    public static final int SUBSTRUCTURE_REDIRECT = 1<<20;
    public static final int FOCUS_CHANGE = 1<<21;
    public static final int PROPERTY_CHANGE = 1<<22;
    public static final int COLORMAP_CHANGE = 1<<23;
    public static final int OWNER_GRAB_BUTTON = 1<<24;
    protected final byte code;

    public Event(int code) {
        this.code = (byte)code;
    }

    public abstract void send(short sequenceNumber) throws IOException;
}
