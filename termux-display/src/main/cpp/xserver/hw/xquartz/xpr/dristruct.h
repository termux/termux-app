/**************************************************************************

   Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
   Copyright (c) 2002-2012 Apple Computer, Inc.
   All Rights Reserved.

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sub license, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice (including the
   next paragraph) shall be included in all copies or substantial portions
   of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
   IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
   ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Jens Owen <jens@precisioninsight.com>
 *   Jeremy Huddleston <jeremyhu@apple.com>
 */

#ifndef DRI_STRUCT_H
#define DRI_STRUCT_H

#include "dri.h"
#include "x-list.h"

#define DRI_MAX_DRAWABLES 256

#define DRI_DRAWABLE_PRIV_FROM_WINDOW(pWin) ((DRIDrawablePrivPtr) \
                                             dixLookupPrivate(&(pWin)-> \
                                                              devPrivates, \
                                                              DRIWindowPrivKey))

#define DRI_DRAWABLE_PRIV_FROM_PIXMAP(pPix) ((DRIDrawablePrivPtr) \
                                             dixLookupPrivate(&(pPix)-> \
                                                              devPrivates, \
                                                              DRIPixmapPrivKey))

typedef struct _DRIDrawablePrivRec {
    xp_surface_id sid;
    int drawableIndex;
    DrawablePtr pDraw;
    ScreenPtr pScreen;
    int refCount;
    unsigned int key[2];
    x_list          *notifiers;     /* list of (FUN . DATA) */
} DRIDrawablePrivRec, *DRIDrawablePrivPtr;

#define DRI_SCREEN_PRIV(pScreen)                ((DRIScreenPrivPtr) \
                                                 dixLookupPrivate(&(pScreen) \
                                                                  -> \
                                                                  devPrivates, \
                                                                  DRIScreenPrivKey))

#define DRI_SCREEN_PRIV_FROM_INDEX(screenIndex) ((DRIScreenPrivPtr) \
                                                 dixLookupPrivate(&screenInfo \
                                                                  .screens[ \
                                                                      screenIndex \
                                                                  ]-> \
                                                                  devPrivates, \
                                                                  DRIScreenPrivKey))

typedef struct _DRIScreenPrivRec {
    Bool directRenderingSupport;
    int nrWindows;
    DRIWrappedFuncsRec wrap;
    DrawablePtr DRIDrawables[DRI_MAX_DRAWABLES];
} DRIScreenPrivRec, *DRIScreenPrivPtr;

#endif /* DRI_STRUCT_H */
