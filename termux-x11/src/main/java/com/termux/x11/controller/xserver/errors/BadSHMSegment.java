package com.termux.x11.controller.xserver.errors;

public class BadSHMSegment extends XRequestError {
    public BadSHMSegment(int id) {
        super(-128, id);
    }
}
