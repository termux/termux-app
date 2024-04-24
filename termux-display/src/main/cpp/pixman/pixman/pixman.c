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

#ifdef HAVE_CONFIG_H
#include <pixman-config.h>
#endif
#include "pixman-private.h"

#include <stdlib.h>

pixman_implementation_t *global_implementation;

#ifdef TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR
static void __attribute__((constructor))
pixman_constructor (void)
{
    global_implementation = _pixman_choose_implementation ();
}
#endif

typedef struct operator_info_t operator_info_t;

struct operator_info_t
{
    uint8_t	opaque_info[4];
};

#define PACK(neither, src, dest, both)			\
    {{	    (uint8_t)PIXMAN_OP_ ## neither,		\
	    (uint8_t)PIXMAN_OP_ ## src,			\
	    (uint8_t)PIXMAN_OP_ ## dest,		\
	    (uint8_t)PIXMAN_OP_ ## both		}}

static const operator_info_t operator_table[] =
{
    /*    Neither Opaque         Src Opaque             Dst Opaque             Both Opaque */
    PACK (CLEAR,                 CLEAR,                 CLEAR,                 CLEAR),
    PACK (SRC,                   SRC,                   SRC,                   SRC),
    PACK (DST,                   DST,                   DST,                   DST),
    PACK (OVER,                  SRC,                   OVER,                  SRC),
    PACK (OVER_REVERSE,          OVER_REVERSE,          DST,                   DST),
    PACK (IN,                    IN,                    SRC,                   SRC),
    PACK (IN_REVERSE,            DST,                   IN_REVERSE,            DST),
    PACK (OUT,                   OUT,                   CLEAR,                 CLEAR),
    PACK (OUT_REVERSE,           CLEAR,                 OUT_REVERSE,           CLEAR),
    PACK (ATOP,                  IN,                    OVER,                  SRC),
    PACK (ATOP_REVERSE,          OVER_REVERSE,          IN_REVERSE,            DST),
    PACK (XOR,                   OUT,                   OUT_REVERSE,           CLEAR),
    PACK (ADD,                   ADD,                   ADD,                   ADD),
    PACK (SATURATE,              OVER_REVERSE,          DST,                   DST),

    {{ 0 /* 0x0e */ }},
    {{ 0 /* 0x0f */ }},

    PACK (CLEAR,                 CLEAR,                 CLEAR,                 CLEAR),
    PACK (SRC,                   SRC,                   SRC,                   SRC),
    PACK (DST,                   DST,                   DST,                   DST),
    PACK (DISJOINT_OVER,         DISJOINT_OVER,         DISJOINT_OVER,         DISJOINT_OVER),
    PACK (DISJOINT_OVER_REVERSE, DISJOINT_OVER_REVERSE, DISJOINT_OVER_REVERSE, DISJOINT_OVER_REVERSE),
    PACK (DISJOINT_IN,           DISJOINT_IN,           DISJOINT_IN,           DISJOINT_IN),
    PACK (DISJOINT_IN_REVERSE,   DISJOINT_IN_REVERSE,   DISJOINT_IN_REVERSE,   DISJOINT_IN_REVERSE),
    PACK (DISJOINT_OUT,          DISJOINT_OUT,          DISJOINT_OUT,          DISJOINT_OUT),
    PACK (DISJOINT_OUT_REVERSE,  DISJOINT_OUT_REVERSE,  DISJOINT_OUT_REVERSE,  DISJOINT_OUT_REVERSE),
    PACK (DISJOINT_ATOP,         DISJOINT_ATOP,         DISJOINT_ATOP,         DISJOINT_ATOP),
    PACK (DISJOINT_ATOP_REVERSE, DISJOINT_ATOP_REVERSE, DISJOINT_ATOP_REVERSE, DISJOINT_ATOP_REVERSE),
    PACK (DISJOINT_XOR,          DISJOINT_XOR,          DISJOINT_XOR,          DISJOINT_XOR),

    {{ 0 /* 0x1c */ }},
    {{ 0 /* 0x1d */ }},
    {{ 0 /* 0x1e */ }},
    {{ 0 /* 0x1f */ }},

    PACK (CLEAR,                 CLEAR,                 CLEAR,                 CLEAR),
    PACK (SRC,                   SRC,                   SRC,                   SRC),
    PACK (DST,                   DST,                   DST,                   DST),
    PACK (CONJOINT_OVER,         CONJOINT_OVER,         CONJOINT_OVER,         CONJOINT_OVER),
    PACK (CONJOINT_OVER_REVERSE, CONJOINT_OVER_REVERSE, CONJOINT_OVER_REVERSE, CONJOINT_OVER_REVERSE),
    PACK (CONJOINT_IN,           CONJOINT_IN,           CONJOINT_IN,           CONJOINT_IN),
    PACK (CONJOINT_IN_REVERSE,   CONJOINT_IN_REVERSE,   CONJOINT_IN_REVERSE,   CONJOINT_IN_REVERSE),
    PACK (CONJOINT_OUT,          CONJOINT_OUT,          CONJOINT_OUT,          CONJOINT_OUT),
    PACK (CONJOINT_OUT_REVERSE,  CONJOINT_OUT_REVERSE,  CONJOINT_OUT_REVERSE,  CONJOINT_OUT_REVERSE),
    PACK (CONJOINT_ATOP,         CONJOINT_ATOP,         CONJOINT_ATOP,         CONJOINT_ATOP),
    PACK (CONJOINT_ATOP_REVERSE, CONJOINT_ATOP_REVERSE, CONJOINT_ATOP_REVERSE, CONJOINT_ATOP_REVERSE),
    PACK (CONJOINT_XOR,          CONJOINT_XOR,          CONJOINT_XOR,          CONJOINT_XOR),

    {{ 0 /* 0x2c */ }},
    {{ 0 /* 0x2d */ }},
    {{ 0 /* 0x2e */ }},
    {{ 0 /* 0x2f */ }},

    PACK (MULTIPLY,              MULTIPLY,              MULTIPLY,              MULTIPLY),
    PACK (SCREEN,                SCREEN,                SCREEN,                SCREEN),
    PACK (OVERLAY,               OVERLAY,               OVERLAY,               OVERLAY),
    PACK (DARKEN,                DARKEN,                DARKEN,                DARKEN),
    PACK (LIGHTEN,               LIGHTEN,               LIGHTEN,               LIGHTEN),
    PACK (COLOR_DODGE,           COLOR_DODGE,           COLOR_DODGE,           COLOR_DODGE),
    PACK (COLOR_BURN,            COLOR_BURN,            COLOR_BURN,            COLOR_BURN),
    PACK (HARD_LIGHT,            HARD_LIGHT,            HARD_LIGHT,            HARD_LIGHT),
    PACK (SOFT_LIGHT,            SOFT_LIGHT,            SOFT_LIGHT,            SOFT_LIGHT),
    PACK (DIFFERENCE,            DIFFERENCE,            DIFFERENCE,            DIFFERENCE),
    PACK (EXCLUSION,             EXCLUSION,             EXCLUSION,             EXCLUSION),
    PACK (HSL_HUE,               HSL_HUE,               HSL_HUE,               HSL_HUE),
    PACK (HSL_SATURATION,        HSL_SATURATION,        HSL_SATURATION,        HSL_SATURATION),
    PACK (HSL_COLOR,             HSL_COLOR,             HSL_COLOR,             HSL_COLOR),
    PACK (HSL_LUMINOSITY,        HSL_LUMINOSITY,        HSL_LUMINOSITY,        HSL_LUMINOSITY),
};

/*
 * Optimize the current operator based on opacity of source or destination
 * The output operator should be mathematically equivalent to the source.
 */
static pixman_op_t
optimize_operator (pixman_op_t     op,
		   uint32_t        src_flags,
		   uint32_t        mask_flags,
		   uint32_t        dst_flags)
{
    pixman_bool_t is_source_opaque, is_dest_opaque;

#define OPAQUE_SHIFT 13
    
    COMPILE_TIME_ASSERT (FAST_PATH_IS_OPAQUE == (1 << OPAQUE_SHIFT));
    
    is_dest_opaque = (dst_flags & FAST_PATH_IS_OPAQUE);
    is_source_opaque = ((src_flags & mask_flags) & FAST_PATH_IS_OPAQUE);

    is_dest_opaque >>= OPAQUE_SHIFT - 1;
    is_source_opaque >>= OPAQUE_SHIFT;

    return operator_table[op].opaque_info[is_dest_opaque | is_source_opaque];
}

/*
 * Computing composite region
 */
static inline pixman_bool_t
clip_general_image (pixman_region32_t * region,
                    pixman_region32_t * clip,
                    int                 dx,
                    int                 dy)
{
    if (pixman_region32_n_rects (region) == 1 &&
        pixman_region32_n_rects (clip) == 1)
    {
	pixman_box32_t *  rbox = pixman_region32_rectangles (region, NULL);
	pixman_box32_t *  cbox = pixman_region32_rectangles (clip, NULL);
	int v;

	if (rbox->x1 < (v = cbox->x1 + dx))
	    rbox->x1 = v;
	if (rbox->x2 > (v = cbox->x2 + dx))
	    rbox->x2 = v;
	if (rbox->y1 < (v = cbox->y1 + dy))
	    rbox->y1 = v;
	if (rbox->y2 > (v = cbox->y2 + dy))
	    rbox->y2 = v;
	if (rbox->x1 >= rbox->x2 || rbox->y1 >= rbox->y2)
	{
	    pixman_region32_init (region);
	    return FALSE;
	}
    }
    else if (pixman_region32_empty (clip))
    {
	return FALSE;
    }
    else
    {
	if (dx || dy)
	    pixman_region32_translate (region, -dx, -dy);

	if (!pixman_region32_intersect (region, region, clip))
	    return FALSE;

	if (dx || dy)
	    pixman_region32_translate (region, dx, dy);
    }

    return pixman_region32_not_empty (region);
}

static inline pixman_bool_t
clip_source_image (pixman_region32_t * region,
                   pixman_image_t *    image,
                   int                 dx,
                   int                 dy)
{
    /* Source clips are ignored, unless they are explicitly turned on
     * and the clip in question was set by an X client. (Because if
     * the clip was not set by a client, then it is a hierarchy
     * clip and those should always be ignored for sources).
     */
    if (!image->common.clip_sources || !image->common.client_clip)
	return TRUE;

    return clip_general_image (region,
                               &image->common.clip_region,
                               dx, dy);
}

/*
 * returns FALSE if the final region is empty.  Indistinguishable from
 * an allocation failure, but rendering ignores those anyways.
 */
pixman_bool_t
_pixman_compute_composite_region32 (pixman_region32_t * region,
				    pixman_image_t *    src_image,
				    pixman_image_t *    mask_image,
				    pixman_image_t *    dest_image,
				    int32_t             src_x,
				    int32_t             src_y,
				    int32_t             mask_x,
				    int32_t             mask_y,
				    int32_t             dest_x,
				    int32_t             dest_y,
				    int32_t             width,
				    int32_t             height)
{
    region->extents.x1 = dest_x;
    region->extents.x2 = dest_x + width;
    region->extents.y1 = dest_y;
    region->extents.y2 = dest_y + height;

    region->extents.x1 = MAX (region->extents.x1, 0);
    region->extents.y1 = MAX (region->extents.y1, 0);
    region->extents.x2 = MIN (region->extents.x2, dest_image->bits.width);
    region->extents.y2 = MIN (region->extents.y2, dest_image->bits.height);

    region->data = 0;

    /* Check for empty operation */
    if (region->extents.x1 >= region->extents.x2 ||
        region->extents.y1 >= region->extents.y2)
    {
	region->extents.x1 = 0;
	region->extents.x2 = 0;
	region->extents.y1 = 0;
	region->extents.y2 = 0;
	return FALSE;
    }

    if (dest_image->common.have_clip_region)
    {
	if (!clip_general_image (region, &dest_image->common.clip_region, 0, 0))
	    return FALSE;
    }

    if (dest_image->common.alpha_map)
    {
	if (!pixman_region32_intersect_rect (region, region,
					     dest_image->common.alpha_origin_x,
					     dest_image->common.alpha_origin_y,
					     dest_image->common.alpha_map->width,
					     dest_image->common.alpha_map->height))
	{
	    return FALSE;
	}
	if (pixman_region32_empty (region))
	    return FALSE;
	if (dest_image->common.alpha_map->common.have_clip_region)
	{
	    if (!clip_general_image (region, &dest_image->common.alpha_map->common.clip_region,
				     -dest_image->common.alpha_origin_x,
				     -dest_image->common.alpha_origin_y))
	    {
		return FALSE;
	    }
	}
    }

    /* clip against src */
    if (src_image->common.have_clip_region)
    {
	if (!clip_source_image (region, src_image, dest_x - src_x, dest_y - src_y))
	    return FALSE;
    }
    if (src_image->common.alpha_map && src_image->common.alpha_map->common.have_clip_region)
    {
	if (!clip_source_image (region, (pixman_image_t *)src_image->common.alpha_map,
	                        dest_x - (src_x - src_image->common.alpha_origin_x),
	                        dest_y - (src_y - src_image->common.alpha_origin_y)))
	{
	    return FALSE;
	}
    }
    /* clip against mask */
    if (mask_image && mask_image->common.have_clip_region)
    {
	if (!clip_source_image (region, mask_image, dest_x - mask_x, dest_y - mask_y))
	    return FALSE;

	if (mask_image->common.alpha_map && mask_image->common.alpha_map->common.have_clip_region)
	{
	    if (!clip_source_image (region, (pixman_image_t *)mask_image->common.alpha_map,
	                            dest_x - (mask_x - mask_image->common.alpha_origin_x),
	                            dest_y - (mask_y - mask_image->common.alpha_origin_y)))
	    {
		return FALSE;
	    }
	}
    }

    return TRUE;
}

typedef struct box_48_16 box_48_16_t;

struct box_48_16
{
    pixman_fixed_48_16_t        x1;
    pixman_fixed_48_16_t        y1;
    pixman_fixed_48_16_t        x2;
    pixman_fixed_48_16_t        y2;
};

static pixman_bool_t
compute_transformed_extents (pixman_transform_t   *transform,
			     const pixman_box32_t *extents,
			     box_48_16_t          *transformed)
{
    pixman_fixed_48_16_t tx1, ty1, tx2, ty2;
    pixman_fixed_t x1, y1, x2, y2;
    int i;

    x1 = pixman_int_to_fixed (extents->x1) + pixman_fixed_1 / 2;
    y1 = pixman_int_to_fixed (extents->y1) + pixman_fixed_1 / 2;
    x2 = pixman_int_to_fixed (extents->x2) - pixman_fixed_1 / 2;
    y2 = pixman_int_to_fixed (extents->y2) - pixman_fixed_1 / 2;

    if (!transform)
    {
	transformed->x1 = x1;
	transformed->y1 = y1;
	transformed->x2 = x2;
	transformed->y2 = y2;

	return TRUE;
    }

    tx1 = ty1 = INT64_MAX;
    tx2 = ty2 = INT64_MIN;

    for (i = 0; i < 4; ++i)
    {
	pixman_fixed_48_16_t tx, ty;
	pixman_vector_t v;

	v.vector[0] = (i & 0x01)? x1 : x2;
	v.vector[1] = (i & 0x02)? y1 : y2;
	v.vector[2] = pixman_fixed_1;

	if (!pixman_transform_point (transform, &v))
	    return FALSE;

	tx = (pixman_fixed_48_16_t)v.vector[0];
	ty = (pixman_fixed_48_16_t)v.vector[1];

	if (tx < tx1)
	    tx1 = tx;
	if (ty < ty1)
	    ty1 = ty;
	if (tx > tx2)
	    tx2 = tx;
	if (ty > ty2)
	    ty2 = ty;
    }

    transformed->x1 = tx1;
    transformed->y1 = ty1;
    transformed->x2 = tx2;
    transformed->y2 = ty2;

    return TRUE;
}

#define IS_16BIT(x) (((x) >= INT16_MIN) && ((x) <= INT16_MAX))
#define ABS(f)      (((f) < 0)?  (-(f)) : (f))
#define IS_16_16(f) (((f) >= pixman_min_fixed_48_16 && ((f) <= pixman_max_fixed_48_16)))

static pixman_bool_t
analyze_extent (pixman_image_t       *image,
		const pixman_box32_t *extents,
		uint32_t             *flags)
{
    pixman_transform_t *transform;
    pixman_fixed_t x_off, y_off;
    pixman_fixed_t width, height;
    pixman_fixed_t *params;
    box_48_16_t transformed;
    pixman_box32_t exp_extents;

    if (!image)
	return TRUE;

    /* Some compositing functions walk one step
     * outside the destination rectangle, so we
     * check here that the expanded-by-one source
     * extents in destination space fits in 16 bits
     */
    if (!IS_16BIT (extents->x1 - 1)		||
	!IS_16BIT (extents->y1 - 1)		||
	!IS_16BIT (extents->x2 + 1)		||
	!IS_16BIT (extents->y2 + 1))
    {
	return FALSE;
    }

    transform = image->common.transform;
    if (image->common.type == BITS)
    {
	/* During repeat mode calculations we might convert the
	 * width/height of an image to fixed 16.16, so we need
	 * them to be smaller than 16 bits.
	 */
	if (image->bits.width >= 0x7fff	|| image->bits.height >= 0x7fff)
	    return FALSE;

	if ((image->common.flags & FAST_PATH_ID_TRANSFORM) == FAST_PATH_ID_TRANSFORM &&
	    extents->x1 >= 0 &&
	    extents->y1 >= 0 &&
	    extents->x2 <= image->bits.width &&
	    extents->y2 <= image->bits.height)
	{
	    *flags |= FAST_PATH_SAMPLES_COVER_CLIP_NEAREST;
	    return TRUE;
	}

	switch (image->common.filter)
	{
	case PIXMAN_FILTER_CONVOLUTION:
	    params = image->common.filter_params;
	    x_off = - pixman_fixed_e - ((params[0] - pixman_fixed_1) >> 1);
	    y_off = - pixman_fixed_e - ((params[1] - pixman_fixed_1) >> 1);
	    width = params[0];
	    height = params[1];
	    break;

	case PIXMAN_FILTER_SEPARABLE_CONVOLUTION:
	    params = image->common.filter_params;
	    x_off = - pixman_fixed_e - ((params[0] - pixman_fixed_1) >> 1);
	    y_off = - pixman_fixed_e - ((params[1] - pixman_fixed_1) >> 1);
	    width = params[0];
	    height = params[1];
	    break;
	    
	case PIXMAN_FILTER_GOOD:
	case PIXMAN_FILTER_BEST:
	case PIXMAN_FILTER_BILINEAR:
	    x_off = - pixman_fixed_1 / 2;
	    y_off = - pixman_fixed_1 / 2;
	    width = pixman_fixed_1;
	    height = pixman_fixed_1;
	    break;

	case PIXMAN_FILTER_FAST:
	case PIXMAN_FILTER_NEAREST:
	    x_off = - pixman_fixed_e;
	    y_off = - pixman_fixed_e;
	    width = 0;
	    height = 0;
	    break;

	default:
	    return FALSE;
	}
    }
    else
    {
	x_off = 0;
	y_off = 0;
	width = 0;
	height = 0;
    }

    if (!compute_transformed_extents (transform, extents, &transformed))
	return FALSE;

    if (image->common.type == BITS)
    {
	if (pixman_fixed_to_int (transformed.x1 - pixman_fixed_e) >= 0                &&
	    pixman_fixed_to_int (transformed.y1 - pixman_fixed_e) >= 0                &&
	    pixman_fixed_to_int (transformed.x2 - pixman_fixed_e) < image->bits.width &&
	    pixman_fixed_to_int (transformed.y2 - pixman_fixed_e) < image->bits.height)
	{
	    *flags |= FAST_PATH_SAMPLES_COVER_CLIP_NEAREST;
	}

	if (pixman_fixed_to_int (transformed.x1 - pixman_fixed_1 / 2) >= 0		  &&
	    pixman_fixed_to_int (transformed.y1 - pixman_fixed_1 / 2) >= 0		  &&
	    pixman_fixed_to_int (transformed.x2 + pixman_fixed_1 / 2) < image->bits.width &&
	    pixman_fixed_to_int (transformed.y2 + pixman_fixed_1 / 2) < image->bits.height)
	{
	    *flags |= FAST_PATH_SAMPLES_COVER_CLIP_BILINEAR;
	}
    }

    /* Check we don't overflow when the destination extents are expanded by one.
     * This ensures that compositing functions can simply walk the source space
     * using 16.16 variables without worrying about overflow.
     */
    exp_extents = *extents;
    exp_extents.x1 -= 1;
    exp_extents.y1 -= 1;
    exp_extents.x2 += 1;
    exp_extents.y2 += 1;

    if (!compute_transformed_extents (transform, &exp_extents, &transformed))
	return FALSE;
    
    if (!IS_16_16 (transformed.x1 + x_off - 8 * pixman_fixed_e)	||
	!IS_16_16 (transformed.y1 + y_off - 8 * pixman_fixed_e)	||
	!IS_16_16 (transformed.x2 + x_off + 8 * pixman_fixed_e + width)	||
	!IS_16_16 (transformed.y2 + y_off + 8 * pixman_fixed_e + height))
    {
	return FALSE;
    }

    return TRUE;
}

/*
 * Work around GCC bug causing crashes in Mozilla with SSE2
 *
 * When using -msse, gcc generates movdqa instructions assuming that
 * the stack is 16 byte aligned. Unfortunately some applications, such
 * as Mozilla and Mono, end up aligning the stack to 4 bytes, which
 * causes the movdqa instructions to fail.
 *
 * The __force_align_arg_pointer__ makes gcc generate a prologue that
 * realigns the stack pointer to 16 bytes.
 *
 * On x86-64 this is not necessary because the standard ABI already
 * calls for a 16 byte aligned stack.
 *
 * See https://bugs.freedesktop.org/show_bug.cgi?id=15693
 */
#if defined (USE_SSE2) && defined(__GNUC__) && !defined(__x86_64__) && !defined(__amd64__)
__attribute__((__force_align_arg_pointer__))
#endif
PIXMAN_EXPORT void
pixman_image_composite32 (pixman_op_t      op,
                          pixman_image_t * src,
                          pixman_image_t * mask,
                          pixman_image_t * dest,
                          int32_t          src_x,
                          int32_t          src_y,
                          int32_t          mask_x,
                          int32_t          mask_y,
                          int32_t          dest_x,
                          int32_t          dest_y,
                          int32_t          width,
                          int32_t          height)
{
    pixman_format_code_t src_format, mask_format, dest_format;
    pixman_region32_t region;
    pixman_box32_t extents;
    pixman_implementation_t *imp;
    pixman_composite_func_t func;
    pixman_composite_info_t info;
    const pixman_box32_t *pbox;
    int n;

    _pixman_image_validate (src);
    if (mask)
	_pixman_image_validate (mask);
    _pixman_image_validate (dest);

    src_format = src->common.extended_format_code;
    info.src_flags = src->common.flags;

    if (mask && !(mask->common.flags & FAST_PATH_IS_OPAQUE))
    {
	mask_format = mask->common.extended_format_code;
	info.mask_flags = mask->common.flags;
    }
    else
    {
	mask_format = PIXMAN_null;
	info.mask_flags = FAST_PATH_IS_OPAQUE | FAST_PATH_NO_ALPHA_MAP;
    }

    dest_format = dest->common.extended_format_code;
    info.dest_flags = dest->common.flags;

    /* Check for pixbufs */
    if ((mask_format == PIXMAN_a8r8g8b8 || mask_format == PIXMAN_a8b8g8r8) &&
	(src->type == BITS && src->bits.bits == mask->bits.bits)	   &&
	(src->common.repeat == mask->common.repeat)			   &&
	(info.src_flags & info.mask_flags & FAST_PATH_ID_TRANSFORM)	   &&
	(src_x == mask_x && src_y == mask_y))
    {
	if (src_format == PIXMAN_x8b8g8r8)
	    src_format = mask_format = PIXMAN_pixbuf;
	else if (src_format == PIXMAN_x8r8g8b8)
	    src_format = mask_format = PIXMAN_rpixbuf;
    }

    pixman_region32_init (&region);

    if (!_pixman_compute_composite_region32 (
	    &region, src, mask, dest,
	    src_x, src_y, mask_x, mask_y, dest_x, dest_y, width, height))
    {
	goto out;
    }

    extents = *pixman_region32_extents (&region);

    extents.x1 -= dest_x - src_x;
    extents.y1 -= dest_y - src_y;
    extents.x2 -= dest_x - src_x;
    extents.y2 -= dest_y - src_y;

    if (!analyze_extent (src, &extents, &info.src_flags))
	goto out;

    extents.x1 -= src_x - mask_x;
    extents.y1 -= src_y - mask_y;
    extents.x2 -= src_x - mask_x;
    extents.y2 -= src_y - mask_y;

    if (!analyze_extent (mask, &extents, &info.mask_flags))
	goto out;

    /* If the clip is within the source samples, and the samples are
     * opaque, then the source is effectively opaque.
     */
#define NEAREST_OPAQUE	(FAST_PATH_SAMPLES_OPAQUE |			\
			 FAST_PATH_NEAREST_FILTER |			\
			 FAST_PATH_SAMPLES_COVER_CLIP_NEAREST)
#define BILINEAR_OPAQUE	(FAST_PATH_SAMPLES_OPAQUE |			\
			 FAST_PATH_BILINEAR_FILTER |			\
			 FAST_PATH_SAMPLES_COVER_CLIP_BILINEAR)

    if ((info.src_flags & NEAREST_OPAQUE) == NEAREST_OPAQUE ||
	(info.src_flags & BILINEAR_OPAQUE) == BILINEAR_OPAQUE)
    {
	info.src_flags |= FAST_PATH_IS_OPAQUE;
    }

    if ((info.mask_flags & NEAREST_OPAQUE) == NEAREST_OPAQUE ||
	(info.mask_flags & BILINEAR_OPAQUE) == BILINEAR_OPAQUE)
    {
	info.mask_flags |= FAST_PATH_IS_OPAQUE;
    }

    /*
     * Check if we can replace our operator by a simpler one
     * if the src or dest are opaque. The output operator should be
     * mathematically equivalent to the source.
     */
    info.op = optimize_operator (op, info.src_flags, info.mask_flags, info.dest_flags);

    _pixman_implementation_lookup_composite (
	get_implementation (), info.op,
	src_format, info.src_flags,
	mask_format, info.mask_flags,
	dest_format, info.dest_flags,
	&imp, &func);

    info.src_image = src;
    info.mask_image = mask;
    info.dest_image = dest;

    pbox = pixman_region32_rectangles (&region, &n);

    while (n--)
    {
	info.src_x = pbox->x1 + src_x - dest_x;
	info.src_y = pbox->y1 + src_y - dest_y;
	info.mask_x = pbox->x1 + mask_x - dest_x;
	info.mask_y = pbox->y1 + mask_y - dest_y;
	info.dest_x = pbox->x1;
	info.dest_y = pbox->y1;
	info.width = pbox->x2 - pbox->x1;
	info.height = pbox->y2 - pbox->y1;

	func (imp, &info);

	pbox++;
    }

out:
    pixman_region32_fini (&region);
}

PIXMAN_EXPORT void
pixman_image_composite (pixman_op_t      op,
                        pixman_image_t * src,
                        pixman_image_t * mask,
                        pixman_image_t * dest,
                        int16_t          src_x,
                        int16_t          src_y,
                        int16_t          mask_x,
                        int16_t          mask_y,
                        int16_t          dest_x,
                        int16_t          dest_y,
                        uint16_t         width,
                        uint16_t         height)
{
    pixman_image_composite32 (op, src, mask, dest, src_x, src_y, 
                              mask_x, mask_y, dest_x, dest_y, width, height);
}

PIXMAN_EXPORT pixman_bool_t
pixman_blt (uint32_t *src_bits,
            uint32_t *dst_bits,
            int       src_stride,
            int       dst_stride,
            int       src_bpp,
            int       dst_bpp,
            int       src_x,
            int       src_y,
            int       dest_x,
            int       dest_y,
            int       width,
            int       height)
{
    return _pixman_implementation_blt (get_implementation(),
				       src_bits, dst_bits, src_stride, dst_stride,
                                       src_bpp, dst_bpp,
                                       src_x, src_y,
                                       dest_x, dest_y,
                                       width, height);
}

PIXMAN_EXPORT pixman_bool_t
pixman_fill (uint32_t *bits,
             int       stride,
             int       bpp,
             int       x,
             int       y,
             int       width,
             int       height,
             uint32_t  filler)
{
    return _pixman_implementation_fill (
	get_implementation(), bits, stride, bpp, x, y, width, height, filler);
}

static uint32_t
color_to_uint32 (const pixman_color_t *color)
{
    return
        (color->alpha >> 8 << 24) |
        (color->red >> 8 << 16) |
        (color->green & 0xff00) |
        (color->blue >> 8);
}

static pixman_bool_t
color_to_pixel (const pixman_color_t *color,
                uint32_t *            pixel,
                pixman_format_code_t  format)
{
    uint32_t c = color_to_uint32 (color);

    if (PIXMAN_FORMAT_TYPE (format) == PIXMAN_TYPE_RGBA_FLOAT)
    {
	return FALSE;
    }

    if (!(format == PIXMAN_a8r8g8b8     ||
          format == PIXMAN_x8r8g8b8     ||
          format == PIXMAN_a8b8g8r8     ||
          format == PIXMAN_x8b8g8r8     ||
          format == PIXMAN_b8g8r8a8     ||
          format == PIXMAN_b8g8r8x8     ||
          format == PIXMAN_r8g8b8a8     ||
          format == PIXMAN_r8g8b8x8     ||
          format == PIXMAN_r5g6b5       ||
          format == PIXMAN_b5g6r5       ||
          format == PIXMAN_a8           ||
          format == PIXMAN_a1))
    {
	return FALSE;
    }

    if (PIXMAN_FORMAT_TYPE (format) == PIXMAN_TYPE_ABGR)
    {
	c = ((c & 0xff000000) >>  0) |
	    ((c & 0x00ff0000) >> 16) |
	    ((c & 0x0000ff00) >>  0) |
	    ((c & 0x000000ff) << 16);
    }
    if (PIXMAN_FORMAT_TYPE (format) == PIXMAN_TYPE_BGRA)
    {
	c = ((c & 0xff000000) >> 24) |
	    ((c & 0x00ff0000) >>  8) |
	    ((c & 0x0000ff00) <<  8) |
	    ((c & 0x000000ff) << 24);
    }
    if (PIXMAN_FORMAT_TYPE (format) == PIXMAN_TYPE_RGBA)
	c = ((c & 0xff000000) >> 24) | (c << 8);

    if (format == PIXMAN_a1)
	c = c >> 31;
    else if (format == PIXMAN_a8)
	c = c >> 24;
    else if (format == PIXMAN_r5g6b5 ||
             format == PIXMAN_b5g6r5)
	c = convert_8888_to_0565 (c);

#if 0
    printf ("color: %x %x %x %x\n", color->alpha, color->red, color->green, color->blue);
    printf ("pixel: %x\n", c);
#endif

    *pixel = c;
    return TRUE;
}

PIXMAN_EXPORT pixman_bool_t
pixman_image_fill_rectangles (pixman_op_t                 op,
                              pixman_image_t *            dest,
			      const pixman_color_t *      color,
                              int                         n_rects,
                              const pixman_rectangle16_t *rects)
{
    pixman_box32_t stack_boxes[6];
    pixman_box32_t *boxes;
    pixman_bool_t result;
    int i;

    if (n_rects > 6)
    {
        boxes = pixman_malloc_ab (sizeof (pixman_box32_t), n_rects);
        if (boxes == NULL)
            return FALSE;
    }
    else
    {
        boxes = stack_boxes;
    }

    for (i = 0; i < n_rects; ++i)
    {
        boxes[i].x1 = rects[i].x;
        boxes[i].y1 = rects[i].y;
        boxes[i].x2 = boxes[i].x1 + rects[i].width;
        boxes[i].y2 = boxes[i].y1 + rects[i].height;
    }

    result = pixman_image_fill_boxes (op, dest, color, n_rects, boxes);

    if (boxes != stack_boxes)
        free (boxes);
    
    return result;
}

PIXMAN_EXPORT pixman_bool_t
pixman_image_fill_boxes (pixman_op_t           op,
                         pixman_image_t *      dest,
                         const pixman_color_t *color,
                         int                   n_boxes,
                         const pixman_box32_t *boxes)
{
    pixman_image_t *solid;
    pixman_color_t c;
    int i;

    _pixman_image_validate (dest);
    
    if (color->alpha == 0xffff)
    {
        if (op == PIXMAN_OP_OVER)
            op = PIXMAN_OP_SRC;
    }

    if (op == PIXMAN_OP_CLEAR)
    {
        c.red = 0;
        c.green = 0;
        c.blue = 0;
        c.alpha = 0;

        color = &c;

        op = PIXMAN_OP_SRC;
    }

    if (op == PIXMAN_OP_SRC)
    {
        uint32_t pixel;

        if (color_to_pixel (color, &pixel, dest->bits.format))
        {
            pixman_region32_t fill_region;
            int n_rects, j;
            pixman_box32_t *rects;

            if (!pixman_region32_init_rects (&fill_region, boxes, n_boxes))
                return FALSE;

            if (dest->common.have_clip_region)
            {
                if (!pixman_region32_intersect (&fill_region,
                                                &fill_region,
                                                &dest->common.clip_region))
                    return FALSE;
            }

            rects = pixman_region32_rectangles (&fill_region, &n_rects);
            for (j = 0; j < n_rects; ++j)
            {
                const pixman_box32_t *rect = &(rects[j]);
                pixman_fill (dest->bits.bits, dest->bits.rowstride, PIXMAN_FORMAT_BPP (dest->bits.format),
                             rect->x1, rect->y1, rect->x2 - rect->x1, rect->y2 - rect->y1,
                             pixel);
            }

            pixman_region32_fini (&fill_region);
            return TRUE;
        }
    }

    solid = pixman_image_create_solid_fill (color);
    if (!solid)
        return FALSE;

    for (i = 0; i < n_boxes; ++i)
    {
        const pixman_box32_t *box = &(boxes[i]);

        pixman_image_composite32 (op, solid, NULL, dest,
                                  0, 0, 0, 0,
                                  box->x1, box->y1,
                                  box->x2 - box->x1, box->y2 - box->y1);
    }

    pixman_image_unref (solid);

    return TRUE;
}

/**
 * pixman_version:
 *
 * Returns the version of the pixman library encoded in a single
 * integer as per %PIXMAN_VERSION_ENCODE. The encoding ensures that
 * later versions compare greater than earlier versions.
 *
 * A run-time comparison to check that pixman's version is greater than
 * or equal to version X.Y.Z could be performed as follows:
 *
 * <informalexample><programlisting>
 * if (pixman_version() >= PIXMAN_VERSION_ENCODE(X,Y,Z)) {...}
 * </programlisting></informalexample>
 *
 * See also pixman_version_string() as well as the compile-time
 * equivalents %PIXMAN_VERSION and %PIXMAN_VERSION_STRING.
 *
 * Return value: the encoded version.
 **/
PIXMAN_EXPORT int
pixman_version (void)
{
    return PIXMAN_VERSION;
}

/**
 * pixman_version_string:
 *
 * Returns the version of the pixman library as a human-readable string
 * of the form "X.Y.Z".
 *
 * See also pixman_version() as well as the compile-time equivalents
 * %PIXMAN_VERSION_STRING and %PIXMAN_VERSION.
 *
 * Return value: a string containing the version.
 **/
PIXMAN_EXPORT const char*
pixman_version_string (void)
{
    return PIXMAN_VERSION_STRING;
}

/**
 * pixman_format_supported_source:
 * @format: A pixman_format_code_t format
 *
 * Return value: whether the provided format code is a supported
 * format for a pixman surface used as a source in
 * rendering.
 *
 * Currently, all pixman_format_code_t values are supported.
 **/
PIXMAN_EXPORT pixman_bool_t
pixman_format_supported_source (pixman_format_code_t format)
{
    switch (format)
    {
    /* 32 bpp formats */
    case PIXMAN_a2b10g10r10:
    case PIXMAN_x2b10g10r10:
    case PIXMAN_a2r10g10b10:
    case PIXMAN_x2r10g10b10:
    case PIXMAN_a8r8g8b8:
    case PIXMAN_a8r8g8b8_sRGB:
    case PIXMAN_r8g8b8_sRGB:
    case PIXMAN_x8r8g8b8:
    case PIXMAN_a8b8g8r8:
    case PIXMAN_x8b8g8r8:
    case PIXMAN_b8g8r8a8:
    case PIXMAN_b8g8r8x8:
    case PIXMAN_r8g8b8a8:
    case PIXMAN_r8g8b8x8:
    case PIXMAN_r8g8b8:
    case PIXMAN_b8g8r8:
    case PIXMAN_r5g6b5:
    case PIXMAN_b5g6r5:
    case PIXMAN_x14r6g6b6:
    /* 16 bpp formats */
    case PIXMAN_a1r5g5b5:
    case PIXMAN_x1r5g5b5:
    case PIXMAN_a1b5g5r5:
    case PIXMAN_x1b5g5r5:
    case PIXMAN_a4r4g4b4:
    case PIXMAN_x4r4g4b4:
    case PIXMAN_a4b4g4r4:
    case PIXMAN_x4b4g4r4:
    /* 8bpp formats */
    case PIXMAN_a8:
    case PIXMAN_r3g3b2:
    case PIXMAN_b2g3r3:
    case PIXMAN_a2r2g2b2:
    case PIXMAN_a2b2g2r2:
    case PIXMAN_c8:
    case PIXMAN_g8:
    case PIXMAN_x4a4:
    /* Collides with PIXMAN_c8
       case PIXMAN_x4c4:
     */
    /* Collides with PIXMAN_g8
       case PIXMAN_x4g4:
     */
    /* 4bpp formats */
    case PIXMAN_a4:
    case PIXMAN_r1g2b1:
    case PIXMAN_b1g2r1:
    case PIXMAN_a1r1g1b1:
    case PIXMAN_a1b1g1r1:
    case PIXMAN_c4:
    case PIXMAN_g4:
    /* 1bpp formats */
    case PIXMAN_a1:
    case PIXMAN_g1:
    /* YUV formats */
    case PIXMAN_yuy2:
    case PIXMAN_yv12:
	return TRUE;

    default:
	return FALSE;
    }
}

/**
 * pixman_format_supported_destination:
 * @format: A pixman_format_code_t format
 *
 * Return value: whether the provided format code is a supported
 * format for a pixman surface used as a destination in
 * rendering.
 *
 * Currently, all pixman_format_code_t values are supported
 * except for the YUV formats.
 **/
PIXMAN_EXPORT pixman_bool_t
pixman_format_supported_destination (pixman_format_code_t format)
{
    /* YUV formats cannot be written to at the moment */
    if (format == PIXMAN_yuy2 || format == PIXMAN_yv12)
	return FALSE;

    return pixman_format_supported_source (format);
}

PIXMAN_EXPORT pixman_bool_t
pixman_compute_composite_region (pixman_region16_t * region,
                                 pixman_image_t *    src_image,
                                 pixman_image_t *    mask_image,
                                 pixman_image_t *    dest_image,
                                 int16_t             src_x,
                                 int16_t             src_y,
                                 int16_t             mask_x,
                                 int16_t             mask_y,
                                 int16_t             dest_x,
                                 int16_t             dest_y,
                                 uint16_t            width,
                                 uint16_t            height)
{
    pixman_region32_t r32;
    pixman_bool_t retval;

    pixman_region32_init (&r32);

    retval = _pixman_compute_composite_region32 (
	&r32, src_image, mask_image, dest_image,
	src_x, src_y, mask_x, mask_y, dest_x, dest_y,
	width, height);

    if (retval)
    {
	if (!pixman_region16_copy_from_region32 (region, &r32))
	    retval = FALSE;
    }

    pixman_region32_fini (&r32);
    return retval;
}
