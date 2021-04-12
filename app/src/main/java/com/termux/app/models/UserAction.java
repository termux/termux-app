package com.termux.app.models;

public enum UserAction {

    PLUGIN_EXECUTION_COMMAND("plugin execution command"),
    CRASH_REPORT("crash report"),
    REPORT_ISSUE_FROM_TRANSCRIPT("report issue from transcript");

    private final String name;

    UserAction(final String name) {
        this.name = name;
    }

    public String getName() {
        return name;
    }

}
