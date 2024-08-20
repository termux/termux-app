package com.termux.x11.controller.winhandler;


import com.termux.x11.controller.xserver.Pointer;

public abstract class MouseEventFlags {
    public static final int MOVE = 0x0001;
    public static final int LEFTDOWN = 0x0002;
    public static final int LEFTUP = 0x0004;
    public static final int RIGHTDOWN = 0x0008;
    public static final int RIGHTUP = 0x0010;
    public static final int MIDDLEDOWN = 0x0020;
    public static final int MIDDLEUP = 0x0040;
    public static final int XDOWN = 0x0080;
    public static final int XUP = 0x0100;
    public static final int WHEEL = 0x0800;
    public static final int VIRTUALDESK = 0x4000;
    public static final int ABSOLUTE = 0x8000;

    public static int getFlagFor(Pointer.Button button, boolean isActionDown) {
        switch (button) {
            case BUTTON_LEFT:
                return isActionDown ? MouseEventFlags.LEFTDOWN : MouseEventFlags.LEFTUP;
            case BUTTON_MIDDLE:
                return isActionDown ? MouseEventFlags.MIDDLEDOWN : MouseEventFlags.MIDDLEUP;
            case BUTTON_RIGHT:
                return isActionDown ? MouseEventFlags.RIGHTDOWN : MouseEventFlags.RIGHTUP;
            case BUTTON_SCROLL_DOWN:
            case BUTTON_SCROLL_UP:
                return MouseEventFlags.WHEEL;
            default:
                return 0;
        }
    }
}
