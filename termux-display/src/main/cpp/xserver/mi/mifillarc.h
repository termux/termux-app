/************************************************************

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

********************************************************/

#ifndef __MIFILLARC_H__
#define __MIFILLARC_H__

#define FULLCIRCLE (360 * 64)

typedef struct _miFillArc {
    int xorg, yorg;
    int y;
    int dx, dy;
    int e;
    int ym, yk, xm, xk;
} miFillArcRec;

/* could use 64-bit integers */
typedef struct _miFillArcD {
    int xorg, yorg;
    int y;
    int dx, dy;
    double e;
    double ym, yk, xm, xk;
} miFillArcDRec;

#define miFillArcEmpty(arc) (!(arc)->angle2 || \
			     !(arc)->width || !(arc)->height || \
			     (((arc)->width == 1) && ((arc)->height & 1)))

#define miCanFillArc(arc) (((arc)->width == (arc)->height) || \
			   (((arc)->width <= 800) && ((arc)->height <= 800)))

#define MIFILLARCSETUP() \
    x = 0; \
    y = info.y; \
    e = info.e; \
    xk = info.xk; \
    xm = info.xm; \
    yk = info.yk; \
    ym = info.ym; \
    dx = info.dx; \
    dy = info.dy; \
    xorg = info.xorg; \
    yorg = info.yorg

#define MIFILLARCSTEP(slw) \
    e += yk; \
    while (e >= 0) \
    { \
	x++; \
	xk -= xm; \
	e += xk; \
    } \
    y--; \
    yk -= ym; \
    slw = (x << 1) + dx; \
    if ((e == xk) && (slw > 1)) \
	slw--

#define MIFILLCIRCSTEP(slw) MIFILLARCSTEP(slw)
#define MIFILLELLSTEP(slw) MIFILLARCSTEP(slw)

#define miFillArcLower(slw) (((y + dy) != 0) && ((slw > 1) || (e != xk)))

typedef struct _miSliceEdge {
    int x;
    int stepx;
    int deltax;
    int e;
    int dy;
    int dx;
} miSliceEdgeRec, *miSliceEdgePtr;

typedef struct _miArcSlice {
    miSliceEdgeRec edge1, edge2;
    int min_top_y, max_top_y;
    int min_bot_y, max_bot_y;
    Bool edge1_top, edge2_top;
    Bool flip_top, flip_bot;
} miArcSliceRec;

#define MIARCSLICESTEP(edge) \
    edge.x -= edge.stepx; \
    edge.e -= edge.dx; \
    if (edge.e <= 0) \
    { \
	edge.x -= edge.deltax; \
	edge.e += edge.dy; \
    }

#define miFillSliceUpper(slice) \
		((y >= slice.min_top_y) && (y <= slice.max_top_y))

#define miFillSliceLower(slice) \
		((y >= slice.min_bot_y) && (y <= slice.max_bot_y))

#define MIARCSLICEUPPER(xl,xr,slice,slw) \
    xl = xorg - x; \
    xr = xl + slw - 1; \
    if (slice.edge1_top && (slice.edge1.x < xr)) \
	xr = slice.edge1.x; \
    if (slice.edge2_top && (slice.edge2.x > xl)) \
	xl = slice.edge2.x;

#define MIARCSLICELOWER(xl,xr,slice,slw) \
    xl = xorg - x; \
    xr = xl + slw - 1; \
    if (!slice.edge1_top && (slice.edge1.x > xl)) \
	xl = slice.edge1.x; \
    if (!slice.edge2_top && (slice.edge2.x < xr)) \
	xr = slice.edge2.x;

#define MIWIDEARCSETUP(x,y,dy,slw,e,xk,xm,yk,ym) \
    x = 0; \
    y = slw >> 1; \
    yk = y << 3; \
    xm = 8; \
    ym = 8; \
    if (dy) \
    { \
	xk = 0; \
	if (slw & 1) \
	    e = -1; \
	else \
	    e = -(y << 2) - 2; \
    } \
    else \
    { \
	y++; \
	yk += 4; \
	xk = -4; \
	if (slw & 1) \
	    e = -(y << 2) - 3; \
	else \
	    e = - (y << 3); \
    }

#define MIFILLINARCSTEP(slw) \
    ine += inyk; \
    while (ine >= 0) \
    { \
	inx++; \
	inxk -= inxm; \
	ine += inxk; \
    } \
    iny--; \
    inyk -= inym; \
    slw = (inx << 1) + dx; \
    if ((ine == inxk) && (slw > 1)) \
	slw--

#define miFillInArcLower(slw) (((iny + dy) != 0) && \
			       ((slw > 1) || (ine != inxk)))

#endif                          /* __MIFILLARC_H__ */
