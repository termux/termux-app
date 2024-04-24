/* main.m
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

#include <pthread.h>
#include <unistd.h>
#include <X11/extensions/applewm.h>

Display *xpbproxy_dpy;
int xpbproxy_apple_wm_event_base, xpbproxy_apple_wm_error_base;
int xpbproxy_xfixes_event_base, xpbproxy_xfixes_error_base;
BOOL xpbproxy_have_xfixes;

extern char *display;

#ifdef STANDALONE_XPBPROXY
BOOL xpbproxy_is_standalone = NO;
#endif

x_selection *_selection_object;

static int
x_io_error_handler(Display *dpy)
{
    /* We lost our connection to the server. */

    TRACE();

    /* trigger the thread to restart?
     *   NO - this would be to a "deeper" problem, and restarts would just
     *        make things worse...
     */
#ifdef STANDALONE_XPBPROXY
    if (xpbproxy_is_standalone)
        exit(EXIT_FAILURE);
#endif

    /* Prevent _XIOError from calling exit() */
    pthread_exit(NULL);
    return 0;
}

static int
x_error_handler(Display *dpy, XErrorEvent *errevent)
{
    return 0;
}

int
xpbproxy_run(void)
{
    @autoreleasepool {
        size_t i;

        for (i = 0, xpbproxy_dpy = NULL; !xpbproxy_dpy && i < 5; i++) {
            xpbproxy_dpy = XOpenDisplay(NULL);

            if (!xpbproxy_dpy && display) {
                char _display[32];
                snprintf(_display, sizeof(_display), ":%s", display);
                setenv("DISPLAY", _display, TRUE);

                xpbproxy_dpy = XOpenDisplay(_display);
            }
            if (!xpbproxy_dpy)
                sleep(1);
        }

        if (xpbproxy_dpy == NULL) {
            ErrorF("xpbproxy: can't open default display\n");
            return EXIT_FAILURE;
        }

        XSetIOErrorHandler(x_io_error_handler);
        XSetErrorHandler(x_error_handler);

        if (!XAppleWMQueryExtension(xpbproxy_dpy, &xpbproxy_apple_wm_event_base,
                                    &xpbproxy_apple_wm_error_base)) {
            ErrorF("xpbproxy: can't open AppleWM server extension\n");
            return EXIT_FAILURE;
        }

        xpbproxy_have_xfixes = XFixesQueryExtension(xpbproxy_dpy, &xpbproxy_xfixes_event_base,
                                                    &xpbproxy_xfixes_error_base);

        XAppleWMSelectInput(xpbproxy_dpy, AppleWMActivationNotifyMask | AppleWMPasteboardNotifyMask);

        _selection_object = [x_selection new];

        if (!xpbproxy_input_register()) {
            return EXIT_FAILURE;
        }
    }

    CFRunLoopRun();

    return EXIT_SUCCESS;
}

id
xpbproxy_selection_object(void)
{
    return _selection_object;
}

Time
xpbproxy_current_timestamp(void)
{
    /* FIXME: may want to fetch a timestamp from the server.. */
    return CurrentTime;
}
