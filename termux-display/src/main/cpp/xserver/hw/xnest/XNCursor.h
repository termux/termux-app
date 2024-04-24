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

#ifndef XNESTCURSOR_H
#define XNESTCURSOR_H

#include "mipointrst.h"

typedef struct {
    miPointerSpriteFuncPtr spriteFuncs;
} xnestCursorFuncRec, *xnestCursorFuncPtr;

extern DevPrivateKeyRec xnestCursorScreenKeyRec;

#define xnestCursorScreenKey (&xnestCursorScreenKeyRec)
extern xnestCursorFuncRec xnestCursorFuncs;

typedef struct {
    Cursor cursor;
} xnestPrivCursor;

#define xnestGetCursorPriv(pCursor, pScreen) ((xnestPrivCursor *) \
    dixLookupScreenPrivate(&(pCursor)->devPrivates, CursorScreenKey, pScreen))

#define xnestSetCursorPriv(pCursor, pScreen, v) \
    dixSetScreenPrivate(&(pCursor)->devPrivates, CursorScreenKey, pScreen, v)

#define xnestCursor(pCursor, pScreen) \
  (xnestGetCursorPriv(pCursor, pScreen)->cursor)

Bool xnestRealizeCursor(DeviceIntPtr pDev,
                        ScreenPtr pScreen, CursorPtr pCursor);
Bool xnestUnrealizeCursor(DeviceIntPtr pDev,
                          ScreenPtr pScreen, CursorPtr pCursor);
void xnestRecolorCursor(ScreenPtr pScreen, CursorPtr pCursor, Bool displayed);
void xnestSetCursor(DeviceIntPtr pDev,
                    ScreenPtr pScreen, CursorPtr pCursor, int x, int y);
void xnestMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y);
Bool xnestDeviceCursorInitialize(DeviceIntPtr pDev, ScreenPtr pScreen);
void xnestDeviceCursorCleanup(DeviceIntPtr pDev, ScreenPtr pScreen);
#endif                          /* XNESTCURSOR_H */
