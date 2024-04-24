
/*
 * Code and supporting documentation (c) Copyright 1990 1991 Tektronix, Inc.
 * 	All Rights Reserved
 *
 * This file is a component of an X Window System-specific implementation
 * of Xcms based on the TekColor Color Management System.  Permission is
 * hereby granted to use, copy, modify, sell, and otherwise distribute this
 * software and its documentation for any purpose and without fee, provided
 * that this copyright, permission, and disclaimer notice is reproduced in
 * all copies of this software and in supporting documentation.  TekColor
 * is a trademark of Tektronix, Inc.
 *
 * Tektronix makes no representation about the suitability of this software
 * for any purpose.  It is provided "as is" and with all faults.
 *
 * TEKTRONIX DISCLAIMS ALL WARRANTIES APPLICABLE TO THIS SOFTWARE,
 * INCLUDING THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  IN NO EVENT SHALL TEKTRONIX BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA, OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR THE PERFORMANCE OF THIS SOFTWARE.
 *
 *
 *	NAME
 *		XcmsRtoX.c
 *
 *	DESCRIPTION
 *		Convert color specifications in XcmsRGB format in one array of
 *		XcmsColor structures to RGB in an array of XColor structures.
 *
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "Xlibint.h"
#include "Xcmsint.h"
#include "Cv.h"

/*
 *      LOCAL VARIABLES
 */

static unsigned short const MASK[17] = {
    0x0000,	/*  0 bitsPerRGB */
    0x8000,	/*  1 bitsPerRGB */
    0xc000,	/*  2 bitsPerRGB */
    0xe000,	/*  3 bitsPerRGB */
    0xf000,	/*  4 bitsPerRGB */
    0xf800,	/*  5 bitsPerRGB */
    0xfc00,	/*  6 bitsPerRGB */
    0xfe00,	/*  7 bitsPerRGB */
    0xff00,	/*  8 bitsPerRGB */
    0xff80,	/*  9 bitsPerRGB */
    0xffc0,	/* 10 bitsPerRGB */
    0xffe0,	/* 11 bitsPerRGB */
    0xfff0,	/* 12 bitsPerRGB */
    0xfff8,	/* 13 bitsPerRGB */
    0xfffc,	/* 14 bitsPerRGB */
    0xfffe,	/* 15 bitsPerRGB */
    0xffff	/* 16 bitsPerRGB */
};



/************************************************************************
 *									*
 *			API PRIVATE ROUTINES				*
 *									*
 ************************************************************************/

/*
 *	NAME
 *		_XcmsRGB_to_XColor -
 *
 *	SYNOPSIS
 */
void
_XcmsRGB_to_XColor(
    XcmsColor *pColors,
    XColor *pXColors,
    unsigned int nColors)
/*
 *	DESCRIPTION
 *	    Translates a color specification in XcmsRGBFormat in a XcmsColor
 * 	    structure to an XColor structure.
 *
 *	RETURNS
 *		void.
 */
{
    for (; nColors--; pXColors++, pColors++) {
	pXColors->pixel = pColors->pixel;
	pXColors->red = pColors->spec.RGB.red;
	pXColors->green = pColors->spec.RGB.green;
	pXColors->blue  = pColors->spec.RGB.blue;
	pXColors->flags = (DoRed | DoGreen | DoBlue);
    }
}


/*
 *	NAME
 *		_XColor_to_XcmsRGB
 *
 *	SYNOPSIS
 */
void
_XColor_to_XcmsRGB(
    XcmsCCC ccc,
    XColor *pXColors,
    XcmsColor *pColors,
    unsigned int nColors)
/*
 *	DESCRIPTION
 *		Translates an RGB color specification in an XColor
 *		structure to an XcmsRGB structure.
 *
 *		IMPORTANT NOTE:  Bit replication that may have been caused
 *		with ResolveColor() routine in the X Server is undone
 *		here if requested!  For example, if red = 0xcaca and the
 *		bits_per_rgb is 8, then spec.RGB.red will be 0xca00.
 *
 *	RETURNS
 *		void
 */
{
    int bits_per_rgb = ccc->visual->bits_per_rgb;

    for (; nColors--; pXColors++, pColors++) {
	pColors->spec.RGB.red = (pXColors->red & MASK[bits_per_rgb]);
	pColors->spec.RGB.green = (pXColors->green & MASK[bits_per_rgb]);
	pColors->spec.RGB.blue = (pXColors->blue & MASK[bits_per_rgb]);
	pColors->format = XcmsRGBFormat;
	pColors->pixel = pXColors->pixel;
    }
}


/*
 *	NAME
 *		_XcmsResolveColor
 *
 *	SYNOPSIS
 */
void
_XcmsResolveColor(
    XcmsCCC ccc,
    XcmsColor *pXcmsColor)
/*
 *	DESCRIPTION
 *	    Uses the X Server ResolveColor() algorithm to
 *	    modify values to closest values supported by hardware.
 *	    Old algorithm simply masked low-order bits.  The new algorithm
 *	    has the effect of replicating significant bits into lower order
 *	    bits in order to stretch the hardware value into all 16 bits.
 *
 *	    On a display with N-bit DACs, the "hardware" color is computed as:
 *
 *	    ((unsignedlong)(ClientValue >> (16-N)) * 0xFFFF) / ((1 << N) - 1)
 *
 *
 *	RETURNS
 *		void.
 */
{
    int shift;
    int max_color;

    shift = 16 - ccc->visual->bits_per_rgb;
    max_color = (1 << ccc->visual->bits_per_rgb) - 1;


    pXcmsColor->spec.RGB.red =
	    ((unsigned long)(pXcmsColor->spec.RGB.red >> shift) * 0xFFFF)
	    / max_color;
    pXcmsColor->spec.RGB.green =
	    ((unsigned long)(pXcmsColor->spec.RGB.green >> shift) * 0xFFFF)
	    / max_color;
    pXcmsColor->spec.RGB.blue =
	    ((unsigned long)(pXcmsColor->spec.RGB.blue  >> shift) * 0xFFFF)
	    / max_color;
}


/*
 *	NAME
 *		_XcmsUnresolveColor
 *
 *	SYNOPSIS
 */
void
_XcmsUnresolveColor(
    XcmsCCC ccc,
    XcmsColor *pColor)
/*
 *	DESCRIPTION
 *		Masks out insignificant bits.
 *
 *	RETURNS
 *		void.
 *
 *	ASSUMPTIONS
 *		format == XcmsRGBFormat
 */
{
    int bits_per_rgb = ccc->visual->bits_per_rgb;

    pColor->spec.RGB.red &= MASK[bits_per_rgb];
    pColor->spec.RGB.green &= MASK[bits_per_rgb];
    pColor->spec.RGB.blue &= MASK[bits_per_rgb];
}


/*
 *	NAME
 *		_XUnresolveColor
 *
 *	SYNOPSIS
 */
void
_XUnresolveColor(
    XcmsCCC ccc,
    XColor *pXColor)
/*
 *	DESCRIPTION
 *		Masks out insignificant bits.
 *
 *	RETURNS
 *		void.
 */
{
    int bits_per_rgb = ccc->visual->bits_per_rgb;

    pXColor->red &= MASK[bits_per_rgb];
    pXColor->green &= MASK[bits_per_rgb];
    pXColor->blue &= MASK[bits_per_rgb];
}

