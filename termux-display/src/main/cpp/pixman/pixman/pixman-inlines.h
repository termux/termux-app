/* -*- Mode: c; c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t; -*- */
/*
 * Copyright © 2000 SuSE, Inc.
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#ifndef PIXMAN_FAST_PATH_H__
#define PIXMAN_FAST_PATH_H__

#include "pixman-private.h"

#define PIXMAN_REPEAT_COVER -1

/* Flags describing input parameters to fast path macro template.
 * Turning on some flag values may indicate that
 * "some property X is available so template can use this" or
 * "some property X should be handled by template".
 *
 * FLAG_HAVE_SOLID_MASK
 *  Input mask is solid so template should handle this.
 *
 * FLAG_HAVE_NON_SOLID_MASK
 *  Input mask is bits mask so template should handle this.
 *
 * FLAG_HAVE_SOLID_MASK and FLAG_HAVE_NON_SOLID_MASK are mutually
 * exclusive. (It's not allowed to turn both flags on)
 */
#define FLAG_NONE				(0)
#define FLAG_HAVE_SOLID_MASK			(1 <<   1)
#define FLAG_HAVE_NON_SOLID_MASK		(1 <<   2)

/* To avoid too short repeated scanline function calls, extend source
 * scanlines having width less than below constant value.
 */
#define REPEAT_NORMAL_MIN_WIDTH			64

static force_inline pixman_bool_t
repeat (pixman_repeat_t repeat, int *c, int size)
{
    if (repeat == PIXMAN_REPEAT_NONE)
    {
	if (*c < 0 || *c >= size)
	    return FALSE;
    }
    else if (repeat == PIXMAN_REPEAT_NORMAL)
    {
	while (*c >= size)
	    *c -= size;
	while (*c < 0)
	    *c += size;
    }
    else if (repeat == PIXMAN_REPEAT_PAD)
    {
	*c = CLIP (*c, 0, size - 1);
    }
    else /* REFLECT */
    {
	*c = MOD (*c, size * 2);
	if (*c >= size)
	    *c = size * 2 - *c - 1;
    }
    return TRUE;
}

static force_inline int
pixman_fixed_to_bilinear_weight (pixman_fixed_t x)
{
    return (x >> (16 - BILINEAR_INTERPOLATION_BITS)) &
	   ((1 << BILINEAR_INTERPOLATION_BITS) - 1);
}

#if BILINEAR_INTERPOLATION_BITS <= 4
/* Inspired by Filter_32_opaque from Skia */
static force_inline uint32_t
bilinear_interpolation (uint32_t tl, uint32_t tr,
			uint32_t bl, uint32_t br,
			int distx, int disty)
{
    int distxy, distxiy, distixy, distixiy;
    uint32_t lo, hi;

    distx <<= (4 - BILINEAR_INTERPOLATION_BITS);
    disty <<= (4 - BILINEAR_INTERPOLATION_BITS);

    distxy = distx * disty;
    distxiy = (distx << 4) - distxy;	/* distx * (16 - disty) */
    distixy = (disty << 4) - distxy;	/* disty * (16 - distx) */
    distixiy =
	16 * 16 - (disty << 4) -
	(distx << 4) + distxy; /* (16 - distx) * (16 - disty) */

    lo = (tl & 0xff00ff) * distixiy;
    hi = ((tl >> 8) & 0xff00ff) * distixiy;

    lo += (tr & 0xff00ff) * distxiy;
    hi += ((tr >> 8) & 0xff00ff) * distxiy;

    lo += (bl & 0xff00ff) * distixy;
    hi += ((bl >> 8) & 0xff00ff) * distixy;

    lo += (br & 0xff00ff) * distxy;
    hi += ((br >> 8) & 0xff00ff) * distxy;

    return ((lo >> 8) & 0xff00ff) | (hi & ~0xff00ff);
}

#else
#if SIZEOF_LONG > 4

static force_inline uint32_t
bilinear_interpolation (uint32_t tl, uint32_t tr,
			uint32_t bl, uint32_t br,
			int distx, int disty)
{
    uint64_t distxy, distxiy, distixy, distixiy;
    uint64_t tl64, tr64, bl64, br64;
    uint64_t f, r;

    distx <<= (8 - BILINEAR_INTERPOLATION_BITS);
    disty <<= (8 - BILINEAR_INTERPOLATION_BITS);

    distxy = distx * disty;
    distxiy = distx * (256 - disty);
    distixy = (256 - distx) * disty;
    distixiy = (256 - distx) * (256 - disty);

    /* Alpha and Blue */
    tl64 = tl & 0xff0000ff;
    tr64 = tr & 0xff0000ff;
    bl64 = bl & 0xff0000ff;
    br64 = br & 0xff0000ff;

    f = tl64 * distixiy + tr64 * distxiy + bl64 * distixy + br64 * distxy;
    r = f & 0x0000ff0000ff0000ull;

    /* Red and Green */
    tl64 = tl;
    tl64 = ((tl64 << 16) & 0x000000ff00000000ull) | (tl64 & 0x0000ff00ull);

    tr64 = tr;
    tr64 = ((tr64 << 16) & 0x000000ff00000000ull) | (tr64 & 0x0000ff00ull);

    bl64 = bl;
    bl64 = ((bl64 << 16) & 0x000000ff00000000ull) | (bl64 & 0x0000ff00ull);

    br64 = br;
    br64 = ((br64 << 16) & 0x000000ff00000000ull) | (br64 & 0x0000ff00ull);

    f = tl64 * distixiy + tr64 * distxiy + bl64 * distixy + br64 * distxy;
    r |= ((f >> 16) & 0x000000ff00000000ull) | (f & 0xff000000ull);

    return (uint32_t)(r >> 16);
}

#else

static force_inline uint32_t
bilinear_interpolation (uint32_t tl, uint32_t tr,
			uint32_t bl, uint32_t br,
			int distx, int disty)
{
    int distxy, distxiy, distixy, distixiy;
    uint32_t f, r;

    distx <<= (8 - BILINEAR_INTERPOLATION_BITS);
    disty <<= (8 - BILINEAR_INTERPOLATION_BITS);

    distxy = distx * disty;
    distxiy = (distx << 8) - distxy;	/* distx * (256 - disty) */
    distixy = (disty << 8) - distxy;	/* disty * (256 - distx) */
    distixiy =
	256 * 256 - (disty << 8) -
	(distx << 8) + distxy;		/* (256 - distx) * (256 - disty) */

    /* Blue */
    r = (tl & 0x000000ff) * distixiy + (tr & 0x000000ff) * distxiy
      + (bl & 0x000000ff) * distixy  + (br & 0x000000ff) * distxy;

    /* Green */
    f = (tl & 0x0000ff00) * distixiy + (tr & 0x0000ff00) * distxiy
      + (bl & 0x0000ff00) * distixy  + (br & 0x0000ff00) * distxy;
    r |= f & 0xff000000;

    tl >>= 16;
    tr >>= 16;
    bl >>= 16;
    br >>= 16;
    r >>= 16;

    /* Red */
    f = (tl & 0x000000ff) * distixiy + (tr & 0x000000ff) * distxiy
      + (bl & 0x000000ff) * distixy  + (br & 0x000000ff) * distxy;
    r |= f & 0x00ff0000;

    /* Alpha */
    f = (tl & 0x0000ff00) * distixiy + (tr & 0x0000ff00) * distxiy
      + (bl & 0x0000ff00) * distixy  + (br & 0x0000ff00) * distxy;
    r |= f & 0xff000000;

    return r;
}

#endif
#endif // BILINEAR_INTERPOLATION_BITS <= 4

static force_inline argb_t
bilinear_interpolation_float (argb_t tl, argb_t tr,
			      argb_t bl, argb_t br,
			      float distx, float disty)
{
    float distxy, distxiy, distixy, distixiy;
    argb_t r;

    distxy = distx * disty;
    distxiy = distx * (1.f - disty);
    distixy = (1.f - distx) * disty;
    distixiy = (1.f - distx) * (1.f - disty);

    r.a = tl.a * distixiy + tr.a * distxiy +
          bl.a * distixy  + br.a * distxy;
    r.r = tl.r * distixiy + tr.r * distxiy +
          bl.r * distixy  + br.r * distxy;
    r.g = tl.g * distixiy + tr.g * distxiy +
          bl.g * distixy  + br.g * distxy;
    r.b = tl.b * distixiy + tr.b * distxiy +
          bl.b * distixy  + br.b * distxy;

    return r;
}

/*
 * For each scanline fetched from source image with PAD repeat:
 * - calculate how many pixels need to be padded on the left side
 * - calculate how many pixels need to be padded on the right side
 * - update width to only count pixels which are fetched from the image
 * All this information is returned via 'width', 'left_pad', 'right_pad'
 * arguments. The code is assuming that 'unit_x' is positive.
 *
 * Note: 64-bit math is used in order to avoid potential overflows, which
 *       is probably excessive in many cases. This particular function
 *       may need its own correctness test and performance tuning.
 */
static force_inline void
pad_repeat_get_scanline_bounds (int32_t         source_image_width,
				pixman_fixed_t  vx,
				pixman_fixed_t  unit_x,
				int32_t *       width,
				int32_t *       left_pad,
				int32_t *       right_pad)
{
    int64_t max_vx = (int64_t) source_image_width << 16;
    int64_t tmp;
    if (vx < 0)
    {
	tmp = ((int64_t) unit_x - 1 - vx) / unit_x;
	if (tmp > *width)
	{
	    *left_pad = *width;
	    *width = 0;
	}
	else
	{
	    *left_pad = (int32_t) tmp;
	    *width -= (int32_t) tmp;
	}
    }
    else
    {
	*left_pad = 0;
    }
    tmp = ((int64_t) unit_x - 1 - vx + max_vx) / unit_x - *left_pad;
    if (tmp < 0)
    {
	*right_pad = *width;
	*width = 0;
    }
    else if (tmp >= *width)
    {
	*right_pad = 0;
    }
    else
    {
	*right_pad = *width - (int32_t) tmp;
	*width = (int32_t) tmp;
    }
}

/* A macroified version of specialized nearest scalers for some
 * common 8888 and 565 formats. It supports SRC and OVER ops.
 *
 * There are two repeat versions, one that handles repeat normal,
 * and one without repeat handling that only works if the src region
 * used is completely covered by the pre-repeated source samples.
 *
 * The loops are unrolled to process two pixels per iteration for better
 * performance on most CPU architectures (superscalar processors
 * can issue several operations simultaneously, other processors can hide
 * instructions latencies by pipelining operations). Unrolling more
 * does not make much sense because the compiler will start running out
 * of spare registers soon.
 */

#define GET_8888_ALPHA(s) ((s) >> 24)
 /* This is not actually used since we don't have an OVER with
    565 source, but it is needed to build. */
#define GET_0565_ALPHA(s) 0xff
#define GET_x888_ALPHA(s) 0xff

#define FAST_NEAREST_SCANLINE(scanline_func_name, SRC_FORMAT, DST_FORMAT,			\
			      src_type_t, dst_type_t, OP, repeat_mode)				\
static force_inline void									\
scanline_func_name (dst_type_t       *dst,							\
		    const src_type_t *src,							\
		    int32_t           w,							\
		    pixman_fixed_t    vx,							\
		    pixman_fixed_t    unit_x,							\
		    pixman_fixed_t    src_width_fixed,						\
		    pixman_bool_t     fully_transparent_src)					\
{												\
	uint32_t   d;										\
	src_type_t s1, s2;									\
	uint8_t    a1, a2;									\
	int        x1, x2;									\
												\
	if (PIXMAN_OP_ ## OP == PIXMAN_OP_OVER && fully_transparent_src)			\
	    return;										\
												\
	if (PIXMAN_OP_ ## OP != PIXMAN_OP_SRC && PIXMAN_OP_ ## OP != PIXMAN_OP_OVER)		\
	    abort();										\
												\
	while ((w -= 2) >= 0)									\
	{											\
	    x1 = pixman_fixed_to_int (vx);							\
	    vx += unit_x;									\
	    if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NORMAL)				\
	    {											\
		/* This works because we know that unit_x is positive */			\
		while (vx >= 0)									\
		    vx -= src_width_fixed;							\
	    }											\
	    s1 = *(src + x1);									\
												\
	    x2 = pixman_fixed_to_int (vx);							\
	    vx += unit_x;									\
	    if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NORMAL)				\
	    {											\
		/* This works because we know that unit_x is positive */			\
		while (vx >= 0)									\
		    vx -= src_width_fixed;							\
	    }											\
	    s2 = *(src + x2);									\
												\
	    if (PIXMAN_OP_ ## OP == PIXMAN_OP_OVER)						\
	    {											\
		a1 = GET_ ## SRC_FORMAT ## _ALPHA(s1);						\
		a2 = GET_ ## SRC_FORMAT ## _ALPHA(s2);						\
												\
		if (a1 == 0xff)									\
		{										\
		    *dst = convert_ ## SRC_FORMAT ## _to_ ## DST_FORMAT (s1);			\
		}										\
		else if (s1)									\
		{										\
		    d = convert_ ## DST_FORMAT ## _to_8888 (*dst);				\
		    s1 = convert_ ## SRC_FORMAT ## _to_8888 (s1);				\
		    a1 ^= 0xff;									\
		    UN8x4_MUL_UN8_ADD_UN8x4 (d, a1, s1);					\
		    *dst = convert_8888_to_ ## DST_FORMAT (d);					\
		}										\
		dst++;										\
												\
		if (a2 == 0xff)									\
		{										\
		    *dst = convert_ ## SRC_FORMAT ## _to_ ## DST_FORMAT (s2);			\
		}										\
		else if (s2)									\
		{										\
		    d = convert_## DST_FORMAT ## _to_8888 (*dst);				\
		    s2 = convert_## SRC_FORMAT ## _to_8888 (s2);				\
		    a2 ^= 0xff;									\
		    UN8x4_MUL_UN8_ADD_UN8x4 (d, a2, s2);					\
		    *dst = convert_8888_to_ ## DST_FORMAT (d);					\
		}										\
		dst++;										\
	    }											\
	    else /* PIXMAN_OP_SRC */								\
	    {											\
		*dst++ = convert_ ## SRC_FORMAT ## _to_ ## DST_FORMAT (s1);			\
		*dst++ = convert_ ## SRC_FORMAT ## _to_ ## DST_FORMAT (s2);			\
	    }											\
	}											\
												\
	if (w & 1)										\
	{											\
	    x1 = pixman_fixed_to_int (vx);							\
	    s1 = *(src + x1);									\
												\
	    if (PIXMAN_OP_ ## OP == PIXMAN_OP_OVER)						\
	    {											\
		a1 = GET_ ## SRC_FORMAT ## _ALPHA(s1);						\
												\
		if (a1 == 0xff)									\
		{										\
		    *dst = convert_ ## SRC_FORMAT ## _to_ ## DST_FORMAT (s1);			\
		}										\
		else if (s1)									\
		{										\
		    d = convert_## DST_FORMAT ## _to_8888 (*dst);				\
		    s1 = convert_ ## SRC_FORMAT ## _to_8888 (s1);				\
		    a1 ^= 0xff;									\
		    UN8x4_MUL_UN8_ADD_UN8x4 (d, a1, s1);					\
		    *dst = convert_8888_to_ ## DST_FORMAT (d);					\
		}										\
		dst++;										\
	    }											\
	    else /* PIXMAN_OP_SRC */								\
	    {											\
		*dst++ = convert_ ## SRC_FORMAT ## _to_ ## DST_FORMAT (s1);			\
	    }											\
	}											\
}

#define FAST_NEAREST_MAINLOOP_INT(scale_func_name, scanline_func, src_type_t, mask_type_t,	\
				  dst_type_t, repeat_mode, have_mask, mask_is_solid)		\
static void											\
fast_composite_scaled_nearest  ## scale_func_name (pixman_implementation_t *imp,		\
						   pixman_composite_info_t *info)               \
{												\
    PIXMAN_COMPOSITE_ARGS (info);					                        \
    dst_type_t *dst_line;						                        \
    mask_type_t *mask_line;									\
    src_type_t *src_first_line;									\
    int       y;										\
    pixman_fixed_t src_width_fixed = pixman_int_to_fixed (src_image->bits.width);		\
    pixman_fixed_t max_vy;									\
    pixman_vector_t v;										\
    pixman_fixed_t vx, vy;									\
    pixman_fixed_t unit_x, unit_y;								\
    int32_t left_pad, right_pad;								\
												\
    src_type_t *src;										\
    dst_type_t *dst;										\
    mask_type_t solid_mask;									\
    const mask_type_t *mask = &solid_mask;							\
    int src_stride, mask_stride, dst_stride;							\
												\
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type_t, dst_stride, dst_line, 1);	\
    if (have_mask)										\
    {												\
	if (mask_is_solid)									\
	    solid_mask = _pixman_image_get_solid (imp, mask_image, dest_image->bits.format);	\
	else											\
	    PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, mask_type_t,			\
				   mask_stride, mask_line, 1);					\
    }												\
    /* pass in 0 instead of src_x and src_y because src_x and src_y need to be			\
     * transformed from destination space to source space */					\
    PIXMAN_IMAGE_GET_LINE (src_image, 0, 0, src_type_t, src_stride, src_first_line, 1);		\
												\
    /* reference point is the center of the pixel */						\
    v.vector[0] = pixman_int_to_fixed (src_x) + pixman_fixed_1 / 2;				\
    v.vector[1] = pixman_int_to_fixed (src_y) + pixman_fixed_1 / 2;				\
    v.vector[2] = pixman_fixed_1;								\
												\
    if (!pixman_transform_point_3d (src_image->common.transform, &v))				\
	return;											\
												\
    unit_x = src_image->common.transform->matrix[0][0];						\
    unit_y = src_image->common.transform->matrix[1][1];						\
												\
    /* Round down to closest integer, ensuring that 0.5 rounds to 0, not 1 */			\
    v.vector[0] -= pixman_fixed_e;								\
    v.vector[1] -= pixman_fixed_e;								\
												\
    vx = v.vector[0];										\
    vy = v.vector[1];										\
												\
    if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NORMAL)					\
    {												\
	max_vy = pixman_int_to_fixed (src_image->bits.height);					\
												\
	/* Clamp repeating positions inside the actual samples */				\
	repeat (PIXMAN_REPEAT_NORMAL, &vx, src_width_fixed);					\
	repeat (PIXMAN_REPEAT_NORMAL, &vy, max_vy);						\
    }												\
												\
    if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_PAD ||					\
	PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NONE)					\
    {												\
	pad_repeat_get_scanline_bounds (src_image->bits.width, vx, unit_x,			\
					&width, &left_pad, &right_pad);				\
	vx += left_pad * unit_x;								\
    }												\
												\
    while (--height >= 0)									\
    {												\
	dst = dst_line;										\
	dst_line += dst_stride;									\
	if (have_mask && !mask_is_solid)							\
	{											\
	    mask = mask_line;									\
	    mask_line += mask_stride;								\
	}											\
												\
	y = pixman_fixed_to_int (vy);								\
	vy += unit_y;										\
	if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NORMAL)				\
	    repeat (PIXMAN_REPEAT_NORMAL, &vy, max_vy);						\
	if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_PAD)					\
	{											\
	    repeat (PIXMAN_REPEAT_PAD, &y, src_image->bits.height);				\
	    src = src_first_line + src_stride * y;						\
	    if (left_pad > 0)									\
	    {											\
		scanline_func (mask, dst,							\
			       src + src_image->bits.width - src_image->bits.width + 1,		\
			       left_pad, -pixman_fixed_e, 0, src_width_fixed, FALSE);		\
	    }											\
	    if (width > 0)									\
	    {											\
		scanline_func (mask + (mask_is_solid ? 0 : left_pad),				\
			       dst + left_pad, src + src_image->bits.width, width,		\
			       vx - src_width_fixed, unit_x, src_width_fixed, FALSE);		\
	    }											\
	    if (right_pad > 0)									\
	    {											\
		scanline_func (mask + (mask_is_solid ? 0 : left_pad + width),			\
			       dst + left_pad + width, src + src_image->bits.width,		\
			       right_pad, -pixman_fixed_e, 0, src_width_fixed, FALSE);		\
	    }											\
	}											\
	else if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NONE)				\
	{											\
	    static const src_type_t zero[1] = { 0 };						\
	    if (y < 0 || y >= src_image->bits.height)						\
	    {											\
		scanline_func (mask, dst, zero + 1, left_pad + width + right_pad,		\
			       -pixman_fixed_e, 0, src_width_fixed, TRUE);			\
		continue;									\
	    }											\
	    src = src_first_line + src_stride * y;						\
	    if (left_pad > 0)									\
	    {											\
		scanline_func (mask, dst, zero + 1, left_pad,					\
			       -pixman_fixed_e, 0, src_width_fixed, TRUE);			\
	    }											\
	    if (width > 0)									\
	    {											\
		scanline_func (mask + (mask_is_solid ? 0 : left_pad),				\
			       dst + left_pad, src + src_image->bits.width, width,		\
			       vx - src_width_fixed, unit_x, src_width_fixed, FALSE);		\
	    }											\
	    if (right_pad > 0)									\
	    {											\
		scanline_func (mask + (mask_is_solid ? 0 : left_pad + width),			\
			       dst + left_pad + width, zero + 1, right_pad,			\
			       -pixman_fixed_e, 0, src_width_fixed, TRUE);			\
	    }											\
	}											\
	else											\
	{											\
	    src = src_first_line + src_stride * y;						\
	    scanline_func (mask, dst, src + src_image->bits.width, width, vx - src_width_fixed,	\
			   unit_x, src_width_fixed, FALSE);					\
	}											\
    }												\
}

/* A workaround for old sun studio, see: https://bugs.freedesktop.org/show_bug.cgi?id=32764 */
#define FAST_NEAREST_MAINLOOP_COMMON(scale_func_name, scanline_func, src_type_t, mask_type_t,	\
				  dst_type_t, repeat_mode, have_mask, mask_is_solid)		\
	FAST_NEAREST_MAINLOOP_INT(_ ## scale_func_name, scanline_func, src_type_t, mask_type_t,	\
				  dst_type_t, repeat_mode, have_mask, mask_is_solid)

#define FAST_NEAREST_MAINLOOP_NOMASK(scale_func_name, scanline_func, src_type_t, dst_type_t,	\
			      repeat_mode)							\
    static force_inline void									\
    scanline_func##scale_func_name##_wrapper (							\
		    const uint8_t    *mask,							\
		    dst_type_t       *dst,							\
		    const src_type_t *src,							\
		    int32_t          w,								\
		    pixman_fixed_t   vx,							\
		    pixman_fixed_t   unit_x,							\
		    pixman_fixed_t   max_vx,							\
		    pixman_bool_t    fully_transparent_src)					\
    {												\
	scanline_func (dst, src, w, vx, unit_x, max_vx, fully_transparent_src);			\
    }												\
    FAST_NEAREST_MAINLOOP_INT (scale_func_name, scanline_func##scale_func_name##_wrapper,	\
			       src_type_t, uint8_t, dst_type_t, repeat_mode, FALSE, FALSE)

#define FAST_NEAREST_MAINLOOP(scale_func_name, scanline_func, src_type_t, dst_type_t,		\
			      repeat_mode)							\
	FAST_NEAREST_MAINLOOP_NOMASK(_ ## scale_func_name, scanline_func, src_type_t,		\
			      dst_type_t, repeat_mode)

#define FAST_NEAREST(scale_func_name, SRC_FORMAT, DST_FORMAT,				\
		     src_type_t, dst_type_t, OP, repeat_mode)				\
    FAST_NEAREST_SCANLINE(scaled_nearest_scanline_ ## scale_func_name ## _ ## OP,	\
			  SRC_FORMAT, DST_FORMAT, src_type_t, dst_type_t,		\
			  OP, repeat_mode)						\
    FAST_NEAREST_MAINLOOP_NOMASK(_ ## scale_func_name ## _ ## OP,			\
			  scaled_nearest_scanline_ ## scale_func_name ## _ ## OP,	\
			  src_type_t, dst_type_t, repeat_mode)


#define SCALED_NEAREST_FLAGS						\
    (FAST_PATH_SCALE_TRANSFORM	|					\
     FAST_PATH_NO_ALPHA_MAP	|					\
     FAST_PATH_NEAREST_FILTER	|					\
     FAST_PATH_NO_ACCESSORS	|					\
     FAST_PATH_NARROW_FORMAT)

#define SIMPLE_NEAREST_FAST_PATH_NORMAL(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_NORMAL_REPEAT	|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _normal ## _ ## op,	\
    }

#define SIMPLE_NEAREST_FAST_PATH_PAD(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_PAD_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _pad ## _ ## op,	\
    }

#define SIMPLE_NEAREST_FAST_PATH_NONE(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_NONE_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _none ## _ ## op,	\
    }

#define SIMPLE_NEAREST_FAST_PATH_COVER(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	SCALED_NEAREST_FLAGS | FAST_PATH_SAMPLES_COVER_CLIP_NEAREST,    \
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _cover ## _ ## op,	\
    }

#define SIMPLE_NEAREST_A8_MASK_FAST_PATH_NORMAL(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_NORMAL_REPEAT	|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_a8, MASK_FLAGS (a8, FAST_PATH_UNIFIED_ALPHA),		\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _normal ## _ ## op,	\
    }

#define SIMPLE_NEAREST_A8_MASK_FAST_PATH_PAD(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_PAD_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_a8, MASK_FLAGS (a8, FAST_PATH_UNIFIED_ALPHA),		\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _pad ## _ ## op,	\
    }

#define SIMPLE_NEAREST_A8_MASK_FAST_PATH_NONE(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_NONE_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_a8, MASK_FLAGS (a8, FAST_PATH_UNIFIED_ALPHA),		\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _none ## _ ## op,	\
    }

#define SIMPLE_NEAREST_A8_MASK_FAST_PATH_COVER(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	SCALED_NEAREST_FLAGS | FAST_PATH_SAMPLES_COVER_CLIP_NEAREST,	\
	PIXMAN_a8, MASK_FLAGS (a8, FAST_PATH_UNIFIED_ALPHA),		\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _cover ## _ ## op,	\
    }

#define SIMPLE_NEAREST_SOLID_MASK_FAST_PATH_NORMAL(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_NORMAL_REPEAT	|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_solid, MASK_FLAGS (solid, FAST_PATH_UNIFIED_ALPHA),	\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _normal ## _ ## op,	\
    }

#define SIMPLE_NEAREST_SOLID_MASK_FAST_PATH_PAD(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_PAD_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_solid, MASK_FLAGS (solid, FAST_PATH_UNIFIED_ALPHA),	\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _pad ## _ ## op,	\
    }

#define SIMPLE_NEAREST_SOLID_MASK_FAST_PATH_NONE(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_NEAREST_FLAGS		|				\
	 FAST_PATH_NONE_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_solid, MASK_FLAGS (solid, FAST_PATH_UNIFIED_ALPHA),	\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _none ## _ ## op,	\
    }

#define SIMPLE_NEAREST_SOLID_MASK_FAST_PATH_COVER(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	SCALED_NEAREST_FLAGS | FAST_PATH_SAMPLES_COVER_CLIP_NEAREST,	\
	PIXMAN_solid, MASK_FLAGS (solid, FAST_PATH_UNIFIED_ALPHA),	\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_nearest_ ## func ## _cover ## _ ## op,	\
    }

/* Prefer the use of 'cover' variant, because it is faster */
#define SIMPLE_NEAREST_FAST_PATH(op,s,d,func)				\
    SIMPLE_NEAREST_FAST_PATH_COVER (op,s,d,func),			\
    SIMPLE_NEAREST_FAST_PATH_NONE (op,s,d,func),			\
    SIMPLE_NEAREST_FAST_PATH_PAD (op,s,d,func),				\
    SIMPLE_NEAREST_FAST_PATH_NORMAL (op,s,d,func)

#define SIMPLE_NEAREST_A8_MASK_FAST_PATH(op,s,d,func)			\
    SIMPLE_NEAREST_A8_MASK_FAST_PATH_COVER (op,s,d,func),		\
    SIMPLE_NEAREST_A8_MASK_FAST_PATH_NONE (op,s,d,func),		\
    SIMPLE_NEAREST_A8_MASK_FAST_PATH_PAD (op,s,d,func)

#define SIMPLE_NEAREST_SOLID_MASK_FAST_PATH(op,s,d,func)		\
    SIMPLE_NEAREST_SOLID_MASK_FAST_PATH_COVER (op,s,d,func),		\
    SIMPLE_NEAREST_SOLID_MASK_FAST_PATH_NONE (op,s,d,func),		\
    SIMPLE_NEAREST_SOLID_MASK_FAST_PATH_PAD (op,s,d,func),              \
    SIMPLE_NEAREST_SOLID_MASK_FAST_PATH_NORMAL (op,s,d,func)

/*****************************************************************************/

/*
 * Identify 5 zones in each scanline for bilinear scaling. Depending on
 * whether 2 pixels to be interpolated are fetched from the image itself,
 * from the padding area around it or from both image and padding area.
 */
static force_inline void
bilinear_pad_repeat_get_scanline_bounds (int32_t         source_image_width,
					 pixman_fixed_t  vx,
					 pixman_fixed_t  unit_x,
					 int32_t *       left_pad,
					 int32_t *       left_tz,
					 int32_t *       width,
					 int32_t *       right_tz,
					 int32_t *       right_pad)
{
	int width1 = *width, left_pad1, right_pad1;
	int width2 = *width, left_pad2, right_pad2;

	pad_repeat_get_scanline_bounds (source_image_width, vx, unit_x,
					&width1, &left_pad1, &right_pad1);
	pad_repeat_get_scanline_bounds (source_image_width, vx + pixman_fixed_1,
					unit_x, &width2, &left_pad2, &right_pad2);

	*left_pad = left_pad2;
	*left_tz = left_pad1 - left_pad2;
	*right_tz = right_pad2 - right_pad1;
	*right_pad = right_pad1;
	*width -= *left_pad + *left_tz + *right_tz + *right_pad;
}

/*
 * Main loop template for single pass bilinear scaling. It needs to be
 * provided with 'scanline_func' which should do the compositing operation.
 * The needed function has the following prototype:
 *
 *	scanline_func (dst_type_t *       dst,
 *		       const mask_type_ * mask,
 *		       const src_type_t * src_top,
 *		       const src_type_t * src_bottom,
 *		       int32_t            width,
 *		       int                weight_top,
 *		       int                weight_bottom,
 *		       pixman_fixed_t     vx,
 *		       pixman_fixed_t     unit_x,
 *		       pixman_fixed_t     max_vx,
 *		       pixman_bool_t      zero_src)
 *
 * Where:
 *  dst                 - destination scanline buffer for storing results
 *  mask                - mask buffer (or single value for solid mask)
 *  src_top, src_bottom - two source scanlines
 *  width               - number of pixels to process
 *  weight_top          - weight of the top row for interpolation
 *  weight_bottom       - weight of the bottom row for interpolation
 *  vx                  - initial position for fetching the first pair of
 *                        pixels from the source buffer
 *  unit_x              - position increment needed to move to the next pair
 *                        of pixels
 *  max_vx              - image size as a fixed point value, can be used for
 *                        implementing NORMAL repeat (when it is supported)
 *  zero_src            - boolean hint variable, which is set to TRUE when
 *                        all source pixels are fetched from zero padding
 *                        zone for NONE repeat
 *
 * Note: normally the sum of 'weight_top' and 'weight_bottom' is equal to
 *       BILINEAR_INTERPOLATION_RANGE, but sometimes it may be less than that
 *       for NONE repeat when handling fuzzy antialiased top or bottom image
 *       edges. Also both top and bottom weight variables are guaranteed to
 *       have value, which is less than BILINEAR_INTERPOLATION_RANGE.
 *       For example, the weights can fit into unsigned byte or be used
 *       with 8-bit SIMD multiplication instructions for 8-bit interpolation
 *       precision.
 */
#define FAST_BILINEAR_MAINLOOP_INT(scale_func_name, scanline_func, src_type_t, mask_type_t,	\
				  dst_type_t, repeat_mode, flags)				\
static void											\
fast_composite_scaled_bilinear ## scale_func_name (pixman_implementation_t *imp,		\
						   pixman_composite_info_t *info)		\
{												\
    PIXMAN_COMPOSITE_ARGS (info);								\
    dst_type_t *dst_line;									\
    mask_type_t *mask_line;									\
    src_type_t *src_first_line;									\
    int       y1, y2;										\
    pixman_fixed_t max_vx = INT32_MAX; /* suppress uninitialized variable warning */		\
    pixman_vector_t v;										\
    pixman_fixed_t vx, vy;									\
    pixman_fixed_t unit_x, unit_y;								\
    int32_t left_pad, left_tz, right_tz, right_pad;						\
												\
    dst_type_t *dst;										\
    mask_type_t solid_mask;									\
    const mask_type_t *mask = &solid_mask;							\
    int src_stride, mask_stride, dst_stride;							\
												\
    int src_width;										\
    pixman_fixed_t src_width_fixed;								\
    int max_x;											\
    pixman_bool_t need_src_extension;								\
												\
    PIXMAN_IMAGE_GET_LINE (dest_image, dest_x, dest_y, dst_type_t, dst_stride, dst_line, 1);	\
    if (flags & FLAG_HAVE_SOLID_MASK)								\
    {												\
	solid_mask = _pixman_image_get_solid (imp, mask_image, dest_image->bits.format);	\
	mask_stride = 0;									\
    }												\
    else if (flags & FLAG_HAVE_NON_SOLID_MASK)							\
    {												\
	PIXMAN_IMAGE_GET_LINE (mask_image, mask_x, mask_y, mask_type_t,				\
			       mask_stride, mask_line, 1);					\
    }												\
												\
    /* pass in 0 instead of src_x and src_y because src_x and src_y need to be			\
     * transformed from destination space to source space */					\
    PIXMAN_IMAGE_GET_LINE (src_image, 0, 0, src_type_t, src_stride, src_first_line, 1);		\
												\
    /* reference point is the center of the pixel */						\
    v.vector[0] = pixman_int_to_fixed (src_x) + pixman_fixed_1 / 2;				\
    v.vector[1] = pixman_int_to_fixed (src_y) + pixman_fixed_1 / 2;				\
    v.vector[2] = pixman_fixed_1;								\
												\
    if (!pixman_transform_point_3d (src_image->common.transform, &v))				\
	return;											\
												\
    unit_x = src_image->common.transform->matrix[0][0];						\
    unit_y = src_image->common.transform->matrix[1][1];						\
												\
    v.vector[0] -= pixman_fixed_1 / 2;								\
    v.vector[1] -= pixman_fixed_1 / 2;								\
												\
    vy = v.vector[1];										\
												\
    if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_PAD ||					\
	PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NONE)					\
    {												\
	bilinear_pad_repeat_get_scanline_bounds (src_image->bits.width, v.vector[0], unit_x,	\
					&left_pad, &left_tz, &width, &right_tz, &right_pad);	\
	if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_PAD)					\
	{											\
	    /* PAD repeat does not need special handling for 'transition zones' and */		\
	    /* they can be combined with 'padding zones' safely */				\
	    left_pad += left_tz;								\
	    right_pad += right_tz;								\
	    left_tz = right_tz = 0;								\
	}											\
	v.vector[0] += left_pad * unit_x;							\
    }												\
												\
    if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NORMAL)					\
    {												\
	vx = v.vector[0];									\
	repeat (PIXMAN_REPEAT_NORMAL, &vx, pixman_int_to_fixed(src_image->bits.width));		\
	max_x = pixman_fixed_to_int (vx + (width - 1) * (int64_t)unit_x) + 1;			\
												\
	if (src_image->bits.width < REPEAT_NORMAL_MIN_WIDTH)					\
	{											\
	    src_width = 0;									\
												\
	    while (src_width < REPEAT_NORMAL_MIN_WIDTH && src_width <= max_x)			\
		src_width += src_image->bits.width;						\
												\
	    need_src_extension = TRUE;								\
	}											\
	else											\
	{											\
	    src_width = src_image->bits.width;							\
	    need_src_extension = FALSE;								\
	}											\
												\
	src_width_fixed = pixman_int_to_fixed (src_width);					\
    }												\
												\
    while (--height >= 0)									\
    {												\
	int weight1, weight2;									\
	dst = dst_line;										\
	dst_line += dst_stride;									\
	vx = v.vector[0];									\
	if (flags & FLAG_HAVE_NON_SOLID_MASK)							\
	{											\
	    mask = mask_line;									\
	    mask_line += mask_stride;								\
	}											\
												\
	y1 = pixman_fixed_to_int (vy);								\
	weight2 = pixman_fixed_to_bilinear_weight (vy);						\
	if (weight2)										\
	{											\
	    /* both weight1 and weight2 are smaller than BILINEAR_INTERPOLATION_RANGE */	\
	    y2 = y1 + 1;									\
	    weight1 = BILINEAR_INTERPOLATION_RANGE - weight2;					\
	}											\
	else											\
	{											\
	    /* set both top and bottom row to the same scanline and tweak weights */		\
	    y2 = y1;										\
	    weight1 = weight2 = BILINEAR_INTERPOLATION_RANGE / 2;				\
	}											\
	vy += unit_y;										\
	if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_PAD)					\
	{											\
	    src_type_t *src1, *src2;								\
	    src_type_t buf1[2];									\
	    src_type_t buf2[2];									\
	    repeat (PIXMAN_REPEAT_PAD, &y1, src_image->bits.height);				\
	    repeat (PIXMAN_REPEAT_PAD, &y2, src_image->bits.height);				\
	    src1 = src_first_line + src_stride * y1;						\
	    src2 = src_first_line + src_stride * y2;						\
												\
	    if (left_pad > 0)									\
	    {											\
		buf1[0] = buf1[1] = src1[0];							\
		buf2[0] = buf2[1] = src2[0];							\
		scanline_func (dst, mask,							\
			       buf1, buf2, left_pad, weight1, weight2, 0, 0, 0, FALSE);		\
		dst += left_pad;								\
		if (flags & FLAG_HAVE_NON_SOLID_MASK)						\
		    mask += left_pad;								\
	    }											\
	    if (width > 0)									\
	    {											\
		scanline_func (dst, mask,							\
			       src1, src2, width, weight1, weight2, vx, unit_x, 0, FALSE);	\
		dst += width;									\
		if (flags & FLAG_HAVE_NON_SOLID_MASK)						\
		    mask += width;								\
	    }											\
	    if (right_pad > 0)									\
	    {											\
		buf1[0] = buf1[1] = src1[src_image->bits.width - 1];				\
		buf2[0] = buf2[1] = src2[src_image->bits.width - 1];				\
		scanline_func (dst, mask,							\
			       buf1, buf2, right_pad, weight1, weight2, 0, 0, 0, FALSE);	\
	    }											\
	}											\
	else if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NONE)				\
	{											\
	    src_type_t *src1, *src2;								\
	    src_type_t buf1[2];									\
	    src_type_t buf2[2];									\
	    /* handle top/bottom zero padding by just setting weights to 0 if needed */		\
	    if (y1 < 0)										\
	    {											\
		weight1 = 0;									\
		y1 = 0;										\
	    }											\
	    if (y1 >= src_image->bits.height)							\
	    {											\
		weight1 = 0;									\
		y1 = src_image->bits.height - 1;						\
	    }											\
	    if (y2 < 0)										\
	    {											\
		weight2 = 0;									\
		y2 = 0;										\
	    }											\
	    if (y2 >= src_image->bits.height)							\
	    {											\
		weight2 = 0;									\
		y2 = src_image->bits.height - 1;						\
	    }											\
	    src1 = src_first_line + src_stride * y1;						\
	    src2 = src_first_line + src_stride * y2;						\
												\
	    if (left_pad > 0)									\
	    {											\
		buf1[0] = buf1[1] = 0;								\
		buf2[0] = buf2[1] = 0;								\
		scanline_func (dst, mask,							\
			       buf1, buf2, left_pad, weight1, weight2, 0, 0, 0, TRUE);		\
		dst += left_pad;								\
		if (flags & FLAG_HAVE_NON_SOLID_MASK)						\
		    mask += left_pad;								\
	    }											\
	    if (left_tz > 0)									\
	    {											\
		buf1[0] = 0;									\
		buf1[1] = src1[0];								\
		buf2[0] = 0;									\
		buf2[1] = src2[0];								\
		scanline_func (dst, mask,							\
			       buf1, buf2, left_tz, weight1, weight2,				\
			       pixman_fixed_frac (vx), unit_x, 0, FALSE);			\
		dst += left_tz;									\
		if (flags & FLAG_HAVE_NON_SOLID_MASK)						\
		    mask += left_tz;								\
		vx += left_tz * unit_x;								\
	    }											\
	    if (width > 0)									\
	    {											\
		scanline_func (dst, mask,							\
			       src1, src2, width, weight1, weight2, vx, unit_x, 0, FALSE);	\
		dst += width;									\
		if (flags & FLAG_HAVE_NON_SOLID_MASK)						\
		    mask += width;								\
		vx += width * unit_x;								\
	    }											\
	    if (right_tz > 0)									\
	    {											\
		buf1[0] = src1[src_image->bits.width - 1];					\
		buf1[1] = 0;									\
		buf2[0] = src2[src_image->bits.width - 1];					\
		buf2[1] = 0;									\
		scanline_func (dst, mask,							\
			       buf1, buf2, right_tz, weight1, weight2,				\
			       pixman_fixed_frac (vx), unit_x, 0, FALSE);			\
		dst += right_tz;								\
		if (flags & FLAG_HAVE_NON_SOLID_MASK)						\
		    mask += right_tz;								\
	    }											\
	    if (right_pad > 0)									\
	    {											\
		buf1[0] = buf1[1] = 0;								\
		buf2[0] = buf2[1] = 0;								\
		scanline_func (dst, mask,							\
			       buf1, buf2, right_pad, weight1, weight2, 0, 0, 0, TRUE);		\
	    }											\
	}											\
	else if (PIXMAN_REPEAT_ ## repeat_mode == PIXMAN_REPEAT_NORMAL)				\
	{											\
	    int32_t	    num_pixels;								\
	    int32_t	    width_remain;							\
	    src_type_t *    src_line_top;							\
	    src_type_t *    src_line_bottom;							\
	    src_type_t	    buf1[2];								\
	    src_type_t	    buf2[2];								\
	    src_type_t	    extended_src_line0[REPEAT_NORMAL_MIN_WIDTH*2];			\
	    src_type_t	    extended_src_line1[REPEAT_NORMAL_MIN_WIDTH*2];			\
	    int		    i, j;								\
												\
	    repeat (PIXMAN_REPEAT_NORMAL, &y1, src_image->bits.height);				\
	    repeat (PIXMAN_REPEAT_NORMAL, &y2, src_image->bits.height);				\
	    src_line_top = src_first_line + src_stride * y1;					\
	    src_line_bottom = src_first_line + src_stride * y2;					\
												\
	    if (need_src_extension)								\
	    {											\
		for (i=0; i<src_width;)								\
		{										\
		    for (j=0; j<src_image->bits.width; j++, i++)				\
		    {										\
			extended_src_line0[i] = src_line_top[j];				\
			extended_src_line1[i] = src_line_bottom[j];				\
		    }										\
		}										\
												\
		src_line_top = &extended_src_line0[0];						\
		src_line_bottom = &extended_src_line1[0];					\
	    }											\
												\
	    /* Top & Bottom wrap around buffer */						\
	    buf1[0] = src_line_top[src_width - 1];						\
	    buf1[1] = src_line_top[0];								\
	    buf2[0] = src_line_bottom[src_width - 1];						\
	    buf2[1] = src_line_bottom[0];							\
												\
	    width_remain = width;								\
												\
	    while (width_remain > 0)								\
	    {											\
		/* We use src_width_fixed because it can make vx in original source range */	\
		repeat (PIXMAN_REPEAT_NORMAL, &vx, src_width_fixed);				\
												\
		/* Wrap around part */								\
		if (pixman_fixed_to_int (vx) == src_width - 1)					\
		{										\
		    /* for positive unit_x							\
		     * num_pixels = max(n) + 1, where vx + n*unit_x < src_width_fixed		\
		     *										\
		     * vx is in range [0, src_width_fixed - pixman_fixed_e]			\
		     * So we are safe from overflow.						\
		     */										\
		    num_pixels = ((src_width_fixed - vx - pixman_fixed_e) / unit_x) + 1;	\
												\
		    if (num_pixels > width_remain)						\
			num_pixels = width_remain;						\
												\
		    scanline_func (dst, mask, buf1, buf2, num_pixels,				\
				   weight1, weight2, pixman_fixed_frac(vx),			\
				   unit_x, src_width_fixed, FALSE);				\
												\
		    width_remain -= num_pixels;							\
		    vx += num_pixels * unit_x;							\
		    dst += num_pixels;								\
												\
		    if (flags & FLAG_HAVE_NON_SOLID_MASK)					\
			mask += num_pixels;							\
												\
		    repeat (PIXMAN_REPEAT_NORMAL, &vx, src_width_fixed);			\
		}										\
												\
		/* Normal scanline composite */							\
		if (pixman_fixed_to_int (vx) != src_width - 1 && width_remain > 0)		\
		{										\
		    /* for positive unit_x							\
		     * num_pixels = max(n) + 1, where vx + n*unit_x < (src_width_fixed - 1)	\
		     *										\
		     * vx is in range [0, src_width_fixed - pixman_fixed_e]			\
		     * So we are safe from overflow here.					\
		     */										\
		    num_pixels = ((src_width_fixed - pixman_fixed_1 - vx - pixman_fixed_e)	\
				  / unit_x) + 1;						\
												\
		    if (num_pixels > width_remain)						\
			num_pixels = width_remain;						\
												\
		    scanline_func (dst, mask, src_line_top, src_line_bottom, num_pixels,	\
				   weight1, weight2, vx, unit_x, src_width_fixed, FALSE);	\
												\
		    width_remain -= num_pixels;							\
		    vx += num_pixels * unit_x;							\
		    dst += num_pixels;								\
												\
		    if (flags & FLAG_HAVE_NON_SOLID_MASK)					\
		        mask += num_pixels;							\
		}										\
	    }											\
	}											\
	else											\
	{											\
	    scanline_func (dst, mask, src_first_line + src_stride * y1,				\
			   src_first_line + src_stride * y2, width,				\
			   weight1, weight2, vx, unit_x, max_vx, FALSE);			\
	}											\
    }												\
}

/* A workaround for old sun studio, see: https://bugs.freedesktop.org/show_bug.cgi?id=32764 */
#define FAST_BILINEAR_MAINLOOP_COMMON(scale_func_name, scanline_func, src_type_t, mask_type_t,	\
				  dst_type_t, repeat_mode, flags)				\
	FAST_BILINEAR_MAINLOOP_INT(_ ## scale_func_name, scanline_func, src_type_t, mask_type_t,\
				  dst_type_t, repeat_mode, flags)

#define SCALED_BILINEAR_FLAGS						\
    (FAST_PATH_SCALE_TRANSFORM	|					\
     FAST_PATH_NO_ALPHA_MAP	|					\
     FAST_PATH_BILINEAR_FILTER	|					\
     FAST_PATH_NO_ACCESSORS	|					\
     FAST_PATH_NARROW_FORMAT)

#define SIMPLE_BILINEAR_FAST_PATH_PAD(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_BILINEAR_FLAGS		|				\
	 FAST_PATH_PAD_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _pad ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_FAST_PATH_NONE(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_BILINEAR_FLAGS		|				\
	 FAST_PATH_NONE_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _none ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_FAST_PATH_COVER(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	SCALED_BILINEAR_FLAGS | FAST_PATH_SAMPLES_COVER_CLIP_BILINEAR,	\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _cover ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_FAST_PATH_NORMAL(op,s,d,func)			\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_BILINEAR_FLAGS		|				\
	 FAST_PATH_NORMAL_REPEAT	|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_null, 0,							\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _normal ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_A8_MASK_FAST_PATH_PAD(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_BILINEAR_FLAGS		|				\
	 FAST_PATH_PAD_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_a8, MASK_FLAGS (a8, FAST_PATH_UNIFIED_ALPHA),		\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _pad ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_A8_MASK_FAST_PATH_NONE(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_BILINEAR_FLAGS		|				\
	 FAST_PATH_NONE_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_a8, MASK_FLAGS (a8, FAST_PATH_UNIFIED_ALPHA),		\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _none ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_A8_MASK_FAST_PATH_COVER(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	SCALED_BILINEAR_FLAGS | FAST_PATH_SAMPLES_COVER_CLIP_BILINEAR,	\
	PIXMAN_a8, MASK_FLAGS (a8, FAST_PATH_UNIFIED_ALPHA),		\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _cover ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_A8_MASK_FAST_PATH_NORMAL(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_BILINEAR_FLAGS		|				\
	 FAST_PATH_NORMAL_REPEAT	|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_a8, MASK_FLAGS (a8, FAST_PATH_UNIFIED_ALPHA),		\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _normal ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_SOLID_MASK_FAST_PATH_PAD(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_BILINEAR_FLAGS		|				\
	 FAST_PATH_PAD_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_solid, MASK_FLAGS (solid, FAST_PATH_UNIFIED_ALPHA),	\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _pad ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_SOLID_MASK_FAST_PATH_NONE(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_BILINEAR_FLAGS		|				\
	 FAST_PATH_NONE_REPEAT		|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_solid, MASK_FLAGS (solid, FAST_PATH_UNIFIED_ALPHA),	\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _none ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_SOLID_MASK_FAST_PATH_COVER(op,s,d,func)		\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	SCALED_BILINEAR_FLAGS | FAST_PATH_SAMPLES_COVER_CLIP_BILINEAR,	\
	PIXMAN_solid, MASK_FLAGS (solid, FAST_PATH_UNIFIED_ALPHA),	\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _cover ## _ ## op,	\
    }

#define SIMPLE_BILINEAR_SOLID_MASK_FAST_PATH_NORMAL(op,s,d,func)	\
    {   PIXMAN_OP_ ## op,						\
	PIXMAN_ ## s,							\
	(SCALED_BILINEAR_FLAGS		|				\
	 FAST_PATH_NORMAL_REPEAT	|				\
	 FAST_PATH_X_UNIT_POSITIVE),					\
	PIXMAN_solid, MASK_FLAGS (solid, FAST_PATH_UNIFIED_ALPHA),	\
	PIXMAN_ ## d, FAST_PATH_STD_DEST_FLAGS,				\
	fast_composite_scaled_bilinear_ ## func ## _normal ## _ ## op,	\
    }

/* Prefer the use of 'cover' variant, because it is faster */
#define SIMPLE_BILINEAR_FAST_PATH(op,s,d,func)				\
    SIMPLE_BILINEAR_FAST_PATH_COVER (op,s,d,func),			\
    SIMPLE_BILINEAR_FAST_PATH_NONE (op,s,d,func),			\
    SIMPLE_BILINEAR_FAST_PATH_PAD (op,s,d,func),			\
    SIMPLE_BILINEAR_FAST_PATH_NORMAL (op,s,d,func)

#define SIMPLE_BILINEAR_A8_MASK_FAST_PATH(op,s,d,func)			\
    SIMPLE_BILINEAR_A8_MASK_FAST_PATH_COVER (op,s,d,func),		\
    SIMPLE_BILINEAR_A8_MASK_FAST_PATH_NONE (op,s,d,func),		\
    SIMPLE_BILINEAR_A8_MASK_FAST_PATH_PAD (op,s,d,func),		\
    SIMPLE_BILINEAR_A8_MASK_FAST_PATH_NORMAL (op,s,d,func)

#define SIMPLE_BILINEAR_SOLID_MASK_FAST_PATH(op,s,d,func)		\
    SIMPLE_BILINEAR_SOLID_MASK_FAST_PATH_COVER (op,s,d,func),		\
    SIMPLE_BILINEAR_SOLID_MASK_FAST_PATH_NONE (op,s,d,func),		\
    SIMPLE_BILINEAR_SOLID_MASK_FAST_PATH_PAD (op,s,d,func),		\
    SIMPLE_BILINEAR_SOLID_MASK_FAST_PATH_NORMAL (op,s,d,func)

#endif
