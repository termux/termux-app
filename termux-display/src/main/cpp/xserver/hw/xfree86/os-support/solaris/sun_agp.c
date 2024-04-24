/*
 * Abstraction of the AGP GART interface.
 *
 * This version is for Solaris.
 *
 * Copyright © 2000 VA Linux Systems, Inc.
 * Copyright © 2001 The XFree86 Project, Inc.
 */
/* Copyright (c) 2005, Oracle and/or its affiliates. All rights reserved.
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
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "xf86_OSproc.h"
#include <unistd.h>
#include <sys/ioccom.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/agpgart.h>

/* AGP page size is independent of the host page size. */
#ifndef	AGP_PAGE_SIZE
#define	AGP_PAGE_SIZE		4096
#endif

static int gartFd = -1;
static int acquiredScreen = -1;
static Bool initDone = FALSE;

/*
 * Close /dev/agpgart.  This frees all associated memory allocated during
 * this server generation.
 */
Bool
xf86GARTCloseScreen(int screenNum)
{
    if (gartFd != -1) {
        close(gartFd);
        acquiredScreen = -1;
        gartFd = -1;
        initDone = FALSE;

        xf86DrvMsg(screenNum, X_INFO,
                   "xf86GARTCloseScreen: device closed successfully\n");

    }
    return TRUE;
}

/*
 * Open /dev/agpgart.  Keep it open until xf86GARTCloseScreen is called.
 */
static Bool
GARTInit(int screenNum)
{
    if (initDone)
        return gartFd != -1;

    if (gartFd == -1)
        gartFd = open(AGP_DEVICE, O_RDWR);
    else
        return FALSE;

    if (gartFd == -1) {
        xf86DrvMsg(screenNum, X_ERROR,
                   "GARTInit: Unable to open " AGP_DEVICE " (%s)\n",
                   strerror(errno));
        return FALSE;
    }

    initDone = TRUE;
    xf86DrvMsg(screenNum, X_INFO,
               "GARTInit: " AGP_DEVICE " opened successfully\n");

    return TRUE;
}

Bool
xf86AgpGARTSupported(void)
{
    return (GARTInit(-1));

}

AgpInfoPtr
xf86GetAGPInfo(int screenNum)
{
    agp_info_t agpinf;
    AgpInfoPtr info;

    if (!GARTInit(screenNum))
        return NULL;

    if (ioctl(gartFd, AGPIOC_INFO, &agpinf) != 0) {
        xf86DrvMsg(screenNum, X_ERROR,
                   "xf86GetAGPInfo: AGPIOC_INFO failed (%s)\n",
                   strerror(errno));
        return NULL;
    }

    if ((info = calloc(sizeof(AgpInfo), 1)) == NULL) {
        xf86DrvMsg(screenNum, X_ERROR,
                   "xf86GetAGPInfo: Failed to allocate AgpInfo\n");
        return NULL;
    }

    info->bridgeId = agpinf.agpi_devid;
    info->agpMode = agpinf.agpi_mode;
    info->base = agpinf.agpi_aperbase;
    info->size = agpinf.agpi_apersize;
    info->totalPages = (unsigned long) agpinf.agpi_pgtotal;
    info->systemPages = (unsigned long) agpinf.agpi_pgsystem;
    info->usedPages = (unsigned long) agpinf.agpi_pgused;

    return info;
}

Bool
xf86AcquireGART(int screenNum)
{

    if (!GARTInit(screenNum))
        return FALSE;

    if (acquiredScreen != screenNum) {
        if (ioctl(gartFd, AGPIOC_ACQUIRE, 0) != 0) {
            xf86DrvMsg(screenNum, X_WARNING,
                       "xf86AcquireGART: AGPIOC_ACQUIRE failed (%s)\n",
                       strerror(errno));
            return FALSE;
        }
        acquiredScreen = screenNum;
        xf86DrvMsg(screenNum, X_INFO,
                   "xf86AcquireGART: AGPIOC_ACQUIRE succeeded\n");
    }
    return TRUE;
}

Bool
xf86ReleaseGART(int screenNum)
{

    if (!GARTInit(screenNum))
        return FALSE;

    if (acquiredScreen == screenNum) {
        /*
         * The FreeBSD agp driver removes allocations on release.
         * The Solaris driver doesn't.  xf86ReleaseGART() is expected
         * to give up access to the GART, but not to remove any
         * allocations.
         */

        if (ioctl(gartFd, AGPIOC_RELEASE, 0) != 0) {
            xf86DrvMsg(screenNum, X_WARNING,
                       "xf86ReleaseGART: AGPIOC_RELEASE failed (%s)\n",
                       strerror(errno));
            return FALSE;
        }
        acquiredScreen = -1;
        xf86DrvMsg(screenNum, X_INFO,
                   "xf86ReleaseGART: AGPIOC_RELEASE succeeded\n");
        return TRUE;
    }
    return FALSE;
}

int
xf86AllocateGARTMemory(int screenNum, unsigned long size, int type,
                       unsigned long *physical)
{
    agp_allocate_t alloc;
    int pages;

    /*
     * Allocates "size" bytes of GART memory (rounds up to the next
     * page multiple) or type "type".  A handle (key) for the allocated
     * memory is returned.  On error, the return value is -1.
     * "size" should be larger than 0, or AGPIOC_ALLOCATE ioctl will
     * return error.
     */

    if (!GARTInit(screenNum) || (acquiredScreen != screenNum))
        return -1;

    pages = (size / AGP_PAGE_SIZE);
    if (size % AGP_PAGE_SIZE != 0)
        pages++;

    alloc.agpa_pgcount = pages;
    alloc.agpa_type = type;

    if (ioctl(gartFd, AGPIOC_ALLOCATE, &alloc) != 0) {
        xf86DrvMsg(screenNum, X_WARNING, "xf86AllocateGARTMemory: "
                   "allocation of %d pages failed\n\t(%s)\n", pages,
                   strerror(errno));
        return -1;
    }

    if (physical)
        *physical = (unsigned long) alloc.agpa_physical;

    return alloc.agpa_key;
}

Bool
xf86DeallocateGARTMemory(int screenNum, int key)
{
    if (!GARTInit(screenNum) || (acquiredScreen != screenNum))
        return FALSE;

    if (ioctl(gartFd, AGPIOC_DEALLOCATE, (int *) (uintptr_t) key) != 0) {
        xf86DrvMsg(screenNum, X_WARNING, "xf86DeAllocateGARTMemory: "
                   "deallocation of gart memory with key %d failed\n"
                   "\t(%s)\n", key, strerror(errno));
        return FALSE;
    }

    return TRUE;
}

/* Bind GART memory with "key" at "offset" */
Bool
xf86BindGARTMemory(int screenNum, int key, unsigned long offset)
{
    agp_bind_t bind;
    int pageOffset;

    if (!GARTInit(screenNum) || (acquiredScreen != screenNum))
        return FALSE;

    if (offset % AGP_PAGE_SIZE != 0) {
        xf86DrvMsg(screenNum, X_WARNING, "xf86BindGARTMemory: "
                   "offset (0x%lx) is not page-aligned (%d)\n",
                   offset, AGP_PAGE_SIZE);
        return FALSE;
    }
    pageOffset = offset / AGP_PAGE_SIZE;

    xf86DrvMsgVerb(screenNum, X_INFO, 3,
                   "xf86BindGARTMemory: bind key %d at 0x%08lx "
                   "(pgoffset %d)\n", key, offset, pageOffset);

    bind.agpb_pgstart = pageOffset;
    bind.agpb_key = key;

    if (ioctl(gartFd, AGPIOC_BIND, &bind) != 0) {
        xf86DrvMsg(screenNum, X_WARNING, "xf86BindGARTMemory: "
                   "binding of gart memory with key %d\n"
                   "\tat offset 0x%lx failed (%s)\n",
                   key, offset, strerror(errno));
        return FALSE;
    }

    return TRUE;
}

/* Unbind GART memory with "key" */
Bool
xf86UnbindGARTMemory(int screenNum, int key)
{
    agp_unbind_t unbind;

    if (!GARTInit(screenNum) || (acquiredScreen != screenNum))
        return FALSE;

    unbind.agpu_pri = 0;
    unbind.agpu_key = key;

    if (ioctl(gartFd, AGPIOC_UNBIND, &unbind) != 0) {
        xf86DrvMsg(screenNum, X_WARNING, "xf86UnbindGARTMemory: "
                   "unbinding of gart memory with key %d "
                   "failed (%s)\n", key, strerror(errno));
        return FALSE;
    }

    xf86DrvMsgVerb(screenNum, X_INFO, 3,
                   "xf86UnbindGARTMemory: unbind key %d\n", key);

    return TRUE;
}

/* XXX Interface may change. */
Bool
xf86EnableAGP(int screenNum, CARD32 mode)
{
    agp_setup_t setup;

    if (!GARTInit(screenNum) || (acquiredScreen != screenNum))
        return FALSE;

    setup.agps_mode = mode;
    if (ioctl(gartFd, AGPIOC_SETUP, &setup) != 0) {
        xf86DrvMsg(screenNum, X_WARNING, "xf86EnableAGP: "
                   "AGPIOC_SETUP with mode %x failed (%s)\n",
                   (unsigned int) mode, strerror(errno));
        return FALSE;
    }

    return TRUE;
}
