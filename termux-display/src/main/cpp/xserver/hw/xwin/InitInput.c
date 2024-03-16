/*

  Copyright 1993, 1998  The Open Group

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

*/

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"
#include "dixstruct.h"
#include "inputstr.h"

/*
 * Local function prototypes
 */

int winProcEstablishConnection(ClientPtr /* client */ );

/*
 * Local global declarations
 */

DeviceIntPtr g_pwinPointer;
DeviceIntPtr g_pwinKeyboard;

/* Called from dix/dispatch.c */
/*
 * Run through the Windows message queue(s) one more time.
 * Tell mi to dequeue the events that we have sent it.
 */
void
ProcessInputEvents(void)
{
#if 0
    ErrorF("ProcessInputEvents\n");
#endif

    mieqProcessInputEvents();

#if 0
    ErrorF("ProcessInputEvents - returning\n");
#endif
}

void
DDXRingBell(int volume, int pitch, int duration)
{
    /* winKeybdBell is used instead */
    return;
}


#ifdef HAS_DEVWINDOWS
static void
xwinDevWindowsHandlerNotify(int fd, int ready, void *data)
{
    /* This should process Windows messages, but instead all of that is delayed
     * until the wakeup handler is called.
     */
    ;
}
#endif

/* See Porting Layer Definition - p. 17 */
void
InitInput(int argc, char *argv[])
{
#if CYGDEBUG
    winDebug("InitInput\n");
#endif

    /*
     * Wrap some functions at every generation of the server.
     */
    if (InitialVector[2] != winProcEstablishConnection) {
        winProcEstablishConnectionOrig = InitialVector[2];
        InitialVector[2] = winProcEstablishConnection;
    }

    if (AllocDevicePair(serverClient, "Windows",
                        &g_pwinPointer, &g_pwinKeyboard,
                        winMouseProc, winKeybdProc,
                        FALSE) != Success)
        FatalError("InitInput - Failed to allocate slave devices.\n");

    mieqInit();

    /* Initialize the mode key states */
    winInitializeModeKeyStates();

#ifdef HAS_DEVWINDOWS
    /* Only open the windows message queue device once */
    if (g_fdMessageQueue == WIN_FD_INVALID) {
        /* Open a file descriptor for the Windows message queue */
        g_fdMessageQueue = open(WIN_MSG_QUEUE_FNAME, O_RDONLY);

        if (g_fdMessageQueue == -1) {
            FatalError("InitInput - Failed opening %s\n", WIN_MSG_QUEUE_FNAME);
        }

        /* Add the message queue as a device to wait for in WaitForSomething */
        SetNotifyFd(g_fdMessageQueue, xwinDevWindowsHandlerNotify, X_NOTIFY_READ, NULL);
    }
#endif

#if CYGDEBUG
    winDebug("InitInput - returning\n");
#endif
}

void
CloseInput(void)
{
    mieqFini();
}
