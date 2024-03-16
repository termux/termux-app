/*
 * Copyright © 2009 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Red Hat not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Red Hat makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
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
#include <config.h>
#endif
#include <stdlib.h>
#include "pixman-private.h"

pixman_implementation_t *
_pixman_implementation_create (pixman_implementation_t *fallback,
			       const pixman_fast_path_t *fast_paths)
{
    pixman_implementation_t *imp;

    assert (fast_paths);

    if ((imp = malloc (sizeof (pixman_implementation_t))))
    {
	pixman_implementation_t *d;

	memset (imp, 0, sizeof *imp);

	imp->fallback = fallback;
	imp->fast_paths = fast_paths;
	
	/* Make sure the whole fallback chain has the right toplevel */
	for (d = imp; d != NULL; d = d->fallback)
	    d->toplevel = imp;
    }

    return imp;
}

#define N_CACHED_FAST_PATHS 8

typedef struct
{
    struct
    {
	pixman_implementation_t *	imp;
	pixman_fast_path_t		fast_path;
    } cache [N_CACHED_FAST_PATHS];
} cache_t;

PIXMAN_DEFINE_THREAD_LOCAL (cache_t, fast_path_cache)

static void
dummy_composite_rect (pixman_implementation_t *imp,
		      pixman_composite_info_t *info)
{
}

void
_pixman_implementation_lookup_composite (pixman_implementation_t  *toplevel,
					 pixman_op_t               op,
					 pixman_format_code_t      src_format,
					 uint32_t                  src_flags,
					 pixman_format_code_t      mask_format,
					 uint32_t                  mask_flags,
					 pixman_format_code_t      dest_format,
					 uint32_t                  dest_flags,
					 pixman_implementation_t **out_imp,
					 pixman_composite_func_t  *out_func)
{
    pixman_implementation_t *imp;
    cache_t *cache;
    int i;

    /* Check cache for fast paths */
    cache = PIXMAN_GET_THREAD_LOCAL (fast_path_cache);

    for (i = 0; i < N_CACHED_FAST_PATHS; ++i)
    {
	const pixman_fast_path_t *info = &(cache->cache[i].fast_path);

	/* Note that we check for equality here, not whether
	 * the cached fast path matches. This is to prevent
	 * us from selecting an overly general fast path
	 * when a more specific one would work.
	 */
	if (info->op == op			&&
	    info->src_format == src_format	&&
	    info->mask_format == mask_format	&&
	    info->dest_format == dest_format	&&
	    info->src_flags == src_flags	&&
	    info->mask_flags == mask_flags	&&
	    info->dest_flags == dest_flags	&&
	    info->func)
	{
	    *out_imp = cache->cache[i].imp;
	    *out_func = cache->cache[i].fast_path.func;

	    goto update_cache;
	}
    }

    for (imp = toplevel; imp != NULL; imp = imp->fallback)
    {
	const pixman_fast_path_t *info = imp->fast_paths;

	while (info->op != PIXMAN_OP_NONE)
	{
	    if ((info->op == op || info->op == PIXMAN_OP_any)		&&
		/* Formats */
		((info->src_format == src_format) ||
		 (info->src_format == PIXMAN_any))			&&
		((info->mask_format == mask_format) ||
		 (info->mask_format == PIXMAN_any))			&&
		((info->dest_format == dest_format) ||
		 (info->dest_format == PIXMAN_any))			&&
		/* Flags */
		(info->src_flags & src_flags) == info->src_flags	&&
		(info->mask_flags & mask_flags) == info->mask_flags	&&
		(info->dest_flags & dest_flags) == info->dest_flags)
	    {
		*out_imp = imp;
		*out_func = info->func;

		/* Set i to the last spot in the cache so that the
		 * move-to-front code below will work
		 */
		i = N_CACHED_FAST_PATHS - 1;

		goto update_cache;
	    }

	    ++info;
	}
    }

    /* We should never reach this point */
    _pixman_log_error (
        FUNC,
        "No composite function found\n"
        "\n"
        "The most likely cause of this is that this system has issues with\n"
        "thread local storage\n");

    *out_imp = NULL;
    *out_func = dummy_composite_rect;
    return;

update_cache:
    if (i)
    {
	while (i--)
	    cache->cache[i + 1] = cache->cache[i];

	cache->cache[0].imp = *out_imp;
	cache->cache[0].fast_path.op = op;
	cache->cache[0].fast_path.src_format = src_format;
	cache->cache[0].fast_path.src_flags = src_flags;
	cache->cache[0].fast_path.mask_format = mask_format;
	cache->cache[0].fast_path.mask_flags = mask_flags;
	cache->cache[0].fast_path.dest_format = dest_format;
	cache->cache[0].fast_path.dest_flags = dest_flags;
	cache->cache[0].fast_path.func = *out_func;
    }
}

static void
dummy_combine (pixman_implementation_t *imp,
	       pixman_op_t              op,
	       uint32_t *               pd,
	       const uint32_t *         ps,
	       const uint32_t *         pm,
	       int                      w)
{
}

pixman_combine_32_func_t
_pixman_implementation_lookup_combiner (pixman_implementation_t *imp,
					pixman_op_t		 op,
					pixman_bool_t		 component_alpha,
					pixman_bool_t		 narrow)
{
    while (imp)
    {
	pixman_combine_32_func_t f = NULL;

	switch ((narrow << 1) | component_alpha)
	{
	case 0: /* not narrow, not component alpha */
	    f = (pixman_combine_32_func_t)imp->combine_float[op];
	    break;
	    
	case 1: /* not narrow, component_alpha */
	    f = (pixman_combine_32_func_t)imp->combine_float_ca[op];
	    break;

	case 2: /* narrow, not component alpha */
	    f = imp->combine_32[op];
	    break;

	case 3: /* narrow, component_alpha */
	    f = imp->combine_32_ca[op];
	    break;
	}

	if (f)
	    return f;

	imp = imp->fallback;
    }

    /* We should never reach this point */
    _pixman_log_error (FUNC, "No known combine function\n");
    return dummy_combine;
}

pixman_bool_t
_pixman_implementation_blt (pixman_implementation_t * imp,
                            uint32_t *                src_bits,
                            uint32_t *                dst_bits,
                            int                       src_stride,
                            int                       dst_stride,
                            int                       src_bpp,
                            int                       dst_bpp,
                            int                       src_x,
                            int                       src_y,
                            int                       dest_x,
                            int                       dest_y,
                            int                       width,
                            int                       height)
{
    while (imp)
    {
	if (imp->blt &&
	    (*imp->blt) (imp, src_bits, dst_bits, src_stride, dst_stride,
			 src_bpp, dst_bpp, src_x, src_y, dest_x, dest_y,
			 width, height))
	{
	    return TRUE;
	}

	imp = imp->fallback;
    }

    return FALSE;
}

pixman_bool_t
_pixman_implementation_fill (pixman_implementation_t *imp,
                             uint32_t *               bits,
                             int                      stride,
                             int                      bpp,
                             int                      x,
                             int                      y,
                             int                      width,
                             int                      height,
                             uint32_t                 filler)
{
    while (imp)
    {
	if (imp->fill &&
	    ((*imp->fill) (imp, bits, stride, bpp, x, y, width, height, filler)))
	{
	    return TRUE;
	}

	imp = imp->fallback;
    }

    return FALSE;
}

static uint32_t *
get_scanline_null (pixman_iter_t *iter, const uint32_t *mask)
{
    return NULL;
}

void
_pixman_implementation_iter_init (pixman_implementation_t *imp,
                                  pixman_iter_t           *iter,
                                  pixman_image_t          *image,
                                  int                      x,
                                  int                      y,
                                  int                      width,
                                  int                      height,
                                  uint8_t                 *buffer,
                                  iter_flags_t             iter_flags,
                                  uint32_t                 image_flags)
{
    pixman_format_code_t format;

    iter->image = image;
    iter->buffer = (uint32_t *)buffer;
    iter->x = x;
    iter->y = y;
    iter->width = width;
    iter->height = height;
    iter->iter_flags = iter_flags;
    iter->image_flags = image_flags;
    iter->fini = NULL;

    if (!iter->image)
    {
	iter->get_scanline = get_scanline_null;
	return;
    }

    format = iter->image->common.extended_format_code;

    while (imp)
    {
        if (imp->iter_info)
        {
            const pixman_iter_info_t *info;

            for (info = imp->iter_info; info->format != PIXMAN_null; ++info)
            {
                if ((info->format == PIXMAN_any || info->format == format) &&
                    (info->image_flags & image_flags) == info->image_flags &&
                    (info->iter_flags & iter_flags) == info->iter_flags)
                {
                    iter->get_scanline = info->get_scanline;
                    iter->write_back = info->write_back;

                    if (info->initializer)
                        info->initializer (iter, info);
                    return;
                }
            }
        }

        imp = imp->fallback;
    }
}

pixman_bool_t
_pixman_disabled (const char *name)
{
    const char *env;

    if ((env = getenv ("PIXMAN_DISABLE")))
    {
	do
	{
	    const char *end;
	    int len;

	    if ((end = strchr (env, ' ')))
		len = end - env;
	    else
		len = strlen (env);

	    if (strlen (name) == len && strncmp (name, env, len) == 0)
	    {
		printf ("pixman: Disabled %s implementation\n", name);
		return TRUE;
	    }

	    env += len;
	}
	while (*env++);
    }

    return FALSE;
}

static const pixman_fast_path_t empty_fast_path[] =
{
    { PIXMAN_OP_NONE }
};

pixman_implementation_t *
_pixman_choose_implementation (void)
{
    pixman_implementation_t *imp;

    imp = _pixman_implementation_create_general();

    if (!_pixman_disabled ("fast"))
	imp = _pixman_implementation_create_fast_path (imp);

    imp = _pixman_x86_get_implementations (imp);
    imp = _pixman_arm_get_implementations (imp);
    imp = _pixman_ppc_get_implementations (imp);
    imp = _pixman_mips_get_implementations (imp);

    imp = _pixman_implementation_create_noop (imp);

    if (_pixman_disabled ("wholeops"))
    {
        pixman_implementation_t *cur;

        /* Disable all whole-operation paths except the general one,
         * so that optimized iterators are used as much as possible.
         */
        for (cur = imp; cur->fallback; cur = cur->fallback)
            cur->fast_paths = empty_fast_path;
    }

    return imp;
}
