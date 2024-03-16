/*

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
Copyright 1994 Quarterdeck Office Systems.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital and
Quarterdeck not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

DIGITAL AND QUARTERDECK DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
OR PERFORMANCE OF THIS SOFTWARE.

*/

/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/Xos.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <stdlib.h>             /* for malloc() */
#include <errno.h>

#include "input.h"
#include "opaque.h"

#ifdef WIN32
#include <process.h>
#define getpid(x) _getpid(x)
#endif

#ifdef XF86BIGFONT
#include "xf86bigfontsrv.h"
#endif

#ifdef __clang__
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#endif

#ifdef DDXOSVERRORF
void (*OsVendorVErrorFProc) (const char *, va_list args) = NULL;
#endif

/* Default logging parameters. */
#ifndef DEFAULT_LOG_VERBOSITY
#define DEFAULT_LOG_VERBOSITY		0
#endif
#ifndef DEFAULT_LOG_FILE_VERBOSITY
#define DEFAULT_LOG_FILE_VERBOSITY	3
#endif

static FILE *logFile = NULL;
static int logFileFd = -1;
static Bool logFlush = FALSE;
static Bool logSync = FALSE;
static int logVerbosity = DEFAULT_LOG_VERBOSITY;
static int logFileVerbosity = DEFAULT_LOG_FILE_VERBOSITY;

/* Buffer to information logged before the log file is opened. */
static char *saveBuffer = NULL;
static int bufferSize = 0, bufferUnused = 0, bufferPos = 0;
static Bool needBuffer = TRUE;

#ifdef __APPLE__
static char __crashreporter_info_buff__[4096] = { 0 };

static const char *__crashreporter_info__ __attribute__ ((__used__)) =
    &__crashreporter_info_buff__[0];
asm(".desc ___crashreporter_info__, 0x10");
#endif

/* Prefix strings for log messages. */
#ifndef X_UNKNOWN_STRING
#define X_UNKNOWN_STRING		"(\?\?)"
#endif
#ifndef X_PROBE_STRING
#define X_PROBE_STRING			"(--)"
#endif
#ifndef X_CONFIG_STRING
#define X_CONFIG_STRING			"(**)"
#endif
#ifndef X_DEFAULT_STRING
#define X_DEFAULT_STRING		"(==)"
#endif
#ifndef X_CMDLINE_STRING
#define X_CMDLINE_STRING		"(++)"
#endif
#ifndef X_NOTICE_STRING
#define X_NOTICE_STRING			"(!!)"
#endif
#ifndef X_ERROR_STRING
#define X_ERROR_STRING			"(EE)"
#endif
#ifndef X_WARNING_STRING
#define X_WARNING_STRING		"(WW)"
#endif
#ifndef X_INFO_STRING
#define X_INFO_STRING			"(II)"
#endif
#ifndef X_NOT_IMPLEMENTED_STRING
#define X_NOT_IMPLEMENTED_STRING	"(NI)"
#endif
#ifndef X_DEBUG_STRING
#define X_DEBUG_STRING			"(DB)"
#endif
#ifndef X_NONE_STRING
#define X_NONE_STRING			""
#endif

static size_t
strlen_sigsafe(const char *s)
{
    size_t len;
    for (len = 0; s[len]; len++);
    return len;
}

/*
 * LogFilePrep is called to setup files for logging, including getting
 * an old file out of the way, but it doesn't actually open the file,
 * since it may be used for renaming a file we're already logging to.
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

static char *
LogFilePrep(const char *fname, const char *backup, const char *idstring)
{
    char *logFileName = NULL;

    /* the format string below is controlled by the user,
       this code should never be called with elevated privileges */
    if (asprintf(&logFileName, fname, idstring) == -1)
        FatalError("Cannot allocate space for the log file name\n");

    if (backup && *backup) {
        struct stat buf;

        if (!stat(logFileName, &buf) && S_ISREG(buf.st_mode)) {
            char *suffix;
            char *oldLog;

            if ((asprintf(&suffix, backup, idstring) == -1) ||
                (asprintf(&oldLog, "%s%s", logFileName, suffix) == -1)) {
                FatalError("Cannot allocate space for the log file name\n");
            }
            free(suffix);

            if (rename(logFileName, oldLog) == -1) {
                FatalError("Cannot move old log file \"%s\" to \"%s\"\n",
                           logFileName, oldLog);
            }
            free(oldLog);
        }
    }
    else {
        if (remove(logFileName) != 0 && errno != ENOENT) {
            FatalError("Cannot remove old log file \"%s\": %s\n",
                       logFileName, strerror(errno));
        }
    }

    return logFileName;
}
#pragma GCC diagnostic pop

/*
 * LogInit is called to start logging to a file.  It is also called (with
 * NULL arguments) when logging to a file is not wanted.  It must always be
 * called, otherwise log messages will continue to accumulate in a buffer.
 *
 * %s, if present in the fname or backup strings, is expanded to the display
 * string (or to a string containing the pid if the display is not yet set).
 */

static char *saved_log_fname;
static char *saved_log_backup;
static char *saved_log_tempname;

const char *
LogInit(const char *fname, const char *backup)
{
    char *logFileName = NULL;

    if (fname && *fname) {
        if (displayfd != -1) {
            /* Display isn't set yet, so we can't use it in filenames yet. */
            char pidstring[32];
            snprintf(pidstring, sizeof(pidstring), "pid-%ld",
                     (unsigned long) getpid());
            logFileName = LogFilePrep(fname, backup, pidstring);
            saved_log_tempname = logFileName;

            /* Save the patterns for use when the display is named. */
            saved_log_fname = strdup(fname);
            if (backup == NULL)
                saved_log_backup = NULL;
            else
                saved_log_backup = strdup(backup);
        } else
            logFileName = LogFilePrep(fname, backup, display);
        if ((logFile = fopen(logFileName, "w")) == NULL)
            FatalError("Cannot open log file \"%s\"\n", logFileName);
        setvbuf(logFile, NULL, _IONBF, 0);

        logFileFd = fileno(logFile);

        /* Flush saved log information. */
        if (saveBuffer && bufferSize > 0) {
            fwrite(saveBuffer, bufferPos, 1, logFile);
            fflush(logFile);
#ifndef WIN32
            fsync(fileno(logFile));
#endif
        }
    }

    /*
     * Unconditionally free the buffer, and flag that the buffer is no longer
     * needed.
     */
    if (saveBuffer && bufferSize > 0) {
        free(saveBuffer);
        saveBuffer = NULL;
        bufferSize = 0;
    }
    needBuffer = FALSE;

    return logFileName;
}

void
LogSetDisplay(void)
{
    if (saved_log_fname && strstr(saved_log_fname, "%s")) {
        char *logFileName;

        logFileName = LogFilePrep(saved_log_fname, saved_log_backup, display);

        if (rename(saved_log_tempname, logFileName) == 0) {
            LogMessageVerb(X_PROBED, 0,
                           "Log file renamed from \"%s\" to \"%s\"\n",
                           saved_log_tempname, logFileName);

            if (strlen(saved_log_tempname) >= strlen(logFileName))
                strncpy(saved_log_tempname, logFileName,
                        strlen(saved_log_tempname));
        }
        else {
            ErrorF("Failed to rename log file \"%s\" to \"%s\": %s\n",
                   saved_log_tempname, logFileName, strerror(errno));
        }

        /* free newly allocated string - can't free old one since existing
           pointers to it may exist in DDX callers. */
        free(logFileName);
        free(saved_log_fname);
        free(saved_log_backup);
    }
}

void
LogClose(enum ExitCode error)
{
    if (logFile) {
        int msgtype = (error == EXIT_NO_ERROR) ? X_INFO : X_ERROR;
        LogMessageVerbSigSafe(msgtype, -1,
                "Server terminated %s (%d). Closing log file.\n",
                (error == EXIT_NO_ERROR) ? "successfully" : "with error",
                error);
        fclose(logFile);
        logFile = NULL;
        logFileFd = -1;
    }
}

Bool
LogSetParameter(LogParameter param, int value)
{
    switch (param) {
    case XLOG_FLUSH:
        logFlush = value ? TRUE : FALSE;
        return TRUE;
    case XLOG_SYNC:
        logSync = value ? TRUE : FALSE;
        return TRUE;
    case XLOG_VERBOSITY:
        logVerbosity = value;
        return TRUE;
    case XLOG_FILE_VERBOSITY:
        logFileVerbosity = value;
        return TRUE;
    default:
        return FALSE;
    }
}

enum {
    LMOD_LONG     = 0x1,
    LMOD_LONGLONG = 0x2,
    LMOD_SHORT    = 0x4,
    LMOD_SIZET    = 0x8,
};

/**
 * Parse non-digit length modifiers and set the corresponding flag in
 * flags_return.
 *
 * @return the number of bytes parsed
 */
static int parse_length_modifier(const char *format, size_t len, int *flags_return)
{
    int idx = 0;
    int length_modifier = 0;

    while (idx < len) {
        switch (format[idx]) {
            case 'l':
                BUG_RETURN_VAL(length_modifier & LMOD_SHORT, 0);

                if (length_modifier & LMOD_LONG)
                    length_modifier |= LMOD_LONGLONG;
                else
                    length_modifier |= LMOD_LONG;
                break;
            case 'h':
                BUG_RETURN_VAL(length_modifier & (LMOD_LONG|LMOD_LONGLONG), 0);
                length_modifier |= LMOD_SHORT;
                /* gcc says 'short int' is promoted to 'int' when
                 * passed through '...', so ignored during
                 * processing */
                break;
            case 'z':
                length_modifier |= LMOD_SIZET;
                break;
            default:
                goto out;
        }
        idx++;
    }

out:
    *flags_return = length_modifier;
    return idx;
}

/**
 * Signal-safe snprintf, with some limitations over snprintf. Be careful
 * which directives you use.
 */
static int
vpnprintf(char *string, int size_in, const char *f, va_list args)
{
    int f_idx = 0;
    int s_idx = 0;
    int f_len = strlen_sigsafe(f);
    char *string_arg;
    char number[21];
    int p_len;
    int i;
    uint64_t ui;
    int64_t si;
    size_t size = size_in;
    int precision;

    for (; f_idx < f_len && s_idx < size - 1; f_idx++) {
        int length_modifier = 0;
        if (f[f_idx] != '%') {
            string[s_idx++] = f[f_idx];
            continue;
        }

        f_idx++;

        /* silently swallow minimum field width */
        if (f[f_idx] == '*') {
            f_idx++;
            va_arg(args, int);
        } else {
            while (f_idx < f_len && ((f[f_idx] >= '0' && f[f_idx] <= '9')))
                f_idx++;
        }

        /* is there a precision? */
        precision = size;
        if (f[f_idx] == '.') {
            f_idx++;
            if (f[f_idx] == '*') {
                f_idx++;
                /* precision is supplied in an int argument */
                precision = va_arg(args, int);
            } else {
                /* silently swallow precision digits */
                while (f_idx < f_len && ((f[f_idx] >= '0' && f[f_idx] <= '9')))
                    f_idx++;
            }
        }

        /* non-digit length modifiers */
        if (f_idx < f_len) {
            int parsed_bytes = parse_length_modifier(&f[f_idx], f_len - f_idx, &length_modifier);
            if (parsed_bytes < 0)
                return 0;
            f_idx += parsed_bytes;
        }

        if (f_idx >= f_len)
            break;

        switch (f[f_idx]) {
        case 's':
            string_arg = va_arg(args, char*);

            for (i = 0; string_arg[i] != 0 && s_idx < size - 1 && s_idx < precision; i++)
                string[s_idx++] = string_arg[i];
            break;

        case 'u':
            if (length_modifier & LMOD_LONGLONG)
                ui = va_arg(args, unsigned long long);
            else if (length_modifier & LMOD_LONG)
                ui = va_arg(args, unsigned long);
            else if (length_modifier & LMOD_SIZET)
                ui = va_arg(args, size_t);
            else
                ui = va_arg(args, unsigned);

            FormatUInt64(ui, number);
            p_len = strlen_sigsafe(number);

            for (i = 0; i < p_len && s_idx < size - 1; i++)
                string[s_idx++] = number[i];
            break;
        case 'i':
        case 'd':
            if (length_modifier & LMOD_LONGLONG)
                si = va_arg(args, long long);
            else if (length_modifier & LMOD_LONG)
                si = va_arg(args, long);
            else if (length_modifier & LMOD_SIZET)
                si = va_arg(args, ssize_t);
            else
                si = va_arg(args, int);

            FormatInt64(si, number);
            p_len = strlen_sigsafe(number);

            for (i = 0; i < p_len && s_idx < size - 1; i++)
                string[s_idx++] = number[i];
            break;

        case 'p':
            string[s_idx++] = '0';
            if (s_idx < size - 1)
                string[s_idx++] = 'x';
            ui = (uintptr_t)va_arg(args, void*);
            FormatUInt64Hex(ui, number);
            p_len = strlen_sigsafe(number);

            for (i = 0; i < p_len && s_idx < size - 1; i++)
                string[s_idx++] = number[i];
            break;

        case 'x':
            if (length_modifier & LMOD_LONGLONG)
                ui = va_arg(args, unsigned long long);
            else if (length_modifier & LMOD_LONG)
                ui = va_arg(args, unsigned long);
            else if (length_modifier & LMOD_SIZET)
                ui = va_arg(args, size_t);
            else
                ui = va_arg(args, unsigned);

            FormatUInt64Hex(ui, number);
            p_len = strlen_sigsafe(number);

            for (i = 0; i < p_len && s_idx < size - 1; i++)
                string[s_idx++] = number[i];
            break;
        case 'f':
            {
                double d = va_arg(args, double);
                FormatDouble(d, number);
                p_len = strlen_sigsafe(number);

                for (i = 0; i < p_len && s_idx < size - 1; i++)
                    string[s_idx++] = number[i];
            }
            break;
        case 'c':
            {
                char c = va_arg(args, int);
                if (s_idx < size - 1)
                    string[s_idx++] = c;
            }
            break;
        case '%':
            string[s_idx++] = '%';
            break;
        default:
            BUG_WARN_MSG(f[f_idx], "Unsupported printf directive '%c'\n", f[f_idx]);
            va_arg(args, char*);
            string[s_idx++] = '%';
            if (s_idx < size - 1)
                string[s_idx++] = f[f_idx];
            break;
        }
    }

    string[s_idx] = '\0';

    return s_idx;
}

static int
pnprintf(char *string, int size, const char *f, ...)
{
    int rc;
    va_list args;

    va_start(args, f);
    rc = vpnprintf(string, size, f, args);
    va_end(args);

    return rc;
}

/* This function does the actual log message writes. It must be signal safe.
 * When attempting to call non-signal-safe functions, guard them with a check
 * of the inSignalContext global variable. */
static void
LogSWrite(int verb, const char *buf, size_t len, Bool end_line)
{
    static Bool newline = TRUE;
    int ret;

    if (verb < 0 || logVerbosity >= verb)
        ret = write(2, buf, len);

    if (verb < 0 || logFileVerbosity >= verb) {
        if (inSignalContext && logFileFd >= 0) {
            ret = write(logFileFd, buf, len);
#ifndef WIN32
            if (logFlush && logSync)
                fsync(logFileFd);
#endif
        }
        else if (!inSignalContext && logFile) {
            if (newline)
                fprintf(logFile, "[%10.3f] ", GetTimeInMillis() / 1000.0);
            newline = end_line;
            fwrite(buf, len, 1, logFile);
            if (logFlush) {
                fflush(logFile);
#ifndef WIN32
                if (logSync)
                    fsync(fileno(logFile));
#endif
            }
        }
        else if (!inSignalContext && needBuffer) {
            if (len > bufferUnused) {
                bufferSize += 1024;
                bufferUnused += 1024;
                saveBuffer = realloc(saveBuffer, bufferSize);
                if (!saveBuffer)
                    FatalError("realloc() failed while saving log messages\n");
            }
            bufferUnused -= len;
            memcpy(saveBuffer + bufferPos, buf, len);
            bufferPos += len;
        }
    }

    /* There's no place to log an error message if the log write
     * fails...
     */
    (void) ret;
}

void
LogVWrite(int verb, const char *f, va_list args)
{
    return LogVMessageVerb(X_NONE, verb, f, args);
}

void
LogWrite(int verb, const char *f, ...)
{
    va_list args;

    va_start(args, f);
    LogVWrite(verb, f, args);
    va_end(args);
}

/* Returns the Message Type string to prepend to a logging message, or NULL
 * if the message will be dropped due to insufficient verbosity. */
static const char *
LogMessageTypeVerbString(MessageType type, int verb)
{
    if (type == X_ERROR)
        verb = 0;

    if (logVerbosity < verb && logFileVerbosity < verb)
        return NULL;

    switch (type) {
    case X_PROBED:
        return X_PROBE_STRING;
    case X_CONFIG:
        return X_CONFIG_STRING;
    case X_DEFAULT:
        return X_DEFAULT_STRING;
    case X_CMDLINE:
        return X_CMDLINE_STRING;
    case X_NOTICE:
        return X_NOTICE_STRING;
    case X_ERROR:
        return X_ERROR_STRING;
    case X_WARNING:
        return X_WARNING_STRING;
    case X_INFO:
        return X_INFO_STRING;
    case X_NOT_IMPLEMENTED:
        return X_NOT_IMPLEMENTED_STRING;
    case X_UNKNOWN:
        return X_UNKNOWN_STRING;
    case X_NONE:
        return X_NONE_STRING;
    case X_DEBUG:
        return X_DEBUG_STRING;
    default:
        return X_UNKNOWN_STRING;
    }
}

void
LogVMessageVerb(MessageType type, int verb, const char *format, va_list args)
{
    const char *type_str;
    char buf[1024];
    const size_t size = sizeof(buf);
    Bool newline;
    size_t len = 0;

    if (inSignalContext) {
        LogVMessageVerbSigSafe(type, verb, format, args);
        return;
    }

    type_str = LogMessageTypeVerbString(type, verb);
    if (!type_str)
        return;

    /* if type_str is not "", prepend it and ' ', to message */
    if (type_str[0] != '\0')
        len += Xscnprintf(&buf[len], size - len, "%s ", type_str);

    if (size - len > 1)
        len += Xvscnprintf(&buf[len], size - len, format, args);

    /* Force '\n' at end of truncated line */
    if (size - len == 1)
        buf[len - 1] = '\n';

    newline = (buf[len - 1] == '\n');
    LogSWrite(verb, buf, len, newline);
}

/* Log message with verbosity level specified. */
void
LogMessageVerb(MessageType type, int verb, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogVMessageVerb(type, verb, format, ap);
    va_end(ap);
}

/* Log a message with the standard verbosity level of 1. */
void
LogMessage(MessageType type, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogVMessageVerb(type, 1, format, ap);
    va_end(ap);
}

/* Log a message using only signal safe functions. */
void
LogMessageVerbSigSafe(MessageType type, int verb, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    LogVMessageVerbSigSafe(type, verb, format, ap);
    va_end(ap);
}

void
LogVMessageVerbSigSafe(MessageType type, int verb, const char *format, va_list args)
{
    const char *type_str;
    char buf[1024];
    int len;
    Bool newline;

    type_str = LogMessageTypeVerbString(type, verb);
    if (!type_str)
        return;

    /* if type_str is not "", prepend it and ' ', to message */
    if (type_str[0] != '\0') {
        LogSWrite(verb, type_str, strlen_sigsafe(type_str), FALSE);
        LogSWrite(verb, " ", 1, FALSE);
    }

    len = vpnprintf(buf, sizeof(buf), format, args);

    /* Force '\n' at end of truncated line */
    if (sizeof(buf) - len == 1)
        buf[len - 1] = '\n';

    newline = (len > 0 && buf[len - 1] == '\n');
    LogSWrite(verb, buf, len, newline);
}

void
LogVHdrMessageVerb(MessageType type, int verb, const char *msg_format,
                   va_list msg_args, const char *hdr_format, va_list hdr_args)
{
    const char *type_str;
    char buf[1024];
    const size_t size = sizeof(buf);
    Bool newline;
    size_t len = 0;
    int (*vprintf_func)(char *, int, const char* _X_RESTRICT_KYWD f, va_list args)
            _X_ATTRIBUTE_PRINTF(3, 0);
    int (*printf_func)(char *, int, const char* _X_RESTRICT_KYWD f, ...)
            _X_ATTRIBUTE_PRINTF(3, 4);

    type_str = LogMessageTypeVerbString(type, verb);
    if (!type_str)
        return;

    if (inSignalContext) {
        vprintf_func = vpnprintf;
        printf_func = pnprintf;
    } else {
        vprintf_func = Xvscnprintf;
        printf_func = Xscnprintf;
    }

    /* if type_str is not "", prepend it and ' ', to message */
    if (type_str[0] != '\0')
        len += printf_func(&buf[len], size - len, "%s ", type_str);

    if (hdr_format && size - len > 1)
        len += vprintf_func(&buf[len], size - len, hdr_format, hdr_args);

    if (msg_format && size - len > 1)
        len += vprintf_func(&buf[len], size - len, msg_format, msg_args);

    /* Force '\n' at end of truncated line */
    if (size - len == 1)
        buf[len - 1] = '\n';

    newline = (buf[len - 1] == '\n');
    LogSWrite(verb, buf, len, newline);
}

void
LogHdrMessageVerb(MessageType type, int verb, const char *msg_format,
                  va_list msg_args, const char *hdr_format, ...)
{
    va_list hdr_args;

    va_start(hdr_args, hdr_format);
    LogVHdrMessageVerb(type, verb, msg_format, msg_args, hdr_format, hdr_args);
    va_end(hdr_args);
}

void
LogHdrMessage(MessageType type, const char *msg_format, va_list msg_args,
              const char *hdr_format, ...)
{
    va_list hdr_args;

    va_start(hdr_args, hdr_format);
    LogVHdrMessageVerb(type, 1, msg_format, msg_args, hdr_format, hdr_args);
    va_end(hdr_args);
}

void
AbortServer(void)
    _X_NORETURN;

void
AbortServer(void)
{
#ifdef XF86BIGFONT
    XF86BigfontCleanup();
#endif
    CloseWellKnownConnections();
    OsCleanup(TRUE);
    AbortDevices();
    ddxGiveUp(EXIT_ERR_ABORT);
    fflush(stderr);
    if (CoreDump)
        OsAbort();
    exit(1);
}

#define AUDIT_PREFIX "AUDIT: %s: %ld: "
#ifndef AUDIT_TIMEOUT
#define AUDIT_TIMEOUT ((CARD32)(120 * 1000))    /* 2 mn */
#endif

static int nrepeat = 0;
static int oldlen = -1;
static OsTimerPtr auditTimer = NULL;

void
FreeAuditTimer(void)
{
    if (auditTimer != NULL) {
        /* Force output of pending messages */
        TimerForce(auditTimer);
        TimerFree(auditTimer);
        auditTimer = NULL;
    }
}

static char *
AuditPrefix(void)
{
    time_t tm;
    char *autime, *s;
    char *tmpBuf;
    int len;

    time(&tm);
    autime = ctime(&tm);
    if ((s = strchr(autime, '\n')))
        *s = '\0';
    len = strlen(AUDIT_PREFIX) + strlen(autime) + 10 + 1;
    tmpBuf = malloc(len);
    if (!tmpBuf)
        return NULL;
    snprintf(tmpBuf, len, AUDIT_PREFIX, autime, (unsigned long) getpid());
    return tmpBuf;
}

void
AuditF(const char *f, ...)
{
    va_list args;

    va_start(args, f);

    VAuditF(f, args);
    va_end(args);
}

static CARD32
AuditFlush(OsTimerPtr timer, CARD32 now, void *arg)
{
    char *prefix;

    if (nrepeat > 0) {
        prefix = AuditPrefix();
        ErrorF("%slast message repeated %d times\n",
               prefix != NULL ? prefix : "", nrepeat);
        nrepeat = 0;
        free(prefix);
        return AUDIT_TIMEOUT;
    }
    else {
        /* if the timer expires without anything to print, flush the message */
        oldlen = -1;
        return 0;
    }
}

void
VAuditF(const char *f, va_list args)
{
    char *prefix;
    char buf[1024];
    int len;
    static char oldbuf[1024];

    prefix = AuditPrefix();
    len = vsnprintf(buf, sizeof(buf), f, args);

    if (len == oldlen && strcmp(buf, oldbuf) == 0) {
        /* Message already seen */
        nrepeat++;
    }
    else {
        /* new message */
        if (auditTimer != NULL)
            TimerForce(auditTimer);
        ErrorF("%s%s", prefix != NULL ? prefix : "", buf);
        strlcpy(oldbuf, buf, sizeof(oldbuf));
        oldlen = len;
        nrepeat = 0;
        auditTimer = TimerSet(auditTimer, 0, AUDIT_TIMEOUT, AuditFlush, NULL);
    }
    free(prefix);
}

void
FatalError(const char *f, ...)
{
    va_list args;
    va_list args2;
    static Bool beenhere = FALSE;

    if (beenhere)
        ErrorFSigSafe("\nFatalError re-entered, aborting\n");
    else
        ErrorFSigSafe("\nFatal server error:\n");

    va_start(args, f);

    /* Make a copy for OsVendorFatalError */
    va_copy(args2, args);

#ifdef __APPLE__
    {
        va_list apple_args;

        va_copy(apple_args, args);
        (void)vsnprintf(__crashreporter_info_buff__,
                        sizeof(__crashreporter_info_buff__), f, apple_args);
        va_end(apple_args);
    }
#endif
    VErrorFSigSafe(f, args);
    va_end(args);
    ErrorFSigSafe("\n");
    if (!beenhere)
        OsVendorFatalError(f, args2);
    va_end(args2);
    if (!beenhere) {
        beenhere = TRUE;
        AbortServer();
    }
    else
        OsAbort();
 /*NOTREACHED*/}

void
VErrorF(const char *f, va_list args)
{
#ifdef DDXOSVERRORF
    if (OsVendorVErrorFProc)
        OsVendorVErrorFProc(f, args);
    else
        LogVWrite(-1, f, args);
#else
    LogVWrite(-1, f, args);
#endif
}

void
ErrorF(const char *f, ...)
{
    va_list args;

    va_start(args, f);
    VErrorF(f, args);
    va_end(args);
}

void
VErrorFSigSafe(const char *f, va_list args)
{
    LogVMessageVerbSigSafe(X_ERROR, -1, f, args);
}

void
ErrorFSigSafe(const char *f, ...)
{
    va_list args;

    va_start(args, f);
    VErrorFSigSafe(f, args);
    va_end(args);
}

void
LogPrintMarkers(void)
{
    /* Show what the message marker symbols mean. */
    LogWrite(0, "Markers: ");
    LogMessageVerb(X_PROBED, 0, "probed, ");
    LogMessageVerb(X_CONFIG, 0, "from config file, ");
    LogMessageVerb(X_DEFAULT, 0, "default setting,\n\t");
    LogMessageVerb(X_CMDLINE, 0, "from command line, ");
    LogMessageVerb(X_NOTICE, 0, "notice, ");
    LogMessageVerb(X_INFO, 0, "informational,\n\t");
    LogMessageVerb(X_WARNING, 0, "warning, ");
    LogMessageVerb(X_ERROR, 0, "error, ");
    LogMessageVerb(X_NOT_IMPLEMENTED, 0, "not implemented, ");
    LogMessageVerb(X_UNKNOWN, 0, "unknown.\n");
}
