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

#ifndef CURSORSTRUCT_H
#define CURSORSTRUCT_H

#include "cursor.h"
#include "privates.h"
/*
 * device-independent cursor storage
 */

/*
 * source and mask point directly to the bits, which are in the server-defined
 * bitmap format.
 */
typedef struct _CursorBits {
    unsigned char *source;      /* points to bits */
    unsigned char *mask;        /* points to bits */
    Bool emptyMask;             /* all zeros mask */
    unsigned short width, height, xhot, yhot;   /* metrics */
    int refcnt;                 /* can be shared */
    PrivateRec *devPrivates;    /* set by pScr->RealizeCursor */
    CARD32 *argb;               /* full-color alpha blended */
} CursorBits, *CursorBitsPtr;

#define CURSOR_BITS_SIZE (sizeof(CursorBits) + dixPrivatesSize(PRIVATE_CURSOR_BITS))

typedef struct _Cursor {
    CursorBitsPtr bits;
    unsigned short foreRed, foreGreen, foreBlue;        /* device-independent color */
    unsigned short backRed, backGreen, backBlue;        /* device-independent color */
    int refcnt;
    PrivateRec *devPrivates;    /* set by pScr->RealizeCursor */
    XID id;
    CARD32 serialNumber;
    Atom name;
} CursorRec;

#define CURSOR_REC_SIZE (sizeof(CursorRec) + dixPrivatesSize(PRIVATE_CURSOR))

typedef struct _CursorMetric {
    unsigned short width, height, xhot, yhot;
} CursorMetricRec;

typedef struct {
    int x, y;
    ScreenPtr pScreen;
} HotSpot;

#endif                          /* CURSORSTRUCT_H */
