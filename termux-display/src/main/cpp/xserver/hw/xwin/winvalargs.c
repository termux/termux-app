/*
 *Copyright (C) 2003-2004 Harold L Hunt II All Rights Reserved.
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
 *Except as contained in this notice, the name of Harold L Hunt II
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from Harold L Hunt II.
 *
 * Authors:	Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"
#include "winmsg.h"

/*
 * Verify all screens have been explicitly specified
 */
static BOOL
isEveryScreenExplicit(void)
{
    int i;

    for (i = 0; i < g_iNumScreens; i++)
        if (!g_ScreenInfo[i].fExplicitScreen)
            return FALSE;

    return TRUE;
}

/*
 * winValidateArgs - Look for invalid argument combinations
 */

Bool
winValidateArgs(void)
{
    int i;
    BOOL fHasNormalScreen0 = FALSE;

    /*
     * Check for a malformed set of -screen parameters.
     * Examples of malformed parameters:
     *    XWin -screen 1
     *    XWin -screen 0 -screen 2
     *    XWin -screen 1 -screen 2
     */
    if (!isEveryScreenExplicit()) {
        ErrorF("winValidateArgs - Malformed set of screen parameter(s).  "
               "Screens must be specified consecutively starting with "
               "screen 0.  That is, you cannot have only a screen 1, nor "
               "could you have screen 0 and screen 2.  You instead must "
               "have screen 0, or screen 0 and screen 1, respectively.  "
               "You can specify as many screens as you want.\n");
        return FALSE;
    }

    /* Loop through all screens */
    for (i = 0; i < g_iNumScreens; ++i) {
        /*
         * Check for any combination of
         * -multiwindow and -rootless.
         */
        {
            int iCount = 0;

            /* Count conflicting options */
            if (g_ScreenInfo[i].fMultiWindow)
                ++iCount;

            if (g_ScreenInfo[i].fRootless)
                ++iCount;

            /* Check if the first screen is without rootless and multiwindow */
            if (iCount == 0 && i == 0)
                fHasNormalScreen0 = TRUE;

            /* Fail if two or more conflicting options */
            if (iCount > 1) {
                ErrorF("winValidateArgs - Only one of -multiwindow "
                       "and -rootless can be specific at a time.\n");
                return FALSE;
            }
        }

        /* Check for -multiwindow and Xdmcp */
        /* allow xdmcp if screen 0 is normal. */
        if (g_fXdmcpEnabled && !fHasNormalScreen0 && (FALSE
                                                      || g_ScreenInfo[i].
                                                      fMultiWindow

            )
            ) {
            ErrorF("winValidateArgs - Xdmcp (-query, -broadcast, or -indirect) "
                   "is invalid with -multiwindow.\n");
            return FALSE;
        }

        /* Check for -multiwindow or -rootless and -fullscreen */
        if (g_ScreenInfo[i].fFullScreen && (FALSE
                                            || g_ScreenInfo[i].fMultiWindow
                                            || g_ScreenInfo[i].fRootless)
            ) {
            ErrorF("winValidateArgs - -fullscreen is invalid with "
                   "-multiwindow or -rootless.\n");
            return FALSE;
        }

        /* Check for -multiwindow or -rootless and -nodecoration */
        if (!g_ScreenInfo[i].fDecoration && (FALSE
                                            || g_ScreenInfo[i].fMultiWindow
                                            || g_ScreenInfo[i].fRootless)
            ) {
            ErrorF("winValidateArgs - -nodecoration is invalid with "
                   "-multiwindow or -rootless.\n");
            return FALSE;
        }

        /* Check for !fullscreen and any fullscreen-only parameters */
        if (!g_ScreenInfo[i].fFullScreen
            && (g_ScreenInfo[i].dwRefreshRate != WIN_DEFAULT_REFRESH
                || g_ScreenInfo[i].dwBPP != WIN_DEFAULT_BPP)) {
            ErrorF("winValidateArgs - -refresh and -depth are only valid "
                   "with -fullscreen.\n");
            return FALSE;
        }

        /* Check for fullscreen and any non-fullscreen parameters */
        if (g_ScreenInfo[i].fFullScreen
            && ((g_ScreenInfo[i].iResizeMode != resizeNotAllowed)
                || !g_ScreenInfo[i].fDecoration
                || g_ScreenInfo[i].fLessPointer)) {
            ErrorF("winValidateArgs - -fullscreen is invalid with "
                   "-scrollbars, -resize, -nodecoration, or -lesspointer.\n");
            return FALSE;
        }

        /* Ignore -swcursor if -multiwindow -compositewm is requested */
        if (g_ScreenInfo[i].fMultiWindow && g_ScreenInfo[i].fCompositeWM) {
            if (g_fSoftwareCursor) {
                g_fSoftwareCursor = FALSE;
                winMsg(X_WARNING, "Ignoring -swcursor due to -compositewm\n");
            }
        }
    }

    winDebug("winValidateArgs - Returning.\n");

    return TRUE;
}
