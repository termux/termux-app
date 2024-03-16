/* app-main.m
 *
 * Copyright (c) 2002-2012 Apple Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 * HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above
 * copyright holders shall not be used in advertising or otherwise to
 * promote the sale, use or other dealings in this Software without
 * prior written authorization.
 */

#include "pbproxy.h"
#import "x-selection.h"

/* Stubs */
char *display = NULL;

static void
signal_handler(int sig)
{
    switch (sig) {
    case SIGHUP:
        xpbproxy_prefs_reload = YES;
        break;

    default:
        _exit(EXIT_SUCCESS);
    }
}

void
ErrorF(const char * f, ...)
{
    va_list args;

    va_start(args, f);
    vfprintf(stderr, f, args);
    va_end(args);
}

/* TODO: Have this actually log to ASL */
void
xq_asl_log(int level, const char *subsystem, const char *file,
           const char *function, int line, const char *fmt,
           ...)
{
#ifdef DEBUG
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
#endif
}

int
main(int argc, const char *argv[])
{
    xpbproxy_is_standalone = YES;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    return xpbproxy_run();
}
