/*
 *Copyright (C) 2003-2004 Harold L Hunt II All Rights Reserved.
 *Copyright (C) Colin Harrison 2005-2008
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL HAROLD L HUNT II BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the copyright holder(s)
 *and author(s) shall not be used in advertising or otherwise to promote
 *the sale, use or other dealings in this Software without prior written
 *authorization from the copyright holder(s) and author(s).
 *
 * Authors:	Harold L Hunt II
 *              Colin Harrison
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include "win.h"
#include "dixstruct.h"

/*
 * Local function prototypes
 */

DISPATCH_PROC(winProcEstablishConnection);

/*
 * Wrapper for internal EstablishConnection function.
 * Initializes internal clients that must not be started until
 * an external client has connected.
 */

int
winProcEstablishConnection(ClientPtr client)
{
    int iReturn;
    static int s_iCallCount = 0;
    static unsigned long s_ulServerGeneration = 0;

    if (s_iCallCount == 0)
        winDebug("winProcEstablishConnection - Hello\n");

    /* Do nothing if clipboard is not enabled */
    if (!g_fClipboard) {
        ErrorF("winProcEstablishConnection - Clipboard is not enabled, "
               "returning.\n");

        /* Unwrap the original function, call it, and return */
        InitialVector[2] = winProcEstablishConnectionOrig;
        iReturn = (*winProcEstablishConnectionOrig) (client);
        winProcEstablishConnectionOrig = NULL;
        return iReturn;
    }

    /* Watch for server reset */
    if (s_ulServerGeneration != serverGeneration) {
        /* Save new generation number */
        s_ulServerGeneration = serverGeneration;

        /* Reset call count */
        s_iCallCount = 0;
    }

    /* Increment call count */
    ++s_iCallCount;

    /*
     * This procedure is only used for initialization.
     * We can unwrap the original procedure at this point
     * so that this function is no longer called until the
     * server resets and the function is wrapped again.
     */
    InitialVector[2] = winProcEstablishConnectionOrig;

    /*
     * Call original function and bail if it fails.
     * NOTE: We must do this first, since we need XdmcpOpenDisplay
     * to be called before we initialize our clipboard client.
     */
    iReturn = (*winProcEstablishConnectionOrig) (client);
    if (iReturn != 0) {
        ErrorF("winProcEstablishConnection - ProcEstablishConnection "
               "failed, bailing.\n");
        return iReturn;
    }

    /* Clear original function pointer */
    winProcEstablishConnectionOrig = NULL;

    /* Startup the clipboard client if clipboard mode is being used */
    if (g_fClipboard) {
        /*
         * NOTE: The clipboard client is started here for a reason:
         * 1) Assume you are using XDMCP (e.g. XWin -query %hostname%)
         * 2) If the clipboard client attaches during X Server startup,
         *    then it becomes the "magic client" that causes the X Server
         *    to reset if it exits.
         * 3) XDMCP calls KillAllClients when it starts up.
         * 4) The clipboard client is a client, so it is killed.
         * 5) The clipboard client is the "magic client", so the X Server
         *    resets itself.
         * 6) This repeats ad infinitum.
         * 7) We avoid this by waiting until at least one client (could
         *    be XDM, could be another client) connects, which makes it
         *    almost certain that the clipboard client will not connect
         *    until after XDM when using XDMCP.
         */

        /* Create the clipboard client thread */
        if (!winInitClipboard()) {
            ErrorF("winProcEstablishConnection - winClipboardInit "
                   "failed.\n");
            return iReturn;
        }

        ErrorF("winProcEstablishConnection - winInitClipboard returned.\n");
    }

    return iReturn;
}
