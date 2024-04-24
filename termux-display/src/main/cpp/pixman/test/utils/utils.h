#ifdef HAVE_CONFIG_H
#include <pixman-config.h>
#endif

#include <assert.h>
#include "pixman-private.h" /* For 'inline' definition */
#include "utils-prng.h"

#if defined(_MSC_VER)
#define snprintf _snprintf
#define strcasecmp _stricmp
#endif

#define ARRAY_LENGTH(A) ((int) (sizeof (A) / sizeof ((A) [0])))

/* A primitive pseudorandom number generator,
 * taken from POSIX.1-2001 example
 */

extern prng_t prng_state_data;
extern prng_t *prng_state;
#ifdef USE_OPENMP
#pragma omp threadprivate(prng_state_data)
#pragma omp threadprivate(prng_state)
#endif

static inline uint32_t
prng_rand (void)
{
    return prng_rand_r (prng_state);
}

static inline void
prng_srand (uint32_t seed)
{
    if (!prng_state)
    {
        /* Without setting a seed, PRNG does not work properly (is just
         * returning zeros). So we only initialize the pointer here to
         * make sure that 'prng_srand' is always called before any
         * other 'prng_*' function. The wrongdoers violating this order
         * will get a segfault. */
        prng_state = &prng_state_data;
    }
    prng_srand_r (prng_state, seed);
}

static inline uint32_t
prng_rand_n (int max)
{
    return prng_rand () % max;
}

static inline void
prng_randmemset (void *buffer, size_t size, prng_randmemset_flags_t flags)
{
    prng_randmemset_r (prng_state, buffer, size, flags);
}

/* CRC 32 computation
 */
uint32_t
compute_crc32 (uint32_t    in_crc32,
	       const void *buf,
	       size_t      buf_len);

uint32_t
compute_crc32_for_image (uint32_t        in_crc32,
			 pixman_image_t *image);

/* Print the image in hexadecimal */
void
print_image (pixman_image_t *image);

/* Returns TRUE if running on a little endian system
 */
static force_inline pixman_bool_t
is_little_endian (void)
{
    unsigned long endian_check_var = 1;
    return *(unsigned char *)&endian_check_var == 1;
}

/* perform endian conversion of pixel data
 */
void
image_endian_swap (pixman_image_t *img);

#if defined (HAVE_MPROTECT) && defined (HAVE_GETPAGESIZE) && \
    defined (HAVE_SYS_MMAN_H) && defined (HAVE_MMAP)
/* fence_malloc and friends have working fence implementation.
 * Without this, fence_malloc still allocs but does not catch
 * out-of-bounds accesses.
 */
#define FENCE_MALLOC_ACTIVE 1
#else
#define FENCE_MALLOC_ACTIVE 0
#endif

/* Allocate memory that is bounded by protected pages,
 * so that out-of-bounds access will cause segfaults
 */
void *
fence_malloc (int64_t len);

void
fence_free (void *data);

pixman_image_t *
fence_image_create_bits (pixman_format_code_t format,
                         int min_width,
                         int height,
                         pixman_bool_t stride_fence);

/* Return the page size if FENCE_MALLOC_ACTIVE, or zero otherwise */
unsigned long
fence_get_page_size ();

/* Generate n_bytes random bytes in fence_malloced memory */
uint8_t *
make_random_bytes (int n_bytes);
float *
make_random_floats (int n_bytes);

/* Return current time in seconds */
double
gettime (void);

uint32_t
get_random_seed (void);

/* main body of the fuzzer test */
int
fuzzer_test_main (const char *test_name,
		  int         default_number_of_iterations,
		  uint32_t    expected_checksum,
		  uint32_t    (*test_function)(int testnum, int verbose),
		  int         argc,
		  const char *argv[]);

void
fail_after (int seconds, const char *msg);

/* If possible, enable traps for floating point exceptions */
void enable_divbyzero_exceptions(void);
void enable_invalid_exceptions(void);

/* Converts a8r8g8b8 pixels to pixels that
 *  - are not premultiplied,
 *  - are stored in this order in memory: R, G, B, A, regardless of
 *    the endianness of the computer.
 * It is allowed for @src and @dst to point to the same memory buffer.
 */
void
a8r8g8b8_to_rgba_np (uint32_t *dst, uint32_t *src, int n_pixels);

pixman_bool_t
write_png (pixman_image_t *image, const char *filename);

void
draw_checkerboard (pixman_image_t *image,
		   int check_size,
		   uint32_t color1, uint32_t color2);

/* A pair of macros which can help to detect corruption of
 * floating point registers after a function call. This may
 * happen if _mm_empty() call is forgotten in MMX/SSE2 fast
 * path code, or ARM NEON assembly optimized function forgets
 * to save/restore d8-d15 registers before use.
 */

#define FLOAT_REGS_CORRUPTION_DETECTOR_START()                 \
    static volatile double frcd_volatile_constant1 = 123451;   \
    static volatile double frcd_volatile_constant2 = 123452;   \
    static volatile double frcd_volatile_constant3 = 123453;   \
    static volatile double frcd_volatile_constant4 = 123454;   \
    static volatile double frcd_volatile_constant5 = 123455;   \
    static volatile double frcd_volatile_constant6 = 123456;   \
    static volatile double frcd_volatile_constant7 = 123457;   \
    static volatile double frcd_volatile_constant8 = 123458;   \
    double frcd_canary_variable1 = frcd_volatile_constant1;    \
    double frcd_canary_variable2 = frcd_volatile_constant2;    \
    double frcd_canary_variable3 = frcd_volatile_constant3;    \
    double frcd_canary_variable4 = frcd_volatile_constant4;    \
    double frcd_canary_variable5 = frcd_volatile_constant5;    \
    double frcd_canary_variable6 = frcd_volatile_constant6;    \
    double frcd_canary_variable7 = frcd_volatile_constant7;    \
    double frcd_canary_variable8 = frcd_volatile_constant8;

#define FLOAT_REGS_CORRUPTION_DETECTOR_FINISH()                \
    assert (frcd_canary_variable1 == frcd_volatile_constant1); \
    assert (frcd_canary_variable2 == frcd_volatile_constant2); \
    assert (frcd_canary_variable3 == frcd_volatile_constant3); \
    assert (frcd_canary_variable4 == frcd_volatile_constant4); \
    assert (frcd_canary_variable5 == frcd_volatile_constant5); \
    assert (frcd_canary_variable6 == frcd_volatile_constant6); \
    assert (frcd_canary_variable7 == frcd_volatile_constant7); \
    assert (frcd_canary_variable8 == frcd_volatile_constant8);

/* Try to get an aligned memory chunk */
void *
aligned_malloc (size_t align, size_t size);

double
convert_srgb_to_linear (double component);

double
convert_linear_to_srgb (double component);

void
initialize_palette (pixman_indexed_t *palette, uint32_t depth, int is_rgb);

pixman_format_code_t
format_from_string (const char *s);

void
list_formats (void);

void
list_operators (void);

void list_dithers (void);

pixman_op_t
operator_from_string (const char *s);

pixman_dither_t
dither_from_string (const char *s);

const char *
operator_name (pixman_op_t op);

const char *
format_name (pixman_format_code_t format);

const char *
dither_name (pixman_dither_t dither);

typedef struct
{
    double r, g, b, a;
} color_t;

void
do_composite (pixman_op_t op,
	      const color_t *src,
	      const color_t *mask,
	      const color_t *dst,
	      color_t *result,
	      pixman_bool_t component_alpha);

void
round_color (pixman_format_code_t format, color_t *color);

typedef struct
{
    pixman_format_code_t format;
    uint32_t am, rm, gm, bm;
    uint32_t as, rs, gs, bs;
    uint32_t aw, rw, gw, bw;
    float ad, rd, gd, bd;
} pixel_checker_t;

void
pixel_checker_init (pixel_checker_t *checker, pixman_format_code_t format);

void
pixel_checker_allow_dither (pixel_checker_t *checker);

void
pixel_checker_split_pixel (const pixel_checker_t *checker, uint32_t pixel,
			   int *a, int *r, int *g, int *b);

void
pixel_checker_get_max (const pixel_checker_t *checker, color_t *color,
		       int *a, int *r, int *g, int *b);

void
pixel_checker_get_min (const pixel_checker_t *checker, color_t *color,
		       int *a, int *r, int *g, int *b);

pixman_bool_t
pixel_checker_check (const pixel_checker_t *checker,
		     uint32_t pixel, color_t *color);

void
pixel_checker_convert_pixel_to_color (const pixel_checker_t *checker,
                                      uint32_t pixel, color_t *color);

void
pixel_checker_get_masks (const pixel_checker_t *checker,
                         uint32_t              *am,
                         uint32_t              *rm,
                         uint32_t              *gm,
                         uint32_t              *bm);
