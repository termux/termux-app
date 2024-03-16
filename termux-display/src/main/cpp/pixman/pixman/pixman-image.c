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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "pixman-private.h"

static const pixman_color_t transparent_black = { 0, 0, 0, 0 };

static void
gradient_property_changed (pixman_image_t *image)
{
    gradient_t *gradient = &image->gradient;
    int n = gradient->n_stops;
    pixman_gradient_stop_t *stops = gradient->stops;
    pixman_gradient_stop_t *begin = &(gradient->stops[-1]);
    pixman_gradient_stop_t *end = &(gradient->stops[n]);

    switch (gradient->common.repeat)
    {
    default:
    case PIXMAN_REPEAT_NONE:
	begin->x = INT32_MIN;
	begin->color = transparent_black;
	end->x = INT32_MAX;
	end->color = transparent_black;
	break;

    case PIXMAN_REPEAT_NORMAL:
	begin->x = stops[n - 1].x - pixman_fixed_1;
	begin->color = stops[n - 1].color;
	end->x = stops[0].x + pixman_fixed_1;
	end->color = stops[0].color;
	break;

    case PIXMAN_REPEAT_REFLECT:
	begin->x = - stops[0].x;
	begin->color = stops[0].color;
	end->x = pixman_int_to_fixed (2) - stops[n - 1].x;
	end->color = stops[n - 1].color;
	break;

    case PIXMAN_REPEAT_PAD:
	begin->x = INT32_MIN;
	begin->color = stops[0].color;
	end->x = INT32_MAX;
	end->color = stops[n - 1].color;
	break;
    }
}

pixman_bool_t
_pixman_init_gradient (gradient_t *                  gradient,
                       const pixman_gradient_stop_t *stops,
                       int                           n_stops)
{
    return_val_if_fail (n_stops > 0, FALSE);

    /* We allocate two extra stops, one before the beginning of the stop list,
     * and one after the end. These stops are initialized to whatever color
     * would be used for positions outside the range of the stop list.
     *
     * This saves a bit of computation in the gradient walker.
     *
     * The pointer we store in the gradient_t struct still points to the
     * first user-supplied struct, so when freeing, we will have to
     * subtract one.
     */
    gradient->stops =
	pixman_malloc_ab (n_stops + 2, sizeof (pixman_gradient_stop_t));
    if (!gradient->stops)
	return FALSE;

    gradient->stops += 1;
    memcpy (gradient->stops, stops, n_stops * sizeof (pixman_gradient_stop_t));
    gradient->n_stops = n_stops;

    gradient->common.property_changed = gradient_property_changed;

    return TRUE;
}

void
_pixman_image_init (pixman_image_t *image)
{
    image_common_t *common = &image->common;

    pixman_region32_init (&common->clip_region);

    common->alpha_count = 0;
    common->have_clip_region = FALSE;
    common->clip_sources = FALSE;
    common->transform = NULL;
    common->repeat = PIXMAN_REPEAT_NONE;
    common->filter = PIXMAN_FILTER_NEAREST;
    common->filter_params = NULL;
    common->n_filter_params = 0;
    common->alpha_map = NULL;
    common->component_alpha = FALSE;
    common->ref_count = 1;
    common->property_changed = NULL;
    common->client_clip = FALSE;
    common->destroy_func = NULL;
    common->destroy_data = NULL;
    common->dirty = TRUE;
}

pixman_bool_t
_pixman_image_fini (pixman_image_t *image)
{
    image_common_t *common = (image_common_t *)image;

    common->ref_count--;

    if (common->ref_count == 0)
    {
	if (image->common.destroy_func)
	    image->common.destroy_func (image, image->common.destroy_data);

	pixman_region32_fini (&common->clip_region);

	free (common->transform);
	free (common->filter_params);

	if (common->alpha_map)
	    pixman_image_unref ((pixman_image_t *)common->alpha_map);

	if (image->type == LINEAR ||
	    image->type == RADIAL ||
	    image->type == CONICAL)
	{
	    if (image->gradient.stops)
	    {
		/* See _pixman_init_gradient() for an explanation of the - 1 */
		free (image->gradient.stops - 1);
	    }

	    /* This will trigger if someone adds a property_changed
	     * method to the linear/radial/conical gradient overwriting
	     * the general one.
	     */
	    assert (
		image->common.property_changed == gradient_property_changed);
	}

	if (image->type == BITS && image->bits.free_me)
	    free (image->bits.free_me);

	return TRUE;
    }

    return FALSE;
}

pixman_image_t *
_pixman_image_allocate (void)
{
    pixman_image_t *image = malloc (sizeof (pixman_image_t));

    if (image)
	_pixman_image_init (image);

    return image;
}

static void
image_property_changed (pixman_image_t *image)
{
    image->common.dirty = TRUE;
}

/* Ref Counting */
PIXMAN_EXPORT pixman_image_t *
pixman_image_ref (pixman_image_t *image)
{
    image->common.ref_count++;

    return image;
}

/* returns TRUE when the image is freed */
PIXMAN_EXPORT pixman_bool_t
pixman_image_unref (pixman_image_t *image)
{
    if (_pixman_image_fini (image))
    {
	free (image);
	return TRUE;
    }

    return FALSE;
}

PIXMAN_EXPORT void
pixman_image_set_destroy_function (pixman_image_t *            image,
                                   pixman_image_destroy_func_t func,
                                   void *                      data)
{
    image->common.destroy_func = func;
    image->common.destroy_data = data;
}

PIXMAN_EXPORT void *
pixman_image_get_destroy_data (pixman_image_t *image)
{
  return image->common.destroy_data;
}

void
_pixman_image_reset_clip_region (pixman_image_t *image)
{
    image->common.have_clip_region = FALSE;
}

/* Executive Summary: This function is a no-op that only exists
 * for historical reasons.
 *
 * There used to be a bug in the X server where it would rely on
 * out-of-bounds accesses when it was asked to composite with a
 * window as the source. It would create a pixman image pointing
 * to some bogus position in memory, but then set a clip region
 * to the position where the actual bits were.
 *
 * Due to a bug in old versions of pixman, where it would not clip
 * against the image bounds when a clip region was set, this would
 * actually work. So when the pixman bug was fixed, a workaround was
 * added to allow certain out-of-bound accesses. This function disabled
 * those workarounds.
 *
 * Since 0.21.2, pixman doesn't do these workarounds anymore, so now
 * this function is a no-op.
 */
PIXMAN_EXPORT void
pixman_disable_out_of_bounds_workaround (void)
{
}

static void
compute_image_info (pixman_image_t *image)
{
    pixman_format_code_t code;
    uint32_t flags = 0;

    /* Transform */
    if (!image->common.transform)
    {
	flags |= (FAST_PATH_ID_TRANSFORM	|
		  FAST_PATH_X_UNIT_POSITIVE	|
		  FAST_PATH_Y_UNIT_ZERO		|
		  FAST_PATH_AFFINE_TRANSFORM);
    }
    else
    {
	flags |= FAST_PATH_HAS_TRANSFORM;

	if (image->common.transform->matrix[2][0] == 0			&&
	    image->common.transform->matrix[2][1] == 0			&&
	    image->common.transform->matrix[2][2] == pixman_fixed_1)
	{
	    flags |= FAST_PATH_AFFINE_TRANSFORM;

	    if (image->common.transform->matrix[0][1] == 0 &&
		image->common.transform->matrix[1][0] == 0)
	    {
		if (image->common.transform->matrix[0][0] == -pixman_fixed_1 &&
		    image->common.transform->matrix[1][1] == -pixman_fixed_1)
		{
		    flags |= FAST_PATH_ROTATE_180_TRANSFORM;
		}
		flags |= FAST_PATH_SCALE_TRANSFORM;
	    }
	    else if (image->common.transform->matrix[0][0] == 0 &&
	             image->common.transform->matrix[1][1] == 0)
	    {
		pixman_fixed_t m01 = image->common.transform->matrix[0][1];
		pixman_fixed_t m10 = image->common.transform->matrix[1][0];

		if (m01 == -pixman_fixed_1 && m10 == pixman_fixed_1)
		    flags |= FAST_PATH_ROTATE_90_TRANSFORM;
		else if (m01 == pixman_fixed_1 && m10 == -pixman_fixed_1)
		    flags |= FAST_PATH_ROTATE_270_TRANSFORM;
	    }
	}

	if (image->common.transform->matrix[0][0] > 0)
	    flags |= FAST_PATH_X_UNIT_POSITIVE;

	if (image->common.transform->matrix[1][0] == 0)
	    flags |= FAST_PATH_Y_UNIT_ZERO;
    }

    /* Filter */
    switch (image->common.filter)
    {
    case PIXMAN_FILTER_NEAREST:
    case PIXMAN_FILTER_FAST:
	flags |= (FAST_PATH_NEAREST_FILTER | FAST_PATH_NO_CONVOLUTION_FILTER);
	break;

    case PIXMAN_FILTER_BILINEAR:
    case PIXMAN_FILTER_GOOD:
    case PIXMAN_FILTER_BEST:
	flags |= (FAST_PATH_BILINEAR_FILTER | FAST_PATH_NO_CONVOLUTION_FILTER);

	/* Here we have a chance to optimize BILINEAR filter to NEAREST if
	 * they are equivalent for the currently used transformation matrix.
	 */
	if (flags & FAST_PATH_ID_TRANSFORM)
	{
	    flags |= FAST_PATH_NEAREST_FILTER;
	}
	else if (flags & FAST_PATH_AFFINE_TRANSFORM)
	{
	    /* Suppose the transform is
	     *
	     *    [ t00, t01, t02 ]
	     *    [ t10, t11, t12 ]
	     *    [   0,   0,   1 ]
	     *
	     * and the destination coordinates are (n + 0.5, m + 0.5). Then
	     * the transformed x coordinate is:
	     *
	     *     tx = t00 * (n + 0.5) + t01 * (m + 0.5) + t02
	     *        = t00 * n + t01 * m + t02 + (t00 + t01) * 0.5
	     *
	     * which implies that if t00, t01 and t02 are all integers
	     * and (t00 + t01) is odd, then tx will be an integer plus 0.5,
	     * which means a BILINEAR filter will reduce to NEAREST. The same
	     * applies in the y direction
	     */
	    pixman_fixed_t (*t)[3] = image->common.transform->matrix;

	    if ((pixman_fixed_frac (
		     t[0][0] | t[0][1] | t[0][2] |
		     t[1][0] | t[1][1] | t[1][2]) == 0)			&&
		(pixman_fixed_to_int (
		    (t[0][0] + t[0][1]) & (t[1][0] + t[1][1])) % 2) == 1)
	    {
		/* FIXME: there are some affine-test failures, showing that
		 * handling of BILINEAR and NEAREST filter is not quite
		 * equivalent when getting close to 32K for the translation
		 * components of the matrix. That's likely some bug, but for
		 * now just skip BILINEAR->NEAREST optimization in this case.
		 */
		pixman_fixed_t magic_limit = pixman_int_to_fixed (30000);
		if (image->common.transform->matrix[0][2] <= magic_limit  &&
		    image->common.transform->matrix[1][2] <= magic_limit  &&
		    image->common.transform->matrix[0][2] >= -magic_limit &&
		    image->common.transform->matrix[1][2] >= -magic_limit)
		{
		    flags |= FAST_PATH_NEAREST_FILTER;
		}
	    }
	}
	break;

    case PIXMAN_FILTER_CONVOLUTION:
	break;

    case PIXMAN_FILTER_SEPARABLE_CONVOLUTION:
	flags |= FAST_PATH_SEPARABLE_CONVOLUTION_FILTER;
	break;

    default:
	flags |= FAST_PATH_NO_CONVOLUTION_FILTER;
	break;
    }

    /* Repeat mode */
    switch (image->common.repeat)
    {
    case PIXMAN_REPEAT_NONE:
	flags |=
	    FAST_PATH_NO_REFLECT_REPEAT		|
	    FAST_PATH_NO_PAD_REPEAT		|
	    FAST_PATH_NO_NORMAL_REPEAT;
	break;

    case PIXMAN_REPEAT_REFLECT:
	flags |=
	    FAST_PATH_NO_PAD_REPEAT		|
	    FAST_PATH_NO_NONE_REPEAT		|
	    FAST_PATH_NO_NORMAL_REPEAT;
	break;

    case PIXMAN_REPEAT_PAD:
	flags |=
	    FAST_PATH_NO_REFLECT_REPEAT		|
	    FAST_PATH_NO_NONE_REPEAT		|
	    FAST_PATH_NO_NORMAL_REPEAT;
	break;

    default:
	flags |=
	    FAST_PATH_NO_REFLECT_REPEAT		|
	    FAST_PATH_NO_PAD_REPEAT		|
	    FAST_PATH_NO_NONE_REPEAT;
	break;
    }

    /* Component alpha */
    if (image->common.component_alpha)
	flags |= FAST_PATH_COMPONENT_ALPHA;
    else
	flags |= FAST_PATH_UNIFIED_ALPHA;

    flags |= (FAST_PATH_NO_ACCESSORS | FAST_PATH_NARROW_FORMAT);

    /* Type specific checks */
    switch (image->type)
    {
    case SOLID:
	code = PIXMAN_solid;

	if (image->solid.color.alpha == 0xffff)
	    flags |= FAST_PATH_IS_OPAQUE;
	break;

    case BITS:
	if (image->bits.width == 1	&&
	    image->bits.height == 1	&&
	    image->common.repeat != PIXMAN_REPEAT_NONE)
	{
	    code = PIXMAN_solid;
	}
	else
	{
	    code = image->bits.format;
	    flags |= FAST_PATH_BITS_IMAGE;
	}

	if (!PIXMAN_FORMAT_A (image->bits.format)				&&
	    PIXMAN_FORMAT_TYPE (image->bits.format) != PIXMAN_TYPE_GRAY		&&
	    PIXMAN_FORMAT_TYPE (image->bits.format) != PIXMAN_TYPE_COLOR)
	{
	    flags |= FAST_PATH_SAMPLES_OPAQUE;

	    if (image->common.repeat != PIXMAN_REPEAT_NONE)
		flags |= FAST_PATH_IS_OPAQUE;
	}

	if (image->bits.read_func || image->bits.write_func)
	    flags &= ~FAST_PATH_NO_ACCESSORS;

	if (PIXMAN_FORMAT_IS_WIDE (image->bits.format))
	    flags &= ~FAST_PATH_NARROW_FORMAT;
	break;

    case RADIAL:
	code = PIXMAN_unknown;

	/*
	 * As explained in pixman-radial-gradient.c, every point of
	 * the plane has a valid associated radius (and thus will be
	 * colored) if and only if a is negative (i.e. one of the two
	 * circles contains the other one).
	 */

        if (image->radial.a >= 0)
	    break;

	/* Fall through */

    case CONICAL:
    case LINEAR:
	code = PIXMAN_unknown;

	if (image->common.repeat != PIXMAN_REPEAT_NONE)
	{
	    int i;

	    flags |= FAST_PATH_IS_OPAQUE;
	    for (i = 0; i < image->gradient.n_stops; ++i)
	    {
		if (image->gradient.stops[i].color.alpha != 0xffff)
		{
		    flags &= ~FAST_PATH_IS_OPAQUE;
		    break;
		}
	    }
	}
	break;

    default:
	code = PIXMAN_unknown;
	break;
    }

    /* Alpha maps are only supported for BITS images, so it's always
     * safe to ignore their presense for non-BITS images
     */
    if (!image->common.alpha_map || image->type != BITS)
    {
	flags |= FAST_PATH_NO_ALPHA_MAP;
    }
    else
    {
	if (PIXMAN_FORMAT_IS_WIDE (image->common.alpha_map->format))
	    flags &= ~FAST_PATH_NARROW_FORMAT;
    }

    /* Both alpha maps and convolution filters can introduce
     * non-opaqueness in otherwise opaque images. Also
     * an image with component alpha turned on is only opaque
     * if all channels are opaque, so we simply turn it off
     * unconditionally for those images.
     */
    if (image->common.alpha_map						||
	image->common.filter == PIXMAN_FILTER_CONVOLUTION		||
        image->common.filter == PIXMAN_FILTER_SEPARABLE_CONVOLUTION     ||
	image->common.component_alpha)
    {
	flags &= ~(FAST_PATH_IS_OPAQUE | FAST_PATH_SAMPLES_OPAQUE);
    }

    image->common.flags = flags;
    image->common.extended_format_code = code;
}

void
_pixman_image_validate (pixman_image_t *image)
{
    if (image->common.dirty)
    {
	compute_image_info (image);

	/* It is important that property_changed is
	 * called *after* compute_image_info() because
	 * property_changed() can make use of the flags
	 * to set up accessors etc.
	 */
	if (image->common.property_changed)
	    image->common.property_changed (image);

	image->common.dirty = FALSE;
    }

    if (image->common.alpha_map)
	_pixman_image_validate ((pixman_image_t *)image->common.alpha_map);
}

PIXMAN_EXPORT pixman_bool_t
pixman_image_set_clip_region32 (pixman_image_t *   image,
                                pixman_region32_t *region)
{
    image_common_t *common = (image_common_t *)image;
    pixman_bool_t result;

    if (region)
    {
	if ((result = pixman_region32_copy (&common->clip_region, region)))
	    image->common.have_clip_region = TRUE;
    }
    else
    {
	_pixman_image_reset_clip_region (image);

	result = TRUE;
    }

    image_property_changed (image);

    return result;
}

PIXMAN_EXPORT pixman_bool_t
pixman_image_set_clip_region (pixman_image_t *   image,
                              pixman_region16_t *region)
{
    image_common_t *common = (image_common_t *)image;
    pixman_bool_t result;

    if (region)
    {
	if ((result = pixman_region32_copy_from_region16 (&common->clip_region, region)))
	    image->common.have_clip_region = TRUE;
    }
    else
    {
	_pixman_image_reset_clip_region (image);

	result = TRUE;
    }

    image_property_changed (image);

    return result;
}

PIXMAN_EXPORT void
pixman_image_set_has_client_clip (pixman_image_t *image,
                                  pixman_bool_t   client_clip)
{
    image->common.client_clip = client_clip;
}

PIXMAN_EXPORT pixman_bool_t
pixman_image_set_transform (pixman_image_t *          image,
                            const pixman_transform_t *transform)
{
    static const pixman_transform_t id =
    {
	{ { pixman_fixed_1, 0, 0 },
	  { 0, pixman_fixed_1, 0 },
	  { 0, 0, pixman_fixed_1 } }
    };

    image_common_t *common = (image_common_t *)image;
    pixman_bool_t result;

    if (common->transform == transform)
	return TRUE;

    if (!transform || memcmp (&id, transform, sizeof (pixman_transform_t)) == 0)
    {
	free (common->transform);
	common->transform = NULL;
	result = TRUE;

	goto out;
    }

    if (common->transform &&
	memcmp (common->transform, transform, sizeof (pixman_transform_t)) == 0)
    {
	return TRUE;
    }

    if (common->transform == NULL)
	common->transform = malloc (sizeof (pixman_transform_t));

    if (common->transform == NULL)
    {
	result = FALSE;

	goto out;
    }

    memcpy (common->transform, transform, sizeof(pixman_transform_t));

    result = TRUE;

out:
    image_property_changed (image);

    return result;
}

PIXMAN_EXPORT void
pixman_image_set_repeat (pixman_image_t *image,
                         pixman_repeat_t repeat)
{
    if (image->common.repeat == repeat)
	return;

    image->common.repeat = repeat;

    image_property_changed (image);
}

PIXMAN_EXPORT void
pixman_image_set_dither (pixman_image_t *image,
			 pixman_dither_t dither)
{
    if (image->type == BITS)
    {
	if (image->bits.dither == dither)
	    return;

	image->bits.dither = dither;

	image_property_changed (image);
    }
}

PIXMAN_EXPORT void
pixman_image_set_dither_offset (pixman_image_t *image,
				int             offset_x,
				int             offset_y)
{
    if (image->type == BITS)
    {
	if (image->bits.dither_offset_x == offset_x &&
	    image->bits.dither_offset_y == offset_y)
	{
	    return;
	}

	image->bits.dither_offset_x = offset_x;
	image->bits.dither_offset_y = offset_y;

	image_property_changed (image);
    }
}

PIXMAN_EXPORT pixman_bool_t
pixman_image_set_filter (pixman_image_t *      image,
                         pixman_filter_t       filter,
                         const pixman_fixed_t *params,
                         int                   n_params)
{
    image_common_t *common = (image_common_t *)image;
    pixman_fixed_t *new_params;

    if (params == common->filter_params && filter == common->filter)
	return TRUE;

    if (filter == PIXMAN_FILTER_SEPARABLE_CONVOLUTION)
    {
	int width = pixman_fixed_to_int (params[0]);
	int height = pixman_fixed_to_int (params[1]);
	int x_phase_bits = pixman_fixed_to_int (params[2]);
	int y_phase_bits = pixman_fixed_to_int (params[3]);
	int n_x_phases = (1 << x_phase_bits);
	int n_y_phases = (1 << y_phase_bits);

	return_val_if_fail (
	    n_params == 4 + n_x_phases * width + n_y_phases * height, FALSE);
    }
    
    new_params = NULL;
    if (params)
    {
	new_params = pixman_malloc_ab (n_params, sizeof (pixman_fixed_t));
	if (!new_params)
	    return FALSE;

	memcpy (new_params,
	        params, n_params * sizeof (pixman_fixed_t));
    }

    common->filter = filter;

    if (common->filter_params)
	free (common->filter_params);

    common->filter_params = new_params;
    common->n_filter_params = n_params;

    image_property_changed (image);
    return TRUE;
}

PIXMAN_EXPORT void
pixman_image_set_source_clipping (pixman_image_t *image,
                                  pixman_bool_t   clip_sources)
{
    if (image->common.clip_sources == clip_sources)
	return;

    image->common.clip_sources = clip_sources;

    image_property_changed (image);
}

/* Unlike all the other property setters, this function does not
 * copy the content of indexed. Doing this copying is simply
 * way, way too expensive.
 */
PIXMAN_EXPORT void
pixman_image_set_indexed (pixman_image_t *        image,
                          const pixman_indexed_t *indexed)
{
    bits_image_t *bits = (bits_image_t *)image;

    if (bits->indexed == indexed)
	return;

    bits->indexed = indexed;

    image_property_changed (image);
}

PIXMAN_EXPORT void
pixman_image_set_alpha_map (pixman_image_t *image,
                            pixman_image_t *alpha_map,
                            int16_t         x,
                            int16_t         y)
{
    image_common_t *common = (image_common_t *)image;

    return_if_fail (!alpha_map || alpha_map->type == BITS);

    if (alpha_map && common->alpha_count > 0)
    {
	/* If this image is being used as an alpha map itself,
	 * then you can't give it an alpha map of its own.
	 */
	return;
    }

    if (alpha_map && alpha_map->common.alpha_map)
    {
	/* If the image has an alpha map of its own,
	 * then it can't be used as an alpha map itself
	 */
	return;
    }

    if (common->alpha_map != (bits_image_t *)alpha_map)
    {
	if (common->alpha_map)
	{
	    common->alpha_map->common.alpha_count--;

	    pixman_image_unref ((pixman_image_t *)common->alpha_map);
	}

	if (alpha_map)
	{
	    common->alpha_map = (bits_image_t *)pixman_image_ref (alpha_map);

	    common->alpha_map->common.alpha_count++;
	}
	else
	{
	    common->alpha_map = NULL;
	}
    }

    common->alpha_origin_x = x;
    common->alpha_origin_y = y;

    image_property_changed (image);
}

PIXMAN_EXPORT void
pixman_image_set_component_alpha   (pixman_image_t *image,
                                    pixman_bool_t   component_alpha)
{
    if (image->common.component_alpha == component_alpha)
	return;

    image->common.component_alpha = component_alpha;

    image_property_changed (image);
}

PIXMAN_EXPORT pixman_bool_t
pixman_image_get_component_alpha   (pixman_image_t       *image)
{
    return image->common.component_alpha;
}

PIXMAN_EXPORT void
pixman_image_set_accessors (pixman_image_t *           image,
                            pixman_read_memory_func_t  read_func,
                            pixman_write_memory_func_t write_func)
{
    return_if_fail (image != NULL);

    if (image->type == BITS)
    {
	/* Accessors only work for <= 32 bpp. */
	if (PIXMAN_FORMAT_BPP(image->bits.format) > 32)
	    return_if_fail (!read_func && !write_func);

	image->bits.read_func = read_func;
	image->bits.write_func = write_func;

	image_property_changed (image);
    }
}

PIXMAN_EXPORT uint32_t *
pixman_image_get_data (pixman_image_t *image)
{
    if (image->type == BITS)
	return image->bits.bits;

    return NULL;
}

PIXMAN_EXPORT int
pixman_image_get_width (pixman_image_t *image)
{
    if (image->type == BITS)
	return image->bits.width;

    return 0;
}

PIXMAN_EXPORT int
pixman_image_get_height (pixman_image_t *image)
{
    if (image->type == BITS)
	return image->bits.height;

    return 0;
}

PIXMAN_EXPORT int
pixman_image_get_stride (pixman_image_t *image)
{
    if (image->type == BITS)
	return image->bits.rowstride * (int) sizeof (uint32_t);

    return 0;
}

PIXMAN_EXPORT int
pixman_image_get_depth (pixman_image_t *image)
{
    if (image->type == BITS)
	return PIXMAN_FORMAT_DEPTH (image->bits.format);

    return 0;
}

PIXMAN_EXPORT pixman_format_code_t
pixman_image_get_format (pixman_image_t *image)
{
    if (image->type == BITS)
	return image->bits.format;

    return PIXMAN_null;
}

uint32_t
_pixman_image_get_solid (pixman_implementation_t *imp,
			 pixman_image_t *         image,
                         pixman_format_code_t     format)
{
    uint32_t result;

    if (image->type == SOLID)
    {
	result = image->solid.color_32;
    }
    else if (image->type == BITS)
    {
	if (image->bits.format == PIXMAN_a8r8g8b8)
	    result = image->bits.bits[0];
	else if (image->bits.format == PIXMAN_x8r8g8b8)
	    result = image->bits.bits[0] | 0xff000000;
	else if (image->bits.format == PIXMAN_a8)
	    result = (uint32_t)(*(uint8_t *)image->bits.bits) << 24;
	else
	    goto otherwise;
    }
    else
    {
	pixman_iter_t iter;

    otherwise:
	_pixman_implementation_iter_init (
	    imp, &iter, image, 0, 0, 1, 1,
	    (uint8_t *)&result,
	    ITER_NARROW | ITER_SRC, image->common.flags);
	
	result = *iter.get_scanline (&iter, NULL);

	if (iter.fini)
	    iter.fini (&iter);
    }

    /* If necessary, convert RGB <--> BGR. */
    if (PIXMAN_FORMAT_TYPE (format) != PIXMAN_TYPE_ARGB
	&& PIXMAN_FORMAT_TYPE (format) != PIXMAN_TYPE_ARGB_SRGB)
    {
	result = (((result & 0xff000000) >>  0) |
	          ((result & 0x00ff0000) >> 16) |
	          ((result & 0x0000ff00) >>  0) |
	          ((result & 0x000000ff) << 16));
    }

    return result;
}
