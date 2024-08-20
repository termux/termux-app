package com.termux.x11.controller.xserver.errors;

public class BadGraphicsContext extends XRequestError {
    public BadGraphicsContext(int id) {
        super(13, id);
    }
}
