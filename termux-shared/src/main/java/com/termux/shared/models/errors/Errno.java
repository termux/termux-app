package com.termux.shared.models.errors;

import android.app.Activity;

import androidx.annotation.NonNull;

import com.termux.shared.logger.Logger;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;

/** The {@link Class} that defines error messages and codes. */
public class Errno {

    public static final String TYPE = "Error";


    public static final Errno ERRNO_SUCCESS = new Errno(TYPE, Activity.RESULT_OK, "Success");
    public static final Errno ERRNO_CANCELLED = new Errno(TYPE, Activity.RESULT_CANCELED, "Cancelled");
    public static final Errno ERRNO_MINOR_FAILURES = new Errno(TYPE, Activity.RESULT_FIRST_USER, "Minor failure");
    public static final Errno ERRNO_FAILED = new Errno(TYPE, Activity.RESULT_FIRST_USER + 1, "Failed");

    /** The errno type. */
    protected String type;
    /** The errno code. */
    protected final int code;
    /** The errno message. */
    protected final String message;

    private static final String LOG_TAG = "Errno";


    public Errno(final String type, final int code, final String message) {
        this.type = type;
        this.code = code;
        this.message = message;
    }

    @NonNull
    @Override
    public String toString() {
        return "type=" + type + ", code=" + code + ", message=\"" + message + "\"";
    }


    public String getType() {
        return type;
    }

    public String getMessage() {
        return message;
    }

    public int getCode() {
        return code;
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
        return getError(Collections.singletonList(throwable), args);
    }

    public Error getError(List<Throwable> throwablesList, Object... args) {
        try {
            return new Error(getType(), getCode(), String.format(getMessage(), args), throwablesList);
        } catch (Exception e) {
            Logger.logWarn(LOG_TAG, "Exception raised while calling String.format() for error message of errno " + this + " with args" + Arrays.toString(args) + "\n" + e.getMessage());
            // Return unformatted message as a backup
            return new Error(getType(), getCode(), getMessage() + ": " + Arrays.toString(args), throwablesList);
        }
    }

}
