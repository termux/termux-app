package com.termux.app.models;

public enum UserAction {

    PLUGIN_EXECUTION_COMMAND("plugin execution command"),
    CRASH_REPORT("crash report");

    private final String name;

    UserAction(final String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

}
