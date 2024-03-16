#ifndef PIXMAN_PRIVATE_H
#define PIXMAN_PRIVATE_H

/*
 * The defines which are shared between C and assembly code
 */

/* bilinear interpolation precision (must be < 8) */
#define BILINEAR_INTERPOLATION_BITS 7
#define BILINEAR_INTERPOLATION_RANGE (1 << BILINEAR_INTERPOLATION_BITS)

/*
 * C specific part
 */

#ifndef __ASSEMBLER__

#ifndef PACKAGE
#  error config.h must be included before pixman-private.h
#endif

#define PIXMAN_DISABLE_DEPRECATED
#define PIXMAN_USE_INTERNAL_API

#include "pixman.h"
#include <time.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <float.h>

#include "pixman-compiler.h"

/*
 * Images
 */
typedef struct image_common image_common_t;
typedef struct solid_fill solid_fill_t;
typedef struct gradient gradient_t;
typedef struct linear_gradient linear_gradient_t;
typedef struct horizontal_gradient horizontal_gradient_t;
typedef struct vertical_gradient vertical_gradient_t;
typedef struct conical_gradient conical_gradient_t;
typedef struct radial_gradient radial_gradient_t;
typedef struct bits_image bits_image_t;
typedef struct circle circle_t;

typedef struct argb_t argb_t;

struct argb_t
{
    float a;
    float r;
    float g;
    float b;
};

typedef void (*fetch_scanline_t) (bits_image_t   *image,
				  int             x,
				  int             y,
				  int             width,
				  uint32_t       *buffer,
				  const uint32_t *mask);

typedef uint32_t (*fetch_pixel_32_t) (bits_image_t *image,
				      int           x,
				      int           y);

typedef argb_t (*fetch_pixel_float_t) (bits_image_t *image,
				       int           x,
				       int           y);

typedef void (*store_scanline_t) (bits_image_t *  image,
				  int             x,
				  int             y,
				  int             width,
				  const uint32_t *values);

typedef enum
{
    BITS,
    LINEAR,
    CONICAL,
    RADIAL,
    SOLID
} image_type_t;

typedef void (*property_changed_func_t) (pixman_image_t *image);

struct image_common
{
    image_type_t                type;
    int32_t                     ref_count;
    pixman_region32_t           clip_region;
    int32_t			alpha_count;	    /* How many times this image is being used as an alpha map */
    pixman_bool_t               have_clip_region;   /* FALSE if there is no clip */
    pixman_bool_t               client_clip;        /* Whether the source clip was
						       set by a client */
    pixman_bool_t               clip_sources;       /* Whether the clip applies when
						     * the image is used as a source
						     */
    pixman_bool_t		dirty;
    pixman_transform_t *        transform;
    pixman_repeat_t             repeat;
    pixman_filter_t             filter;
    pixman_fixed_t *            filter_params;
    int                         n_filter_params;
    bits_image_t *              alpha_map;
    int                         alpha_origin_x;
    int                         alpha_origin_y;
    pixman_bool_t               component_alpha;
    property_changed_func_t     property_changed;

    pixman_image_destroy_func_t destroy_func;
    void *                      destroy_data;

    uint32_t			flags;
    pixman_format_code_t	extended_format_code;
};

struct solid_fill
{
    image_common_t common;
    pixman_color_t color;

    uint32_t	   color_32;
    argb_t	   color_float;
};

struct gradient
{
    image_common_t	    common;
    int                     n_stops;
    pixman_gradient_stop_t *stops;
};

struct linear_gradient
{
    gradient_t           common;
    pixman_point_fixed_t p1;
    pixman_point_fixed_t p2;
};

struct circle
{
    pixman_fixed_t x;
    pixman_fixed_t y;
    pixman_fixed_t radius;
};

struct radial_gradient
{
    gradient_t common;

    circle_t   c1;
    circle_t   c2;

    circle_t   delta;
    double     a;
    double     inva;
    double     mindr;
};

struct conical_gradient
{
    gradient_t           common;
    pixman_point_fixed_t center;
    double		 angle;
};

struct bits_image
{
    image_common_t             common;
    pixman_format_code_t       format;
    const pixman_indexed_t *   indexed;
    int                        width;
    int                        height;
    uint32_t *                 bits;
    uint32_t *                 free_me;
    int                        rowstride;  /* in number of uint32_t's */

    pixman_dither_t            dither;
    uint32_t                   dither_offset_y;
    uint32_t                   dither_offset_x;

    fetch_scanline_t           fetch_scanline_32;
    fetch_pixel_32_t	       fetch_pixel_32;
    store_scanline_t           store_scanline_32;

    fetch_scanline_t	       fetch_scanline_float;
    fetch_pixel_float_t	       fetch_pixel_float;
    store_scanline_t           store_scanline_float;

    /* Used for indirect access to the bits */
    pixman_read_memory_func_t  read_func;
    pixman_write_memory_func_t write_func;
};

union pixman_image
{
    image_type_t       type;
    image_common_t     common;
    bits_image_t       bits;
    gradient_t         gradient;
    linear_gradient_t  linear;
    conical_gradient_t conical;
    radial_gradient_t  radial;
    solid_fill_t       solid;
};

typedef struct pixman_iter_t pixman_iter_t;
typedef uint32_t *(* pixman_iter_get_scanline_t) (pixman_iter_t *iter, const uint32_t *mask);
typedef void      (* pixman_iter_write_back_t)   (pixman_iter_t *iter);
typedef void	  (* pixman_iter_fini_t)	 (pixman_iter_t *iter);

typedef enum
{
    ITER_NARROW =               (1 << 0),
    ITER_WIDE =                 (1 << 1),

    /* "Localized alpha" is when the alpha channel is used only to compute
     * the alpha value of the destination. This means that the computation
     * of the RGB values of the result is independent of the alpha value.
     *
     * For example, the OVER operator has localized alpha for the
     * destination, because the RGB values of the result can be computed
     * without knowing the destination alpha. Similarly, ADD has localized
     * alpha for both source and destination because the RGB values of the
     * result can be computed without knowing the alpha value of source or
     * destination.
     *
     * When he destination is xRGB, this is useful knowledge, because then
     * we can treat it as if it were ARGB, which means in some cases we can
     * avoid copying it to a temporary buffer.
     */
    ITER_LOCALIZED_ALPHA =	(1 << 2),
    ITER_IGNORE_ALPHA =		(1 << 3),
    ITER_IGNORE_RGB =		(1 << 4),

    /* These indicate whether the iterator is for a source
     * or a destination image
     */
    ITER_SRC =			(1 << 5),
    ITER_DEST =			(1 << 6)
} iter_flags_t;

struct pixman_iter_t
{
    /* These are initialized by _pixman_implementation_{src,dest}_init */
    pixman_image_t *		image;
    uint32_t *			buffer;
    int				x, y;
    int				width;
    int				height;
    iter_flags_t		iter_flags;
    uint32_t			image_flags;

    /* These function pointers are initialized by the implementation */
    pixman_iter_get_scanline_t	get_scanline;
    pixman_iter_write_back_t	write_back;
    pixman_iter_fini_t          fini;

    /* These fields are scratch data that implementations can use */
    void *			data;
    uint8_t *			bits;
    int				stride;
};

typedef struct pixman_iter_info_t pixman_iter_info_t;
typedef void (* pixman_iter_initializer_t) (pixman_iter_t *iter,
                                            const pixman_iter_info_t *info);
struct pixman_iter_info_t
{
    pixman_format_code_t	format;
    uint32_t			image_flags;
    iter_flags_t		iter_flags;
    pixman_iter_initializer_t	initializer;
    pixman_iter_get_scanline_t	get_scanline;
    pixman_iter_write_back_t	write_back;
};

void
_pixman_bits_image_setup_accessors (bits_image_t *image);

void
_pixman_bits_image_src_iter_init (pixman_image_t *image, pixman_iter_t *iter);

void
_pixman_bits_image_dest_iter_init (pixman_image_t *image, pixman_iter_t *iter);

void
_pixman_linear_gradient_iter_init (pixman_image_t *image, pixman_iter_t  *iter);

void
_pixman_radial_gradient_iter_init (pixman_image_t *image, pixman_iter_t *iter);

void
_pixman_conical_gradient_iter_init (pixman_image_t *image, pixman_iter_t *iter);

void
_pixman_image_init (pixman_image_t *image);

pixman_bool_t
_pixman_bits_image_init (pixman_image_t *     image,
                         pixman_format_code_t format,
                         int                  width,
                         int                  height,
                         uint32_t *           bits,
                         int                  rowstride,
			 pixman_bool_t	      clear);
pixman_bool_t
_pixman_image_fini (pixman_image_t *image);

pixman_image_t *
_pixman_image_allocate (void);

pixman_bool_t
_pixman_init_gradient (gradient_t *                  gradient,
                       const pixman_gradient_stop_t *stops,
                       int                           n_stops);
void
_pixman_image_reset_clip_region (pixman_image_t *image);

void
_pixman_image_validate (pixman_image_t *image);

#define PIXMAN_IMAGE_GET_LINE(image, x, y, type, out_stride, line, mul)	\
    do									\
    {									\
	uint32_t *__bits__;						\
	int       __stride__;						\
        								\
	__bits__ = image->bits.bits;					\
	__stride__ = image->bits.rowstride;				\
	(out_stride) =							\
	    __stride__ * (int) sizeof (uint32_t) / (int) sizeof (type);	\
	(line) =							\
	    ((type *) __bits__) + (out_stride) * (y) + (mul) * (x);	\
    } while (0)

/*
 * Gradient walker
 */
typedef struct
{
    float		    a_s, a_b;
    float		    r_s, r_b;
    float		    g_s, g_b;
    float		    b_s, b_b;
    pixman_fixed_48_16_t    left_x;
    pixman_fixed_48_16_t    right_x;

    pixman_gradient_stop_t *stops;
    int                     num_stops;
    pixman_repeat_t	    repeat;

    pixman_bool_t           need_reset;
} pixman_gradient_walker_t;

void
_pixman_gradient_walker_init (pixman_gradient_walker_t *walker,
                              gradient_t *              gradient,
			      pixman_repeat_t           repeat);

void
_pixman_gradient_walker_reset (pixman_gradient_walker_t *walker,
                               pixman_fixed_48_16_t      pos);

typedef void (*pixman_gradient_walker_write_t) (
    pixman_gradient_walker_t *walker,
    pixman_fixed_48_16_t      x,
    uint32_t                 *buffer);

void
_pixman_gradient_walker_write_narrow(pixman_gradient_walker_t *walker,
				     pixman_fixed_48_16_t      x,
				     uint32_t                 *buffer);

void
_pixman_gradient_walker_write_wide(pixman_gradient_walker_t *walker,
				   pixman_fixed_48_16_t      x,
				   uint32_t                 *buffer);

typedef void (*pixman_gradient_walker_fill_t) (
    pixman_gradient_walker_t *walker,
    pixman_fixed_48_16_t      x,
    uint32_t                 *buffer,
    uint32_t                 *end);

void
_pixman_gradient_walker_fill_narrow(pixman_gradient_walker_t *walker,
				    pixman_fixed_48_16_t      x,
				    uint32_t                 *buffer,
				    uint32_t                 *end);

void
_pixman_gradient_walker_fill_wide(pixman_gradient_walker_t *walker,
				  pixman_fixed_48_16_t      x,
				  uint32_t                 *buffer,
				  uint32_t                 *end);

/*
 * Edges
 */

#define MAX_ALPHA(n)    ((1 << (n)) - 1)
#define N_Y_FRAC(n)     ((n) == 1 ? 1 : (1 << ((n) / 2)) - 1)
#define N_X_FRAC(n)     ((n) == 1 ? 1 : (1 << ((n) / 2)) + 1)

#define STEP_Y_SMALL(n) (pixman_fixed_1 / N_Y_FRAC (n))
#define STEP_Y_BIG(n)   (pixman_fixed_1 - (N_Y_FRAC (n) - 1) * STEP_Y_SMALL (n))

#define Y_FRAC_FIRST(n) (STEP_Y_BIG (n) / 2)
#define Y_FRAC_LAST(n)  (Y_FRAC_FIRST (n) + (N_Y_FRAC (n) - 1) * STEP_Y_SMALL (n))

#define STEP_X_SMALL(n) (pixman_fixed_1 / N_X_FRAC (n))
#define STEP_X_BIG(n)   (pixman_fixed_1 - (N_X_FRAC (n) - 1) * STEP_X_SMALL (n))

#define X_FRAC_FIRST(n) (STEP_X_BIG (n) / 2)
#define X_FRAC_LAST(n)  (X_FRAC_FIRST (n) + (N_X_FRAC (n) - 1) * STEP_X_SMALL (n))

#define RENDER_SAMPLES_X(x, n)						\
    ((n) == 1? 0 : (pixman_fixed_frac (x) +				\
		    X_FRAC_FIRST (n)) / STEP_X_SMALL (n))

void
pixman_rasterize_edges_accessors (pixman_image_t *image,
                                  pixman_edge_t * l,
                                  pixman_edge_t * r,
                                  pixman_fixed_t  t,
                                  pixman_fixed_t  b);

/*
 * Implementations
 */
typedef struct pixman_implementation_t pixman_implementation_t;

typedef struct
{
    pixman_op_t              op;
    pixman_image_t *         src_image;
    pixman_image_t *         mask_image;
    pixman_image_t *         dest_image;
    int32_t                  src_x;
    int32_t                  src_y;
    int32_t                  mask_x;
    int32_t                  mask_y;
    int32_t                  dest_x;
    int32_t                  dest_y;
    int32_t                  width;
    int32_t                  height;

    uint32_t                 src_flags;
    uint32_t                 mask_flags;
    uint32_t                 dest_flags;
} pixman_composite_info_t;

#define PIXMAN_COMPOSITE_ARGS(info)					\
    MAYBE_UNUSED pixman_op_t        op = info->op;			\
    MAYBE_UNUSED pixman_image_t *   src_image = info->src_image;	\
    MAYBE_UNUSED pixman_image_t *   mask_image = info->mask_image;	\
    MAYBE_UNUSED pixman_image_t *   dest_image = info->dest_image;	\
    MAYBE_UNUSED int32_t            src_x = info->src_x;		\
    MAYBE_UNUSED int32_t            src_y = info->src_y;		\
    MAYBE_UNUSED int32_t            mask_x = info->mask_x;		\
    MAYBE_UNUSED int32_t            mask_y = info->mask_y;		\
    MAYBE_UNUSED int32_t            dest_x = info->dest_x;		\
    MAYBE_UNUSED int32_t            dest_y = info->dest_y;		\
    MAYBE_UNUSED int32_t            width = info->width;		\
    MAYBE_UNUSED int32_t            height = info->height

typedef void (*pixman_combine_32_func_t) (pixman_implementation_t *imp,
					  pixman_op_t              op,
					  uint32_t *               dest,
					  const uint32_t *         src,
					  const uint32_t *         mask,
					  int                      width);

typedef void (*pixman_combine_float_func_t) (pixman_implementation_t *imp,
					     pixman_op_t	      op,
					     float *		      dest,
					     const float *	      src,
					     const float *	      mask,
					     int		      n_pixels);

typedef void (*pixman_composite_func_t) (pixman_implementation_t *imp,
					 pixman_composite_info_t *info);
typedef pixman_bool_t (*pixman_blt_func_t) (pixman_implementation_t *imp,
					    uint32_t *               src_bits,
					    uint32_t *               dst_bits,
					    int                      src_stride,
					    int                      dst_stride,
					    int                      src_bpp,
					    int                      dst_bpp,
					    int                      src_x,
					    int                      src_y,
					    int                      dest_x,
					    int                      dest_y,
					    int                      width,
					    int                      height);
typedef pixman_bool_t (*pixman_fill_func_t) (pixman_implementation_t *imp,
					     uint32_t *               bits,
					     int                      stride,
					     int                      bpp,
					     int                      x,
					     int                      y,
					     int                      width,
					     int                      height,
					     uint32_t                 filler);

void _pixman_setup_combiner_functions_32 (pixman_implementation_t *imp);
void _pixman_setup_combiner_functions_float (pixman_implementation_t *imp);

typedef struct
{
    pixman_op_t             op;
    pixman_format_code_t    src_format;
    uint32_t		    src_flags;
    pixman_format_code_t    mask_format;
    uint32_t		    mask_flags;
    pixman_format_code_t    dest_format;
    uint32_t		    dest_flags;
    pixman_composite_func_t func;
} pixman_fast_path_t;

struct pixman_implementation_t
{
    pixman_implementation_t *	toplevel;
    pixman_implementation_t *	fallback;
    const pixman_fast_path_t *	fast_paths;
    const pixman_iter_info_t *  iter_info;

    pixman_blt_func_t		blt;
    pixman_fill_func_t		fill;

    pixman_combine_32_func_t	combine_32[PIXMAN_N_OPERATORS];
    pixman_combine_32_func_t	combine_32_ca[PIXMAN_N_OPERATORS];
    pixman_combine_float_func_t	combine_float[PIXMAN_N_OPERATORS];
    pixman_combine_float_func_t	combine_float_ca[PIXMAN_N_OPERATORS];
};

uint32_t
_pixman_image_get_solid (pixman_implementation_t *imp,
			 pixman_image_t *         image,
                         pixman_format_code_t     format);

pixman_implementation_t *
_pixman_implementation_create (pixman_implementation_t *fallback,
			       const pixman_fast_path_t *fast_paths);

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
					 pixman_composite_func_t  *out_func);

pixman_combine_32_func_t
_pixman_implementation_lookup_combiner (pixman_implementation_t *imp,
					pixman_op_t		 op,
					pixman_bool_t		 component_alpha,
					pixman_bool_t		 wide);

pixman_bool_t
_pixman_implementation_blt (pixman_implementation_t *imp,
                            uint32_t *               src_bits,
                            uint32_t *               dst_bits,
                            int                      src_stride,
                            int                      dst_stride,
                            int                      src_bpp,
                            int                      dst_bpp,
                            int                      src_x,
                            int                      src_y,
                            int                      dest_x,
                            int                      dest_y,
                            int                      width,
                            int                      height);

pixman_bool_t
_pixman_implementation_fill (pixman_implementation_t *imp,
                             uint32_t *               bits,
                             int                      stride,
                             int                      bpp,
                             int                      x,
                             int                      y,
                             int                      width,
                             int                      height,
                             uint32_t                 filler);

void
_pixman_implementation_iter_init (pixman_implementation_t       *imp,
                                  pixman_iter_t                 *iter,
                                  pixman_image_t                *image,
                                  int                            x,
                                  int                            y,
                                  int                            width,
                                  int                            height,
                                  uint8_t                       *buffer,
                                  iter_flags_t                   flags,
                                  uint32_t                       image_flags);

/* Specific implementations */
pixman_implementation_t *
_pixman_implementation_create_general (void);

pixman_implementation_t *
_pixman_implementation_create_fast_path (pixman_implementation_t *fallback);

pixman_implementation_t *
_pixman_implementation_create_noop (pixman_implementation_t *fallback);

#if defined USE_X86_MMX || defined USE_ARM_IWMMXT || defined USE_LOONGSON_MMI
pixman_implementation_t *
_pixman_implementation_create_mmx (pixman_implementation_t *fallback);
#endif

#ifdef USE_SSE2
pixman_implementation_t *
_pixman_implementation_create_sse2 (pixman_implementation_t *fallback);
#endif

#ifdef USE_SSSE3
pixman_implementation_t *
_pixman_implementation_create_ssse3 (pixman_implementation_t *fallback);
#endif

#ifdef USE_ARM_SIMD
pixman_implementation_t *
_pixman_implementation_create_arm_simd (pixman_implementation_t *fallback);
#endif

#ifdef USE_ARM_NEON
pixman_implementation_t *
_pixman_implementation_create_arm_neon (pixman_implementation_t *fallback);
#endif

#ifdef USE_ARM_A64_NEON
pixman_implementation_t *
_pixman_implementation_create_arm_neon (pixman_implementation_t *fallback);
#endif

#ifdef USE_MIPS_DSPR2
pixman_implementation_t *
_pixman_implementation_create_mips_dspr2 (pixman_implementation_t *fallback);
#endif

#ifdef USE_VMX
pixman_implementation_t *
_pixman_implementation_create_vmx (pixman_implementation_t *fallback);
#endif

pixman_bool_t
_pixman_implementation_disabled (const char *name);

pixman_implementation_t *
_pixman_x86_get_implementations (pixman_implementation_t *imp);

pixman_implementation_t *
_pixman_arm_get_implementations (pixman_implementation_t *imp);

pixman_implementation_t *
_pixman_ppc_get_implementations (pixman_implementation_t *imp);

pixman_implementation_t *
_pixman_mips_get_implementations (pixman_implementation_t *imp);

pixman_implementation_t *
_pixman_choose_implementation (void);

pixman_bool_t
_pixman_disabled (const char *name);


/*
 * Utilities
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
				    int32_t             height);
uint32_t *
_pixman_iter_get_scanline_noop (pixman_iter_t *iter, const uint32_t *mask);

void
_pixman_iter_init_bits_stride (pixman_iter_t *iter, const pixman_iter_info_t *info);

/* These "formats" all have depth 0, so they
 * will never clash with any real ones
 */
#define PIXMAN_null             PIXMAN_FORMAT (0, 0, 0, 0, 0, 0)
#define PIXMAN_solid            PIXMAN_FORMAT (0, 1, 0, 0, 0, 0)
#define PIXMAN_pixbuf		PIXMAN_FORMAT (0, 2, 0, 0, 0, 0)
#define PIXMAN_rpixbuf		PIXMAN_FORMAT (0, 3, 0, 0, 0, 0)
#define PIXMAN_unknown		PIXMAN_FORMAT (0, 4, 0, 0, 0, 0)
#define PIXMAN_any		PIXMAN_FORMAT (0, 5, 0, 0, 0, 0)

#define PIXMAN_OP_any		(PIXMAN_N_OPERATORS + 1)

#define FAST_PATH_ID_TRANSFORM			(1 <<  0)
#define FAST_PATH_NO_ALPHA_MAP			(1 <<  1)
#define FAST_PATH_NO_CONVOLUTION_FILTER		(1 <<  2)
#define FAST_PATH_NO_PAD_REPEAT			(1 <<  3)
#define FAST_PATH_NO_REFLECT_REPEAT		(1 <<  4)
#define FAST_PATH_NO_ACCESSORS			(1 <<  5)
#define FAST_PATH_NARROW_FORMAT			(1 <<  6)
#define FAST_PATH_COMPONENT_ALPHA		(1 <<  8)
#define FAST_PATH_SAMPLES_OPAQUE		(1 <<  7)
#define FAST_PATH_UNIFIED_ALPHA			(1 <<  9)
#define FAST_PATH_SCALE_TRANSFORM		(1 << 10)
#define FAST_PATH_NEAREST_FILTER		(1 << 11)
#define FAST_PATH_HAS_TRANSFORM			(1 << 12)
#define FAST_PATH_IS_OPAQUE			(1 << 13)
#define FAST_PATH_NO_NORMAL_REPEAT		(1 << 14)
#define FAST_PATH_NO_NONE_REPEAT		(1 << 15)
#define FAST_PATH_X_UNIT_POSITIVE		(1 << 16)
#define FAST_PATH_AFFINE_TRANSFORM		(1 << 17)
#define FAST_PATH_Y_UNIT_ZERO			(1 << 18)
#define FAST_PATH_BILINEAR_FILTER		(1 << 19)
#define FAST_PATH_ROTATE_90_TRANSFORM		(1 << 20)
#define FAST_PATH_ROTATE_180_TRANSFORM		(1 << 21)
#define FAST_PATH_ROTATE_270_TRANSFORM		(1 << 22)
#define FAST_PATH_SAMPLES_COVER_CLIP_NEAREST	(1 << 23)
#define FAST_PATH_SAMPLES_COVER_CLIP_BILINEAR	(1 << 24)
#define FAST_PATH_BITS_IMAGE			(1 << 25)
#define FAST_PATH_SEPARABLE_CONVOLUTION_FILTER  (1 << 26)

#define FAST_PATH_PAD_REPEAT						\
    (FAST_PATH_NO_NONE_REPEAT		|				\
     FAST_PATH_NO_NORMAL_REPEAT		|				\
     FAST_PATH_NO_REFLECT_REPEAT)

#define FAST_PATH_NORMAL_REPEAT						\
    (FAST_PATH_NO_NONE_REPEAT		|				\
     FAST_PATH_NO_PAD_REPEAT		|				\
     FAST_PATH_NO_REFLECT_REPEAT)

#define FAST_PATH_NONE_REPEAT						\
    (FAST_PATH_NO_NORMAL_REPEAT		|				\
     FAST_PATH_NO_PAD_REPEAT		|				\
     FAST_PATH_NO_REFLECT_REPEAT)

#define FAST_PATH_REFLECT_REPEAT					\
    (FAST_PATH_NO_NONE_REPEAT		|				\
     FAST_PATH_NO_NORMAL_REPEAT		|				\
     FAST_PATH_NO_PAD_REPEAT)

#define FAST_PATH_STANDARD_FLAGS					\
    (FAST_PATH_NO_CONVOLUTION_FILTER	|				\
     FAST_PATH_NO_ACCESSORS		|				\
     FAST_PATH_NO_ALPHA_MAP		|				\
     FAST_PATH_NARROW_FORMAT)

#define FAST_PATH_STD_DEST_FLAGS					\
    (FAST_PATH_NO_ACCESSORS		|				\
     FAST_PATH_NO_ALPHA_MAP		|				\
     FAST_PATH_NARROW_FORMAT)

#define SOURCE_FLAGS(format)						\
    (FAST_PATH_STANDARD_FLAGS |						\
     ((PIXMAN_ ## format == PIXMAN_solid) ?				\
      0 : (FAST_PATH_SAMPLES_COVER_CLIP_NEAREST | FAST_PATH_NEAREST_FILTER | FAST_PATH_ID_TRANSFORM)))

#define MASK_FLAGS(format, extra)					\
    ((PIXMAN_ ## format == PIXMAN_null) ? 0 : (SOURCE_FLAGS (format) | extra))

#define FAST_PATH(op, src, src_flags, mask, mask_flags, dest, dest_flags, func) \
    PIXMAN_OP_ ## op,							\
    PIXMAN_ ## src,							\
    src_flags,							        \
    PIXMAN_ ## mask,						        \
    mask_flags,							        \
    PIXMAN_ ## dest,	                                                \
    dest_flags,							        \
    func

#define PIXMAN_STD_FAST_PATH(op, src, mask, dest, func)			\
    { FAST_PATH (							\
	    op,								\
	    src,  SOURCE_FLAGS (src),					\
	    mask, MASK_FLAGS (mask, FAST_PATH_UNIFIED_ALPHA),		\
	    dest, FAST_PATH_STD_DEST_FLAGS,				\
	    func) }

#define PIXMAN_STD_FAST_PATH_CA(op, src, mask, dest, func)		\
    { FAST_PATH (							\
	    op,								\
	    src,  SOURCE_FLAGS (src),					\
	    mask, MASK_FLAGS (mask, FAST_PATH_COMPONENT_ALPHA),		\
	    dest, FAST_PATH_STD_DEST_FLAGS,				\
	    func) }

extern pixman_implementation_t *global_implementation;

static force_inline pixman_implementation_t *
get_implementation (void)
{
#ifndef TOOLCHAIN_SUPPORTS_ATTRIBUTE_CONSTRUCTOR
    if (!global_implementation)
	global_implementation = _pixman_choose_implementation ();
#endif
    return global_implementation;
}

/* This function is exported for the sake of the test suite and not part
 * of the ABI.
 */
PIXMAN_EXPORT pixman_implementation_t *
_pixman_internal_only_get_implementation (void);

/* Memory allocation helpers */
void *
pixman_malloc_ab (unsigned int n, unsigned int b);

void *
pixman_malloc_abc (unsigned int a, unsigned int b, unsigned int c);

void *
pixman_malloc_ab_plus_c (unsigned int a, unsigned int b, unsigned int c);

pixman_bool_t
_pixman_multiply_overflows_size (size_t a, size_t b);

pixman_bool_t
_pixman_multiply_overflows_int (unsigned int a, unsigned int b);

pixman_bool_t
_pixman_addition_overflows_int (unsigned int a, unsigned int b);

/* Compositing utilities */
void
pixman_expand_to_float (argb_t               *dst,
			const uint32_t       *src,
			pixman_format_code_t  format,
			int                   width);

void
pixman_contract_from_float (uint32_t     *dst,
			    const argb_t *src,
			    int           width);

/* Region Helpers */
pixman_bool_t
pixman_region32_copy_from_region16 (pixman_region32_t *dst,
                                    pixman_region16_t *src);

pixman_bool_t
pixman_region16_copy_from_region32 (pixman_region16_t *dst,
                                    pixman_region32_t *src);

/* Doubly linked lists */
typedef struct pixman_link_t pixman_link_t;
struct pixman_link_t
{
    pixman_link_t *next;
    pixman_link_t *prev;
};

typedef struct pixman_list_t pixman_list_t;
struct pixman_list_t
{
    pixman_link_t *head;
    pixman_link_t *tail;
};

static force_inline void
pixman_list_init (pixman_list_t *list)
{
    list->head = (pixman_link_t *)list;
    list->tail = (pixman_link_t *)list;
}

static force_inline void
pixman_list_prepend (pixman_list_t *list, pixman_link_t *link)
{
    link->next = list->head;
    link->prev = (pixman_link_t *)list;
    list->head->prev = link;
    list->head = link;
}

static force_inline void
pixman_list_unlink (pixman_link_t *link)
{
    link->prev->next = link->next;
    link->next->prev = link->prev;
}

static force_inline void
pixman_list_move_to_front (pixman_list_t *list, pixman_link_t *link)
{
    pixman_list_unlink (link);
    pixman_list_prepend (list, link);
}

/* Misc macros */

#ifndef FALSE
#   define FALSE 0
#endif

#ifndef TRUE
#   define TRUE 1
#endif

#ifndef MIN
#  define MIN(a, b) ((a < b) ? a : b)
#endif

#ifndef MAX
#  define MAX(a, b) ((a > b) ? a : b)
#endif

/* Integer division that rounds towards -infinity */
#define DIV(a, b)					   \
    ((((a) < 0) == ((b) < 0)) ? (a) / (b) :                \
     ((a) - (b) + 1 - (((b) < 0) << 1)) / (b))

/* Modulus that produces the remainder wrt. DIV */
#define MOD(a, b) ((a) < 0 ? ((b) - ((-(a) - 1) % (b))) - 1 : (a) % (b))

#define CLIP(v, low, high) ((v) < (low) ? (low) : ((v) > (high) ? (high) : (v)))

#define FLOAT_IS_ZERO(f)     (-FLT_MIN < (f) && (f) < FLT_MIN)

/* Conversion between 8888 and 0565 */

static force_inline uint16_t
convert_8888_to_0565 (uint32_t s)
{
    /* The following code can be compiled into just 4 instructions on ARM */
    uint32_t a, b;
    a = (s >> 3) & 0x1F001F;
    b = s & 0xFC00;
    a |= a >> 5;
    a |= b >> 5;
    return (uint16_t)a;
}

static force_inline uint32_t
convert_0565_to_0888 (uint16_t s)
{
    return (((((s) << 3) & 0xf8) | (((s) >> 2) & 0x7)) |
            ((((s) << 5) & 0xfc00) | (((s) >> 1) & 0x300)) |
            ((((s) << 8) & 0xf80000) | (((s) << 3) & 0x70000)));
}

static force_inline uint32_t
convert_0565_to_8888 (uint16_t s)
{
    return convert_0565_to_0888 (s) | 0xff000000;
}

/* Trivial versions that are useful in macros */

static force_inline uint32_t
convert_8888_to_8888 (uint32_t s)
{
    return s;
}

static force_inline uint32_t
convert_x888_to_8888 (uint32_t s)
{
    return s | 0xff000000;
}

static force_inline uint16_t
convert_0565_to_0565 (uint16_t s)
{
    return s;
}

#define PIXMAN_FORMAT_IS_WIDE(f)					\
    (PIXMAN_FORMAT_A (f) > 8 ||						\
     PIXMAN_FORMAT_R (f) > 8 ||						\
     PIXMAN_FORMAT_G (f) > 8 ||						\
     PIXMAN_FORMAT_B (f) > 8 ||						\
     PIXMAN_FORMAT_TYPE (f) == PIXMAN_TYPE_ARGB_SRGB)

#ifdef WORDS_BIGENDIAN
#   define SCREEN_SHIFT_LEFT(x,n)	((x) << (n))
#   define SCREEN_SHIFT_RIGHT(x,n)	((x) >> (n))
#else
#   define SCREEN_SHIFT_LEFT(x,n)	((x) >> (n))
#   define SCREEN_SHIFT_RIGHT(x,n)	((x) << (n))
#endif

static force_inline uint32_t
unorm_to_unorm (uint32_t val, int from_bits, int to_bits)
{
    uint32_t result;

    if (from_bits == 0)
	return 0;

    /* Delete any extra bits */
    val &= ((1 << from_bits) - 1);

    if (from_bits >= to_bits)
	return val >> (from_bits - to_bits);

    /* Start out with the high bit of val in the high bit of result. */
    result = val << (to_bits - from_bits);

    /* Copy the bits in result, doubling the number of bits each time, until
     * we fill all to_bits. Unrolled manually because from_bits and to_bits
     * are usually known statically, so the compiler can turn all of this
     * into a few shifts.
     */
#define REPLICATE()							\
    do									\
    {									\
	if (from_bits < to_bits)					\
	{								\
	    result |= result >> from_bits;				\
									\
	    from_bits *= 2;						\
	}								\
    }									\
    while (0)

    REPLICATE();
    REPLICATE();
    REPLICATE();
    REPLICATE();
    REPLICATE();

    return result;
}

uint16_t pixman_float_to_unorm (float f, int n_bits);
float pixman_unorm_to_float (uint16_t u, int n_bits);

/*
 * Various debugging code
 */

#undef DEBUG

#define COMPILE_TIME_ASSERT(x)						\
    do { typedef int compile_time_assertion [(x)?1:-1]; } while (0)

/* Turn on debugging depending on what type of release this is
 */
#if (((PIXMAN_VERSION_MICRO % 2) == 0) && ((PIXMAN_VERSION_MINOR % 2) == 1))

/* Debugging gets turned on for development releases because these
 * are the things that end up in bleeding edge distributions such
 * as Rawhide etc.
 *
 * For performance reasons we don't turn it on for stable releases or
 * random git checkouts. (Random git checkouts are often used for
 * performance work).
 */

#    define DEBUG

#endif

void
_pixman_log_error (const char *function, const char *message);

#define return_if_fail(expr)                                            \
    do                                                                  \
    {                                                                   \
	if (unlikely (!(expr)))                                         \
	{								\
	    _pixman_log_error (FUNC, "The expression " # expr " was false"); \
	    return;							\
	}								\
    }                                                                   \
    while (0)

#define return_val_if_fail(expr, retval)                                \
    do                                                                  \
    {                                                                   \
	if (unlikely (!(expr)))                                         \
	{								\
	    _pixman_log_error (FUNC, "The expression " # expr " was false"); \
	    return (retval);						\
	}								\
    }                                                                   \
    while (0)

#define critical_if_fail(expr)						\
    do									\
    {									\
	if (unlikely (!(expr)))                                         \
	    _pixman_log_error (FUNC, "The expression " # expr " was false"); \
    }									\
    while (0)

/*
 * Matrix
 */

typedef struct { pixman_fixed_48_16_t v[3]; } pixman_vector_48_16_t;

PIXMAN_EXPORT
pixman_bool_t
pixman_transform_point_31_16 (const pixman_transform_t    *t,
                              const pixman_vector_48_16_t *v,
                              pixman_vector_48_16_t       *result);

PIXMAN_EXPORT
void
pixman_transform_point_31_16_3d (const pixman_transform_t    *t,
                                 const pixman_vector_48_16_t *v,
                                 pixman_vector_48_16_t       *result);

PIXMAN_EXPORT
void
pixman_transform_point_31_16_affine (const pixman_transform_t    *t,
                                     const pixman_vector_48_16_t *v,
                                     pixman_vector_48_16_t       *result);

/*
 * Timers
 */

#ifdef PIXMAN_TIMERS

static inline uint64_t
oil_profile_stamp_rdtsc (void)
{
    uint32_t hi, lo;

    __asm__ __volatile__ ("rdtsc\n" : "=a" (lo), "=d" (hi));

    return lo | (((uint64_t)hi) << 32);
}

#define OIL_STAMP oil_profile_stamp_rdtsc

typedef struct pixman_timer_t pixman_timer_t;

struct pixman_timer_t
{
    int             initialized;
    const char *    name;
    uint64_t        n_times;
    uint64_t        total;
    pixman_timer_t *next;
};

extern int timer_defined;

void pixman_timer_register (pixman_timer_t *timer);

#define TIMER_BEGIN(tname)                                              \
    {                                                                   \
	static pixman_timer_t timer ## tname;                           \
	uint64_t              begin ## tname;                           \
        								\
	if (!timer ## tname.initialized)				\
	{                                                               \
	    timer ## tname.initialized = 1;				\
	    timer ## tname.name = # tname;				\
	    pixman_timer_register (&timer ## tname);			\
	}                                                               \
									\
	timer ## tname.n_times++;					\
	begin ## tname = OIL_STAMP ();

#define TIMER_END(tname)                                                \
    timer ## tname.total += OIL_STAMP () - begin ## tname;		\
    }

#else

#define TIMER_BEGIN(tname)
#define TIMER_END(tname)

#endif /* PIXMAN_TIMERS */

#endif /* __ASSEMBLER__ */

#endif /* PIXMAN_PRIVATE_H */
