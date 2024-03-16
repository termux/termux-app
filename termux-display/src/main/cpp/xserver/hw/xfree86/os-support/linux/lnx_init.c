/*
 * Copyright 1992 by Orest Zborowski <obz@Kodak.com>
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Orest Zborowski and David Wexelblat
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  Orest Zborowski
 * and David Wexelblat make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * OREST ZBOROWSKI AND DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL OREST ZBOROWSKI OR DAVID WEXELBLAT BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <X11/X.h>
#include <X11/Xmd.h>

#include "compiler.h"
#include "linux.h"

#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"

#include <sys/stat.h>
#ifdef HAVE_SYS_SYSMACROS_H
#include <sys/sysmacros.h>
#endif

#ifndef K_OFF
#define K_OFF 0x4
#endif

static Bool KeepTty = FALSE;
static int activeVT = -1;

static char vtname[11];
static struct termios tty_attr; /* tty state to restore */
static int tty_mode;            /* kbd mode to restore */

static void
drain_console(int fd, void *closure)
{
    errno = 0;
    if (tcflush(fd, TCIOFLUSH) == -1 && errno == EIO) {
        xf86SetConsoleHandler(NULL, NULL);
    }
}

static int
switch_to(int vt, const char *from)
{
    int ret;

    SYSCALL(ret = ioctl(xf86Info.consoleFd, VT_ACTIVATE, vt));
    if (ret < 0) {
        xf86Msg(X_WARNING, "%s: VT_ACTIVATE failed: %s\n", from, strerror(errno));
        return 0;
    }

    SYSCALL(ret = ioctl(xf86Info.consoleFd, VT_WAITACTIVE, vt));
    if (ret < 0) {
        xf86Msg(X_WARNING, "%s: VT_WAITACTIVE failed: %s\n", from, strerror(errno));
        return 0;
    }

    return 1;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"

int
linux_parse_vt_settings(int may_fail)
{
    int i, fd = -1, ret, current_vt = -1;
    struct vt_stat vts;
    struct stat st;
    MessageType from = X_PROBED;

    /* Only do this once */
    static int vt_settings_parsed = 0;

    if (vt_settings_parsed)
        return 1;

    /*
     * setup the virtual terminal manager
     */
    if (xf86Info.vtno != -1) {
        from = X_CMDLINE;
    }
    else {
        fd = open("/dev/tty0", O_WRONLY, 0);
        if (fd < 0) {
            if (may_fail)
                return 0;
            FatalError("parse_vt_settings: Cannot open /dev/tty0 (%s)\n",
                       strerror(errno));
        }

        if (xf86Info.ShareVTs) {
            SYSCALL(ret = ioctl(fd, VT_GETSTATE, &vts));
            if (ret < 0) {
                if (may_fail)
                    return 0;
                FatalError("parse_vt_settings: Cannot find the current"
                           " VT (%s)\n", strerror(errno));
            }
            xf86Info.vtno = vts.v_active;
        }
        else {
            SYSCALL(ret = ioctl(fd, VT_OPENQRY, &xf86Info.vtno));
            if (ret < 0) {
                if (may_fail)
                    return 0;
                FatalError("parse_vt_settings: Cannot find a free VT: "
                           "%s\n", strerror(errno));
            }
            if (xf86Info.vtno == -1) {
                if (may_fail)
                    return 0;
                FatalError("parse_vt_settings: Cannot find a free VT\n");
            }
        }
        close(fd);
    }

    xf86Msg(from, "using VT number %d\n\n", xf86Info.vtno);

    /* Some of stdin / stdout / stderr maybe redirected to a file */
    for (i = STDIN_FILENO; i <= STDERR_FILENO; i++) {
        ret = fstat(i, &st);
        if (ret == 0 && S_ISCHR(st.st_mode) && major(st.st_rdev) == 4) {
            current_vt = minor(st.st_rdev);
            break;
        }
    }

    if (!KeepTty && current_vt == xf86Info.vtno) {
        xf86Msg(X_PROBED,
                "controlling tty is VT number %d, auto-enabling KeepTty\n",
                current_vt);
        KeepTty = TRUE;
    }

    vt_settings_parsed = 1;
    return 1;
}

int
linux_get_keeptty(void)
{
    return KeepTty;
}

void
xf86OpenConsole(void)
{
    int i, ret;
    struct vt_stat vts;
    struct vt_mode VT;
    const char *vcs[] = { "/dev/vc/%d", "/dev/tty%d", NULL };

    if (serverGeneration == 1) {
        linux_parse_vt_settings(FALSE);

        if (!KeepTty) {
            pid_t ppid = getppid();
            pid_t ppgid;

            ppgid = getpgid(ppid);

            /*
             * change to parent process group that pgid != pid so
             * that setsid() doesn't fail and we become process
             * group leader
             */
            if (setpgid(0, ppgid) < 0)
                xf86Msg(X_WARNING, "xf86OpenConsole: setpgid failed: %s\n",
                        strerror(errno));

            /* become process group leader */
            if ((setsid() < 0))
                xf86Msg(X_WARNING, "xf86OpenConsole: setsid failed: %s\n",
                        strerror(errno));
        }

        i = 0;
        while (vcs[i] != NULL) {
            snprintf(vtname, sizeof(vtname), vcs[i], xf86Info.vtno);    /* /dev/tty1-64 */
            if ((xf86Info.consoleFd = open(vtname, O_RDWR | O_NDELAY, 0)) >= 0)
                break;
            i++;
        }

        if (xf86Info.consoleFd < 0)
            FatalError("xf86OpenConsole: Cannot open virtual console"
                       " %d (%s)\n", xf86Info.vtno, strerror(errno));

        /*
         * Linux doesn't switch to an active vt after the last close of a vt,
         * so we do this ourselves by remembering which is active now.
         */
        SYSCALL(ret = ioctl(xf86Info.consoleFd, VT_GETSTATE, &vts));
        if (ret < 0)
            xf86Msg(X_WARNING, "xf86OpenConsole: VT_GETSTATE failed: %s\n",
                    strerror(errno));
        else
            activeVT = vts.v_active;

        if (!xf86Info.ShareVTs) {
            struct termios nTty;

            /*
             * now get the VT.  This _must_ succeed, or else fail completely.
             */
            if (!switch_to(xf86Info.vtno, "xf86OpenConsole"))
                FatalError("xf86OpenConsole: Switching VT failed\n");

            SYSCALL(ret = ioctl(xf86Info.consoleFd, VT_GETMODE, &VT));
            if (ret < 0)
                FatalError("xf86OpenConsole: VT_GETMODE failed %s\n",
                           strerror(errno));

            OsSignal(SIGUSR1, xf86VTRequest);

            VT.mode = VT_PROCESS;
            VT.relsig = SIGUSR1;
            VT.acqsig = SIGUSR1;

            SYSCALL(ret = ioctl(xf86Info.consoleFd, VT_SETMODE, &VT));
            if (ret < 0)
                FatalError
                    ("xf86OpenConsole: VT_SETMODE VT_PROCESS failed: %s\n",
                     strerror(errno));

            SYSCALL(ret = ioctl(xf86Info.consoleFd, KDSETMODE, KD_GRAPHICS));
            if (ret < 0)
                FatalError("xf86OpenConsole: KDSETMODE KD_GRAPHICS failed %s\n",
                           strerror(errno));

            tcgetattr(xf86Info.consoleFd, &tty_attr);
            SYSCALL(ioctl(xf86Info.consoleFd, KDGKBMODE, &tty_mode));

            /* disable kernel special keys and buffering */
            SYSCALL(ret = ioctl(xf86Info.consoleFd, KDSKBMODE, K_OFF));
            if (ret < 0)
            {
                /* fine, just disable special keys */
                SYSCALL(ret = ioctl(xf86Info.consoleFd, KDSKBMODE, K_RAW));
                if (ret < 0)
                    FatalError("xf86OpenConsole: KDSKBMODE K_RAW failed %s\n",
                               strerror(errno));

                /* ... and drain events, else the kernel gets angry */
                xf86SetConsoleHandler(drain_console, NULL);
            }

            nTty = tty_attr;
            nTty.c_iflag = (IGNPAR | IGNBRK) & (~PARMRK) & (~ISTRIP);
            nTty.c_oflag = 0;
            nTty.c_cflag = CREAD | CS8;
            nTty.c_lflag = 0;
            nTty.c_cc[VTIME] = 0;
            nTty.c_cc[VMIN] = 1;
            cfsetispeed(&nTty, 9600);
            cfsetospeed(&nTty, 9600);
            tcsetattr(xf86Info.consoleFd, TCSANOW, &nTty);
        }
    }
    else {                      /* serverGeneration != 1 */
        if (!xf86Info.ShareVTs && xf86Info.autoVTSwitch) {
            /* now get the VT */
            if (!switch_to(xf86Info.vtno, "xf86OpenConsole"))
                FatalError("xf86OpenConsole: Switching VT failed\n");
        }
    }
}

#pragma GCC diagnostic pop

void
xf86CloseConsole(void)
{
    struct vt_mode VT;
    struct vt_stat vts;
    int ret;

    if (xf86Info.ShareVTs) {
        close(xf86Info.consoleFd);
        return;
    }

    /*
     * unregister the drain_console handler
     * - what to do if someone else changed it in the meantime?
     */
    xf86SetConsoleHandler(NULL, NULL);

    /* Back to text mode ... */
    SYSCALL(ret = ioctl(xf86Info.consoleFd, KDSETMODE, KD_TEXT));
    if (ret < 0)
        xf86Msg(X_WARNING, "xf86CloseConsole: KDSETMODE failed: %s\n",
                strerror(errno));

    SYSCALL(ioctl(xf86Info.consoleFd, KDSKBMODE, tty_mode));
    tcsetattr(xf86Info.consoleFd, TCSANOW, &tty_attr);

    SYSCALL(ret = ioctl(xf86Info.consoleFd, VT_GETMODE, &VT));
    if (ret < 0)
        xf86Msg(X_WARNING, "xf86CloseConsole: VT_GETMODE failed: %s\n",
                strerror(errno));
    else {
        /* set dflt vt handling */
        VT.mode = VT_AUTO;
        SYSCALL(ret = ioctl(xf86Info.consoleFd, VT_SETMODE, &VT));
        if (ret < 0)
            xf86Msg(X_WARNING, "xf86CloseConsole: VT_SETMODE failed: %s\n",
                    strerror(errno));
    }

    if (xf86Info.autoVTSwitch) {
        /*
        * Perform a switch back to the active VT when we were started if our
        * vt is active now.
        */
        if (activeVT >= 0) {
            SYSCALL(ret = ioctl(xf86Info.consoleFd, VT_GETSTATE, &vts));
            if (ret < 0) {
                xf86Msg(X_WARNING, "xf86OpenConsole: VT_GETSTATE failed: %s\n",
                        strerror(errno));
            } else {
                if (vts.v_active == xf86Info.vtno) {
                    switch_to(activeVT, "xf86CloseConsole");
                }
            }
            activeVT = -1;
        }
    }
    close(xf86Info.consoleFd);  /* make the vt-manager happy */
}

#define CHECK_FOR_REQUIRED_ARGUMENT() \
    if (((i + 1) >= argc) || (!argv[i + 1])) { 				\
      ErrorF("Required argument to %s not specified\n", argv[i]); 	\
      UseMsg(); 							\
      FatalError("Required argument to %s not specified\n", argv[i]);	\
    }

int
xf86ProcessArgument(int argc, char *argv[], int i)
{
    /*
     * Keep server from detaching from controlling tty.  This is useful
     * when debugging (so the server can receive keyboard signals.
     */
    if (!strcmp(argv[i], "-keeptty")) {
        KeepTty = TRUE;
        return 1;
    }

    if ((argv[i][0] == 'v') && (argv[i][1] == 't')) {
        if (sscanf(argv[i], "vt%2d", &xf86Info.vtno) == 0) {
            UseMsg();
            xf86Info.vtno = -1;
            return 0;
        }
        return 1;
    }

    if (!strcmp(argv[i], "-masterfd")) {
        CHECK_FOR_REQUIRED_ARGUMENT();
        if (xf86PrivsElevated())
            FatalError("\nCannot specify -masterfd when server is setuid/setgid\n");
        if (sscanf(argv[++i], "%d", &xf86DRMMasterFd) != 1) {
            UseMsg();
            xf86DRMMasterFd = -1;
            return 0;
        }
        return 2;
    }

    return 0;
}

void
xf86UseMsg(void)
{
    ErrorF("vtXX                   use the specified VT number\n");
    ErrorF("-keeptty               ");
    ErrorF("don't detach controlling tty (for debugging only)\n");
    ErrorF("-masterfd <fd>         use the specified fd as the DRM master fd (not if setuid/gid)\n");
}

void
xf86OSInputThreadInit(void)
{
    return;
}
