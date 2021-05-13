package com.termux.shared.data;

import android.os.Bundle;

import java.util.LinkedHashSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class DataUtils {

    public static final int TRANSACTION_SIZE_LIMIT_IN_BYTES = 100 * 1024; // 100KB

    private static final char[] HEX_ARRAY = "0123456789ABCDEF".toCharArray();

    public static String getTruncatedCommandOutput(String text, int maxLength, boolean fromEnd, boolean onNewline, boolean addPrefix) {
        if (text == null) return null;

        String prefix = "(truncated) ";

        if (addPrefix)
            maxLength = maxLength - prefix.length();

        if (maxLength < 0 || text.length() < maxLength) return text;

        if (fromEnd) {
            text = text.substring(0, Math.min(text.length(), maxLength));
        } else {
            int cutOffIndex = text.length() - maxLength;

            if (onNewline) {
                int nextNewlineIndex = text.indexOf('\n', cutOffIndex);
                if (nextNewlineIndex != -1 && nextNewlineIndex != text.length() - 1) {
                    cutOffIndex = nextNewlineIndex + 1;
                }
            }
            text = text.substring(cutOffIndex);
        }

        if (addPrefix)
            text = prefix + text;

        return text;
    }

    /**
     * Get the {@code float} from a {@link String}.
     *
     * @param value The {@link String} value.
     * @param def The default value if failed to read a valid value.
     * @return Returns the {@code float} value after parsing the {@link String} value, otherwise
     * returns default if failed to read a valid value, like in case of an exception.
     */
    public static float getFloatFromString(String value, float def) {
        if (value == null) return def;

        try {
            return Float.parseFloat(value);
        }
        catch (Exception e) {
            return def;
        }
    }

    /**
     * Get the {@code int} from a {@link String}.
     *
     * @param value The {@link String} value.
     * @param def The default value if failed to read a valid value.
     * @return Returns the {@code int} value after parsing the {@link String} value, otherwise
     * returns default if failed to read a valid value, like in case of an exception.
     */
    public static int getIntFromString(String value, int def) {
        if (value == null) return def;

        try {
            return Integer.parseInt(value);
        }
        catch (Exception e) {
            return def;
        }
    }

    /**
     * Get the {@code hex string} from a {@link byte[]}.
     *
     * @param bytes The {@link byte[]} value.
     * @return Returns the {@code hex string} value.
     */
    public static String bytesToHex(byte[] bytes) {
        char[] hexChars = new char[bytes.length * 2];
        for (int j = 0; j < bytes.length; j++) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = HEX_ARRAY[v >>> 4];
            hexChars[j * 2 + 1] = HEX_ARRAY[v & 0x0F];
        }
        return new String(hexChars);
    }

    /**
     * Get an {@code int} from {@link Bundle} that is stored as a {@link String}.
     *
     * @param bundle The {@link Bundle} to get the value from.
     * @param key The key for the value.
     * @param def The default value if failed to read a valid value.
     * @return Returns the {@code int} value after parsing the {@link String} value stored in
     * {@link Bundle}, otherwise returns default if failed to read a valid value,
     * like in case of an exception.
     */
    public static int getIntStoredAsStringFromBundle(Bundle bundle, String key, int def) {
        if (bundle == null) return def;
        return getIntFromString(bundle.getString(key, Integer.toString(def)), def);
    }



    /**
     * If value is not in the range [min, max], set it to either min or max.
     */
    public static int clamp(int value, int min, int max) {
        return Math.min(Math.max(value, min), max);
    }

    /**
     * If value is not in the range [min, max], set it to default.
     */
    public static float rangedOrDefault(float value, float def, float min, float max) {
        if (value < min || value > max)
            return def;
        else
            return value;
    }



    /**
     * Get the object itself if it is not {@code null}, otherwise default.
     *
     * @param object The {@link Object} to check.
     * @param def The default {@link Object}.
     * @return Returns {@code object} if it is not {@code null}, otherwise returns {@code def}.
     */
    public static <T> T getDefaultIfNull(@androidx.annotation.Nullable T object, @androidx.annotation.Nullable T def) {
        return (object == null) ? def : object;
    }



    public static LinkedHashSet<CharSequence> extractUrls(String text) {

        StringBuilder regex_sb = new StringBuilder();

        regex_sb.append("(");                       // Begin first matching group.
        regex_sb.append("(?:");                     // Begin scheme group.
        regex_sb.append("dav|");                    // The DAV proto.
        regex_sb.append("dict|");                   // The DICT proto.
        regex_sb.append("dns|");                    // The DNS proto.
        regex_sb.append("file|");                   // File path.
        regex_sb.append("finger|");                 // The Finger proto.
        regex_sb.append("ftp(?:s?)|");              // The FTP proto.
        regex_sb.append("git|");                    // The Git proto.
        regex_sb.append("gopher|");                 // The Gopher proto.
        regex_sb.append("http(?:s?)|");             // The HTTP proto.
        regex_sb.append("imap(?:s?)|");             // The IMAP proto.
        regex_sb.append("irc(?:[6s]?)|");           // The IRC proto.
        regex_sb.append("ip[fn]s|");                // The IPFS proto.
        regex_sb.append("ldap(?:s?)|");             // The LDAP proto.
        regex_sb.append("pop3(?:s?)|");             // The POP3 proto.
        regex_sb.append("redis(?:s?)|");            // The Redis proto.
        regex_sb.append("rsync|");                  // The Rsync proto.
        regex_sb.append("rtsp(?:[su]?)|");          // The RTSP proto.
        regex_sb.append("sftp|");                   // The SFTP proto.
        regex_sb.append("smb(?:s?)|");              // The SAMBA proto.
        regex_sb.append("smtp(?:s?)|");             // The SMTP proto.
        regex_sb.append("svn(?:(?:\\+ssh)?)|");     // The Subversion proto.
        regex_sb.append("tcp|");                    // The TCP proto.
        regex_sb.append("telnet|");                 // The Telnet proto.
        regex_sb.append("tftp|");                   // The TFTP proto.
        regex_sb.append("udp|");                    // The UDP proto.
        regex_sb.append("vnc|");                    // The VNC proto.
        regex_sb.append("ws(?:s?)");                // The Websocket proto.
        regex_sb.append(")://");                    // End scheme group.
        regex_sb.append(")");                       // End first matching group.


        // Begin second matching group.
        regex_sb.append("(");

        // User name and/or password in format 'user:pass@'.
        regex_sb.append("(?:\\S+(?::\\S*)?@)?");

        // Begin host group.
        regex_sb.append("(?:");

        // IP address (from http://www.regular-expressions.info/examples.html).
        regex_sb.append("(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)|");

        // Host name or domain.
        regex_sb.append("(?:(?:[a-z\\u00a1-\\uffff0-9]-*)*[a-z\\u00a1-\\uffff0-9]+)(?:(?:\\.(?:[a-z\\u00a1-\\uffff0-9]-*)*[a-z\\u00a1-\\uffff0-9]+)*(?:\\.(?:[a-z\\u00a1-\\uffff]{2,})))?|");

        // Just path. Used in case of 'file://' scheme.
        regex_sb.append("/(?:(?:[a-z\\u00a1-\\uffff0-9]-*)*[a-z\\u00a1-\\uffff0-9]+)");

        // End host group.
        regex_sb.append(")");

        // Port number.
        regex_sb.append("(?::\\d{1,5})?");

        // Resource path with optional query string.
        regex_sb.append("(?:/[a-zA-Z0-9:@%\\-._~!$&()*+,;=?/]*)?");

        // Fragment.
        regex_sb.append("(?:#[a-zA-Z0-9:@%\\-._~!$&()*+,;=?/]*)?");

        // End second matching group.
        regex_sb.append(")");

        final Pattern urlPattern = Pattern.compile(
            regex_sb.toString(),
            Pattern.CASE_INSENSITIVE | Pattern.MULTILINE | Pattern.DOTALL);

        LinkedHashSet<CharSequence> urlSet = new LinkedHashSet<>();
        Matcher matcher = urlPattern.matcher(text);

        while (matcher.find()) {
            int matchStart = matcher.start(1);
            int matchEnd = matcher.end();
            String url = text.substring(matchStart, matchEnd);
            urlSet.add(url);
        }

        return urlSet;
    }

}
