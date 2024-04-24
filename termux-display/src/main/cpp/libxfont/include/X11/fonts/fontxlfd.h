/*

Copyright 1990, 1994, 1998  The Open Group

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

/*
 * Author:  Keith Packard, MIT X Consortium
 */

#ifndef _FONTXLFD_H_
#define _FONTXLFD_H_

#include <X11/fonts/FSproto.h>

/* Constants for values_supplied bitmap */

#define SIZE_SPECIFY_MASK		0xf

#define PIXELSIZE_MASK			0x3
#define PIXELSIZE_UNDEFINED		0
#define PIXELSIZE_SCALAR		0x1
#define PIXELSIZE_ARRAY			0x2
#define PIXELSIZE_SCALAR_NORMALIZED	0x3	/* Adjusted for resolution */

#define POINTSIZE_MASK			0xc
#define POINTSIZE_UNDEFINED		0
#define POINTSIZE_SCALAR		0x4
#define POINTSIZE_ARRAY			0x8

#define PIXELSIZE_WILDCARD		0x10
#define POINTSIZE_WILDCARD		0x20

#define ENHANCEMENT_SPECIFY_MASK	0x40

#define CHARSUBSET_SPECIFIED		0x40

#define EPS		1.0e-20
#define XLFD_NDIGITS	3		/* Round numbers in pixel and
					   point arrays to this many
					   digits for repeatability */

typedef struct _FontScalable {
    int		values_supplied;	/* Bitmap identifying what advanced
					   capabilities or enhancements
					   were specified in the font name */
    double	pixel_matrix[4];
    double	point_matrix[4];

    /* Pixel and point fields are deprecated in favor of the
       transformation matrices.  They are provided and filled in for the
       benefit of rasterizers that do not handle the matrices.  */

    int		pixel,
		point;

    int         x,
                y,
                width;
    char	*xlfdName;
    int		nranges;
    fsRange	*ranges;
}           FontScalableRec, *FontScalablePtr;


extern double xlfd_round_double ( double x );
extern Bool FontParseXLFDName ( char *fname, FontScalablePtr vals, int subst );
extern fsRange *FontParseRanges ( char *name, int *nranges );

#define FONT_XLFD_REPLACE_NONE	0
#define FONT_XLFD_REPLACE_STAR	1
#define FONT_XLFD_REPLACE_ZERO	2
#define FONT_XLFD_REPLACE_VALUE	3

#endif				/* _FONTXLFD_H_ */
