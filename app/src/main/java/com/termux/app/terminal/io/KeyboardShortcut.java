package com.termux.app.terminal.io;

public class KeyboardShortcut {

    public final int codePoint;
    public final int shortcutAction;

    public KeyboardShortcut(int codePoint, int shortcutAction) {
        this.codePoint = codePoint;
        this.shortcutAction = shortcutAction;
    }

}
