/***********************************************************

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

******************************************************************/

#ifndef GC_H
#define GC_H

#include <X11/X.h>              /* for GContext, Mask */
#include <X11/Xdefs.h>          /* for Bool */
#include <X11/Xproto.h>
#include "screenint.h"          /* for ScreenPtr */
#include "pixmap.h"             /* for DrawablePtr */

/* clientClipType field in GC */
#define CT_NONE			0
#define CT_PIXMAP		1
#define CT_REGION		2
#define CT_UNSORTED		6
#define CT_YSORTED		10
#define CT_YXSORTED		14
#define CT_YXBANDED		18

#define GCQREASON_VALIDATE	1
#define GCQREASON_CHANGE	2
#define GCQREASON_COPY_SRC	3
#define GCQREASON_COPY_DST	4
#define GCQREASON_DESTROY	5

#define GC_CHANGE_SERIAL_BIT        (((unsigned long)1)<<31)
#define GC_CALL_VALIDATE_BIT        (1L<<30)
#define GCExtensionInterest   (1L<<29)

#define DRAWABLE_SERIAL_BITS        (~(GC_CHANGE_SERIAL_BIT))

#define MAX_SERIAL_NUM     (1L<<28)

#define NEXT_SERIAL_NUMBER ((++globalSerialNumber) > MAX_SERIAL_NUM ? \
	    (globalSerialNumber  = 1): globalSerialNumber)

typedef struct _GCInterest *GCInterestPtr;
typedef struct _GC *GCPtr;
typedef struct _GCOps *GCOpsPtr;

extern _X_EXPORT void ValidateGC(DrawablePtr /*pDraw */ ,
                                 GCPtr /*pGC */ );

typedef union {
    CARD32 val;
    void *ptr;
} ChangeGCVal, *ChangeGCValPtr;

extern int ChangeGCXIDs(ClientPtr /*client */ ,
                        GCPtr /*pGC */ ,
                        BITS32 /*mask */ ,
                        CARD32 * /*pval */ );

extern _X_EXPORT int ChangeGC(ClientPtr /*client */ ,
                              GCPtr /*pGC */ ,
                              BITS32 /*mask */ ,
                              ChangeGCValPtr /*pCGCV */ );

extern _X_EXPORT GCPtr CreateGC(DrawablePtr /*pDrawable */ ,
                                BITS32 /*mask */ ,
                                XID * /*pval */ ,
                                int * /*pStatus */ ,
                                XID /*gcid */ ,
                                ClientPtr /*client */ );

extern _X_EXPORT int CopyGC(GCPtr /*pgcSrc */ ,
                            GCPtr /*pgcDst */ ,
                            BITS32 /*mask */ );

extern _X_EXPORT int FreeGC(void *pGC,
                            XID gid);

extern _X_EXPORT void FreeGCperDepth(int /*screenNum */ );

extern _X_EXPORT Bool CreateGCperDepth(int /*screenNum */ );

extern _X_EXPORT Bool CreateDefaultStipple(int /*screenNum */ );

extern _X_EXPORT void FreeDefaultStipple(int /*screenNum */ );

extern _X_EXPORT int SetDashes(GCPtr /*pGC */ ,
                               unsigned /*offset */ ,
                               unsigned /*ndash */ ,
                               unsigned char * /*pdash */ );

extern _X_EXPORT int VerifyRectOrder(int /*nrects */ ,
                                     xRectangle * /*prects */ ,
                                     int /*ordering */ );

extern _X_EXPORT int SetClipRects(GCPtr /*pGC */ ,
                                  int /*xOrigin */ ,
                                  int /*yOrigin */ ,
                                  int /*nrects */ ,
                                  xRectangle * /*prects */ ,
                                  int /*ordering */ );

extern _X_EXPORT GCPtr GetScratchGC(unsigned /*depth */ ,
                                    ScreenPtr /*pScreen */ );

extern _X_EXPORT void FreeScratchGC(GCPtr /*pGC */ );

#endif                          /* GC_H */
