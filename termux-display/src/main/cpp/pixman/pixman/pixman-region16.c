/*
 * Copyright © 2008 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of
 * Red Hat, Inc. not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission. Red Hat, Inc. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * RED HAT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL RED HAT, INC. BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author: Soren Sandmann <sandmann@redhat.com>
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#undef PIXMAN_DISABLE_DEPRECATED

#include "pixman-private.h"

#include <stdlib.h>

typedef pixman_box16_t		box_type_t;
typedef pixman_region16_data_t	region_data_type_t;
typedef pixman_region16_t	region_type_t;
typedef int32_t                 overflow_int_t;

typedef struct {
    int x, y;
} point_type_t;

#define PREFIX(x) pixman_region##x

#define PIXMAN_REGION_MAX INT16_MAX
#define PIXMAN_REGION_MIN INT16_MIN

#include "pixman-region.c"

/* This function exists only to make it possible to preserve the X ABI -
 * it should go away at first opportunity.
 *
 * The problem is that the X ABI exports the three structs and has used
 * them through macros. So the X server calls this function with
 * the addresses of those structs which makes the existing code continue to
 * work.
 */
PIXMAN_EXPORT void
pixman_region_set_static_pointers (pixman_box16_t *empty_box,
				   pixman_region16_data_t *empty_data,
				   pixman_region16_data_t *broken_data)
{
    pixman_region_empty_box = empty_box;
    pixman_region_empty_data = empty_data;
    pixman_broken_data = broken_data;
}
