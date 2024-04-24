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

/* THIS IS NOT AN X PROJECT TEAM SPECIFICATION */

/*
 *	PanoramiX definitions
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#ifndef _PANORAMIX_H_
#define _PANORAMIX_H_

#define _PANORAMIX_SERVER
#include <X11/extensions/panoramiXproto.h>
#undef _PANORAMIX_SERVER
#include "gcstruct.h"
#include "dixstruct.h"

typedef struct _PanoramiXInfo {
    XID id;
} PanoramiXInfo;

typedef struct {
    PanoramiXInfo info[MAXSCREENS];
    RESTYPE type;
    union {
        struct {
            char visibility;
            char class;
            char root;
        } win;
        struct {
            Bool shared;
        } pix;
        struct {
            Bool root;
        } pict;
        char raw_data[4];
    } u;
} PanoramiXRes;

#define FOR_NSCREENS_FORWARD(j) for(j = 0; j < PanoramiXNumScreens; j++)
#define FOR_NSCREENS_FORWARD_SKIP(j) for(j = 1; j < PanoramiXNumScreens; j++)
#define FOR_NSCREENS_BACKWARD(j) for(j = PanoramiXNumScreens - 1; j >= 0; j--)
#define FOR_NSCREENS(j) FOR_NSCREENS_FORWARD(j)

#define IS_SHARED_PIXMAP(r) (((r)->type == XRT_PIXMAP) && (r)->u.pix.shared)

#define IS_ROOT_DRAWABLE(d) (((d)->type == XRT_WINDOW) && (d)->u.win.root)
#endif                          /* _PANORAMIX_H_ */
