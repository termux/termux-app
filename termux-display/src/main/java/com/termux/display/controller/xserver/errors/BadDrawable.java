package com.termux.display.controller.xserver.errors;

public class BadDrawable extends XRequestError {
    public BadDrawable(int id) {
        super(9, id);
    }
}
