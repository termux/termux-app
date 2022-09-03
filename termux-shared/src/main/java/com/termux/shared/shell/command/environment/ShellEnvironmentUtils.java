package com.termux.shared.shell.command.environment;

import static com.termux.shared.shell.command.environment.UnixShellEnvironment.*;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.termux.shared.errors.Error;
import com.termux.shared.file.FileUtils;
import com.termux.shared.logger.Logger;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class ShellEnvironmentUtils {

    private static final String LOG_TAG = "ShellEnvironmentUtils";

    /**
     * Convert environment {@link HashMap} to `environ` {@link List <String>}.
     *
     * The items in the environ will have the format `name=value`.
     *
     * Check {@link #isValidEnvironmentVariableName(String)} and {@link #isValidEnvironmentVariableValue(String)}
     * for valid variable names and values.
     *
     * https://manpages.debian.org/testing/manpages/environ.7.en.html
     * https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap08.html
     */
    @NonNull
    public static List<String> convertEnvironmentToEnviron(@NonNull HashMap<String, String> environmentMap) {
        List<String> environmentList = new ArrayList<>(environmentMap.size());
        String value;
        for (String name : environmentMap.keySet()) {
            value = environmentMap.get(name);
            if (isValidEnvironmentVariableNameValuePair(name, value, true))
                environmentList.add(name + "=" + environmentMap.get(name));
        }
        return environmentList;
    }

    /**
     * Convert environment {@link HashMap} to {@link String} where each item equals "key=value".
     *
     */
    @NonNull
    public static String convertEnvironmentToDotEnvFile(@NonNull HashMap<String, String> environmentMap) {
        return convertEnvironmentToDotEnvFile(convertEnvironmentMapToEnvironmentVariableList(environmentMap));
    }

    /**
     * Convert environment {@link HashMap} to `.env` file {@link String}.
     *
     * The items in the `.env` file have the format `export name="value"`.
     *
     * If the {@link ShellEnvironmentVariable#escaped} is set to {@code true}, then
     * {@link ShellEnvironmentVariable#value} will be considered to be a literal value that has
     * already been escaped by the caller, otherwise all the `"`\$` in the value will be escaped
     * with `a backslash `\`, like `\"`. Note that if `$` is escaped and if its part of variable,
     * then variable expansion will not happen if `.env` file is sourced.
     *
     * The `\` at the end of a value line means line continuation. Value can contain newline characters.
     *
     * Check {@link #isValidEnvironmentVariableName(String)} and {@link #isValidEnvironmentVariableValue(String)}
     * for valid variable names and values.
     *
     * https://github.com/ko1nksm/shdotenv#env-file-syntax
     * https://github.com/ko1nksm/shdotenv/blob/main/docs/specification.md
     */
    @NonNull
    public static String convertEnvironmentToDotEnvFile(@NonNull List<ShellEnvironmentVariable> environmentList) {
        StringBuilder environment = new StringBuilder();
        Collections.sort(environmentList);
        for (ShellEnvironmentVariable variable : environmentList) {
            if (isValidEnvironmentVariableNameValuePair(variable.name, variable.value, true) && variable.value != null) {
                environment.append("export ").append(variable.name).append("=\"")
                    .append(variable.escaped ? variable.value : variable.value.replaceAll("([\"`\\\\$])", "\\\\$1"))
                    .append("\"\n");
            }
        }
        return environment.toString();
    }

    /**
     * Convert environment {@link HashMap} to {@link List< ShellEnvironmentVariable >}. Each item
     * will have its {@link ShellEnvironmentVariable#escaped} set to {@code false}.
     */
    @NonNull
    public static List<ShellEnvironmentVariable> convertEnvironmentMapToEnvironmentVariableList(@NonNull HashMap<String, String> environmentMap) {
        List<ShellEnvironmentVariable> environmentList = new ArrayList<>();
        for (String name :environmentMap.keySet()) {
            environmentList.add(new ShellEnvironmentVariable(name, environmentMap.get(name), false));
        }
        return environmentList;
    }

    /**
     * Check if environment variable name and value pair is valid. Errors will be logged if
     * {@code logErrors} is {@code true}.
     *
     * Check {@link #isValidEnvironmentVariableName(String)} and {@link #isValidEnvironmentVariableValue(String)}
     * for valid variable names and values.
     */
    public static boolean isValidEnvironmentVariableNameValuePair(@Nullable String name, @Nullable String value, boolean logErrors) {
        if (!isValidEnvironmentVariableName(name)) {
            if (logErrors)
                Logger.logErrorPrivate(LOG_TAG, "Invalid environment variable name. name=`" + name + "`, value=`" + value + "`");
            return false;
        }

        if (!isValidEnvironmentVariableValue(value)) {
            if (logErrors)
                Logger.logErrorPrivate(LOG_TAG, "Invalid environment variable value. name=`" + name + "`, value=`" + value + "`");
            return false;
        }

        return true;
    }

    /**
     * Check if environment variable name is valid. It must not be {@code null} and must not contain
     * the null byte ('\0') and must only contain alphanumeric and underscore characters and must not
     * start with a digit.
     */
    public static boolean isValidEnvironmentVariableName(@Nullable String name) {
        return name != null && !name.contains("\0") && name.matches("[a-zA-Z_][a-zA-Z0-9_]*");
    }

    /**
     * Check if environment variable value is valid. It must not be {@code null} and must not contain
     * the null byte ('\0').
     */
    public static boolean isValidEnvironmentVariableValue(@Nullable String value) {
        return value != null && !value.contains("\0");
    }



    /** Put value in environment if variable exists in {@link System) environment. */
    public static void putToEnvIfInSystemEnv(@NonNull HashMap<String, String> environment,
                                             @NonNull String name) {
        String value = System.getenv(name);
        if (value != null) {
            environment.put(name, value);
        }
    }

    /** Put {@link String} value in environment if value set. */
    public static void putToEnvIfSet(@NonNull HashMap<String, String> environment, @NonNull String name,
                                     @Nullable String value) {
        if (value != null) {
            environment.put(name, value);
        }
    }

    /** Put {@link Boolean} value "true" or "false" in environment if value set. */
    public static void putToEnvIfSet(@NonNull HashMap<String, String> environment, @NonNull String name,
                                     @Nullable Boolean value) {
        if (value != null) {
            environment.put(name, String.valueOf(value));
        }
    }



    /** Create HOME directory in environment {@link Map} if set. */
    public static void createHomeDir(@NonNull HashMap<String, String> environment) {
        String homeDirectory = environment.get(ENV_HOME);
        if (homeDirectory != null && !homeDirectory.isEmpty()) {
            Error error = FileUtils.createDirectoryFile("shell home", homeDirectory);
            if (error != null) {
                Logger.logErrorExtended(LOG_TAG, "Failed to create shell home directory\n" + error.toString());
            }
        }
    }

}
