package com.termux.models;

public enum UserAction {

    PLUGIN_EXECUTION_COMMAND("plugin execution command");

    private final String name;

    UserAction(final String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

}
