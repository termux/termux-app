package com.termux.x11.controller.xserver.errors;

public class BadAccess extends XRequestError {
    public BadAccess() {
        super(10, 0);
    }
}
