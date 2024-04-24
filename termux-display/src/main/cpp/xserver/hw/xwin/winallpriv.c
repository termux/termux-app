/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
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
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Keith Packard, MIT X Consortium
 *		Harold L Hunt II
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

/* See Porting Layer Definition - p. 58 */
/*
 * Allocate indexes for the privates that we use.
 * Allocate memory directly for the screen privates.
 * Reserve space in GCs and Pixmaps for our privates.
 * Colormap privates are handled in winAllocateCmapPrivates ()
 */

Bool
winAllocatePrivates(ScreenPtr pScreen)
{
    winPrivScreenPtr pScreenPriv;

#if CYGDEBUG
    winDebug("winAllocateScreenPrivates - g_ulServerGeneration: %lu "
             "serverGeneration: %lu\n", g_ulServerGeneration, serverGeneration);
#endif

    /* We need a new slot for our privates if the screen gen has changed */
    if (g_ulServerGeneration != serverGeneration) {
        g_ulServerGeneration = serverGeneration;
    }

    /* Allocate memory for the screen private structure */
    pScreenPriv = malloc(sizeof(winPrivScreenRec));
    if (!pScreenPriv) {
        ErrorF("winAllocateScreenPrivates - malloc () failed\n");
        return FALSE;
    }

    /* Initialize the memory of the private structure */
    ZeroMemory(pScreenPriv, sizeof(winPrivScreenRec));

    /* Initialize private structure members */
    pScreenPriv->fActive = TRUE;

    /* Register our screen private */
    if (!dixRegisterPrivateKey(g_iScreenPrivateKey, PRIVATE_SCREEN, 0)) {
        ErrorF("winAllocatePrivates - AllocateScreenPrivate () failed\n");
        return FALSE;
    }

    /* Save the screen private pointer */
    winSetScreenPriv(pScreen, pScreenPriv);

    /* Reserve Pixmap memory for our privates */
    if (!dixRegisterPrivateKey
        (g_iPixmapPrivateKey, PRIVATE_PIXMAP, sizeof(winPrivPixmapRec))) {
        ErrorF("winAllocatePrivates - AllocatePixmapPrivates () failed\n");
        return FALSE;
    }

    /* Reserve Window memory for our privates */
    if (!dixRegisterPrivateKey
        (g_iWindowPrivateKey, PRIVATE_WINDOW, sizeof(winPrivWinRec))) {
        ErrorF("winAllocatePrivates () - AllocateWindowPrivates () failed\n");
        return FALSE;
    }

    return TRUE;
}

/*
 * Colormap privates may be allocated after the default colormap has
 * already been created for some screens.  This initialization procedure
 * is called for each default colormap that is found.
 */

Bool
winInitCmapPrivates(ColormapPtr pcmap, int i)
{
#if CYGDEBUG
    winDebug("winInitCmapPrivates\n");
#endif

    /*
     * I see no way that this function can do anything useful
     * with only a ColormapPtr.  We don't have the index for
     * our dev privates yet, so we can't really initialize
     * anything.  Perhaps I am misunderstanding the purpose
     * of this function.
     */
    /*  That's definitely true.
     *  I therefore changed the API and added the index as argument.
     */
    return TRUE;
}

/*
 * Allocate memory for our colormap privates
 */

Bool
winAllocateCmapPrivates(ColormapPtr pCmap)
{
    winPrivCmapPtr pCmapPriv;
    static unsigned long s_ulPrivateGeneration = 0;

#if CYGDEBUG
    winDebug("winAllocateCmapPrivates\n");
#endif

    /* Get a new privates index when the server generation changes */
    if (s_ulPrivateGeneration != serverGeneration) {
        /* Save the new server generation */
        s_ulPrivateGeneration = serverGeneration;
    }

    /* Allocate memory for our private structure */
    pCmapPriv = malloc(sizeof(winPrivCmapRec));
    if (!pCmapPriv) {
        ErrorF("winAllocateCmapPrivates - malloc () failed\n");
        return FALSE;
    }

    /* Initialize the memory of the private structure */
    ZeroMemory(pCmapPriv, sizeof(winPrivCmapRec));

    /* Register our colourmap private */
    if (!dixRegisterPrivateKey(g_iCmapPrivateKey, PRIVATE_COLORMAP, 0)) {
        ErrorF("winAllocateCmapPrivates - AllocateCmapPrivate () failed\n");
        return FALSE;
    }

    /* Save the cmap private pointer */
    winSetCmapPriv(pCmap, pCmapPriv);

#if CYGDEBUG
    winDebug("winAllocateCmapPrivates - Returning\n");
#endif

    return TRUE;
}
