package com.termux.x11.controller.core;

import org.json.JSONArray;
import org.json.JSONException;

import java.util.Arrays;

public abstract class ArrayUtils {
    public static byte[] concat(byte[]... elements) {
        byte[] result = Arrays.copyOf(elements[0], elements[0].length);
        for (int i = 1; i < elements.length; i++) {
            byte[] newArray = Arrays.copyOf(result, result.length + elements[i].length);
            System.arraycopy(elements[i], 0, newArray, result.length, elements[i].length);
            result = newArray;
        }
        return result;
    }

    @SafeVarargs
    public static <T> T[] concat(T[]... elements) {
        T[] result = Arrays.copyOf(elements[0], elements[0].length);
        for (int i = 1; i < elements.length; i++) {
            T[] newArray = Arrays.copyOf(result, result.length + elements[i].length);
            System.arraycopy(elements[i], 0, newArray, result.length, elements[i].length);
            result = newArray;
        }
        return result;
    }

    public static String[] toStringArray(JSONArray data) {
        String[] stringArray = new String[data.length()];
        for (int i = 0; i < data.length(); i++) {
            try {
                stringArray[i] = data.getString(i);
            }
            catch (JSONException e) {}
        }
        return stringArray;
    }
}
