/*

Copyright (c) 2006, Red Hat, Inc.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

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

			All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

*/

/* The panoramix components contained the following notice */
/*****************************************************************

Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "misc.h"
#include "scrnintstr.h"
#include "os.h"
#include "regionstr.h"
#include "validate.h"
#include "windowstr.h"
#include "propertyst.h"
#include "input.h"
#include "inputstr.h"
#include "resource.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "servermd.h"
#include "mivalidate.h"
#ifdef PANORAMIX
#include "panoramiX.h"
#include "panoramiXsrv.h"
#endif
#include "dixevents.h"
#include "globals.h"
#include "mi.h"                 /* miPaintWindow */
#ifdef COMPOSITE
#include "compint.h"
#endif
#include "selection.h"
#include "inpututils.h"

#include "privates.h"
#include "xace.h"
#include "exevents.h"

#include <X11/Xatom.h>          /* must come after server includes */

/******
 * Window stuff for server
 *
 *    CreateRootWindow, CreateWindow, ChangeWindowAttributes,
 *    GetWindowAttributes, DeleteWindow, DestroySubWindows,
 *    HandleSaveSet, ReparentWindow, MapWindow, MapSubWindows,
 *    UnmapWindow, UnmapSubWindows, ConfigureWindow, CirculateWindow,
 *    ChangeWindowDeviceCursor
 ******/

Bool bgNoneRoot = FALSE;

static unsigned char _back_lsb[4] = { 0x88, 0x22, 0x44, 0x11 };
static unsigned char _back_msb[4] = { 0x11, 0x44, 0x22, 0x88 };

static Bool WindowParentHasDeviceCursor(WindowPtr pWin,
                                        DeviceIntPtr pDev, CursorPtr pCurs);
static Bool

WindowSeekDeviceCursor(WindowPtr pWin,
                       DeviceIntPtr pDev,
                       DevCursNodePtr * pNode, DevCursNodePtr * pPrev);

int screenIsSaved = SCREEN_SAVER_OFF;

static Bool TileScreenSaver(ScreenPtr pScreen, int kind);

#define INPUTONLY_LEGAL_MASK (CWWinGravity | CWEventMask | \
			      CWDontPropagate | CWOverrideRedirect | CWCursor )

#define BOXES_OVERLAP(b1, b2) \
      (!( ((b1)->x2 <= (b2)->x1)  || \
	( ((b1)->x1 >= (b2)->x2)) || \
	( ((b1)->y2 <= (b2)->y1)) || \
	( ((b1)->y1 >= (b2)->y2)) ) )

#define RedirectSend(pWin) \
    ((pWin->eventMask|wOtherEventMasks(pWin)) & SubstructureRedirectMask)

#define SubSend(pWin) \
    ((pWin->eventMask|wOtherEventMasks(pWin)) & SubstructureNotifyMask)

#define StrSend(pWin) \
    ((pWin->eventMask|wOtherEventMasks(pWin)) & StructureNotifyMask)

#define SubStrSend(pWin,pParent) (StrSend(pWin) || SubSend(pParent))

#ifdef COMPOSITE
static const char *overlay_win_name = "<composite overlay>";
#endif

static const char *
get_window_name(WindowPtr pWin)
{
#define WINDOW_NAME_BUF_LEN 512
    PropertyPtr prop;
    static char buf[WINDOW_NAME_BUF_LEN];
    int len;

#ifdef COMPOSITE
    CompScreenPtr comp_screen = GetCompScreen(pWin->drawable.pScreen);

    if (comp_screen && pWin == comp_screen->pOverlayWin)
        return overlay_win_name;
#endif

    for (prop = wUserProps(pWin); prop; prop = prop->next) {
        if (prop->propertyName == XA_WM_NAME && prop->type == XA_STRING &&
            prop->data) {
            len = min(prop->size, WINDOW_NAME_BUF_LEN - 1);
            memcpy(buf, prop->data, len);
            buf[len] = '\0';
            return buf;
        }
    }

    return NULL;
#undef WINDOW_NAME_BUF_LEN
}

static void
log_window_info(WindowPtr pWin, int depth)
{
    int i;
    const char *win_name, *visibility;
    BoxPtr rects;

    for (i = 0; i < (depth << 2); i++)
        ErrorF(" ");

    win_name = get_window_name(pWin);
    ErrorF("win 0x%.8x (%s), [%d, %d] to [%d, %d]",
           (unsigned) pWin->drawable.id,
           win_name ? win_name : "no name",
           pWin->drawable.x, pWin->drawable.y,
           pWin->drawable.x + pWin->drawable.width,
           pWin->drawable.y + pWin->drawable.height);

    if (pWin->overrideRedirect)
        ErrorF(" (override redirect)");
#ifdef COMPOSITE
    if (pWin->redirectDraw)
        ErrorF(" (%s compositing: pixmap %x)",
               (pWin->redirectDraw == RedirectDrawAutomatic) ?
               "automatic" : "manual",
               (unsigned) pWin->drawable.pScreen->GetWindowPixmap(pWin)->drawable.id);
#endif

    switch (pWin->visibility) {
    case VisibilityUnobscured:
        visibility = "unobscured";
        break;
    case VisibilityPartiallyObscured:
        visibility = "partially obscured";
        break;
    case VisibilityFullyObscured:
        visibility = "fully obscured";
        break;
    case VisibilityNotViewable:
        visibility = "unviewable";
        break;
    }
    ErrorF(", %s", visibility);

    if (RegionNotEmpty(&pWin->clipList)) {
        ErrorF(", clip list:");
        rects = RegionRects(&pWin->clipList);
        for (i = 0; i < RegionNumRects(&pWin->clipList); i++)
            ErrorF(" [(%d, %d) to (%d, %d)]",
                   rects[i].x1, rects[i].y1, rects[i].x2, rects[i].y2);
        ErrorF("; extents [(%d, %d) to (%d, %d)]",
               pWin->clipList.extents.x1, pWin->clipList.extents.y1,
               pWin->clipList.extents.x2, pWin->clipList.extents.y2);
    }

    ErrorF("\n");
}

static const char*
grab_grabtype_to_text(GrabPtr pGrab)
{
    switch (pGrab->grabtype) {
        case XI2:
            return "xi2";
        case CORE:
            return "core";
        default:
            return "xi1";
    }
}

static const char*
grab_type_to_text(GrabPtr pGrab)
{
    switch (pGrab->type) {
        case ButtonPress:
            return "ButtonPress";
        case KeyPress:
            return "KeyPress";
        case XI_Enter:
            return "XI_Enter";
        case XI_FocusIn:
            return "XI_FocusIn";
        default:
            return "unknown?!";
    }
}

static void
log_grab_info(void *value, XID id, void *cdata)
{
    int i, j;
    GrabPtr pGrab = (GrabPtr)value;

    ErrorF("  grab 0x%lx (%s), type '%s' on window 0x%lx\n",
           (unsigned long) pGrab->resource,
           grab_grabtype_to_text(pGrab),
           grab_type_to_text(pGrab),
           (unsigned long) pGrab->window->drawable.id);
    ErrorF("    detail %d (mask %lu), modifiersDetail %d (mask %lu)\n",
           pGrab->detail.exact,
           pGrab->detail.pMask ? (unsigned long) *(pGrab->detail.pMask) : 0,
           pGrab->modifiersDetail.exact,
           pGrab->modifiersDetail.pMask ?
           (unsigned long) *(pGrab->modifiersDetail.pMask) :
           (unsigned long) 0);
    ErrorF("    device '%s' (%d), modifierDevice '%s' (%d)\n",
           pGrab->device->name, pGrab->device->id,
           pGrab->modifierDevice->name, pGrab->modifierDevice->id);
    if (pGrab->grabtype == CORE) {
        ErrorF("    core event mask 0x%lx\n",
               (unsigned long) pGrab->eventMask);
    }
    else if (pGrab->grabtype == XI) {
        ErrorF("    xi1 event mask 0x%lx\n",
               (unsigned long) pGrab->eventMask);
    }
    else if (pGrab->grabtype == XI2) {
        for (i = 0; i < xi2mask_num_masks(pGrab->xi2mask); i++) {
            const unsigned char *mask;
            int print;

            print = 0;
            for (j = 0; j < XI2MASKSIZE; j++) {
                mask = xi2mask_get_one_mask(pGrab->xi2mask, i);
                if (mask[j]) {
                    print = 1;
                    break;
                }
            }
            if (!print)
                continue;
            ErrorF("      xi2 event mask 0x");
            for (j = 0; j < xi2mask_mask_size(pGrab->xi2mask); j++)
                ErrorF("%x ", mask[j]);
            ErrorF("\n");
        }
    }
    ErrorF("    owner-events %s, kb %d ptr %d, confine 0x%lx, cursor 0x%lx\n",
           pGrab->ownerEvents ? "true" : "false",
           pGrab->keyboardMode, pGrab->pointerMode,
           pGrab->confineTo ? (unsigned long) pGrab->confineTo->drawable.id : 0,
           pGrab->cursor ? (unsigned long) pGrab->cursor->id : 0);
}

void
PrintPassiveGrabs(void)
{
    int i;
    LocalClientCredRec *lcc;
    pid_t clientpid;
    const char *cmdname;
    const char *cmdargs;

    ErrorF("Printing all currently registered grabs\n");

    for (i = 1; i < currentMaxClients; i++) {
        if (!clients[i] || clients[i]->clientState != ClientStateRunning)
            continue;

        clientpid = GetClientPid(clients[i]);
        cmdname = GetClientCmdName(clients[i]);
        cmdargs = GetClientCmdArgs(clients[i]);
        if ((clientpid > 0) && (cmdname != NULL)) {
            ErrorF("  Printing all registered grabs of client pid %ld %s %s\n",
                   (long) clientpid, cmdname, cmdargs ? cmdargs : "");
        } else {
            if (GetLocalClientCreds(clients[i], &lcc) == -1) {
                ErrorF("  GetLocalClientCreds() failed\n");
                continue;
            }
            ErrorF("  Printing all registered grabs of client pid %ld uid %ld gid %ld\n",
                   (lcc->fieldsSet & LCC_PID_SET) ? (long) lcc->pid : 0,
                   (lcc->fieldsSet & LCC_UID_SET) ? (long) lcc->euid : 0,
                   (lcc->fieldsSet & LCC_GID_SET) ? (long) lcc->egid : 0);
            FreeLocalClientCreds(lcc);
        }

        FindClientResourcesByType(clients[i], RT_PASSIVEGRAB, log_grab_info, NULL);
    }
    ErrorF("End list of registered passive grabs\n");
}

void
PrintWindowTree(void)
{
    int scrnum, depth;
    ScreenPtr pScreen;
    WindowPtr pWin;

    for (scrnum = 0; scrnum < screenInfo.numScreens; scrnum++) {
        pScreen = screenInfo.screens[scrnum];
        ErrorF("[dix] Dumping windows for screen %d (pixmap %x):\n", scrnum,
               (unsigned) pScreen->GetScreenPixmap(pScreen)->drawable.id);
        pWin = pScreen->root;
        depth = 1;
        while (pWin) {
            log_window_info(pWin, depth);
            if (pWin->firstChild) {
                pWin = pWin->firstChild;
                depth++;
                continue;
            }
            while (pWin && !pWin->nextSib) {
                pWin = pWin->parent;
                depth--;
            }
            if (!pWin)
                break;
            pWin = pWin->nextSib;
        }
    }
}

int
TraverseTree(WindowPtr pWin, VisitWindowProcPtr func, void *data)
{
    int result;
    WindowPtr pChild;

    if (!(pChild = pWin))
        return WT_NOMATCH;
    while (1) {
        result = (*func) (pChild, data);
        if (result == WT_STOPWALKING)
            return WT_STOPWALKING;
        if ((result == WT_WALKCHILDREN) && pChild->firstChild) {
            pChild = pChild->firstChild;
            continue;
        }
        while (!pChild->nextSib && (pChild != pWin))
            pChild = pChild->parent;
        if (pChild == pWin)
            break;
        pChild = pChild->nextSib;
    }
    return WT_NOMATCH;
}

/*****
 * WalkTree
 *   Walk the window tree, for SCREEN, performing FUNC(pWin, data) on
 *   each window.  If FUNC returns WT_WALKCHILDREN, traverse the children,
 *   if it returns WT_DONTWALKCHILDREN, don't.  If it returns WT_STOPWALKING,
 *   exit WalkTree.  Does depth-first traverse.
 *****/

int
WalkTree(ScreenPtr pScreen, VisitWindowProcPtr func, void *data)
{
    return (TraverseTree(pScreen->root, func, data));
}

/* hack to force no backing store */
Bool disableBackingStore = FALSE;
Bool enableBackingStore = FALSE;

static void
SetWindowToDefaults(WindowPtr pWin)
{
    pWin->prevSib = NullWindow;
    pWin->firstChild = NullWindow;
    pWin->lastChild = NullWindow;

    pWin->valdata = NULL;
    pWin->optional = NULL;
    pWin->cursorIsNone = TRUE;

    pWin->backingStore = NotUseful;

    pWin->mapped = FALSE;       /* off */
    pWin->realized = FALSE;     /* off */
    pWin->viewable = FALSE;
    pWin->visibility = VisibilityNotViewable;
    pWin->overrideRedirect = FALSE;
    pWin->saveUnder = FALSE;

    pWin->bitGravity = ForgetGravity;
    pWin->winGravity = NorthWestGravity;

    pWin->eventMask = 0;
    pWin->deliverableEvents = 0;
    pWin->dontPropagate = 0;
    pWin->redirectDraw = RedirectDrawNone;
    pWin->forcedBG = FALSE;
    pWin->unhittable = FALSE;

#ifdef COMPOSITE
    pWin->damagedDescendants = FALSE;
#endif
}

static void
MakeRootTile(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    GCPtr pGC;
    unsigned char back[128];
    int len = BitmapBytePad(sizeof(long));
    unsigned char *from, *to;
    int i, j;

    pWin->background.pixmap = (*pScreen->CreatePixmap) (pScreen, 4, 4,
                                                        pScreen->rootDepth, 0);

    pWin->backgroundState = BackgroundPixmap;
    pGC = GetScratchGC(pScreen->rootDepth, pScreen);
    if (!pWin->background.pixmap || !pGC)
        FatalError("could not create root tile");

    {
        ChangeGCVal attributes[2];

        attributes[0].val = pScreen->whitePixel;
        attributes[1].val = pScreen->blackPixel;

        (void) ChangeGC(NullClient, pGC, GCForeground | GCBackground,
                        attributes);
    }

    ValidateGC((DrawablePtr) pWin->background.pixmap, pGC);

    from = (screenInfo.bitmapBitOrder == LSBFirst) ? _back_lsb : _back_msb;
    to = back;

    for (i = 4; i > 0; i--, from++)
        for (j = len; j > 0; j--)
            *to++ = *from;

    (*pGC->ops->PutImage) ((DrawablePtr) pWin->background.pixmap, pGC, 1,
                           0, 0, len, 4, 0, XYBitmap, (char *) back);

    FreeScratchGC(pGC);

}

/*****
 * CreateRootWindow
 *    Makes a window at initialization time for specified screen
 *****/

Bool
CreateRootWindow(ScreenPtr pScreen)
{
    WindowPtr pWin;
    BoxRec box;
    PixmapFormatRec *format;

    pWin = dixAllocateScreenObjectWithPrivates(pScreen, WindowRec, PRIVATE_WINDOW);
    if (!pWin)
        return FALSE;

    pScreen->screensaver.pWindow = NULL;
    pScreen->screensaver.wid = FakeClientID(0);
    pScreen->screensaver.ExternalScreenSaver = NULL;
    screenIsSaved = SCREEN_SAVER_OFF;

    pScreen->root = pWin;

    pWin->drawable.pScreen = pScreen;
    pWin->drawable.type = DRAWABLE_WINDOW;

    pWin->drawable.depth = pScreen->rootDepth;
    for (format = screenInfo.formats;
         format->depth != pScreen->rootDepth; format++);
    pWin->drawable.bitsPerPixel = format->bitsPerPixel;

    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;

    pWin->parent = NullWindow;
    SetWindowToDefaults(pWin);

    pWin->optional = malloc(sizeof(WindowOptRec));
    if (!pWin->optional)
        return FALSE;

    pWin->optional->dontPropagateMask = 0;
    pWin->optional->otherEventMasks = 0;
    pWin->optional->otherClients = NULL;
    pWin->optional->passiveGrabs = NULL;
    pWin->optional->userProps = NULL;
    pWin->optional->backingBitPlanes = ~0L;
    pWin->optional->backingPixel = 0;
    pWin->optional->boundingShape = NULL;
    pWin->optional->clipShape = NULL;
    pWin->optional->inputShape = NULL;
    pWin->optional->inputMasks = NULL;
    pWin->optional->deviceCursors = NULL;
    pWin->optional->colormap = pScreen->defColormap;
    pWin->optional->visual = pScreen->rootVisual;

    pWin->nextSib = NullWindow;

    pWin->drawable.id = FakeClientID(0);

    pWin->origin.x = pWin->origin.y = 0;
    pWin->drawable.height = pScreen->height;
    pWin->drawable.width = pScreen->width;
    pWin->drawable.x = pWin->drawable.y = 0;

    box.x1 = 0;
    box.y1 = 0;
    box.x2 = pScreen->width;
    box.y2 = pScreen->height;
    RegionInit(&pWin->clipList, &box, 1);
    RegionInit(&pWin->winSize, &box, 1);
    RegionInit(&pWin->borderSize, &box, 1);
    RegionInit(&pWin->borderClip, &box, 1);

    pWin->drawable.class = InputOutput;
    pWin->optional->visual = pScreen->rootVisual;

    pWin->backgroundState = BackgroundPixel;
    pWin->background.pixel = pScreen->whitePixel;

    pWin->borderIsPixel = TRUE;
    pWin->border.pixel = pScreen->blackPixel;
    pWin->borderWidth = 0;

    /*  security creation/labeling check
     */
    if (XaceHook(XACE_RESOURCE_ACCESS, serverClient, pWin->drawable.id,
                 RT_WINDOW, pWin, RT_NONE, NULL, DixCreateAccess))
        return FALSE;

    if (!AddResource(pWin->drawable.id, RT_WINDOW, (void *) pWin))
        return FALSE;

    if (disableBackingStore)
        pScreen->backingStoreSupport = NotUseful;
    if (enableBackingStore)
        pScreen->backingStoreSupport = WhenMapped;
#ifdef COMPOSITE
    if (noCompositeExtension)
        pScreen->backingStoreSupport = NotUseful;
#endif

    pScreen->saveUnderSupport = NotUseful;

    return TRUE;
}

void
InitRootWindow(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    int backFlag = CWBorderPixel | CWCursor | CWBackingStore;

    if (!(*pScreen->CreateWindow) (pWin))
        return;                 /* XXX */
    (*pScreen->PositionWindow) (pWin, 0, 0);

    pWin->cursorIsNone = FALSE;
    pWin->optional->cursor = RefCursor(rootCursor);

    if (party_like_its_1989) {
        MakeRootTile(pWin);
        backFlag |= CWBackPixmap;
    }
    else if (pScreen->canDoBGNoneRoot && bgNoneRoot) {
        pWin->backgroundState = XaceBackgroundNoneState(pWin);
        pWin->background.pixel = pScreen->whitePixel;
        backFlag |= CWBackPixmap;
    }
    else {
        pWin->backgroundState = BackgroundPixel;
        if (whiteRoot)
            pWin->background.pixel = pScreen->whitePixel;
        else
            pWin->background.pixel = pScreen->blackPixel;
        backFlag |= CWBackPixel;
    }

    pWin->backingStore = NotUseful;
    /* We SHOULD check for an error value here XXX */
    (*pScreen->ChangeWindowAttributes) (pWin, backFlag);

    MapWindow(pWin, serverClient);
}

/* Set the region to the intersection of the rectangle and the
 * window's winSize.  The window is typically the parent of the
 * window from which the region came.
 */

static void
ClippedRegionFromBox(WindowPtr pWin, RegionPtr Rgn, int x, int y, int w, int h)
{
    BoxRec box = *RegionExtents(&pWin->winSize);

    /* we do these calculations to avoid overflows */
    if (x > box.x1)
        box.x1 = x;
    if (y > box.y1)
        box.y1 = y;
    x += w;
    if (x < box.x2)
        box.x2 = x;
    y += h;
    if (y < box.y2)
        box.y2 = y;
    if (box.x1 > box.x2)
        box.x2 = box.x1;
    if (box.y1 > box.y2)
        box.y2 = box.y1;
    RegionReset(Rgn, &box);
    RegionIntersect(Rgn, Rgn, &pWin->winSize);
}

static RealChildHeadProc realChildHeadProc = NULL;

void
RegisterRealChildHeadProc(RealChildHeadProc proc)
{
    realChildHeadProc = proc;
}

WindowPtr
RealChildHead(WindowPtr pWin)
{
    if (realChildHeadProc) {
        return realChildHeadProc(pWin);
    }

    if (!pWin->parent &&
        (screenIsSaved == SCREEN_SAVER_ON) &&
        (HasSaverWindow(pWin->drawable.pScreen)))
        return pWin->firstChild;
    else
        return NullWindow;
}

/*****
 * CreateWindow
 *    Makes a window in response to client request
 *****/

WindowPtr
CreateWindow(Window wid, WindowPtr pParent, int x, int y, unsigned w,
             unsigned h, unsigned bw, unsigned class, Mask vmask, XID *vlist,
             int depth, ClientPtr client, VisualID visual, int *error)
{
    WindowPtr pWin;
    WindowPtr pHead;
    ScreenPtr pScreen;
    int idepth, ivisual;
    Bool fOK;
    DepthPtr pDepth;
    PixmapFormatRec *format;
    WindowOptPtr ancwopt;

    if (class == CopyFromParent)
        class = pParent->drawable.class;

    if ((class != InputOutput) && (class != InputOnly)) {
        *error = BadValue;
        client->errorValue = class;
        return NullWindow;
    }

    if ((class != InputOnly) && (pParent->drawable.class == InputOnly)) {
        *error = BadMatch;
        return NullWindow;
    }

    if ((class == InputOnly) && ((bw != 0) || (depth != 0))) {
        *error = BadMatch;
        return NullWindow;
    }

    pScreen = pParent->drawable.pScreen;
    if ((class == InputOutput) && (depth == 0))
        depth = pParent->drawable.depth;
    ancwopt = pParent->optional;
    if (!ancwopt)
        ancwopt = FindWindowWithOptional(pParent)->optional;
    if (visual == CopyFromParent) {
        visual = ancwopt->visual;
    }

    /* Find out if the depth and visual are acceptable for this Screen */
    if ((visual != ancwopt->visual) || (depth != pParent->drawable.depth)) {
        fOK = FALSE;
        for (idepth = 0; idepth < pScreen->numDepths; idepth++) {
            pDepth = (DepthPtr) &pScreen->allowedDepths[idepth];
            if ((depth == pDepth->depth) || (depth == 0)) {
                for (ivisual = 0; ivisual < pDepth->numVids; ivisual++) {
                    if (visual == pDepth->vids[ivisual]) {
                        fOK = TRUE;
                        break;
                    }
                }
            }
        }
        if (fOK == FALSE) {
            *error = BadMatch;
            return NullWindow;
        }
    }

    if (((vmask & (CWBorderPixmap | CWBorderPixel)) == 0) &&
        (class != InputOnly) && (depth != pParent->drawable.depth)) {
        *error = BadMatch;
        return NullWindow;
    }

    if (((vmask & CWColormap) == 0) &&
        (class != InputOnly) &&
        ((visual != ancwopt->visual) || (ancwopt->colormap == None))) {
        *error = BadMatch;
        return NullWindow;
    }

    pWin = dixAllocateScreenObjectWithPrivates(pScreen, WindowRec, PRIVATE_WINDOW);
    if (!pWin) {
        *error = BadAlloc;
        return NullWindow;
    }
    pWin->drawable = pParent->drawable;
    pWin->drawable.depth = depth;
    if (depth == pParent->drawable.depth)
        pWin->drawable.bitsPerPixel = pParent->drawable.bitsPerPixel;
    else {
        for (format = screenInfo.formats; format->depth != depth; format++);
        pWin->drawable.bitsPerPixel = format->bitsPerPixel;
    }
    if (class == InputOnly)
        pWin->drawable.type = (short) UNDRAWABLE_WINDOW;
    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;

    pWin->drawable.id = wid;
    pWin->drawable.class = class;

    pWin->parent = pParent;
    SetWindowToDefaults(pWin);

    if (visual != ancwopt->visual) {
        if (!MakeWindowOptional(pWin)) {
            dixFreeObjectWithPrivates(pWin, PRIVATE_WINDOW);
            *error = BadAlloc;
            return NullWindow;
        }
        pWin->optional->visual = visual;
        pWin->optional->colormap = None;
    }

    pWin->borderWidth = bw;

    /*  security creation/labeling check
     */
    *error = XaceHook(XACE_RESOURCE_ACCESS, client, wid, RT_WINDOW, pWin,
                      RT_WINDOW, pWin->parent,
                      DixCreateAccess | DixSetAttrAccess);
    if (*error != Success) {
        dixFreeObjectWithPrivates(pWin, PRIVATE_WINDOW);
        return NullWindow;
    }

    pWin->backgroundState = XaceBackgroundNoneState(pWin);
    pWin->background.pixel = pScreen->whitePixel;

    pWin->borderIsPixel = pParent->borderIsPixel;
    pWin->border = pParent->border;
    if (pWin->borderIsPixel == FALSE)
        pWin->border.pixmap->refcnt++;

    pWin->origin.x = x + (int) bw;
    pWin->origin.y = y + (int) bw;
    pWin->drawable.width = w;
    pWin->drawable.height = h;
    pWin->drawable.x = pParent->drawable.x + x + (int) bw;
    pWin->drawable.y = pParent->drawable.y + y + (int) bw;

    /* set up clip list correctly for unobscured WindowPtr */
    RegionNull(&pWin->clipList);
    RegionNull(&pWin->borderClip);
    RegionNull(&pWin->winSize);
    RegionNull(&pWin->borderSize);

    pHead = RealChildHead(pParent);
    if (pHead) {
        pWin->nextSib = pHead->nextSib;
        if (pHead->nextSib)
            pHead->nextSib->prevSib = pWin;
        else
            pParent->lastChild = pWin;
        pHead->nextSib = pWin;
        pWin->prevSib = pHead;
    }
    else {
        pWin->nextSib = pParent->firstChild;
        if (pParent->firstChild)
            pParent->firstChild->prevSib = pWin;
        else
            pParent->lastChild = pWin;
        pParent->firstChild = pWin;
    }

    SetWinSize(pWin);
    SetBorderSize(pWin);

    /* We SHOULD check for an error value here XXX */
    if (!(*pScreen->CreateWindow) (pWin)) {
        *error = BadAlloc;
        DeleteWindow(pWin, None);
        return NullWindow;
    }
    /* We SHOULD check for an error value here XXX */
    (*pScreen->PositionWindow) (pWin, pWin->drawable.x, pWin->drawable.y);

    if (!(vmask & CWEventMask))
        RecalculateDeliverableEvents(pWin);

    if (vmask)
        *error = ChangeWindowAttributes(pWin, vmask, vlist, wClient(pWin));
    else
        *error = Success;

    if (*error != Success) {
        DeleteWindow(pWin, None);
        return NullWindow;
    }

    if (SubSend(pParent)) {
        xEvent event = {
            .u.createNotify.window = wid,
            .u.createNotify.parent = pParent->drawable.id,
            .u.createNotify.x = x,
            .u.createNotify.y = y,
            .u.createNotify.width = w,
            .u.createNotify.height = h,
            .u.createNotify.borderWidth = bw,
            .u.createNotify.override = pWin->overrideRedirect
        };
        event.u.u.type = CreateNotify;
        DeliverEvents(pParent, &event, 1, NullWindow);
    }
    return pWin;
}

static void
DisposeWindowOptional(WindowPtr pWin)
{
    if (!pWin->optional)
        return;
    /*
     * everything is peachy.  Delete the optional record
     * and clean up
     */
    if (pWin->optional->cursor) {
        FreeCursor(pWin->optional->cursor, (Cursor) 0);
        pWin->cursorIsNone = FALSE;
    }
    else
        pWin->cursorIsNone = TRUE;

    if (pWin->optional->deviceCursors) {
        DevCursorList pList;
        DevCursorList pPrev;

        pList = pWin->optional->deviceCursors;
        while (pList) {
            if (pList->cursor)
                FreeCursor(pList->cursor, (XID) 0);
            pPrev = pList;
            pList = pList->next;
            free(pPrev);
        }
        pWin->optional->deviceCursors = NULL;
    }

    free(pWin->optional);
    pWin->optional = NULL;
}

static void
FreeWindowResources(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;

    DeleteWindowFromAnySaveSet(pWin);
    DeleteWindowFromAnySelections(pWin);
    DeleteWindowFromAnyEvents(pWin, TRUE);
    RegionUninit(&pWin->clipList);
    RegionUninit(&pWin->winSize);
    RegionUninit(&pWin->borderClip);
    RegionUninit(&pWin->borderSize);
    if (wBoundingShape(pWin))
        RegionDestroy(wBoundingShape(pWin));
    if (wClipShape(pWin))
        RegionDestroy(wClipShape(pWin));
    if (wInputShape(pWin))
        RegionDestroy(wInputShape(pWin));
    if (pWin->borderIsPixel == FALSE)
        (*pScreen->DestroyPixmap) (pWin->border.pixmap);
    if (pWin->backgroundState == BackgroundPixmap)
        (*pScreen->DestroyPixmap) (pWin->background.pixmap);

    DeleteAllWindowProperties(pWin);
    /* We SHOULD check for an error value here XXX */
    (*pScreen->DestroyWindow) (pWin);
    DisposeWindowOptional(pWin);
}

static void
CrushTree(WindowPtr pWin)
{
    WindowPtr pChild, pSib, pParent;
    UnrealizeWindowProcPtr UnrealizeWindow;

    if (!(pChild = pWin->firstChild))
        return;
    UnrealizeWindow = pWin->drawable.pScreen->UnrealizeWindow;
    while (1) {
        if (pChild->firstChild) {
            pChild = pChild->firstChild;
            continue;
        }
        while (1) {
            pParent = pChild->parent;
            if (SubStrSend(pChild, pParent)) {
                xEvent event = { .u.u.type = DestroyNotify };
                event.u.destroyNotify.window = pChild->drawable.id;
                DeliverEvents(pChild, &event, 1, NullWindow);
            }
            FreeResource(pChild->drawable.id, RT_WINDOW);
            pSib = pChild->nextSib;
            pChild->viewable = FALSE;
            if (pChild->realized) {
                pChild->realized = FALSE;
                (*UnrealizeWindow) (pChild);
            }
            FreeWindowResources(pChild);
            dixFreeObjectWithPrivates(pChild, PRIVATE_WINDOW);
            if ((pChild = pSib))
                break;
            pChild = pParent;
            pChild->firstChild = NullWindow;
            pChild->lastChild = NullWindow;
            if (pChild == pWin)
                return;
        }
    }
}

/*****
 *  DeleteWindow
 *	 Deletes child of window then window itself
 *	 If wid is None, don't send any events
 *****/

int
DeleteWindow(void *value, XID wid)
{
    WindowPtr pParent;
    WindowPtr pWin = (WindowPtr) value;

    UnmapWindow(pWin, FALSE);

    CrushTree(pWin);

    pParent = pWin->parent;
    if (wid && pParent && SubStrSend(pWin, pParent)) {
        xEvent event = { .u.u.type = DestroyNotify };
        event.u.destroyNotify.window = pWin->drawable.id;
        DeliverEvents(pWin, &event, 1, NullWindow);
    }

    FreeWindowResources(pWin);
    if (pParent) {
        if (pParent->firstChild == pWin)
            pParent->firstChild = pWin->nextSib;
        if (pParent->lastChild == pWin)
            pParent->lastChild = pWin->prevSib;
        if (pWin->nextSib)
            pWin->nextSib->prevSib = pWin->prevSib;
        if (pWin->prevSib)
            pWin->prevSib->nextSib = pWin->nextSib;
    }
    else
        pWin->drawable.pScreen->root = NULL;
    dixFreeObjectWithPrivates(pWin, PRIVATE_WINDOW);
    return Success;
}

int
DestroySubwindows(WindowPtr pWin, ClientPtr client)
{
    /* XXX
     * The protocol is quite clear that each window should be
     * destroyed in turn, however, unmapping all of the first
     * eliminates most of the calls to ValidateTree.  So,
     * this implementation is incorrect in that all of the
     * UnmapNotifies occur before all of the DestroyNotifies.
     * If you care, simply delete the call to UnmapSubwindows.
     */
    UnmapSubwindows(pWin);
    while (pWin->lastChild) {
        int rc = XaceHook(XACE_RESOURCE_ACCESS, client,
                          pWin->lastChild->drawable.id, RT_WINDOW,
                          pWin->lastChild, RT_NONE, NULL, DixDestroyAccess);

        if (rc != Success)
            return rc;
        FreeResource(pWin->lastChild->drawable.id, RT_NONE);
    }
    return Success;
}

static void
SetRootWindowBackground(WindowPtr pWin, ScreenPtr pScreen, Mask *index2)
{
    /* following the protocol: "Changing the background of a root window to
     * None or ParentRelative restores the default background pixmap" */
    if (bgNoneRoot) {
        pWin->backgroundState = XaceBackgroundNoneState(pWin);
        pWin->background.pixel = pScreen->whitePixel;
    }
    else if (party_like_its_1989)
        MakeRootTile(pWin);
    else {
        pWin->backgroundState = BackgroundPixel;
        if (whiteRoot)
            pWin->background.pixel = pScreen->whitePixel;
        else
            pWin->background.pixel = pScreen->blackPixel;
        *index2 = CWBackPixel;
    }
}

/*****
 *  ChangeWindowAttributes
 *
 *  The value-mask specifies which attributes are to be changed; the
 *  value-list contains one value for each one bit in the mask, from least
 *  to most significant bit in the mask.
 *****/

int
ChangeWindowAttributes(WindowPtr pWin, Mask vmask, XID *vlist, ClientPtr client)
{
    XID *pVlist;
    PixmapPtr pPixmap;
    Pixmap pixID;
    CursorPtr pCursor, pOldCursor;
    Cursor cursorID;
    WindowPtr pChild;
    Colormap cmap;
    ColormapPtr pCmap;
    xEvent xE;
    int error, rc;
    ScreenPtr pScreen;
    Mask index2, tmask, vmaskCopy = 0;
    unsigned int val;
    Bool checkOptional = FALSE, borderRelative = FALSE;

    if ((pWin->drawable.class == InputOnly) &&
        (vmask & (~INPUTONLY_LEGAL_MASK)))
        return BadMatch;

    error = Success;
    pScreen = pWin->drawable.pScreen;
    pVlist = vlist;
    tmask = vmask;
    while (tmask) {
        index2 = (Mask) lowbit(tmask);
        tmask &= ~index2;
        switch (index2) {
        case CWBackPixmap:
            pixID = (Pixmap) * pVlist;
            pVlist++;
            if (pWin->backgroundState == ParentRelative)
                borderRelative = TRUE;
            if (pixID == None) {
                if (pWin->backgroundState == BackgroundPixmap)
                    (*pScreen->DestroyPixmap) (pWin->background.pixmap);
                if (!pWin->parent)
                    SetRootWindowBackground(pWin, pScreen, &index2);
                else {
                    pWin->backgroundState = XaceBackgroundNoneState(pWin);
                    pWin->background.pixel = pScreen->whitePixel;
                }
            }
            else if (pixID == ParentRelative) {
                if (pWin->parent &&
                    pWin->drawable.depth != pWin->parent->drawable.depth) {
                    error = BadMatch;
                    goto PatchUp;
                }
                if (pWin->backgroundState == BackgroundPixmap)
                    (*pScreen->DestroyPixmap) (pWin->background.pixmap);
                if (!pWin->parent)
                    SetRootWindowBackground(pWin, pScreen, &index2);
                else
                    pWin->backgroundState = ParentRelative;
                borderRelative = TRUE;
                /* Note that the parent's backgroundTile's refcnt is NOT
                 * incremented. */
            }
            else {
                rc = dixLookupResourceByType((void **) &pPixmap, pixID,
                                             RT_PIXMAP, client, DixReadAccess);
                if (rc == Success) {
                    if ((pPixmap->drawable.depth != pWin->drawable.depth) ||
                        (pPixmap->drawable.pScreen != pScreen)) {
                        error = BadMatch;
                        goto PatchUp;
                    }
                    if (pWin->backgroundState == BackgroundPixmap)
                        (*pScreen->DestroyPixmap) (pWin->background.pixmap);
                    pWin->backgroundState = BackgroundPixmap;
                    pWin->background.pixmap = pPixmap;
                    pPixmap->refcnt++;
                }
                else {
                    error = rc;
                    client->errorValue = pixID;
                    goto PatchUp;
                }
            }
            break;
        case CWBackPixel:
            if (pWin->backgroundState == ParentRelative)
                borderRelative = TRUE;
            if (pWin->backgroundState == BackgroundPixmap)
                (*pScreen->DestroyPixmap) (pWin->background.pixmap);
            pWin->backgroundState = BackgroundPixel;
            pWin->background.pixel = (CARD32) *pVlist;
            /* background pixel overrides background pixmap,
               so don't let the ddx layer see both bits */
            vmaskCopy &= ~CWBackPixmap;
            pVlist++;
            break;
        case CWBorderPixmap:
            pixID = (Pixmap) * pVlist;
            pVlist++;
            if (pixID == CopyFromParent) {
                if (!pWin->parent ||
                    (pWin->drawable.depth != pWin->parent->drawable.depth)) {
                    error = BadMatch;
                    goto PatchUp;
                }
                if (pWin->parent->borderIsPixel == TRUE) {
                    if (pWin->borderIsPixel == FALSE)
                        (*pScreen->DestroyPixmap) (pWin->border.pixmap);
                    pWin->border = pWin->parent->border;
                    pWin->borderIsPixel = TRUE;
                    index2 = CWBorderPixel;
                    break;
                }
                else {
                    pixID = pWin->parent->border.pixmap->drawable.id;
                }
            }
            rc = dixLookupResourceByType((void **) &pPixmap, pixID, RT_PIXMAP,
                                         client, DixReadAccess);
            if (rc == Success) {
                if ((pPixmap->drawable.depth != pWin->drawable.depth) ||
                    (pPixmap->drawable.pScreen != pScreen)) {
                    error = BadMatch;
                    goto PatchUp;
                }
                if (pWin->borderIsPixel == FALSE)
                    (*pScreen->DestroyPixmap) (pWin->border.pixmap);
                pWin->borderIsPixel = FALSE;
                pWin->border.pixmap = pPixmap;
                pPixmap->refcnt++;
            }
            else {
                error = rc;
                client->errorValue = pixID;
                goto PatchUp;
            }
            break;
        case CWBorderPixel:
            if (pWin->borderIsPixel == FALSE)
                (*pScreen->DestroyPixmap) (pWin->border.pixmap);
            pWin->borderIsPixel = TRUE;
            pWin->border.pixel = (CARD32) *pVlist;
            /* border pixel overrides border pixmap,
               so don't let the ddx layer see both bits */
            vmaskCopy &= ~CWBorderPixmap;
            pVlist++;
            break;
        case CWBitGravity:
            val = (CARD8) *pVlist;
            pVlist++;
            if (val > StaticGravity) {
                error = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            pWin->bitGravity = val;
            break;
        case CWWinGravity:
            val = (CARD8) *pVlist;
            pVlist++;
            if (val > StaticGravity) {
                error = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            pWin->winGravity = val;
            break;
        case CWBackingStore:
            val = (CARD8) *pVlist;
            pVlist++;
            if ((val != NotUseful) && (val != WhenMapped) && (val != Always)) {
                error = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            /* if we're not actually changing the window's state, hide
             * CWBackingStore from vmaskCopy so it doesn't get passed to
             * ->ChangeWindowAttributes below
             */
            if (pWin->backingStore == val)
                continue;
            pWin->backingStore = val;
            break;
        case CWBackingPlanes:
            if (pWin->optional || ((CARD32) *pVlist != (CARD32) ~0L)) {
                if (!pWin->optional && !MakeWindowOptional(pWin)) {
                    error = BadAlloc;
                    goto PatchUp;
                }
                pWin->optional->backingBitPlanes = (CARD32) *pVlist;
                if ((CARD32) *pVlist == (CARD32) ~0L)
                    checkOptional = TRUE;
            }
            pVlist++;
            break;
        case CWBackingPixel:
            if (pWin->optional || (CARD32) *pVlist) {
                if (!pWin->optional && !MakeWindowOptional(pWin)) {
                    error = BadAlloc;
                    goto PatchUp;
                }
                pWin->optional->backingPixel = (CARD32) *pVlist;
                if (!*pVlist)
                    checkOptional = TRUE;
            }
            pVlist++;
            break;
        case CWSaveUnder:
            val = (BOOL) * pVlist;
            pVlist++;
            if ((val != xTrue) && (val != xFalse)) {
                error = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            pWin->saveUnder = val;
            break;
        case CWEventMask:
            rc = EventSelectForWindow(pWin, client, (Mask) *pVlist);
            if (rc) {
                error = rc;
                goto PatchUp;
            }
            pVlist++;
            break;
        case CWDontPropagate:
            rc = EventSuppressForWindow(pWin, client, (Mask) *pVlist,
                                        &checkOptional);
            if (rc) {
                error = rc;
                goto PatchUp;
            }
            pVlist++;
            break;
        case CWOverrideRedirect:
            val = (BOOL) * pVlist;
            pVlist++;
            if ((val != xTrue) && (val != xFalse)) {
                error = BadValue;
                client->errorValue = val;
                goto PatchUp;
            }
            if (val == xTrue) {
                rc = XaceHook(XACE_RESOURCE_ACCESS, client, pWin->drawable.id,
                              RT_WINDOW, pWin, RT_NONE, NULL, DixGrabAccess);
                if (rc != Success) {
                    error = rc;
                    client->errorValue = pWin->drawable.id;
                    goto PatchUp;
                }
            }
            pWin->overrideRedirect = val;
            break;
        case CWColormap:
            cmap = (Colormap) * pVlist;
            pVlist++;
            if (cmap == CopyFromParent) {
                if (pWin->parent &&
                    (!pWin->optional ||
                     pWin->optional->visual == wVisual(pWin->parent))) {
                    cmap = wColormap(pWin->parent);
                }
                else
                    cmap = None;
            }
            if (cmap == None) {
                error = BadMatch;
                goto PatchUp;
            }
            rc = dixLookupResourceByType((void **) &pCmap, cmap, RT_COLORMAP,
                                         client, DixUseAccess);
            if (rc != Success) {
                error = rc;
                client->errorValue = cmap;
                goto PatchUp;
            }
            if (pCmap->pVisual->vid != wVisual(pWin) ||
                pCmap->pScreen != pScreen) {
                error = BadMatch;
                goto PatchUp;
            }
            if (cmap != wColormap(pWin)) {
                if (!pWin->optional) {
                    if (!MakeWindowOptional(pWin)) {
                        error = BadAlloc;
                        goto PatchUp;
                    }
                }
                else if (pWin->parent && cmap == wColormap(pWin->parent))
                    checkOptional = TRUE;

                /*
                 * propagate the original colormap to any children
                 * inheriting it
                 */

                for (pChild = pWin->firstChild; pChild;
                     pChild = pChild->nextSib) {
                    if (!pChild->optional && !MakeWindowOptional(pChild)) {
                        error = BadAlloc;
                        goto PatchUp;
                    }
                }

                pWin->optional->colormap = cmap;

                /*
                 * check on any children now matching the new colormap
                 */

                for (pChild = pWin->firstChild; pChild;
                     pChild = pChild->nextSib) {
                    if (pChild->optional->colormap == cmap)
                        CheckWindowOptionalNeed(pChild);
                }

                xE = (xEvent) {
                    .u.colormap.window = pWin->drawable.id,
                    .u.colormap.colormap = cmap,
                    .u.colormap.new = xTrue,
                    .u.colormap.state = IsMapInstalled(cmap, pWin)
                };
                xE.u.u.type = ColormapNotify;
                DeliverEvents(pWin, &xE, 1, NullWindow);
            }
            break;
        case CWCursor:
            cursorID = (Cursor) * pVlist;
            pVlist++;
            /*
             * install the new
             */
            if (cursorID == None) {
                if (pWin == pWin->drawable.pScreen->root)
                    pCursor = rootCursor;
                else
                    pCursor = (CursorPtr) None;
            }
            else {
                rc = dixLookupResourceByType((void **) &pCursor, cursorID,
                                             RT_CURSOR, client, DixUseAccess);
                if (rc != Success) {
                    error = rc;
                    client->errorValue = cursorID;
                    goto PatchUp;
                }
            }

            if (pCursor != wCursor(pWin)) {
                /*
                 * patch up child windows so they don't lose cursors.
                 */

                for (pChild = pWin->firstChild; pChild;
                     pChild = pChild->nextSib) {
                    if (!pChild->optional && !pChild->cursorIsNone &&
                        !MakeWindowOptional(pChild)) {
                        error = BadAlloc;
                        goto PatchUp;
                    }
                }

                pOldCursor = 0;
                if (pCursor == (CursorPtr) None) {
                    pWin->cursorIsNone = TRUE;
                    if (pWin->optional) {
                        pOldCursor = pWin->optional->cursor;
                        pWin->optional->cursor = (CursorPtr) None;
                        checkOptional = TRUE;
                    }
                }
                else {
                    if (!pWin->optional) {
                        if (!MakeWindowOptional(pWin)) {
                            error = BadAlloc;
                            goto PatchUp;
                        }
                    }
                    else if (pWin->parent && pCursor == wCursor(pWin->parent))
                        checkOptional = TRUE;
                    pOldCursor = pWin->optional->cursor;
                    pWin->optional->cursor = RefCursor(pCursor);
                    pWin->cursorIsNone = FALSE;
                    /*
                     * check on any children now matching the new cursor
                     */

                    for (pChild = pWin->firstChild; pChild;
                         pChild = pChild->nextSib) {
                        if (pChild->optional &&
                            (pChild->optional->cursor == pCursor))
                            CheckWindowOptionalNeed(pChild);
                    }
                }

                CursorVisible = TRUE;

                if (pWin->realized)
                    WindowHasNewCursor(pWin);

                /* Can't free cursor until here - old cursor
                 * is needed in WindowHasNewCursor
                 */
                if (pOldCursor)
                    FreeCursor(pOldCursor, (Cursor) 0);
            }
            break;
        default:
            error = BadValue;
            client->errorValue = vmask;
            goto PatchUp;
        }
        vmaskCopy |= index2;
    }
 PatchUp:
    if (checkOptional)
        CheckWindowOptionalNeed(pWin);

    /* We SHOULD check for an error value here XXX */
    (*pScreen->ChangeWindowAttributes) (pWin, vmaskCopy);

    /*
       If the border contents have changed, redraw the border.
       Note that this has to be done AFTER pScreen->ChangeWindowAttributes
       for the tile to be rotated, and the correct function selected.
     */
    if (((vmaskCopy & (CWBorderPixel | CWBorderPixmap)) || borderRelative)
        && pWin->viewable && HasBorder(pWin)) {
        RegionRec exposed;

        RegionNull(&exposed);
        RegionSubtract(&exposed, &pWin->borderClip, &pWin->winSize);
        pWin->drawable.pScreen->PaintWindow(pWin, &exposed, PW_BORDER);
        RegionUninit(&exposed);
    }
    return error;
}

/*****
 * GetWindowAttributes
 *    Notice that this is different than ChangeWindowAttributes
 *****/

void
GetWindowAttributes(WindowPtr pWin, ClientPtr client,
                    xGetWindowAttributesReply * wa)
{
    wa->type = X_Reply;
    wa->bitGravity = pWin->bitGravity;
    wa->winGravity = pWin->winGravity;
    wa->backingStore = pWin->backingStore;
    wa->length = bytes_to_int32(sizeof(xGetWindowAttributesReply) -
                                sizeof(xGenericReply));
    wa->sequenceNumber = client->sequence;
    wa->backingBitPlanes = wBackingBitPlanes(pWin);
    wa->backingPixel = wBackingPixel(pWin);
    wa->saveUnder = (BOOL) pWin->saveUnder;
    wa->override = pWin->overrideRedirect;
    if (!pWin->mapped)
        wa->mapState = IsUnmapped;
    else if (pWin->realized)
        wa->mapState = IsViewable;
    else
        wa->mapState = IsUnviewable;

    wa->colormap = wColormap(pWin);
    wa->mapInstalled = (wa->colormap == None) ? xFalse
        : IsMapInstalled(wa->colormap, pWin);

    wa->yourEventMask = EventMaskForClient(pWin, client);
    wa->allEventMasks = pWin->eventMask | wOtherEventMasks(pWin);
    wa->doNotPropagateMask = wDontPropagateMask(pWin);
    wa->class = pWin->drawable.class;
    wa->visualID = wVisual(pWin);
}

WindowPtr
MoveWindowInStack(WindowPtr pWin, WindowPtr pNextSib)
{
    WindowPtr pParent = pWin->parent;
    WindowPtr pFirstChange = pWin;      /* highest window where list changes */

    if (pWin->nextSib != pNextSib) {
        WindowPtr pOldNextSib = pWin->nextSib;

        if (!pNextSib) {        /* move to bottom */
            if (pParent->firstChild == pWin)
                pParent->firstChild = pWin->nextSib;
            /* if (pWin->nextSib) *//* is always True: pNextSib == NULL
             * and pWin->nextSib != pNextSib
             * therefore pWin->nextSib != NULL */
            pFirstChange = pWin->nextSib;
            pWin->nextSib->prevSib = pWin->prevSib;
            if (pWin->prevSib)
                pWin->prevSib->nextSib = pWin->nextSib;
            pParent->lastChild->nextSib = pWin;
            pWin->prevSib = pParent->lastChild;
            pWin->nextSib = NullWindow;
            pParent->lastChild = pWin;
        }
        else if (pParent->firstChild == pNextSib) {     /* move to top */
            pFirstChange = pWin;
            if (pParent->lastChild == pWin)
                pParent->lastChild = pWin->prevSib;
            if (pWin->nextSib)
                pWin->nextSib->prevSib = pWin->prevSib;
            if (pWin->prevSib)
                pWin->prevSib->nextSib = pWin->nextSib;
            pWin->nextSib = pParent->firstChild;
            pWin->prevSib = NULL;
            pNextSib->prevSib = pWin;
            pParent->firstChild = pWin;
        }
        else {                  /* move in middle of list */

            WindowPtr pOldNext = pWin->nextSib;

            pFirstChange = NullWindow;
            if (pParent->firstChild == pWin)
                pFirstChange = pParent->firstChild = pWin->nextSib;
            if (pParent->lastChild == pWin) {
                pFirstChange = pWin;
                pParent->lastChild = pWin->prevSib;
            }
            if (pWin->nextSib)
                pWin->nextSib->prevSib = pWin->prevSib;
            if (pWin->prevSib)
                pWin->prevSib->nextSib = pWin->nextSib;
            pWin->nextSib = pNextSib;
            pWin->prevSib = pNextSib->prevSib;
            if (pNextSib->prevSib)
                pNextSib->prevSib->nextSib = pWin;
            pNextSib->prevSib = pWin;
            if (!pFirstChange) {        /* do we know it yet? */
                pFirstChange = pParent->firstChild;     /* no, search from top */
                while ((pFirstChange != pWin) && (pFirstChange != pOldNext))
                    pFirstChange = pFirstChange->nextSib;
            }
        }
        if (pWin->drawable.pScreen->RestackWindow)
            (*pWin->drawable.pScreen->RestackWindow) (pWin, pOldNextSib);
    }

#ifdef ROOTLESS
    /*
     * In rootless mode we can't optimize away window restacks.
     * There may be non-X windows around, so even if the window
     * is in the correct position from X's point of view,
     * the underlying window system may want to reorder it.
     */
    else if (pWin->drawable.pScreen->RestackWindow)
        (*pWin->drawable.pScreen->RestackWindow) (pWin, pWin->nextSib);
#endif

    return pFirstChange;
}

void
SetWinSize(WindowPtr pWin)
{
#ifdef COMPOSITE
    if (pWin->redirectDraw != RedirectDrawNone) {
        BoxRec box;

        /*
         * Redirected clients get clip list equal to their
         * own geometry, not clipped to their parent
         */
        box.x1 = pWin->drawable.x;
        box.y1 = pWin->drawable.y;
        box.x2 = pWin->drawable.x + pWin->drawable.width;
        box.y2 = pWin->drawable.y + pWin->drawable.height;
        RegionReset(&pWin->winSize, &box);
    }
    else
#endif
        ClippedRegionFromBox(pWin->parent, &pWin->winSize,
                             pWin->drawable.x, pWin->drawable.y,
                             (int) pWin->drawable.width,
                             (int) pWin->drawable.height);
    if (wBoundingShape(pWin) || wClipShape(pWin)) {
        RegionTranslate(&pWin->winSize, -pWin->drawable.x, -pWin->drawable.y);
        if (wBoundingShape(pWin))
            RegionIntersect(&pWin->winSize, &pWin->winSize,
                            wBoundingShape(pWin));
        if (wClipShape(pWin))
            RegionIntersect(&pWin->winSize, &pWin->winSize, wClipShape(pWin));
        RegionTranslate(&pWin->winSize, pWin->drawable.x, pWin->drawable.y);
    }
}

void
SetBorderSize(WindowPtr pWin)
{
    int bw;

    if (HasBorder(pWin)) {
        bw = wBorderWidth(pWin);
#ifdef COMPOSITE
        if (pWin->redirectDraw != RedirectDrawNone) {
            BoxRec box;

            /*
             * Redirected clients get clip list equal to their
             * own geometry, not clipped to their parent
             */
            box.x1 = pWin->drawable.x - bw;
            box.y1 = pWin->drawable.y - bw;
            box.x2 = pWin->drawable.x + pWin->drawable.width + bw;
            box.y2 = pWin->drawable.y + pWin->drawable.height + bw;
            RegionReset(&pWin->borderSize, &box);
        }
        else
#endif
            ClippedRegionFromBox(pWin->parent, &pWin->borderSize,
                                 pWin->drawable.x - bw, pWin->drawable.y - bw,
                                 (int) (pWin->drawable.width + (bw << 1)),
                                 (int) (pWin->drawable.height + (bw << 1)));
        if (wBoundingShape(pWin)) {
            RegionTranslate(&pWin->borderSize, -pWin->drawable.x,
                            -pWin->drawable.y);
            RegionIntersect(&pWin->borderSize, &pWin->borderSize,
                            wBoundingShape(pWin));
            RegionTranslate(&pWin->borderSize, pWin->drawable.x,
                            pWin->drawable.y);
            RegionUnion(&pWin->borderSize, &pWin->borderSize, &pWin->winSize);
        }
    }
    else {
        RegionCopy(&pWin->borderSize, &pWin->winSize);
    }
}

/**
 *
 *  \param x,y          new window position
 *  \param oldx,oldy    old window position
 *  \param destx,desty  position relative to gravity
 */

void
GravityTranslate(int x, int y, int oldx, int oldy,
                 int dw, int dh, unsigned gravity, int *destx, int *desty)
{
    switch (gravity) {
    case NorthGravity:
        *destx = x + dw / 2;
        *desty = y;
        break;
    case NorthEastGravity:
        *destx = x + dw;
        *desty = y;
        break;
    case WestGravity:
        *destx = x;
        *desty = y + dh / 2;
        break;
    case CenterGravity:
        *destx = x + dw / 2;
        *desty = y + dh / 2;
        break;
    case EastGravity:
        *destx = x + dw;
        *desty = y + dh / 2;
        break;
    case SouthWestGravity:
        *destx = x;
        *desty = y + dh;
        break;
    case SouthGravity:
        *destx = x + dw / 2;
        *desty = y + dh;
        break;
    case SouthEastGravity:
        *destx = x + dw;
        *desty = y + dh;
        break;
    case StaticGravity:
        *destx = oldx;
        *desty = oldy;
        break;
    default:
        *destx = x;
        *desty = y;
        break;
    }
}

/* XXX need to retile border on each window with ParentRelative origin */
void
ResizeChildrenWinSize(WindowPtr pWin, int dx, int dy, int dw, int dh)
{
    ScreenPtr pScreen;
    WindowPtr pSib, pChild;
    Bool resized = (dw || dh);

    pScreen = pWin->drawable.pScreen;

    for (pSib = pWin->firstChild; pSib; pSib = pSib->nextSib) {
        if (resized && (pSib->winGravity > NorthWestGravity)) {
            int cwsx, cwsy;

            cwsx = pSib->origin.x;
            cwsy = pSib->origin.y;
            GravityTranslate(cwsx, cwsy, cwsx - dx, cwsy - dy, dw, dh,
                             pSib->winGravity, &cwsx, &cwsy);
            if (cwsx != pSib->origin.x || cwsy != pSib->origin.y) {
                xEvent event = {
                    .u.gravity.window = pSib->drawable.id,
                    .u.gravity.x = cwsx - wBorderWidth(pSib),
                    .u.gravity.y = cwsy - wBorderWidth(pSib)
                };
                event.u.u.type = GravityNotify;
                DeliverEvents(pSib, &event, 1, NullWindow);
                pSib->origin.x = cwsx;
                pSib->origin.y = cwsy;
            }
        }
        pSib->drawable.x = pWin->drawable.x + pSib->origin.x;
        pSib->drawable.y = pWin->drawable.y + pSib->origin.y;
        SetWinSize(pSib);
        SetBorderSize(pSib);
        (*pScreen->PositionWindow) (pSib, pSib->drawable.x, pSib->drawable.y);

        if ((pChild = pSib->firstChild)) {
            while (1) {
                pChild->drawable.x = pChild->parent->drawable.x +
                    pChild->origin.x;
                pChild->drawable.y = pChild->parent->drawable.y +
                    pChild->origin.y;
                SetWinSize(pChild);
                SetBorderSize(pChild);
                (*pScreen->PositionWindow) (pChild,
                                            pChild->drawable.x,
                                            pChild->drawable.y);
                if (pChild->firstChild) {
                    pChild = pChild->firstChild;
                    continue;
                }
                while (!pChild->nextSib && (pChild != pSib))
                    pChild = pChild->parent;
                if (pChild == pSib)
                    break;
                pChild = pChild->nextSib;
            }
        }
    }
}

#define GET_INT16(m, f) \
	if (m & mask) \
	  { \
	     f = (INT16) *pVlist;\
	    pVlist++; \
	 }
#define GET_CARD16(m, f) \
	if (m & mask) \
	 { \
	    f = (CARD16) *pVlist;\
	    pVlist++;\
	 }

#define GET_CARD8(m, f) \
	if (m & mask) \
	 { \
	    f = (CARD8) *pVlist;\
	    pVlist++;\
	 }

#define ChangeMask ((Mask)(CWX | CWY | CWWidth | CWHeight))

/*
 * IsSiblingAboveMe
 *     returns Above if pSib above pMe in stack or Below otherwise
 */

static int
IsSiblingAboveMe(WindowPtr pMe, WindowPtr pSib)
{
    WindowPtr pWin;

    pWin = pMe->parent->firstChild;
    while (pWin) {
        if (pWin == pSib)
            return Above;
        else if (pWin == pMe)
            return Below;
        pWin = pWin->nextSib;
    }
    return Below;
}

static BoxPtr
WindowExtents(WindowPtr pWin, BoxPtr pBox)
{
    pBox->x1 = pWin->drawable.x - wBorderWidth(pWin);
    pBox->y1 = pWin->drawable.y - wBorderWidth(pWin);
    pBox->x2 = pWin->drawable.x + (int) pWin->drawable.width
        + wBorderWidth(pWin);
    pBox->y2 = pWin->drawable.y + (int) pWin->drawable.height
        + wBorderWidth(pWin);
    return pBox;
}

#define IS_SHAPED(pWin)	(wBoundingShape (pWin) != NULL)

static RegionPtr
MakeBoundingRegion(WindowPtr pWin, BoxPtr pBox)
{
    RegionPtr pRgn = RegionCreate(pBox, 1);

    if (wBoundingShape(pWin)) {
        RegionTranslate(pRgn, -pWin->origin.x, -pWin->origin.y);
        RegionIntersect(pRgn, pRgn, wBoundingShape(pWin));
        RegionTranslate(pRgn, pWin->origin.x, pWin->origin.y);
    }
    return pRgn;
}

static Bool
ShapeOverlap(WindowPtr pWin, BoxPtr pWinBox, WindowPtr pSib, BoxPtr pSibBox)
{
    RegionPtr pWinRgn, pSibRgn;
    Bool ret;

    if (!IS_SHAPED(pWin) && !IS_SHAPED(pSib))
        return TRUE;
    pWinRgn = MakeBoundingRegion(pWin, pWinBox);
    pSibRgn = MakeBoundingRegion(pSib, pSibBox);
    RegionIntersect(pWinRgn, pWinRgn, pSibRgn);
    ret = RegionNotEmpty(pWinRgn);
    RegionDestroy(pWinRgn);
    RegionDestroy(pSibRgn);
    return ret;
}

static Bool
AnyWindowOverlapsMe(WindowPtr pWin, WindowPtr pHead, BoxPtr box)
{
    WindowPtr pSib;
    BoxRec sboxrec;
    BoxPtr sbox;

    for (pSib = pWin->prevSib; pSib != pHead; pSib = pSib->prevSib) {
        if (pSib->mapped) {
            sbox = WindowExtents(pSib, &sboxrec);
            if (BOXES_OVERLAP(sbox, box)
                && ShapeOverlap(pWin, box, pSib, sbox))
                return TRUE;
        }
    }
    return FALSE;
}

static Bool
IOverlapAnyWindow(WindowPtr pWin, BoxPtr box)
{
    WindowPtr pSib;
    BoxRec sboxrec;
    BoxPtr sbox;

    for (pSib = pWin->nextSib; pSib; pSib = pSib->nextSib) {
        if (pSib->mapped) {
            sbox = WindowExtents(pSib, &sboxrec);
            if (BOXES_OVERLAP(sbox, box)
                && ShapeOverlap(pWin, box, pSib, sbox))
                return TRUE;
        }
    }
    return FALSE;
}

/*
 *   WhereDoIGoInTheStack()
 *	  Given pWin and pSib and the relationshipe smode, return
 *	  the window that pWin should go ABOVE.
 *	  If a pSib is specified:
 *	      Above:  pWin is placed just above pSib
 *	      Below:  pWin is placed just below pSib
 *	      TopIf:  if pSib occludes pWin, then pWin is placed
 *		      at the top of the stack
 *	      BottomIf:	 if pWin occludes pSib, then pWin is
 *			 placed at the bottom of the stack
 *	      Opposite: if pSib occludes pWin, then pWin is placed at the
 *			top of the stack, else if pWin occludes pSib, then
 *			pWin is placed at the bottom of the stack
 *
 *	  If pSib is NULL:
 *	      Above:  pWin is placed at the top of the stack
 *	      Below:  pWin is placed at the bottom of the stack
 *	      TopIf:  if any sibling occludes pWin, then pWin is placed at
 *		      the top of the stack
 *	      BottomIf: if pWin occludes any sibline, then pWin is placed at
 *			the bottom of the stack
 *	      Opposite: if any sibling occludes pWin, then pWin is placed at
 *			the top of the stack, else if pWin occludes any
 *			sibling, then pWin is placed at the bottom of the stack
 *
 */

static WindowPtr
WhereDoIGoInTheStack(WindowPtr pWin,
                     WindowPtr pSib,
                     short x,
                     short y, unsigned short w, unsigned short h, int smode)
{
    BoxRec box;
    WindowPtr pHead, pFirst;

    if ((pWin == pWin->parent->firstChild) && (pWin == pWin->parent->lastChild))
        return NULL;
    pHead = RealChildHead(pWin->parent);
    pFirst = pHead ? pHead->nextSib : pWin->parent->firstChild;
    box.x1 = x;
    box.y1 = y;
    box.x2 = x + (int) w;
    box.y2 = y + (int) h;
    switch (smode) {
    case Above:
        if (pSib)
            return pSib;
        else if (pWin == pFirst)
            return pWin->nextSib;
        else
            return pFirst;
    case Below:
        if (pSib)
            if (pSib->nextSib != pWin)
                return pSib->nextSib;
            else
                return pWin->nextSib;
        else
            return NullWindow;
    case TopIf:
        if ((!pWin->mapped || (pSib && !pSib->mapped)))
            return pWin->nextSib;
        else if (pSib) {
            if ((IsSiblingAboveMe(pWin, pSib) == Above) &&
                (RegionContainsRect(&pSib->borderSize, &box) != rgnOUT))
                return pFirst;
            else
                return pWin->nextSib;
        }
        else if (AnyWindowOverlapsMe(pWin, pHead, &box))
            return pFirst;
        else
            return pWin->nextSib;
    case BottomIf:
        if ((!pWin->mapped || (pSib && !pSib->mapped)))
            return pWin->nextSib;
        else if (pSib) {
            if ((IsSiblingAboveMe(pWin, pSib) == Below) &&
                (RegionContainsRect(&pSib->borderSize, &box) != rgnOUT))
                return NullWindow;
            else
                return pWin->nextSib;
        }
        else if (IOverlapAnyWindow(pWin, &box))
            return NullWindow;
        else
            return pWin->nextSib;
    case Opposite:
        if ((!pWin->mapped || (pSib && !pSib->mapped)))
            return pWin->nextSib;
        else if (pSib) {
            if (RegionContainsRect(&pSib->borderSize, &box) != rgnOUT) {
                if (IsSiblingAboveMe(pWin, pSib) == Above)
                    return pFirst;
                else
                    return NullWindow;
            }
            else
                return pWin->nextSib;
        }
        else if (AnyWindowOverlapsMe(pWin, pHead, &box)) {
            /* If I'm occluded, I can't possibly be the first child
             * if (pWin == pWin->parent->firstChild)
             *    return pWin->nextSib;
             */
            return pFirst;
        }
        else if (IOverlapAnyWindow(pWin, &box))
            return NullWindow;
        else
            return pWin->nextSib;
    default:
    {
        /* should never happen; make something up. */
        return pWin->nextSib;
    }
    }
}

static void
ReflectStackChange(WindowPtr pWin, WindowPtr pSib, VTKind kind)
{
/* Note that pSib might be NULL */

    Bool WasViewable = (Bool) pWin->viewable;
    Bool anyMarked;
    WindowPtr pFirstChange;
    WindowPtr pLayerWin;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    /* if this is a root window, can't be restacked */
    if (!pWin->parent)
        return;

    pFirstChange = MoveWindowInStack(pWin, pSib);

    if (WasViewable) {
        anyMarked = (*pScreen->MarkOverlappedWindows) (pWin, pFirstChange,
                                                       &pLayerWin);
        if (pLayerWin != pWin)
            pFirstChange = pLayerWin;
        if (anyMarked) {
            (*pScreen->ValidateTree) (pLayerWin->parent, pFirstChange, kind);
            (*pScreen->HandleExposures) (pLayerWin->parent);
            if (pWin->drawable.pScreen->PostValidateTree)
                (*pScreen->PostValidateTree) (pLayerWin->parent, pFirstChange,
                                              kind);
        }
    }
    if (pWin->realized)
        WindowsRestructured();
}

/*****
 * ConfigureWindow
 *****/

int
ConfigureWindow(WindowPtr pWin, Mask mask, XID *vlist, ClientPtr client)
{
#define RESTACK_WIN    0
#define MOVE_WIN       1
#define RESIZE_WIN     2
#define REBORDER_WIN   3
    WindowPtr pSib = NullWindow;
    WindowPtr pParent = pWin->parent;
    Window sibwid = 0;
    Mask index2, tmask;
    XID *pVlist;
    short x, y, beforeX, beforeY;
    unsigned short w = pWin->drawable.width,
        h = pWin->drawable.height, bw = pWin->borderWidth;
    int rc, action, smode = Above;

    if ((pWin->drawable.class == InputOnly) && (mask & CWBorderWidth))
        return BadMatch;

    if ((mask & CWSibling) && !(mask & CWStackMode))
        return BadMatch;

    pVlist = vlist;

    if (pParent) {
        x = pWin->drawable.x - pParent->drawable.x - (int) bw;
        y = pWin->drawable.y - pParent->drawable.y - (int) bw;
    }
    else {
        x = pWin->drawable.x;
        y = pWin->drawable.y;
    }
    beforeX = x;
    beforeY = y;
    action = RESTACK_WIN;
    if ((mask & (CWX | CWY)) && (!(mask & (CWHeight | CWWidth)))) {
        GET_INT16(CWX, x);
        GET_INT16(CWY, y);
        action = MOVE_WIN;
    }
    /* or should be resized */
    else if (mask & (CWX | CWY | CWWidth | CWHeight)) {
        GET_INT16(CWX, x);
        GET_INT16(CWY, y);
        GET_CARD16(CWWidth, w);
        GET_CARD16(CWHeight, h);
        if (!w || !h) {
            client->errorValue = 0;
            return BadValue;
        }
        action = RESIZE_WIN;
    }
    tmask = mask & ~ChangeMask;
    while (tmask) {
        index2 = (Mask) lowbit(tmask);
        tmask &= ~index2;
        switch (index2) {
        case CWBorderWidth:
            GET_CARD16(CWBorderWidth, bw);
            break;
        case CWSibling:
            sibwid = (Window) *pVlist;
            pVlist++;
            rc = dixLookupWindow(&pSib, sibwid, client, DixGetAttrAccess);
            if (rc != Success) {
                client->errorValue = sibwid;
                return rc;
            }
            if (pSib->parent != pParent)
                return BadMatch;
            if (pSib == pWin)
                return BadMatch;
            break;
        case CWStackMode:
            GET_CARD8(CWStackMode, smode);
            if ((smode != TopIf) && (smode != BottomIf) &&
                (smode != Opposite) && (smode != Above) && (smode != Below)) {
                client->errorValue = smode;
                return BadValue;
            }
            break;
        default:
            client->errorValue = mask;
            return BadValue;
        }
    }
    /* root really can't be reconfigured, so just return */
    if (!pParent)
        return Success;

    /* Figure out if the window should be moved.  Doesn't
       make the changes to the window if event sent. */

    if (mask & CWStackMode)
        pSib = WhereDoIGoInTheStack(pWin, pSib, pParent->drawable.x + x,
                                    pParent->drawable.y + y,
                                    w + (bw << 1), h + (bw << 1), smode);
    else
        pSib = pWin->nextSib;

    if ((!pWin->overrideRedirect) && (RedirectSend(pParent))) {
        xEvent event = {
            .u.configureRequest.window = pWin->drawable.id,
            .u.configureRequest.sibling = (mask & CWSibling) ? sibwid : None,
            .u.configureRequest.x = x,
            .u.configureRequest.y = y,
            .u.configureRequest.width = w,
            .u.configureRequest.height = h,
            .u.configureRequest.borderWidth = bw,
            .u.configureRequest.valueMask = mask,
            .u.configureRequest.parent = pParent->drawable.id
        };
        event.u.u.type = ConfigureRequest;
        event.u.u.detail = (mask & CWStackMode) ? smode : Above;
#ifdef PANORAMIX
        if (!noPanoramiXExtension && (!pParent || !pParent->parent)) {
            event.u.configureRequest.x += screenInfo.screens[0]->x;
            event.u.configureRequest.y += screenInfo.screens[0]->y;
        }
#endif
        if (MaybeDeliverEventsToClient(pParent, &event, 1,
                                       SubstructureRedirectMask, client) == 1)
            return Success;
    }
    if (action == RESIZE_WIN) {
        Bool size_change = (w != pWin->drawable.width)
            || (h != pWin->drawable.height);

        if (size_change &&
            ((pWin->eventMask | wOtherEventMasks(pWin)) & ResizeRedirectMask)) {
            xEvent eventT = {
                .u.resizeRequest.window = pWin->drawable.id,
                .u.resizeRequest.width = w,
                .u.resizeRequest.height = h
            };
            eventT.u.u.type = ResizeRequest;
            if (MaybeDeliverEventsToClient(pWin, &eventT, 1,
                                           ResizeRedirectMask, client) == 1) {
                /* if event is delivered, leave the actual size alone. */
                w = pWin->drawable.width;
                h = pWin->drawable.height;
                size_change = FALSE;
            }
        }
        if (!size_change) {
            if (mask & (CWX | CWY))
                action = MOVE_WIN;
            else if (mask & (CWStackMode | CWBorderWidth))
                action = RESTACK_WIN;
            else                /* really nothing to do */
                return (Success);
        }
    }

    if (action == RESIZE_WIN)
        /* we've already checked whether there's really a size change */
        goto ActuallyDoSomething;
    if ((mask & CWX) && (x != beforeX))
        goto ActuallyDoSomething;
    if ((mask & CWY) && (y != beforeY))
        goto ActuallyDoSomething;
    if ((mask & CWBorderWidth) && (bw != wBorderWidth(pWin)))
        goto ActuallyDoSomething;
    if (mask & CWStackMode) {
#ifndef ROOTLESS
        /* See above for why we always reorder in rootless mode. */
        if (pWin->nextSib != pSib)
#endif
            goto ActuallyDoSomething;
    }
    return Success;

 ActuallyDoSomething:
    if (pWin->drawable.pScreen->ConfigNotify) {
        int ret;

        ret =
            (*pWin->drawable.pScreen->ConfigNotify) (pWin, x, y, w, h, bw,
                                                     pSib);
        if (ret) {
            client->errorValue = 0;
            return ret;
        }
    }

    if (SubStrSend(pWin, pParent)) {
        xEvent event = {
            .u.configureNotify.window = pWin->drawable.id,
            .u.configureNotify.aboveSibling = pSib ? pSib->drawable.id : None,
            .u.configureNotify.x = x,
            .u.configureNotify.y = y,
            .u.configureNotify.width = w,
            .u.configureNotify.height = h,
            .u.configureNotify.borderWidth = bw,
            .u.configureNotify.override = pWin->overrideRedirect
        };
        event.u.u.type = ConfigureNotify;
#ifdef PANORAMIX
        if (!noPanoramiXExtension && (!pParent || !pParent->parent)) {
            event.u.configureNotify.x += screenInfo.screens[0]->x;
            event.u.configureNotify.y += screenInfo.screens[0]->y;
        }
#endif
        DeliverEvents(pWin, &event, 1, NullWindow);
    }
    if (mask & CWBorderWidth) {
        if (action == RESTACK_WIN) {
            action = MOVE_WIN;
            pWin->borderWidth = bw;
        }
        else if ((action == MOVE_WIN) &&
                 (beforeX + wBorderWidth(pWin) == x + (int) bw) &&
                 (beforeY + wBorderWidth(pWin) == y + (int) bw)) {
            action = REBORDER_WIN;
            (*pWin->drawable.pScreen->ChangeBorderWidth) (pWin, bw);
        }
        else
            pWin->borderWidth = bw;
    }
    if (action == MOVE_WIN)
        (*pWin->drawable.pScreen->MoveWindow) (pWin, x, y, pSib,
                                               (mask & CWBorderWidth) ? VTOther
                                               : VTMove);
    else if (action == RESIZE_WIN)
        (*pWin->drawable.pScreen->ResizeWindow) (pWin, x, y, w, h, pSib);
    else if (mask & CWStackMode)
        ReflectStackChange(pWin, pSib, VTOther);

    if (action != RESTACK_WIN)
        CheckCursorConfinement(pWin);
    return Success;
#undef RESTACK_WIN
#undef MOVE_WIN
#undef RESIZE_WIN
#undef REBORDER_WIN
}

/******
 *
 * CirculateWindow
 *    For RaiseLowest, raises the lowest mapped child (if any) that is
 *    obscured by another child to the top of the stack.  For LowerHighest,
 *    lowers the highest mapped child (if any) that is obscuring another
 *    child to the bottom of the stack.	 Exposure processing is performed
 *
 ******/

int
CirculateWindow(WindowPtr pParent, int direction, ClientPtr client)
{
    WindowPtr pWin, pHead, pFirst;
    xEvent event;
    BoxRec box;

    pHead = RealChildHead(pParent);
    pFirst = pHead ? pHead->nextSib : pParent->firstChild;
    if (direction == RaiseLowest) {
        for (pWin = pParent->lastChild;
             (pWin != pHead) &&
             !(pWin->mapped &&
               AnyWindowOverlapsMe(pWin, pHead, WindowExtents(pWin, &box)));
             pWin = pWin->prevSib);
        if (pWin == pHead)
            return Success;
    }
    else {
        for (pWin = pFirst;
             pWin &&
             !(pWin->mapped &&
               IOverlapAnyWindow(pWin, WindowExtents(pWin, &box)));
             pWin = pWin->nextSib);
        if (!pWin)
            return Success;
    }

    event = (xEvent) {
        .u.circulate.window = pWin->drawable.id,
        .u.circulate.parent = pParent->drawable.id,
        .u.circulate.event = pParent->drawable.id,
        .u.circulate.place = (direction == RaiseLowest) ?
                              PlaceOnTop : PlaceOnBottom,
    };

    if (RedirectSend(pParent)) {
        event.u.u.type = CirculateRequest;
        if (MaybeDeliverEventsToClient(pParent, &event, 1,
                                       SubstructureRedirectMask, client) == 1)
            return Success;
    }

    event.u.u.type = CirculateNotify;
    DeliverEvents(pWin, &event, 1, NullWindow);
    ReflectStackChange(pWin,
                       (direction == RaiseLowest) ? pFirst : NullWindow,
                       VTStack);

    return Success;
}

static int
CompareWIDs(WindowPtr pWin, void *value)
{                               /* must conform to VisitWindowProcPtr */
    Window *wid = (Window *) value;

    if (pWin->drawable.id == *wid)
        return WT_STOPWALKING;
    else
        return WT_WALKCHILDREN;
}

/*****
 *  ReparentWindow
 *****/

int
ReparentWindow(WindowPtr pWin, WindowPtr pParent,
               int x, int y, ClientPtr client)
{
    WindowPtr pPrev, pPriorParent;
    Bool WasMapped = (Bool) (pWin->mapped);
    xEvent event;
    int bw = wBorderWidth(pWin);
    ScreenPtr pScreen;

    pScreen = pWin->drawable.pScreen;
    if (TraverseTree(pWin, CompareWIDs, (void *) &pParent->drawable.id) ==
        WT_STOPWALKING)
        return BadMatch;
    if (!MakeWindowOptional(pWin))
        return BadAlloc;

    if (WasMapped)
        UnmapWindow(pWin, FALSE);

    event = (xEvent) {
        .u.reparent.window = pWin->drawable.id,
        .u.reparent.parent = pParent->drawable.id,
        .u.reparent.x = x,
        .u.reparent.y = y,
        .u.reparent.override = pWin->overrideRedirect
    };
    event.u.u.type = ReparentNotify;
#ifdef PANORAMIX
    if (!noPanoramiXExtension && !pParent->parent) {
        event.u.reparent.x += screenInfo.screens[0]->x;
        event.u.reparent.y += screenInfo.screens[0]->y;
    }
#endif
    DeliverEvents(pWin, &event, 1, pParent);

    /* take out of sibling chain */

    pPriorParent = pPrev = pWin->parent;
    if (pPrev->firstChild == pWin)
        pPrev->firstChild = pWin->nextSib;
    if (pPrev->lastChild == pWin)
        pPrev->lastChild = pWin->prevSib;

    if (pWin->nextSib)
        pWin->nextSib->prevSib = pWin->prevSib;
    if (pWin->prevSib)
        pWin->prevSib->nextSib = pWin->nextSib;

    /* insert at beginning of pParent */
    pWin->parent = pParent;
    pPrev = RealChildHead(pParent);
    if (pPrev) {
        pWin->nextSib = pPrev->nextSib;
        if (pPrev->nextSib)
            pPrev->nextSib->prevSib = pWin;
        else
            pParent->lastChild = pWin;
        pPrev->nextSib = pWin;
        pWin->prevSib = pPrev;
    }
    else {
        pWin->nextSib = pParent->firstChild;
        pWin->prevSib = NullWindow;
        if (pParent->firstChild)
            pParent->firstChild->prevSib = pWin;
        else
            pParent->lastChild = pWin;
        pParent->firstChild = pWin;
    }

    pWin->origin.x = x + bw;
    pWin->origin.y = y + bw;
    pWin->drawable.x = x + bw + pParent->drawable.x;
    pWin->drawable.y = y + bw + pParent->drawable.y;

    /* clip to parent */
    SetWinSize(pWin);
    SetBorderSize(pWin);

    if (pScreen->ReparentWindow)
        (*pScreen->ReparentWindow) (pWin, pPriorParent);
    (*pScreen->PositionWindow) (pWin, pWin->drawable.x, pWin->drawable.y);
    ResizeChildrenWinSize(pWin, 0, 0, 0, 0);

    CheckWindowOptionalNeed(pWin);

    if (WasMapped)
        MapWindow(pWin, client);
    RecalculateDeliverableEvents(pWin);
    return Success;
}

static void
RealizeTree(WindowPtr pWin)
{
    WindowPtr pChild;
    RealizeWindowProcPtr Realize;

    Realize = pWin->drawable.pScreen->RealizeWindow;
    pChild = pWin;
    while (1) {
        if (pChild->mapped) {
            pChild->realized = TRUE;
            pChild->viewable = (pChild->drawable.class == InputOutput);
            (*Realize) (pChild);
            if (pChild->firstChild) {
                pChild = pChild->firstChild;
                continue;
            }
        }
        while (!pChild->nextSib && (pChild != pWin))
            pChild = pChild->parent;
        if (pChild == pWin)
            return;
        pChild = pChild->nextSib;
    }
}

static Bool
MaybeDeliverMapRequest(WindowPtr pWin, WindowPtr pParent, ClientPtr client)
{
    xEvent event = {
        .u.mapRequest.window = pWin->drawable.id,
        .u.mapRequest.parent = pParent->drawable.id
    };
    event.u.u.type = MapRequest;

    return MaybeDeliverEventsToClient(pParent, &event, 1,
                                      SubstructureRedirectMask,
                                      client) == 1;
}

static void
DeliverMapNotify(WindowPtr pWin)
{
    xEvent event = {
        .u.mapNotify.window = pWin->drawable.id,
        .u.mapNotify.override = pWin->overrideRedirect,
    };
    event.u.u.type = MapNotify;
    DeliverEvents(pWin, &event, 1, NullWindow);
}

/*****
 * MapWindow
 *    If some other client has selected SubStructureReDirect on the parent
 *    and override-redirect is xFalse, then a MapRequest event is generated,
 *    but the window remains unmapped.	Otherwise, the window is mapped and a
 *    MapNotify event is generated.
 *****/

int
MapWindow(WindowPtr pWin, ClientPtr client)
{
    ScreenPtr pScreen;

    WindowPtr pParent;
    WindowPtr pLayerWin;

    if (pWin->mapped)
        return Success;

    /* general check for permission to map window */
    if (XaceHook(XACE_RESOURCE_ACCESS, client, pWin->drawable.id, RT_WINDOW,
                 pWin, RT_NONE, NULL, DixShowAccess) != Success)
        return Success;

    pScreen = pWin->drawable.pScreen;
    if ((pParent = pWin->parent)) {
        Bool anyMarked;

        if ((!pWin->overrideRedirect) && (RedirectSend(pParent)))
            if (MaybeDeliverMapRequest(pWin, pParent, client))
                return Success;

        pWin->mapped = TRUE;
        if (SubStrSend(pWin, pParent))
            DeliverMapNotify(pWin);

        if (!pParent->realized)
            return Success;
        RealizeTree(pWin);
        if (pWin->viewable) {
            anyMarked = (*pScreen->MarkOverlappedWindows) (pWin, pWin,
                                                           &pLayerWin);
            if (anyMarked) {
                (*pScreen->ValidateTree) (pLayerWin->parent, pLayerWin, VTMap);
                (*pScreen->HandleExposures) (pLayerWin->parent);
                if (pScreen->PostValidateTree)
                    (*pScreen->PostValidateTree) (pLayerWin->parent, pLayerWin,
                                                  VTMap);
            }
        }
        WindowsRestructured();
    }
    else {
        RegionRec temp;

        pWin->mapped = TRUE;
        pWin->realized = TRUE;  /* for roots */
        pWin->viewable = pWin->drawable.class == InputOutput;
        /* We SHOULD check for an error value here XXX */
        (*pScreen->RealizeWindow) (pWin);
        if (pScreen->ClipNotify)
            (*pScreen->ClipNotify) (pWin, 0, 0);
        if (pScreen->PostValidateTree)
            (*pScreen->PostValidateTree) (NullWindow, pWin, VTMap);
        RegionNull(&temp);
        RegionCopy(&temp, &pWin->clipList);
        (*pScreen->WindowExposures) (pWin, &temp);
        RegionUninit(&temp);
    }

    return Success;
}

/*****
 * MapSubwindows
 *    Performs a MapWindow all unmapped children of the window, in top
 *    to bottom stacking order.
 *****/

void
MapSubwindows(WindowPtr pParent, ClientPtr client)
{
    WindowPtr pWin;
    WindowPtr pFirstMapped = NullWindow;
    ScreenPtr pScreen;
    Mask parentRedirect;
    Mask parentNotify;
    Bool anyMarked;
    WindowPtr pLayerWin;

    pScreen = pParent->drawable.pScreen;
    parentRedirect = RedirectSend(pParent);
    parentNotify = SubSend(pParent);
    anyMarked = FALSE;
    for (pWin = pParent->firstChild; pWin; pWin = pWin->nextSib) {
        if (!pWin->mapped) {
            if (parentRedirect && !pWin->overrideRedirect)
                if (MaybeDeliverMapRequest(pWin, pParent, client))
                    continue;

            pWin->mapped = TRUE;
            if (parentNotify || StrSend(pWin))
                DeliverMapNotify(pWin);

            if (!pFirstMapped)
                pFirstMapped = pWin;
            if (pParent->realized) {
                RealizeTree(pWin);
                if (pWin->viewable) {
                    anyMarked |= (*pScreen->MarkOverlappedWindows) (pWin, pWin,
                                                                    NULL);
                }
            }
        }
    }

    if (pFirstMapped) {
        pLayerWin = (*pScreen->GetLayerWindow) (pParent);
        if (pLayerWin->parent != pParent) {
            anyMarked |= (*pScreen->MarkOverlappedWindows) (pLayerWin,
                                                            pLayerWin, NULL);
            pFirstMapped = pLayerWin;
        }
        if (anyMarked) {
            (*pScreen->ValidateTree) (pLayerWin->parent, pFirstMapped, VTMap);
            (*pScreen->HandleExposures) (pLayerWin->parent);
            if (pScreen->PostValidateTree)
                (*pScreen->PostValidateTree) (pLayerWin->parent, pFirstMapped,
                                              VTMap);
        }
        WindowsRestructured();
    }
}

static void
UnrealizeTree(WindowPtr pWin, Bool fromConfigure)
{
    WindowPtr pChild;
    UnrealizeWindowProcPtr Unrealize;
    MarkUnrealizedWindowProcPtr MarkUnrealizedWindow;

    Unrealize = pWin->drawable.pScreen->UnrealizeWindow;
    MarkUnrealizedWindow = pWin->drawable.pScreen->MarkUnrealizedWindow;
    pChild = pWin;
    while (1) {
        if (pChild->realized) {
            pChild->realized = FALSE;
            pChild->visibility = VisibilityNotViewable;
#ifdef PANORAMIX
            if (!noPanoramiXExtension && !pChild->drawable.pScreen->myNum) {
                PanoramiXRes *win;
                int rc = dixLookupResourceByType((void **) &win,
                                                 pChild->drawable.id,
                                                 XRT_WINDOW,
                                                 serverClient, DixWriteAccess);

                if (rc == Success)
                    win->u.win.visibility = VisibilityNotViewable;
            }
#endif
            (*Unrealize) (pChild);
            DeleteWindowFromAnyEvents(pChild, FALSE);
            if (pChild->viewable) {
                pChild->viewable = FALSE;
                (*MarkUnrealizedWindow) (pChild, pWin, fromConfigure);
                pChild->drawable.serialNumber = NEXT_SERIAL_NUMBER;
            }
            if (pChild->firstChild) {
                pChild = pChild->firstChild;
                continue;
            }
        }
        while (!pChild->nextSib && (pChild != pWin))
            pChild = pChild->parent;
        if (pChild == pWin)
            return;
        pChild = pChild->nextSib;
    }
}

static void
DeliverUnmapNotify(WindowPtr pWin, Bool fromConfigure)
{
    xEvent event = {
        .u.unmapNotify.window = pWin->drawable.id,
        .u.unmapNotify.fromConfigure = fromConfigure
    };
    event.u.u.type = UnmapNotify;
    DeliverEvents(pWin, &event, 1, NullWindow);
}

/*****
 * UnmapWindow
 *    If the window is already unmapped, this request has no effect.
 *    Otherwise, the window is unmapped and an UnMapNotify event is
 *    generated.  Cannot unmap a root window.
 *****/

int
UnmapWindow(WindowPtr pWin, Bool fromConfigure)
{
    WindowPtr pParent;
    Bool wasRealized = (Bool) pWin->realized;
    Bool wasViewable = (Bool) pWin->viewable;
    ScreenPtr pScreen = pWin->drawable.pScreen;
    WindowPtr pLayerWin = pWin;

    if ((!pWin->mapped) || (!(pParent = pWin->parent)))
        return Success;
    if (SubStrSend(pWin, pParent))
        DeliverUnmapNotify(pWin, fromConfigure);
    if (wasViewable && !fromConfigure) {
        pWin->valdata = UnmapValData;
        (*pScreen->MarkOverlappedWindows) (pWin, pWin->nextSib, &pLayerWin);
        (*pScreen->MarkWindow) (pLayerWin->parent);
    }
    pWin->mapped = FALSE;
    if (wasRealized)
        UnrealizeTree(pWin, fromConfigure);
    if (wasViewable && !fromConfigure) {
        (*pScreen->ValidateTree) (pLayerWin->parent, pWin, VTUnmap);
        (*pScreen->HandleExposures) (pLayerWin->parent);
        if (pScreen->PostValidateTree)
            (*pScreen->PostValidateTree) (pLayerWin->parent, pWin, VTUnmap);
    }
    if (wasRealized && !fromConfigure) {
        WindowsRestructured();
        WindowGone(pWin);
    }
    return Success;
}

/*****
 * UnmapSubwindows
 *    Performs an UnmapWindow request with the specified mode on all mapped
 *    children of the window, in bottom to top stacking order.
 *****/

void
UnmapSubwindows(WindowPtr pWin)
{
    WindowPtr pChild, pHead;
    Bool wasRealized = (Bool) pWin->realized;
    Bool wasViewable = (Bool) pWin->viewable;
    Bool anyMarked = FALSE;
    Mask parentNotify;
    WindowPtr pLayerWin = NULL;
    ScreenPtr pScreen = pWin->drawable.pScreen;

    if (!pWin->firstChild)
        return;
    parentNotify = SubSend(pWin);
    pHead = RealChildHead(pWin);

    if (wasViewable)
        pLayerWin = (*pScreen->GetLayerWindow) (pWin);

    for (pChild = pWin->lastChild; pChild != pHead; pChild = pChild->prevSib) {
        if (pChild->mapped) {
            if (parentNotify || StrSend(pChild))
                DeliverUnmapNotify(pChild, xFalse);
            if (pChild->viewable) {
                pChild->valdata = UnmapValData;
                anyMarked = TRUE;
            }
            pChild->mapped = FALSE;
            if (pChild->realized)
                UnrealizeTree(pChild, FALSE);
        }
    }
    if (wasViewable && anyMarked) {
        if (pLayerWin->parent == pWin)
            (*pScreen->MarkWindow) (pWin);
        else {
            WindowPtr ptmp;

            (*pScreen->MarkOverlappedWindows) (pWin, pLayerWin, NULL);
            (*pScreen->MarkWindow) (pLayerWin->parent);

            /* Windows between pWin and pLayerWin may not have been marked */
            ptmp = pWin;

            while (ptmp != pLayerWin->parent) {
                (*pScreen->MarkWindow) (ptmp);
                ptmp = ptmp->parent;
            }
            pHead = pWin->firstChild;
        }
        (*pScreen->ValidateTree) (pLayerWin->parent, pHead, VTUnmap);
        (*pScreen->HandleExposures) (pLayerWin->parent);
        if (pScreen->PostValidateTree)
            (*pScreen->PostValidateTree) (pLayerWin->parent, pHead, VTUnmap);
    }
    if (wasRealized) {
        WindowsRestructured();
        WindowGone(pWin);
    }
}

void
HandleSaveSet(ClientPtr client)
{
    WindowPtr pParent, pWin;
    int j;

    for (j = 0; j < client->numSaved; j++) {
        pWin = SaveSetWindow(client->saveSet[j]);
        if (SaveSetToRoot(client->saveSet[j]))
            pParent = pWin->drawable.pScreen->root;
        else
        {
            pParent = pWin->parent;
            while (pParent && (wClient(pParent) == client))
                pParent = pParent->parent;
        }
        if (pParent) {
            if (pParent != pWin->parent) {
                /* unmap first so that ReparentWindow doesn't remap */
                if (!SaveSetShouldMap(client->saveSet[j]))
                    UnmapWindow(pWin, FALSE);
                ReparentWindow(pWin, pParent,
                               pWin->drawable.x - wBorderWidth(pWin) -
                               pParent->drawable.x,
                               pWin->drawable.y - wBorderWidth(pWin) -
                               pParent->drawable.y, client);
                if (!pWin->realized && pWin->mapped)
                    pWin->mapped = FALSE;
            }
            if (SaveSetShouldMap(client->saveSet[j]))
                MapWindow(pWin, client);
        }
    }
    free(client->saveSet);
    client->numSaved = 0;
    client->saveSet = NULL;
}

/**
 *
 * \param x,y  in root
 */
Bool
PointInWindowIsVisible(WindowPtr pWin, int x, int y)
{
    BoxRec box;

    if (!pWin->realized)
        return FALSE;
    if (RegionContainsPoint(&pWin->borderClip, x, y, &box)
        && (!wInputShape(pWin) ||
            RegionContainsPoint(wInputShape(pWin),
                                x - pWin->drawable.x,
                                y - pWin->drawable.y, &box)))
        return TRUE;
    return FALSE;
}

RegionPtr
NotClippedByChildren(WindowPtr pWin)
{
    RegionPtr pReg = RegionCreate(NullBox, 1);

    if (pWin->parent ||
        screenIsSaved != SCREEN_SAVER_ON ||
        !HasSaverWindow(pWin->drawable.pScreen)) {
        RegionIntersect(pReg, &pWin->borderClip, &pWin->winSize);
    }
    return pReg;
}

void
SendVisibilityNotify(WindowPtr pWin)
{
    xEvent event;
    unsigned int visibility = pWin->visibility;

#ifdef PANORAMIX
    /* This is not quite correct yet, but it's close */
    if (!noPanoramiXExtension) {
        PanoramiXRes *win;
        WindowPtr pWin2;
        int rc, i, Scrnum;

        Scrnum = pWin->drawable.pScreen->myNum;

        win = PanoramiXFindIDByScrnum(XRT_WINDOW, pWin->drawable.id, Scrnum);

        if (!win || (win->u.win.visibility == visibility))
            return;

        switch (visibility) {
        case VisibilityUnobscured:
        FOR_NSCREENS(i) {
            if (i == Scrnum)
                continue;

            rc = dixLookupWindow(&pWin2, win->info[i].id, serverClient,
                                 DixWriteAccess);

            if (rc == Success) {
                if (pWin2->visibility == VisibilityPartiallyObscured)
                    return;

                if (!i)
                    pWin = pWin2;
            }
        }
            break;
        case VisibilityPartiallyObscured:
            if (Scrnum) {
                rc = dixLookupWindow(&pWin2, win->info[0].id, serverClient,
                                     DixWriteAccess);
                if (rc == Success)
                    pWin = pWin2;
            }
            break;
        case VisibilityFullyObscured:
        FOR_NSCREENS(i) {
            if (i == Scrnum)
                continue;

            rc = dixLookupWindow(&pWin2, win->info[i].id, serverClient,
                                 DixWriteAccess);

            if (rc == Success) {
                if (pWin2->visibility != VisibilityFullyObscured)
                    return;

                if (!i)
                    pWin = pWin2;
            }
        }
            break;
        }

        win->u.win.visibility = visibility;
    }
#endif

    event = (xEvent) {
        .u.visibility.window = pWin->drawable.id,
        .u.visibility.state = visibility
    };
    event.u.u.type = VisibilityNotify;
    DeliverEvents(pWin, &event, 1, NullWindow);
}

#define RANDOM_WIDTH 32
int
dixSaveScreens(ClientPtr client, int on, int mode)
{
    int rc, i, what, type;
    XID vlist[2];

    if (on == SCREEN_SAVER_FORCER) {
        if (mode == ScreenSaverReset)
            what = SCREEN_SAVER_OFF;
        else
            what = SCREEN_SAVER_ON;
        type = what;
    }
    else {
        what = on;
        type = what;
        if (what == screenIsSaved)
            type = SCREEN_SAVER_CYCLE;
    }

    for (i = 0; i < screenInfo.numScreens; i++) {
        rc = XaceHook(XACE_SCREENSAVER_ACCESS, client, screenInfo.screens[i],
                      DixShowAccess | DixHideAccess);
        if (rc != Success)
            return rc;
    }
    for (i = 0; i < screenInfo.numScreens; i++) {
        ScreenPtr pScreen = screenInfo.screens[i];

        if (on == SCREEN_SAVER_FORCER)
            (*pScreen->SaveScreen) (pScreen, on);
        if (pScreen->screensaver.ExternalScreenSaver) {
            if ((*pScreen->screensaver.ExternalScreenSaver)
                (pScreen, type, on == SCREEN_SAVER_FORCER))
                continue;
        }
        if (type == screenIsSaved)
            continue;
        switch (type) {
        case SCREEN_SAVER_OFF:
            if (pScreen->screensaver.blanked == SCREEN_IS_BLANKED) {
                (*pScreen->SaveScreen) (pScreen, what);
            }
            else if (HasSaverWindow(pScreen)) {
                pScreen->screensaver.pWindow = NullWindow;
                FreeResource(pScreen->screensaver.wid, RT_NONE);
            }
            break;
        case SCREEN_SAVER_CYCLE:
            if (pScreen->screensaver.blanked == SCREEN_IS_TILED) {
                WindowPtr pWin = pScreen->screensaver.pWindow;

                /* make it look like screen saver is off, so that
                 * NotClippedByChildren will compute a clip list
                 * for the root window, so PaintWindow works
                 */
                screenIsSaved = SCREEN_SAVER_OFF;

                vlist[0] = -(rand() % RANDOM_WIDTH);
                vlist[1] = -(rand() % RANDOM_WIDTH);
                ConfigureWindow(pWin, CWX | CWY, vlist, client);

                screenIsSaved = SCREEN_SAVER_ON;
            }
            /*
             * Call the DDX saver in case it wants to do something
             * at cycle time
             */
            else if (pScreen->screensaver.blanked == SCREEN_IS_BLANKED) {
                (*pScreen->SaveScreen) (pScreen, type);
            }
            break;
        case SCREEN_SAVER_ON:
            if (ScreenSaverBlanking != DontPreferBlanking) {
                if ((*pScreen->SaveScreen) (pScreen, what)) {
                    pScreen->screensaver.blanked = SCREEN_IS_BLANKED;
                    continue;
                }
                if ((ScreenSaverAllowExposures != DontAllowExposures) &&
                    TileScreenSaver(pScreen, SCREEN_IS_BLACK)) {
                    pScreen->screensaver.blanked = SCREEN_IS_BLACK;
                    continue;
                }
            }
            if ((ScreenSaverAllowExposures != DontAllowExposures) &&
                TileScreenSaver(pScreen, SCREEN_IS_TILED)) {
                pScreen->screensaver.blanked = SCREEN_IS_TILED;
            }
            else
                pScreen->screensaver.blanked = SCREEN_ISNT_SAVED;
            break;
        }
    }
    screenIsSaved = what;
    if (mode == ScreenSaverReset) {
        if (on == SCREEN_SAVER_FORCER) {
            DeviceIntPtr dev;
            UpdateCurrentTimeIf();
            nt_list_for_each_entry(dev, inputInfo.devices, next)
                NoticeTime(dev, currentTime);
        }
        SetScreenSaverTimer();
    }
    return Success;
}

int
SaveScreens(int on, int mode)
{
    return dixSaveScreens(serverClient, on, mode);
}

static Bool
TileScreenSaver(ScreenPtr pScreen, int kind)
{
    int j;
    int result;
    XID attributes[3];
    Mask mask;
    WindowPtr pWin;
    CursorMetricRec cm;
    unsigned char *srcbits, *mskbits;
    CursorPtr cursor;
    XID cursorID = 0;
    int attri;

    mask = 0;
    attri = 0;
    switch (kind) {
    case SCREEN_IS_TILED:
        switch (pScreen->root->backgroundState) {
        case BackgroundPixel:
            attributes[attri++] = pScreen->root->background.pixel;
            mask |= CWBackPixel;
            break;
        case BackgroundPixmap:
            attributes[attri++] = None;
            mask |= CWBackPixmap;
            break;
        default:
            break;
        }
        break;
    case SCREEN_IS_BLACK:
        attributes[attri++] = pScreen->root->drawable.pScreen->blackPixel;
        mask |= CWBackPixel;
        break;
    }
    mask |= CWOverrideRedirect;
    attributes[attri++] = xTrue;

    /*
     * create a blank cursor
     */

    cm.width = 16;
    cm.height = 16;
    cm.xhot = 8;
    cm.yhot = 8;
    srcbits = malloc(BitmapBytePad(32) * 16);
    mskbits = malloc(BitmapBytePad(32) * 16);
    if (!srcbits || !mskbits) {
        free(srcbits);
        free(mskbits);
        cursor = 0;
    }
    else {
        for (j = 0; j < BitmapBytePad(32) * 16; j++)
            srcbits[j] = mskbits[j] = 0x0;
        result = AllocARGBCursor(srcbits, mskbits, NULL, &cm, 0, 0, 0, 0, 0, 0,
                                 &cursor, serverClient, (XID) 0);
        if (cursor) {
            cursorID = FakeClientID(0);
            if (AddResource(cursorID, RT_CURSOR, (void *) cursor)) {
                attributes[attri] = cursorID;
                mask |= CWCursor;
            }
            else
                cursor = 0;
        }
        else {
            free(srcbits);
            free(mskbits);
        }
    }

    pWin = pScreen->screensaver.pWindow =
        CreateWindow(pScreen->screensaver.wid,
                     pScreen->root,
                     -RANDOM_WIDTH, -RANDOM_WIDTH,
                     (unsigned short) pScreen->width + RANDOM_WIDTH,
                     (unsigned short) pScreen->height + RANDOM_WIDTH,
                     0, InputOutput, mask, attributes, 0, serverClient,
                     wVisual(pScreen->root), &result);

    if (cursor)
        FreeResource(cursorID, RT_NONE);

    if (!pWin)
        return FALSE;

    if (!AddResource(pWin->drawable.id, RT_WINDOW,
                     (void *) pScreen->screensaver.pWindow))
        return FALSE;

    if (mask & CWBackPixmap) {
        MakeRootTile(pWin);
        (*pWin->drawable.pScreen->ChangeWindowAttributes) (pWin, CWBackPixmap);
    }
    MapWindow(pWin, serverClient);
    return TRUE;
}

/*
 * FindWindowWithOptional
 *
 * search ancestors of the given window for an entry containing
 * a WindowOpt structure.  Assumptions:	 some parent will
 * contain the structure.
 */

WindowPtr
FindWindowWithOptional(WindowPtr w)
{
    do
        w = w->parent;
    while (!w->optional);
    return w;
}

/*
 * CheckWindowOptionalNeed
 *
 * check each optional entry in the given window to see if
 * the value is satisfied by the default rules.	 If so,
 * release the optional record
 */

void
CheckWindowOptionalNeed(WindowPtr w)
{
    WindowOptPtr optional;
    WindowOptPtr parentOptional;

    if (!w->parent || !w->optional)
        return;
    optional = w->optional;
    if (optional->dontPropagateMask != DontPropagateMasks[w->dontPropagate])
        return;
    if (optional->otherEventMasks != 0)
        return;
    if (optional->otherClients != NULL)
        return;
    if (optional->passiveGrabs != NULL)
        return;
    if (optional->userProps != NULL)
        return;
    if (optional->backingBitPlanes != (CARD32)~0L)
        return;
    if (optional->backingPixel != 0)
        return;
    if (optional->boundingShape != NULL)
        return;
    if (optional->clipShape != NULL)
        return;
    if (optional->inputShape != NULL)
        return;
    if (optional->inputMasks != NULL)
        return;
    if (optional->deviceCursors != NULL) {
        DevCursNodePtr pNode = optional->deviceCursors;

        while (pNode) {
            if (pNode->cursor != None)
                return;
            pNode = pNode->next;
        }
    }

    parentOptional = FindWindowWithOptional(w)->optional;
    if (optional->visual != parentOptional->visual)
        return;
    if (optional->cursor != None &&
        (optional->cursor != parentOptional->cursor || w->parent->cursorIsNone))
        return;
    if (optional->colormap != parentOptional->colormap)
        return;
    DisposeWindowOptional(w);
}

/*
 * MakeWindowOptional
 *
 * create an optional record and initialize it with the default
 * values.
 */

Bool
MakeWindowOptional(WindowPtr pWin)
{
    WindowOptPtr optional;
    WindowOptPtr parentOptional;

    if (pWin->optional)
        return TRUE;
    optional = malloc(sizeof(WindowOptRec));
    if (!optional)
        return FALSE;
    optional->dontPropagateMask = DontPropagateMasks[pWin->dontPropagate];
    optional->otherEventMasks = 0;
    optional->otherClients = NULL;
    optional->passiveGrabs = NULL;
    optional->userProps = NULL;
    optional->backingBitPlanes = ~0L;
    optional->backingPixel = 0;
    optional->boundingShape = NULL;
    optional->clipShape = NULL;
    optional->inputShape = NULL;
    optional->inputMasks = NULL;
    optional->deviceCursors = NULL;

    parentOptional = FindWindowWithOptional(pWin)->optional;
    optional->visual = parentOptional->visual;
    if (!pWin->cursorIsNone) {
        optional->cursor = RefCursor(parentOptional->cursor);
    }
    else {
        optional->cursor = None;
    }
    optional->colormap = parentOptional->colormap;
    pWin->optional = optional;
    return TRUE;
}

/*
 * Changes the cursor struct for the given device and the given window.
 * A cursor that does not have a device cursor set will use whatever the
 * standard cursor is for the window. If all devices have a cursor set,
 * changing the window cursor (e.g. using XDefineCursor()) will not have any
 * visible effect. Only when one of the device cursors is set to None again,
 * this device's cursor will display the changed standard cursor.
 *
 * CursorIsNone of the window struct is NOT modified if you set a device
 * cursor.
 *
 * Assumption: If there is a node for a device in the list, the device has a
 * cursor. If the cursor is set to None, it is inherited by the parent.
 */
int
ChangeWindowDeviceCursor(WindowPtr pWin, DeviceIntPtr pDev, CursorPtr pCursor)
{
    DevCursNodePtr pNode, pPrev;
    CursorPtr pOldCursor = NULL;
    ScreenPtr pScreen;
    WindowPtr pChild;

    if (!pWin->optional && !MakeWindowOptional(pWin))
        return BadAlloc;

    /* 1) Check if window has device cursor set
     *  Yes: 1.1) swap cursor with given cursor if parent does not have same
     *            cursor, free old cursor
     *       1.2) free old cursor, use parent cursor
     *  No: 1.1) add node to beginning of list.
     *      1.2) add cursor to node if parent does not have same cursor
     *      1.3) use parent cursor if parent does not have same cursor
     *  2) Patch up children if child has a devcursor
     *  2.1) if child has cursor None, it inherited from parent, set to old
     *  cursor
     *  2.2) if child has same cursor as new cursor, remove and set to None
     */

    pScreen = pWin->drawable.pScreen;

    if (WindowSeekDeviceCursor(pWin, pDev, &pNode, &pPrev)) {
        /* has device cursor */

        if (pNode->cursor == pCursor)
            return Success;

        pOldCursor = pNode->cursor;

        if (!pCursor) {         /* remove from list */
            if (pPrev)
                pPrev->next = pNode->next;
            else
                /* first item in list */
                pWin->optional->deviceCursors = pNode->next;

            free(pNode);
            goto out;
        }

    }
    else {
        /* no device cursor yet */
        DevCursNodePtr pNewNode;

        if (!pCursor)
            return Success;

        pNewNode = malloc(sizeof(DevCursNodeRec));
        pNewNode->dev = pDev;
        pNewNode->next = pWin->optional->deviceCursors;
        pWin->optional->deviceCursors = pNewNode;
        pNode = pNewNode;

    }

    if (pCursor && WindowParentHasDeviceCursor(pWin, pDev, pCursor))
        pNode->cursor = None;
    else {
        pNode->cursor = RefCursor(pCursor);
    }

    pNode = pPrev = NULL;
    /* fix up children */
    for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
        if (WindowSeekDeviceCursor(pChild, pDev, &pNode, &pPrev)) {
            if (pNode->cursor == None) {        /* inherited from parent */
                pNode->cursor = RefCursor(pOldCursor);
            }
            else if (pNode->cursor == pCursor) {
                pNode->cursor = None;
                FreeCursor(pCursor, (Cursor) 0);        /* fix up refcnt */
            }
        }
    }

 out:
    CursorVisible = TRUE;

    if (pWin->realized)
        WindowHasNewCursor(pWin);

    if (pOldCursor)
        FreeCursor(pOldCursor, (Cursor) 0);

    /* FIXME: We SHOULD check for an error value here XXX
       (comment taken from ChangeWindowAttributes) */
    (*pScreen->ChangeWindowAttributes) (pWin, CWCursor);

    return Success;
}

/* Get device cursor for given device or None if none is set */
CursorPtr
WindowGetDeviceCursor(WindowPtr pWin, DeviceIntPtr pDev)
{
    DevCursorList pList;

    if (!pWin->optional || !pWin->optional->deviceCursors)
        return NULL;

    pList = pWin->optional->deviceCursors;

    while (pList) {
        if (pList->dev == pDev) {
            if (pList->cursor == None)  /* inherited from parent */
                return WindowGetDeviceCursor(pWin->parent, pDev);
            else
                return pList->cursor;
        }
        pList = pList->next;
    }
    return NULL;
}

/* Searches for a DevCursorNode for the given window and device. If one is
 * found, return True and set pNode and pPrev to the node and to the node
 * before the node respectively. Otherwise return False.
 * If the device is the first in list, pPrev is set to NULL.
 */
static Bool
WindowSeekDeviceCursor(WindowPtr pWin,
                       DeviceIntPtr pDev,
                       DevCursNodePtr * pNode, DevCursNodePtr * pPrev)
{
    DevCursorList pList;

    if (!pWin->optional)
        return FALSE;

    pList = pWin->optional->deviceCursors;

    if (pList && pList->dev == pDev) {
        *pNode = pList;
        *pPrev = NULL;
        return TRUE;
    }

    while (pList) {
        if (pList->next) {
            if (pList->next->dev == pDev) {
                *pNode = pList->next;
                *pPrev = pList;
                return TRUE;
            }
        }
        pList = pList->next;
    }
    return FALSE;
}

/* Return True if a parent has the same device cursor set or False if
 * otherwise
 */
static Bool
WindowParentHasDeviceCursor(WindowPtr pWin,
                            DeviceIntPtr pDev, CursorPtr pCursor)
{
    WindowPtr pParent;
    DevCursNodePtr pParentNode, pParentPrev;

    pParent = pWin->parent;
    while (pParent) {
        if (WindowSeekDeviceCursor(pParent, pDev, &pParentNode, &pParentPrev)) {
            /* if there is a node in the list, the win has a dev cursor */
            if (!pParentNode->cursor)   /* inherited. */
                pParent = pParent->parent;
            else if (pParentNode->cursor == pCursor)    /* inherit */
                return TRUE;
            else                /* different cursor */
                return FALSE;
        }
        else
            /* parent does not have a device cursor for our device */
            return FALSE;
    }
    return FALSE;
}

/*
 * SetRootClip --
 *	Enable or disable rendering to the screen by
 *	setting the root clip list and revalidating
 *	all of the windows
 */
void
SetRootClip(ScreenPtr pScreen, int enable)
{
    WindowPtr pWin = pScreen->root;
    WindowPtr pChild;
    Bool WasViewable;
    Bool anyMarked = FALSE;
    WindowPtr pLayerWin;
    BoxRec box;
    enum RootClipMode mode = enable;

    if (!pWin)
        return;
    WasViewable = (Bool) (pWin->viewable);
    if (WasViewable) {
        for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib) {
            (void) (*pScreen->MarkOverlappedWindows) (pChild,
                                                      pChild, &pLayerWin);
        }
        (*pScreen->MarkWindow) (pWin);
        anyMarked = TRUE;
        if (pWin->valdata) {
            if (HasBorder(pWin)) {
                RegionPtr borderVisible;

                borderVisible = RegionCreate(NullBox, 1);
                RegionSubtract(borderVisible,
                               &pWin->borderClip, &pWin->winSize);
                pWin->valdata->before.borderVisible = borderVisible;
            }
            pWin->valdata->before.resized = TRUE;
        }
    }

    if (mode != ROOT_CLIP_NONE) {
        pWin->drawable.width = pScreen->width;
        pWin->drawable.height = pScreen->height;

        box.x1 = 0;
        box.y1 = 0;
        box.x2 = pScreen->width;
        box.y2 = pScreen->height;

        RegionInit(&pWin->winSize, &box, 1);
        RegionInit(&pWin->borderSize, &box, 1);

        /*
         * Use REGION_BREAK to avoid optimizations in ValidateTree
         * that assume the root borderClip can't change well, normally
         * it doesn't...)
         */
        RegionBreak(&pWin->clipList);

	/* For INPUT_ONLY, empty the borderClip so no rendering will ever
	 * be attempted to the screen pixmap (only redirected windows),
	 * but we keep borderSize as full regardless. */
        if (WasViewable && mode == ROOT_CLIP_FULL)
            RegionReset(&pWin->borderClip, &box);
        else
            RegionEmpty(&pWin->borderClip);
    }
    else {
        RegionEmpty(&pWin->borderClip);
        RegionBreak(&pWin->clipList);
    }

    ResizeChildrenWinSize(pWin, 0, 0, 0, 0);

    if (WasViewable) {
        if (pWin->firstChild) {
            anyMarked |= (*pScreen->MarkOverlappedWindows) (pWin->firstChild,
                                                            pWin->firstChild,
                                                            NULL);
        }
        else {
            (*pScreen->MarkWindow) (pWin);
            anyMarked = TRUE;
        }

        if (anyMarked) {
            (*pScreen->ValidateTree) (pWin, NullWindow, VTOther);
            (*pScreen->HandleExposures) (pWin);
            if (pScreen->PostValidateTree)
                (*pScreen->PostValidateTree) (pWin, NullWindow, VTOther);
        }
    }
    if (pWin->realized)
        WindowsRestructured();
    FlushAllOutput();
}

VisualPtr
WindowGetVisual(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    VisualID vid = wVisual(pWin);
    int i;

    for (i = 0; i < pScreen->numVisuals; i++)
        if (pScreen->visuals[i].vid == vid)
            return &pScreen->visuals[i];
    return 0;
}
