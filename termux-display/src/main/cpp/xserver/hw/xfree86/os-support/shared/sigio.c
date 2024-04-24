/* sigio.c -- Support for SIGIO handler installation and removal
 * Created: Thu Jun  3 15:39:18 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors: Rickard E. (Rik) Faith <faith@valinux.com>
 */
/*
 * Copyright (c) 2002 by The XFree86 Project, Inc.
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include <xserver_poll.h>
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "inputstr.h"

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef MAXDEVICES
/* MAXDEVICES represents the maximum number of input devices usable
 * at the same time plus one entry for DRM support.
 */
#define MAX_FUNCS   (MAXDEVICES + 1)
#else
#define MAX_FUNCS 16
#endif

typedef struct _xf86SigIOFunc {
    void (*f) (int, void *);
    int fd;
    void *closure;
} Xf86SigIOFunc;

static Xf86SigIOFunc xf86SigIOFuncs[MAX_FUNCS];
static int xf86SigIOMax;
static struct pollfd *xf86SigIOFds;
static int xf86SigIONum;

static Bool
xf86SigIOAdd(int fd)
{
    struct pollfd *n;

    n = realloc(xf86SigIOFds, (xf86SigIONum + 1) * sizeof (struct pollfd));
    if (!n)
        return FALSE;

    n[xf86SigIONum].fd = fd;
    n[xf86SigIONum].events = POLLIN;
    xf86SigIONum++;
    xf86SigIOFds = n;
    return TRUE;
}

static void
xf86SigIORemove(int fd)
{
    int i;
    for (i = 0; i < xf86SigIONum; i++)
        if (xf86SigIOFds[i].fd == fd) {
            memmove(&xf86SigIOFds[i], &xf86SigIOFds[i+1], (xf86SigIONum - i - 1) * sizeof (struct pollfd));
            xf86SigIONum--;
            break;
        }
}

/*
 * SIGIO gives no way of discovering which fd signalled, select
 * to discover
 */
static void
xf86SIGIO(int sig)
{
    int i, f;
    int save_errno = errno;     /* do not clobber the global errno */
    int r;

    inSignalContext = TRUE;

    SYSCALL(r = xserver_poll(xf86SigIOFds, xf86SigIONum, 0));
    for (f = 0; r > 0 && f < xf86SigIONum; f++) {
        if (xf86SigIOFds[f].revents & POLLIN) {
            for (i = 0; i < xf86SigIOMax; i++)
                if (xf86SigIOFuncs[i].f && xf86SigIOFuncs[i].fd == xf86SigIOFds[f].fd)
                    (*xf86SigIOFuncs[i].f) (xf86SigIOFuncs[i].fd,
                                            xf86SigIOFuncs[i].closure);
            r--;
        }
    }
    if (r > 0) {
        xf86Msg(X_ERROR, "SIGIO %d descriptors not handled\n", r);
    }
    /* restore global errno */
    errno = save_errno;

    inSignalContext = FALSE;
}

static int
xf86IsPipe(int fd)
{
    struct stat buf;

    if (fstat(fd, &buf) < 0)
        return 0;
    return S_ISFIFO(buf.st_mode);
}

static void
block_sigio(void)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGIO);
    xthread_sigmask(SIG_BLOCK, &set, NULL);
}

static void
release_sigio(void)
{
    sigset_t set;

    sigemptyset(&set);
    sigaddset(&set, SIGIO);
    xthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

int
xf86InstallSIGIOHandler(int fd, void (*f) (int, void *), void *closure)
{
    struct sigaction sa;
    struct sigaction osa;
    int i;
    int installed = FALSE;

    for (i = 0; i < MAX_FUNCS; i++) {
        if (!xf86SigIOFuncs[i].f) {
            if (xf86IsPipe(fd))
                return 0;
            block_sigio();
#ifdef O_ASYNC
            if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_ASYNC) == -1) {
                xf86Msg(X_WARNING, "fcntl(%d, O_ASYNC): %s\n",
                        fd, strerror(errno));
            }
            else {
                if (fcntl(fd, F_SETOWN, getpid()) == -1) {
                    xf86Msg(X_WARNING, "fcntl(%d, F_SETOWN): %s\n",
                            fd, strerror(errno));
                }
                else {
                    installed = TRUE;
                }
            }
#endif
#if defined(I_SETSIG) && defined(HAVE_ISASTREAM)
            /* System V Streams - used on Solaris for input devices */
            if (!installed && isastream(fd)) {
                if (ioctl(fd, I_SETSIG, S_INPUT | S_ERROR | S_HANGUP) == -1) {
                    xf86Msg(X_WARNING, "fcntl(%d, I_SETSIG): %s\n",
                            fd, strerror(errno));
                }
                else {
                    installed = TRUE;
                }
            }
#endif
            if (!installed) {
                release_sigio();
                return 0;
            }
            sigemptyset(&sa.sa_mask);
            sigaddset(&sa.sa_mask, SIGIO);
            sa.sa_flags = SA_RESTART;
            sa.sa_handler = xf86SIGIO;
            sigaction(SIGIO, &sa, &osa);
            xf86SigIOFuncs[i].fd = fd;
            xf86SigIOFuncs[i].closure = closure;
            xf86SigIOFuncs[i].f = f;
            if (i >= xf86SigIOMax)
                xf86SigIOMax = i + 1;
            xf86SigIOAdd(fd);
            release_sigio();
            return 1;
        }
        /* Allow overwriting of the closure and callback */
        else if (xf86SigIOFuncs[i].fd == fd) {
            xf86SigIOFuncs[i].closure = closure;
            xf86SigIOFuncs[i].f = f;
            return 1;
        }
    }
    return 0;
}

int
xf86RemoveSIGIOHandler(int fd)
{
    struct sigaction sa;
    struct sigaction osa;
    int i;
    int max;
    int ret;

    max = 0;
    ret = 0;
    for (i = 0; i < MAX_FUNCS; i++) {
        if (xf86SigIOFuncs[i].f) {
            if (xf86SigIOFuncs[i].fd == fd) {
                xf86SigIOFuncs[i].f = 0;
                xf86SigIOFuncs[i].fd = 0;
                xf86SigIOFuncs[i].closure = 0;
                xf86SigIORemove(fd);
                ret = 1;
            }
            else {
                max = i + 1;
            }
        }
    }
    if (ret) {
#ifdef O_ASYNC
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) & ~O_ASYNC);
#endif
#if defined(I_SETSIG) && defined(HAVE_ISASTREAM)
        if (isastream(fd)) {
            if (ioctl(fd, I_SETSIG, 0) == -1) {
                xf86Msg(X_WARNING, "fcntl(%d, I_SETSIG, 0): %s\n",
                        fd, strerror(errno));
            }
        }
#endif
        xf86SigIOMax = max;
        if (!max) {
            sigemptyset(&sa.sa_mask);
            sigaddset(&sa.sa_mask, SIGIO);
            sa.sa_flags = 0;
            sa.sa_handler = SIG_IGN;
            sigaction(SIGIO, &sa, &osa);
        }
    }
    return ret;
}
