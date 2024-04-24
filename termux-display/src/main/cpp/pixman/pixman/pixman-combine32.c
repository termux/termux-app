/*
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *             2005 Lars Knoll & Zack Rusin, Trolltech
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
#ifdef HAVE_CONFIG_H
#include <pixman-config.h>
#endif

#include <math.h>
#include <string.h>

#include "pixman-private.h"
#include "pixman-combine32.h"

/* component alpha helper functions */

static void
combine_mask_ca (uint32_t *src, uint32_t *mask)
{
    uint32_t a = *mask;

    uint32_t x;
    uint16_t xa;

    if (!a)
    {
	*(src) = 0;
	return;
    }

    x = *(src);
    if (a == ~0)
    {
	x = x >> A_SHIFT;
	x |= x << G_SHIFT;
	x |= x << R_SHIFT;
	*(mask) = x;
	return;
    }

    xa = x >> A_SHIFT;
    UN8x4_MUL_UN8x4 (x, a);
    *(src) = x;
    
    UN8x4_MUL_UN8 (a, xa);
    *(mask) = a;
}

static void
combine_mask_value_ca (uint32_t *src, const uint32_t *mask)
{
    uint32_t a = *mask;
    uint32_t x;

    if (!a)
    {
	*(src) = 0;
	return;
    }

    if (a == ~0)
	return;

    x = *(src);
    UN8x4_MUL_UN8x4 (x, a);
    *(src) = x;
}

static void
combine_mask_alpha_ca (const uint32_t *src, uint32_t *mask)
{
    uint32_t a = *(mask);
    uint32_t x;

    if (!a)
	return;

    x = *(src) >> A_SHIFT;
    if (x == MASK)
	return;

    if (a == ~0)
    {
	x |= x << G_SHIFT;
	x |= x << R_SHIFT;
	*(mask) = x;
	return;
    }

    UN8x4_MUL_UN8 (a, x);
    *(mask) = a;
}

/*
 * There are two ways of handling alpha -- either as a single unified value or
 * a separate value for each component, hence each macro must have two
 * versions.  The unified alpha version has a 'u' at the end of the name,
 * the component version has a 'ca'.  Similarly, functions which deal with
 * this difference will have two versions using the same convention.
 */

static force_inline uint32_t
combine_mask (const uint32_t *src, const uint32_t *mask, int i)
{
    uint32_t s, m;

    if (mask)
    {
	m = *(mask + i) >> A_SHIFT;

	if (!m)
	    return 0;
    }

    s = *(src + i);

    if (mask)
	UN8x4_MUL_UN8 (s, m);

    return s;
}

static void
combine_clear (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint32_t *               dest,
               const uint32_t *         src,
               const uint32_t *         mask,
               int                      width)
{
    memset (dest, 0, width * sizeof (uint32_t));
}

static void
combine_dst (pixman_implementation_t *imp,
	     pixman_op_t	      op,
	     uint32_t *		      dest,
	     const uint32_t *	      src,
	     const uint32_t *         mask,
	     int		      width)
{
    return;
}

static void
combine_src_u (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint32_t *               dest,
               const uint32_t *         src,
               const uint32_t *         mask,
               int                      width)
{
    int i;

    if (!mask)
    {
	memcpy (dest, src, width * sizeof (uint32_t));
    }
    else
    {
	for (i = 0; i < width; ++i)
	{
	    uint32_t s = combine_mask (src, mask, i);

	    *(dest + i) = s;
	}
    }
}

static void
combine_over_u (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint32_t *               dest,
                const uint32_t *         src,
                const uint32_t *         mask,
                int                      width)
{
    int i;

    if (!mask)
    {
	for (i = 0; i < width; ++i)
	{
	    uint32_t s = *(src + i);
	    uint32_t a = ALPHA_8 (s);
	    if (a == 0xFF)
	    {
		*(dest + i) = s;
	    }
	    else if (s)
	    {
		uint32_t d = *(dest + i);
		uint32_t ia = a ^ 0xFF;
		UN8x4_MUL_UN8_ADD_UN8x4 (d, ia, s);
		*(dest + i) = d;
	    }
	}
    }
    else
    {
	for (i = 0; i < width; ++i)
	{
	    uint32_t m = ALPHA_8 (*(mask + i));
	    if (m == 0xFF)
	    {
		uint32_t s = *(src + i);
		uint32_t a = ALPHA_8 (s);
		if (a == 0xFF)
		{
		    *(dest + i) = s;
		}
		else if (s)
		{
		    uint32_t d = *(dest + i);
		    uint32_t ia = a ^ 0xFF;
		    UN8x4_MUL_UN8_ADD_UN8x4 (d, ia, s);
		    *(dest + i) = d;
		}
	    }
	    else if (m)
	    {
		uint32_t s = *(src + i);
		if (s)
		{
		    uint32_t d = *(dest + i);
		    UN8x4_MUL_UN8 (s, m);
		    UN8x4_MUL_UN8_ADD_UN8x4 (d, ALPHA_8 (~s), s);
		    *(dest + i) = d;
		}
	    }
	}
    }
}

static void
combine_over_reverse_u (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint32_t *               dest,
                        const uint32_t *         src,
                        const uint32_t *         mask,
                        int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = combine_mask (src, mask, i);
	uint32_t d = *(dest + i);
	uint32_t ia = ALPHA_8 (~*(dest + i));
	UN8x4_MUL_UN8_ADD_UN8x4 (s, ia, d);
	*(dest + i) = s;
    }
}

static void
combine_in_u (pixman_implementation_t *imp,
              pixman_op_t              op,
              uint32_t *               dest,
              const uint32_t *         src,
              const uint32_t *         mask,
              int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = combine_mask (src, mask, i);
	uint32_t a = ALPHA_8 (*(dest + i));
	UN8x4_MUL_UN8 (s, a);
	*(dest + i) = s;
    }
}

static void
combine_in_reverse_u (pixman_implementation_t *imp,
                      pixman_op_t              op,
                      uint32_t *               dest,
                      const uint32_t *         src,
                      const uint32_t *         mask,
                      int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = combine_mask (src, mask, i);
	uint32_t d = *(dest + i);
	uint32_t a = ALPHA_8 (s);
	UN8x4_MUL_UN8 (d, a);
	*(dest + i) = d;
    }
}

static void
combine_out_u (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint32_t *               dest,
               const uint32_t *         src,
               const uint32_t *         mask,
               int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = combine_mask (src, mask, i);
	uint32_t a = ALPHA_8 (~*(dest + i));
	UN8x4_MUL_UN8 (s, a);
	*(dest + i) = s;
    }
}

static void
combine_out_reverse_u (pixman_implementation_t *imp,
                       pixman_op_t              op,
                       uint32_t *               dest,
                       const uint32_t *         src,
                       const uint32_t *         mask,
                       int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = combine_mask (src, mask, i);
	uint32_t d = *(dest + i);
	uint32_t a = ALPHA_8 (~s);
	UN8x4_MUL_UN8 (d, a);
	*(dest + i) = d;
    }
}

static void
combine_atop_u (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint32_t *               dest,
                const uint32_t *         src,
                const uint32_t *         mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = combine_mask (src, mask, i);
	uint32_t d = *(dest + i);
	uint32_t dest_a = ALPHA_8 (d);
	uint32_t src_ia = ALPHA_8 (~s);

	UN8x4_MUL_UN8_ADD_UN8x4_MUL_UN8 (s, dest_a, d, src_ia);
	*(dest + i) = s;
    }
}

static void
combine_atop_reverse_u (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint32_t *               dest,
                        const uint32_t *         src,
                        const uint32_t *         mask,
                        int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = combine_mask (src, mask, i);
	uint32_t d = *(dest + i);
	uint32_t src_a = ALPHA_8 (s);
	uint32_t dest_ia = ALPHA_8 (~d);

	UN8x4_MUL_UN8_ADD_UN8x4_MUL_UN8 (s, dest_ia, d, src_a);
	*(dest + i) = s;
    }
}

static void
combine_xor_u (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint32_t *               dest,
               const uint32_t *         src,
               const uint32_t *         mask,
               int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = combine_mask (src, mask, i);
	uint32_t d = *(dest + i);
	uint32_t src_ia = ALPHA_8 (~s);
	uint32_t dest_ia = ALPHA_8 (~d);

	UN8x4_MUL_UN8_ADD_UN8x4_MUL_UN8 (s, dest_ia, d, src_ia);
	*(dest + i) = s;
    }
}

static void
combine_add_u (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint32_t *               dest,
               const uint32_t *         src,
               const uint32_t *         mask,
               int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = combine_mask (src, mask, i);
	uint32_t d = *(dest + i);
	UN8x4_ADD_UN8x4 (d, s);
	*(dest + i) = d;
    }
}

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

/*
 * Multiply
 *
 *      ad * as * B(d / ad, s / as)
 *    = ad * as * d/ad * s/as
 *    = d * s
 *
 */
static void
combine_multiply_u (pixman_implementation_t *imp,
                    pixman_op_t              op,
                    uint32_t *               dest,
                    const uint32_t *         src,
                    const uint32_t *         mask,
                    int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = combine_mask (src, mask, i);
	uint32_t d = *(dest + i);
	uint32_t ss = s;
	uint32_t src_ia = ALPHA_8 (~s);
	uint32_t dest_ia = ALPHA_8 (~d);

	UN8x4_MUL_UN8_ADD_UN8x4_MUL_UN8 (ss, dest_ia, d, src_ia);
	UN8x4_MUL_UN8x4 (d, s);
	UN8x4_ADD_UN8x4 (d, ss);

	*(dest + i) = d;
    }
}

static void
combine_multiply_ca (pixman_implementation_t *imp,
                     pixman_op_t              op,
                     uint32_t *               dest,
                     const uint32_t *         src,
                     const uint32_t *         mask,
                     int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t m = *(mask + i);
	uint32_t s = *(src + i);
	uint32_t d = *(dest + i);
	uint32_t r = d;
	uint32_t dest_ia = ALPHA_8 (~d);

	combine_mask_ca (&s, &m);

	UN8x4_MUL_UN8x4_ADD_UN8x4_MUL_UN8 (r, ~m, s, dest_ia);
	UN8x4_MUL_UN8x4 (d, s);
	UN8x4_ADD_UN8x4 (r, d);

	*(dest + i) = r;
    }
}

#define CLAMP(v, low, high)						\
    do									\
    {									\
	if (v < (low))							\
	    v = (low);							\
	if (v > (high))							\
	    v = (high);							\
    } while (0)

#define PDF_SEPARABLE_BLEND_MODE(name)					\
    static void								\
    combine_ ## name ## _u (pixman_implementation_t *imp,		\
			    pixman_op_t              op,		\
                            uint32_t *               dest,		\
			    const uint32_t *         src,		\
			    const uint32_t *         mask,		\
			    int                      width)		\
    {									\
	int i;								\
	for (i = 0; i < width; ++i)					\
	{								\
	    uint32_t s = combine_mask (src, mask, i);			\
	    uint32_t d = *(dest + i);					\
	    uint8_t sa = ALPHA_8 (s);					\
	    uint8_t isa = ~sa;						\
	    uint8_t da = ALPHA_8 (d);					\
	    uint8_t ida = ~da;						\
	    uint32_t ra, rr, rg, rb;					\
	    								\
	    ra = da * 0xff + sa * 0xff - sa * da;			\
	    rr = isa * RED_8 (d) + ida * RED_8 (s);			\
	    rg = isa * GREEN_8 (d) + ida * GREEN_8 (s);			\
	    rb = isa * BLUE_8 (d) + ida * BLUE_8 (s);			\
									\
	    rr += blend_ ## name (RED_8 (d), da, RED_8 (s), sa);	\
	    rg += blend_ ## name (GREEN_8 (d), da, GREEN_8 (s), sa);    \
	    rb += blend_ ## name (BLUE_8 (d), da, BLUE_8 (s), sa);	\
                                                                        \
	    CLAMP (ra, 0, 255 * 255);				        \
	    CLAMP (rr, 0, 255 * 255);				        \
	    CLAMP (rg, 0, 255 * 255);				        \
	    CLAMP (rb, 0, 255 * 255);				        \
									\
	    ra = DIV_ONE_UN8 (ra);					\
	    rr = DIV_ONE_UN8 (rr);					\
	    rg = DIV_ONE_UN8 (rg);					\
	    rb = DIV_ONE_UN8 (rb);					\
									\
	    *(dest + i) = ra << 24 | rr << 16 | rg << 8 | rb;		\
	}								\
    }									\
    									\
    static void								\
    combine_ ## name ## _ca (pixman_implementation_t *imp,		\
			     pixman_op_t              op,		\
                             uint32_t *               dest,		\
			     const uint32_t *         src,		\
			     const uint32_t *         mask,		\
			     int                      width)		\
    {									\
	int i;								\
	for (i = 0; i < width; ++i)					\
	{								\
	    uint32_t m = *(mask + i);					\
	    uint32_t s = *(src + i);					\
	    uint32_t d = *(dest + i);					\
	    uint8_t da = ALPHA_8 (d);					\
	    uint8_t ida = ~da;						\
	    uint32_t ra, rr, rg, rb;					\
	    uint8_t ira, iga, iba;					\
	    								\
	    combine_mask_ca (&s, &m);					\
	    								\
	    ira = ~RED_8 (m);						\
	    iga = ~GREEN_8 (m);						\
	    iba = ~BLUE_8 (m);						\
									\
	    ra = da * 0xff + ALPHA_8 (s) * 0xff - ALPHA_8 (s) * da;	\
	    rr = ira * RED_8 (d) + ida * RED_8 (s);			\
	    rg = iga * GREEN_8 (d) + ida * GREEN_8 (s);			\
	    rb = iba * BLUE_8 (d) + ida * BLUE_8 (s);			\
									\
	    rr += blend_ ## name (RED_8 (d), da, RED_8 (s), RED_8 (m));	\
	    rg += blend_ ## name (GREEN_8 (d), da, GREEN_8 (s), GREEN_8 (m)); \
	    rb += blend_ ## name (BLUE_8 (d), da, BLUE_8 (s), BLUE_8 (m)); \
									\
	    CLAMP (ra, 0, 255 * 255);				        \
	    CLAMP (rr, 0, 255 * 255);				        \
	    CLAMP (rg, 0, 255 * 255);				        \
	    CLAMP (rb, 0, 255 * 255);				        \
									\
	    ra = DIV_ONE_UN8 (ra);					\
	    rr = DIV_ONE_UN8 (rr);					\
	    rg = DIV_ONE_UN8 (rg);					\
	    rb = DIV_ONE_UN8 (rb);					\
									\
	    *(dest + i) = ra << 24 | rr << 16 | rg << 8 | rb;		\
	}								\
    }

/*
 * Screen
 *
 *      ad * as * B(d/ad, s/as)
 *    = ad * as * (d/ad + s/as - s/as * d/ad)
 *    = ad * s + as * d - s * d
 */
static inline int32_t
blend_screen (int32_t d, int32_t ad, int32_t s, int32_t as)
{
    return s * ad + d * as - s * d;
}

PDF_SEPARABLE_BLEND_MODE (screen)

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
static inline int32_t
blend_overlay (int32_t d, int32_t ad, int32_t s, int32_t as)
{
    uint32_t r;

    if (2 * d < ad)
	r = 2 * s * d;
    else
	r = as * ad - 2 * (ad - d) * (as - s);

    return r;
}

PDF_SEPARABLE_BLEND_MODE (overlay)

/*
 * Darken
 *
 *     ad * as * B(d/ad, s/as)
 *   = ad * as * MIN(d/ad, s/as)
 *   = MIN (as * d, ad * s)
 */
static inline int32_t
blend_darken (int32_t d, int32_t ad, int32_t s, int32_t as)
{
    s = ad * s;
    d = as * d;

    return s > d ? d : s;
}

PDF_SEPARABLE_BLEND_MODE (darken)

/*
 * Lighten
 *
 *     ad * as * B(d/ad, s/as)
 *   = ad * as * MAX(d/ad, s/as)
 *   = MAX (as * d, ad * s)
 */
static inline int32_t
blend_lighten (int32_t d, int32_t ad, int32_t s, int32_t as)
{
    s = ad * s;
    d = as * d;
    
    return s > d ? s : d;
}

PDF_SEPARABLE_BLEND_MODE (lighten)

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
static inline int32_t
blend_hard_light (int32_t d, int32_t ad, int32_t s, int32_t as)
{
    if (2 * s < as)
	return 2 * s * d;
    else
	return as * ad - 2 * (ad - d) * (as - s);
}

PDF_SEPARABLE_BLEND_MODE (hard_light)

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
static inline int32_t
blend_difference (int32_t d, int32_t ad, int32_t s, int32_t as)
{
    int32_t das = d * as;
    int32_t sad = s * ad;

    if (sad < das)
	return das - sad;
    else
	return sad - das;
}

PDF_SEPARABLE_BLEND_MODE (difference)

/*
 * Exclusion
 *
 *     ad * as * B(s/as, d/ad)
 *   = ad * as * (d/ad + s/as - 2 * d/ad * s/as)
 *   = as * d + ad * s - 2 * s * d
 */

/* This can be made faster by writing it directly and not using
 * PDF_SEPARABLE_BLEND_MODE, but that's a performance optimization */

static inline int32_t
blend_exclusion (int32_t d, int32_t ad, int32_t s, int32_t as)
{
    return s * ad + d * as - 2 * d * s;
}

PDF_SEPARABLE_BLEND_MODE (exclusion)

#undef PDF_SEPARABLE_BLEND_MODE

/* Component alpha combiners */

static void
combine_clear_ca (pixman_implementation_t *imp,
                  pixman_op_t              op,
                  uint32_t *                dest,
                  const uint32_t *          src,
                  const uint32_t *          mask,
                  int                      width)
{
    memset (dest, 0, width * sizeof(uint32_t));
}

static void
combine_src_ca (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint32_t *                dest,
                const uint32_t *          src,
                const uint32_t *          mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = *(src + i);
	uint32_t m = *(mask + i);

	combine_mask_value_ca (&s, &m);

	*(dest + i) = s;
    }
}

static void
combine_over_ca (pixman_implementation_t *imp,
                 pixman_op_t              op,
                 uint32_t *                dest,
                 const uint32_t *          src,
                 const uint32_t *          mask,
                 int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = *(src + i);
	uint32_t m = *(mask + i);
	uint32_t a;

	combine_mask_ca (&s, &m);

	a = ~m;
	if (a)
	{
	    uint32_t d = *(dest + i);
	    UN8x4_MUL_UN8x4_ADD_UN8x4 (d, a, s);
	    s = d;
	}

	*(dest + i) = s;
    }
}

static void
combine_over_reverse_ca (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint32_t *                dest,
                         const uint32_t *          src,
                         const uint32_t *          mask,
                         int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t d = *(dest + i);
	uint32_t a = ~d >> A_SHIFT;

	if (a)
	{
	    uint32_t s = *(src + i);
	    uint32_t m = *(mask + i);

	    UN8x4_MUL_UN8x4 (s, m);
	    UN8x4_MUL_UN8_ADD_UN8x4 (s, a, d);

	    *(dest + i) = s;
	}
    }
}

static void
combine_in_ca (pixman_implementation_t *imp,
               pixman_op_t              op,
               uint32_t *                dest,
               const uint32_t *          src,
               const uint32_t *          mask,
               int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t d = *(dest + i);
	uint16_t a = d >> A_SHIFT;
	uint32_t s = 0;

	if (a)
	{
	    uint32_t m = *(mask + i);

	    s = *(src + i);
	    combine_mask_value_ca (&s, &m);

	    if (a != MASK)
		UN8x4_MUL_UN8 (s, a);
	}

	*(dest + i) = s;
    }
}

static void
combine_in_reverse_ca (pixman_implementation_t *imp,
                       pixman_op_t              op,
                       uint32_t *                dest,
                       const uint32_t *          src,
                       const uint32_t *          mask,
                       int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = *(src + i);
	uint32_t m = *(mask + i);
	uint32_t a;

	combine_mask_alpha_ca (&s, &m);

	a = m;
	if (a != ~0)
	{
	    uint32_t d = 0;

	    if (a)
	    {
		d = *(dest + i);
		UN8x4_MUL_UN8x4 (d, a);
	    }

	    *(dest + i) = d;
	}
    }
}

static void
combine_out_ca (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint32_t *                dest,
                const uint32_t *          src,
                const uint32_t *          mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t d = *(dest + i);
	uint16_t a = ~d >> A_SHIFT;
	uint32_t s = 0;

	if (a)
	{
	    uint32_t m = *(mask + i);

	    s = *(src + i);
	    combine_mask_value_ca (&s, &m);

	    if (a != MASK)
		UN8x4_MUL_UN8 (s, a);
	}

	*(dest + i) = s;
    }
}

static void
combine_out_reverse_ca (pixman_implementation_t *imp,
                        pixman_op_t              op,
                        uint32_t *                dest,
                        const uint32_t *          src,
                        const uint32_t *          mask,
                        int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = *(src + i);
	uint32_t m = *(mask + i);
	uint32_t a;

	combine_mask_alpha_ca (&s, &m);

	a = ~m;
	if (a != ~0)
	{
	    uint32_t d = 0;

	    if (a)
	    {
		d = *(dest + i);
		UN8x4_MUL_UN8x4 (d, a);
	    }

	    *(dest + i) = d;
	}
    }
}

static void
combine_atop_ca (pixman_implementation_t *imp,
                 pixman_op_t              op,
                 uint32_t *                dest,
                 const uint32_t *          src,
                 const uint32_t *          mask,
                 int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t d = *(dest + i);
	uint32_t s = *(src + i);
	uint32_t m = *(mask + i);
	uint32_t ad;
	uint16_t as = d >> A_SHIFT;

	combine_mask_ca (&s, &m);

	ad = ~m;

	UN8x4_MUL_UN8x4_ADD_UN8x4_MUL_UN8 (d, ad, s, as);

	*(dest + i) = d;
    }
}

static void
combine_atop_reverse_ca (pixman_implementation_t *imp,
                         pixman_op_t              op,
                         uint32_t *                dest,
                         const uint32_t *          src,
                         const uint32_t *          mask,
                         int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t d = *(dest + i);
	uint32_t s = *(src + i);
	uint32_t m = *(mask + i);
	uint32_t ad;
	uint16_t as = ~d >> A_SHIFT;

	combine_mask_ca (&s, &m);

	ad = m;

	UN8x4_MUL_UN8x4_ADD_UN8x4_MUL_UN8 (d, ad, s, as);

	*(dest + i) = d;
    }
}

static void
combine_xor_ca (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint32_t *                dest,
                const uint32_t *          src,
                const uint32_t *          mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t d = *(dest + i);
	uint32_t s = *(src + i);
	uint32_t m = *(mask + i);
	uint32_t ad;
	uint16_t as = ~d >> A_SHIFT;

	combine_mask_ca (&s, &m);

	ad = ~m;

	UN8x4_MUL_UN8x4_ADD_UN8x4_MUL_UN8 (d, ad, s, as);

	*(dest + i) = d;
    }
}

static void
combine_add_ca (pixman_implementation_t *imp,
                pixman_op_t              op,
                uint32_t *                dest,
                const uint32_t *          src,
                const uint32_t *          mask,
                int                      width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t s = *(src + i);
	uint32_t m = *(mask + i);
	uint32_t d = *(dest + i);

	combine_mask_value_ca (&s, &m);

	UN8x4_ADD_UN8x4 (d, s);

	*(dest + i) = d;
    }
}

void
_pixman_setup_combiner_functions_32 (pixman_implementation_t *imp)
{
    /* Unified alpha */
    imp->combine_32[PIXMAN_OP_CLEAR] = combine_clear;
    imp->combine_32[PIXMAN_OP_SRC] = combine_src_u;
    imp->combine_32[PIXMAN_OP_DST] = combine_dst;
    imp->combine_32[PIXMAN_OP_OVER] = combine_over_u;
    imp->combine_32[PIXMAN_OP_OVER_REVERSE] = combine_over_reverse_u;
    imp->combine_32[PIXMAN_OP_IN] = combine_in_u;
    imp->combine_32[PIXMAN_OP_IN_REVERSE] = combine_in_reverse_u;
    imp->combine_32[PIXMAN_OP_OUT] = combine_out_u;
    imp->combine_32[PIXMAN_OP_OUT_REVERSE] = combine_out_reverse_u;
    imp->combine_32[PIXMAN_OP_ATOP] = combine_atop_u;
    imp->combine_32[PIXMAN_OP_ATOP_REVERSE] = combine_atop_reverse_u;
    imp->combine_32[PIXMAN_OP_XOR] = combine_xor_u;
    imp->combine_32[PIXMAN_OP_ADD] = combine_add_u;

    imp->combine_32[PIXMAN_OP_MULTIPLY] = combine_multiply_u;
    imp->combine_32[PIXMAN_OP_SCREEN] = combine_screen_u;
    imp->combine_32[PIXMAN_OP_OVERLAY] = combine_overlay_u;
    imp->combine_32[PIXMAN_OP_DARKEN] = combine_darken_u;
    imp->combine_32[PIXMAN_OP_LIGHTEN] = combine_lighten_u;
    imp->combine_32[PIXMAN_OP_HARD_LIGHT] = combine_hard_light_u;
    imp->combine_32[PIXMAN_OP_DIFFERENCE] = combine_difference_u;
    imp->combine_32[PIXMAN_OP_EXCLUSION] = combine_exclusion_u;

    /* Component alpha combiners */
    imp->combine_32_ca[PIXMAN_OP_CLEAR] = combine_clear_ca;
    imp->combine_32_ca[PIXMAN_OP_SRC] = combine_src_ca;
    /* dest */
    imp->combine_32_ca[PIXMAN_OP_OVER] = combine_over_ca;
    imp->combine_32_ca[PIXMAN_OP_OVER_REVERSE] = combine_over_reverse_ca;
    imp->combine_32_ca[PIXMAN_OP_IN] = combine_in_ca;
    imp->combine_32_ca[PIXMAN_OP_IN_REVERSE] = combine_in_reverse_ca;
    imp->combine_32_ca[PIXMAN_OP_OUT] = combine_out_ca;
    imp->combine_32_ca[PIXMAN_OP_OUT_REVERSE] = combine_out_reverse_ca;
    imp->combine_32_ca[PIXMAN_OP_ATOP] = combine_atop_ca;
    imp->combine_32_ca[PIXMAN_OP_ATOP_REVERSE] = combine_atop_reverse_ca;
    imp->combine_32_ca[PIXMAN_OP_XOR] = combine_xor_ca;
    imp->combine_32_ca[PIXMAN_OP_ADD] = combine_add_ca;

    imp->combine_32_ca[PIXMAN_OP_MULTIPLY] = combine_multiply_ca;
    imp->combine_32_ca[PIXMAN_OP_SCREEN] = combine_screen_ca;
    imp->combine_32_ca[PIXMAN_OP_OVERLAY] = combine_overlay_ca;
    imp->combine_32_ca[PIXMAN_OP_DARKEN] = combine_darken_ca;
    imp->combine_32_ca[PIXMAN_OP_LIGHTEN] = combine_lighten_ca;
    imp->combine_32_ca[PIXMAN_OP_HARD_LIGHT] = combine_hard_light_ca;
    imp->combine_32_ca[PIXMAN_OP_DIFFERENCE] = combine_difference_ca;
    imp->combine_32_ca[PIXMAN_OP_EXCLUSION] = combine_exclusion_ca;
}
