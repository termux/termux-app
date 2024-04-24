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
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>

#include "misc.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmap.h"
#include "mi.h"
#include "miline.h"

/*

The bresenham error equation used in the mi/mfb/cfb line routines is:

	e = error
	dx = difference in raw X coordinates
	dy = difference in raw Y coordinates
	M = # of steps in X direction
	N = # of steps in Y direction
	B = 0 to prefer diagonal steps in a given octant,
	    1 to prefer axial steps in a given octant

	For X major lines:
		e = 2Mdy - 2Ndx - dx - B
		-2dx <= e < 0

	For Y major lines:
		e = 2Ndx - 2Mdy - dy - B
		-2dy <= e < 0

At the start of the line, we have taken 0 X steps and 0 Y steps,
so M = 0 and N = 0:

	X major	e = 2Mdy - 2Ndx - dx - B
		  = -dx - B

	Y major	e = 2Ndx - 2Mdy - dy - B
		  = -dy - B

At the end of the line, we have taken dx X steps and dy Y steps,
so M = dx and N = dy:

	X major	e = 2Mdy - 2Ndx - dx - B
		  = 2dxdy - 2dydx - dx - B
		  = -dx - B
	Y major e = 2Ndx - 2Mdy - dy - B
		  = 2dydx - 2dxdy - dy - B
		  = -dy - B

Thus, the error term is the same at the start and end of the line.

Let us consider clipping an X coordinate.  There are 4 cases which
represent the two independent cases of clipping the start vs. the
end of the line and an X major vs. a Y major line.  In any of these
cases, we know the number of X steps (M) and we wish to find the
number of Y steps (N).  Thus, we will solve our error term equation.
If we are clipping the start of the line, we will find the smallest
N that satisfies our error term inequality.  If we are clipping the
end of the line, we will find the largest number of Y steps that
satisfies the inequality.  In that case, since we are representing
the Y steps as (dy - N), we will actually want to solve for the
smallest N in that equation.

Case 1:  X major, starting X coordinate moved by M steps

		-2dx <= 2Mdy - 2Ndx - dx - B < 0
	2Ndx <= 2Mdy - dx - B + 2dx	2Ndx > 2Mdy - dx - B
	2Ndx <= 2Mdy + dx - B		N > (2Mdy - dx - B) / 2dx
	N <= (2Mdy + dx - B) / 2dx

Since we are trying to find the smallest N that satisfies these
equations, we should use the > inequality to find the smallest:

	N = floor((2Mdy - dx - B) / 2dx) + 1
	  = floor((2Mdy - dx - B + 2dx) / 2dx)
	  = floor((2Mdy + dx - B) / 2dx)

Case 1b: X major, ending X coordinate moved to M steps

Same derivations as Case 1, but we want the largest N that satisfies
the equations, so we use the <= inequality:

	N = floor((2Mdy + dx - B) / 2dx)

Case 2: X major, ending X coordinate moved by M steps

		-2dx <= 2(dx - M)dy - 2(dy - N)dx - dx - B < 0
		-2dx <= 2dxdy - 2Mdy - 2dxdy + 2Ndx - dx - B < 0
		-2dx <= 2Ndx - 2Mdy - dx - B < 0
	2Ndx >= 2Mdy + dx + B - 2dx	2Ndx < 2Mdy + dx + B
	2Ndx >= 2Mdy - dx + B		N < (2Mdy + dx + B) / 2dx
	N >= (2Mdy - dx + B) / 2dx

Since we are trying to find the highest number of Y steps that
satisfies these equations, we need to find the smallest N, so
we should use the >= inequality to find the smallest:

	N = ceiling((2Mdy - dx + B) / 2dx)
	  = floor((2Mdy - dx + B + 2dx - 1) / 2dx)
	  = floor((2Mdy + dx + B - 1) / 2dx)

Case 2b: X major, starting X coordinate moved to M steps from end

Same derivations as Case 2, but we want the smallest number of Y
steps, so we want the highest N, so we use the < inequality:

	N = ceiling((2Mdy + dx + B) / 2dx) - 1
	  = floor((2Mdy + dx + B + 2dx - 1) / 2dx) - 1
	  = floor((2Mdy + dx + B + 2dx - 1 - 2dx) / 2dx)
	  = floor((2Mdy + dx + B - 1) / 2dx)

Case 3: Y major, starting X coordinate moved by M steps

		-2dy <= 2Ndx - 2Mdy - dy - B < 0
	2Ndx >= 2Mdy + dy + B - 2dy	2Ndx < 2Mdy + dy + B
	2Ndx >= 2Mdy - dy + B		N < (2Mdy + dy + B) / 2dx
	N >= (2Mdy - dy + B) / 2dx

Since we are trying to find the smallest N that satisfies these
equations, we should use the >= inequality to find the smallest:

	N = ceiling((2Mdy - dy + B) / 2dx)
	  = floor((2Mdy - dy + B + 2dx - 1) / 2dx)
	  = floor((2Mdy - dy + B - 1) / 2dx) + 1

Case 3b: Y major, ending X coordinate moved to M steps

Same derivations as Case 3, but we want the largest N that satisfies
the equations, so we use the < inequality:

	N = ceiling((2Mdy + dy + B) / 2dx) - 1
	  = floor((2Mdy + dy + B + 2dx - 1) / 2dx) - 1
	  = floor((2Mdy + dy + B + 2dx - 1 - 2dx) / 2dx)
	  = floor((2Mdy + dy + B - 1) / 2dx)

Case 4: Y major, ending X coordinate moved by M steps

		-2dy <= 2(dy - N)dx - 2(dx - M)dy - dy - B < 0
		-2dy <= 2dxdy - 2Ndx - 2dxdy + 2Mdy - dy - B < 0
		-2dy <= 2Mdy - 2Ndx - dy - B < 0
	2Ndx <= 2Mdy - dy - B + 2dy	2Ndx > 2Mdy - dy - B
	2Ndx <= 2Mdy + dy - B		N > (2Mdy - dy - B) / 2dx
	N <= (2Mdy + dy - B) / 2dx

Since we are trying to find the highest number of Y steps that
satisfies these equations, we need to find the smallest N, so
we should use the > inequality to find the smallest:

	N = floor((2Mdy - dy - B) / 2dx) + 1

Case 4b: Y major, starting X coordinate moved to M steps from end

Same analysis as Case 4, but we want the smallest number of Y steps
which means the largest N, so we use the <= inequality:

	N = floor((2Mdy + dy - B) / 2dx)

Now let's try the Y coordinates, we have the same 4 cases.

Case 5: X major, starting Y coordinate moved by N steps

		-2dx <= 2Mdy - 2Ndx - dx - B < 0
	2Mdy >= 2Ndx + dx + B - 2dx	2Mdy < 2Ndx + dx + B
	2Mdy >= 2Ndx - dx + B		M < (2Ndx + dx + B) / 2dy
	M >= (2Ndx - dx + B) / 2dy

Since we are trying to find the smallest M, we use the >= inequality:

	M = ceiling((2Ndx - dx + B) / 2dy)
	  = floor((2Ndx - dx + B + 2dy - 1) / 2dy)
	  = floor((2Ndx - dx + B - 1) / 2dy) + 1

Case 5b: X major, ending Y coordinate moved to N steps

Same derivations as Case 5, but we want the largest M that satisfies
the equations, so we use the < inequality:

	M = ceiling((2Ndx + dx + B) / 2dy) - 1
	  = floor((2Ndx + dx + B + 2dy - 1) / 2dy) - 1
	  = floor((2Ndx + dx + B + 2dy - 1 - 2dy) / 2dy)
	  = floor((2Ndx + dx + B - 1) / 2dy)

Case 6: X major, ending Y coordinate moved by N steps

		-2dx <= 2(dx - M)dy - 2(dy - N)dx - dx - B < 0
		-2dx <= 2dxdy - 2Mdy - 2dxdy + 2Ndx - dx - B < 0
		-2dx <= 2Ndx - 2Mdy - dx - B < 0
	2Mdy <= 2Ndx - dx - B + 2dx	2Mdy > 2Ndx - dx - B
	2Mdy <= 2Ndx + dx - B		M > (2Ndx - dx - B) / 2dy
	M <= (2Ndx + dx - B) / 2dy

Largest # of X steps means smallest M, so use the > inequality:

	M = floor((2Ndx - dx - B) / 2dy) + 1

Case 6b: X major, starting Y coordinate moved to N steps from end

Same derivations as Case 6, but we want the smallest # of X steps
which means the largest M, so use the <= inequality:

	M = floor((2Ndx + dx - B) / 2dy)

Case 7: Y major, starting Y coordinate moved by N steps

		-2dy <= 2Ndx - 2Mdy - dy - B < 0
	2Mdy <= 2Ndx - dy - B + 2dy	2Mdy > 2Ndx - dy - B
	2Mdy <= 2Ndx + dy - B		M > (2Ndx - dy - B) / 2dy
	M <= (2Ndx + dy - B) / 2dy

To find the smallest M, use the > inequality:

	M = floor((2Ndx - dy - B) / 2dy) + 1
	  = floor((2Ndx - dy - B + 2dy) / 2dy)
	  = floor((2Ndx + dy - B) / 2dy)

Case 7b: Y major, ending Y coordinate moved to N steps

Same derivations as Case 7, but we want the largest M that satisfies
the equations, so use the <= inequality:

	M = floor((2Ndx + dy - B) / 2dy)

Case 8: Y major, ending Y coordinate moved by N steps

		-2dy <= 2(dy - N)dx - 2(dx - M)dy - dy - B < 0
		-2dy <= 2dxdy - 2Ndx - 2dxdy + 2Mdy - dy - B < 0
		-2dy <= 2Mdy - 2Ndx - dy - B < 0
	2Mdy >= 2Ndx + dy + B - 2dy	2Mdy < 2Ndx + dy + B
	2Mdy >= 2Ndx - dy + B		M < (2Ndx + dy + B) / 2dy
	M >= (2Ndx - dy + B) / 2dy

To find the highest X steps, find the smallest M, use the >= inequality:

	M = ceiling((2Ndx - dy + B) / 2dy)
	  = floor((2Ndx - dy + B + 2dy - 1) / 2dy)
	  = floor((2Ndx + dy + B - 1) / 2dy)

Case 8b: Y major, starting Y coordinate moved to N steps from the end

Same derivations as Case 8, but we want to find the smallest # of X
steps which means the largest M, so we use the < inequality:

	M = ceiling((2Ndx + dy + B) / 2dy) - 1
	  = floor((2Ndx + dy + B + 2dy - 1) / 2dy) - 1
	  = floor((2Ndx + dy + B + 2dy - 1 - 2dy) / 2dy)
	  = floor((2Ndx + dy + B - 1) / 2dy)

So, our equations are:

	1:  X major move x1 to x1+M	floor((2Mdy + dx - B) / 2dx)
	1b: X major move x2 to x1+M	floor((2Mdy + dx - B) / 2dx)
	2:  X major move x2 to x2-M	floor((2Mdy + dx + B - 1) / 2dx)
	2b: X major move x1 to x2-M	floor((2Mdy + dx + B - 1) / 2dx)

	3:  Y major move x1 to x1+M	floor((2Mdy - dy + B - 1) / 2dx) + 1
	3b: Y major move x2 to x1+M	floor((2Mdy + dy + B - 1) / 2dx)
	4:  Y major move x2 to x2-M	floor((2Mdy - dy - B) / 2dx) + 1
	4b: Y major move x1 to x2-M	floor((2Mdy + dy - B) / 2dx)

	5:  X major move y1 to y1+N	floor((2Ndx - dx + B - 1) / 2dy) + 1
	5b: X major move y2 to y1+N	floor((2Ndx + dx + B - 1) / 2dy)
	6:  X major move y2 to y2-N	floor((2Ndx - dx - B) / 2dy) + 1
	6b: X major move y1 to y2-N	floor((2Ndx + dx - B) / 2dy)

	7:  Y major move y1 to y1+N	floor((2Ndx + dy - B) / 2dy)
	7b: Y major move y2 to y1+N	floor((2Ndx + dy - B) / 2dy)
	8:  Y major move y2 to y2-N	floor((2Ndx + dy + B - 1) / 2dy)
	8b: Y major move y1 to y2-N	floor((2Ndx + dy + B - 1) / 2dy)

We have the following constraints on all of the above terms:

	0 < M,N <= 2^15		 2^15 can be imposed by miZeroClipLine
	0 <= dx/dy <= 2^16 - 1
	0 <= B <= 1

The floor in all of the above equations can be accomplished with a
simple C divide operation provided that both numerator and denominator
are positive.

Since dx,dy >= 0 and since moving an X coordinate implies that dx != 0
and moving a Y coordinate implies dy != 0, we know that the denominators
are all > 0.

For all lines, (-B) and (B-1) are both either 0 or -1, depending on the
bias.  Thus, we have to show that the 2MNdxy +/- dxy terms are all >= 1
or > 0 to prove that the numerators are positive (or zero).

For X Major lines we know that dx > 0 and since 2Mdy is >= 0 due to the
constraints, the first four equations all have numerators >= 0.

For the second four equations, M > 0, so 2Mdy >= 2dy so (2Mdy - dy) >= dy
So (2Mdy - dy) > 0, since they are Y major lines.  Also, (2Mdy + dy) >= 3dy
or (2Mdy + dy) > 0.  So all of their numerators are >= 0.

For the third set of four equations, N > 0, so 2Ndx >= 2dx so (2Ndx - dx)
>= dx > 0.  Similarly (2Ndx + dx) >= 3dx > 0.  So all numerators >= 0.

For the fourth set of equations, dy > 0 and 2Ndx >= 0, so all numerators
are > 0.

To consider overflow, consider the case of 2 * M,N * dx,dy + dx,dy.  This
is bounded <= 2 * 2^15 * (2^16 - 1) + (2^16 - 1)
	   <= 2^16 * (2^16 - 1) + (2^16 - 1)
	   <= 2^32 - 2^16 + 2^16 - 1
	   <= 2^32 - 1
Since the (-B) and (B-1) terms are all 0 or -1, the maximum value of
the numerator is therefore (2^32 - 1), which does not overflow an unsigned
32 bit variable.

*/

/* Bit codes for the terms of the 16 clipping equations defined below. */

#define T_2NDX		(1 << 0)
#define T_2MDY		(0)     /* implicit term */
#define T_DXNOTY	(1 << 1)
#define T_DYNOTX	(0)     /* implicit term */
#define T_SUBDXORY	(1 << 2)
#define T_ADDDX		(T_DXNOTY)      /* composite term */
#define T_SUBDX		(T_DXNOTY | T_SUBDXORY) /* composite term */
#define T_ADDDY		(T_DYNOTX)      /* composite term */
#define T_SUBDY		(T_DYNOTX | T_SUBDXORY) /* composite term */
#define T_BIASSUBONE	(1 << 3)
#define T_SUBBIAS	(0)     /* implicit term */
#define T_DIV2DX	(1 << 4)
#define T_DIV2DY	(0)     /* implicit term */
#define T_ADDONE	(1 << 5)

/* Bit masks defining the 16 equations used in miZeroClipLine. */

#define EQN1	(T_2MDY | T_ADDDX | T_SUBBIAS    | T_DIV2DX)
#define EQN1B	(T_2MDY | T_ADDDX | T_SUBBIAS    | T_DIV2DX)
#define EQN2	(T_2MDY | T_ADDDX | T_BIASSUBONE | T_DIV2DX)
#define EQN2B	(T_2MDY | T_ADDDX | T_BIASSUBONE | T_DIV2DX)

#define EQN3	(T_2MDY | T_SUBDY | T_BIASSUBONE | T_DIV2DX | T_ADDONE)
#define EQN3B	(T_2MDY | T_ADDDY | T_BIASSUBONE | T_DIV2DX)
#define EQN4	(T_2MDY | T_SUBDY | T_SUBBIAS    | T_DIV2DX | T_ADDONE)
#define EQN4B	(T_2MDY | T_ADDDY | T_SUBBIAS    | T_DIV2DX)

#define EQN5	(T_2NDX | T_SUBDX | T_BIASSUBONE | T_DIV2DY | T_ADDONE)
#define EQN5B	(T_2NDX | T_ADDDX | T_BIASSUBONE | T_DIV2DY)
#define EQN6	(T_2NDX | T_SUBDX | T_SUBBIAS    | T_DIV2DY | T_ADDONE)
#define EQN6B	(T_2NDX | T_ADDDX | T_SUBBIAS    | T_DIV2DY)

#define EQN7	(T_2NDX | T_ADDDY | T_SUBBIAS    | T_DIV2DY)
#define EQN7B	(T_2NDX | T_ADDDY | T_SUBBIAS    | T_DIV2DY)
#define EQN8	(T_2NDX | T_ADDDY | T_BIASSUBONE | T_DIV2DY)
#define EQN8B	(T_2NDX | T_ADDDY | T_BIASSUBONE | T_DIV2DY)

/* miZeroClipLine
 *
 * returns:  1 for partially clipped line
 *          -1 for completely clipped line
 *
 */
int
miZeroClipLine(int xmin, int ymin, int xmax, int ymax,
               int *new_x1, int *new_y1, int *new_x2, int *new_y2,
               unsigned int adx, unsigned int ady,
               int *pt1_clipped, int *pt2_clipped,
               int octant, unsigned int bias, int oc1, int oc2)
{
    int swapped = 0;
    int clipDone = 0;
    CARD32 utmp = 0;
    int clip1, clip2;
    int x1, y1, x2, y2;
    int x1_orig, y1_orig, x2_orig, y2_orig;
    int xmajor;
    int negslope = 0, anchorval = 0;
    unsigned int eqn = 0;

    x1 = x1_orig = *new_x1;
    y1 = y1_orig = *new_y1;
    x2 = x2_orig = *new_x2;
    y2 = y2_orig = *new_y2;

    clip1 = 0;
    clip2 = 0;

    xmajor = IsXMajorOctant(octant);
    bias = ((bias >> octant) & 1);

    while (1) {
        if ((oc1 & oc2) != 0) { /* trivial reject */
            clipDone = -1;
            clip1 = oc1;
            clip2 = oc2;
            break;
        }
        else if ((oc1 | oc2) == 0) {    /* trivial accept */
            clipDone = 1;
            if (swapped) {
                SWAPINT_PAIR(x1, y1, x2, y2);
                SWAPINT(clip1, clip2);
            }
            break;
        }
        else {                  /* have to clip */

            /* only clip one point at a time */
            if (oc1 == 0) {
                SWAPINT_PAIR(x1, y1, x2, y2);
                SWAPINT_PAIR(x1_orig, y1_orig, x2_orig, y2_orig);
                SWAPINT(oc1, oc2);
                SWAPINT(clip1, clip2);
                swapped = !swapped;
            }

            clip1 |= oc1;
            if (oc1 & OUT_LEFT) {
                negslope = IsYDecreasingOctant(octant);
                utmp = xmin - x1_orig;
                if (utmp <= 32767) {    /* clip based on near endpt */
                    if (xmajor)
                        eqn = (swapped) ? EQN2 : EQN1;
                    else
                        eqn = (swapped) ? EQN4 : EQN3;
                    anchorval = y1_orig;
                }
                else {          /* clip based on far endpt */

                    utmp = x2_orig - xmin;
                    if (xmajor)
                        eqn = (swapped) ? EQN1B : EQN2B;
                    else
                        eqn = (swapped) ? EQN3B : EQN4B;
                    anchorval = y2_orig;
                    negslope = !negslope;
                }
                x1 = xmin;
            }
            else if (oc1 & OUT_ABOVE) {
                negslope = IsXDecreasingOctant(octant);
                utmp = ymin - y1_orig;
                if (utmp <= 32767) {    /* clip based on near endpt */
                    if (xmajor)
                        eqn = (swapped) ? EQN6 : EQN5;
                    else
                        eqn = (swapped) ? EQN8 : EQN7;
                    anchorval = x1_orig;
                }
                else {          /* clip based on far endpt */

                    utmp = y2_orig - ymin;
                    if (xmajor)
                        eqn = (swapped) ? EQN5B : EQN6B;
                    else
                        eqn = (swapped) ? EQN7B : EQN8B;
                    anchorval = x2_orig;
                    negslope = !negslope;
                }
                y1 = ymin;
            }
            else if (oc1 & OUT_RIGHT) {
                negslope = IsYDecreasingOctant(octant);
                utmp = x1_orig - xmax;
                if (utmp <= 32767) {    /* clip based on near endpt */
                    if (xmajor)
                        eqn = (swapped) ? EQN2 : EQN1;
                    else
                        eqn = (swapped) ? EQN4 : EQN3;
                    anchorval = y1_orig;
                }
                else {          /* clip based on far endpt */

                    /*
                     * Technically since the equations can handle
                     * utmp == 32768, this overflow code isn't
                     * needed since X11 protocol can't generate
                     * a line which goes more than 32768 pixels
                     * to the right of a clip rectangle.
                     */
                    utmp = xmax - x2_orig;
                    if (xmajor)
                        eqn = (swapped) ? EQN1B : EQN2B;
                    else
                        eqn = (swapped) ? EQN3B : EQN4B;
                    anchorval = y2_orig;
                    negslope = !negslope;
                }
                x1 = xmax;
            }
            else if (oc1 & OUT_BELOW) {
                negslope = IsXDecreasingOctant(octant);
                utmp = y1_orig - ymax;
                if (utmp <= 32767) {    /* clip based on near endpt */
                    if (xmajor)
                        eqn = (swapped) ? EQN6 : EQN5;
                    else
                        eqn = (swapped) ? EQN8 : EQN7;
                    anchorval = x1_orig;
                }
                else {          /* clip based on far endpt */

                    /*
                     * Technically since the equations can handle
                     * utmp == 32768, this overflow code isn't
                     * needed since X11 protocol can't generate
                     * a line which goes more than 32768 pixels
                     * below the bottom of a clip rectangle.
                     */
                    utmp = ymax - y2_orig;
                    if (xmajor)
                        eqn = (swapped) ? EQN5B : EQN6B;
                    else
                        eqn = (swapped) ? EQN7B : EQN8B;
                    anchorval = x2_orig;
                    negslope = !negslope;
                }
                y1 = ymax;
            }

            if (swapped)
                negslope = !negslope;

            utmp <<= 1;         /* utmp = 2N or 2M */
            if (eqn & T_2NDX)
                utmp = (utmp * adx);
            else                /* (eqn & T_2MDY) */
                utmp = (utmp * ady);
            if (eqn & T_DXNOTY)
                if (eqn & T_SUBDXORY)
                    utmp -= adx;
                else
                    utmp += adx;
            else /* (eqn & T_DYNOTX) */ if (eqn & T_SUBDXORY)
                utmp -= ady;
            else
                utmp += ady;
            if (eqn & T_BIASSUBONE)
                utmp += bias - 1;
            else                /* (eqn & T_SUBBIAS) */
                utmp -= bias;
            if (eqn & T_DIV2DX)
                utmp /= (adx << 1);
            else                /* (eqn & T_DIV2DY) */
                utmp /= (ady << 1);
            if (eqn & T_ADDONE)
                utmp++;

            if (negslope)
                utmp = -utmp;

            if (eqn & T_2NDX)   /* We are calculating X steps */
                x1 = anchorval + utmp;
            else                /* else, Y steps */
                y1 = anchorval + utmp;

            oc1 = 0;
            MIOUTCODES(oc1, x1, y1, xmin, ymin, xmax, ymax);
        }
    }

    *new_x1 = x1;
    *new_y1 = y1;
    *new_x2 = x2;
    *new_y2 = y2;

    *pt1_clipped = clip1;
    *pt2_clipped = clip2;

    return clipDone;
}
