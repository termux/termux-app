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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "xf86.h"
#include "xf86Priv.h"

#include <errno.h>
#include <sys/mman.h>

#include "xf86_OSlib.h"
#include "xf86OSpriv.h"

#if defined(__NetBSD__) && !defined(MAP_FILE)
#define MAP_FLAGS MAP_SHARED
#else
#define MAP_FLAGS (MAP_FILE | MAP_SHARED)
#endif

#ifdef __OpenBSD__
#define SYSCTL_MSG "\tCheck that you have set 'machdep.allowaperture=1'\n"\
		   "\tin /etc/sysctl.conf and reboot your machine\n" \
		   "\trefer to xf86(4) for details"
#define SYSCTL_MSG2 \
		"Check that you have set 'machdep.allowaperture=2'\n" \
		"\tin /etc/sysctl.conf and reboot your machine\n" \
		"\trefer to xf86(4) for details"
#endif

/***************************************************************************/
/* Video Memory Mapping section                                            */
/***************************************************************************/

static Bool useDevMem = FALSE;
static int devMemFd = -1;

#ifdef HAS_APERTURE_DRV
#define DEV_APERTURE "/dev/xf86"
#endif

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
                    MAP_FLAGS, fd, (off_t) 0xA0000);

        if (base != MAP_FAILED) {
            munmap((caddr_t) base, 4096);
            devMemFd = fd;
            useDevMem = TRUE;
            return;
        }
        else {
            /* This should not happen */
            if (warn) {
                xf86Msg(X_WARNING, "checkDevMem: failed to mmap %s (%s)\n",
                        DEV_MEM, strerror(errno));
            }
            useDevMem = FALSE;
            return;
        }
    }
#ifndef HAS_APERTURE_DRV
    if (warn) {
        xf86Msg(X_WARNING, "checkDevMem: failed to open %s (%s)\n",
                DEV_MEM, strerror(errno));
    }
    useDevMem = FALSE;
    return;
#else
    /* Failed to open /dev/mem, try the aperture driver */
    if ((fd = open(DEV_APERTURE, O_RDWR)) >= 0) {
        /* Try to map a page at the VGA address */
        base = mmap((caddr_t) 0, 4096, PROT_READ | PROT_WRITE,
                    MAP_FLAGS, fd, (off_t) 0xA0000);

        if (base != MAP_FAILED) {
            munmap((caddr_t) base, 4096);
            devMemFd = fd;
            useDevMem = TRUE;
            xf86Msg(X_INFO, "checkDevMem: using aperture driver %s\n",
                    DEV_APERTURE);
            return;
        }
        else {

            if (warn) {
                xf86Msg(X_WARNING, "checkDevMem: failed to mmap %s (%s)\n",
                        DEV_APERTURE, strerror(errno));
            }
        }
    }
    else {
        if (warn) {
#ifndef __OpenBSD__
            xf86Msg(X_WARNING, "checkDevMem: failed to open %s and %s\n"
                    "\t(%s)\n", DEV_MEM, DEV_APERTURE, strerror(errno));
#else                           /* __OpenBSD__ */
            xf86Msg(X_WARNING, "checkDevMem: failed to open %s and %s\n"
                    "\t(%s)\n%s", DEV_MEM, DEV_APERTURE, strerror(errno),
                    SYSCTL_MSG);
#endif                          /* __OpenBSD__ */
        }
    }

    useDevMem = FALSE;
    return;

#endif
}

void
xf86OSInitVidMem(VidMemInfoPtr pVidMem)
{
    checkDevMem(TRUE);

    pci_system_init_dev_mem(devMemFd);

    pVidMem->initialised = TRUE;
}

#ifdef USE_I386_IOPL
/***************************************************************************/
/* I/O Permissions section                                                 */
/***************************************************************************/

static Bool ExtendedEnabled = FALSE;

Bool
xf86EnableIO()
{
    if (ExtendedEnabled)
        return TRUE;

    if (i386_iopl(TRUE) < 0) {
#ifndef __OpenBSD__
        xf86Msg(X_WARNING, "%s: Failed to set IOPL for extended I/O",
                "xf86EnableIO");
#else
        xf86Msg(X_WARNING, "%s: Failed to set IOPL for extended I/O\n%s",
                "xf86EnableIO", SYSCTL_MSG);
#endif
        return FALSE;
    }
    ExtendedEnabled = TRUE;

    return TRUE;
}

void
xf86DisableIO()
{
    if (!ExtendedEnabled)
        return;

    i386_iopl(FALSE);
    ExtendedEnabled = FALSE;

    return;
}

#endif                          /* USE_I386_IOPL */

#ifdef USE_AMD64_IOPL
/***************************************************************************/
/* I/O Permissions section                                                 */
/***************************************************************************/

static Bool ExtendedEnabled = FALSE;

Bool
xf86EnableIO()
{
    if (ExtendedEnabled)
        return TRUE;

    if (amd64_iopl(TRUE) < 0) {
#ifndef __OpenBSD__
        xf86Msg(X_WARNING, "%s: Failed to set IOPL for extended I/O",
                "xf86EnableIO");
#else
        xf86Msg(X_WARNING, "%s: Failed to set IOPL for extended I/O\n%s",
                "xf86EnableIO", SYSCTL_MSG);
#endif
        return FALSE;
    }
    ExtendedEnabled = TRUE;

    return TRUE;
}

void
xf86DisableIO()
{
    if (!ExtendedEnabled)
        return;

    if (amd64_iopl(FALSE) == 0) {
        ExtendedEnabled = FALSE;
    }
    /* Otherwise, the X server has revoqued its root uid,
       and thus cannot give up IO privileges any more */

    return;
}

#endif                          /* USE_AMD64_IOPL */

#ifdef USE_DEV_IO
static int IoFd = -1;

Bool
xf86EnableIO()
{
    if (IoFd >= 0)
        return TRUE;

    if ((IoFd = open("/dev/io", O_RDWR)) == -1) {
        xf86Msg(X_WARNING, "xf86EnableIO: "
                "Failed to open /dev/io for extended I/O");
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

#ifdef __NetBSD__
/***************************************************************************/
/* Set TV output mode                                                      */
/***************************************************************************/
void
xf86SetTVOut(int mode)
{
    switch (xf86Info.consType) {
#ifdef PCCONS_SUPPORT
    case PCCONS:{

        if (ioctl(xf86Info.consoleFd, CONSOLE_X_TV_ON, &mode) < 0) {
            xf86Msg(X_WARNING,
                    "xf86SetTVOut: Could not set console to TV output, %s\n",
                    strerror(errno));
        }
    }
        break;
#endif                          /* PCCONS_SUPPORT */

    default:
        FatalError("Xf86SetTVOut: Unsupported console");
        break;
    }
    return;
}

void
xf86SetRGBOut()
{
    switch (xf86Info.consType) {
#ifdef PCCONS_SUPPORT
    case PCCONS:{

        if (ioctl(xf86Info.consoleFd, CONSOLE_X_TV_OFF, 0) < 0) {
            xf86Msg(X_WARNING,
                    "xf86SetTVOut: Could not set console to RGB output, %s\n",
                    strerror(errno));
        }
    }
        break;
#endif                          /* PCCONS_SUPPORT */

    default:
        FatalError("Xf86SetTVOut: Unsupported console");
        break;
    }
    return;
}
#endif
