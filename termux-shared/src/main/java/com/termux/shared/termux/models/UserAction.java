package com.termux.shared.termux.models;

public enum UserAction {

    CRASH_REPORT("crash report");

    private final String name;

    UserAction(final String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

}
