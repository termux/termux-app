/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors: Alexander Gottwald	
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"
#include "winmsg.h"
#if CYGDEBUG
#include "winmessages.h"
#endif
#include <stdarg.h>

#ifdef XWIN_XF86CONFIG
void
winDrvMsg(int scrnIndex, MessageType type, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogVMessageVerb(type, 0, format, ap);
    va_end(ap);
}

void
winDrvMsgVerb(int scrnIndex, MessageType type, int verb, const char *format,
              ...)
{
    va_list ap;

    va_start(ap, format);
    LogVMessageVerb(type, verb, format, ap);
    va_end(ap);
}
#endif

void
winErrorFVerb(int verb, const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogVMessageVerb(X_NONE, verb, format, ap);
    va_end(ap);
}

void
winDebug(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogVMessageVerb(X_NONE, 3, format, ap);
    va_end(ap);
}

void
winTrace(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    LogVMessageVerb(X_NONE, 10, format, ap);
    va_end(ap);
}

void
winW32Error(int verb, const char *msg)
{
    winW32ErrorEx(verb, msg, GetLastError());
}

void
winW32ErrorEx(int verb, const char *msg, DWORD errorcode)
{
    LPVOID buffer;

    if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       errorcode,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPTSTR) &buffer, 0, NULL)) {
        winErrorFVerb(verb, "Unknown error in FormatMessage!\n");
    }
    else {
        winErrorFVerb(verb, "%s %s", msg, (char *) buffer);
        LocalFree(buffer);
    }
}

#if CYGDEBUG
void
winDebugWin32Message(const char *function, HWND hwnd, UINT message,
                     WPARAM wParam, LPARAM lParam)
{
    static int force = 0;

    if (message >= WM_USER) {
        if (force || getenv("WIN_DEBUG_MESSAGES") ||
            getenv("WIN_DEBUG_WM_USER")) {
            winDebug("%s - Message WM_USER + %d\n", function,
                     message - WM_USER);
            winDebug("\thwnd 0x%p wParam 0x%x lParam 0x%x\n", hwnd, (int)wParam,
                     (int)lParam);
        }
    }
    else if (message < MESSAGE_NAMES_LEN && MESSAGE_NAMES[message]) {
        const char *msgname = MESSAGE_NAMES[message];
        char buffer[64];

        snprintf(buffer, sizeof(buffer), "WIN_DEBUG_%s", msgname);
        buffer[63] = 0;
        if (force || getenv("WIN_DEBUG_MESSAGES") || getenv(buffer)) {
            winDebug("%s - Message %s\n", function, MESSAGE_NAMES[message]);
            winDebug("\thwnd 0x%p wParam 0x%x lParam 0x%x\n", hwnd, (int)wParam,
                     (int)lParam);
        }
    }
}
#else
void
winDebugWin32Message(const char *function, HWND hwnd, UINT message,
                     WPARAM wParam, LPARAM lParam)
{
}
#endif
