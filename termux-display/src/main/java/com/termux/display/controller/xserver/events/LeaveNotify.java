package com.termux.display.controller.xserver.events;

import com.termux.display.controller.xserver.Bitmask;
import com.termux.display.controller.xserver.Window;

public class LeaveNotify extends PointerWindowEvent {
    public LeaveNotify(Detail detail, Window root, Window event, Window child, short rootX, short rootY, short eventX, short eventY, Bitmask state, Mode mode, boolean sameScreenAndFocus) {
        super(8, detail, root, event, child, rootX, rootY, eventX, eventY, state, mode, sameScreenAndFocus);
    }
}
