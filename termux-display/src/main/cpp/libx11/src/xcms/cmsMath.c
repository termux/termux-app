
/*

Copyright 1990, 1998  The Open Group

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

/*
 * Stephen Gildea, MIT X Consortium, January 1991
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"

#ifdef DEBUG
#include <stdio.h>
#endif

#include <float.h>
#ifndef DBL_EPSILON
#define DBL_EPSILON 1e-6
#endif

#ifdef _X_ROOT_STATS
int cbrt_loopcount;
int sqrt_loopcount;
#endif

/* Newton's Method:  x_n+1 = x_n - ( f(x_n) / f'(x_n) ) */


/* for cube roots, x^3 - a = 0,  x_new = x - 1/3 (x - a/x^2) */

double
_XcmsCubeRoot(double a)
{
    register double abs_a, cur_guess, delta;

#ifdef DEBUG
    printf("_XcmsCubeRoot passed in %g\n", a);
#endif
#ifdef _X_ROOT_STATS
    cbrt_loopcount = 0;
#endif
    if (a == 0.)
	return 0.;

    abs_a = a<0. ? -a : a;	/* convert to positive to speed loop tests */

    /* arbitrary first guess */
    if (abs_a > 1.)
	cur_guess = abs_a/8.;
    else
	cur_guess = abs_a*8.;

    do {
#ifdef _X_ROOT_STATS
	cbrt_loopcount++;
#endif
	delta = (cur_guess - abs_a/(cur_guess*cur_guess))/3.;
	cur_guess -= delta;
	if (delta < 0.) delta = -delta;
    } while (delta >= cur_guess*DBL_EPSILON);

    if (a < 0.)
	cur_guess = -cur_guess;

#ifdef DEBUG
    printf("_XcmsCubeRoot returning %g\n", cur_guess);
#endif
    return cur_guess;
}



/* for square roots, x^2 - a = 0,  x_new = x - 1/2 (x - a/x) */

double
_XcmsSquareRoot(double a)
{
    register double cur_guess, delta;

#ifdef DEBUG
    printf("_XcmsSquareRoot passed in %g\n", a);
#endif
#ifdef _X_ROOT_STATS
    sqrt_loopcount = 0;
#endif
    if (a == 0.)
	return 0.;

    if (a < 0.) {
	/* errno = EDOM; */
	return 0.;
    }

    /* arbitrary first guess */
    if (a > 1.)
	cur_guess = a/4.;
    else
	cur_guess = a*4.;

    do {
#ifdef _X_ROOT_STATS
	sqrt_loopcount++;
#endif
	delta = (cur_guess - a/cur_guess)/2.;
	cur_guess -= delta;
	if (delta < 0.) delta = -delta;
    } while (delta >= cur_guess*DBL_EPSILON);

#ifdef DEBUG
    printf("_XcmsSquareRoot returning %g\n", cur_guess);
#endif
    return cur_guess;
}

