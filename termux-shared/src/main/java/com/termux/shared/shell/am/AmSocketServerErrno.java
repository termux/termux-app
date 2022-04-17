package com.termux.shared.shell.am;

import com.termux.shared.errors.Errno;

public class AmSocketServerErrno extends Errno {

    public static final String TYPE = "AmSocketServer Error";


    /** Errors for {@link AmSocketServer} (100-150) */
    public static final Errno ERRNO_PARSE_AM_COMMAND_FAILED_WITH_EXCEPTION = new Errno(TYPE, 100, "Parse am command `%1$s` failed.\nException: %2$s");
    public static final Errno ERRNO_RUN_AM_COMMAND_FAILED_WITH_EXCEPTION = new Errno(TYPE, 101, "Run am command `%1$s` failed.\nException: %2$s");

    AmSocketServerErrno(final String type, final int code, final String message) {
        super(type, code, message);
    }

}
