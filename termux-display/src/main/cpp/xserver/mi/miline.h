
/*

Copyright 1994, 1998  The Open Group

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

#ifndef MILINE_H

#include "screenint.h"
#include "privates.h"

/*
 * Public definitions used for configuring basic pixelization aspects
 * of the sample implementation line-drawing routines provided in
 * {mfb,mi,cfb*} at run-time.
 */

#define XDECREASING	4
#define YDECREASING	2
#define YMAJOR		1

#define OCTANT1		(1 << (YDECREASING))
#define OCTANT2		(1 << (YDECREASING|YMAJOR))
#define OCTANT3		(1 << (XDECREASING|YDECREASING|YMAJOR))
#define OCTANT4		(1 << (XDECREASING|YDECREASING))
#define OCTANT5		(1 << (XDECREASING))
#define OCTANT6		(1 << (XDECREASING|YMAJOR))
#define OCTANT7		(1 << (YMAJOR))
#define OCTANT8		(1 << (0))

#define XMAJOROCTANTS		(OCTANT1 | OCTANT4 | OCTANT5 | OCTANT8)

#define DEFAULTZEROLINEBIAS	(OCTANT2 | OCTANT3 | OCTANT4 | OCTANT5)

/*
 * Devices can configure the rendering of routines in mi, mfb, and cfb*
 * by specifying a thin line bias to be applied to a particular screen
 * using the following function.  The bias parameter is an OR'ing of
 * the appropriate OCTANT constants defined above to indicate which
 * octants to bias a line to prefer an axial step when the Bresenham
 * error term is exactly zero.  The octants are mapped as follows:
 *
 *   \    |    /
 *    \ 3 | 2 /
 *     \  |  /
 *    4 \ | / 1
 *       \|/
 *   -----------
 *       /|\
 *    5 / | \ 8
 *     /  |  \
 *    / 6 | 7 \
 *   /    |    \
 *
 * For more information, see "Ambiguities in Incremental Line Rastering,"
 * Jack E. Bresenham, IEEE CG&A, May 1987.
 */

extern _X_EXPORT void miSetZeroLineBias(ScreenPtr /* pScreen */ ,
                                        unsigned int    /* bias */
    );

/*
 * Private definitions needed for drawing thin (zero width) lines
 * Used by the mi, mfb, and all cfb* components.
 */

#define X_AXIS	0
#define Y_AXIS	1

#define OUT_LEFT  0x08
#define OUT_RIGHT 0x04
#define OUT_ABOVE 0x02
#define OUT_BELOW 0x01

#define OUTCODES(_result, _x, _y, _pbox) \
    if	    ( (_x) <  (_pbox)->x1) (_result) |= OUT_LEFT; \
    else if ( (_x) >= (_pbox)->x2) (_result) |= OUT_RIGHT; \
    if	    ( (_y) <  (_pbox)->y1) (_result) |= OUT_ABOVE; \
    else if ( (_y) >= (_pbox)->y2) (_result) |= OUT_BELOW;

#define MIOUTCODES(outcode, x, y, xmin, ymin, xmax, ymax) \
{\
     if (x < xmin) outcode |= OUT_LEFT;\
     if (x > xmax) outcode |= OUT_RIGHT;\
     if (y < ymin) outcode |= OUT_ABOVE;\
     if (y > ymax) outcode |= OUT_BELOW;\
}

#define SWAPINT(i, j) \
{  int _t = i;  i = j;  j = _t; }

#define SWAPPT(i, j) \
{  DDXPointRec _t; _t = i;  i = j; j = _t; }

#define SWAPINT_PAIR(x1, y1, x2, y2)\
{   int t = x1;  x1 = x2;  x2 = t;\
        t = y1;  y1 = y2;  y2 = t;\
}

#define miGetZeroLineBias(_pScreen) ((unsigned long) (unsigned long*)\
    dixLookupPrivate(&(_pScreen)->devPrivates, miZeroLineScreenKey))

#define CalcLineDeltas(_x1,_y1,_x2,_y2,_adx,_ady,_sx,_sy,_SX,_SY,_octant) \
    (_octant) = 0;				\
    (_sx) = (_SX);				\
    if (((_adx) = (_x2) - (_x1)) < 0) {		\
	(_adx) = -(_adx);			\
	(_sx = -(_sx));				\
	(_octant) |= XDECREASING;		\
    }						\
    (_sy) = (_SY);				\
    if (((_ady) = (_y2) - (_y1)) < 0) {		\
	(_ady) = -(_ady);			\
	(_sy = -(_sy));				\
	(_octant) |= YDECREASING;		\
    }

#define SetYMajorOctant(_octant)	((_octant) |= YMAJOR)

#define FIXUP_ERROR(_e, _octant, _bias) \
    (_e) -= (((_bias) >> (_octant)) & 1)

#define IsXMajorOctant(_octant)		(!((_octant) & YMAJOR))
#define IsYMajorOctant(_octant)		((_octant) & YMAJOR)
#define IsXDecreasingOctant(_octant)	((_octant) & XDECREASING)
#define IsYDecreasingOctant(_octant)	((_octant) & YDECREASING)

extern _X_EXPORT DevPrivateKeyRec miZeroLineScreenKeyRec;

#define miZeroLineScreenKey (&miZeroLineScreenKeyRec)

extern _X_EXPORT int miZeroClipLine(int /*xmin */ ,
                                    int /*ymin */ ,
                                    int /*xmax */ ,
                                    int /*ymax */ ,
                                    int * /*new_x1 */ ,
                                    int * /*new_y1 */ ,
                                    int * /*new_x2 */ ,
                                    int * /*new_y2 */ ,
                                    unsigned int /*adx */ ,
                                    unsigned int /*ady */ ,
                                    int * /*pt1_clipped */ ,
                                    int * /*pt2_clipped */ ,
                                    int /*octant */ ,
                                    unsigned int /*bias */ ,
                                    int /*oc1 */ ,
                                    int /*oc2 */
    );

#endif                          /* MILINE_H */
