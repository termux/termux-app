/*
 * Copyright © 2008 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

/*
 * Matrix interfaces
 */

#ifdef HAVE_CONFIG_H
#include <pixman-config.h>
#endif

#include <math.h>
#include <string.h>
#include "pixman-private.h"

#define F(x)    pixman_int_to_fixed (x)

static force_inline int
count_leading_zeros (uint32_t x)
{
#ifdef HAVE_BUILTIN_CLZ
    return __builtin_clz (x);
#else
    int n = 0;
    while (x)
    {
        n++;
        x >>= 1;
    }
    return 32 - n;
#endif
}

/*
 * Large signed/unsigned integer division with rounding for the platforms with
 * only 64-bit integer data type supported (no 128-bit data type).
 *
 * Arguments:
 *     hi, lo - high and low 64-bit parts of the dividend
 *     div    - 48-bit divisor
 *
 * Returns: lowest 64 bits of the result as a return value and highest 64
 *          bits of the result to "result_hi" pointer
 */

/* grade-school unsigned division (128-bit by 48-bit) with rounding to nearest */
static force_inline uint64_t
rounded_udiv_128_by_48 (uint64_t  hi,
                        uint64_t  lo,
                        uint64_t  div,
                        uint64_t *result_hi)
{
    uint64_t tmp, remainder, result_lo;
    assert(div < ((uint64_t)1 << 48));

    remainder = hi % div;
    *result_hi = hi / div;

    tmp = (remainder << 16) + (lo >> 48);
    result_lo = tmp / div;
    remainder = tmp % div;

    tmp = (remainder << 16) + ((lo >> 32) & 0xFFFF);
    result_lo = (result_lo << 16) + (tmp / div);
    remainder = tmp % div;

    tmp = (remainder << 16) + ((lo >> 16) & 0xFFFF);
    result_lo = (result_lo << 16) + (tmp / div);
    remainder = tmp % div;

    tmp = (remainder << 16) + (lo & 0xFFFF);
    result_lo = (result_lo << 16) + (tmp / div);
    remainder = tmp % div;

    /* round to nearest */
    if (remainder * 2 >= div && ++result_lo == 0)
        *result_hi += 1;

    return result_lo;
}

/* signed division (128-bit by 49-bit) with rounding to nearest */
static inline int64_t
rounded_sdiv_128_by_49 (int64_t   hi,
                        uint64_t  lo,
                        int64_t   div,
                        int64_t  *signed_result_hi)
{
    uint64_t result_lo, result_hi;
    int sign = 0;
    if (div < 0)
    {
        div = -div;
        sign ^= 1;
    }
    if (hi < 0)
    {
        if (lo != 0)
            hi++;
        hi = -hi;
        lo = -lo;
        sign ^= 1;
    }
    result_lo = rounded_udiv_128_by_48 (hi, lo, div, &result_hi);
    if (sign)
    {
        if (result_lo != 0)
            result_hi++;
        result_hi = -result_hi;
        result_lo = -result_lo;
    }
    if (signed_result_hi)
    {
        *signed_result_hi = result_hi;
    }
    return result_lo;
}

/*
 * Multiply 64.16 fixed point value by (2^scalebits) and convert
 * to 128-bit integer.
 */
static force_inline void
fixed_64_16_to_int128 (int64_t  hi,
                       int64_t  lo,
                       int64_t *rhi,
                       int64_t *rlo,
                       int      scalebits)
{
    /* separate integer and fractional parts */
    hi += lo >> 16;
    lo &= 0xFFFF;

    if (scalebits <= 0)
    {
        *rlo = hi >> (-scalebits);
        *rhi = *rlo >> 63;
    }
    else
    {
        *rhi = hi >> (64 - scalebits);
        *rlo = (uint64_t)hi << scalebits;
        if (scalebits < 16)
            *rlo += lo >> (16 - scalebits);
        else
            *rlo += lo << (scalebits - 16);
    }
}

/*
 * Convert 112.16 fixed point value to 48.16 with clamping for the out
 * of range values.
 */
static force_inline pixman_fixed_48_16_t
fixed_112_16_to_fixed_48_16 (int64_t hi, int64_t lo, pixman_bool_t *clampflag)
{
    if ((lo >> 63) != hi)
    {
        *clampflag = TRUE;
        return hi >= 0 ? INT64_MAX : INT64_MIN;
    }
    else
    {
        return lo;
    }
}

/*
 * Transform a point with 31.16 fixed point coordinates from the destination
 * space to a point with 48.16 fixed point coordinates in the source space.
 * No overflows are possible for affine transformations and the results are
 * accurate including the least significant bit. Projective transformations
 * may overflow, in this case the results are just clamped to return maximum
 * or minimum 48.16 values (so that the caller can at least handle the NONE
 * and PAD repeats correctly) and the return value is FALSE to indicate that
 * such clamping has happened.
 */
PIXMAN_EXPORT pixman_bool_t
pixman_transform_point_31_16 (const pixman_transform_t    *t,
                              const pixman_vector_48_16_t *v,
                              pixman_vector_48_16_t       *result)
{
    pixman_bool_t clampflag = FALSE;
    int i;
    int64_t tmp[3][2], divint;
    uint16_t divfrac;

    /* input vector values must have no more than 31 bits (including sign)
     * in the integer part */
    assert (v->v[0] <   ((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[0] >= -((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[1] <   ((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[1] >= -((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[2] <   ((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[2] >= -((pixman_fixed_48_16_t)1 << (30 + 16)));

    for (i = 0; i < 3; i++)
    {
        tmp[i][0] = (int64_t)t->matrix[i][0] * (v->v[0] >> 16);
        tmp[i][1] = (int64_t)t->matrix[i][0] * (v->v[0] & 0xFFFF);
        tmp[i][0] += (int64_t)t->matrix[i][1] * (v->v[1] >> 16);
        tmp[i][1] += (int64_t)t->matrix[i][1] * (v->v[1] & 0xFFFF);
        tmp[i][0] += (int64_t)t->matrix[i][2] * (v->v[2] >> 16);
        tmp[i][1] += (int64_t)t->matrix[i][2] * (v->v[2] & 0xFFFF);
    }

    /*
     * separate 64-bit integer and 16-bit fractional parts for the divisor,
     * which is also scaled by 65536 after fixed point multiplication.
     */
    divint  = tmp[2][0] + (tmp[2][1] >> 16);
    divfrac = tmp[2][1] & 0xFFFF;

    if (divint == pixman_fixed_1 && divfrac == 0)
    {
        /*
         * this is a simple affine transformation
         */
        result->v[0] = tmp[0][0] + ((tmp[0][1] + 0x8000) >> 16);
        result->v[1] = tmp[1][0] + ((tmp[1][1] + 0x8000) >> 16);
        result->v[2] = pixman_fixed_1;
    }
    else if (divint == 0 && divfrac == 0)
    {
        /*
         * handle zero divisor (if the values are non-zero, set the
         * results to maximum positive or minimum negative)
         */
        clampflag = TRUE;

        result->v[0] = tmp[0][0] + ((tmp[0][1] + 0x8000) >> 16);
        result->v[1] = tmp[1][0] + ((tmp[1][1] + 0x8000) >> 16);

        if (result->v[0] > 0)
            result->v[0] = INT64_MAX;
        else if (result->v[0] < 0)
            result->v[0] = INT64_MIN;

        if (result->v[1] > 0)
            result->v[1] = INT64_MAX;
        else if (result->v[1] < 0)
            result->v[1] = INT64_MIN;
    }
    else
    {
        /*
         * projective transformation, analyze the top 32 bits of the divisor
         */
        int32_t hi32divbits = divint >> 32;
        if (hi32divbits < 0)
            hi32divbits = ~hi32divbits;

        if (hi32divbits == 0)
        {
            /* the divisor is small, we can actually keep all the bits */
            int64_t hi, rhi, lo, rlo;
            int64_t div = ((uint64_t)divint << 16) + divfrac;

            fixed_64_16_to_int128 (tmp[0][0], tmp[0][1], &hi, &lo, 32);
            rlo = rounded_sdiv_128_by_49 (hi, lo, div, &rhi);
            result->v[0] = fixed_112_16_to_fixed_48_16 (rhi, rlo, &clampflag);

            fixed_64_16_to_int128 (tmp[1][0], tmp[1][1], &hi, &lo, 32);
            rlo = rounded_sdiv_128_by_49 (hi, lo, div, &rhi);
            result->v[1] = fixed_112_16_to_fixed_48_16 (rhi, rlo, &clampflag);
        }
        else
        {
            /* the divisor needs to be reduced to 48 bits */
            int64_t hi, rhi, lo, rlo, div;
            int shift = 32 - count_leading_zeros (hi32divbits);
            fixed_64_16_to_int128 (divint, divfrac, &hi, &div, 16 - shift);

            fixed_64_16_to_int128 (tmp[0][0], tmp[0][1], &hi, &lo, 32 - shift);
            rlo = rounded_sdiv_128_by_49 (hi, lo, div, &rhi);
            result->v[0] = fixed_112_16_to_fixed_48_16 (rhi, rlo, &clampflag);

            fixed_64_16_to_int128 (tmp[1][0], tmp[1][1], &hi, &lo, 32 - shift);
            rlo = rounded_sdiv_128_by_49 (hi, lo, div, &rhi);
            result->v[1] = fixed_112_16_to_fixed_48_16 (rhi, rlo, &clampflag);
        }
    }
    result->v[2] = pixman_fixed_1;
    return !clampflag;
}

PIXMAN_EXPORT void
pixman_transform_point_31_16_affine (const pixman_transform_t    *t,
                                     const pixman_vector_48_16_t *v,
                                     pixman_vector_48_16_t       *result)
{
    int64_t hi0, lo0, hi1, lo1;

    /* input vector values must have no more than 31 bits (including sign)
     * in the integer part */
    assert (v->v[0] <   ((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[0] >= -((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[1] <   ((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[1] >= -((pixman_fixed_48_16_t)1 << (30 + 16)));

    hi0  = (int64_t)t->matrix[0][0] * (v->v[0] >> 16);
    lo0  = (int64_t)t->matrix[0][0] * (v->v[0] & 0xFFFF);
    hi0 += (int64_t)t->matrix[0][1] * (v->v[1] >> 16);
    lo0 += (int64_t)t->matrix[0][1] * (v->v[1] & 0xFFFF);
    hi0 += (int64_t)t->matrix[0][2];

    hi1  = (int64_t)t->matrix[1][0] * (v->v[0] >> 16);
    lo1  = (int64_t)t->matrix[1][0] * (v->v[0] & 0xFFFF);
    hi1 += (int64_t)t->matrix[1][1] * (v->v[1] >> 16);
    lo1 += (int64_t)t->matrix[1][1] * (v->v[1] & 0xFFFF);
    hi1 += (int64_t)t->matrix[1][2];

    result->v[0] = hi0 + ((lo0 + 0x8000) >> 16);
    result->v[1] = hi1 + ((lo1 + 0x8000) >> 16);
    result->v[2] = pixman_fixed_1;
}

PIXMAN_EXPORT void
pixman_transform_point_31_16_3d (const pixman_transform_t    *t,
                                 const pixman_vector_48_16_t *v,
                                 pixman_vector_48_16_t       *result)
{
    int i;
    int64_t tmp[3][2];

    /* input vector values must have no more than 31 bits (including sign)
     * in the integer part */
    assert (v->v[0] <   ((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[0] >= -((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[1] <   ((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[1] >= -((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[2] <   ((pixman_fixed_48_16_t)1 << (30 + 16)));
    assert (v->v[2] >= -((pixman_fixed_48_16_t)1 << (30 + 16)));

    for (i = 0; i < 3; i++)
    {
        tmp[i][0] = (int64_t)t->matrix[i][0] * (v->v[0] >> 16);
        tmp[i][1] = (int64_t)t->matrix[i][0] * (v->v[0] & 0xFFFF);
        tmp[i][0] += (int64_t)t->matrix[i][1] * (v->v[1] >> 16);
        tmp[i][1] += (int64_t)t->matrix[i][1] * (v->v[1] & 0xFFFF);
        tmp[i][0] += (int64_t)t->matrix[i][2] * (v->v[2] >> 16);
        tmp[i][1] += (int64_t)t->matrix[i][2] * (v->v[2] & 0xFFFF);
    }

    result->v[0] = tmp[0][0] + ((tmp[0][1] + 0x8000) >> 16);
    result->v[1] = tmp[1][0] + ((tmp[1][1] + 0x8000) >> 16);
    result->v[2] = tmp[2][0] + ((tmp[2][1] + 0x8000) >> 16);
}

PIXMAN_EXPORT void
pixman_transform_init_identity (struct pixman_transform *matrix)
{
    int i;

    memset (matrix, '\0', sizeof (struct pixman_transform));
    for (i = 0; i < 3; i++)
	matrix->matrix[i][i] = F (1);
}

typedef pixman_fixed_32_32_t pixman_fixed_34_30_t;

PIXMAN_EXPORT pixman_bool_t
pixman_transform_point_3d (const struct pixman_transform *transform,
                           struct pixman_vector *         vector)
{
    pixman_vector_48_16_t tmp;
    tmp.v[0] = vector->vector[0];
    tmp.v[1] = vector->vector[1];
    tmp.v[2] = vector->vector[2];

    pixman_transform_point_31_16_3d (transform, &tmp, &tmp);

    vector->vector[0] = tmp.v[0];
    vector->vector[1] = tmp.v[1];
    vector->vector[2] = tmp.v[2];

    return vector->vector[0] == tmp.v[0] &&
           vector->vector[1] == tmp.v[1] &&
           vector->vector[2] == tmp.v[2];
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_point (const struct pixman_transform *transform,
                        struct pixman_vector *         vector)
{
    pixman_vector_48_16_t tmp;
    tmp.v[0] = vector->vector[0];
    tmp.v[1] = vector->vector[1];
    tmp.v[2] = vector->vector[2];

    if (!pixman_transform_point_31_16 (transform, &tmp, &tmp))
        return FALSE;

    vector->vector[0] = tmp.v[0];
    vector->vector[1] = tmp.v[1];
    vector->vector[2] = tmp.v[2];

    return vector->vector[0] == tmp.v[0] &&
           vector->vector[1] == tmp.v[1] &&
           vector->vector[2] == tmp.v[2];
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_multiply (struct pixman_transform *      dst,
                           const struct pixman_transform *l,
                           const struct pixman_transform *r)
{
    struct pixman_transform d;
    int dx, dy;
    int o;

    for (dy = 0; dy < 3; dy++)
    {
	for (dx = 0; dx < 3; dx++)
	{
	    pixman_fixed_48_16_t v;
	    pixman_fixed_32_32_t partial;
	    
	    v = 0;
	    for (o = 0; o < 3; o++)
	    {
		partial =
		    (pixman_fixed_32_32_t) l->matrix[dy][o] *
		    (pixman_fixed_32_32_t) r->matrix[o][dx];

		v += (partial + 0x8000) >> 16;
	    }

	    if (v > pixman_max_fixed_48_16 || v < pixman_min_fixed_48_16)
		return FALSE;
	    
	    d.matrix[dy][dx] = (pixman_fixed_t) v;
	}
    }

    *dst = d;
    return TRUE;
}

PIXMAN_EXPORT void
pixman_transform_init_scale (struct pixman_transform *t,
                             pixman_fixed_t           sx,
                             pixman_fixed_t           sy)
{
    memset (t, '\0', sizeof (struct pixman_transform));

    t->matrix[0][0] = sx;
    t->matrix[1][1] = sy;
    t->matrix[2][2] = F (1);
}

static pixman_fixed_t
fixed_inverse (pixman_fixed_t x)
{
    return (pixman_fixed_t) ((((pixman_fixed_48_16_t) F (1)) * F (1)) / x);
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_scale (struct pixman_transform *forward,
                        struct pixman_transform *reverse,
                        pixman_fixed_t           sx,
                        pixman_fixed_t           sy)
{
    struct pixman_transform t;

    if (sx == 0 || sy == 0)
	return FALSE;

    if (forward)
    {
	pixman_transform_init_scale (&t, sx, sy);
	if (!pixman_transform_multiply (forward, &t, forward))
	    return FALSE;
    }
    
    if (reverse)
    {
	pixman_transform_init_scale (&t, fixed_inverse (sx),
	                             fixed_inverse (sy));
	if (!pixman_transform_multiply (reverse, reverse, &t))
	    return FALSE;
    }
    
    return TRUE;
}

PIXMAN_EXPORT void
pixman_transform_init_rotate (struct pixman_transform *t,
                              pixman_fixed_t           c,
                              pixman_fixed_t           s)
{
    memset (t, '\0', sizeof (struct pixman_transform));

    t->matrix[0][0] = c;
    t->matrix[0][1] = -s;
    t->matrix[1][0] = s;
    t->matrix[1][1] = c;
    t->matrix[2][2] = F (1);
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_rotate (struct pixman_transform *forward,
                         struct pixman_transform *reverse,
                         pixman_fixed_t           c,
                         pixman_fixed_t           s)
{
    struct pixman_transform t;

    if (forward)
    {
	pixman_transform_init_rotate (&t, c, s);
	if (!pixman_transform_multiply (forward, &t, forward))
	    return FALSE;
    }

    if (reverse)
    {
	pixman_transform_init_rotate (&t, c, -s);
	if (!pixman_transform_multiply (reverse, reverse, &t))
	    return FALSE;
    }
    
    return TRUE;
}

PIXMAN_EXPORT void
pixman_transform_init_translate (struct pixman_transform *t,
                                 pixman_fixed_t           tx,
                                 pixman_fixed_t           ty)
{
    memset (t, '\0', sizeof (struct pixman_transform));

    t->matrix[0][0] = F (1);
    t->matrix[0][2] = tx;
    t->matrix[1][1] = F (1);
    t->matrix[1][2] = ty;
    t->matrix[2][2] = F (1);
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_translate (struct pixman_transform *forward,
                            struct pixman_transform *reverse,
                            pixman_fixed_t           tx,
                            pixman_fixed_t           ty)
{
    struct pixman_transform t;

    if (forward)
    {
	pixman_transform_init_translate (&t, tx, ty);

	if (!pixman_transform_multiply (forward, &t, forward))
	    return FALSE;
    }

    if (reverse)
    {
	pixman_transform_init_translate (&t, -tx, -ty);

	if (!pixman_transform_multiply (reverse, reverse, &t))
	    return FALSE;
    }
    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_bounds (const struct pixman_transform *matrix,
                         struct pixman_box16 *          b)

{
    struct pixman_vector v[4];
    int i;
    int x1, y1, x2, y2;

    v[0].vector[0] = F (b->x1);
    v[0].vector[1] = F (b->y1);
    v[0].vector[2] = F (1);

    v[1].vector[0] = F (b->x2);
    v[1].vector[1] = F (b->y1);
    v[1].vector[2] = F (1);

    v[2].vector[0] = F (b->x2);
    v[2].vector[1] = F (b->y2);
    v[2].vector[2] = F (1);

    v[3].vector[0] = F (b->x1);
    v[3].vector[1] = F (b->y2);
    v[3].vector[2] = F (1);

    for (i = 0; i < 4; i++)
    {
	if (!pixman_transform_point (matrix, &v[i]))
	    return FALSE;

	x1 = pixman_fixed_to_int (v[i].vector[0]);
	y1 = pixman_fixed_to_int (v[i].vector[1]);
	x2 = pixman_fixed_to_int (pixman_fixed_ceil (v[i].vector[0]));
	y2 = pixman_fixed_to_int (pixman_fixed_ceil (v[i].vector[1]));

	if (i == 0)
	{
	    b->x1 = x1;
	    b->y1 = y1;
	    b->x2 = x2;
	    b->y2 = y2;
	}
	else
	{
	    if (x1 < b->x1) b->x1 = x1;
	    if (y1 < b->y1) b->y1 = y1;
	    if (x2 > b->x2) b->x2 = x2;
	    if (y2 > b->y2) b->y2 = y2;
	}
    }

    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_invert (struct pixman_transform *      dst,
                         const struct pixman_transform *src)
{
    struct pixman_f_transform m;

    pixman_f_transform_from_pixman_transform (&m, src);

    if (!pixman_f_transform_invert (&m, &m))
	return FALSE;

    if (!pixman_transform_from_pixman_f_transform (dst, &m))
	return FALSE;

    return TRUE;
}

static pixman_bool_t
within_epsilon (pixman_fixed_t a,
                pixman_fixed_t b,
                pixman_fixed_t epsilon)
{
    pixman_fixed_t t = a - b;

    if (t < 0)
	t = -t;

    return t <= epsilon;
}

#define EPSILON (pixman_fixed_t) (2)

#define IS_SAME(a, b) (within_epsilon (a, b, EPSILON))
#define IS_ZERO(a)    (within_epsilon (a, 0, EPSILON))
#define IS_ONE(a)     (within_epsilon (a, F (1), EPSILON))
#define IS_UNIT(a)			    \
    (within_epsilon (a, F (1), EPSILON) ||  \
     within_epsilon (a, F (-1), EPSILON) || \
     IS_ZERO (a))
#define IS_INT(a)    (IS_ZERO (pixman_fixed_frac (a)))

PIXMAN_EXPORT pixman_bool_t
pixman_transform_is_identity (const struct pixman_transform *t)
{
    return (IS_SAME (t->matrix[0][0], t->matrix[1][1]) &&
	    IS_SAME (t->matrix[0][0], t->matrix[2][2]) &&
	    !IS_ZERO (t->matrix[0][0]) &&
	    IS_ZERO (t->matrix[0][1]) &&
	    IS_ZERO (t->matrix[0][2]) &&
	    IS_ZERO (t->matrix[1][0]) &&
	    IS_ZERO (t->matrix[1][2]) &&
	    IS_ZERO (t->matrix[2][0]) &&
	    IS_ZERO (t->matrix[2][1]));
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_is_scale (const struct pixman_transform *t)
{
    return (!IS_ZERO (t->matrix[0][0]) &&
            IS_ZERO (t->matrix[0][1]) &&
            IS_ZERO (t->matrix[0][2]) &&

            IS_ZERO (t->matrix[1][0]) &&
            !IS_ZERO (t->matrix[1][1]) &&
            IS_ZERO (t->matrix[1][2]) &&

            IS_ZERO (t->matrix[2][0]) &&
            IS_ZERO (t->matrix[2][1]) &&
            !IS_ZERO (t->matrix[2][2]));
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_is_int_translate (const struct pixman_transform *t)
{
    return (IS_ONE (t->matrix[0][0]) &&
            IS_ZERO (t->matrix[0][1]) &&
            IS_INT (t->matrix[0][2]) &&

            IS_ZERO (t->matrix[1][0]) &&
            IS_ONE (t->matrix[1][1]) &&
            IS_INT (t->matrix[1][2]) &&

            IS_ZERO (t->matrix[2][0]) &&
            IS_ZERO (t->matrix[2][1]) &&
            IS_ONE (t->matrix[2][2]));
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_is_inverse (const struct pixman_transform *a,
                             const struct pixman_transform *b)
{
    struct pixman_transform t;

    if (!pixman_transform_multiply (&t, a, b))
	return FALSE;

    return pixman_transform_is_identity (&t);
}

PIXMAN_EXPORT void
pixman_f_transform_from_pixman_transform (struct pixman_f_transform *    ft,
                                          const struct pixman_transform *t)
{
    int i, j;

    for (j = 0; j < 3; j++)
    {
	for (i = 0; i < 3; i++)
	    ft->m[j][i] = pixman_fixed_to_double (t->matrix[j][i]);
    }
}

PIXMAN_EXPORT pixman_bool_t
pixman_transform_from_pixman_f_transform (struct pixman_transform *        t,
                                          const struct pixman_f_transform *ft)
{
    int i, j;

    for (j = 0; j < 3; j++)
    {
	for (i = 0; i < 3; i++)
	{
	    double d = ft->m[j][i];
	    if (d < -32767.0 || d > 32767.0)
		return FALSE;
	    d = d * 65536.0 + 0.5;
	    t->matrix[j][i] = (pixman_fixed_t) floor (d);
	}
    }
    
    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
pixman_f_transform_invert (struct pixman_f_transform *      dst,
                           const struct pixman_f_transform *src)
{
    static const int a[3] = { 2, 2, 1 };
    static const int b[3] = { 1, 0, 0 };
    pixman_f_transform_t d;
    double det;
    int i, j;

    det = 0;
    for (i = 0; i < 3; i++)
    {
	double p;
	int ai = a[i];
	int bi = b[i];
	p = src->m[i][0] * (src->m[ai][2] * src->m[bi][1] -
	                    src->m[ai][1] * src->m[bi][2]);
	if (i == 1)
	    p = -p;
	det += p;
    }
    
    if (det == 0)
	return FALSE;
    
    det = 1 / det;
    for (j = 0; j < 3; j++)
    {
	for (i = 0; i < 3; i++)
	{
	    double p;
	    int ai = a[i];
	    int aj = a[j];
	    int bi = b[i];
	    int bj = b[j];

	    p = (src->m[ai][aj] * src->m[bi][bj] -
	         src->m[ai][bj] * src->m[bi][aj]);
	    
	    if (((i + j) & 1) != 0)
		p = -p;
	    
	    d.m[j][i] = det * p;
	}
    }

    *dst = d;

    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
pixman_f_transform_point (const struct pixman_f_transform *t,
                          struct pixman_f_vector *         v)
{
    struct pixman_f_vector result;
    int i, j;
    double a;

    for (j = 0; j < 3; j++)
    {
	a = 0;
	for (i = 0; i < 3; i++)
	    a += t->m[j][i] * v->v[i];
	result.v[j] = a;
    }
    
    if (!result.v[2])
	return FALSE;

    for (j = 0; j < 2; j++)
	v->v[j] = result.v[j] / result.v[2];

    v->v[2] = 1;

    return TRUE;
}

PIXMAN_EXPORT void
pixman_f_transform_point_3d (const struct pixman_f_transform *t,
                             struct pixman_f_vector *         v)
{
    struct pixman_f_vector result;
    int i, j;
    double a;

    for (j = 0; j < 3; j++)
    {
	a = 0;
	for (i = 0; i < 3; i++)
	    a += t->m[j][i] * v->v[i];
	result.v[j] = a;
    }
    
    *v = result;
}

PIXMAN_EXPORT void
pixman_f_transform_multiply (struct pixman_f_transform *      dst,
                             const struct pixman_f_transform *l,
                             const struct pixman_f_transform *r)
{
    struct pixman_f_transform d;
    int dx, dy;
    int o;

    for (dy = 0; dy < 3; dy++)
    {
	for (dx = 0; dx < 3; dx++)
	{
	    double v = 0;
	    for (o = 0; o < 3; o++)
		v += l->m[dy][o] * r->m[o][dx];
	    d.m[dy][dx] = v;
	}
    }
    
    *dst = d;
}

PIXMAN_EXPORT void
pixman_f_transform_init_scale (struct pixman_f_transform *t,
                               double                     sx,
                               double                     sy)
{
    t->m[0][0] = sx;
    t->m[0][1] = 0;
    t->m[0][2] = 0;
    t->m[1][0] = 0;
    t->m[1][1] = sy;
    t->m[1][2] = 0;
    t->m[2][0] = 0;
    t->m[2][1] = 0;
    t->m[2][2] = 1;
}

PIXMAN_EXPORT pixman_bool_t
pixman_f_transform_scale (struct pixman_f_transform *forward,
                          struct pixman_f_transform *reverse,
                          double                     sx,
                          double                     sy)
{
    struct pixman_f_transform t;

    if (sx == 0 || sy == 0)
	return FALSE;

    if (forward)
    {
	pixman_f_transform_init_scale (&t, sx, sy);
	pixman_f_transform_multiply (forward, &t, forward);
    }
    
    if (reverse)
    {
	pixman_f_transform_init_scale (&t, 1 / sx, 1 / sy);
	pixman_f_transform_multiply (reverse, reverse, &t);
    }
    
    return TRUE;
}

PIXMAN_EXPORT void
pixman_f_transform_init_rotate (struct pixman_f_transform *t,
                                double                     c,
                                double                     s)
{
    t->m[0][0] = c;
    t->m[0][1] = -s;
    t->m[0][2] = 0;
    t->m[1][0] = s;
    t->m[1][1] = c;
    t->m[1][2] = 0;
    t->m[2][0] = 0;
    t->m[2][1] = 0;
    t->m[2][2] = 1;
}

PIXMAN_EXPORT pixman_bool_t
pixman_f_transform_rotate (struct pixman_f_transform *forward,
                           struct pixman_f_transform *reverse,
                           double                     c,
                           double                     s)
{
    struct pixman_f_transform t;

    if (forward)
    {
	pixman_f_transform_init_rotate (&t, c, s);
	pixman_f_transform_multiply (forward, &t, forward);
    }
    
    if (reverse)
    {
	pixman_f_transform_init_rotate (&t, c, -s);
	pixman_f_transform_multiply (reverse, reverse, &t);
    }

    return TRUE;
}

PIXMAN_EXPORT void
pixman_f_transform_init_translate (struct pixman_f_transform *t,
                                   double                     tx,
                                   double                     ty)
{
    t->m[0][0] = 1;
    t->m[0][1] = 0;
    t->m[0][2] = tx;
    t->m[1][0] = 0;
    t->m[1][1] = 1;
    t->m[1][2] = ty;
    t->m[2][0] = 0;
    t->m[2][1] = 0;
    t->m[2][2] = 1;
}

PIXMAN_EXPORT pixman_bool_t
pixman_f_transform_translate (struct pixman_f_transform *forward,
                              struct pixman_f_transform *reverse,
                              double                     tx,
                              double                     ty)
{
    struct pixman_f_transform t;

    if (forward)
    {
	pixman_f_transform_init_translate (&t, tx, ty);
	pixman_f_transform_multiply (forward, &t, forward);
    }

    if (reverse)
    {
	pixman_f_transform_init_translate (&t, -tx, -ty);
	pixman_f_transform_multiply (reverse, reverse, &t);
    }

    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
pixman_f_transform_bounds (const struct pixman_f_transform *t,
                           struct pixman_box16 *            b)
{
    struct pixman_f_vector v[4];
    int i;
    int x1, y1, x2, y2;

    v[0].v[0] = b->x1;
    v[0].v[1] = b->y1;
    v[0].v[2] = 1;
    v[1].v[0] = b->x2;
    v[1].v[1] = b->y1;
    v[1].v[2] = 1;
    v[2].v[0] = b->x2;
    v[2].v[1] = b->y2;
    v[2].v[2] = 1;
    v[3].v[0] = b->x1;
    v[3].v[1] = b->y2;
    v[3].v[2] = 1;

    for (i = 0; i < 4; i++)
    {
	if (!pixman_f_transform_point (t, &v[i]))
	    return FALSE;

	x1 = floor (v[i].v[0]);
	y1 = floor (v[i].v[1]);
	x2 = ceil (v[i].v[0]);
	y2 = ceil (v[i].v[1]);

	if (i == 0)
	{
	    b->x1 = x1;
	    b->y1 = y1;
	    b->x2 = x2;
	    b->y2 = y2;
	}
	else
	{
	    if (x1 < b->x1) b->x1 = x1;
	    if (y1 < b->y1) b->y1 = y1;
	    if (x2 > b->x2) b->x2 = x2;
	    if (y2 > b->y2) b->y2 = y2;
	}
    }

    return TRUE;
}

PIXMAN_EXPORT void
pixman_f_transform_init_identity (struct pixman_f_transform *t)
{
    int i, j;

    for (j = 0; j < 3; j++)
    {
	for (i = 0; i < 3; i++)
	    t->m[j][i] = i == j ? 1 : 0;
    }
}
