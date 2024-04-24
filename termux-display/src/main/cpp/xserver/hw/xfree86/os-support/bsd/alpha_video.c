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

#include <sys/param.h>
#ifndef __NetBSD__
#include <sys/sysctl.h>
#endif

#include "xf86_OSlib.h"
#include "xf86OSpriv.h"

#if defined(__NetBSD__) && !defined(MAP_FILE)
#define MAP_FLAGS MAP_SHARED
#else
#define MAP_FLAGS (MAP_FILE | MAP_SHARED)
#endif

#ifndef __NetBSD__
extern unsigned long dense_base(void);
#else                           /* __NetBSD__ */
static struct alpha_bus_window *abw;
static int abw_count = -1;

static void
init_abw(void)
{
    if (abw_count < 0) {
        abw_count = alpha_bus_getwindows(ALPHA_BUS_TYPE_PCI_MEM, &abw);
        if (abw_count <= 0)
            FatalError("init_abw: alpha_bus_getwindows failed\n");
    }
}

static unsigned long
dense_base(void)
{
    if (abw_count < 0)
        init_abw();

    /* XXX check abst_flags for ABST_DENSE just to be safe? */
    xf86Msg(X_INFO, "dense base = %#lx\n", abw[0].abw_abst.abst_sys_start);     /* XXXX */
    return abw[0].abw_abst.abst_sys_start;
}

#endif                          /* __NetBSD__ */

#define BUS_BASE	dense_base()

/***************************************************************************/
/* Video Memory Mapping section                                            */
/***************************************************************************/

#ifdef __OpenBSD__
#define SYSCTL_MSG "\tCheck that you have set 'machdep.allowaperture=1'\n"\
                  "\tin /etc/sysctl.conf and reboot your machine\n" \
                  "\trefer to xf86(4) for details"
#endif

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

#ifdef HAS_APERTURE_DRV
    /* Try the aperture driver first */
    if ((fd = open(DEV_APERTURE, O_RDWR)) >= 0) {
        /* Try to map a page at the VGA address */
        base = mmap((caddr_t) 0, 4096, PROT_READ | PROT_WRITE,
                    MAP_FLAGS, fd, (off_t) 0xA0000 + BUS_BASE);

        if (base != MAP_FAILED) {
            munmap((caddr_t) base, 4096);
            devMemFd = fd;
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
#endif
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
            if (warn) {
                xf86Msg(X_WARNING, "checkDevMem: failed to mmap %s (%s)\n",
                        DEV_MEM, strerror(errno));
            }
        }
    }
    if (warn) {
#ifndef HAS_APERTURE_DRV
        xf86Msg(X_WARNING, "checkDevMem: failed to open/mmap %s (%s)\n",
                DEV_MEM, strerror(errno));
#else
#ifndef __OpenBSD__
        xf86Msg(X_WARNING, "checkDevMem: failed to open %s and %s\n"
                "\t(%s)\n", DEV_APERTURE, DEV_MEM, strerror(errno));
#else                           /* __OpenBSD__ */
        xf86Msg(X_WARNING, "checkDevMem: failed to open %s and %s\n"
                "\t(%s)\n%s", DEV_APERTURE, DEV_MEM, strerror(errno),
                SYSCTL_MSG);
#endif                          /* __OpenBSD__ */
#endif
        xf86ErrorF("\tlinear framebuffer access unavailable\n");
    }
    return;
}

void
xf86OSInitVidMem(VidMemInfoPtr pVidMem)
{
    checkDevMem(TRUE);

    pVidMem->initialised = TRUE;
}

#if defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__)

extern int ioperm(unsigned long from, unsigned long num, int on);

Bool
xf86EnableIO()
{
    if (!ioperm(0, 65536, TRUE))
        return TRUE;
    return FALSE;
}

void
xf86DisableIO()
{
    return;
}

#endif                          /* __FreeBSD_kernel__ || __OpenBSD__ */

#ifdef USE_ALPHA_PIO

Bool
xf86EnableIO()
{
    alpha_pci_io_enable(1);
    return TRUE;
}

void
xf86DisableIO()
{
    alpha_pci_io_enable(0);
}

#endif                          /* USE_ALPHA_PIO */

extern int readDense8(void *Base, register unsigned long Offset);
extern int readDense16(void *Base, register unsigned long Offset);
extern int readDense32(void *Base, register unsigned long Offset);
extern void
 writeDense8(int Value, void *Base, register unsigned long Offset);
extern void
 writeDense16(int Value, void *Base, register unsigned long Offset);
extern void
 writeDense32(int Value, void *Base, register unsigned long Offset);

void (*xf86WriteMmio8) (int Value, void *Base, unsigned long Offset)
    = writeDense8;
void (*xf86WriteMmio16) (int Value, void *Base, unsigned long Offset)
    = writeDense16;
void (*xf86WriteMmio32) (int Value, void *Base, unsigned long Offset)
    = writeDense32;
int (*xf86ReadMmio8) (void *Base, unsigned long Offset)
    = readDense8;
int (*xf86ReadMmio16) (void *Base, unsigned long Offset)
    = readDense16;
int (*xf86ReadMmio32) (void *Base, unsigned long Offset)
    = readDense32;
