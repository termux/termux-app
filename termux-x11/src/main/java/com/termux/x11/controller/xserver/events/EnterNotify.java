package com.termux.x11.controller.xserver.events;

import com.termux.x11.controller.xserver.Bitmask;
import com.termux.x11.controller.xserver.Window;

public class EnterNotify extends PointerWindowEvent {
    public EnterNotify(Detail detail, Window root, Window event, Window child, short rootX, short rootY, short eventX, short eventY, Bitmask state, Mode mode, boolean sameScreenAndFocus) {
        super(7, detail, root, event, child, rootX, rootY, eventX, eventY, state, mode, sameScreenAndFocus);
    }
}
