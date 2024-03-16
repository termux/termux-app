/*

   XFree86 Xv DDX written by Mark Vojkovich (markv@valinux.com)
   Adapted for KDrive by Pontus Lidman <pontus.lidman@nokia.com>

   Copyright (C) 2000, 2001 - Nokia Home Communications
   Copyright (C) 1998, 1999 - The XFree86 Project Inc.

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, and/or sell copies of the Software, and to permit persons
to whom the Software is furnished to do so, provided that the above
copyright notice(s) and this permission notice appear in all copies of
the Software and that both the above copyright notice(s) and this
permission notice appear in supporting documentation.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT
OF THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
HOLDERS INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY
SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Except as contained in this notice, the name of a copyright holder
shall not be used in advertising or otherwise to promote the sale, use
or other dealings in this Software without prior written authorization
of the copyright holder.

*/

#ifndef _XVDIX_H_
#define _XVDIX_H_

#include "scrnintstr.h"
#include "regionstr.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "mivalidate.h"
#include "validate.h"
#include "resource.h"
#include "gcstruct.h"
#include "dixstruct.h"

#include "../../Xext/xvdix.h"

#define VIDEO_OVERLAID_IMAGES			0x00000004
#define VIDEO_OVERLAID_STILLS			0x00000008
#define VIDEO_CLIP_TO_VIEWPORT			0x00000010

typedef int (*PutVideoFuncPtr) (KdScreenInfo * screen, DrawablePtr pDraw,
                                short vid_x, short vid_y, short drw_x,
                                short drw_y, short vid_w, short vid_h,
                                short drw_w, short drw_h, RegionPtr clipBoxes,
                                void *data);
typedef int (*PutStillFuncPtr) (KdScreenInfo * screen, DrawablePtr pDraw,
                                short vid_x, short vid_y, short drw_x,
                                short drw_y, short vid_w, short vid_h,
                                short drw_w, short drw_h, RegionPtr clipBoxes,
                                void *data);
typedef int (*GetVideoFuncPtr) (KdScreenInfo * screen, DrawablePtr pDraw,
                                short vid_x, short vid_y, short drw_x,
                                short drw_y, short vid_w, short vid_h,
                                short drw_w, short drw_h, RegionPtr clipBoxes,
                                void *data);
typedef int (*GetStillFuncPtr) (KdScreenInfo * screen, DrawablePtr pDraw,
                                short vid_x, short vid_y, short drw_x,
                                short drw_y, short vid_w, short vid_h,
                                short drw_w, short drw_h, RegionPtr clipBoxes,
                                void *data);
typedef void (*StopVideoFuncPtr) (KdScreenInfo * screen, void *data,
                                  Bool Exit);
typedef int (*SetPortAttributeFuncPtr) (KdScreenInfo * screen, Atom attribute,
                                        int value, void *data);
typedef int (*GetPortAttributeFuncPtr) (KdScreenInfo * screen, Atom attribute,
                                        int *value, void *data);
typedef void (*QueryBestSizeFuncPtr) (KdScreenInfo * screen, Bool motion,
                                      short vid_w, short vid_h, short drw_w,
                                      short drw_h, unsigned int *p_w,
                                      unsigned int *p_h, void *data);
typedef int (*PutImageFuncPtr) (KdScreenInfo * screen, DrawablePtr pDraw,
                                short src_x, short src_y, short drw_x,
                                short drw_y, short src_w, short src_h,
                                short drw_w, short drw_h, int image,
                                unsigned char *buf, short width, short height,
                                Bool Sync, RegionPtr clipBoxes, void *data);
typedef int (*ReputImageFuncPtr) (KdScreenInfo * screen, DrawablePtr pDraw,
                                  short drw_x, short drw_y, RegionPtr clipBoxes,
                                  void *data);
typedef int (*QueryImageAttributesFuncPtr) (KdScreenInfo * screen, int image,
                                            unsigned short *width,
                                            unsigned short *height,
                                            int *pitches, int *offsets);

typedef enum {
    XV_OFF,
    XV_PENDING,
    XV_ON
} XvStatus;

/*** this is what the driver needs to fill out ***/

typedef struct {
    int id;
    const char *name;
    unsigned short width, height;
    XvRationalRec rate;
} KdVideoEncodingRec, *KdVideoEncodingPtr;

typedef struct {
    char depth;
    short class;
} KdVideoFormatRec, *KdVideoFormatPtr;

typedef struct {
    unsigned int type;
    int flags;
    const char *name;
    int nEncodings;
    KdVideoEncodingPtr pEncodings;
    int nFormats;
    KdVideoFormatPtr pFormats;
    int nPorts;
    DevUnion *pPortPrivates;
    int nAttributes;
    XvAttributePtr pAttributes;
    int nImages;
    XvImagePtr pImages;
    PutVideoFuncPtr PutVideo;
    PutStillFuncPtr PutStill;
    GetVideoFuncPtr GetVideo;
    GetStillFuncPtr GetStill;
    StopVideoFuncPtr StopVideo;
    SetPortAttributeFuncPtr SetPortAttribute;
    GetPortAttributeFuncPtr GetPortAttribute;
    QueryBestSizeFuncPtr QueryBestSize;
    PutImageFuncPtr PutImage;
    ReputImageFuncPtr ReputImage;
    QueryImageAttributesFuncPtr QueryImageAttributes;
} KdVideoAdaptorRec, *KdVideoAdaptorPtr;

Bool
 KdXVScreenInit(ScreenPtr pScreen, KdVideoAdaptorPtr Adaptors, int num);

/*** These are DDX layer privates ***/

typedef struct {
    DestroyWindowProcPtr DestroyWindow;
    ClipNotifyProcPtr ClipNotify;
    WindowExposuresProcPtr WindowExposures;
    CloseScreenProcPtr CloseScreen;
} KdXVScreenRec, *KdXVScreenPtr;

typedef struct {
    int flags;
    PutVideoFuncPtr PutVideo;
    PutStillFuncPtr PutStill;
    GetVideoFuncPtr GetVideo;
    GetStillFuncPtr GetStill;
    StopVideoFuncPtr StopVideo;
    SetPortAttributeFuncPtr SetPortAttribute;
    GetPortAttributeFuncPtr GetPortAttribute;
    QueryBestSizeFuncPtr QueryBestSize;
    PutImageFuncPtr PutImage;
    ReputImageFuncPtr ReputImage;
    QueryImageAttributesFuncPtr QueryImageAttributes;
} XvAdaptorRecPrivate, *XvAdaptorRecPrivatePtr;

typedef struct {
    KdScreenInfo *screen;
    DrawablePtr pDraw;
    unsigned char type;
    unsigned int subWindowMode;
    DDXPointRec clipOrg;
    RegionPtr clientClip;
    RegionPtr pCompositeClip;
    Bool FreeCompositeClip;
    XvAdaptorRecPrivatePtr AdaptorRec;
    XvStatus isOn;
    Bool moved;
    int vid_x, vid_y, vid_w, vid_h;
    int drw_x, drw_y, drw_w, drw_h;
    DevUnion DevPriv;
} XvPortRecPrivate, *XvPortRecPrivatePtr;

typedef struct _KdXVWindowRec {
    XvPortRecPrivatePtr PortRec;
    struct _KdXVWindowRec *next;
} KdXVWindowRec, *KdXVWindowPtr;

#endif                          /* _XVDIX_H_ */
