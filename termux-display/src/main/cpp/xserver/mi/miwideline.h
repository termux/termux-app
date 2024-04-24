/*

Copyright 1988, 1998  The Open Group

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

*/

/* Author:  Keith Packard, MIT X Consortium */

#include "mifpoly.h"            /* for ICEIL */

/*
 * Polygon edge description for integer wide-line routines
 */

typedef struct _PolyEdge {
    int height;                 /* number of scanlines to process */
    int x;                      /* starting x coordinate */
    int stepx;                  /* fixed integral dx */
    int signdx;                 /* variable dx sign */
    int e;                      /* initial error term */
    int dy;
    int dx;
} PolyEdgeRec, *PolyEdgePtr;

#define SQSECANT 108.856472512142       /* 1/sin^2(11/2) - miter limit constant */

/*
 * types for general polygon routines
 */

typedef struct _PolyVertex {
    double x, y;
} PolyVertexRec, *PolyVertexPtr;

typedef struct _PolySlope {
    int dx, dy;
    double k;                   /* x0 * dy - y0 * dx */
} PolySlopeRec, *PolySlopePtr;

/*
 * Line face description for caps/joins
 */

typedef struct _LineFace {
    double xa, ya;
    int dx, dy;
    int x, y;
    double k;
} LineFaceRec, *LineFacePtr;

/*
 * macros for polygon fillers
 */

#define MILINESETPIXEL(pDrawable, pGC, pixel, oldPixel) { \
    oldPixel = pGC->fgPixel; \
    if (pixel != oldPixel) { \
	ChangeGCVal gcval; \
	gcval.val = pixel; \
	ChangeGC (NullClient, pGC, GCForeground, &gcval); \
	ValidateGC (pDrawable, pGC); \
    } \
}
#define MILINERESETPIXEL(pDrawable, pGC, pixel, oldPixel) { \
    if (pixel != oldPixel) { \
	ChangeGCVal gcval; \
	gcval.val = oldPixel; \
	ChangeGC (NullClient, pGC, GCForeground, &gcval); \
	ValidateGC (pDrawable, pGC); \
    } \
}
