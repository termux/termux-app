/*
 * File: windisplay.c
 * Purpose: Retrieve server display name
 *
 * Copyright (C) Jon TURNEY 2009
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <opaque.h>             // for display
#include "windisplay.h"
#include "winmsg.h"

#define XSERV_t
#define TRANS_SERVER
#include <X11/Xtrans/Xtrans.h>

/*
  Generate a display name string referring to the display of this server,
  using a transport we know is enabled
*/

void
winGetDisplayName(char *szDisplay, unsigned int screen)
{
    if (_XSERVTransIsListening("local")) {
        snprintf(szDisplay, 512, ":%s.%d", display, screen);
    }
    else if (_XSERVTransIsListening("inet")) {
        snprintf(szDisplay, 512, "127.0.0.1:%s.%d", display, screen);
    }
    else if (_XSERVTransIsListening("inet6")) {
        snprintf(szDisplay, 512, "::1:%s.%d", display, screen);
    }
    else {
        // this can't happen!
        ErrorF("winGetDisplay: Don't know what to use for DISPLAY\n");
        snprintf(szDisplay, 512, "localhost:%s.%d", display, screen);
    }

    winDebug("winGetDisplay: DISPLAY=%s\n", szDisplay);
}
