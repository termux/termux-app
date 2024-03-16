/*

Copyright 1991, 1998  The Open Group

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
 * Author:  Keith Packard, MIT X Consortium
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include "src/util/replace.h"
#include    <X11/fonts/fntfilst.h>
#include <math.h>

Bool
FontFileAddScaledInstance (FontEntryPtr entry, FontScalablePtr vals,
			   FontPtr pFont, char *bitmapName)
{
    FontScalableEntryPtr    scalable;
    FontScalableExtraPtr    extra;
    FontScaledPtr	    new;
    int			    newsize;

    scalable = &entry->u.scalable;
    extra = scalable->extra;
    if (extra->numScaled == extra->sizeScaled)
    {
	newsize = extra->sizeScaled + 4;
	new = reallocarray (extra->scaled, newsize, sizeof (FontScaledRec));
	if (!new)
	    return FALSE;
	extra->sizeScaled = newsize;
	extra->scaled = new;
    }
    new = &extra->scaled[extra->numScaled++];
    new->vals = *vals;
    new->pFont = pFont;
    new->bitmap = (FontEntryPtr) bitmapName;
    if (pFont)
	pFont->fpePrivate = (pointer) entry;
    return TRUE;
}

/* Must call this after the directory is sorted */

void
FontFileSwitchStringsToBitmapPointers (FontDirectoryPtr dir)
{
    int	    s;
    int	    b;
    int	    i;
    FontEntryPtr	    scalable;
    FontEntryPtr	    nonScalable;
    FontScaledPtr	    scaled;
    FontScalableExtraPtr    extra;

    scalable = dir->scalable.entries;
    nonScalable = dir->nonScalable.entries;
    for (s = 0; s < dir->scalable.used; s++)
    {
	extra = scalable[s].u.scalable.extra;
	scaled = extra->scaled;
	for (i = 0; i < extra->numScaled; i++)
	    for (b = 0; b < dir->nonScalable.used; b++)
		if (nonScalable[b].name.name == (char *) scaled[i].bitmap)
		    scaled[i].bitmap = &nonScalable[b];
    }
}

void
FontFileRemoveScaledInstance (FontEntryPtr entry, FontPtr pFont)
{
    FontScalableEntryPtr    scalable;
    FontScalableExtraPtr    extra;
    int			    i;

    scalable = &entry->u.scalable;
    extra = scalable->extra;
    for (i = 0; i < extra->numScaled; i++)
    {
	if (extra->scaled[i].pFont == pFont)
	{
	    if (extra->scaled[i].vals.ranges)
		free (extra->scaled[i].vals.ranges);
	    extra->numScaled--;
	    for (; i < extra->numScaled; i++)
		extra->scaled[i] = extra->scaled[i+1];
	}
    }
}

Bool
FontFileCompleteXLFD (FontScalablePtr vals, FontScalablePtr def)
{
    FontResolutionPtr res;
    int		num_res;
    double	sx, sy, temp_matrix[4];
    double	pixel_setsize_adjustment = 1.0;
    /*
     * If two of the three vertical scale values are specified, compute the
     * third.  If all three are specified, make sure they are consistent
     * (within a pixel)
     *
     * One purpose of this procedure is to complete XLFD names in a
     * repeatable manner.  That is, if the user partially specifies
     * a name (say, pixelsize but not pointsize), the results generated
     * here result in a fully specified name that will result in the
     * same font.
     */

    res = GetClientResolutions(&num_res);

    if (!(vals->values_supplied & PIXELSIZE_MASK) ||
	!(vals->values_supplied & POINTSIZE_MASK))
    {
	/* If resolution(s) unspecified and cannot be computed from
	   pixelsize and pointsize, get appropriate defaults. */

	if (num_res)
	{
	    if (vals->x <= 0)
		vals->x = res->x_resolution;
	    if (vals->y <= 0)
		vals->y = res->y_resolution;
	}

	if (vals->x <= 0)
	    vals->x = def->x;
	if (vals->y <= 0)
	    vals->y = def->y;
    }
    else
    {
	/* If needed, compute resolution values from the pixel and
	   pointsize information we were given.  This problem is
	   overdetermined (four equations, two unknowns), but we don't
	   check for inconsistencies here.  If they exist, they will
	   show up in later tests for the point and pixel sizes.  */

	if (vals->y <= 0)
	{
	    double x = hypot(vals->pixel_matrix[1], vals->pixel_matrix[3]);
	    double y = hypot(vals->point_matrix[1], vals->point_matrix[3]);
	    if (y < EPS) return FALSE;
	    vals->y = (int)(x * 72.27 / y + .5);
	}
	if (vals->x <= 0)
	{
	    /* If the pixelsize was given as an array, or as a scalar that
	       has been normalized for the pixel shape, we have enough
	       information to compute a separate horizontal resolution */

	    if ((vals->values_supplied & PIXELSIZE_MASK) == PIXELSIZE_ARRAY ||
	        (vals->values_supplied & PIXELSIZE_MASK) ==
		    PIXELSIZE_SCALAR_NORMALIZED)
	    {
		double x = hypot(vals->pixel_matrix[0], vals->pixel_matrix[2]);
		double y = hypot(vals->point_matrix[0], vals->point_matrix[2]);
		if (y < EPS) return FALSE;
		vals->x = (int)(x * 72.27 / y + .5);
	    }
	    else
	    {
		/* Not enough information in the pixelsize array.  Just
		   assume the pixels are square. */
		vals->x = vals->y;
	    }
	}
    }

    if (vals->x <= 0 || vals->y <= 0) return FALSE;

    /* If neither pixelsize nor pointsize is defined, take the pointsize
       from the defaults structure we've been passed. */
    if (!(vals->values_supplied & PIXELSIZE_MASK) &&
	!(vals->values_supplied & POINTSIZE_MASK))
    {
	if (num_res)
	{
	    vals->point_matrix[0] =
	    vals->point_matrix[3] = (double)res->point_size / 10.0;
	    vals->point_matrix[1] =
	    vals->point_matrix[2] = 0;
	    vals->values_supplied = (vals->values_supplied & ~POINTSIZE_MASK) |
				    POINTSIZE_SCALAR;
	}
	else if (def->values_supplied & POINTSIZE_MASK)
	{
	    vals->point_matrix[0] = def->point_matrix[0];
	    vals->point_matrix[1] = def->point_matrix[1];
	    vals->point_matrix[2] = def->point_matrix[2];
	    vals->point_matrix[3] = def->point_matrix[3];
	    vals->values_supplied = (vals->values_supplied & ~POINTSIZE_MASK) |
				    (def->values_supplied & POINTSIZE_MASK);
	}
	else return FALSE;
    }

    /* At this point, at least two of the three vertical scale values
       should be specified.  Our job now is to compute the missing ones
       and check for agreement between overspecified values */

    /* If pixelsize was specified by a scalar, we need to fix the matrix
       now that we know the resolutions.  */
    if ((vals->values_supplied & PIXELSIZE_MASK) == PIXELSIZE_SCALAR)
    {
	/* pixel_setsize_adjustment used below to modify permissible
	   error in pixel/pointsize matching, since multiplying a
	   number rounded to integer changes the amount of the error
	   caused by the rounding */

	pixel_setsize_adjustment = (double)vals->x / (double)vals->y;
	vals->pixel_matrix[0] *= pixel_setsize_adjustment;
	vals->values_supplied  = (vals->values_supplied & ~PIXELSIZE_MASK) |
				 PIXELSIZE_SCALAR_NORMALIZED;
    }

    sx = (double)vals->x / 72.27;
    sy = (double)vals->y / 72.27;

    /* If a pointsize was specified, make sure pixelsize is consistent
       to within 1 pixel, then replace pixelsize with a consistent
       floating-point value.  */

    if (vals->values_supplied & POINTSIZE_MASK)
    {
    recompute_pixelsize: ;
	temp_matrix[0] = vals->point_matrix[0] * sx;
	temp_matrix[1] = vals->point_matrix[1] * sy;
	temp_matrix[2] = vals->point_matrix[2] * sx;
	temp_matrix[3] = vals->point_matrix[3] * sy;
	if (vals->values_supplied & PIXELSIZE_MASK)
	{
	    if (fabs(vals->pixel_matrix[0] - temp_matrix[0]) >
		    pixel_setsize_adjustment ||
		fabs(vals->pixel_matrix[1] - temp_matrix[1]) > 1 ||
		fabs(vals->pixel_matrix[2] - temp_matrix[2]) > 1 ||
		fabs(vals->pixel_matrix[3] - temp_matrix[3]) > 1)
		return FALSE;
	}
	if ((vals->values_supplied & PIXELSIZE_MASK) == PIXELSIZE_ARRAY &&
	    (vals->values_supplied & POINTSIZE_MASK) == POINTSIZE_SCALAR)
	{
	    /* In the special case that pixelsize came as an array and
	       pointsize as a scalar, recompute the pointsize matrix
	       from the pixelsize matrix. */
	    goto recompute_pointsize;
	}

	/* Refresh pixel matrix with precise values computed from
	   pointsize and resolution.  */
	vals->pixel_matrix[0] = temp_matrix[0];
	vals->pixel_matrix[1] = temp_matrix[1];
	vals->pixel_matrix[2] = temp_matrix[2];
	vals->pixel_matrix[3] = temp_matrix[3];

	/* Set values_supplied for pixel to match that for point */
	vals->values_supplied =
	    (vals->values_supplied & ~PIXELSIZE_MASK) |
	    (((vals->values_supplied & POINTSIZE_MASK) == POINTSIZE_ARRAY) ?
		PIXELSIZE_ARRAY : PIXELSIZE_SCALAR_NORMALIZED);
    }
    else
    {
	/* Pointsize unspecified...  compute from pixel size and
	   resolutions */
    recompute_pointsize: ;
	if (fabs(sx) < EPS || fabs(sy) < EPS) return FALSE;
	vals->point_matrix[0] = vals->pixel_matrix[0] / sx;
	vals->point_matrix[1] = vals->pixel_matrix[1] / sy;
	vals->point_matrix[2] = vals->pixel_matrix[2] / sx;
	vals->point_matrix[3] = vals->pixel_matrix[3] / sy;

	/* Set values_supplied for pixel to match that for point */
	vals->values_supplied =
	    (vals->values_supplied & ~POINTSIZE_MASK) |
	    (((vals->values_supplied & PIXELSIZE_MASK) == PIXELSIZE_ARRAY) ?
		POINTSIZE_ARRAY : POINTSIZE_SCALAR);

	/* If we computed scalar pointsize from scalar pixelsize, round
	   pointsize to decipoints and recompute pixelsize so we end up
	   with a repeatable name */
	if ((vals->values_supplied & POINTSIZE_MASK) == POINTSIZE_SCALAR)
	{
	    /* Off-diagonal elements should be zero since no matrix was
	       specified. */
	    vals->point_matrix[0] =
		(double)(int)(vals->point_matrix[0] * 10.0 + .5) / 10.0;
	    vals->point_matrix[3] =
		(double)(int)(vals->point_matrix[3] * 10.0 + .5) / 10.0;
	    goto recompute_pixelsize;
	}
    }

    /* We've succeeded.  Round everything to a few decimal places
       for repeatability. */

    vals->pixel_matrix[0] = xlfd_round_double(vals->pixel_matrix[0]);
    vals->pixel_matrix[1] = xlfd_round_double(vals->pixel_matrix[1]);
    vals->pixel_matrix[2] = xlfd_round_double(vals->pixel_matrix[2]);
    vals->pixel_matrix[3] = xlfd_round_double(vals->pixel_matrix[3]);
    vals->point_matrix[0] = xlfd_round_double(vals->point_matrix[0]);
    vals->point_matrix[1] = xlfd_round_double(vals->point_matrix[1]);
    vals->point_matrix[2] = xlfd_round_double(vals->point_matrix[2]);
    vals->point_matrix[3] = xlfd_round_double(vals->point_matrix[3]);

    /* Fill in the deprecated fields for the benefit of rasterizers
       that do not handle the matrices. */
    vals->point = vals->point_matrix[3] * 10;
    vals->pixel = vals->pixel_matrix[3];

    return TRUE;
}

static Bool
MatchScalable (FontScalablePtr a, FontScalablePtr b)
{
    int i;

    /* Some asymmetry here:  we assume that the first argument (a) is
       the table entry and the second (b) the item we're trying to match
       (the key).  We'll consider the fonts matched if the relevant
       metrics match *and* if a) the table entry doesn't have charset
       subsetting or b) the table entry has identical charset subsetting
       to that in the key.  We could add logic to check if the table
       entry has a superset of the charset required by the key, but
       we'll resist the urge for now.  */

#define EQUAL(a,b) ((a)[0] == (b)[0] && \
                    (a)[1] == (b)[1] && \
                    (a)[2] == (b)[2] && \
                    (a)[3] == (b)[3])

    if (!(a->x == b->x &&
	  a->y == b->y &&
	  (a->width == b->width || a->width == 0 || b->width == 0 || b->width == -1) &&
	  (!(b->values_supplied & PIXELSIZE_MASK) ||
	    ((a->values_supplied & PIXELSIZE_MASK) ==
	     (b->values_supplied & PIXELSIZE_MASK) &&
	    EQUAL(a->pixel_matrix, b->pixel_matrix))) &&
	  (!(b->values_supplied & POINTSIZE_MASK) ||
	    ((a->values_supplied & POINTSIZE_MASK) ==
	     (b->values_supplied & POINTSIZE_MASK) &&
	    EQUAL(a->point_matrix, b->point_matrix))) &&
	  (a->nranges == 0 || a->nranges == b->nranges)))
      return FALSE;

    for (i = 0; i < a->nranges; i++)
	if (a->ranges[i].min_char_low != b->ranges[i].min_char_low ||
	    a->ranges[i].min_char_high != b->ranges[i].min_char_high ||
	    a->ranges[i].max_char_low != b->ranges[i].max_char_low ||
	    a->ranges[i].max_char_high != b->ranges[i].max_char_high)
		return FALSE;

    return TRUE;
}

FontScaledPtr
FontFileFindScaledInstance (FontEntryPtr entry, FontScalablePtr vals,
			    int noSpecificSize)
{
    FontScalableEntryPtr    scalable;
    FontScalableExtraPtr    extra;
    FontScalablePtr	    mvals;
    int			    dist, i;
    int			    mini;
    double		    mindist;
    register double	    temp, sum=0.0;

#define NORMDIFF(a, b) ( \
    temp = (a)[0] - (b)[0], \
    sum = temp * temp, \
    temp = (a)[1] - (b)[1], \
    sum += temp * temp, \
    temp = (a)[2] - (b)[2], \
    sum += temp * temp, \
    temp = (a)[3] - (b)[3], \
    sum + temp * temp )

    scalable = &entry->u.scalable;
    extra = scalable->extra;
    if (noSpecificSize && extra->numScaled)
    {
	mini = 0;
	mindist = NORMDIFF(extra->scaled[0].vals.point_matrix,
			   vals->point_matrix);
	for (i = 1; i < extra->numScaled; i++)
	{
	    if (extra->scaled[i].pFont &&
		!extra->scaled[i].pFont->info.cachable) continue;
	    mvals = &extra->scaled[i].vals;
	    dist = NORMDIFF(mvals->point_matrix, vals->point_matrix);
	    if (dist < mindist)
	    {
		mindist = dist;
		mini = i;
	    }
	}
	if (extra->scaled[mini].pFont &&
	    !extra->scaled[mini].pFont->info.cachable) return 0;
	return &extra->scaled[mini];
    }
    else
    {
    	/* See if we've scaled to this value yet */
    	for (i = 0; i < extra->numScaled; i++)
    	{
	    if (extra->scaled[i].pFont &&
		!extra->scaled[i].pFont->info.cachable) continue;
	    if (MatchScalable (&extra->scaled[i].vals, vals))
	    	return &extra->scaled[i];
    	}
    }
    return 0;
}
