package com.termux.display.controller.xserver.errors;

public class BadAtom extends XRequestError {
    public BadAtom(int id) {
        super(5, id);
    }
}
