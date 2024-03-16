/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 * Copyright 1999 by David Holland <davidh@iquest.net>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the names of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, AND IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#ifdef HAVE_SYS_KD_H
#include <sys/kd.h>
#endif

/*
 * Applications see VT number as consecutive integers starting from 1.
 * VT number			VT device
 * -------------------------------------------------------
 *     1             :          /dev/vt/0 (Alt + Ctrl + F1)
 *     2             :          /dev/vt/2 (Alt + Ctrl + F2)
 *     3             :          /dev/vt/3 (Alt + Ctrl + F3)
 *  ... ...
 */
#define	CONSOLE_VTNO	1
#define	SOL_CONSOLE_DEV	"/dev/console"

static Bool KeepTty = FALSE;
static Bool UseConsole = FALSE;

#ifdef HAS_USL_VTS
static int VTnum = -1;
static int xf86StartVT = -1;
static int vtEnabled = 0;
#endif

/* Device to open as xf86Info.consoleFd */
static char consoleDev[PATH_MAX] = "/dev/fb";

/* Set by -dev argument on CLI
   Used by hw/xfree86/common/xf86AutoConfig.c for VIS_GETIDENTIFIER */
_X_HIDDEN char xf86SolarisFbDev[PATH_MAX] = "/dev/fb";

static void
switch_to(int vt, const char *from)
{
    int ret;

    SYSCALL(ret = ioctl(xf86Info.consoleFd, VT_ACTIVATE, vt));
    if (ret != 0)
        xf86Msg(X_WARNING, "%s: VT_ACTIVATE failed: %s\n",
                from, strerror(errno));

    SYSCALL(ret = ioctl(xf86Info.consoleFd, VT_WAITACTIVE, vt));
    if (ret != 0)
        xf86Msg(X_WARNING, "%s: VT_WAITACTIVE failed: %s\n",
                from, strerror(errno));
}

void
xf86OpenConsole(void)
{
    int i;

#ifdef HAS_USL_VTS
    int fd;
    struct vt_mode VT;
    struct vt_stat vtinfo;
    MessageType from = X_PROBED;
#endif

    if (serverGeneration == 1) {
        /* Check if we're run with euid==0 */
        if (geteuid() != 0)
            FatalError("xf86OpenConsole: Server must be suid root\n");

#ifdef HAS_USL_VTS

        /*
         * Setup the virtual terminal manager
         */
        if ((fd = open("/dev/vt/0", O_RDWR, 0)) == -1) {
            xf86ErrorF("xf86OpenConsole: Cannot open /dev/vt/0 (%s)\n",
                       strerror(errno));
            vtEnabled = 0;
        }
        else {
            if (ioctl(fd, VT_ENABLED, &vtEnabled) < 0) {
                xf86ErrorF("xf86OpenConsole: VT_ENABLED failed (%s)\n",
                           strerror(errno));
                vtEnabled = 0;
            }
        }
#endif                          /*  HAS_USL_VTS */

        if (UseConsole) {
            strlcpy(consoleDev, SOL_CONSOLE_DEV, sizeof(consoleDev));

#ifdef HAS_USL_VTS
            xf86Info.vtno = CONSOLE_VTNO;

            if (vtEnabled == 0) {
                xf86StartVT = 0;
            }
            else {
                if (ioctl(fd, VT_GETSTATE, &vtinfo) < 0)
                    FatalError
                        ("xf86OpenConsole: Cannot determine current VT\n");
                xf86StartVT = vtinfo.v_active;
            }
#endif                          /*  HAS_USL_VTS */
            goto OPENCONSOLE;
        }

#ifdef HAS_USL_VTS
        if (vtEnabled == 0) {
            /* VT not enabled - kernel too old or Sparc platforms
               without visual_io support */
            xf86Msg(from, "VT infrastructure is not available\n");

            xf86StartVT = 0;
            xf86Info.vtno = 0;
            strlcpy(consoleDev, xf86SolarisFbDev, sizeof(consoleDev));
            goto OPENCONSOLE;
        }

        if (ioctl(fd, VT_GETSTATE, &vtinfo) < 0)
            FatalError("xf86OpenConsole: Cannot determine current VT\n");

        xf86StartVT = vtinfo.v_active;

        if (VTnum != -1) {
            xf86Info.vtno = VTnum;
            from = X_CMDLINE;
        }
        else if (xf86Info.ShareVTs) {
            xf86Info.vtno = vtinfo.v_active;
            from = X_CMDLINE;
        }
        else {
            if ((ioctl(fd, VT_OPENQRY, &xf86Info.vtno) < 0) ||
                (xf86Info.vtno == -1)) {
                FatalError("xf86OpenConsole: Cannot find a free VT\n");
            }
        }

        xf86Msg(from, "using VT number %d\n\n", xf86Info.vtno);
        snprintf(consoleDev, PATH_MAX, "/dev/vt/%d", xf86Info.vtno);

        if (fd != -1) {
            close(fd);
        }

#endif                          /* HAS_USL_VTS */

 OPENCONSOLE:
        if (!KeepTty)
            setpgrp();

        if (((xf86Info.consoleFd = open(consoleDev, O_RDWR | O_NDELAY, 0)) < 0))
            FatalError("xf86OpenConsole: Cannot open %s (%s)\n",
                       consoleDev, strerror(errno));

        /* Change ownership of the vt or console */
        chown(consoleDev, getuid(), getgid());

#ifdef HAS_USL_VTS
        if (xf86Info.ShareVTs)
            return;

        if (vtEnabled) {
            /*
             * Now get the VT
             */
            switch_to(xf86Info.vtno, "xf86OpenConsole");

#ifdef VT_SET_CONSUSER          /* added in snv_139 */
            if (strcmp(display, "0") == 0)
                if (ioctl(xf86Info.consoleFd, VT_SET_CONSUSER) != 0)
                    xf86Msg(X_WARNING,
                            "xf86OpenConsole: VT_SET_CONSUSER failed\n");
#endif

            if (ioctl(xf86Info.consoleFd, VT_GETMODE, &VT) < 0)
                FatalError("xf86OpenConsole: VT_GETMODE failed\n");

            OsSignal(SIGUSR1, xf86VTAcquire);
            OsSignal(SIGUSR2, xf86VTRelease);

            VT.mode = VT_PROCESS;
            VT.acqsig = SIGUSR1;
            VT.relsig = SIGUSR2;

            if (ioctl(xf86Info.consoleFd, VT_SETMODE, &VT) < 0)
                FatalError("xf86OpenConsole: VT_SETMODE VT_PROCESS failed\n");

            if (ioctl(xf86Info.consoleFd, VT_SETDISPINFO, atoi(display)) < 0)
                xf86Msg(X_WARNING, "xf86OpenConsole: VT_SETDISPINFO failed\n");
        }
#endif

#ifdef KDSETMODE
        SYSCALL(i = ioctl(xf86Info.consoleFd, KDSETMODE, KD_GRAPHICS));
        if (i < 0) {
            xf86Msg(X_WARNING,
                    "xf86OpenConsole: KDSETMODE KD_GRAPHICS failed on %s (%s)\n",
                    consoleDev, strerror(errno));
        }
#endif
    }
    else {                      /* serverGeneration != 1 */

#ifdef HAS_USL_VTS
        if (vtEnabled && !xf86Info.ShareVTs) {
            /*
             * Now re-get the VT
             */
            if (xf86Info.autoVTSwitch)
                switch_to(xf86Info.vtno, "xf86OpenConsole");

#ifdef VT_SET_CONSUSER          /* added in snv_139 */
            if (strcmp(display, "0") == 0)
                if (ioctl(xf86Info.consoleFd, VT_SET_CONSUSER) != 0)
                    xf86Msg(X_WARNING,
                            "xf86OpenConsole: VT_SET_CONSUSER failed\n");
#endif

            /*
             * If the server doesn't have the VT when the reset occurs,
             * this is to make sure we don't continue until the activate
             * signal is received.
             */
            if (!xf86VTOwner())
                sleep(5);
        }
#endif                          /* HAS_USL_VTS */

    }
}

void
xf86CloseConsole(void)
{
#ifdef HAS_USL_VTS
    struct vt_mode VT;
#endif

#if !defined(__i386__) && !defined(__i386) && !defined(__x86)

    if (!xf86DoConfigure) {
        int fd;

        /*
         * Wipe out framebuffer just like the non-SI Xsun server does.  This
         * could be improved by saving framebuffer contents in
         * xf86OpenConsole() above and restoring them here.  Also, it's unclear
         * at this point whether this should be done for all framebuffers in
         * the system, rather than only the console.
         */
        if ((fd = open(xf86SolarisFbDev, O_RDWR, 0)) < 0) {
            xf86Msg(X_WARNING,
                    "xf86CloseConsole():  unable to open framebuffer (%s)\n",
                    strerror(errno));
        }
        else {
            struct fbgattr fbattr;

            if ((ioctl(fd, FBIOGATTR, &fbattr) < 0) &&
                (ioctl(fd, FBIOGTYPE, &fbattr.fbtype) < 0)) {
                xf86Msg(X_WARNING,
                        "xf86CloseConsole():  unable to retrieve framebuffer"
                        " attributes (%s)\n", strerror(errno));
            }
            else {
                void *fbdata;

                fbdata = mmap(NULL, fbattr.fbtype.fb_size,
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                if (fbdata == MAP_FAILED) {
                    xf86Msg(X_WARNING,
                            "xf86CloseConsole():  unable to mmap framebuffer"
                            " (%s)\n", strerror(errno));
                }
                else {
                    memset(fbdata, 0, fbattr.fbtype.fb_size);
                    munmap(fbdata, fbattr.fbtype.fb_size);
                }
            }

            close(fd);
        }
    }

#endif

#ifdef KDSETMODE
    /* Reset the display back to text mode */
    SYSCALL(ioctl(xf86Info.consoleFd, KDSETMODE, KD_TEXT));
#endif

#ifdef HAS_USL_VTS
    if (vtEnabled) {
        if (ioctl(xf86Info.consoleFd, VT_GETMODE, &VT) != -1) {
            VT.mode = VT_AUTO;  /* Set default vt handling */
            ioctl(xf86Info.consoleFd, VT_SETMODE, &VT);
        }

        /* Activate the VT that X was started on */
        if (xf86Info.autoVTSwitch)
            switch_to(xf86StartVT, "xf86CloseConsole");
    }
#endif                          /* HAS_USL_VTS */

    close(xf86Info.consoleFd);
}

int
xf86ProcessArgument(int argc, char **argv, int i)
{
    /*
     * Keep server from detaching from controlling tty.  This is useful when
     * debugging, so the server can receive keyboard signals.
     */
    if (!strcmp(argv[i], "-keeptty")) {
        KeepTty = TRUE;
        return 1;
    }

    /*
     * Use /dev/console as the console device.
     */
    if (!strcmp(argv[i], "-C")) {
        UseConsole = TRUE;
        return 1;
    }

#ifdef HAS_USL_VTS

    if ((argv[i][0] == 'v') && (argv[i][1] == 't')) {
        if (sscanf(argv[i], "vt%d", &VTnum) == 0) {
            UseMsg();
            VTnum = -1;
            return 0;
        }

        return 1;
    }

#endif                          /* HAS_USL_VTS */

    if ((i + 1) < argc) {
        if (!strcmp(argv[i], "-dev")) {
            strlcpy(xf86SolarisFbDev, argv[i + 1], sizeof(xf86SolarisFbDev));
            return 2;
        }
    }

    return 0;
}

void
xf86UseMsg(void)
{
#ifdef HAS_USL_VTS
    ErrorF("vtX                    Use the specified VT number\n");
#endif
    ErrorF("-dev <fb>              Framebuffer device\n");
    ErrorF("-keeptty               Don't detach controlling tty\n");
    ErrorF("                       (for debugging only)\n");
    ErrorF("-C                     Use /dev/console as the console device\n");
}
