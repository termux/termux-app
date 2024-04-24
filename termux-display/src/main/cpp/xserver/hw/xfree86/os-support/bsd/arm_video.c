/*
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Rich Murphey and David Wexelblat
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  Rich Murphey and
 * David Wexelblat make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * RICH MURPHEY AND DAVID WEXELBLAT DISCLAIM ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RICH MURPHEY OR DAVID WEXELBLAT BE LIABLE FOR
 * ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/*
 * The ARM32 code here carries the following copyright:
 *
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in
 * accordance with the following terms and conditions.  Subject to these
 * conditions, you may download, copy, install, use, modify and distribute
 * this software in source and/or binary form. No title or ownership is
 * transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce and retain
 *    this copyright notice and list of conditions as they appear in the
 *    source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital
 *    Equipment Corporation. Neither the "Digital Equipment Corporation"
 *    name nor any trademark or logo of Digital Equipment Corporation may be
 *    used to endorse or promote products derived from this software without
 *    the prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties,
 *    including but not limited to, any implied warranties of merchantability,
 *    fitness for a particular purpose, or non-infringement are disclaimed.
 *    In no event shall DIGITAL be liable for any damages whatsoever, and in
 *    particular, DIGITAL shall not be liable for special, indirect,
 *    consequential, or incidental damages or damages for lost profits, loss
 *    of revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise, even
 *    if advised of the possibility of such damage.
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86OSpriv.h"
#include "compiler.h"

#if defined(__NetBSD__) && !defined(MAP_FILE)
#define MAP_FLAGS MAP_SHARED
#else
#define MAP_FLAGS (MAP_FILE | MAP_SHARED)
#endif

#define BUS_BASE	0L
#define BUS_BASE_BWX	0L

/***************************************************************************/
/* Video Memory Mapping section                                            */
/***************************************************************************/

static int devMemFd = -1;

/*
 * Check if /dev/mem can be mmap'd.  If it can't print a warning when
 * "warn" is TRUE.
 */
static void
checkDevMem(Bool warn)
{
    static Bool devMemChecked = FALSE;
    int fd;
    void *base;

    if (devMemChecked)
        return;
    devMemChecked = TRUE;

    if ((fd = open(DEV_MEM, O_RDWR)) >= 0) {
        /* Try to map a page at the VGA address */
        base = mmap((caddr_t) 0, 4096, PROT_READ | PROT_WRITE,
                    MAP_FLAGS, fd, (off_t) 0xA0000 + BUS_BASE);

        if (base != MAP_FAILED) {
            munmap((caddr_t) base, 4096);
            devMemFd = fd;
            return;
        }
        else {
            /* This should not happen */
            if (warn) {
                xf86Msg(X_WARNING, "checkDevMem: failed to mmap %s (%s)\n",
                        DEV_MEM, strerror(errno));
            }
            return;
        }
    }
    if (warn) {
        xf86Msg(X_WARNING, "checkDevMem: failed to open %s (%s)\n",
                DEV_MEM, strerror(errno));
    }
    return;
}

void
xf86OSInitVidMem(VidMemInfoPtr pVidMem)
{
    checkDevMem(TRUE);

    pVidMem->initialised = TRUE;
}

#ifdef USE_DEV_IO
static int IoFd = -1;

Bool
xf86EnableIO()
{
    if (IoFd >= 0)
        return TRUE;

    if ((IoFd = open("/dev/io", O_RDWR)) == -1) {
        xf86Msg(X_WARNING, "xf86EnableIO: "
                "Failed to open /dev/io for extended I/O\n");
        return FALSE;
    }
    return TRUE;
}

void
xf86DisableIO()
{
    if (IoFd < 0)
        return;

    close(IoFd);
    IoFd = -1;
    return;
}

#endif

#if defined(USE_ARC_MMAP) || defined(__arm32__)

unsigned int IOPortBase;

Bool
xf86EnableIO()
{
    int fd;
    void *base;

    if (ExtendedEnabled)
        return TRUE;

    if ((fd = open("/dev/ttyC0", O_RDWR)) >= 0) {
        /* Try to map a page at the pccons I/O space */
        base = (void *) mmap((caddr_t) 0, 65536, PROT_READ | PROT_WRITE,
                             MAP_FLAGS, fd, (off_t) 0x0000);

        if (base != (void *) -1) {
            IOPortBase = base;
        }
        else {
            xf86Msg(X_WARNING, "EnableIO: failed to mmap %s (%s)\n",
                    "/dev/ttyC0", strerror(errno));
            return FALSE;
        }
    }
    else {
        xf86Msg("EnableIO: failed to open %s (%s)\n",
                "/dev/ttyC0", strerror(errno));
        return FALSE;
    }

    ExtendedEnabled = TRUE;

    return TRUE;
}

void
xf86DisableIO()
{
    return;
}

#endif                          /* USE_ARC_MMAP */
