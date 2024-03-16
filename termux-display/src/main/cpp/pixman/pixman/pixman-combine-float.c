/* -*- Mode: c; c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t; -*- */
/*
 * Copyright © 2010, 2012 Soren Sandmann Pedersen
 * Copyright © 2010, 2012 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Soren Sandmann Pedersen (sandmann@cs.au.dk)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <string.h>
#include <float.h>

#include "pixman-private.h"

/* Workaround for http://gcc.gnu.org/PR54965 */
/* GCC 4.6 has problems with force_inline, so just use normal inline instead */
#if defined(__GNUC__) && (__GNUC__ == 4) && (__GNUC_MINOR__ == 6)
#undef force_inline
#define force_inline __inline__
#endif

typedef float (* combine_channel_t) (float sa, float s, float da, float d);

static force_inline void
combine_inner (pixman_bool_t component,
	       float *dest, const float *src, const float *mask, int n_pixels,
	       combine_channel_t combine_a, combine_channel_t combine_c)
{
    int i;

    if (!mask)
    {
	for (i = 0; i < 4 * n_pixels; i += 4)
	{
	    float sa = src[i + 0];
	    float sr = src[i + 1];
	    float sg = src[i + 2];
	    float sb = src[i + 3];
	    
	    float da = dest[i + 0];
	    float dr = dest[i + 1];
	    float dg = dest[i + 2];
	    float db = dest[i + 3];					
	    
	    dest[i + 0] = combine_a (sa, sa, da, da);
	    dest[i + 1] = combine_c (sa, sr, da, dr);
	    dest[i + 2] = combine_c (sa, sg, da, dg);
	    dest[i + 3] = combine_c (sa, sb, da, db);
	}
    }
    else
    {
	for (i = 0; i < 4 * n_pixels; i += 4)
	{
	    float sa, sr, sg, sb;
	    float ma, mr, mg, mb;
	    float da, dr, dg, db;
	    
	    sa = src[i + 0];
	    sr = src[i + 1];
	    sg = src[i + 2];
	    sb = src[i + 3];
	    
	    if (component)
	    {
		ma = mask[i + 0];
		mr = mask[i + 1];
		mg = mask[i + 2];
		mb = mask[i + 3];

		sr *= mr;
		sg *= mg;
		sb *= mb;

		ma *= sa;
		mr *= sa;
		mg *= sa;
		mb *= sa;
		
		sa = ma;
	    }
	    else
	    {
		ma = mask[i + 0];

		sa *= ma;
		sr *= ma;
		sg *= ma;
		sb *= ma;

		ma = mr = mg = mb = sa;
	    }
	    
	    da = dest[i + 0];
	    dr = dest[i + 1];
	    dg = dest[i + 2];
	    db = dest[i + 3];
	    
	    dest[i + 0] = combine_a (ma, sa, da, da);
	    dest[i + 1] = combine_c (mr, sr, da, dr);
	    dest[i + 2] = combine_c (mg, sg, da, dg);
	    dest[i + 3] = combine_c (mb, sb, da, db);
	}
    }
}

#define MAKE_COMBINER(name, component, combine_a, combine_c)		\
    static void								\
    combine_ ## name ## _float (pixman_implementation_t *imp,		\
				pixman_op_t              op,		\
				float                   *dest,		\
				const float             *src,		\
				const float             *mask,		\
				int		         n_pixels)	\
    {									\
	combine_inner (component, dest, src, mask, n_pixels,		\
		       combine_a, combine_c);				\
    }

#define MAKE_COMBINERS(name, combine_a, combine_c)			\
    MAKE_COMBINER(name ## _ca, TRUE, combine_a, combine_c)		\
    MAKE_COMBINER(name ## _u, FALSE, combine_a, combine_c)


/*
 * Porter/Duff operators
 */
typedef enum
{
    ZERO,
    ONE,
    SRC_ALPHA,
    DEST_ALPHA,
    INV_SA,
    INV_DA,
    SA_OVER_DA,
    DA_OVER_SA,
    INV_SA_OVER_DA,
    INV_DA_OVER_SA,
    ONE_MINUS_SA_OVER_DA,
    ONE_MINUS_DA_OVER_SA,
    ONE_MINUS_INV_DA_OVER_SA,
    ONE_MINUS_INV_SA_OVER_DA
} combine_factor_t;

#define CLAMP(f)					\
    (((f) < 0)? 0 : (((f) > 1.0) ? 1.0 : (f)))

static force_inline float
get_factor (combine_factor_t factor, float sa, float da)
{
    float f = -1;

    switch (factor)
    {
    case ZERO:
	f = 0.0f;
	break;

    case ONE:
	f = 1.0f;
	break;

    case SRC_ALPHA:
	f = sa;
	break;

    case DEST_ALPHA:
	f = da;
	break;

    case INV_SA:
	f = 1 - sa;
	break;

    case INV_DA:
	f = 1 - da;
	break;

    case SA_OVER_DA:
	if (FLOAT_IS_ZERO (da))
	    f = 1.0f;
	else
	    f = CLAMP (sa / da);
	break;

    case DA_OVER_SA:
	if (FLOAT_IS_ZERO (sa))
	    f = 1.0f;
	else
	    f = CLAMP (da / sa);
	break;

    case INV_SA_OVER_DA:
	if (FLOAT_IS_ZERO (da))
	    f = 1.0f;
	else
	    f = CLAMP ((1.0f - sa) / da);
	break;

    case INV_DA_OVER_SA:
	if (FLOAT_IS_ZERO (sa))
	    f = 1.0f;
	else
	    f = CLAMP ((1.0f - da) / sa);
	break;

    case ONE_MINUS_SA_OVER_DA:
	if (FLOAT_IS_ZERO (da))
	    f = 0.0f;
	else
	    f = CLAMP (1.0f - sa / da);
	break;

    case ONE_MINUS_DA_OVER_SA:
	if (FLOAT_IS_ZERO (sa))
	    f = 0.0f;
	else
	    f = CLAMP (1.0f - da / sa);
	break;

    case ONE_MINUS_INV_DA_OVER_SA:
	if (FLOAT_IS_ZERO (sa))
	    f = 0.0f;
	else
	    f = CLAMP (1.0f - (1.0f - da) / sa);
	break;

    case ONE_MINUS_INV_SA_OVER_DA:
	if (FLOAT_IS_ZERO (da))
	    f = 0.0f;
	else
	    f = CLAMP (1.0f - (1.0f - sa) / da);
	break;
    }

    return f;
}

#define MAKE_PD_COMBINERS(name, a, b)					\
    static float force_inline						\
    pd_combine_ ## name (float sa, float s, float da, float d)		\
    {									\
	const float fa = get_factor (a, sa, da);			\
	const float fb = get_factor (b, sa, da);			\
									\
	return MIN (1.0f, s * fa + d * fb);				\
    }									\
    									\
    MAKE_COMBINERS(name, pd_combine_ ## name, pd_combine_ ## name)

MAKE_PD_COMBINERS (clear,			ZERO,				ZERO)
MAKE_PD_COMBINERS (src,				ONE,				ZERO)
MAKE_PD_COMBINERS (dst,				ZERO,				ONE)
MAKE_PD_COMBINERS (over,			ONE,				INV_SA)
MAKE_PD_COMBINERS (over_reverse,		INV_DA,				ONE)
MAKE_PD_COMBINERS (in,				DEST_ALPHA,			ZERO)
MAKE_PD_COMBINERS (in_reverse,			ZERO,				SRC_ALPHA)
MAKE_PD_COMBINERS (out,				INV_DA,				ZERO)
MAKE_PD_COMBINERS (out_reverse,			ZERO,				INV_SA)
MAKE_PD_COMBINERS (atop,			DEST_ALPHA,			INV_SA)
MAKE_PD_COMBINERS (atop_reverse,		INV_DA,				SRC_ALPHA)
MAKE_PD_COMBINERS (xor,				INV_DA,				INV_SA)
MAKE_PD_COMBINERS (add,				ONE,				ONE)

MAKE_PD_COMBINERS (saturate,			INV_DA_OVER_SA,			ONE)

MAKE_PD_COMBINERS (disjoint_clear,		ZERO,				ZERO)
MAKE_PD_COMBINERS (disjoint_src,		ONE,				ZERO)
MAKE_PD_COMBINERS (disjoint_dst,		ZERO,				ONE)
MAKE_PD_COMBINERS (disjoint_over,		ONE,				INV_SA_OVER_DA)
MAKE_PD_COMBINERS (disjoint_over_reverse,	INV_DA_OVER_SA,			ONE)
MAKE_PD_COMBINERS (disjoint_in,			ONE_MINUS_INV_DA_OVER_SA,	ZERO)
MAKE_PD_COMBINERS (disjoint_in_reverse,		ZERO,				ONE_MINUS_INV_SA_OVER_DA)
MAKE_PD_COMBINERS (disjoint_out,		INV_DA_OVER_SA,			ZERO)
MAKE_PD_COMBINERS (disjoint_out_reverse,	ZERO,				INV_SA_OVER_DA)
MAKE_PD_COMBINERS (disjoint_atop,		ONE_MINUS_INV_DA_OVER_SA,	INV_SA_OVER_DA)
MAKE_PD_COMBINERS (disjoint_atop_reverse,	INV_DA_OVER_SA,			ONE_MINUS_INV_SA_OVER_DA)
MAKE_PD_COMBINERS (disjoint_xor,		INV_DA_OVER_SA,			INV_SA_OVER_DA)

MAKE_PD_COMBINERS (conjoint_clear,		ZERO,				ZERO)
MAKE_PD_COMBINERS (conjoint_src,		ONE,				ZERO)
MAKE_PD_COMBINERS (conjoint_dst,		ZERO,				ONE)
MAKE_PD_COMBINERS (conjoint_over,		ONE,				ONE_MINUS_SA_OVER_DA)
MAKE_PD_COMBINERS (conjoint_over_reverse,	ONE_MINUS_DA_OVER_SA,		ONE)
MAKE_PD_COMBINERS (conjoint_in,			DA_OVER_SA,			ZERO)
MAKE_PD_COMBINERS (conjoint_in_reverse,		ZERO,				SA_OVER_DA)
MAKE_PD_COMBINERS (conjoint_out,		ONE_MINUS_DA_OVER_SA,		ZERO)
MAKE_PD_COMBINERS (conjoint_out_reverse,	ZERO,				ONE_MINUS_SA_OVER_DA)
MAKE_PD_COMBINERS (conjoint_atop,		DA_OVER_SA,			ONE_MINUS_SA_OVER_DA)
MAKE_PD_COMBINERS (conjoint_atop_reverse,	ONE_MINUS_DA_OVER_SA,		SA_OVER_DA)
MAKE_PD_COMBINERS (conjoint_xor,		ONE_MINUS_DA_OVER_SA,		ONE_MINUS_SA_OVER_DA)

/*
 * PDF blend modes:
 *
 * The following blend modes have been taken from the PDF ISO 32000
 * specification, which at this point in time is available from
 *
 *     http://www.adobe.com/devnet/pdf/pdf_reference.html
 *
 * The specific documents of interest are the PDF spec itself:
 *
 *     http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/pdf/pdfs/PDF32000_2008.pdf
 *
 * chapters 11.3.5 and 11.3.6 and a later supplement for Adobe Acrobat
 * 9.1 and Reader 9.1:
 *
 *     http://wwwimages.adobe.com/www.adobe.com/content/dam/Adobe/en/devnet/pdf/pdfs/adobe_supplement_iso32000_1.pdf
 *
 * that clarifies the specifications for blend modes ColorDodge and
 * ColorBurn.
 *
 * The formula for computing the final pixel color given in 11.3.6 is:
 *
 *     αr × Cr = (1 – αs) × αb × Cb + (1 – αb) × αs × Cs + αb × αs × B(Cb, Cs)
 *
 * with B() is the blend function. When B(Cb, Cs) = Cs, this formula
 * reduces to the regular OVER operator.
 *
 * Cs and Cb are not premultiplied, so in our implementation we instead
 * use:
 *
 *     cr = (1 – αs) × cb  +  (1 – αb) × cs  +  αb × αs × B (cb/αb, cs/αs)
 *
 * where cr, cs, and cb are premultiplied colors, and where the
 *
 *     αb × αs × B(cb/αb, cs/αs)
 *
 * part is first arithmetically simplified under the assumption that αb
 * and αs are not 0, and then updated to produce a meaningful result when
 * they are.
 *
 * For all the blend mode operators, the alpha channel is given by
 *
 *     αr = αs + αb + αb × αs
 */

#define MAKE_SEPARABLE_PDF_COMBINERS(name)				\
    static force_inline float						\
    combine_ ## name ## _a (float sa, float s, float da, float d)	\
    {									\
	return da + sa - da * sa;					\
    }									\
    									\
    static force_inline float						\
    combine_ ## name ## _c (float sa, float s, float da, float d)	\
    {									\
	float f = (1 - sa) * d + (1 - da) * s;				\
									\
	return f + blend_ ## name (sa, s, da, d);			\
    }									\
    									\
    MAKE_COMBINERS (name, combine_ ## name ## _a, combine_ ## name ## _c)

/*
 * Multiply
 *
 *      ad * as * B(d / ad, s / as)
 *    = ad * as * d/ad * s/as
 *    = d * s
 *
 */
static force_inline float
blend_multiply (float sa, float s, float da, float d)
{
    return d * s;
}

/*
 * Screen
 *
 *      ad * as * B(d/ad, s/as)
 *    = ad * as * (d/ad + s/as - s/as * d/ad)
 *    = ad * s + as * d - s * d
 */
static force_inline float
blend_screen (float sa, float s, float da, float d)
{
    return d * sa + s * da - s * d;
}

/*
 * Overlay
 *
 *     ad * as * B(d/ad, s/as)
 *   = ad * as * Hardlight (s, d)
 *   = if (d / ad < 0.5)
 *         as * ad * Multiply (s/as, 2 * d/ad)
 *     else
 *         as * ad * Screen (s/as, 2 * d / ad - 1)
 *   = if (d < 0.5 * ad)
 *         as * ad * s/as * 2 * d /ad
 *     else
 *         as * ad * (s/as + 2 * d / ad - 1 - s / as * (2 * d / ad - 1))
 *   = if (2 * d < ad)
 *         2 * s * d
 *     else
 *         ad * s + 2 * as * d - as * ad - ad * s * (2 * d / ad - 1)
 *   = if (2 * d < ad)
 *         2 * s * d
 *     else
 *         as * ad - 2 * (ad - d) * (as - s)
 */
static force_inline float
blend_overlay (float sa, float s, float da, float d)
{
    if (2 * d < da)
	return 2 * s * d;
    else
	return sa * da - 2 * (da - d) * (sa - s);
}

/*
 * Darken
 *
 *     ad * as * B(d/ad, s/as)
 *   = ad * as * MIN(d/ad, s/as)
 *   = MIN (as * d, ad * s)
 */
static force_inline float
blend_darken (float sa, float s, float da, float d)
{
    s = s * da;
    d = d * sa;

    if (s > d)
	return d;
    else
	return s;
}

/*
 * Lighten
 *
 *     ad * as * B(d/ad, s/as)
 *   = ad * as * MAX(d/ad, s/as)
 *   = MAX (as * d, ad * s)
 */
static force_inline float
blend_lighten (float sa, float s, float da, float d)
{
    s = s * da;
    d = d * sa;

    if (s > d)
	return s;
    else
	return d;
}

/*
 * Color dodge
 *
 *     ad * as * B(d/ad, s/as)
 *   = if d/ad = 0
 *         ad * as * 0
 *     else if (d/ad >= (1 - s/as)
 *         ad * as * 1
 *     else
 *         ad * as * ((d/ad) / (1 - s/as))
 *   = if d = 0
 *         0
 *     elif as * d >= ad * (as - s)
 *         ad * as
 *     else
 *         as * (as * d / (as - s))
 *
 */
static force_inline float
blend_color_dodge (float sa, float s, float da, float d)
{
    if (FLOAT_IS_ZERO (d))
	return 0.0f;
    else if (d * sa >= sa * da - s * da)
	return sa * da;
    else if (FLOAT_IS_ZERO (sa - s))
	return sa * da;
    else
	return sa * sa * d / (sa - s);
}

/*
 * Color burn
 *
 * We modify the first clause "if d = 1" to "if d >= 1" since with
 * premultiplied colors d > 1 can actually happen.
 *
 *     ad * as * B(d/ad, s/as)
 *   = if d/ad >= 1
 *         ad * as * 1
 *     elif (1 - d/ad) >= s/as
 *         ad * as * 0
 *     else
 *         ad * as * (1 - ((1 - d/ad) / (s/as)))
 *   = if d >= ad
 *         ad * as
 *     elif as * ad - as * d >= ad * s
 *         0
 *     else
 *         ad * as  - as * as * (ad - d) / s
 */
static force_inline float
blend_color_burn (float sa, float s, float da, float d)
{
    if (d >= da)
	return sa * da;
    else if (sa * (da - d) >= s * da)
	return 0.0f;
    else if (FLOAT_IS_ZERO (s))
	return 0.0f;
    else
	return sa * (da - sa * (da - d) / s);
}

/*
 * Hard light
 *
 *     ad * as * B(d/ad, s/as)
 *   = if (s/as <= 0.5)
 *         ad * as * Multiply (d/ad, 2 * s/as)
 *     else
 *         ad * as * Screen (d/ad, 2 * s/as - 1)
 *   = if 2 * s <= as
 *         ad * as * d/ad * 2 * s / as
 *     else
 *         ad * as * (d/ad + (2 * s/as - 1) + d/ad * (2 * s/as - 1))
 *   = if 2 * s <= as
 *         2 * s * d
 *     else
 *         as * ad - 2 * (ad - d) * (as - s)
 */
static force_inline float
blend_hard_light (float sa, float s, float da, float d)
{
    if (2 * s < sa)
	return 2 * s * d;
    else
	return sa * da - 2 * (da - d) * (sa - s);
}

/*
 * Soft light
 *
 *     ad * as * B(d/ad, s/as)
 *   = if (s/as <= 0.5)
 *         ad * as * (d/ad - (1 - 2 * s/as) * d/ad * (1 - d/ad))
 *     else if (d/ad <= 0.25)
 *         ad * as * (d/ad + (2 * s/as - 1) * ((((16 * d/ad - 12) * d/ad + 4) * d/ad) - d/ad))
 *     else
 *         ad * as * (d/ad + (2 * s/as - 1) * sqrt (d/ad))
 *   = if (2 * s <= as)
 *         d * as - d * (ad - d) * (as - 2 * s) / ad;
 *     else if (4 * d <= ad)
 *         (2 * s - as) * d * ((16 * d / ad - 12) * d / ad + 3);
 *     else
 *         d * as + (sqrt (d * ad) - d) * (2 * s - as);
 */
static force_inline float
blend_soft_light (float sa, float s, float da, float d)
{
    if (2 * s <= sa)
    {
	if (FLOAT_IS_ZERO (da))
	    return d * sa;
	else
	    return d * sa - d * (da - d) * (sa - 2 * s) / da;
    }
    else
    {
	if (FLOAT_IS_ZERO (da))
	{
	    return d * sa;
	}
	else
	{
	    if (4 * d <= da)
		return d * sa + (2 * s - sa) * d * ((16 * d / da - 12) * d / da + 3);
	    else
		return d * sa + (sqrtf (d * da) - d) * (2 * s - sa);
	}
    }
}

/*
 * Difference
 *
 *     ad * as * B(s/as, d/ad)
 *   = ad * as * abs (s/as - d/ad)
 *   = if (s/as <= d/ad)
 *         ad * as * (d/ad - s/as)
 *     else
 *         ad * as * (s/as - d/ad)
 *   = if (ad * s <= as * d)
 *        as * d - ad * s
 *     else
 *        ad * s - as * d
 */
static force_inline float
blend_difference (float sa, float s, float da, float d)
{
    float dsa = d * sa;
    float sda = s * da;

    if (sda < dsa)
	return dsa - sda;
    else
	return sda - dsa;
}

/*
 * Exclusion
 *
 *     ad * as * B(s/as, d/ad)
 *   = ad * as * (d/ad + s/as - 2 * d/ad * s/as)
 *   = as * d + ad * s - 2 * s * d
 */
static force_inline float
blend_exclusion (float sa, float s, float da, float d)
{
    return s * da + d * sa - 2 * d * s;
}

MAKE_SEPARABLE_PDF_COMBINERS (multiply)
MAKE_SEPARABLE_PDF_COMBINERS (screen)
MAKE_SEPARABLE_PDF_COMBINERS (overlay)
MAKE_SEPARABLE_PDF_COMBINERS (darken)
MAKE_SEPARABLE_PDF_COMBINERS (lighten)
MAKE_SEPARABLE_PDF_COMBINERS (color_dodge)
MAKE_SEPARABLE_PDF_COMBINERS (color_burn)
MAKE_SEPARABLE_PDF_COMBINERS (hard_light)
MAKE_SEPARABLE_PDF_COMBINERS (soft_light)
MAKE_SEPARABLE_PDF_COMBINERS (difference)
MAKE_SEPARABLE_PDF_COMBINERS (exclusion)

/*
 * PDF nonseperable blend modes are implemented using the following functions
 * to operate in Hsl space, with Cmax, Cmid, Cmin referring to the max, mid
 * and min value of the red, green and blue components.
 *
 * LUM (C) = 0.3 × Cred + 0.59 × Cgreen + 0.11 × Cblue
 *
 * clip_color (C):
 *     l = LUM (C)
 *     min = Cmin
 *     max = Cmax
 *     if n < 0.0
 *         C = l + (((C – l) × l) ⁄ (l – min))
 *     if x > 1.0
 *         C = l + (((C – l) × (1 – l) ) ⁄ (max – l))
 *     return C
 *
 * set_lum (C, l):
 *     d = l – LUM (C)
 *     C += d
 *     return clip_color (C)
 *
 * SAT (C) = CH_MAX (C) - CH_MIN (C)
 *
 * set_sat (C, s):
 *     if Cmax > Cmin
 *         Cmid = ( ( ( Cmid – Cmin ) × s ) ⁄ ( Cmax – Cmin ) )
 *         Cmax = s
 *     else
 *         Cmid = Cmax = 0.0
 *         Cmin = 0.0
 *     return C
 */

/* For premultiplied colors, we need to know what happens when C is
 * multiplied by a real number. LUM and SAT are linear:
 *
 *     LUM (r × C) = r × LUM (C)	SAT (r * C) = r * SAT (C)
 *
 * If we extend clip_color with an extra argument a and change
 *
 *     if x >= 1.0
 *
 * into
 *
 *     if x >= a
 *
 * then clip_color is also linear:
 *
 *     r * clip_color (C, a) = clip_color (r * C, r * a);
 *
 * for positive r.
 *
 * Similarly, we can extend set_lum with an extra argument that is just passed
 * on to clip_color:
 *
 *       r * set_lum (C, l, a)
 *
 *     = r × clip_color (C + l - LUM (C), a)
 *
 *     = clip_color (r * C + r × l - r * LUM (C), r * a)
 *
 *     = set_lum (r * C, r * l, r * a)
 *
 * Finally, set_sat:
 *
 *       r * set_sat (C, s) = set_sat (x * C, r * s)
 *
 * The above holds for all non-zero x, because the x'es in the fraction for
 * C_mid cancel out. Specifically, it holds for x = r:
 *
 *       r * set_sat (C, s) = set_sat (r * C, r * s)
 *
 */
typedef struct
{
    float	r;
    float	g;
    float	b;
} rgb_t;

static force_inline float
minf (float a, float b)
{
    return a < b? a : b;
}

static force_inline float
maxf (float a, float b)
{
    return a > b? a : b;
}

static force_inline float
channel_min (const rgb_t *c)
{
    return minf (minf (c->r, c->g), c->b);
}

static force_inline float
channel_max (const rgb_t *c)
{
    return maxf (maxf (c->r, c->g), c->b);
}

static force_inline float
get_lum (const rgb_t *c)
{
    return c->r * 0.3f + c->g * 0.59f + c->b * 0.11f;
}

static force_inline float
get_sat (const rgb_t *c)
{
    return channel_max (c) - channel_min (c);
}

static void
clip_color (rgb_t *color, float a)
{
    float l = get_lum (color);
    float n = channel_min (color);
    float x = channel_max (color);
    float t;

    if (n < 0.0f)
    {
	t = l - n;
	if (FLOAT_IS_ZERO (t))
	{
	    color->r = 0.0f;
	    color->g = 0.0f;
	    color->b = 0.0f;
	}
	else
	{
	    color->r = l + (((color->r - l) * l) / t);
	    color->g = l + (((color->g - l) * l) / t);
	    color->b = l + (((color->b - l) * l) / t);
	}
    }
    if (x > a)
    {
	t = x - l;
	if (FLOAT_IS_ZERO (t))
	{
	    color->r = a;
	    color->g = a;
	    color->b = a;
	}
	else
	{
	    color->r = l + (((color->r - l) * (a - l) / t));
	    color->g = l + (((color->g - l) * (a - l) / t));
	    color->b = l + (((color->b - l) * (a - l) / t));
	}
    }
}

static void
set_lum (rgb_t *color, float sa, float l)
{
    float d = l - get_lum (color);

    color->r = color->r + d;
    color->g = color->g + d;
    color->b = color->b + d;

    clip_color (color, sa);
}

static void
set_sat (rgb_t *src, float sat)
{
    float *max, *mid, *min;
    float t;

    if (src->r > src->g)
    {
	if (src->r > src->b)
	{
	    max = &(src->r);

	    if (src->g > src->b)
	    {
		mid = &(src->g);
		min = &(src->b);
	    }
	    else
	    {
		mid = &(src->b);
		min = &(src->g);
	    }
	}
	else
	{
	    max = &(src->b);
	    mid = &(src->r);
	    min = &(src->g);
	}
    }
    else
    {
	if (src->r > src->b)
	{
	    max = &(src->g);
	    mid = &(src->r);
	    min = &(src->b);
	}
	else
	{
	    min = &(src->r);

	    if (src->g > src->b)
	    {
		max = &(src->g);
		mid = &(src->b);
	    }
	    else
	    {
		max = &(src->b);
		mid = &(src->g);
	    }
	}
    }

    t = *max - *min;

    if (FLOAT_IS_ZERO (t))
    {
	*mid = *max = 0.0f;
    }
    else
    {
	*mid = ((*mid - *min) * sat) / t;
	*max = sat;
    }

    *min = 0.0f;
}

/* Hue:
 *
 *       as * ad * B(s/as, d/as)
 *     = as * ad * set_lum (set_sat (s/as, SAT (d/ad)), LUM (d/ad), 1)
 *     = set_lum (set_sat (ad * s, as * SAT (d)), as * LUM (d), as * ad)
 *
 */
static force_inline void
blend_hsl_hue (rgb_t *res,
	       const rgb_t *dest, float da,
	       const rgb_t *src, float sa)
{
    res->r = src->r * da;
    res->g = src->g * da;
    res->b = src->b * da;

    set_sat (res, get_sat (dest) * sa);
    set_lum (res, sa * da, get_lum (dest) * sa);
}

/* 
 * Saturation
 *
 *     as * ad * B(s/as, d/ad)
 *   = as * ad * set_lum (set_sat (d/ad, SAT (s/as)), LUM (d/ad), 1)
 *   = set_lum (as * ad * set_sat (d/ad, SAT (s/as)),
 *                                       as * LUM (d), as * ad)
 *   = set_lum (set_sat (as * d, ad * SAT (s), as * LUM (d), as * ad))
 */
static force_inline void
blend_hsl_saturation (rgb_t *res,
		      const rgb_t *dest, float da,
		      const rgb_t *src, float sa)
{
    res->r = dest->r * sa;
    res->g = dest->g * sa;
    res->b = dest->b * sa;

    set_sat (res, get_sat (src) * da);
    set_lum (res, sa * da, get_lum (dest) * sa);
}

/* 
 * Color
 *
 *     as * ad * B(s/as, d/as)
 *   = as * ad * set_lum (s/as, LUM (d/ad), 1)
 *   = set_lum (s * ad, as * LUM (d), as * ad)
 */
static force_inline void
blend_hsl_color (rgb_t *res,
		 const rgb_t *dest, float da,
		 const rgb_t *src, float sa)
{
    res->r = src->r * da;
    res->g = src->g * da;
    res->b = src->b * da;

    set_lum (res, sa * da, get_lum (dest) * sa);
}

/*
 * Luminosity
 *
 *     as * ad * B(s/as, d/ad)
 *   = as * ad * set_lum (d/ad, LUM (s/as), 1)
 *   = set_lum (as * d, ad * LUM (s), as * ad)
 */
static force_inline void
blend_hsl_luminosity (rgb_t *res,
		      const rgb_t *dest, float da,
		      const rgb_t *src, float sa)
{
    res->r = dest->r * sa;
    res->g = dest->g * sa;
    res->b = dest->b * sa;

    set_lum (res, sa * da, get_lum (src) * da);
}

#define MAKE_NON_SEPARABLE_PDF_COMBINERS(name)				\
    static void								\
    combine_ ## name ## _u_float (pixman_implementation_t *imp,		\
				  pixman_op_t              op,		\
				  float                   *dest,	\
				  const float             *src,		\
				  const float             *mask,	\
				  int		           n_pixels)	\
    {									\
    	int i;								\
									\
	for (i = 0; i < 4 * n_pixels; i += 4)				\
	{								\
	    float sa, da;						\
	    rgb_t sc, dc, rc;						\
									\
	    sa = src[i + 0];						\
	    sc.r = src[i + 1];						\
	    sc.g = src[i + 2];						\
	    sc.b = src[i + 3];						\
									\
	    da = dest[i + 0];						\
	    dc.r = dest[i + 1];						\
	    dc.g = dest[i + 2];						\
	    dc.b = dest[i + 3];						\
									\
	    if (mask)							\
	    {								\
		float ma = mask[i + 0];					\
									\
		/* Component alpha is not supported for HSL modes */	\
		sa *= ma;						\
		sc.r *= ma;						\
		sc.g *= ma;						\
		sc.g *= ma;						\
	    }								\
									\
	    blend_ ## name (&rc, &dc, da, &sc, sa);			\
									\
	    dest[i + 0] = sa + da - sa * da;				\
	    dest[i + 1] = (1 - sa) * dc.r + (1 - da) * sc.r + rc.r;	\
	    dest[i + 2] = (1 - sa) * dc.g + (1 - da) * sc.g + rc.g;	\
	    dest[i + 3] = (1 - sa) * dc.b + (1 - da) * sc.b + rc.b;	\
	}								\
    }

MAKE_NON_SEPARABLE_PDF_COMBINERS(hsl_hue)
MAKE_NON_SEPARABLE_PDF_COMBINERS(hsl_saturation)
MAKE_NON_SEPARABLE_PDF_COMBINERS(hsl_color)
MAKE_NON_SEPARABLE_PDF_COMBINERS(hsl_luminosity)

void
_pixman_setup_combiner_functions_float (pixman_implementation_t *imp)
{
    /* Unified alpha */
    imp->combine_float[PIXMAN_OP_CLEAR] = combine_clear_u_float;
    imp->combine_float[PIXMAN_OP_SRC] = combine_src_u_float;
    imp->combine_float[PIXMAN_OP_DST] = combine_dst_u_float;
    imp->combine_float[PIXMAN_OP_OVER] = combine_over_u_float;
    imp->combine_float[PIXMAN_OP_OVER_REVERSE] = combine_over_reverse_u_float;
    imp->combine_float[PIXMAN_OP_IN] = combine_in_u_float;
    imp->combine_float[PIXMAN_OP_IN_REVERSE] = combine_in_reverse_u_float;
    imp->combine_float[PIXMAN_OP_OUT] = combine_out_u_float;
    imp->combine_float[PIXMAN_OP_OUT_REVERSE] = combine_out_reverse_u_float;
    imp->combine_float[PIXMAN_OP_ATOP] = combine_atop_u_float;
    imp->combine_float[PIXMAN_OP_ATOP_REVERSE] = combine_atop_reverse_u_float;
    imp->combine_float[PIXMAN_OP_XOR] = combine_xor_u_float;
    imp->combine_float[PIXMAN_OP_ADD] = combine_add_u_float;
    imp->combine_float[PIXMAN_OP_SATURATE] = combine_saturate_u_float;

    /* Disjoint, unified */
    imp->combine_float[PIXMAN_OP_DISJOINT_CLEAR] = combine_disjoint_clear_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_SRC] = combine_disjoint_src_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_DST] = combine_disjoint_dst_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_OVER] = combine_disjoint_over_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_OVER_REVERSE] = combine_disjoint_over_reverse_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_IN] = combine_disjoint_in_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_IN_REVERSE] = combine_disjoint_in_reverse_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_OUT] = combine_disjoint_out_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_OUT_REVERSE] = combine_disjoint_out_reverse_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_ATOP] = combine_disjoint_atop_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_ATOP_REVERSE] = combine_disjoint_atop_reverse_u_float;
    imp->combine_float[PIXMAN_OP_DISJOINT_XOR] = combine_disjoint_xor_u_float;

    /* Conjoint, unified */
    imp->combine_float[PIXMAN_OP_CONJOINT_CLEAR] = combine_conjoint_clear_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_SRC] = combine_conjoint_src_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_DST] = combine_conjoint_dst_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_OVER] = combine_conjoint_over_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_OVER_REVERSE] = combine_conjoint_over_reverse_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_IN] = combine_conjoint_in_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_IN_REVERSE] = combine_conjoint_in_reverse_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_OUT] = combine_conjoint_out_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_OUT_REVERSE] = combine_conjoint_out_reverse_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_ATOP] = combine_conjoint_atop_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_ATOP_REVERSE] = combine_conjoint_atop_reverse_u_float;
    imp->combine_float[PIXMAN_OP_CONJOINT_XOR] = combine_conjoint_xor_u_float;

    /* PDF operators, unified */
    imp->combine_float[PIXMAN_OP_MULTIPLY] = combine_multiply_u_float;
    imp->combine_float[PIXMAN_OP_SCREEN] = combine_screen_u_float;
    imp->combine_float[PIXMAN_OP_OVERLAY] = combine_overlay_u_float;
    imp->combine_float[PIXMAN_OP_DARKEN] = combine_darken_u_float;
    imp->combine_float[PIXMAN_OP_LIGHTEN] = combine_lighten_u_float;
    imp->combine_float[PIXMAN_OP_COLOR_DODGE] = combine_color_dodge_u_float;
    imp->combine_float[PIXMAN_OP_COLOR_BURN] = combine_color_burn_u_float;
    imp->combine_float[PIXMAN_OP_HARD_LIGHT] = combine_hard_light_u_float;
    imp->combine_float[PIXMAN_OP_SOFT_LIGHT] = combine_soft_light_u_float;
    imp->combine_float[PIXMAN_OP_DIFFERENCE] = combine_difference_u_float;
    imp->combine_float[PIXMAN_OP_EXCLUSION] = combine_exclusion_u_float;

    imp->combine_float[PIXMAN_OP_HSL_HUE] = combine_hsl_hue_u_float;
    imp->combine_float[PIXMAN_OP_HSL_SATURATION] = combine_hsl_saturation_u_float;
    imp->combine_float[PIXMAN_OP_HSL_COLOR] = combine_hsl_color_u_float;
    imp->combine_float[PIXMAN_OP_HSL_LUMINOSITY] = combine_hsl_luminosity_u_float;

    /* Component alpha combiners */
    imp->combine_float_ca[PIXMAN_OP_CLEAR] = combine_clear_ca_float;
    imp->combine_float_ca[PIXMAN_OP_SRC] = combine_src_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DST] = combine_dst_ca_float;
    imp->combine_float_ca[PIXMAN_OP_OVER] = combine_over_ca_float;
    imp->combine_float_ca[PIXMAN_OP_OVER_REVERSE] = combine_over_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_IN] = combine_in_ca_float;
    imp->combine_float_ca[PIXMAN_OP_IN_REVERSE] = combine_in_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_OUT] = combine_out_ca_float;
    imp->combine_float_ca[PIXMAN_OP_OUT_REVERSE] = combine_out_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_ATOP] = combine_atop_ca_float;
    imp->combine_float_ca[PIXMAN_OP_ATOP_REVERSE] = combine_atop_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_XOR] = combine_xor_ca_float;
    imp->combine_float_ca[PIXMAN_OP_ADD] = combine_add_ca_float;
    imp->combine_float_ca[PIXMAN_OP_SATURATE] = combine_saturate_ca_float;

    /* Disjoint CA */
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_CLEAR] = combine_disjoint_clear_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_SRC] = combine_disjoint_src_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_DST] = combine_disjoint_dst_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_OVER] = combine_disjoint_over_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_OVER_REVERSE] = combine_disjoint_over_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_IN] = combine_disjoint_in_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_IN_REVERSE] = combine_disjoint_in_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_OUT] = combine_disjoint_out_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_OUT_REVERSE] = combine_disjoint_out_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_ATOP] = combine_disjoint_atop_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_ATOP_REVERSE] = combine_disjoint_atop_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DISJOINT_XOR] = combine_disjoint_xor_ca_float;

    /* Conjoint CA */
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_CLEAR] = combine_conjoint_clear_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_SRC] = combine_conjoint_src_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_DST] = combine_conjoint_dst_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_OVER] = combine_conjoint_over_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_OVER_REVERSE] = combine_conjoint_over_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_IN] = combine_conjoint_in_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_IN_REVERSE] = combine_conjoint_in_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_OUT] = combine_conjoint_out_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_OUT_REVERSE] = combine_conjoint_out_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_ATOP] = combine_conjoint_atop_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_ATOP_REVERSE] = combine_conjoint_atop_reverse_ca_float;
    imp->combine_float_ca[PIXMAN_OP_CONJOINT_XOR] = combine_conjoint_xor_ca_float;

    /* PDF operators CA */
    imp->combine_float_ca[PIXMAN_OP_MULTIPLY] = combine_multiply_ca_float;
    imp->combine_float_ca[PIXMAN_OP_SCREEN] = combine_screen_ca_float;
    imp->combine_float_ca[PIXMAN_OP_OVERLAY] = combine_overlay_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DARKEN] = combine_darken_ca_float;
    imp->combine_float_ca[PIXMAN_OP_LIGHTEN] = combine_lighten_ca_float;
    imp->combine_float_ca[PIXMAN_OP_COLOR_DODGE] = combine_color_dodge_ca_float;
    imp->combine_float_ca[PIXMAN_OP_COLOR_BURN] = combine_color_burn_ca_float;
    imp->combine_float_ca[PIXMAN_OP_HARD_LIGHT] = combine_hard_light_ca_float;
    imp->combine_float_ca[PIXMAN_OP_SOFT_LIGHT] = combine_soft_light_ca_float;
    imp->combine_float_ca[PIXMAN_OP_DIFFERENCE] = combine_difference_ca_float;
    imp->combine_float_ca[PIXMAN_OP_EXCLUSION] = combine_exclusion_ca_float;

    /* It is not clear that these make sense, so make them noops for now */
    imp->combine_float_ca[PIXMAN_OP_HSL_HUE] = combine_dst_u_float;
    imp->combine_float_ca[PIXMAN_OP_HSL_SATURATION] = combine_dst_u_float;
    imp->combine_float_ca[PIXMAN_OP_HSL_COLOR] = combine_dst_u_float;
    imp->combine_float_ca[PIXMAN_OP_HSL_LUMINOSITY] = combine_dst_u_float;
}
