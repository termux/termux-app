package com.termux.shared.models.errors;

/** The {@link Class} that defines function error messages and codes. */
public class FunctionErrno extends Errno {

    public static final String TYPE = "Function Error";


    /* Errors for null or empty parameters (100-150) */
    public static final Errno ERRNO_NULL_OR_EMPTY_PARAMETER = new Errno(TYPE, 100, "The %1$s parameter passed to \"%2$s\" is null or empty.");
    public static final Errno ERRNO_NULL_OR_EMPTY_PARAMETERS = new Errno(TYPE, 101, "The %1$s parameters passed to \"%2$s\" are null or empty.");
    public static final Errno ERRNO_UNSET_PARAMETER = new Errno(TYPE, 102, "The %1$s parameter passed to \"%2$s\" must be set.");
    public static final Errno ERRNO_UNSET_PARAMETERS = new Errno(TYPE, 103, "The %1$s parameters passed to \"%2$s\" must be set.");
    public static final Errno ERRNO_INVALID_PARAMETER = new Errno(TYPE, 104, "The %1$s parameter passed to \"%2$s\" is invalid.\"%3$s\"");


    FunctionErrno(final String type, final int code, final String message) {
        super(type, code, message);
    }

}
