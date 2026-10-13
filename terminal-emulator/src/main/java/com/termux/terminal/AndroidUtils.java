package com.termux.terminal;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class AndroidUtils {

    /**
     * Get system properties as {@link Properties} from `/system/bin/getprop` command output
     * that reads the `/system/build.prop` file.
     *
     * Sourced from https://github.com/termux/termux-app/blob/30ebb2dee381d292ade0f2868cfde0f9f20b89fe/termux-shared/src/main/java/com/termux/shared/android/AndroidUtils.java#L170.
     */
    public static Properties getSystemProperties(String logTag) {
        Properties systemProperties = new Properties();

        // getprop commands returns values in the format `[key]: [value]`
        // Regex matches string starting with a literal `[`,
        // followed by one or more characters that do not match a closing square bracket as the key,
        // followed by a literal `]: [`,
        // followed by one or more characters as the value,
        // followed by string ending with literal `]`
        // multiline values will be ignored
        Pattern propertiesPattern = Pattern.compile("^\\[([^]]+)]: \\[(.+)]$");

        try {
            Process process = new ProcessBuilder()
                .command("/system/bin/getprop")
                .redirectErrorStream(true)
                .start();

            InputStream inputStream = process.getInputStream();
            BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(inputStream));
            String line, key, value;

            while ((line = bufferedReader.readLine()) != null) {
                Matcher matcher = propertiesPattern.matcher(line);
                if (matcher.matches()) {
                    key = matcher.group(1);
                    value = matcher.group(2);
                    if (key != null && value != null && !key.isEmpty() && !value.isEmpty())
                        systemProperties.put(key, value);
                }
            }

            bufferedReader.close();
            process.destroy();

        } catch (IOException e) {
            Logger.logStackTraceWithMessage(null, logTag, "Failed to get run \"/system/bin/getprop\" to get system properties.", e);
        }

        //for (String key : systemProperties.stringPropertyNames()) {
        //    Logger.logVerbose(null, logTag, key + ": " +  systemProperties.get(key));
        //}

        return systemProperties;
    }

}
