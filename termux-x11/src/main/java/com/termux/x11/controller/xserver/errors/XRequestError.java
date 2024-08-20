package com.termux.x11.controller.xserver.errors;

public class XRequestError extends Exception  {
    private final byte code;
    private final int data;

    public XRequestError(int code, int data) {
        this.code = (byte)code;
        this.data = data;
    }

    public byte getCode() {
        return code;
    }

    public int getData() {
        return data;
    }

}
