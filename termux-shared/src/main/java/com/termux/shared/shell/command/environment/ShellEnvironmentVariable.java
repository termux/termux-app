package com.termux.shared.shell.command.environment;

public class ShellEnvironmentVariable implements Comparable<ShellEnvironmentVariable> {

    /** The name for environment variable */
    public String name;

    /** The value for environment variable */
    public String value;

    /** If environment variable {@link #value} is already escaped. */
    public boolean escaped;

    public ShellEnvironmentVariable(String name, String value) {
        this(name, value, false);
    }

    public ShellEnvironmentVariable(String name, String value, boolean escaped) {
        this.name = name;
        this.value = value;
        this.escaped = escaped;
    }

    @Override
    public int compareTo(ShellEnvironmentVariable other) {
        return this.name.compareTo(other.name);
    }
}
