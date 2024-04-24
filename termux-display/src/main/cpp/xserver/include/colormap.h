/*

Copyright 1987, 1998  The Open Group

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

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

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

#ifndef CMAP_H
#define CMAP_H 1

#include <X11/Xproto.h>
#include "screenint.h"
#include "window.h"

/* these follow X.h's AllocNone and AllocAll */
#define CM_PSCREEN 2
#define CM_PWIN	   3
/* Passed internally in colormap.c */
#define REDMAP 0
#define GREENMAP 1
#define BLUEMAP 2
#define PSEUDOMAP 3
#define AllocPrivate (-1)
#define AllocTemporary (-2)
#define DynamicClass  1

/* Values for the flags field of a colormap. These should have 1 bit set
 * and not overlap */
#define IsDefault 1
#define AllAllocated 2
#define BeingCreated 4

typedef CARD32 Pixel;
typedef struct _CMEntry *EntryPtr;

/* moved to screenint.h: typedef struct _ColormapRec *ColormapPtr */
typedef struct _colorResource *colorResourcePtr;

extern _X_EXPORT int CreateColormap(Colormap /*mid */ ,
                                    ScreenPtr /*pScreen */ ,
                                    VisualPtr /*pVisual */ ,
                                    ColormapPtr * /*ppcmap */ ,
                                    int /*alloc */ ,
                                    int /*client */ );

extern _X_EXPORT int FreeColormap(void *pmap,
                                  XID mid);

extern _X_EXPORT int TellLostMap(WindowPtr pwin,
                                 void *value);

extern _X_EXPORT int TellGainedMap(WindowPtr pwin,
                                   void *value);

extern _X_EXPORT int CopyColormapAndFree(Colormap /*mid */ ,
                                         ColormapPtr /*pSrc */ ,
                                         int /*client */ );

extern _X_EXPORT int AllocColor(ColormapPtr /*pmap */ ,
                                unsigned short * /*pred */ ,
                                unsigned short * /*pgreen */ ,
                                unsigned short * /*pblue */ ,
                                Pixel * /*pPix */ ,
                                int /*client */ );

extern _X_EXPORT void FakeAllocColor(ColormapPtr /*pmap */ ,
                                     xColorItem * /*item */ );

extern _X_EXPORT void FakeFreeColor(ColormapPtr /*pmap */ ,
                                    Pixel /*pixel */ );

extern _X_EXPORT int QueryColors(ColormapPtr /*pmap */ ,
                                 int /*count */ ,
                                 Pixel * /*ppixIn */ ,
                                 xrgb * /*prgbList */ ,
                                 ClientPtr client);

extern _X_EXPORT int FreeClientPixels(void *pcr,
                                      XID fakeid);

extern _X_EXPORT int AllocColorCells(int /*client */ ,
                                     ColormapPtr /*pmap */ ,
                                     int /*colors */ ,
                                     int /*planes */ ,
                                     Bool /*contig */ ,
                                     Pixel * /*ppix */ ,
                                     Pixel * /*masks */ );

extern _X_EXPORT int AllocColorPlanes(int /*client */ ,
                                      ColormapPtr /*pmap */ ,
                                      int /*colors */ ,
                                      int /*r */ ,
                                      int /*g */ ,
                                      int /*b */ ,
                                      Bool /*contig */ ,
                                      Pixel * /*pixels */ ,
                                      Pixel * /*prmask */ ,
                                      Pixel * /*pgmask */ ,
                                      Pixel * /*pbmask */ );

extern _X_EXPORT int FreeColors(ColormapPtr /*pmap */ ,
                                int /*client */ ,
                                int /*count */ ,
                                Pixel * /*pixels */ ,
                                Pixel /*mask */ );

extern _X_EXPORT int StoreColors(ColormapPtr /*pmap */ ,
                                 int /*count */ ,
                                 xColorItem * /*defs */ ,
                                 ClientPtr client);

extern _X_EXPORT int IsMapInstalled(Colormap /*map */ ,
                                    WindowPtr /*pWin */ );

extern _X_EXPORT Bool ResizeVisualArray(ScreenPtr /* pScreen */ ,
                                        int /* new_vis_count */ ,
                                        DepthPtr /* depth */ );

#endif                          /* CMAP_H */
