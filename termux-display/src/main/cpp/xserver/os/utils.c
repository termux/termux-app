/*

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
Copyright 1994 Quarterdeck Office Systems.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Digital and
Quarterdeck not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

DIGITAL AND QUARTERDECK DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE
OR PERFORMANCE OF THIS SOFTWARE.

*/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifdef __CYGWIN__
#include <stdlib.h>
#include <signal.h>
/*
   Sigh... We really need a prototype for this to know it is stdcall,
   but #include-ing <windows.h> here is not a good idea...
*/
__stdcall unsigned long GetTickCount(void);
#endif

#if defined(WIN32) && !defined(__CYGWIN__)
#include <X11/Xwinsock.h>
#endif
#include <X11/Xos.h>
#include <stdio.h>
#include <time.h>
#if !defined(WIN32) || !defined(__MINGW32__)
#include <sys/time.h>
#include <sys/resource.h>
#endif
#include "misc.h"
#include <X11/X.h>
#define XSERV_t
#define TRANS_SERVER
#define TRANS_REOPEN
#include <X11/Xtrans/Xtrans.h>
#include "input.h"
#include "dixfont.h"
#include <X11/fonts/libxfont2.h>
#include "osdep.h"
#include "extension.h"
#include <signal.h>
#ifndef WIN32
#include <sys/wait.h>
#endif
#if !defined(SYSV) && !defined(WIN32)
#include <sys/resource.h>
#endif
#include <sys/stat.h>
#include <ctype.h>              /* for isspace */
#include <stdarg.h>

#include <stdlib.h>             /* for malloc() */

#if defined(TCPCONN)
#ifndef WIN32
#include <netdb.h>
#endif
#endif

#include "opaque.h"

#include "dixstruct.h"

#include "xkbsrv.h"

#include "picture.h"

#include "miinitext.h"

#include "present.h"

Bool noTestExtensions;

#ifdef COMPOSITE
Bool noCompositeExtension = FALSE;
#endif

#ifdef DAMAGE
Bool noDamageExtension = FALSE;
#endif
#ifdef DBE
Bool noDbeExtension = FALSE;
#endif
#ifdef DPMSExtension
#include "dpmsproc.h"
Bool noDPMSExtension = FALSE;
#endif
#ifdef GLXEXT
Bool noGlxExtension = FALSE;
#endif
#ifdef SCREENSAVER
Bool noScreenSaverExtension = FALSE;
#endif
#ifdef MITSHM
Bool noMITShmExtension = FALSE;
#endif
#ifdef RANDR
Bool noRRExtension = FALSE;
#endif
Bool noRenderExtension = FALSE;

#ifdef XCSECURITY
Bool noSecurityExtension = FALSE;
#endif
#ifdef RES
Bool noResExtension = FALSE;
#endif
#ifdef XF86BIGFONT
Bool noXFree86BigfontExtension = FALSE;
#endif
#ifdef XFreeXDGA
Bool noXFree86DGAExtension = FALSE;
#endif
#ifdef XF86DRI
Bool noXFree86DRIExtension = FALSE;
#endif
#ifdef XF86VIDMODE
Bool noXFree86VidModeExtension = FALSE;
#endif
Bool noXFixesExtension = FALSE;
#ifdef PANORAMIX
/* Xinerama is disabled by default unless enabled via +xinerama */
Bool noPanoramiXExtension = TRUE;
#endif
#ifdef XSELINUX
Bool noSELinuxExtension = FALSE;
int selinuxEnforcingState = SELINUX_MODE_DEFAULT;
#endif
#ifdef XV
Bool noXvExtension = FALSE;
#endif
#ifdef DRI2
Bool noDRI2Extension = FALSE;
#endif

Bool noGEExtension = FALSE;

#define X_INCLUDE_NETDB_H
#include <X11/Xos_r.h>

#include <errno.h>

Bool CoreDump;

Bool enableIndirectGLX = FALSE;

#ifdef PANORAMIX
Bool PanoramiXExtensionDisabledHack = FALSE;
#endif

int auditTrailLevel = 1;

char *SeatId = NULL;

sig_atomic_t inSignalContext = FALSE;

#if defined(SVR4) || defined(__linux__) || defined(CSRG_BASED)
#define HAS_SAVED_IDS_AND_SETEUID
#endif

#ifdef MONOTONIC_CLOCK
static clockid_t clockid;
#endif

OsSigHandlerPtr
OsSignal(int sig, OsSigHandlerPtr handler)
{
#if defined(WIN32) && !defined(__CYGWIN__)
    return signal(sig, handler);
#elif defined(__ANDROID__)
    return SIG_DFL;
#else
    struct sigaction act, oact;

    sigemptyset(&act.sa_mask);
    if (handler != SIG_IGN)
        sigaddset(&act.sa_mask, sig);
    act.sa_flags = 0;
    act.sa_handler = handler;
    if (sigaction(sig, &act, &oact))
        perror("sigaction");
    return oact.sa_handler;
#endif
}

/*
 * Explicit support for a server lock file like the ones used for UUCP.
 * For architectures with virtual terminals that can run more than one
 * server at a time.  This keeps the servers from stomping on each other
 * if the user forgets to give them different display numbers.
 */
#define LOCK_DIR (getenv("TMPDIR") ?: "/tmp")
#define LOCK_TMP_PREFIX "/.tX"
#define LOCK_PREFIX "/.X"
#define LOCK_SUFFIX "-lock"

#if !defined(WIN32) || defined(__CYGWIN__)
#define LOCK_SERVER
#endif

#ifndef LOCK_SERVER
void
LockServer(void)
{}

void
UnlockServer(void)
{}
#else /* LOCK_SERVER */
static Bool StillLocking = FALSE;
static char LockFile[PATH_MAX];
static Bool nolock = FALSE;

/*
 * LockServer --
 *      Check if the server lock file exists.  If so, check if the PID
 *      contained inside is valid.  If so, then die.  Otherwise, create
 *      the lock file containing the PID.
 */
void
LockServer(void)
{
    char tmp[PATH_MAX], pid_str[12];
    int lfd, i, haslock, l_pid, t;
    const char *tmppath = LOCK_DIR;
    int len;
    char port[20];

    if (nolock || NoListenAll)
        return;
    /*
     * Path names
     */
    snprintf(port, sizeof(port), "%d", atoi(display));
    len = strlen(LOCK_PREFIX) > strlen(LOCK_TMP_PREFIX) ? strlen(LOCK_PREFIX) :
        strlen(LOCK_TMP_PREFIX);
    len += strlen(tmppath) + strlen(port) + strlen(LOCK_SUFFIX) + 1;
    if (len > sizeof(LockFile))
        FatalError("Display name `%s' is too long\n", port);
    (void) sprintf(tmp, "%s" LOCK_TMP_PREFIX "%s" LOCK_SUFFIX, tmppath, port);
    (void) sprintf(LockFile, "%s" LOCK_PREFIX "%s" LOCK_SUFFIX, tmppath, port);

    /*
     * Create a temporary file containing our PID.  Attempt three times
     * to create the file.
     */
    StillLocking = TRUE;
    i = 0;
    do {
        i++;
        lfd = open(tmp, O_CREAT | O_EXCL | O_WRONLY, 0644);
        if (lfd < 0)
            sleep(2);
        else
            break;
    } while (i < 3);
    if (lfd < 0) {
        unlink(tmp);
        i = 0;
        do {
            i++;
            lfd = open(tmp, O_CREAT | O_EXCL | O_WRONLY, 0644);
            if (lfd < 0)
                sleep(2);
            else
                break;
        } while (i < 3);
    }
    if (lfd < 0)
        FatalError("Could not create lock file in %s\n", tmp);
    snprintf(pid_str, sizeof(pid_str), "%10lu\n", (unsigned long) getpid());
    if (write(lfd, pid_str, 11) != 11)
        FatalError("Could not write pid to lock file in %s\n", tmp);
    (void) fchmod(lfd, 0444);
    (void) close(lfd);

    /*
     * OK.  Now the tmp file exists.  Try three times to move it in place
     * for the lock.
     */
    i = 0;
    haslock = 0;
    while ((!haslock) && (i++ < 3)) {
        haslock = (rename(tmp, LockFile) == 0);
        if (haslock) {
            /*
             * We're done.
             */
            break;
        }
        else if (errno == EEXIST) {
            /*
             * Read the pid from the existing file
             */
            lfd = open(LockFile, O_RDONLY | O_NOFOLLOW);
            if (lfd < 0) {
                unlink(tmp);
                FatalError("Can't read lock file %s\n", LockFile);
            }
            pid_str[0] = '\0';
            if (read(lfd, pid_str, 11) != 11) {
                /*
                 * Bogus lock file.
                 */
                unlink(LockFile);
                close(lfd);
                continue;
            }
            pid_str[11] = '\0';
            sscanf(pid_str, "%d", &l_pid);
            close(lfd);

            /*
             * Now try to kill the PID to see if it exists.
             */
            errno = 0;
            t = kill(l_pid, 0);
            if ((t < 0) && (errno == ESRCH)) {
                /*
                 * Stale lock file.
                 */
                unlink(LockFile);
                continue;
            }
            else if (((t < 0) && (errno == EPERM)) || (t == 0)) {
                /*
                 * Process is still active.
                 */
                unlink(tmp);
                FatalError
                    ("Server is already active for display %s\n%s %s\n%s\n",
                     port, "\tIf this server is no longer running, remove",
                     LockFile, "\tand start again.");
            }
        }
        else {
            unlink(tmp);
            FatalError
                ("Linking lock file (%s) in place failed: %s\n",
                 LockFile, strerror(errno));
        }
    }
    unlink(tmp);
    if (!haslock)
        FatalError("Could not create server lock file: %s\n", LockFile);
    StillLocking = FALSE;
}

/*
 * UnlockServer --
 *      Remove the server lock file.
 */
void
UnlockServer(void)
{
    if (nolock || NoListenAll)
        return;

    if (!StillLocking) {

        (void) unlink(LockFile);
    }
}
#endif /* LOCK_SERVER */

/* Force connections to close on SIGHUP from init */

void
AutoResetServer(int sig)
{
    int olderrno = errno;

    dispatchException |= DE_RESET;
    isItTimeToYield = TRUE;
    errno = olderrno;
}

/* Force connections to close and then exit on SIGTERM, SIGINT */

void
GiveUp(int sig)
{
    int olderrno = errno;

    dispatchException |= DE_TERMINATE;
    isItTimeToYield = TRUE;
    errno = olderrno;
}

#ifdef MONOTONIC_CLOCK
void
ForceClockId(clockid_t forced_clockid)
{
    struct timespec tp;

    BUG_RETURN (clockid);

    clockid = forced_clockid;

    if (clock_gettime(clockid, &tp) != 0) {
        FatalError("Forced clock id failed to retrieve current time: %s\n",
                   strerror(errno));
        return;
    }
}
#endif

#if (defined WIN32 && defined __MINGW32__) || defined(__CYGWIN__)
CARD32
GetTimeInMillis(void)
{
    return GetTickCount();
}
CARD64
GetTimeInMicros(void)
{
    return (CARD64) GetTickCount() * 1000;
}
#else
CARD32
GetTimeInMillis(void)
{
    struct timeval tv;

#ifdef MONOTONIC_CLOCK
    struct timespec tp;

    if (!clockid) {
#ifdef CLOCK_MONOTONIC_COARSE
        if (clock_getres(CLOCK_MONOTONIC_COARSE, &tp) == 0 &&
            (tp.tv_nsec / 1000) <= 1000 &&
            clock_gettime(CLOCK_MONOTONIC_COARSE, &tp) == 0)
            clockid = CLOCK_MONOTONIC_COARSE;
        else
#endif
        if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
            clockid = CLOCK_MONOTONIC;
        else
            clockid = ~0L;
    }
    if (clockid != ~0L && clock_gettime(clockid, &tp) == 0)
        return (tp.tv_sec * 1000) + (tp.tv_nsec / 1000000L);
#endif

    X_GETTIMEOFDAY(&tv);
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

CARD64
GetTimeInMicros(void)
{
    struct timeval tv;
#ifdef MONOTONIC_CLOCK
    struct timespec tp;
    static clockid_t uclockid;

    if (!uclockid) {
        if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0)
            uclockid = CLOCK_MONOTONIC;
        else
            uclockid = ~0L;
    }
    if (uclockid != ~0L && clock_gettime(uclockid, &tp) == 0)
        return (CARD64) tp.tv_sec * (CARD64)1000000 + tp.tv_nsec / 1000;
#endif

    X_GETTIMEOFDAY(&tv);
    return (CARD64) tv.tv_sec * (CARD64)1000000 + (CARD64) tv.tv_usec;
}
#endif

void
UseMsg(void)
{
    ErrorF("use: X [:<display>] [option]\n");
    ErrorF("-a #                   default pointer acceleration (factor)\n");
    ErrorF("-ac                    disable access control restrictions\n");
    ErrorF("-audit int             set audit trail level\n");
    ErrorF("-auth file             select authorization file\n");
    ErrorF("-br                    create root window with black background\n");
    ErrorF("+bs                    enable any backing store support\n");
    ErrorF("-bs                    disable any backing store support\n");
    ErrorF("-c                     turns off key-click\n");
    ErrorF("c #                    key-click volume (0-100)\n");
    ErrorF("-cc int                default color visual class\n");
    ErrorF("-nocursor              disable the cursor\n");
    ErrorF("-core                  generate core dump on fatal error\n");
    ErrorF("-displayfd fd          file descriptor to write display number to when ready to connect\n");
    ErrorF("-dpi int               screen resolution in dots per inch\n");
#ifdef DPMSExtension
    ErrorF("-dpms                  disables VESA DPMS monitor control\n");
#endif
    ErrorF
        ("-deferglyphs [none|all|16] defer loading of [no|all|16-bit] glyphs\n");
    ErrorF("-f #                   bell base (0-100)\n");
    ErrorF("-fakescreenfps #       fake screen default fps (1-600)\n");
    ErrorF("-fp string             default font path\n");
    ErrorF("-help                  prints message with these options\n");
    ErrorF("+iglx                  Allow creating indirect GLX contexts\n");
    ErrorF("-iglx                  Prohibit creating indirect GLX contexts (default)\n");
    ErrorF("-I                     ignore all remaining arguments\n");
#ifdef RLIMIT_DATA
    ErrorF("-ld int                limit data space to N Kb\n");
#endif
#ifdef RLIMIT_NOFILE
    ErrorF("-lf int                limit number of open files to N\n");
#endif
#ifdef RLIMIT_STACK
    ErrorF("-ls int                limit stack space to N Kb\n");
#endif
#ifdef LOCK_SERVER
    ErrorF("-nolock                disable the locking mechanism\n");
#endif
    ErrorF("-maxclients n          set maximum number of clients (power of two)\n");
    ErrorF("-nolisten string       don't listen on protocol\n");
    ErrorF("-listen string         listen on protocol\n");
    ErrorF("-noreset               don't reset after last client exists\n");
    ErrorF("-background [none]     create root window with no background\n");
    ErrorF("-reset                 reset after last client exists\n");
    ErrorF("-p #                   screen-saver pattern duration (minutes)\n");
    ErrorF("-pn                    accept failure to listen on all ports\n");
    ErrorF("-nopn                  reject failure to listen on all ports\n");
    ErrorF("-r                     turns off auto-repeat\n");
    ErrorF("r                      turns on auto-repeat \n");
    ErrorF("-render [default|mono|gray|color] set render color alloc policy\n");
    ErrorF("-retro                 start with classic stipple and cursor\n");
    ErrorF("-s #                   screen-saver timeout (minutes)\n");
    ErrorF("-seat string           seat to run on\n");
    ErrorF("-t #                   default pointer threshold (pixels/t)\n");
    ErrorF("-terminate [delay]     terminate at server reset (optional delay in sec)\n");
    ErrorF("-tst                   disable testing extensions\n");
    ErrorF("ttyxx                  server started from init on /dev/ttyxx\n");
    ErrorF("v                      video blanking for screen-saver\n");
    ErrorF("-v                     screen-saver without video blanking\n");
    ErrorF("-wr                    create root window with white background\n");
    ErrorF("-maxbigreqsize         set maximal bigrequest size \n");
#ifdef PANORAMIX
    ErrorF("+xinerama              Enable XINERAMA extension\n");
    ErrorF("-xinerama              Disable XINERAMA extension\n");
#endif
    ErrorF
        ("-dumbSched             Disable smart scheduling and threaded input, enable old behavior\n");
    ErrorF("-schedInterval int     Set scheduler interval in msec\n");
    ErrorF("-sigstop               Enable SIGSTOP based startup\n");
    ErrorF("+extension name        Enable extension\n");
    ErrorF("-extension name        Disable extension\n");
    ListStaticExtensions();
#ifdef XDMCP
    XdmcpUseMsg();
#endif
    XkbUseMsg();
    ddxUseMsg();
}

/*  This function performs a rudimentary sanity check
 *  on the display name passed in on the command-line,
 *  since this string is used to generate filenames.
 *  It is especially important that the display name
 *  not contain a "/" and not start with a "-".
 *                                            --kvajk
 */
static int
VerifyDisplayName(const char *d)
{
    int i;
    int period_found = FALSE;
    int after_period = 0;

    if (d == (char *) 0)
        return 0;               /*  null  */
    if (*d == '\0')
        return 0;               /*  empty  */
    if (*d == '-')
        return 0;               /*  could be confused for an option  */
    if (*d == '.')
        return 0;               /*  must not equal "." or ".."  */
    if (strchr(d, '/') != (char *) 0)
        return 0;               /*  very important!!!  */

    /* Since we run atoi() on the display later, only allow
       for digits, or exception of :0.0 and similar (two decimal points max)
       */
    for (i = 0; i < strlen(d); i++) {
        if (!isdigit(d[i])) {
            if (d[i] != '.' || period_found)
                return 0;
            period_found = TRUE;
        } else if (period_found)
            after_period++;

        if (after_period > 2)
            return 0;
    }

    /* don't allow for :0. */
    if (period_found && after_period == 0)
        return 0;

    if (atol(d) > INT_MAX)
        return 0;

    return 1;
}

static const char *defaultNoListenList[] = {
#ifndef LISTEN_TCP
    "tcp",
#endif
#ifndef LISTEN_UNIX
    "unix",
#endif
#ifndef LISTEN_LOCAL
    "local",
#endif
    NULL
};

/*
 * This function parses the command line. Handles device-independent fields
 * and allows ddx to handle additional fields.  It is not allowed to modify
 * argc or any of the strings pointed to by argv.
 */
void
ProcessCommandLine(int argc, char *argv[])
{
    int i, skip;

    defaultKeyboardControl.autoRepeat = TRUE;

#ifdef NO_PART_NET
    PartialNetwork = FALSE;
#else
    PartialNetwork = TRUE;
#endif

    for (i = 0; defaultNoListenList[i] != NULL; i++) {
        if (_XSERVTransNoListen(defaultNoListenList[i]))
                    ErrorF("Failed to disable listen for %s transport",
                           defaultNoListenList[i]);
    }

    for (i = 1; i < argc; i++) {
        /* call ddx first, so it can peek/override if it wants */
        if ((skip = ddxProcessArgument(argc, argv, i))) {
            i += (skip - 1);
        }
        else if (argv[i][0] == ':') {
            /* initialize display */
            display = argv[i];
            explicit_display = TRUE;
            display++;
            if (!VerifyDisplayName(display)) {
                ErrorF("Bad display name: %s\n", display);
                UseMsg();
                FatalError("Bad display name, exiting: %s\n", display);
            }
        }
        else if (strcmp(argv[i], "-a") == 0) {
            if (++i < argc)
                defaultPointerControl.num = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-ac") == 0) {
            defeatAccessControl = TRUE;
        }
        else if (strcmp(argv[i], "-audit") == 0) {
            if (++i < argc)
                auditTrailLevel = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-auth") == 0) {
            if (++i < argc)
                InitAuthorization(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-br") == 0);  /* default */
        else if (strcmp(argv[i], "+bs") == 0)
            enableBackingStore = TRUE;
        else if (strcmp(argv[i], "-bs") == 0)
            disableBackingStore = TRUE;
        else if (strcmp(argv[i], "c") == 0) {
            if (++i < argc)
                defaultKeyboardControl.click = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-c") == 0) {
            defaultKeyboardControl.click = 0;
        }
        else if (strcmp(argv[i], "-cc") == 0) {
            if (++i < argc)
                defaultColorVisualClass = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-core") == 0) {
#if !defined(WIN32) || !defined(__MINGW32__)
            struct rlimit core_limit;

            getrlimit(RLIMIT_CORE, &core_limit);
            core_limit.rlim_cur = core_limit.rlim_max;
            setrlimit(RLIMIT_CORE, &core_limit);
#endif
            CoreDump = TRUE;
        }
        else if (strcmp(argv[i], "-nocursor") == 0) {
            EnableCursor = FALSE;
        }
        else if (strcmp(argv[i], "-dpi") == 0) {
            if (++i < argc)
                monitorResolution = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-displayfd") == 0) {
            if (++i < argc) {
                displayfd = atoi(argv[i]);
#ifdef LOCK_SERVER
                nolock = TRUE;
#endif
            }
            else
                UseMsg();
        }
#ifdef DPMSExtension
        else if (strcmp(argv[i], "dpms") == 0)
            /* ignored for compatibility */ ;
        else if (strcmp(argv[i], "-dpms") == 0)
            DPMSDisabledSwitch = TRUE;
#endif
        else if (strcmp(argv[i], "-deferglyphs") == 0) {
            if (++i >= argc || !xfont2_parse_glyph_caching_mode(argv[i]))
                UseMsg();
        }
        else if (strcmp(argv[i], "-f") == 0) {
            if (++i < argc)
                defaultKeyboardControl.bell = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-fakescreenfps") == 0) {
            if (++i < argc) {
                FakeScreenFps = (uint32_t) atoi(argv[i]);
                if (FakeScreenFps < 1 || FakeScreenFps > 600)
                    FatalError("fakescreenfps must be an integer in [1;600] range\n");
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-fp") == 0) {
            if (++i < argc) {
                defaultFontPath = argv[i];
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-help") == 0) {
            UseMsg();
            exit(0);
        }
        else if (strcmp(argv[i], "+iglx") == 0)
            enableIndirectGLX = TRUE;
        else if (strcmp(argv[i], "-iglx") == 0)
            enableIndirectGLX = FALSE;
        else if ((skip = XkbProcessArguments(argc, argv, i)) != 0) {
            if (skip > 0)
                i += skip - 1;
            else
                UseMsg();
        }
#ifdef RLIMIT_DATA
        else if (strcmp(argv[i], "-ld") == 0) {
            if (++i < argc) {
                limitDataSpace = atoi(argv[i]);
                if (limitDataSpace > 0)
                    limitDataSpace *= 1024;
            }
            else
                UseMsg();
        }
#endif
#ifdef RLIMIT_NOFILE
        else if (strcmp(argv[i], "-lf") == 0) {
            if (++i < argc)
                limitNoFile = atoi(argv[i]);
            else
                UseMsg();
        }
#endif
#ifdef RLIMIT_STACK
        else if (strcmp(argv[i], "-ls") == 0) {
            if (++i < argc) {
                limitStackSpace = atoi(argv[i]);
                if (limitStackSpace > 0)
                    limitStackSpace *= 1024;
            }
            else
                UseMsg();
        }
#endif
#ifdef LOCK_SERVER
        else if (strcmp(argv[i], "-nolock") == 0) {
#if !defined(WIN32) && !defined(__CYGWIN__)
            if (getuid() != 0)
                ErrorF
                    ("Warning: the -nolock option can only be used by root\n");
            else
#endif
                nolock = TRUE;
        }
#endif
	else if ( strcmp( argv[i], "-maxclients") == 0)
	{
	    if (++i < argc) {
		LimitClients = atoi(argv[i]);
		if (LimitClients != 64 &&
		    LimitClients != 128 &&
		    LimitClients != 256 &&
		    LimitClients != 512 &&
                    LimitClients != 1024 &&
                    LimitClients != 2048) {
		    FatalError("maxclients must be one of 64, 128, 256, 512, 1024 or 2048\n");
		}
	    } else
		UseMsg();
	}
        else if (strcmp(argv[i], "-nolisten") == 0) {
            if (++i < argc) {
                if (_XSERVTransNoListen(argv[i]))
                    ErrorF("Failed to disable listen for %s transport",
                           argv[i]);
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-listen") == 0) {
            if (++i < argc) {
                if (_XSERVTransListen(argv[i]))
                    ErrorF("Failed to enable listen for %s transport",
                           argv[i]);
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-noreset") == 0) {
            dispatchExceptionAtReset = 0;
        }
        else if (strcmp(argv[i], "-reset") == 0) {
            dispatchExceptionAtReset = DE_RESET;
        }
        else if (strcmp(argv[i], "-p") == 0) {
            if (++i < argc)
                defaultScreenSaverInterval = ((CARD32) atoi(argv[i])) *
                    MILLI_PER_MIN;
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-pogo") == 0) {
            dispatchException = DE_TERMINATE;
        }
        else if (strcmp(argv[i], "-pn") == 0)
            PartialNetwork = TRUE;
        else if (strcmp(argv[i], "-nopn") == 0)
            PartialNetwork = FALSE;
        else if (strcmp(argv[i], "r") == 0)
            defaultKeyboardControl.autoRepeat = TRUE;
        else if (strcmp(argv[i], "-r") == 0)
            defaultKeyboardControl.autoRepeat = FALSE;
        else if (strcmp(argv[i], "-retro") == 0)
            party_like_its_1989 = TRUE;
        else if (strcmp(argv[i], "-s") == 0) {
            if (++i < argc)
                defaultScreenSaverTime = ((CARD32) atoi(argv[i])) *
                    MILLI_PER_MIN;
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-seat") == 0) {
            if (++i < argc)
                SeatId = argv[i];
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-t") == 0) {
            if (++i < argc)
                defaultPointerControl.threshold = atoi(argv[i]);
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-terminate") == 0) {
            dispatchExceptionAtReset = DE_TERMINATE;
            terminateDelay = -1;
            if ((i + 1 < argc) && (isdigit(*argv[i + 1])))
               terminateDelay = atoi(argv[++i]);
            terminateDelay = max(0, terminateDelay);
        }
        else if (strcmp(argv[i], "-tst") == 0) {
            noTestExtensions = TRUE;
        }
        else if (strcmp(argv[i], "v") == 0)
            defaultScreenSaverBlanking = PreferBlanking;
        else if (strcmp(argv[i], "-v") == 0)
            defaultScreenSaverBlanking = DontPreferBlanking;
        else if (strcmp(argv[i], "-wr") == 0)
            whiteRoot = TRUE;
        else if (strcmp(argv[i], "-background") == 0) {
            if (++i < argc) {
                if (!strcmp(argv[i], "none"))
                    bgNoneRoot = TRUE;
                else
                    UseMsg();
            }
        }
        else if (strcmp(argv[i], "-maxbigreqsize") == 0) {
            if (++i < argc) {
                long reqSizeArg = atol(argv[i]);

                /* Request size > 128MB does not make much sense... */
                if (reqSizeArg > 0L && reqSizeArg < 128L) {
                    maxBigRequestSize = (reqSizeArg * 1048576L) - 1L;
                }
                else {
                    UseMsg();
                }
            }
            else {
                UseMsg();
            }
        }
#ifdef PANORAMIX
        else if (strcmp(argv[i], "+xinerama") == 0) {
            noPanoramiXExtension = FALSE;
        }
        else if (strcmp(argv[i], "-xinerama") == 0) {
            noPanoramiXExtension = TRUE;
        }
        else if (strcmp(argv[i], "-disablexineramaextension") == 0) {
            PanoramiXExtensionDisabledHack = TRUE;
        }
#endif
        else if (strcmp(argv[i], "-I") == 0) {
            /* ignore all remaining arguments */
            break;
        }
        else if (strncmp(argv[i], "tty", 3) == 0) {
            /* init supplies us with this useless information */
        }
#ifdef XDMCP
        else if ((skip = XdmcpOptions(argc, argv, i)) != i) {
            i = skip - 1;
        }
#endif
        else if (strcmp(argv[i], "-dumbSched") == 0) {
            InputThreadEnable = FALSE;
#ifdef HAVE_SETITIMER
            SmartScheduleSignalEnable = FALSE;
#endif
        }
        else if (strcmp(argv[i], "-schedInterval") == 0) {
            if (++i < argc) {
                SmartScheduleInterval = atoi(argv[i]);
                SmartScheduleSlice = SmartScheduleInterval;
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-schedMax") == 0) {
            if (++i < argc) {
                SmartScheduleMaxSlice = atoi(argv[i]);
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-render") == 0) {
            if (++i < argc) {
                int policy = PictureParseCmapPolicy(argv[i]);

                if (policy != PictureCmapPolicyInvalid)
                    PictureCmapPolicy = policy;
                else
                    UseMsg();
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-sigstop") == 0) {
            RunFromSigStopParent = TRUE;
        }
        else if (strcmp(argv[i], "+extension") == 0) {
            if (++i < argc) {
                if (!EnableDisableExtension(argv[i], TRUE))
                    EnableDisableExtensionError(argv[i], TRUE);
            }
            else
                UseMsg();
        }
        else if (strcmp(argv[i], "-extension") == 0) {
            if (++i < argc) {
                if (!EnableDisableExtension(argv[i], FALSE))
                    EnableDisableExtensionError(argv[i], FALSE);
            }
            else
                UseMsg();
        }
        else {
            ErrorF("Unrecognized option: %s\n", argv[i]);
            UseMsg();
            FatalError("Unrecognized option: %s\n", argv[i]);
        }
    }
}

/* Implement a simple-minded font authorization scheme.  The authorization
   name is "hp-hostname-1", the contents are simply the host name. */
int
set_font_authorizations(char **authorizations, int *authlen, void *client)
{
#define AUTHORIZATION_NAME "hp-hostname-1"
#if defined(TCPCONN)
    static char *result = NULL;
    static char *p = NULL;

    if (p == NULL) {
        char hname[1024], *hnameptr;
        unsigned int len;

#if defined(IPv6) && defined(AF_INET6)
        struct addrinfo hints, *ai = NULL;
#else
        struct hostent *host;

#ifdef XTHREADS_NEEDS_BYNAMEPARAMS
        _Xgethostbynameparams hparams;
#endif
#endif

        gethostname(hname, 1024);
#if defined(IPv6) && defined(AF_INET6)
        memset(&hints, 0, sizeof(hints));
        hints.ai_flags = AI_CANONNAME;
        if (getaddrinfo(hname, NULL, &hints, &ai) == 0) {
            hnameptr = ai->ai_canonname;
        }
        else {
            hnameptr = hname;
        }
#else
        host = _XGethostbyname(hname, hparams);
        if (host == NULL)
            hnameptr = hname;
        else
            hnameptr = host->h_name;
#endif

        len = strlen(hnameptr) + 1;
        result = malloc(len + sizeof(AUTHORIZATION_NAME) + 4);

        p = result;
        *p++ = sizeof(AUTHORIZATION_NAME) >> 8;
        *p++ = sizeof(AUTHORIZATION_NAME) & 0xff;
        *p++ = (len) >> 8;
        *p++ = (len & 0xff);

        memmove(p, AUTHORIZATION_NAME, sizeof(AUTHORIZATION_NAME));
        p += sizeof(AUTHORIZATION_NAME);
        memmove(p, hnameptr, len);
        p += len;
#if defined(IPv6) && defined(AF_INET6)
        if (ai) {
            freeaddrinfo(ai);
        }
#endif
    }
    *authlen = p - result;
    *authorizations = result;
    return 1;
#else                           /* TCPCONN */
    return 0;
#endif                          /* TCPCONN */
}

void *
XNFalloc(unsigned long amount)
{
    void *ptr = malloc(amount);

    if (!ptr)
        FatalError("Out of memory");
    return ptr;
}

/* The original XNFcalloc was used with the xnfcalloc macro which multiplied
 * the arguments at the call site without allowing calloc to check for overflow.
 * XNFcallocarray was added to fix that without breaking ABI.
 */
void *
XNFcalloc(unsigned long amount)
{
    return XNFcallocarray(1, amount);
}

void *
XNFcallocarray(size_t nmemb, size_t size)
{
    void *ret = calloc(nmemb, size);

    if (!ret)
        FatalError("XNFcalloc: Out of memory");
    return ret;
}

void *
XNFrealloc(void *ptr, unsigned long amount)
{
    void *ret = realloc(ptr, amount);

    if (!ret)
        FatalError("XNFrealloc: Out of memory");
    return ret;
}

void *
XNFreallocarray(void *ptr, size_t nmemb, size_t size)
{
    void *ret = reallocarray(ptr, nmemb, size);

    if (!ret)
        FatalError("XNFreallocarray: Out of memory");
    return ret;
}

char *
Xstrdup(const char *s)
{
    if (s == NULL)
        return NULL;
    return strdup(s);
}

char *
XNFstrdup(const char *s)
{
    char *ret;

    if (s == NULL)
        return NULL;

    ret = strdup(s);
    if (!ret)
        FatalError("XNFstrdup: Out of memory");
    return ret;
}

void
SmartScheduleStopTimer(void)
{
#ifdef HAVE_SETITIMER
    struct itimerval timer;

    if (!SmartScheduleSignalEnable)
        return;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = 0;
    (void) setitimer(ITIMER_REAL, &timer, 0);
#endif
}

void
SmartScheduleStartTimer(void)
{
#ifdef HAVE_SETITIMER
    struct itimerval timer;

    if (!SmartScheduleSignalEnable)
        return;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = SmartScheduleInterval * 1000;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = SmartScheduleInterval * 1000;
    setitimer(ITIMER_REAL, &timer, 0);
#endif
}

#ifdef HAVE_SETITIMER
static void
SmartScheduleTimer(int sig)
{
    SmartScheduleTime += SmartScheduleInterval;
}

static int
SmartScheduleEnable(void)
{
    int ret = 0;
    struct sigaction act;

    if (!SmartScheduleSignalEnable)
        return 0;

    memset((char *) &act, 0, sizeof(struct sigaction));

    /* Set up the timer signal function */
    act.sa_flags = SA_RESTART;
    act.sa_handler = SmartScheduleTimer;
    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGALRM);
    ret = sigaction(SIGALRM, &act, 0);
    return ret;
}

static int
SmartSchedulePause(void)
{
    int ret = 0;
    struct sigaction act;

    if (!SmartScheduleSignalEnable)
        return 0;

    memset((char *) &act, 0, sizeof(struct sigaction));

    act.sa_handler = SIG_IGN;
    sigemptyset(&act.sa_mask);
    ret = sigaction(SIGALRM, &act, 0);
    return ret;
}
#endif

void
SmartScheduleInit(void)
{
#ifdef HAVE_SETITIMER
    if (SmartScheduleEnable() < 0) {
        perror("sigaction for smart scheduler");
        SmartScheduleSignalEnable = FALSE;
    }
#endif
}

#ifdef HAVE_SIGPROCMASK
static sigset_t PreviousSignalMask;
static int BlockedSignalCount;
#endif

void
OsBlockSignals(void)
{
#ifdef HAVE_SIGPROCMASK
    if (BlockedSignalCount++ == 0) {
        sigset_t set;

        sigemptyset(&set);
        sigaddset(&set, SIGALRM);
        sigaddset(&set, SIGVTALRM);
#ifdef SIGWINCH
        sigaddset(&set, SIGWINCH);
#endif
        sigaddset(&set, SIGTSTP);
        sigaddset(&set, SIGTTIN);
        sigaddset(&set, SIGTTOU);
        sigaddset(&set, SIGCHLD);
        xthread_sigmask(SIG_BLOCK, &set, &PreviousSignalMask);
    }
#endif
}

void
OsReleaseSignals(void)
{
#ifdef HAVE_SIGPROCMASK
    if (--BlockedSignalCount == 0) {
        xthread_sigmask(SIG_SETMASK, &PreviousSignalMask, 0);
    }
#endif
}

void
OsResetSignals(void)
{
#ifdef HAVE_SIGPROCMASK
    while (BlockedSignalCount > 0)
        OsReleaseSignals();
    input_force_unlock();
#endif
}

/*
 * Pending signals may interfere with core dumping. Provide a
 * mechanism to block signals when aborting.
 */

void
OsAbort(void)
{
#ifndef __APPLE__
    OsBlockSignals();
#endif
#if !defined(WIN32) || defined(__CYGWIN__)
    /* abort() raises SIGABRT, so we have to stop handling that to prevent
     * recursion
     */
    OsSignal(SIGABRT, SIG_DFL);
#endif
    abort();
}

#if !defined(WIN32)
/*
 * "safer" versions of system(3), popen(3) and pclose(3) which give up
 * all privs before running a command.
 *
 * This is based on the code in FreeBSD 2.2 libc.
 *
 * XXX It'd be good to redirect stderr so that it ends up in the log file
 * as well.  As it is now, xkbcomp messages don't end up in the log file.
 */

int
System(const char *command)
{
    int pid, p;
    void (*csig) (int);
    int status;

    if (!command)
        return 1;

    csig = OsSignal(SIGCHLD, SIG_DFL);
    if (csig == SIG_ERR) {
        perror("signal");
        return -1;
    }
    DebugF("System: `%s'\n", command);

    switch (pid = fork()) {
    case -1:                   /* error */
        p = -1;
        break;
    case 0:                    /* child */
        if (setgid(getgid()) == -1)
            _exit(127);
        if (setuid(getuid()) == -1)
            _exit(127);
        execl("/bin/sh", "sh", "-c", command, (char *) NULL);
        _exit(127);
    default:                   /* parent */
        do {
            p = waitpid(pid, &status, 0);
        } while (p == -1 && errno == EINTR);

    }

    if (OsSignal(SIGCHLD, csig) == SIG_ERR) {
        perror("signal");
        return -1;
    }

    return p == -1 ? -1 : status;
}

static struct pid {
    struct pid *next;
    FILE *fp;
    int pid;
} *pidlist;

extern char* xkbcomp_argv[];
extern int xkbcomp_argc;
int xkbcomp_main(int argc, char *argv[]);

void *
Popen(const char *command, const char *type)
{
    struct pid *cur;
    FILE *iop;
    int pdes[2], pid;

    if (command == NULL || type == NULL)
        return NULL;

    if ((*type != 'r' && *type != 'w') || type[1])
        return NULL;

    if ((cur = malloc(sizeof(struct pid))) == NULL)
        return NULL;

    if (pipe(pdes) < 0) {
        free(cur);
        return NULL;
    }

    /* Ignore the smart scheduler while this is going on */
#ifdef HAVE_SETITIMER
    if (SmartSchedulePause() < 0) {
        close(pdes[0]);
        close(pdes[1]);
        free(cur);
        perror("signal");
        return NULL;
    }
#endif

    switch (pid = fork()) {
    case -1:                   /* error */
        close(pdes[0]);
        close(pdes[1]);
        free(cur);
#ifdef HAVE_SETITIMER
        if (SmartScheduleEnable() < 0)
            perror("signal");
#endif
        return NULL;
    case 0:                    /* child */
        if (setgid(getgid()) == -1)
            _exit(127);
        if (setuid(getuid()) == -1)
            _exit(127);
        if (*type == 'r') {
            if (pdes[1] != 1) {
                /* stdout */
                dup2(pdes[1], 1);
                close(pdes[1]);
            }
            close(pdes[0]);
        }
        else {
            if (pdes[0] != 0) {
                /* stdin */
                dup2(pdes[0], 0);
                close(pdes[0]);
            }
            close(pdes[1]);
        }

        for(int j=1; j<= SIGUNUSED; j++)
            signal(j, SIG_DFL);

        _exit(xkbcomp_main(xkbcomp_argc, xkbcomp_argv));
    }

    /* Avoid EINTR during stdio calls */
    OsBlockSignals();

    /* parent */
    if (*type == 'r') {
        iop = fdopen(pdes[0], type);
        close(pdes[1]);
    }
    else {
        iop = fdopen(pdes[1], type);
        close(pdes[0]);
    }

    cur->fp = iop;
    cur->pid = pid;
    cur->next = pidlist;
    pidlist = cur;

    DebugF("Popen: `%s', fp = %p\n", command, iop);

    return iop;
}

/* fopen that drops privileges */
void *
Fopen(const char *file, const char *type)
{
    FILE *iop;

#ifndef HAS_SAVED_IDS_AND_SETEUID
    struct pid *cur;
    int pdes[2], pid;

    if (file == NULL || type == NULL)
        return NULL;

    if ((*type != 'r' && *type != 'w') || type[1])
        return NULL;

    if ((cur = malloc(sizeof(struct pid))) == NULL)
        return NULL;

    if (pipe(pdes) < 0) {
        free(cur);
        return NULL;
    }

    switch (pid = fork()) {
    case -1:                   /* error */
        close(pdes[0]);
        close(pdes[1]);
        free(cur);
        return NULL;
    case 0:                    /* child */
        if (setgid(getgid()) == -1)
            _exit(127);
        if (setuid(getuid()) == -1)
            _exit(127);
        if (*type == 'r') {
            if (pdes[1] != 1) {
                /* stdout */
                dup2(pdes[1], 1);
                close(pdes[1]);
            }
            close(pdes[0]);
        }
        else {
            if (pdes[0] != 0) {
                /* stdin */
                dup2(pdes[0], 0);
                close(pdes[0]);
            }
            close(pdes[1]);
        }
        execl("/bin/cat", "cat", file, (char *) NULL);
        _exit(127);
    }

    /* Avoid EINTR during stdio calls */
    OsBlockSignals();

    /* parent */
    if (*type == 'r') {
        iop = fdopen(pdes[0], type);
        close(pdes[1]);
    }
    else {
        iop = fdopen(pdes[1], type);
        close(pdes[0]);
    }

    cur->fp = iop;
    cur->pid = pid;
    cur->next = pidlist;
    pidlist = cur;

    DebugF("Fopen(%s), fp = %p\n", file, iop);

    return iop;
#else
    int ruid, euid;

    ruid = getuid();
    euid = geteuid();

    if (seteuid(ruid) == -1) {
        return NULL;
    }
    iop = fopen(file, type);

    if (seteuid(euid) == -1) {
        fclose(iop);
        return NULL;
    }
    return iop;
#endif                          /* HAS_SAVED_IDS_AND_SETEUID */
}

int
Pclose(void *iop)
{
    struct pid *cur, *last;
    int pstat;
    int pid;

    DebugF("Pclose: fp = %p\n", iop);
    fclose(iop);

    for (last = NULL, cur = pidlist; cur; last = cur, cur = cur->next)
        if (cur->fp == iop)
            break;
    if (cur == NULL)
        return -1;

    do {
        pid = waitpid(cur->pid, &pstat, 0);
    } while (pid == -1 && errno == EINTR);

    if (last == NULL)
        pidlist = cur->next;
    else
        last->next = cur->next;
    free(cur);

    /* allow EINTR again */
    OsReleaseSignals();

#ifdef HAVE_SETITIMER
    if (SmartScheduleEnable() < 0) {
        perror("signal");
        return -1;
    }
#endif

    return pid == -1 ? -1 : pstat;
}

int
Fclose(void *iop)
{
#ifdef HAS_SAVED_IDS_AND_SETEUID
    return fclose(iop);
#else
    return Pclose(iop);
#endif
}

#endif                          /* !WIN32 */

#ifdef WIN32

#include <X11/Xwindows.h>

const char *
Win32TempDir(void)
{
    static char buffer[PATH_MAX];

    if (GetTempPath(sizeof(buffer), buffer)) {
        int len;

        buffer[sizeof(buffer) - 1] = 0;
        len = strlen(buffer);
        if (len > 0)
            if (buffer[len - 1] == '\\')
                buffer[len - 1] = 0;
        return buffer;
    }
    if (getenv("TEMP") != NULL)
        return getenv("TEMP");
    else if (getenv("TMP") != NULL)
        return getenv("TMP");
    else
        return "/tmp";
}

int
System(const char *cmdline)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD dwExitCode;
    char *cmd = strdup(cmdline);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        LPVOID buffer;

        if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                           FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
                           NULL,
                           GetLastError(),
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           (LPTSTR) &buffer, 0, NULL)) {
            ErrorF("[xkb] Starting '%s' failed!\n", cmdline);
        }
        else {
            ErrorF("[xkb] Starting '%s' failed: %s", cmdline, (char *) buffer);
            LocalFree(buffer);
        }

        free(cmd);
        return -1;
    }
    /* Wait until child process exits. */
    WaitForSingleObject(pi.hProcess, INFINITE);

    GetExitCodeProcess(pi.hProcess, &dwExitCode);

    /* Close process and thread handles. */
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    free(cmd);

    return dwExitCode;
}
#endif

Bool
PrivsElevated(void)
{
    static Bool privsTested = FALSE;
    static Bool privsElevated = TRUE;

    if (!privsTested) {
#if defined(WIN32)
        privsElevated = FALSE;
#else
        if ((getuid() != geteuid()) || (getgid() != getegid())) {
            privsElevated = TRUE;
        }
        else {
#if defined(HAVE_ISSETUGID)
            privsElevated = issetugid();
#elif defined(HAVE_GETRESUID)
            uid_t ruid, euid, suid;
            gid_t rgid, egid, sgid;

            if ((getresuid(&ruid, &euid, &suid) == 0) &&
                (getresgid(&rgid, &egid, &sgid) == 0)) {
                privsElevated = (euid != suid) || (egid != sgid);
            }
            else {
                printf("Failed getresuid or getresgid");
                /* Something went wrong, make defensive assumption */
                privsElevated = TRUE;
            }
#else
            if (getuid() == 0) {
                /* running as root: uid==euid==0 */
                privsElevated = FALSE;
            }
            else {
                /*
                 * If there are saved ID's the process might still be privileged
                 * even though the above test succeeded. If issetugid() and
                 * getresgid() aren't available, test this by trying to set
                 * euid to 0.
                 */
                unsigned int oldeuid;

                oldeuid = geteuid();

                if (seteuid(0) != 0) {
                    privsElevated = FALSE;
                }
                else {
                    if (seteuid(oldeuid) != 0) {
                        FatalError("Failed to drop privileges.  Exiting\n");
                    }
                    privsElevated = TRUE;
                }
            }
#endif
        }
#endif
        privsTested = TRUE;
    }
    return privsElevated;
}

/*
 * CheckUserParameters: check for long command line arguments and long
 * environment variables.  By default, these checks are only done when
 * the server's euid != ruid.  In 3.3.x, these checks were done in an
 * external wrapper utility.
 */

/* Consider LD* variables insecure? */
#ifndef REMOVE_ENV_LD
#define REMOVE_ENV_LD 1
#endif

/* Remove long environment variables? */
#ifndef REMOVE_LONG_ENV
#define REMOVE_LONG_ENV 1
#endif

/*
 * Disallow stdout or stderr as pipes?  It's possible to block the X server
 * when piping stdout+stderr to a pipe.
 *
 * Don't enable this because it looks like it's going to cause problems.
 */
#ifndef NO_OUTPUT_PIPES
#define NO_OUTPUT_PIPES 0
#endif

/* Check args and env only if running setuid (euid == 0 && euid != uid) ? */
#ifndef CHECK_EUID
#ifndef WIN32
#define CHECK_EUID 1
#else
#define CHECK_EUID 0
#endif
#endif

/*
 * Maybe the locale can be faked to make isprint(3) report that everything
 * is printable?  Avoid it by default.
 */
#ifndef USE_ISPRINT
#define USE_ISPRINT 0
#endif

#define MAX_ARG_LENGTH          128
#define MAX_ENV_LENGTH          256
#define MAX_ENV_PATH_LENGTH     2048    /* Limit for *PATH and TERMCAP */

#if USE_ISPRINT
#include <ctype.h>
#define checkPrintable(c) isprint(c)
#else
#define checkPrintable(c) (((c) & 0x7f) >= 0x20 && ((c) & 0x7f) != 0x7f)
#endif

enum BadCode {
    NotBad = 0,
    UnsafeArg,
    ArgTooLong,
    UnprintableArg,
    EnvTooLong,
    OutputIsPipe,
    InternalError
};

#if defined(VENDORSUPPORT)
#define BUGADDRESS VENDORSUPPORT
#elif defined(BUILDERADDR)
#define BUGADDRESS BUILDERADDR
#else
#define BUGADDRESS "xorg@freedesktop.org"
#endif

void
CheckUserParameters(int argc, char **argv, char **envp)
{
    enum BadCode bad = NotBad;
    int i = 0, j;
    char *a, *e = NULL;

#if CHECK_EUID
    if (PrivsElevated())
#endif
    {
        /* Check each argv[] */
        for (i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-fp") == 0) {
                i++;            /* continue with next argument. skip the length check */
                if (i >= argc)
                    break;
            }
            else {
                if (strlen(argv[i]) > MAX_ARG_LENGTH) {
                    bad = ArgTooLong;
                    break;
                }
            }
            a = argv[i];
            while (*a) {
                if (checkPrintable(*a) == 0) {
                    bad = UnprintableArg;
                    break;
                }
                a++;
            }
            if (bad)
                break;
        }
        if (!bad) {
            /* Check each envp[] */
            for (i = 0; envp[i]; i++) {

                /* Check for bad environment variables and values */
#if REMOVE_ENV_LD
                while (envp[i] && (strncmp(envp[i], "LD", 2) == 0)) {
                    for (j = i; envp[j]; j++) {
                        envp[j] = envp[j + 1];
                    }
                }
#endif
                if (envp[i] && (strlen(envp[i]) > MAX_ENV_LENGTH)) {
#if REMOVE_LONG_ENV
                    for (j = i; envp[j]; j++) {
                        envp[j] = envp[j + 1];
                    }
                    i--;
#else
                    char *eq;
                    int len;

                    eq = strchr(envp[i], '=');
                    if (!eq)
                        continue;
                    len = eq - envp[i];
                    e = strndup(envp[i], len);
                    if (!e) {
                        bad = InternalError;
                        break;
                    }
                    if (len >= 4 &&
                        (strcmp(e + len - 4, "PATH") == 0 ||
                         strcmp(e, "TERMCAP") == 0)) {
                        if (strlen(envp[i]) > MAX_ENV_PATH_LENGTH) {
                            bad = EnvTooLong;
                            break;
                        }
                        else {
                            free(e);
                        }
                    }
                    else {
                        bad = EnvTooLong;
                        break;
                    }
#endif
                }
            }
        }
#if NO_OUTPUT_PIPES
        if (!bad) {
            struct stat buf;

            if (fstat(fileno(stdout), &buf) == 0 && S_ISFIFO(buf.st_mode))
                bad = OutputIsPipe;
            if (fstat(fileno(stderr), &buf) == 0 && S_ISFIFO(buf.st_mode))
                bad = OutputIsPipe;
        }
#endif
    }
    switch (bad) {
    case NotBad:
        return;
    case UnsafeArg:
        ErrorF("Command line argument number %d is unsafe\n", i);
        break;
    case ArgTooLong:
        ErrorF("Command line argument number %d is too long\n", i);
        break;
    case UnprintableArg:
        ErrorF("Command line argument number %d contains unprintable"
               " characters\n", i);
        break;
    case EnvTooLong:
        ErrorF("Environment variable `%s' is too long\n", e);
        break;
    case OutputIsPipe:
        ErrorF("Stdout and/or stderr is a pipe\n");
        break;
    case InternalError:
        ErrorF("Internal Error\n");
        break;
    default:
        ErrorF("Unknown error\n");
        break;
    }
    FatalError("X server aborted because of unsafe environment\n");
}

/*
 * CheckUserAuthorization: check if the user is allowed to start the
 * X server.  This usually means some sort of PAM checking, and it is
 * usually only done for setuid servers (uid != euid).
 */

#ifdef USE_PAM
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <pwd.h>
#endif                          /* USE_PAM */

void
CheckUserAuthorization(void)
{
#ifdef USE_PAM
    static struct pam_conv conv = {
        misc_conv,
        NULL
    };

    pam_handle_t *pamh = NULL;
    struct passwd *pw;
    int retval;

    if (getuid() != geteuid()) {
        pw = getpwuid(getuid());
        if (pw == NULL)
            FatalError("getpwuid() failed for uid %d\n", getuid());

        retval = pam_start("xserver", pw->pw_name, &conv, &pamh);
        if (retval != PAM_SUCCESS)
            FatalError("pam_start() failed.\n"
                       "\tMissing or mangled PAM config file or module?\n");

        retval = pam_authenticate(pamh, 0);
        if (retval != PAM_SUCCESS) {
            pam_end(pamh, retval);
            FatalError("PAM authentication failed, cannot start X server.\n"
                       "\tPerhaps you do not have console ownership?\n");
        }

        retval = pam_acct_mgmt(pamh, 0);
        if (retval != PAM_SUCCESS) {
            pam_end(pamh, retval);
            FatalError("PAM authentication failed, cannot start X server.\n"
                       "\tPerhaps you do not have console ownership?\n");
        }

        /* this is not a session, so do not do session management */
        pam_end(pamh, PAM_SUCCESS);
    }
#endif
}

/*
 * Tokenize a string into a NULL terminated array of strings. Always returns
 * an allocated array unless an error occurs.
 */
char **
xstrtokenize(const char *str, const char *separators)
{
    char **list, **nlist;
    char *tok, *tmp;
    unsigned num = 0, n;

    if (!str)
        return NULL;
    list = calloc(1, sizeof(*list));
    if (!list)
        return NULL;
    tmp = strdup(str);
    if (!tmp)
        goto error;
    for (tok = strtok(tmp, separators); tok; tok = strtok(NULL, separators)) {
        nlist = reallocarray(list, num + 2, sizeof(*list));
        if (!nlist)
            goto error;
        list = nlist;
        list[num] = strdup(tok);
        if (!list[num])
            goto error;
        list[++num] = NULL;
    }
    free(tmp);
    return list;

 error:
    free(tmp);
    for (n = 0; n < num; n++)
        free(list[n]);
    free(list);
    return NULL;
}

/* Format a signed number into a string in a signal safe manner. The string
 * should be at least 21 characters in order to handle all int64_t values.
 */
void
FormatInt64(int64_t num, char *string)
{
    if (num < 0) {
        string[0] = '-';
        num *= -1;
        string++;
    }
    FormatUInt64(num, string);
}

/* Format a number into a string in a signal safe manner. The string should be
 * at least 21 characters in order to handle all uint64_t values. */
void
FormatUInt64(uint64_t num, char *string)
{
    uint64_t divisor;
    int len;
    int i;

    for (len = 1, divisor = 10;
         len < 20 && num / divisor;
         len++, divisor *= 10);

    for (i = len, divisor = 1; i > 0; i--, divisor *= 10)
        string[i - 1] = '0' + ((num / divisor) % 10);

    string[len] = '\0';
}

/**
 * Format a double number as %.2f.
 */
void
FormatDouble(double dbl, char *string)
{
    int slen = 0;
    uint64_t frac;

    frac = (dbl > 0 ? dbl : -dbl) * 100.0 + 0.5;
    frac %= 100;

    /* write decimal part to string */
    if (dbl < 0 && dbl > -1)
        string[slen++] = '-';
    FormatInt64((int64_t)dbl, &string[slen]);

    while(string[slen] != '\0')
        slen++;

    /* append fractional part, but only if we have enough characters. We
     * expect string to be 21 chars (incl trailing \0) */
    if (slen <= 17) {
        string[slen++] = '.';
        if (frac < 10)
            string[slen++] = '0';

        FormatUInt64(frac, &string[slen]);
    }
}


/* Format a number into a hexadecimal string in a signal safe manner. The string
 * should be at least 17 characters in order to handle all uint64_t values. */
void
FormatUInt64Hex(uint64_t num, char *string)
{
    uint64_t divisor;
    int len;
    int i;

    for (len = 1, divisor = 0x10;
         len < 16 && num / divisor;
         len++, divisor *= 0x10);

    for (i = len, divisor = 1; i > 0; i--, divisor *= 0x10) {
        int val = (num / divisor) % 0x10;

        if (val < 10)
            string[i - 1] = '0' + val;
        else
            string[i - 1] = 'a' + val - 10;
    }

    string[len] = '\0';
}

#if !defined(WIN32) || defined(__CYGWIN__)
/* Move a file descriptor out of the way of our select mask; this
 * is useful for file descriptors which will never appear in the
 * select mask to avoid reducing the number of clients that can
 * connect to the server
 */
int
os_move_fd(int fd)
{
    int newfd;

#ifdef F_DUPFD_CLOEXEC
    newfd = fcntl(fd, F_DUPFD_CLOEXEC, MAXCLIENTS);
#else
    newfd = fcntl(fd, F_DUPFD, MAXCLIENTS);
#endif
    if (newfd < 0)
        return fd;
#ifndef F_DUPFD_CLOEXEC
    fcntl(newfd, F_SETFD, FD_CLOEXEC);
#endif
    close(fd);
    return newfd;
}
#endif
