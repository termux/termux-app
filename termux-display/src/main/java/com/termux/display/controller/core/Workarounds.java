package com.termux.display.controller.core;

import com.termux.display.controller.winhandler.WinHandler;
import com.termux.display.controller.xserver.Window;

import java.util.Timer;
import java.util.TimerTask;

public abstract class Workarounds {
    // Workaround for applications that don't work mouse/keyboard
    public static void onMapWindow(final WinHandler winHandler, Window window) {
        final String className = window.getClassName();
        if (className.equals("twfc.exe") ||
            className.equals("daggerfallunity.exe")) {
            final boolean reverse = className.equals("twfc.exe");
            (new Timer()).schedule(new TimerTask() {
                @Override
                public void run() {
                    winHandler.bringToFront(className, reverse);
                }
            }, 500);
        }
    }
}
