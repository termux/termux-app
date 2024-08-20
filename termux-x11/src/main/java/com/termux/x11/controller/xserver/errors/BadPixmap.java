package com.termux.x11.controller.xserver.errors;

public class BadPixmap extends XRequestError {
    public BadPixmap(int id) {
        super(4, id);
    }
}
