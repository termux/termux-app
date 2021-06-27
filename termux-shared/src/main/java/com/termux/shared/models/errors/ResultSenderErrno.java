package com.termux.shared.models.errors;

/** The {@link Class} that defines ResultSender error messages and codes. */
public class ResultSenderErrno extends Errno {

    public static final String TYPE = "ResultSender Error";


    /* Errors for null or empty parameters (100-150) */
    public static final Errno ERROR_RESULT_FILE_BASENAME_NULL_OR_INVALID = new Errno(TYPE, 100, "The result file basename \"%1$s\" is null, empty or contains forward slashes \"/\".");
    public static final Errno ERROR_RESULT_FILES_SUFFIX_INVALID = new Errno(TYPE, 101, "The result files suffix \"%1$s\" contains forward slashes \"/\".");
    public static final Errno ERROR_FORMAT_RESULT_ERROR_FAILED_WITH_EXCEPTION = new Errno(TYPE, 102, "Formatting result error failed.\nException: %1$s");
    public static final Errno ERROR_FORMAT_RESULT_OUTPUT_FAILED_WITH_EXCEPTION = new Errno(TYPE, 103, "Formatting result output failed.\nException: %1$s");


    ResultSenderErrno(final String type, final int code, final String message) {
        super(type, code, message);
    }

}
