package com.termux.shared.activity;

import com.termux.shared.errors.Errno;

public class ActivityErrno extends Errno {

    public static final String TYPE = "Activity Error";


    /* Errors for starting activities (100-150) */
    public static final Errno ERRNO_START_ACTIVITY_FAILED_WITH_EXCEPTION = new Errno(TYPE, 100, "Failed to start \"%1$s\" activity.\nException: %2$s");
    public static final Errno ERRNO_START_ACTIVITY_FOR_RESULT_FAILED_WITH_EXCEPTION = new Errno(TYPE, 101, "Failed to start \"%1$s\" activity for result.\nException: %2$s");
    public static final Errno ERRNO_STARTING_ACTIVITY_WITH_NULL_CONTEXT = new Errno(TYPE, 102, "Cannot start \"%1$s\" activity with null Context");


    ActivityErrno(final String type, final int code, final String message) {
        super(type, code, message);
    }

}
