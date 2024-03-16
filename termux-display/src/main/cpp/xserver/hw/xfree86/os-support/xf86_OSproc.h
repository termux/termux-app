/*
 * Copyright 1990, 1991 by Thomas Roell, Dinkelscherben, Germany
 * Copyright 1992 by David Dawes <dawes@XFree86.org>
 * Copyright 1992 by Jim Tsillas <jtsilla@damon.ccs.northeastern.edu>
 * Copyright 1992 by Rich Murphey <Rich@Rice.edu>
 * Copyright 1992 by Robert Baron <Robert.Baron@ernst.mach.cs.cmu.edu>
 * Copyright 1992 by Orest Zborowski <obz@eskimo.com>
 * Copyright 1993 by Vrije Universiteit, The Netherlands
 * Copyright 1993 by David Wexelblat <dwex@XFree86.org>
 * Copyright 1994, 1996 by Holger Veit <Holger.Veit@gmd.de>
 * Copyright 1994-2003 by The XFree86 Project, Inc
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of the above listed copyright holders
 * not be used in advertising or publicity pertaining to distribution of
 * the software without specific, written prior permission.  The above listed
 * copyright holders make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE ABOVE LISTED COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
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

#ifndef _XF86_OSPROC_H
#define _XF86_OSPROC_H

/*
 * The actual prototypes have been pulled into this separate file so
 * that they can can be used without pulling in all of the OS specific
 * stuff like sys/stat.h, etc. that causes problems for loadable modules.
 */

/*
 * OS-independent modem state flags for xf86SetSerialModemState() and
 * xf86GetSerialModemState().
 */
#define XF86_M_LE		0x001   /* line enable */
#define XF86_M_DTR		0x002   /* data terminal ready */
#define XF86_M_RTS		0x004   /* request to send */
#define XF86_M_ST		0x008   /* secondary transmit */
#define XF86_M_SR		0x010   /* secondary receive */
#define XF86_M_CTS		0x020   /* clear to send */
#define XF86_M_CAR		0x040   /* carrier detect */
#define XF86_M_RNG		0x080   /* ring */
#define XF86_M_DSR		0x100   /* data set ready */

#ifndef NO_OSLIB_PROTOTYPES
/*
 * This is to prevent re-entrancy to FatalError() when aborting.
 * Anything that can be called as a result of ddxGiveUp() should use this
 * instead of FatalError().
 */

#define xf86FatalError(a, b) \
	if (dispatchException & DE_TERMINATE) { \
		ErrorF(a, b); \
		ErrorF("\n"); \
		return; \
	} else FatalError(a, b)

/***************************************************************************/
/* Prototypes                                                              */
/***************************************************************************/

#include <X11/Xfuncproto.h>
#include "opaque.h"
#include "xf86Optionstr.h"

_XFUNCPROTOBEGIN

/* public functions */
extern _X_EXPORT Bool xf86EnableIO(void);
extern _X_EXPORT void xf86DisableIO(void);

#ifdef __NetBSD__
extern _X_EXPORT void xf86SetTVOut(int);
extern _X_EXPORT void xf86SetRGBOut(void);
#endif
extern _X_EXPORT void xf86OSRingBell(int, int, int);
extern _X_EXPORT void xf86SetReallySlowBcopy(void);
extern _X_EXPORT void xf86SlowBcopy(unsigned char *, unsigned char *, int);
extern _X_EXPORT int xf86OpenSerial(XF86OptionPtr options);
extern _X_EXPORT int xf86SetSerial(int fd, XF86OptionPtr options);
extern _X_EXPORT int xf86SetSerialSpeed(int fd, int speed);
extern _X_EXPORT int xf86ReadSerial(int fd, void *buf, int count);
extern _X_EXPORT int xf86WriteSerial(int fd, const void *buf, int count);
extern _X_EXPORT int xf86CloseSerial(int fd);
extern _X_EXPORT int xf86FlushInput(int fd);
extern _X_EXPORT int xf86WaitForInput(int fd, int timeout);
extern _X_EXPORT int xf86SerialSendBreak(int fd, int duration);
extern _X_EXPORT int xf86SetSerialModemState(int fd, int state);
extern _X_EXPORT int xf86GetSerialModemState(int fd);
extern _X_EXPORT int xf86SerialModemSetBits(int fd, int bits);
extern _X_EXPORT int xf86SerialModemClearBits(int fd, int bits);
extern _X_EXPORT int xf86LoadKernelModule(const char *pathname);
extern _X_EXPORT void xf86OSInputThreadInit(void);

/* AGP GART interface */

typedef struct _AgpInfo {
    CARD32 bridgeId;
    CARD32 agpMode;
    unsigned long base;
    unsigned long size;
    unsigned long totalPages;
    unsigned long systemPages;
    unsigned long usedPages;
} AgpInfo, *AgpInfoPtr;

extern _X_EXPORT Bool xf86AgpGARTSupported(void);
extern _X_EXPORT AgpInfoPtr xf86GetAGPInfo(int screenNum);
extern _X_EXPORT Bool xf86AcquireGART(int screenNum);
extern _X_EXPORT Bool xf86ReleaseGART(int screenNum);
extern _X_EXPORT int xf86AllocateGARTMemory(int screenNum, unsigned long size,
                                            int type, unsigned long *physical);
extern _X_EXPORT Bool xf86DeallocateGARTMemory(int screenNum, int key);
extern _X_EXPORT Bool xf86BindGARTMemory(int screenNum, int key,
                                         unsigned long offset);
extern _X_EXPORT Bool xf86UnbindGARTMemory(int screenNum, int key);
extern _X_EXPORT Bool xf86EnableAGP(int screenNum, CARD32 mode);
extern _X_EXPORT Bool xf86GARTCloseScreen(int screenNum);

/* These routines are in shared/sigio.c and are not loaded as part of the
   module.  These routines are small, and the code if very POSIX-signal (or
   OS-signal) specific, so it seemed better to provide more complex
   wrappers than to wrap each individual function called. */
extern _X_EXPORT int xf86InstallSIGIOHandler(int fd, void (*f) (int, void *),
                                             void *);
extern _X_EXPORT int xf86RemoveSIGIOHandler(int fd);

#ifdef XF86_OS_PRIVS
typedef void (*PMClose) (void);
extern _X_EXPORT void xf86OpenConsole(void);
extern _X_EXPORT void xf86CloseConsole(void);
extern _X_HIDDEN Bool xf86VTActivate(int vtno);
extern _X_EXPORT Bool xf86VTSwitchPending(void);
extern _X_EXPORT Bool xf86VTSwitchAway(void);
extern _X_EXPORT Bool xf86VTSwitchTo(void);
extern _X_EXPORT void xf86VTRequest(int sig);
extern _X_EXPORT int xf86ProcessArgument(int, char **, int);
extern _X_EXPORT void xf86UseMsg(void);
extern _X_EXPORT PMClose xf86OSPMOpen(void);

extern _X_EXPORT void xf86InitVidMem(void);

#endif                          /* XF86_OS_PRIVS */

#ifdef XSERVER_PLATFORM_BUS
#include "hotplug.h"
void
xf86PlatformDeviceProbe(struct OdevAttributes *attribs);

void
xf86PlatformReprobeDevice(int index, struct OdevAttributes *attribs);
#endif

_XFUNCPROTOEND
#endif                          /* NO_OSLIB_PROTOTYPES */
#endif                          /* _XF86_OSPROC_H */
