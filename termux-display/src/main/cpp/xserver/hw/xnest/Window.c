/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "gcstruct.h"
#include "window.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "colormapst.h"
#include "scrnintstr.h"
#include "region.h"

#include "mi.h"

#include "Xnest.h"

#include "Display.h"
#include "Screen.h"
#include "XNGC.h"
#include "Drawable.h"
#include "Color.h"
#include "Visual.h"
#include "Events.h"
#include "Args.h"

DevPrivateKeyRec xnestWindowPrivateKeyRec;

static int
xnestFindWindowMatch(WindowPtr pWin, void *ptr)
{
    xnestWindowMatch *wm = (xnestWindowMatch *) ptr;

    if (wm->window == xnestWindow(pWin)) {
        wm->pWin = pWin;
        return WT_STOPWALKING;
    }
    else
        return WT_WALKCHILDREN;
}

WindowPtr
xnestWindowPtr(Window window)
{
    xnestWindowMatch wm;
    int i;

    wm.pWin = NullWindow;
    wm.window = window;

    for (i = 0; i < xnestNumScreens; i++) {
        WalkTree(screenInfo.screens[i], xnestFindWindowMatch, (void *) &wm);
        if (wm.pWin)
            break;
    }

    return wm.pWin;
}

Bool
xnestCreateWindow(WindowPtr pWin)
{
    unsigned long mask;
    XSetWindowAttributes attributes;
    Visual *visual;
    ColormapPtr pCmap;

    if (pWin->drawable.class == InputOnly) {
        mask = 0L;
        visual = CopyFromParent;
    }
    else {
        mask = CWEventMask | CWBackingStore;
        attributes.event_mask = ExposureMask;
        attributes.backing_store = NotUseful;

        if (pWin->parent) {
            if (pWin->optional &&
                pWin->optional->visual != wVisual(pWin->parent)) {
                visual =
                    xnestVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
                mask |= CWColormap;
                if (pWin->optional->colormap) {
                    dixLookupResourceByType((void **) &pCmap, wColormap(pWin),
                                            RT_COLORMAP, serverClient,
                                            DixUseAccess);
                    attributes.colormap = xnestColormap(pCmap);
                }
                else
                    attributes.colormap = xnestDefaultVisualColormap(visual);
            }
            else
                visual = CopyFromParent;
        }
        else {                  /* root windows have their own colormaps at creation time */
            visual = xnestVisualFromID(pWin->drawable.pScreen, wVisual(pWin));
            dixLookupResourceByType((void **) &pCmap, wColormap(pWin),
                                    RT_COLORMAP, serverClient, DixUseAccess);
            mask |= CWColormap;
            attributes.colormap = xnestColormap(pCmap);
        }
    }

    xnestWindowPriv(pWin)->window = XCreateWindow(xnestDisplay,
                                                  xnestWindowParent(pWin),
                                                  pWin->origin.x -
                                                  wBorderWidth(pWin),
                                                  pWin->origin.y -
                                                  wBorderWidth(pWin),
                                                  pWin->drawable.width,
                                                  pWin->drawable.height,
                                                  pWin->borderWidth,
                                                  pWin->drawable.depth,
                                                  pWin->drawable.class,
                                                  visual, mask, &attributes);
    xnestWindowPriv(pWin)->parent = xnestWindowParent(pWin);
    xnestWindowPriv(pWin)->x = pWin->origin.x - wBorderWidth(pWin);
    xnestWindowPriv(pWin)->y = pWin->origin.y - wBorderWidth(pWin);
    xnestWindowPriv(pWin)->width = pWin->drawable.width;
    xnestWindowPriv(pWin)->height = pWin->drawable.height;
    xnestWindowPriv(pWin)->border_width = pWin->borderWidth;
    xnestWindowPriv(pWin)->sibling_above = None;
    if (pWin->nextSib)
        xnestWindowPriv(pWin->nextSib)->sibling_above = xnestWindow(pWin);
    xnestWindowPriv(pWin)->bounding_shape = RegionCreate(NULL, 1);
    xnestWindowPriv(pWin)->clip_shape = RegionCreate(NULL, 1);

    if (!pWin->parent)          /* only the root window will have the right colormap */
        xnestSetInstalledColormapWindows(pWin->drawable.pScreen);

    return True;
}

Bool
xnestDestroyWindow(WindowPtr pWin)
{
    if (pWin->nextSib)
        xnestWindowPriv(pWin->nextSib)->sibling_above =
            xnestWindowPriv(pWin)->sibling_above;
    RegionDestroy(xnestWindowPriv(pWin)->bounding_shape);
    RegionDestroy(xnestWindowPriv(pWin)->clip_shape);
    XDestroyWindow(xnestDisplay, xnestWindow(pWin));
    xnestWindowPriv(pWin)->window = None;

    if (pWin->optional && pWin->optional->colormap && pWin->parent)
        xnestSetInstalledColormapWindows(pWin->drawable.pScreen);

    return True;
}

Bool
xnestPositionWindow(WindowPtr pWin, int x, int y)
{
    xnestConfigureWindow(pWin,
                         CWParent |
                         CWX | CWY | CWWidth | CWHeight | CWBorderWidth);

    return True;
}

void
xnestConfigureWindow(WindowPtr pWin, unsigned int mask)
{
    unsigned int valuemask;
    XWindowChanges values;

    if (mask & CWParent &&
        xnestWindowPriv(pWin)->parent != xnestWindowParent(pWin)) {
        XReparentWindow(xnestDisplay, xnestWindow(pWin),
                        xnestWindowParent(pWin),
                        pWin->origin.x - wBorderWidth(pWin),
                        pWin->origin.y - wBorderWidth(pWin));
        xnestWindowPriv(pWin)->parent = xnestWindowParent(pWin);
        xnestWindowPriv(pWin)->x = pWin->origin.x - wBorderWidth(pWin);
        xnestWindowPriv(pWin)->y = pWin->origin.y - wBorderWidth(pWin);
        xnestWindowPriv(pWin)->sibling_above = None;
        if (pWin->nextSib)
            xnestWindowPriv(pWin->nextSib)->sibling_above = xnestWindow(pWin);
    }

    valuemask = 0;

    if (mask & CWX &&
        xnestWindowPriv(pWin)->x != pWin->origin.x - wBorderWidth(pWin)) {
        valuemask |= CWX;
        values.x =
            xnestWindowPriv(pWin)->x = pWin->origin.x - wBorderWidth(pWin);
    }

    if (mask & CWY &&
        xnestWindowPriv(pWin)->y != pWin->origin.y - wBorderWidth(pWin)) {
        valuemask |= CWY;
        values.y =
            xnestWindowPriv(pWin)->y = pWin->origin.y - wBorderWidth(pWin);
    }

    if (mask & CWWidth && xnestWindowPriv(pWin)->width != pWin->drawable.width) {
        valuemask |= CWWidth;
        values.width = xnestWindowPriv(pWin)->width = pWin->drawable.width;
    }

    if (mask & CWHeight &&
        xnestWindowPriv(pWin)->height != pWin->drawable.height) {
        valuemask |= CWHeight;
        values.height = xnestWindowPriv(pWin)->height = pWin->drawable.height;
    }

    if (mask & CWBorderWidth &&
        xnestWindowPriv(pWin)->border_width != pWin->borderWidth) {
        valuemask |= CWBorderWidth;
        values.border_width =
            xnestWindowPriv(pWin)->border_width = pWin->borderWidth;
    }

    if (valuemask)
        XConfigureWindow(xnestDisplay, xnestWindow(pWin), valuemask, &values);

    if (mask & CWStackingOrder &&
        xnestWindowPriv(pWin)->sibling_above != xnestWindowSiblingAbove(pWin)) {
        WindowPtr pSib;

        /* find the top sibling */
        for (pSib = pWin; pSib->prevSib != NullWindow; pSib = pSib->prevSib);

        /* the top sibling */
        valuemask = CWStackMode;
        values.stack_mode = Above;
        XConfigureWindow(xnestDisplay, xnestWindow(pSib), valuemask, &values);
        xnestWindowPriv(pSib)->sibling_above = None;

        /* the rest of siblings */
        for (pSib = pSib->nextSib; pSib != NullWindow; pSib = pSib->nextSib) {
            valuemask = CWSibling | CWStackMode;
            values.sibling = xnestWindowSiblingAbove(pSib);
            values.stack_mode = Below;
            XConfigureWindow(xnestDisplay, xnestWindow(pSib), valuemask,
                             &values);
            xnestWindowPriv(pSib)->sibling_above =
                xnestWindowSiblingAbove(pSib);
        }
    }
}

Bool
xnestChangeWindowAttributes(WindowPtr pWin, unsigned long mask)
{
    XSetWindowAttributes attributes;

    if (mask & CWBackPixmap)
        switch (pWin->backgroundState) {
        case None:
            attributes.background_pixmap = None;
            break;

        case ParentRelative:
            attributes.background_pixmap = ParentRelative;
            break;

        case BackgroundPixmap:
            attributes.background_pixmap = xnestPixmap(pWin->background.pixmap);
            break;

        case BackgroundPixel:
            mask &= ~CWBackPixmap;
            break;
        }

    if (mask & CWBackPixel) {
        if (pWin->backgroundState == BackgroundPixel)
            attributes.background_pixel = xnestPixel(pWin->background.pixel);
        else
            mask &= ~CWBackPixel;
    }

    if (mask & CWBorderPixmap) {
        if (pWin->borderIsPixel)
            mask &= ~CWBorderPixmap;
        else
            attributes.border_pixmap = xnestPixmap(pWin->border.pixmap);
    }

    if (mask & CWBorderPixel) {
        if (pWin->borderIsPixel)
            attributes.border_pixel = xnestPixel(pWin->border.pixel);
        else
            mask &= ~CWBorderPixel;
    }

    if (mask & CWBitGravity)
        attributes.bit_gravity = pWin->bitGravity;

    if (mask & CWWinGravity)    /* dix does this for us */
        mask &= ~CWWinGravity;

    if (mask & CWBackingStore)  /* this is really not useful */
        mask &= ~CWBackingStore;

    if (mask & CWBackingPlanes) /* this is really not useful */
        mask &= ~CWBackingPlanes;

    if (mask & CWBackingPixel)  /* this is really not useful */
        mask &= ~CWBackingPixel;

    if (mask & CWOverrideRedirect)
        attributes.override_redirect = pWin->overrideRedirect;

    if (mask & CWSaveUnder)     /* this is really not useful */
        mask &= ~CWSaveUnder;

    if (mask & CWEventMask)     /* events are handled elsewhere */
        mask &= ~CWEventMask;

    if (mask & CWDontPropagate) /* events are handled elsewhere */
        mask &= ~CWDontPropagate;

    if (mask & CWColormap) {
        ColormapPtr pCmap;

        dixLookupResourceByType((void **) &pCmap, wColormap(pWin),
                                RT_COLORMAP, serverClient, DixUseAccess);

        attributes.colormap = xnestColormap(pCmap);

        xnestSetInstalledColormapWindows(pWin->drawable.pScreen);
    }

    if (mask & CWCursor)        /* this is handled in cursor code */
        mask &= ~CWCursor;

    if (mask)
        XChangeWindowAttributes(xnestDisplay, xnestWindow(pWin),
                                mask, &attributes);

    return True;
}

Bool
xnestRealizeWindow(WindowPtr pWin)
{
    xnestConfigureWindow(pWin, CWStackingOrder);
    xnestShapeWindow(pWin);
    XMapWindow(xnestDisplay, xnestWindow(pWin));

    return True;
}

Bool
xnestUnrealizeWindow(WindowPtr pWin)
{
    XUnmapWindow(xnestDisplay, xnestWindow(pWin));

    return True;
}

void
xnestCopyWindow(WindowPtr pWin, xPoint oldOrigin, RegionPtr oldRegion)
{
}

void
xnestClipNotify(WindowPtr pWin, int dx, int dy)
{
    xnestConfigureWindow(pWin, CWStackingOrder);
    xnestShapeWindow(pWin);
}

static Bool
xnestWindowExposurePredicate(Display * dpy, XEvent * event, XPointer ptr)
{
    return (event->type == Expose && event->xexpose.window == *(Window *) ptr);
}

void
xnestWindowExposures(WindowPtr pWin, RegionPtr pRgn)
{
    XEvent event;
    Window window;
    BoxRec Box;

    XSync(xnestDisplay, False);

    window = xnestWindow(pWin);

    while (XCheckIfEvent(xnestDisplay, &event,
                         xnestWindowExposurePredicate, (char *) &window)) {

        Box.x1 = pWin->drawable.x + wBorderWidth(pWin) + event.xexpose.x;
        Box.y1 = pWin->drawable.y + wBorderWidth(pWin) + event.xexpose.y;
        Box.x2 = Box.x1 + event.xexpose.width;
        Box.y2 = Box.y1 + event.xexpose.height;

        event.xexpose.type = ProcessedExpose;

        if (RegionContainsRect(pRgn, &Box) != rgnIN)
            XPutBackEvent(xnestDisplay, &event);
    }

    miWindowExposures(pWin, pRgn);
}

void
xnestSetShape(WindowPtr pWin, int kind)
{
    xnestShapeWindow(pWin);
    miSetShape(pWin, kind);
}

static Bool
xnestRegionEqual(RegionPtr pReg1, RegionPtr pReg2)
{
    BoxPtr pBox1, pBox2;
    unsigned int n1, n2;

    if (pReg1 == pReg2)
        return True;

    if (pReg1 == NullRegion || pReg2 == NullRegion)
        return False;

    pBox1 = RegionRects(pReg1);
    n1 = RegionNumRects(pReg1);

    pBox2 = RegionRects(pReg2);
    n2 = RegionNumRects(pReg2);

    if (n1 != n2)
        return False;

    if (pBox1 == pBox2)
        return True;

    if (memcmp(pBox1, pBox2, n1 * sizeof(BoxRec)))
        return False;

    return True;
}

void
xnestShapeWindow(WindowPtr pWin)
{
    Region reg;
    BoxPtr pBox;
    XRectangle rect;
    int i;

    if (!xnestRegionEqual(xnestWindowPriv(pWin)->bounding_shape,
                          wBoundingShape(pWin))) {

        if (wBoundingShape(pWin)) {
            RegionCopy(xnestWindowPriv(pWin)->bounding_shape,
                       wBoundingShape(pWin));

            reg = XCreateRegion();
            pBox = RegionRects(xnestWindowPriv(pWin)->bounding_shape);
            for (i = 0;
                 i < RegionNumRects(xnestWindowPriv(pWin)->bounding_shape);
                 i++) {
                rect.x = pBox[i].x1;
                rect.y = pBox[i].y1;
                rect.width = pBox[i].x2 - pBox[i].x1;
                rect.height = pBox[i].y2 - pBox[i].y1;
                XUnionRectWithRegion(&rect, reg, reg);
            }
            XShapeCombineRegion(xnestDisplay, xnestWindow(pWin),
                                ShapeBounding, 0, 0, reg, ShapeSet);
            XDestroyRegion(reg);
        }
        else {
            RegionEmpty(xnestWindowPriv(pWin)->bounding_shape);

            XShapeCombineMask(xnestDisplay, xnestWindow(pWin),
                              ShapeBounding, 0, 0, None, ShapeSet);
        }
    }

    if (!xnestRegionEqual(xnestWindowPriv(pWin)->clip_shape, wClipShape(pWin))) {

        if (wClipShape(pWin)) {
            RegionCopy(xnestWindowPriv(pWin)->clip_shape, wClipShape(pWin));

            reg = XCreateRegion();
            pBox = RegionRects(xnestWindowPriv(pWin)->clip_shape);
            for (i = 0;
                 i < RegionNumRects(xnestWindowPriv(pWin)->clip_shape); i++) {
                rect.x = pBox[i].x1;
                rect.y = pBox[i].y1;
                rect.width = pBox[i].x2 - pBox[i].x1;
                rect.height = pBox[i].y2 - pBox[i].y1;
                XUnionRectWithRegion(&rect, reg, reg);
            }
            XShapeCombineRegion(xnestDisplay, xnestWindow(pWin),
                                ShapeClip, 0, 0, reg, ShapeSet);
            XDestroyRegion(reg);
        }
        else {
            RegionEmpty(xnestWindowPriv(pWin)->clip_shape);

            XShapeCombineMask(xnestDisplay, xnestWindow(pWin),
                              ShapeClip, 0, 0, None, ShapeSet);
        }
    }
}
