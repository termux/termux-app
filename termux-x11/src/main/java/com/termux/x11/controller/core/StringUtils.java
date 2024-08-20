package com.termux.x11.controller.core;

import android.content.Context;

import java.nio.charset.Charset;
import java.util.Locale;

public class StringUtils {
    public static String removeEndSlash(String value) {
        while (value.endsWith("/") || value.endsWith("\\")) value = value.substring(0, value.length()-1);
        return value;
    }

    public static String addEndSlash(String value) {
        return value.endsWith("/") ? value : value+"/";
    }

    public static String insert(String text, int index, String value) {
        return text.substring(0, index) + value + text.substring(index);
    }

    public static String replace(String text, int start, int end, String value) {
        return text.substring(0, start) + value + text.substring(end);
    }

    public static String unescape(String path) {
        return path.replaceAll("\\\\([^\\\\]+)", "$1").replaceAll("\\\\([^\\\\]+)", "$1").replaceAll("\\\\\\\\", "\\\\").trim();
    }

    public static String parseIdentifier(Object text) {
        return text.toString().toLowerCase(Locale.ENGLISH).replaceAll(" *\\(([^\\)]+)\\)$", "").replaceAll("( \\+ )+| +", "-");
    }

    public static String parseNumber(Object text) {
        return text.toString().replaceAll("[^0-9\\.]+", "");
    }

    public static String getString(Context context, String resName) {
        try {
            resName = resName.toLowerCase(Locale.ENGLISH);
            int resID = context.getResources().getIdentifier(resName, "string", context.getPackageName());
            return context.getString(resID);
        }
        catch (Exception e) {
            return null;
        }
    }

    public static String formatBytes(long bytes) {
        return formatBytes(bytes, true);
    }

    public static String formatBytes(long bytes, boolean withSuffix) {
        if (bytes <= 0) return "0 bytes";
        final String[] units = new String[]{"bytes", "KB", "MB", "GB", "TB"};
        int digitGroups = (int)(Math.log10(bytes) / Math.log10(1024));
        String suffix = withSuffix ? " "+units[digitGroups] : "";
        return String.format(Locale.ENGLISH, "%.2f", bytes / Math.pow(1024, digitGroups))+suffix;
    }

    public static String fromANSIString(byte[] bytes) {
        return fromANSIString(bytes, null);
    }

    public static String fromANSIString(byte[] bytes, Charset charset) {
        String value = charset != null ? new String(bytes, charset) : new String(bytes);
        int indexOfNull = value.indexOf('\0');
        return indexOfNull != -1 ? value.substring(0, indexOfNull) : value;
    }
}
