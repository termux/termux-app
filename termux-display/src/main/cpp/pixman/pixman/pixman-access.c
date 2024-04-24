/*
 *
 * Copyright © 2000 Keith Packard, member of The XFree86 Project, Inc.
 *             2005 Lars Knoll & Zack Rusin, Trolltech
 *             2008 Aaron Plattner, NVIDIA Corporation
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "pixman-accessor.h"
#include "pixman-private.h"

#define CONVERT_RGB24_TO_Y15(s)						\
    (((((s) >> 16) & 0xff) * 153 +					\
      (((s) >>  8) & 0xff) * 301 +					\
      (((s)      ) & 0xff) * 58) >> 2)

#define CONVERT_RGB24_TO_RGB15(s)                                       \
    ((((s) >> 3) & 0x001f) |                                            \
     (((s) >> 6) & 0x03e0) |                                            \
     (((s) >> 9) & 0x7c00))

/* Fetch macros */

#ifdef WORDS_BIGENDIAN
#define FETCH_1(img,l,o)						\
    (((READ ((img), ((uint32_t *)(l)) + ((o) >> 5))) >> (0x1f - ((o) & 0x1f))) & 0x1)
#else
#define FETCH_1(img,l,o)						\
    ((((READ ((img), ((uint32_t *)(l)) + ((o) >> 5))) >> ((o) & 0x1f))) & 0x1)
#endif

#define FETCH_8(img,l,o)    (READ (img, (((uint8_t *)(l)) + ((o) >> 3))))

#ifdef WORDS_BIGENDIAN
#define FETCH_4(img,l,o)						\
    (((4 * (o)) & 4) ? (FETCH_8 (img,l, 4 * (o)) & 0xf) : (FETCH_8 (img,l,(4 * (o))) >> 4))
#else
#define FETCH_4(img,l,o)						\
    (((4 * (o)) & 4) ? (FETCH_8 (img, l, 4 * (o)) >> 4) : (FETCH_8 (img, l, (4 * (o))) & 0xf))
#endif

#ifdef WORDS_BIGENDIAN
#define FETCH_24(img,l,o)                                              \
    ((uint32_t)(READ (img, (((uint8_t *)(l)) + ((o) * 3) + 0)) << 16)    |       \
     (uint32_t)(READ (img, (((uint8_t *)(l)) + ((o) * 3) + 1)) << 8)     |       \
     (uint32_t)(READ (img, (((uint8_t *)(l)) + ((o) * 3) + 2)) << 0))
#else
#define FETCH_24(img,l,o)						\
    ((uint32_t)(READ (img, (((uint8_t *)(l)) + ((o) * 3) + 0)) << 0)	|	\
     (uint32_t)(READ (img, (((uint8_t *)(l)) + ((o) * 3) + 1)) << 8)	|	\
     (uint32_t)(READ (img, (((uint8_t *)(l)) + ((o) * 3) + 2)) << 16))
#endif

/* Store macros */

#ifdef WORDS_BIGENDIAN
#define STORE_1(img,l,o,v)						\
    do									\
    {									\
	uint32_t  *__d = ((uint32_t *)(l)) + ((o) >> 5);		\
	uint32_t __m, __v;						\
									\
	__m = 1U << (0x1f - ((o) & 0x1f));				\
	__v = (v)? __m : 0;						\
									\
	WRITE((img), __d, (READ((img), __d) & ~__m) | __v);		\
    }									\
    while (0)
#else
#define STORE_1(img,l,o,v)						\
    do									\
    {									\
	uint32_t  *__d = ((uint32_t *)(l)) + ((o) >> 5);		\
	uint32_t __m, __v;						\
									\
	__m = 1U << ((o) & 0x1f);					\
	__v = (v)? __m : 0;						\
									\
	WRITE((img), __d, (READ((img), __d) & ~__m) | __v);		\
    }									\
    while (0)
#endif

#define STORE_8(img,l,o,v)  (WRITE (img, (uint8_t *)(l) + ((o) >> 3), (v)))

#ifdef WORDS_BIGENDIAN
#define STORE_4(img,l,o,v)						\
    do									\
    {									\
	int bo = 4 * (o);						\
	int v4 = (v) & 0x0f;						\
									\
	STORE_8 (img, l, bo, (						\
		     bo & 4 ?						\
		     (FETCH_8 (img, l, bo) & 0xf0) | (v4) :		\
		     (FETCH_8 (img, l, bo) & 0x0f) | (v4 << 4)));	\
    } while (0)
#else
#define STORE_4(img,l,o,v)						\
    do									\
    {									\
	int bo = 4 * (o);						\
	int v4 = (v) & 0x0f;						\
									\
	STORE_8 (img, l, bo, (						\
		     bo & 4 ?						\
		     (FETCH_8 (img, l, bo) & 0x0f) | (v4 << 4) :	\
		     (FETCH_8 (img, l, bo) & 0xf0) | (v4)));		\
    } while (0)
#endif

#ifdef WORDS_BIGENDIAN
#define STORE_24(img,l,o,v)                                            \
    do                                                                 \
    {                                                                  \
	uint8_t *__tmp = (l) + 3 * (o);				       \
        							       \
	WRITE ((img), __tmp++, ((v) & 0x00ff0000) >> 16);	       \
	WRITE ((img), __tmp++, ((v) & 0x0000ff00) >>  8);	       \
	WRITE ((img), __tmp++, ((v) & 0x000000ff) >>  0);	       \
    }                                                                  \
    while (0)
#else
#define STORE_24(img,l,o,v)                                            \
    do                                                                 \
    {                                                                  \
	uint8_t *__tmp = (l) + 3 * (o);				       \
        							       \
	WRITE ((img), __tmp++, ((v) & 0x000000ff) >>  0);	       \
	WRITE ((img), __tmp++, ((v) & 0x0000ff00) >>  8);	       \
	WRITE ((img), __tmp++, ((v) & 0x00ff0000) >> 16);	       \
    }								       \
    while (0)
#endif

/*
 * YV12 setup and access macros
 */

#define YV12_SETUP(image)                                               \
    bits_image_t *__bits_image = (bits_image_t *)image;                 \
    uint32_t *bits = __bits_image->bits;                                \
    int stride = __bits_image->rowstride;                               \
    int offset0 = stride < 0 ?                                          \
    ((-stride) >> 1) * ((__bits_image->height - 1) >> 1) - stride :	\
    stride * __bits_image->height;					\
    int offset1 = stride < 0 ?                                          \
    offset0 + ((-stride) >> 1) * ((__bits_image->height) >> 1) :	\
	offset0 + (offset0 >> 2)

/* Note no trailing semicolon on the above macro; if it's there, then
 * the typical usage of YV12_SETUP(image); will have an extra trailing ;
 * that some compilers will interpret as a statement -- and then any further
 * variable declarations will cause an error.
 */

#define YV12_Y(line)                                                    \
    ((uint8_t *) ((bits) + (stride) * (line)))

#define YV12_U(line)                                                    \
    ((uint8_t *) ((bits) + offset1 +                                    \
                  ((stride) >> 1) * ((line) >> 1)))

#define YV12_V(line)                                                    \
    ((uint8_t *) ((bits) + offset0 +                                    \
                  ((stride) >> 1) * ((line) >> 1)))

/* Misc. helpers */

static force_inline void
get_shifts (pixman_format_code_t  format,
	    int			 *a,
	    int			 *r,
	    int                  *g,
	    int                  *b)
{
    switch (PIXMAN_FORMAT_TYPE (format))
    {
    case PIXMAN_TYPE_A:
	*b = 0;
	*g = 0;
	*r = 0;
	*a = 0;
	break;

    case PIXMAN_TYPE_ARGB:
    case PIXMAN_TYPE_ARGB_SRGB:
	*b = 0;
	*g = *b + PIXMAN_FORMAT_B (format);
	*r = *g + PIXMAN_FORMAT_G (format);
	*a = *r + PIXMAN_FORMAT_R (format);
	break;

    case PIXMAN_TYPE_ABGR:
	*r = 0;
	*g = *r + PIXMAN_FORMAT_R (format);
	*b = *g + PIXMAN_FORMAT_G (format);
	*a = *b + PIXMAN_FORMAT_B (format);
	break;

    case PIXMAN_TYPE_BGRA:
	/* With BGRA formats we start counting at the high end of the pixel */
	*b = PIXMAN_FORMAT_BPP (format) - PIXMAN_FORMAT_B (format);
	*g = *b - PIXMAN_FORMAT_B (format);
	*r = *g - PIXMAN_FORMAT_G (format);
	*a = *r - PIXMAN_FORMAT_R (format);
	break;

    case PIXMAN_TYPE_RGBA:
	/* With BGRA formats we start counting at the high end of the pixel */
	*r = PIXMAN_FORMAT_BPP (format) - PIXMAN_FORMAT_R (format);
	*g = *r - PIXMAN_FORMAT_R (format);
	*b = *g - PIXMAN_FORMAT_G (format);
	*a = *b - PIXMAN_FORMAT_B (format);
	break;

    default:
	assert (0);
	break;
    }
}

static force_inline uint32_t
convert_channel (uint32_t pixel, uint32_t def_value,
		 int n_from_bits, int from_shift,
		 int n_to_bits, int to_shift)
{
    uint32_t v;

    if (n_from_bits && n_to_bits)
	v  = unorm_to_unorm (pixel >> from_shift, n_from_bits, n_to_bits);
    else if (n_to_bits)
	v = def_value;
    else
	v = 0;

    return (v & ((1 << n_to_bits) - 1)) << to_shift;
}

static force_inline uint32_t
convert_pixel (pixman_format_code_t from, pixman_format_code_t to, uint32_t pixel)
{
    int a_from_shift, r_from_shift, g_from_shift, b_from_shift;
    int a_to_shift, r_to_shift, g_to_shift, b_to_shift;
    uint32_t a, r, g, b;

    get_shifts (from, &a_from_shift, &r_from_shift, &g_from_shift, &b_from_shift);
    get_shifts (to, &a_to_shift, &r_to_shift, &g_to_shift, &b_to_shift);

    a = convert_channel (pixel, ~0,
			 PIXMAN_FORMAT_A (from), a_from_shift,
			 PIXMAN_FORMAT_A (to), a_to_shift);

    r = convert_channel (pixel, 0,
			 PIXMAN_FORMAT_R (from), r_from_shift,
			 PIXMAN_FORMAT_R (to), r_to_shift);

    g = convert_channel (pixel, 0,
			 PIXMAN_FORMAT_G (from), g_from_shift,
			 PIXMAN_FORMAT_G (to), g_to_shift);

    b = convert_channel (pixel, 0,
			 PIXMAN_FORMAT_B (from), b_from_shift,
			 PIXMAN_FORMAT_B (to), b_to_shift);

    return a | r | g | b;
}

static force_inline uint32_t
convert_pixel_to_a8r8g8b8 (bits_image_t *image,
			   pixman_format_code_t format,
			   uint32_t pixel)
{
    if (PIXMAN_FORMAT_TYPE (format) == PIXMAN_TYPE_GRAY		||
	PIXMAN_FORMAT_TYPE (format) == PIXMAN_TYPE_COLOR)
    {
	return image->indexed->rgba[pixel];
    }
    else
    {
	return convert_pixel (format, PIXMAN_a8r8g8b8, pixel);
    }
}

static force_inline uint32_t
convert_pixel_from_a8r8g8b8 (pixman_image_t *image,
			     pixman_format_code_t format, uint32_t pixel)
{
    if (PIXMAN_FORMAT_TYPE (format) == PIXMAN_TYPE_GRAY)
    {
	pixel = CONVERT_RGB24_TO_Y15 (pixel);

	return image->bits.indexed->ent[pixel & 0x7fff];
    }
    else if (PIXMAN_FORMAT_TYPE (format) == PIXMAN_TYPE_COLOR)
    {
	pixel = convert_pixel (PIXMAN_a8r8g8b8, PIXMAN_x1r5g5b5, pixel);

	return image->bits.indexed->ent[pixel & 0x7fff];
    }
    else
    {
	return convert_pixel (PIXMAN_a8r8g8b8, format, pixel);
    }
}

static force_inline uint32_t
fetch_and_convert_pixel (bits_image_t *		image,
			 const uint8_t *	bits,
			 int			offset,
			 pixman_format_code_t	format)
{
    uint32_t pixel;

    switch (PIXMAN_FORMAT_BPP (format))
    {
    case 1:
	pixel = FETCH_1 (image, bits, offset);
	break;

    case 4:
	pixel = FETCH_4 (image, bits, offset);
	break;

    case 8:
	pixel = READ (image, bits + offset);
	break;

    case 16:
	pixel = READ (image, ((uint16_t *)bits + offset));
	break;

    case 24:
	pixel = FETCH_24 (image, bits, offset);
	break;

    case 32:
	pixel = READ (image, ((uint32_t *)bits + offset));
	break;

    default:
	pixel = 0xffff00ff; /* As ugly as possible to detect the bug */
	break;
    }

    return convert_pixel_to_a8r8g8b8 (image, format, pixel);
}

static force_inline void
convert_and_store_pixel (bits_image_t *		image,
			 uint8_t *		dest,
			 int                    offset,
			 pixman_format_code_t	format,
			 uint32_t		pixel)
{
    uint32_t converted = convert_pixel_from_a8r8g8b8 (
	(pixman_image_t *)image, format, pixel);

    switch (PIXMAN_FORMAT_BPP (format))
    {
    case 1:
	STORE_1 (image, dest, offset, converted & 0x01);
	break;

    case 4:
	STORE_4 (image, dest, offset, converted & 0xf);
	break;

    case 8:
	WRITE (image, (dest + offset), converted & 0xff);
	break;

    case 16:
	WRITE (image, ((uint16_t *)dest + offset), converted & 0xffff);
	break;

    case 24:
	STORE_24 (image, dest, offset, converted);
	break;

    case 32:
	WRITE (image, ((uint32_t *)dest + offset), converted);
	break;

    default:
	*dest = 0x0;
	break;
    }
}

#define MAKE_ACCESSORS(format)						\
    static void								\
    fetch_scanline_ ## format (bits_image_t *image,			\
			       int	       x,			\
			       int             y,			\
			       int             width,			\
			       uint32_t *      buffer,			\
			       const uint32_t *mask)			\
    {									\
	uint8_t *bits =							\
	    (uint8_t *)(image->bits + y * image->rowstride);		\
	int i;								\
									\
	for (i = 0; i < width; ++i)					\
	{								\
	    *buffer++ =							\
		fetch_and_convert_pixel (image, bits, x + i, PIXMAN_ ## format); \
	}								\
    }									\
									\
    static void								\
    store_scanline_ ## format (bits_image_t *  image,			\
			       int             x,			\
			       int             y,			\
			       int             width,			\
			       const uint32_t *values)			\
    {									\
	uint8_t *dest =							\
	    (uint8_t *)(image->bits + y * image->rowstride);		\
	int i;								\
									\
	for (i = 0; i < width; ++i)					\
	{								\
	    convert_and_store_pixel (					\
		image, dest, i + x, PIXMAN_ ## format, values[i]);	\
	}								\
    }									\
									\
    static uint32_t							\
    fetch_pixel_ ## format (bits_image_t *image,			\
			    int		offset,				\
			    int		line)				\
    {									\
	uint8_t *bits =							\
	    (uint8_t *)(image->bits + line * image->rowstride);		\
									\
	return fetch_and_convert_pixel (				\
	    image, bits, offset, PIXMAN_ ## format);			\
    }									\
									\
    static const void *const __dummy__ ## format MAYBE_UNUSED

MAKE_ACCESSORS(a8r8g8b8);
MAKE_ACCESSORS(x8r8g8b8);
MAKE_ACCESSORS(a8b8g8r8);
MAKE_ACCESSORS(x8b8g8r8);
MAKE_ACCESSORS(x14r6g6b6);
MAKE_ACCESSORS(b8g8r8a8);
MAKE_ACCESSORS(b8g8r8x8);
MAKE_ACCESSORS(r8g8b8x8);
MAKE_ACCESSORS(r8g8b8a8);
MAKE_ACCESSORS(r8g8b8);
MAKE_ACCESSORS(b8g8r8);
MAKE_ACCESSORS(r5g6b5);
MAKE_ACCESSORS(b5g6r5);
MAKE_ACCESSORS(a1r5g5b5);
MAKE_ACCESSORS(x1r5g5b5);
MAKE_ACCESSORS(a1b5g5r5);
MAKE_ACCESSORS(x1b5g5r5);
MAKE_ACCESSORS(a4r4g4b4);
MAKE_ACCESSORS(x4r4g4b4);
MAKE_ACCESSORS(a4b4g4r4);
MAKE_ACCESSORS(x4b4g4r4);
MAKE_ACCESSORS(a8);
MAKE_ACCESSORS(c8);
MAKE_ACCESSORS(g8);
MAKE_ACCESSORS(r3g3b2);
MAKE_ACCESSORS(b2g3r3);
MAKE_ACCESSORS(a2r2g2b2);
MAKE_ACCESSORS(a2b2g2r2);
MAKE_ACCESSORS(x4a4);
MAKE_ACCESSORS(a4);
MAKE_ACCESSORS(g4);
MAKE_ACCESSORS(c4);
MAKE_ACCESSORS(r1g2b1);
MAKE_ACCESSORS(b1g2r1);
MAKE_ACCESSORS(a1r1g1b1);
MAKE_ACCESSORS(a1b1g1r1);
MAKE_ACCESSORS(a1);
MAKE_ACCESSORS(g1);

/********************************** Fetch ************************************/
/* Table mapping sRGB-encoded 8 bit numbers to linearly encoded
 * floating point numbers. We assume that single precision
 * floating point follows the IEEE 754 format.
 */
static const uint32_t to_linear_u[256] =
{
    0x00000000, 0x399f22b4, 0x3a1f22b4, 0x3a6eb40e, 0x3a9f22b4, 0x3ac6eb61,
    0x3aeeb40e, 0x3b0b3e5d, 0x3b1f22b4, 0x3b33070b, 0x3b46eb61, 0x3b5b518a,
    0x3b70f18a, 0x3b83e1c5, 0x3b8fe614, 0x3b9c87fb, 0x3ba9c9b5, 0x3bb7ad6d,
    0x3bc63547, 0x3bd5635f, 0x3be539bd, 0x3bf5ba70, 0x3c0373b5, 0x3c0c6152,
    0x3c15a703, 0x3c1f45bc, 0x3c293e68, 0x3c3391f4, 0x3c3e4149, 0x3c494d43,
    0x3c54b6c7, 0x3c607eb1, 0x3c6ca5df, 0x3c792d22, 0x3c830aa8, 0x3c89af9e,
    0x3c9085db, 0x3c978dc5, 0x3c9ec7c0, 0x3ca63432, 0x3cadd37d, 0x3cb5a601,
    0x3cbdac20, 0x3cc5e639, 0x3cce54ab, 0x3cd6f7d2, 0x3cdfd00e, 0x3ce8ddb9,
    0x3cf2212c, 0x3cfb9ac1, 0x3d02a569, 0x3d0798dc, 0x3d0ca7e4, 0x3d11d2ae,
    0x3d171963, 0x3d1c7c2e, 0x3d21fb3a, 0x3d2796af, 0x3d2d4ebb, 0x3d332380,
    0x3d39152b, 0x3d3f23e3, 0x3d454fd0, 0x3d4b991c, 0x3d51ffeb, 0x3d588466,
    0x3d5f26b7, 0x3d65e6fe, 0x3d6cc564, 0x3d73c210, 0x3d7add25, 0x3d810b65,
    0x3d84b793, 0x3d88732e, 0x3d8c3e48, 0x3d9018f4, 0x3d940343, 0x3d97fd48,
    0x3d9c0714, 0x3da020b9, 0x3da44a48, 0x3da883d6, 0x3daccd70, 0x3db12728,
    0x3db59110, 0x3dba0b38, 0x3dbe95b2, 0x3dc3308f, 0x3dc7dbe0, 0x3dcc97b4,
    0x3dd1641c, 0x3dd6412a, 0x3ddb2eec, 0x3de02d75, 0x3de53cd3, 0x3dea5d16,
    0x3def8e52, 0x3df4d091, 0x3dfa23e5, 0x3dff885e, 0x3e027f06, 0x3e05427f,
    0x3e080ea2, 0x3e0ae376, 0x3e0dc104, 0x3e10a752, 0x3e139669, 0x3e168e50,
    0x3e198f0e, 0x3e1c98ab, 0x3e1fab2e, 0x3e22c6a0, 0x3e25eb08, 0x3e29186a,
    0x3e2c4ed0, 0x3e2f8e42, 0x3e32d6c4, 0x3e362861, 0x3e39831e, 0x3e3ce702,
    0x3e405416, 0x3e43ca5e, 0x3e4749e4, 0x3e4ad2ae, 0x3e4e64c2, 0x3e520027,
    0x3e55a4e6, 0x3e595303, 0x3e5d0a8a, 0x3e60cb7c, 0x3e6495e0, 0x3e6869bf,
    0x3e6c4720, 0x3e702e08, 0x3e741e7f, 0x3e78188c, 0x3e7c1c34, 0x3e8014c0,
    0x3e822039, 0x3e84308b, 0x3e8645b8, 0x3e885fc3, 0x3e8a7eb0, 0x3e8ca281,
    0x3e8ecb3a, 0x3e90f8df, 0x3e932b72, 0x3e9562f6, 0x3e979f6f, 0x3e99e0e0,
    0x3e9c274e, 0x3e9e72b8, 0x3ea0c322, 0x3ea31892, 0x3ea57308, 0x3ea7d28a,
    0x3eaa3718, 0x3eaca0b7, 0x3eaf0f69, 0x3eb18332, 0x3eb3fc16, 0x3eb67a15,
    0x3eb8fd34, 0x3ebb8576, 0x3ebe12de, 0x3ec0a56e, 0x3ec33d2a, 0x3ec5da14,
    0x3ec87c30, 0x3ecb2380, 0x3ecdd008, 0x3ed081ca, 0x3ed338c9, 0x3ed5f508,
    0x3ed8b68a, 0x3edb7d52, 0x3ede4962, 0x3ee11abe, 0x3ee3f168, 0x3ee6cd64,
    0x3ee9aeb6, 0x3eec955d, 0x3eef815d, 0x3ef272ba, 0x3ef56976, 0x3ef86594,
    0x3efb6717, 0x3efe6e02, 0x3f00bd2b, 0x3f02460c, 0x3f03d1a5, 0x3f055ff8,
    0x3f06f105, 0x3f0884ce, 0x3f0a1b54, 0x3f0bb499, 0x3f0d509f, 0x3f0eef65,
    0x3f1090ef, 0x3f12353c, 0x3f13dc50, 0x3f15862a, 0x3f1732cc, 0x3f18e237,
    0x3f1a946d, 0x3f1c4970, 0x3f1e013f, 0x3f1fbbde, 0x3f21794c, 0x3f23398c,
    0x3f24fca0, 0x3f26c286, 0x3f288b42, 0x3f2a56d3, 0x3f2c253d, 0x3f2df680,
    0x3f2fca9d, 0x3f31a195, 0x3f337b6a, 0x3f35581e, 0x3f3737b1, 0x3f391a24,
    0x3f3aff7a, 0x3f3ce7b2, 0x3f3ed2d0, 0x3f40c0d2, 0x3f42b1bc, 0x3f44a58e,
    0x3f469c49, 0x3f4895ee, 0x3f4a9280, 0x3f4c91ff, 0x3f4e946c, 0x3f5099c8,
    0x3f52a216, 0x3f54ad55, 0x3f56bb88, 0x3f58ccae, 0x3f5ae0cb, 0x3f5cf7de,
    0x3f5f11ec, 0x3f612ef0, 0x3f634eef, 0x3f6571ea, 0x3f6797e1, 0x3f69c0d6,
    0x3f6beccb, 0x3f6e1bc0, 0x3f704db6, 0x3f7282af, 0x3f74baac, 0x3f76f5ae,
    0x3f7933b6, 0x3f7b74c6, 0x3f7db8de, 0x3f800000
};

static const float * const to_linear = (const float *)to_linear_u;

static uint8_t
to_srgb (float f)
{
    uint8_t low = 0;
    uint8_t high = 255;

    while (high - low > 1)
    {
	uint8_t mid = (low + high) / 2;

	if (to_linear[mid] > f)
	    high = mid;
	else
	    low = mid;
    }

    if (to_linear[high] - f < f - to_linear[low])
	return high;
    else
	return low;
}

static void
fetch_scanline_a8r8g8b8_sRGB_float (bits_image_t *  image,
				    int             x,
				    int             y,
				    int             width,
				    uint32_t *      b,
				    const uint32_t *mask)
{
    const uint32_t *bits = image->bits + y * image->rowstride;
    const uint32_t *pixel = bits + x;
    const uint32_t *end = pixel + width;
    argb_t *buffer = (argb_t *)b;

    while (pixel < end)
    {
	uint32_t p = READ (image, pixel++);
	argb_t *argb = buffer;

	argb->a = pixman_unorm_to_float ((p >> 24) & 0xff, 8);

	argb->r = to_linear [(p >> 16) & 0xff];
	argb->g = to_linear [(p >>  8) & 0xff];
	argb->b = to_linear [(p >>  0) & 0xff];

	buffer++;
    }
}

static void
fetch_scanline_r8g8b8_sRGB_float (bits_image_t *  image,
				  int             x,
				  int             y,
				  int             width,
				  uint32_t *      b,
				  const uint32_t *mask)
{
    const uint8_t *bits = (uint8_t *)(image->bits + y * image->rowstride);
    argb_t *buffer = (argb_t *)b;
    int i;
    for (i = x; i < width; ++i)
    {
	uint32_t p = FETCH_24 (image, bits, i);
	argb_t *argb = buffer;

	argb->a = 1.0f;

	argb->r = to_linear[(p >> 16) & 0xff];
	argb->g = to_linear[(p >>  8) & 0xff];
	argb->b = to_linear[(p >>  0) & 0xff];

	buffer++;
    }
}

/* Expects a float buffer */
static void
fetch_scanline_a2r10g10b10_float (bits_image_t *  image,
				  int             x,
				  int             y,
				  int             width,
				  uint32_t *      b,
				  const uint32_t *mask)
{
    const uint32_t *bits = image->bits + y * image->rowstride;
    const uint32_t *pixel = bits + x;
    const uint32_t *end = pixel + width;
    argb_t *buffer = (argb_t *)b;

    while (pixel < end)
    {
	uint32_t p = READ (image, pixel++);
	uint64_t a = p >> 30;
	uint64_t r = (p >> 20) & 0x3ff;
	uint64_t g = (p >> 10) & 0x3ff;
	uint64_t b = p & 0x3ff;

	buffer->a = pixman_unorm_to_float (a, 2);
	buffer->r = pixman_unorm_to_float (r, 10);
	buffer->g = pixman_unorm_to_float (g, 10);
	buffer->b = pixman_unorm_to_float (b, 10);

	buffer++;
    }
}

/* Expects a float buffer */
#ifndef PIXMAN_FB_ACCESSORS
static void
fetch_scanline_rgbf_float (bits_image_t   *image,
			   int             x,
			   int             y,
			   int             width,
			   uint32_t *      b,
			   const uint32_t *mask)
{
    const float *bits = (float *)image->bits + y * image->rowstride;
    const float *pixel = bits + x * 3;
    argb_t *buffer = (argb_t *)b;

    for (; width--; buffer++) {
	buffer->r = *pixel++;
	buffer->g = *pixel++;
	buffer->b = *pixel++;
	buffer->a = 1.f;
    }
}

static void
fetch_scanline_rgbaf_float (bits_image_t   *image,
			    int             x,
			    int             y,
			    int             width,
			    uint32_t *      b,
			    const uint32_t *mask)
{
    const float *bits = (float *)image->bits + y * image->rowstride;
    const float *pixel = bits + x * 4;
    argb_t *buffer = (argb_t *)b;

    for (; width--; buffer++) {
	buffer->r = *pixel++;
	buffer->g = *pixel++;
	buffer->b = *pixel++;
	buffer->a = *pixel++;
    }
}
#endif

static void
fetch_scanline_x2r10g10b10_float (bits_image_t   *image,
				  int             x,
				  int             y,
				  int             width,
				  uint32_t *      b,
				  const uint32_t *mask)
{
    const uint32_t *bits = image->bits + y * image->rowstride;
    const uint32_t *pixel = (uint32_t *)bits + x;
    const uint32_t *end = pixel + width;
    argb_t *buffer = (argb_t *)b;

    while (pixel < end)
    {
	uint32_t p = READ (image, pixel++);
	uint64_t r = (p >> 20) & 0x3ff;
	uint64_t g = (p >> 10) & 0x3ff;
	uint64_t b = p & 0x3ff;

	buffer->a = 1.0;
	buffer->r = pixman_unorm_to_float (r, 10);
	buffer->g = pixman_unorm_to_float (g, 10);
	buffer->b = pixman_unorm_to_float (b, 10);

	buffer++;
    }
}

/* Expects a float buffer */
static void
fetch_scanline_a2b10g10r10_float (bits_image_t   *image,
				  int             x,
				  int             y,
				  int             width,
				  uint32_t *      b,
				  const uint32_t *mask)
{
    const uint32_t *bits = image->bits + y * image->rowstride;
    const uint32_t *pixel = bits + x;
    const uint32_t *end = pixel + width;
    argb_t *buffer = (argb_t *)b;

    while (pixel < end)
    {
	uint32_t p = READ (image, pixel++);
	uint64_t a = p >> 30;
	uint64_t b = (p >> 20) & 0x3ff;
	uint64_t g = (p >> 10) & 0x3ff;
	uint64_t r = p & 0x3ff;

	buffer->a = pixman_unorm_to_float (a, 2);
	buffer->r = pixman_unorm_to_float (r, 10);
	buffer->g = pixman_unorm_to_float (g, 10);
	buffer->b = pixman_unorm_to_float (b, 10);

	buffer++;
    }
}

/* Expects a float buffer */
static void
fetch_scanline_x2b10g10r10_float (bits_image_t   *image,
				  int             x,
				  int             y,
				  int             width,
				  uint32_t *      b,
				  const uint32_t *mask)
{
    const uint32_t *bits = image->bits + y * image->rowstride;
    const uint32_t *pixel = (uint32_t *)bits + x;
    const uint32_t *end = pixel + width;
    argb_t *buffer = (argb_t *)b;

    while (pixel < end)
    {
	uint32_t p = READ (image, pixel++);
	uint64_t b = (p >> 20) & 0x3ff;
	uint64_t g = (p >> 10) & 0x3ff;
	uint64_t r = p & 0x3ff;

	buffer->a = 1.0;
	buffer->r = pixman_unorm_to_float (r, 10);
	buffer->g = pixman_unorm_to_float (g, 10);
	buffer->b = pixman_unorm_to_float (b, 10);

	buffer++;
    }
}

static void
fetch_scanline_yuy2 (bits_image_t   *image,
                     int             x,
                     int             line,
                     int             width,
                     uint32_t *      buffer,
                     const uint32_t *mask)
{
    const uint32_t *bits = image->bits + image->rowstride * line;
    int i;
    
    for (i = 0; i < width; i++)
    {
	int16_t y, u, v;
	int32_t r, g, b;
	
	y = ((uint8_t *) bits)[(x + i) << 1] - 16;
	u = ((uint8_t *) bits)[(((x + i) << 1) & - 4) + 1] - 128;
	v = ((uint8_t *) bits)[(((x + i) << 1) & - 4) + 3] - 128;
	
	/* R = 1.164(Y - 16) + 1.596(V - 128) */
	r = 0x012b27 * y + 0x019a2e * v;
	/* G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128) */
	g = 0x012b27 * y - 0x00d0f2 * v - 0x00647e * u;
	/* B = 1.164(Y - 16) + 2.018(U - 128) */
	b = 0x012b27 * y + 0x0206a2 * u;
	
	*buffer++ = 0xff000000 |
	    (r >= 0 ? r < 0x1000000 ? r         & 0xff0000 : 0xff0000 : 0) |
	    (g >= 0 ? g < 0x1000000 ? (g >> 8)  & 0x00ff00 : 0x00ff00 : 0) |
	    (b >= 0 ? b < 0x1000000 ? (b >> 16) & 0x0000ff : 0x0000ff : 0);
    }
}

static void
fetch_scanline_yv12 (bits_image_t   *image,
                     int             x,
                     int             line,
                     int             width,
                     uint32_t *      buffer,
                     const uint32_t *mask)
{
    YV12_SETUP (image);
    uint8_t *y_line = YV12_Y (line);
    uint8_t *u_line = YV12_U (line);
    uint8_t *v_line = YV12_V (line);
    int i;
    
    for (i = 0; i < width; i++)
    {
	int16_t y, u, v;
	int32_t r, g, b;

	y = y_line[x + i] - 16;
	u = u_line[(x + i) >> 1] - 128;
	v = v_line[(x + i) >> 1] - 128;

	/* R = 1.164(Y - 16) + 1.596(V - 128) */
	r = 0x012b27 * y + 0x019a2e * v;
	/* G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128) */
	g = 0x012b27 * y - 0x00d0f2 * v - 0x00647e * u;
	/* B = 1.164(Y - 16) + 2.018(U - 128) */
	b = 0x012b27 * y + 0x0206a2 * u;

	*buffer++ = 0xff000000 |
	    (r >= 0 ? r < 0x1000000 ? r         & 0xff0000 : 0xff0000 : 0) |
	    (g >= 0 ? g < 0x1000000 ? (g >> 8)  & 0x00ff00 : 0x00ff00 : 0) |
	    (b >= 0 ? b < 0x1000000 ? (b >> 16) & 0x0000ff : 0x0000ff : 0);
    }
}

/**************************** Pixel wise fetching *****************************/

#ifndef PIXMAN_FB_ACCESSORS
static argb_t
fetch_pixel_rgbf_float (bits_image_t *image,
			int	    offset,
			int	    line)
{
    float *bits = (float *)image->bits + line * image->rowstride;
    argb_t argb;

    argb.r = bits[offset * 3];
    argb.g = bits[offset * 3 + 1];
    argb.b = bits[offset * 3 + 2];
    argb.a = 1.f;

    return argb;
}

static argb_t
fetch_pixel_rgbaf_float (bits_image_t *image,
			 int	    offset,
			 int	    line)
{
    float *bits = (float *)image->bits + line * image->rowstride;
    argb_t argb;

    argb.r = bits[offset * 4];
    argb.g = bits[offset * 4 + 1];
    argb.b = bits[offset * 4 + 2];
    argb.a = bits[offset * 4 + 3];

    return argb;
}
#endif

static argb_t
fetch_pixel_x2r10g10b10_float (bits_image_t *image,
			       int	   offset,
			       int           line)
{
    uint32_t *bits = image->bits + line * image->rowstride;
    uint32_t p = READ (image, bits + offset);
    uint64_t r = (p >> 20) & 0x3ff;
    uint64_t g = (p >> 10) & 0x3ff;
    uint64_t b = p & 0x3ff;
    argb_t argb;

    argb.a = 1.0;
    argb.r = pixman_unorm_to_float (r, 10);
    argb.g = pixman_unorm_to_float (g, 10);
    argb.b = pixman_unorm_to_float (b, 10);

    return argb;
}

static argb_t
fetch_pixel_a2r10g10b10_float (bits_image_t *image,
			       int	     offset,
			       int           line)
{
    uint32_t *bits = image->bits + line * image->rowstride;
    uint32_t p = READ (image, bits + offset);
    uint64_t a = p >> 30;
    uint64_t r = (p >> 20) & 0x3ff;
    uint64_t g = (p >> 10) & 0x3ff;
    uint64_t b = p & 0x3ff;
    argb_t argb;

    argb.a = pixman_unorm_to_float (a, 2);
    argb.r = pixman_unorm_to_float (r, 10);
    argb.g = pixman_unorm_to_float (g, 10);
    argb.b = pixman_unorm_to_float (b, 10);

    return argb;
}

static argb_t
fetch_pixel_a2b10g10r10_float (bits_image_t *image,
			       int           offset,
			       int           line)
{
    uint32_t *bits = image->bits + line * image->rowstride;
    uint32_t p = READ (image, bits + offset);
    uint64_t a = p >> 30;
    uint64_t b = (p >> 20) & 0x3ff;
    uint64_t g = (p >> 10) & 0x3ff;
    uint64_t r = p & 0x3ff;
    argb_t argb;

    argb.a = pixman_unorm_to_float (a, 2);
    argb.r = pixman_unorm_to_float (r, 10);
    argb.g = pixman_unorm_to_float (g, 10);
    argb.b = pixman_unorm_to_float (b, 10);

    return argb;
}

static argb_t
fetch_pixel_x2b10g10r10_float (bits_image_t *image,
			       int           offset,
			       int           line)
{
    uint32_t *bits = image->bits + line * image->rowstride;
    uint32_t p = READ (image, bits + offset);
    uint64_t b = (p >> 20) & 0x3ff;
    uint64_t g = (p >> 10) & 0x3ff;
    uint64_t r = p & 0x3ff;
    argb_t argb;

    argb.a = 1.0;
    argb.r = pixman_unorm_to_float (r, 10);
    argb.g = pixman_unorm_to_float (g, 10);
    argb.b = pixman_unorm_to_float (b, 10);

    return argb;
}

static argb_t
fetch_pixel_a8r8g8b8_sRGB_float (bits_image_t *image,
				 int	       offset,
				 int           line)
{
    uint32_t *bits = image->bits + line * image->rowstride;
    uint32_t p = READ (image, bits + offset);
    argb_t argb;

    argb.a = pixman_unorm_to_float ((p >> 24) & 0xff, 8);

    argb.r = to_linear [(p >> 16) & 0xff];
    argb.g = to_linear [(p >>  8) & 0xff];
    argb.b = to_linear [(p >>  0) & 0xff];

    return argb;
}

static argb_t
fetch_pixel_r8g8b8_sRGB_float (bits_image_t *image,
			       int	     offset,
			       int           line)
{
    uint8_t *bits = (uint8_t *)(image->bits + line * image->rowstride);
    uint32_t p = FETCH_24 (image, bits, offset);
    argb_t argb;

    argb.a = 1.0f;

    argb.r = to_linear[(p >> 16) & 0xff];
    argb.g = to_linear[(p >>  8) & 0xff];
    argb.b = to_linear[(p >>  0) & 0xff];

    return argb;
}

static uint32_t
fetch_pixel_yuy2 (bits_image_t *image,
		  int           offset,
		  int           line)
{
    const uint32_t *bits = image->bits + image->rowstride * line;
    
    int16_t y, u, v;
    int32_t r, g, b;
    
    y = ((uint8_t *) bits)[offset << 1] - 16;
    u = ((uint8_t *) bits)[((offset << 1) & - 4) + 1] - 128;
    v = ((uint8_t *) bits)[((offset << 1) & - 4) + 3] - 128;
    
    /* R = 1.164(Y - 16) + 1.596(V - 128) */
    r = 0x012b27 * y + 0x019a2e * v;
    
    /* G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128) */
    g = 0x012b27 * y - 0x00d0f2 * v - 0x00647e * u;
    
    /* B = 1.164(Y - 16) + 2.018(U - 128) */
    b = 0x012b27 * y + 0x0206a2 * u;
    
    return 0xff000000 |
	(r >= 0 ? r < 0x1000000 ? r         & 0xff0000 : 0xff0000 : 0) |
	(g >= 0 ? g < 0x1000000 ? (g >> 8)  & 0x00ff00 : 0x00ff00 : 0) |
	(b >= 0 ? b < 0x1000000 ? (b >> 16) & 0x0000ff : 0x0000ff : 0);
}

static uint32_t
fetch_pixel_yv12 (bits_image_t *image,
		  int           offset,
		  int           line)
{
    YV12_SETUP (image);
    int16_t y = YV12_Y (line)[offset] - 16;
    int16_t u = YV12_U (line)[offset >> 1] - 128;
    int16_t v = YV12_V (line)[offset >> 1] - 128;
    int32_t r, g, b;
    
    /* R = 1.164(Y - 16) + 1.596(V - 128) */
    r = 0x012b27 * y + 0x019a2e * v;
    
    /* G = 1.164(Y - 16) - 0.813(V - 128) - 0.391(U - 128) */
    g = 0x012b27 * y - 0x00d0f2 * v - 0x00647e * u;
    
    /* B = 1.164(Y - 16) + 2.018(U - 128) */
    b = 0x012b27 * y + 0x0206a2 * u;
    
    return 0xff000000 |
	(r >= 0 ? r < 0x1000000 ? r         & 0xff0000 : 0xff0000 : 0) |
	(g >= 0 ? g < 0x1000000 ? (g >> 8)  & 0x00ff00 : 0x00ff00 : 0) |
	(b >= 0 ? b < 0x1000000 ? (b >> 16) & 0x0000ff : 0x0000ff : 0);
}

/*********************************** Store ************************************/

#ifndef PIXMAN_FB_ACCESSORS
static void
store_scanline_rgbaf_float (bits_image_t *  image,
			    int             x,
			    int             y,
			    int             width,
			    const uint32_t *v)
{
    float *bits = (float *)image->bits + image->rowstride * y + 4 * x;
    const argb_t *values = (argb_t *)v;

    for (; width; width--, values++)
    {
	*bits++ = values->r;
	*bits++ = values->g;
	*bits++ = values->b;
	*bits++ = values->a;
    }
}

static void
store_scanline_rgbf_float (bits_image_t *  image,
			   int             x,
			   int             y,
			   int             width,
			   const uint32_t *v)
{
    float *bits = (float *)image->bits + image->rowstride * y + 3 * x;
    const argb_t *values = (argb_t *)v;

    for (; width; width--, values++)
    {
	*bits++ = values->r;
	*bits++ = values->g;
	*bits++ = values->b;
    }
}
#endif

static void
store_scanline_a2r10g10b10_float (bits_image_t *  image,
				  int             x,
				  int             y,
				  int             width,
				  const uint32_t *v)
{
    uint32_t *bits = image->bits + image->rowstride * y;
    uint32_t *pixel = bits + x;
    argb_t *values = (argb_t *)v;
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t a, r, g, b;

	a = pixman_float_to_unorm (values[i].a, 2);
	r = pixman_float_to_unorm (values[i].r, 10);
	g = pixman_float_to_unorm (values[i].g, 10);
	b = pixman_float_to_unorm (values[i].b, 10);

	WRITE (image, pixel++,
	       (a << 30) | (r << 20) | (g << 10) | b);
    }
}

static void
store_scanline_x2r10g10b10_float (bits_image_t *  image,
				  int             x,
				  int             y,
				  int             width,
				  const uint32_t *v)
{
    uint32_t *bits = image->bits + image->rowstride * y;
    uint32_t *pixel = bits + x;
    argb_t *values = (argb_t *)v;
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t r, g, b;

	r = pixman_float_to_unorm (values[i].r, 10);
	g = pixman_float_to_unorm (values[i].g, 10);
	b = pixman_float_to_unorm (values[i].b, 10);

	WRITE (image, pixel++,
	       (r << 20) | (g << 10) | b);
    }
}

static void
store_scanline_a2b10g10r10_float (bits_image_t *  image,
				  int             x,
				  int             y,
				  int             width,
				  const uint32_t *v)
{
    uint32_t *bits = image->bits + image->rowstride * y;
    uint32_t *pixel = bits + x;
    argb_t *values = (argb_t *)v;
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t a, r, g, b;

	a = pixman_float_to_unorm (values[i].a, 2);
	r = pixman_float_to_unorm (values[i].r, 10);
	g = pixman_float_to_unorm (values[i].g, 10);
	b = pixman_float_to_unorm (values[i].b, 10);

	WRITE (image, pixel++,
	       (a << 30) | (b << 20) | (g << 10) | r);
    }
}

static void
store_scanline_x2b10g10r10_float (bits_image_t *  image,
				  int             x,
				  int             y,
				  int             width,
				  const uint32_t *v)
{
    uint32_t *bits = image->bits + image->rowstride * y;
    uint32_t *pixel = bits + x;
    argb_t *values = (argb_t *)v;
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t r, g, b;

	r = pixman_float_to_unorm (values[i].r, 10);
	g = pixman_float_to_unorm (values[i].g, 10);
	b = pixman_float_to_unorm (values[i].b, 10);

	WRITE (image, pixel++,
	       (b << 20) | (g << 10) | r);
    }
}

static void
store_scanline_a8r8g8b8_sRGB_float (bits_image_t *  image,
				    int             x,
				    int             y,
				    int             width,
				    const uint32_t *v)
{
    uint32_t *bits = image->bits + image->rowstride * y;
    uint32_t *pixel = bits + x;
    argb_t *values = (argb_t *)v;
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t a, r, g, b;

	a = pixman_float_to_unorm (values[i].a, 8);
	r = to_srgb (values[i].r);
	g = to_srgb (values[i].g);
	b = to_srgb (values[i].b);

	WRITE (image, pixel++,
	       (a << 24) | (r << 16) | (g << 8) | b);
    }
}

static void
store_scanline_r8g8b8_sRGB_float (bits_image_t *  image,
				  int             x,
				  int             y,
				  int             width,
				  const uint32_t *v)
{
    uint8_t *bits = (uint8_t *)(image->bits + image->rowstride * y) + 3 * x;
    argb_t *values = (argb_t *)v;
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t r, g, b, rgb;

	r = to_srgb (values[i].r);
	g = to_srgb (values[i].g);
	b = to_srgb (values[i].b);

	rgb = (r << 16) | (g << 8) | b;

	STORE_24 (image, bits, i, rgb);
    }
}

/*
 * Contracts a floating point image to 32bpp and then stores it using a
 * regular 32-bit store proc. Despite the type, this function expects an
 * argb_t buffer.
 */
static void
store_scanline_generic_float (bits_image_t *  image,
			      int             x,
			      int             y,
			      int             width,
			      const uint32_t *values)
{
    uint32_t *argb8_pixels;

    assert (image->common.type == BITS);

    argb8_pixels = pixman_malloc_ab (width, sizeof(uint32_t));
    if (!argb8_pixels)
	return;

    /* Contract the scanline.  We could do this in place if values weren't
     * const.
     */
    pixman_contract_from_float (argb8_pixels, (argb_t *)values, width);

    image->store_scanline_32 (image, x, y, width, argb8_pixels);

    free (argb8_pixels);
}

static void
fetch_scanline_generic_float (bits_image_t *  image,
			      int	      x,
			      int	      y,
			      int	      width,
			      uint32_t *      buffer,
			      const uint32_t *mask)
{
    image->fetch_scanline_32 (image, x, y, width, buffer, NULL);

    pixman_expand_to_float ((argb_t *)buffer, buffer, image->format, width);
}

/* The 32_sRGB paths should be deleted after narrow processing
 * is no longer invoked for formats that are considered wide.
 * (Also see fetch_pixel_generic_lossy_32) */
static void
fetch_scanline_a8r8g8b8_32_sRGB (bits_image_t   *image,
                                 int             x,
                                 int             y,
                                 int             width,
                                 uint32_t       *buffer,
                                 const uint32_t *mask)
{
    const uint32_t *bits = image->bits + y * image->rowstride;
    const uint32_t *pixel = (uint32_t *)bits + x;
    const uint32_t *end = pixel + width;
    uint32_t tmp;
    
    while (pixel < end)
    {
	uint32_t a, r, g, b;

	tmp = READ (image, pixel++);

	a = (tmp >> 24) & 0xff;
	r = (tmp >> 16) & 0xff;
	g = (tmp >> 8) & 0xff;
	b = (tmp >> 0) & 0xff;

	r = to_linear[r] * 255.0f + 0.5f;
	g = to_linear[g] * 255.0f + 0.5f;
	b = to_linear[b] * 255.0f + 0.5f;

	*buffer++ = (a << 24) | (r << 16) | (g << 8) | (b << 0);
    }
}

static void
fetch_scanline_r8g8b8_32_sRGB (bits_image_t   *image,
                               int             x,
                               int             y,
                               int             width,
                               uint32_t       *buffer,
                               const uint32_t *mask)
{
    const uint8_t *bits = (uint8_t *)(image->bits + y * image->rowstride) + 3 * x;
    uint32_t tmp;
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t a, r, g, b;

	tmp = FETCH_24 (image, bits, i);

	a = 0xff;
	r = (tmp >> 16) & 0xff;
	g = (tmp >> 8) & 0xff;
	b = (tmp >> 0) & 0xff;

	r = to_linear[r] * 255.0f + 0.5f;
	g = to_linear[g] * 255.0f + 0.5f;
	b = to_linear[b] * 255.0f + 0.5f;

	*buffer++ = (a << 24) | (r << 16) | (g << 8) | (b << 0);
    }
}

static uint32_t
fetch_pixel_a8r8g8b8_32_sRGB (bits_image_t *image,
			      int           offset,
			      int           line)
{
    uint32_t *bits = image->bits + line * image->rowstride;
    uint32_t tmp = READ (image, bits + offset);
    uint32_t a, r, g, b;

    a = (tmp >> 24) & 0xff;
    r = (tmp >> 16) & 0xff;
    g = (tmp >> 8) & 0xff;
    b = (tmp >> 0) & 0xff;

    r = to_linear[r] * 255.0f + 0.5f;
    g = to_linear[g] * 255.0f + 0.5f;
    b = to_linear[b] * 255.0f + 0.5f;

    return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

static uint32_t
fetch_pixel_r8g8b8_32_sRGB (bits_image_t *image,
			    int           offset,
			    int           line)
{
    uint8_t *bits = (uint8_t *)(image->bits + line * image->rowstride);
    uint32_t tmp = FETCH_24 (image, bits, offset);
    uint32_t a, r, g, b;

    a = 0xff;
    r = (tmp >> 16) & 0xff;
    g = (tmp >> 8) & 0xff;
    b = (tmp >> 0) & 0xff;

    r = to_linear[r] * 255.0f + 0.5f;
    g = to_linear[g] * 255.0f + 0.5f;
    b = to_linear[b] * 255.0f + 0.5f;

    return (a << 24) | (r << 16) | (g << 8) | (b << 0);
}

static void
store_scanline_a8r8g8b8_32_sRGB (bits_image_t   *image,
                                 int             x,
                                 int             y,
                                 int             width,
                                 const uint32_t *v)
{
    uint32_t *bits = image->bits + image->rowstride * y;
    uint64_t *values = (uint64_t *)v;
    uint32_t *pixel = bits + x;
    uint64_t tmp;
    int i;
    
    for (i = 0; i < width; ++i)
    {
	uint32_t a, r, g, b;

	tmp = values[i];

	a = (tmp >> 24) & 0xff;
	r = (tmp >> 16) & 0xff;
	g = (tmp >> 8) & 0xff;
	b = (tmp >> 0) & 0xff;

	r = to_srgb (r * (1/255.0f));
	g = to_srgb (g * (1/255.0f));
	b = to_srgb (b * (1/255.0f));
	
	WRITE (image, pixel++, a | (r << 16) | (g << 8) | (b << 0));
    }
}

static void
store_scanline_r8g8b8_32_sRGB (bits_image_t   *image,
			       int             x,
                               int             y,
                               int             width,
                               const uint32_t *v)
{
    uint8_t *bits = (uint8_t *)(image->bits + image->rowstride * y) + 3 * x;
    uint64_t *values = (uint64_t *)v;
    uint64_t tmp;
    int i;

    for (i = 0; i < width; ++i)
    {
	uint32_t r, g, b;

	tmp = values[i];

	r = (tmp >> 16) & 0xff;
	g = (tmp >> 8) & 0xff;
	b = (tmp >> 0) & 0xff;

	r = to_srgb (r * (1/255.0f));
	g = to_srgb (g * (1/255.0f));
	b = to_srgb (b * (1/255.0f));

	STORE_24 (image, bits, i, (r << 16) | (g << 8) | (b << 0));
    }
}

static argb_t
fetch_pixel_generic_float (bits_image_t *image,
			   int		 offset,
			   int           line)
{
    uint32_t pixel32 = image->fetch_pixel_32 (image, offset, line);
    argb_t f;

    pixman_expand_to_float (&f, &pixel32, image->format, 1);

    return f;
}

/*
 * XXX: The transformed fetch path only works at 32-bpp so far.  When all
 * paths have wide versions, this can be removed.
 *
 * WARNING: This function loses precision!
 */
static uint32_t
fetch_pixel_generic_lossy_32 (bits_image_t *image,
			      int           offset,
			      int           line)
{
    argb_t pixel64 = image->fetch_pixel_float (image, offset, line);
    uint32_t result;

    pixman_contract_from_float (&result, &pixel64, 1);

    return result;
}

typedef struct
{
    pixman_format_code_t	format;
    fetch_scanline_t		fetch_scanline_32;
    fetch_scanline_t		fetch_scanline_float;
    fetch_pixel_32_t		fetch_pixel_32;
    fetch_pixel_float_t		fetch_pixel_float;
    store_scanline_t		store_scanline_32;
    store_scanline_t		store_scanline_float;
} format_info_t;

#define FORMAT_INFO(format) 						\
    {									\
	PIXMAN_ ## format,						\
	    fetch_scanline_ ## format,					\
	    fetch_scanline_generic_float,				\
	    fetch_pixel_ ## format,					\
	    fetch_pixel_generic_float,					\
	    store_scanline_ ## format,					\
	    store_scanline_generic_float				\
    }

static const format_info_t accessors[] =
{
/* 32 bpp formats */
    FORMAT_INFO (a8r8g8b8),
    FORMAT_INFO (x8r8g8b8),
    FORMAT_INFO (a8b8g8r8),
    FORMAT_INFO (x8b8g8r8),
    FORMAT_INFO (b8g8r8a8),
    FORMAT_INFO (b8g8r8x8),
    FORMAT_INFO (r8g8b8a8),
    FORMAT_INFO (r8g8b8x8),
    FORMAT_INFO (x14r6g6b6),

/* sRGB formats */
  { PIXMAN_a8r8g8b8_sRGB,
    fetch_scanline_a8r8g8b8_32_sRGB, fetch_scanline_a8r8g8b8_sRGB_float,
    fetch_pixel_a8r8g8b8_32_sRGB, fetch_pixel_a8r8g8b8_sRGB_float,
    store_scanline_a8r8g8b8_32_sRGB, store_scanline_a8r8g8b8_sRGB_float,
  },
  { PIXMAN_r8g8b8_sRGB,
    fetch_scanline_r8g8b8_32_sRGB, fetch_scanline_r8g8b8_sRGB_float,
    fetch_pixel_r8g8b8_32_sRGB, fetch_pixel_r8g8b8_sRGB_float,
    store_scanline_r8g8b8_32_sRGB, store_scanline_r8g8b8_sRGB_float,
  },

/* 24bpp formats */
    FORMAT_INFO (r8g8b8),
    FORMAT_INFO (b8g8r8),
    
/* 16bpp formats */
    FORMAT_INFO (r5g6b5),
    FORMAT_INFO (b5g6r5),
    
    FORMAT_INFO (a1r5g5b5),
    FORMAT_INFO (x1r5g5b5),
    FORMAT_INFO (a1b5g5r5),
    FORMAT_INFO (x1b5g5r5),
    FORMAT_INFO (a4r4g4b4),
    FORMAT_INFO (x4r4g4b4),
    FORMAT_INFO (a4b4g4r4),
    FORMAT_INFO (x4b4g4r4),
    
/* 8bpp formats */
    FORMAT_INFO (a8),
    FORMAT_INFO (r3g3b2),
    FORMAT_INFO (b2g3r3),
    FORMAT_INFO (a2r2g2b2),
    FORMAT_INFO (a2b2g2r2),
    
    FORMAT_INFO (c8),
    
    FORMAT_INFO (g8),
    
#define fetch_scanline_x4c4 fetch_scanline_c8
#define fetch_pixel_x4c4 fetch_pixel_c8
#define store_scanline_x4c4 store_scanline_c8
    FORMAT_INFO (x4c4),
    
#define fetch_scanline_x4g4 fetch_scanline_g8
#define fetch_pixel_x4g4 fetch_pixel_g8
#define store_scanline_x4g4 store_scanline_g8
    FORMAT_INFO (x4g4),
    
    FORMAT_INFO (x4a4),
    
/* 4bpp formats */
    FORMAT_INFO (a4),
    FORMAT_INFO (r1g2b1),
    FORMAT_INFO (b1g2r1),
    FORMAT_INFO (a1r1g1b1),
    FORMAT_INFO (a1b1g1r1),
    
    FORMAT_INFO (c4),
    
    FORMAT_INFO (g4),
    
/* 1bpp formats */
    FORMAT_INFO (a1),
    FORMAT_INFO (g1),
    
/* Wide formats */
#ifndef PIXMAN_FB_ACCESSORS
    { PIXMAN_rgba_float,
      NULL, fetch_scanline_rgbaf_float,
      fetch_pixel_generic_lossy_32, fetch_pixel_rgbaf_float,
      NULL, store_scanline_rgbaf_float },

    { PIXMAN_rgb_float,
      NULL, fetch_scanline_rgbf_float,
      fetch_pixel_generic_lossy_32, fetch_pixel_rgbf_float,
      NULL, store_scanline_rgbf_float },
#endif

    { PIXMAN_a2r10g10b10,
      NULL, fetch_scanline_a2r10g10b10_float,
      fetch_pixel_generic_lossy_32, fetch_pixel_a2r10g10b10_float,
      NULL, store_scanline_a2r10g10b10_float },

    { PIXMAN_x2r10g10b10,
      NULL, fetch_scanline_x2r10g10b10_float,
      fetch_pixel_generic_lossy_32, fetch_pixel_x2r10g10b10_float,
      NULL, store_scanline_x2r10g10b10_float },

    { PIXMAN_a2b10g10r10,
      NULL, fetch_scanline_a2b10g10r10_float,
      fetch_pixel_generic_lossy_32, fetch_pixel_a2b10g10r10_float,
      NULL, store_scanline_a2b10g10r10_float },

    { PIXMAN_x2b10g10r10,
      NULL, fetch_scanline_x2b10g10r10_float,
      fetch_pixel_generic_lossy_32, fetch_pixel_x2b10g10r10_float,
      NULL, store_scanline_x2b10g10r10_float },

/* YUV formats */
    { PIXMAN_yuy2,
      fetch_scanline_yuy2, fetch_scanline_generic_float,
      fetch_pixel_yuy2, fetch_pixel_generic_float,
      NULL, NULL },

    { PIXMAN_yv12,
      fetch_scanline_yv12, fetch_scanline_generic_float,
      fetch_pixel_yv12, fetch_pixel_generic_float,
      NULL, NULL },
    
    { PIXMAN_null },
};

static void
setup_accessors (bits_image_t *image)
{
    const format_info_t *info = accessors;
    
    while (info->format != PIXMAN_null)
    {
	if (info->format == image->format)
	{
	    image->fetch_scanline_32 = info->fetch_scanline_32;
	    image->fetch_scanline_float = info->fetch_scanline_float;
	    image->fetch_pixel_32 = info->fetch_pixel_32;
	    image->fetch_pixel_float = info->fetch_pixel_float;
	    image->store_scanline_32 = info->store_scanline_32;
	    image->store_scanline_float = info->store_scanline_float;
	    
	    return;
	}
	
	info++;
    }
}

#ifndef PIXMAN_FB_ACCESSORS
void
_pixman_bits_image_setup_accessors_accessors (bits_image_t *image);

void
_pixman_bits_image_setup_accessors (bits_image_t *image)
{
    if (image->read_func || image->write_func)
	_pixman_bits_image_setup_accessors_accessors (image);
    else
	setup_accessors (image);
}

#else

void
_pixman_bits_image_setup_accessors_accessors (bits_image_t *image)
{
    setup_accessors (image);
}

#endif
