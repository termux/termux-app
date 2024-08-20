package com.termux.x11.controller.core;

import androidx.annotation.NonNull;

import java.util.Iterator;

public class KeyValueSet implements Iterable<String[]> {
    private String data = "";

    public KeyValueSet(String data) {
        this.data = data != null && !data.isEmpty() ? data : "";
    }

    private int[] indexOfKey(String key) {
        int start = 0;
        int end = data.indexOf(",");
        if (end == -1) end = data.length();

        while (start < end) {
            int index = data.indexOf("=", start);
            String currKey = data.substring(start, index);
            if (currKey.equals(key)) return new int[]{start, end};
            start = end+1;
            end = data.indexOf(",", start);
            if (end == -1) end = data.length();
        }

        return null;
    }

    public String get(String key) {
        for (String[] keyValue : this) if (keyValue[0].equals(key)) return keyValue[1];
        return "";
    }

    public void put(String key, Object value) {
        int[] range = indexOfKey(key);
        if (range != null) {
            data = StringUtils.replace(data, range[0], range[1], key+"="+value);
        }
        else data = (!data.isEmpty() ? data+"," : "")+key+"="+value;
    }

    @NonNull
    @Override
    public Iterator<String[]> iterator() {
        final int[] start = {0};
        final int[] end = {data.indexOf(",")};
        final String[] item = new String[2];
        return new Iterator<String[]>() {
            @Override
            public boolean hasNext() {
                return start[0] < end[0];
            }

            @Override
            public String[] next() {
                int index = data.indexOf("=", start[0]);
                item[0] = data.substring(start[0], index);
                item[1] = data.substring(index+1, end[0]);
                start[0] = end[0]+1;
                end[0] = data.indexOf(",", start[0]);
                if (end[0] == -1) end[0] = data.length();
                return item;
            }
        };
    }

    @NonNull
    @Override
    public String toString() {
        return data;
    }
}
