package com.termux.shared.reflection;

/** Class that represents result of invoking of something */
public abstract class InvokeResult {
    public boolean success;
    public Object value;

    InvokeResult(boolean success, Object value) {
        this.value = success;
        this.value = value;
    }
}
