/*

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.
*/

#ifndef MIPOINTER_H
#define MIPOINTER_H

#include "cursor.h"
#include "input.h"
#include "privates.h"

typedef struct _miPointerSpriteFuncRec {
    Bool (*RealizeCursor) (DeviceIntPtr /* pDev */ ,
                           ScreenPtr /* pScr */ ,
                           CursorPtr    /* pCurs */
        );
    Bool (*UnrealizeCursor) (DeviceIntPtr /* pDev */ ,
                             ScreenPtr /* pScr */ ,
                             CursorPtr  /* pCurs */
        );
    void (*SetCursor) (DeviceIntPtr /* pDev */ ,
                       ScreenPtr /* pScr */ ,
                       CursorPtr /* pCurs */ ,
                       int /* x */ ,
                       int      /* y */
        );
    void (*MoveCursor) (DeviceIntPtr /* pDev */ ,
                        ScreenPtr /* pScr */ ,
                        int /* x */ ,
                        int     /* y */
        );
    Bool (*DeviceCursorInitialize) (DeviceIntPtr /* pDev */ ,
                                    ScreenPtr   /* pScr */
        );
    void (*DeviceCursorCleanup) (DeviceIntPtr /* pDev */ ,
                                 ScreenPtr      /* pScr */
        );
} miPointerSpriteFuncRec, *miPointerSpriteFuncPtr;

typedef struct _miPointerScreenFuncRec {
    Bool (*CursorOffScreen) (ScreenPtr * /* ppScr */ ,
                             int * /* px */ ,
                             int *      /* py */
        );
    void (*CrossScreen) (ScreenPtr /* pScr */ ,
                         int    /* entering */
        );
    void (*WarpCursor) (DeviceIntPtr /*pDev */ ,
                        ScreenPtr /* pScr */ ,
                        int /* x */ ,
                        int     /* y */
        );
} miPointerScreenFuncRec, *miPointerScreenFuncPtr;

extern _X_EXPORT Bool miDCInitialize(ScreenPtr /*pScreen */ ,
                                     miPointerScreenFuncPtr     /*screenFuncs */
    );

extern _X_EXPORT Bool miPointerInitialize(ScreenPtr /*pScreen */ ,
                                          miPointerSpriteFuncPtr
                                          /*spriteFuncs */ ,
                                          miPointerScreenFuncPtr
                                          /*screenFuncs */ ,
                                          Bool  /*waitForUpdate */
    );

extern _X_EXPORT void miPointerWarpCursor(DeviceIntPtr /*pDev */ ,
                                          ScreenPtr /*pScreen */ ,
                                          int /*x */ ,
                                          int   /*y */
    );

extern _X_EXPORT ScreenPtr
miPointerGetScreen(DeviceIntPtr pDev);
extern _X_EXPORT void
miPointerSetScreen(DeviceIntPtr pDev, int screen_num, int x, int y);

/* Returns the current cursor position. */
extern _X_EXPORT void
miPointerGetPosition(DeviceIntPtr pDev, int *x, int *y);

/* Moves the cursor to the specified position.  May clip the coordinates:
 * x and y are modified in-place. */
extern _X_EXPORT ScreenPtr
miPointerSetPosition(DeviceIntPtr pDev, int mode, double *x, double *y,
                     int *nevents, InternalEvent *events);

extern _X_EXPORT void
miPointerUpdateSprite(DeviceIntPtr pDev);

/* Invalidate current sprite, forcing reload on next
 * sprite setting (window crossing, grab action, etc)
 */
extern _X_EXPORT void
miPointerInvalidateSprite(DeviceIntPtr pDev);

/* Sets whether the sprite should be updated immediately on pointer moves */
extern _X_EXPORT Bool
miPointerSetWaitForUpdate(ScreenPtr pScreen, Bool wait);

extern _X_EXPORT DevPrivateKeyRec miPointerPrivKeyRec;

#define miPointerPrivKey (&miPointerPrivKeyRec)

extern _X_EXPORT DevPrivateKeyRec miPointerScreenKeyRec;

#define miPointerScreenKey (&miPointerScreenKeyRec)

#endif                          /* MIPOINTER_H */
