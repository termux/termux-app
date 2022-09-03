package com.termux.shared.errors;

import android.app.Activity;

import androidx.annotation.NonNull;

import com.termux.shared.logger.Logger;

import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

/** The {@link Class} that defines error messages and codes. */
public class Errno {

    private static final HashMap<String, Errno> map = new HashMap<>();

    public static final String TYPE = "Error";


    public static final Errno ERRNO_SUCCESS = new Errno(TYPE, Activity.RESULT_OK, "Success");
    public static final Errno ERRNO_CANCELLED = new Errno(TYPE, Activity.RESULT_CANCELED, "Cancelled");
    public static final Errno ERRNO_MINOR_FAILURES = new Errno(TYPE, Activity.RESULT_FIRST_USER, "Minor failure");
    public static final Errno ERRNO_FAILED = new Errno(TYPE, Activity.RESULT_FIRST_USER + 1, "Failed");

    /** The errno type. */
    protected final String type;
    /** The errno code. */
    protected final int code;
    /** The errno message. */
    protected final String message;

    private static final String LOG_TAG = "Errno";


    public Errno(@NonNull final String type, final int code, @NonNull final String message) {
        this.type = type;
        this.code = code;
        this.message = message;
        map.put(type + ":" + code, this);
    }

    @NonNull
    @Override
    public String toString() {
        return "type=" + type + ", code=" + code + ", message=\"" + message + "\"";
    }

    @NonNull
    public String getType() {
        return type;
    }

    public int getCode() {
        return code;
    }

    @NonNull
    public String getMessage() {
        return message;
    }



    /**
     * Get the {@link Errno} of a specific type and code.
     *
     * @param type The unique type of the {@link Errno}.
     * @param code The unique code of the {@link Errno}.
     */
    public static Errno valueOf(String type, Integer code) {
        if (type == null || type.isEmpty() || code == null) return null;
        return map.get(type + ":" + code);
    }



    public Error getError() {
        return new Error(getType(), getCode(), getMessage());
    }

    public Error getError(Object... args) {
        try {
            return new Error(getType(), getCode(), String.format(getMessage(), args));
        } catch (Exception e) {
            Logger.logWarn(LOG_TAG, "Exception raised while calling String.format() for error message of errno " + this + " with args" + Arrays.toString(args) + "\n" + e.getMessage());
            // Return unformatted message as a backup
            return new Error(getType(), getCode(), getMessage() + ": " + Arrays.toString(args));
        }
    }

    public Error getError(Throwable throwable, Object... args) {
        if (throwable == null)
            return getError(args);
        else
            return getError(Collections.singletonList(throwable), args);
    }

    public Error getError(List<Throwable> throwablesList, Object... args) {
        try {
            if (throwablesList == null)
                return new Error(getType(), getCode(), String.format(getMessage(), args));
            else
                return new Error(getType(), getCode(), String.format(getMessage(), args), throwablesList);
        } catch (Exception e) {
            Logger.logWarn(LOG_TAG, "Exception raised while calling String.format() for error message of errno " + this + " with args" + Arrays.toString(args) + "\n" + e.getMessage());
            // Return unformatted message as a backup
            return new Error(getType(), getCode(), getMessage() + ": " + Arrays.toString(args), throwablesList);
        }
    }

    public boolean equalsErrorTypeAndCode(Error error) {
        if (error == null) return false;
        return type.equals(error.getType()) && code == error.getCode();
    }

}
