#define _GNU_SOURCE

#include "utils.h"
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <float.h>
#include <ctype.h>
#include <limits.h>

#ifdef HAVE_GETTIMEOFDAY
#include <sys/time.h>
#else
#include <time.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_FENV_H
#include <fenv.h>
#endif

#ifdef HAVE_LIBPNG
#include <png.h>
#endif

#define ROUND_UP(x, mult) (((x) + (mult) - 1) / (mult) * (mult))

/* Random number generator state
 */

prng_t prng_state_data = {0};
prng_t *prng_state = NULL;

/*----------------------------------------------------------------------------*\
 *  CRC-32 version 2.0.0 by Craig Bruce, 2006-04-29.
 *
 *  This program generates the CRC-32 values for the files named in the
 *  command-line arguments.  These are the same CRC-32 values used by GZIP,
 *  PKZIP, and ZMODEM.  The Crc32_ComputeBuf () can also be detached and
 *  used independently.
 *
 *  THIS PROGRAM IS PUBLIC-DOMAIN SOFTWARE.
 *
 *  Based on the byte-oriented implementation "File Verification Using CRC"
 *  by Mark R. Nelson in Dr. Dobb's Journal, May 1992, pp. 64-67.
 *
 *  v1.0.0: original release.
 *  v1.0.1: fixed printf formats.
 *  v1.0.2: fixed something else.
 *  v1.0.3: replaced CRC constant table by generator function.
 *  v1.0.4: reformatted code, made ANSI C.  1994-12-05.
 *  v2.0.0: rewrote to use memory buffer & static table, 2006-04-29.
\*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*\
 *  NAME:
 *     Crc32_ComputeBuf () - computes the CRC-32 value of a memory buffer
 *  DESCRIPTION:
 *     Computes or accumulates the CRC-32 value for a memory buffer.
 *     The 'inCrc32' gives a previously accumulated CRC-32 value to allow
 *     a CRC to be generated for multiple sequential buffer-fuls of data.
 *     The 'inCrc32' for the first buffer must be zero.
 *  ARGUMENTS:
 *     inCrc32 - accumulated CRC-32 value, must be 0 on first call
 *     buf     - buffer to compute CRC-32 value for
 *     bufLen  - number of bytes in buffer
 *  RETURNS:
 *     crc32 - computed CRC-32 value
 *  ERRORS:
 *     (no errors are possible)
\*----------------------------------------------------------------------------*/

uint32_t
compute_crc32 (uint32_t    in_crc32,
	       const void *buf,
	       size_t      buf_len)
{
    static const uint32_t crc_table[256] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
	0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD,	0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
	0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,	0x14015C4F, 0x63066CD9,
	0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
	0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
	0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
	0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
	0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
	0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
	0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
	0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
	0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
	0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
	0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
	0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
	0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
	0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
	0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
	0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
	0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
	0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
	0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
    };

    uint32_t              crc32;
    unsigned char *       byte_buf;
    size_t                i;

    /* accumulate crc32 for buffer */
    crc32 = in_crc32 ^ 0xFFFFFFFF;
    byte_buf = (unsigned char*) buf;

    for (i = 0; i < buf_len; i++)
	crc32 = (crc32 >> 8) ^ crc_table[(crc32 ^ byte_buf[i]) & 0xFF];

    return (crc32 ^ 0xFFFFFFFF);
}

static uint32_t
compute_crc32_for_image_internal (uint32_t        crc32,
				  pixman_image_t *img,
				  pixman_bool_t	  remove_alpha,
				  pixman_bool_t	  remove_rgb)
{
    pixman_format_code_t fmt = pixman_image_get_format (img);
    uint32_t *data = pixman_image_get_data (img);
    int stride = pixman_image_get_stride (img);
    int height = pixman_image_get_height (img);
    uint32_t mask = 0xffffffff;
    int i;

    if (stride < 0)
    {
	data += (stride / 4) * (height - 1);
	stride = - stride;
    }

    /* mask unused 'x' part */
    if (PIXMAN_FORMAT_BPP (fmt) - PIXMAN_FORMAT_DEPTH (fmt) &&
	PIXMAN_FORMAT_DEPTH (fmt) != 0)
    {
	uint32_t m = (1 << PIXMAN_FORMAT_DEPTH (fmt)) - 1;

	if (PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_BGRA ||
	    PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_RGBA)
	{
	    m <<= (PIXMAN_FORMAT_BPP (fmt) - PIXMAN_FORMAT_DEPTH (fmt));
	}

	mask &= m;
    }

    /* mask alpha channel */
    if (remove_alpha && PIXMAN_FORMAT_A (fmt))
    {
	uint32_t m;

	if (PIXMAN_FORMAT_BPP (fmt) == 32)
	    m = 0xffffffff;
	else
	    m = (1 << PIXMAN_FORMAT_BPP (fmt)) - 1;

	m >>= PIXMAN_FORMAT_A (fmt);

	if (PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_BGRA ||
	    PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_RGBA ||
	    PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_A)
	{
	    /* Alpha is at the bottom of the pixel */
	    m <<= PIXMAN_FORMAT_A (fmt);
	}

	mask &= m;
    }

    /* mask rgb channels */
    if (remove_rgb && PIXMAN_FORMAT_RGB (fmt))
    {
	uint32_t m = ((uint32_t)~0) >> (32 - PIXMAN_FORMAT_BPP (fmt));
	uint32_t size = PIXMAN_FORMAT_R (fmt) + PIXMAN_FORMAT_G (fmt) + PIXMAN_FORMAT_B (fmt);

	m &= ~((1 << size) - 1);

	if (PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_BGRA ||
	    PIXMAN_FORMAT_TYPE (fmt) == PIXMAN_TYPE_RGBA)
	{
	    /* RGB channels are at the top of the pixel */
	    m >>= size;
	}

	mask &= m;
    }

    for (i = 0; i * PIXMAN_FORMAT_BPP (fmt) < 32; i++)
	mask |= mask << (i * PIXMAN_FORMAT_BPP (fmt));

    for (i = 0; i < stride * height / 4; i++)
	data[i] &= mask;

    /* swap endiannes in order to provide identical results on both big
     * and litte endian systems
     */
    image_endian_swap (img);

    return compute_crc32 (crc32, data, stride * height);
}

uint32_t
compute_crc32_for_image (uint32_t        crc32,
			 pixman_image_t *img)
{
    if (img->common.alpha_map)
    {
	crc32 = compute_crc32_for_image_internal (crc32, img, TRUE, FALSE);
	crc32 = compute_crc32_for_image_internal (
	    crc32, (pixman_image_t *)img->common.alpha_map, FALSE, TRUE);
    }
    else
    {
	crc32 = compute_crc32_for_image_internal (crc32, img, FALSE, FALSE);
    }

    return crc32;
}

void
print_image (pixman_image_t *image)
{
    int i, j;
    int width, height, stride;
    pixman_format_code_t format;
    uint8_t *buffer;
    int s;

    width = pixman_image_get_width (image);
    height = pixman_image_get_height (image);
    stride = pixman_image_get_stride (image);
    format = pixman_image_get_format (image);
    buffer = (uint8_t *)pixman_image_get_data (image);

    s = (stride >= 0)? stride : - stride;
    
    printf ("---\n");
    for (i = 0; i < height; i++)
    {
	for (j = 0; j < s; j++)
	{
	    if (j == (width * PIXMAN_FORMAT_BPP (format) + 7) / 8)
		printf ("| ");

	    printf ("%02X ", *((uint8_t *)buffer + i * stride + j));
	}
	printf ("\n");
    }
    printf ("---\n");
}

/* perform endian conversion of pixel data
 */
void
image_endian_swap (pixman_image_t *img)
{
    int stride = pixman_image_get_stride (img);
    uint32_t *data = pixman_image_get_data (img);
    int height = pixman_image_get_height (img);
    int bpp = PIXMAN_FORMAT_BPP (pixman_image_get_format (img));
    int i, j;

    /* swap bytes only on big endian systems */
    if (is_little_endian())
	return;

    if (bpp == 8)
	return;

    for (i = 0; i < height; i++)
    {
	uint8_t *line_data = (uint8_t *)data + stride * i;
	int s = (stride >= 0)? stride : - stride;
    	
	switch (bpp)
	{
	case 1:
	    for (j = 0; j < s; j++)
	    {
		line_data[j] =
		    ((line_data[j] & 0x80) >> 7) |
		    ((line_data[j] & 0x40) >> 5) |
		    ((line_data[j] & 0x20) >> 3) |
		    ((line_data[j] & 0x10) >> 1) |
		    ((line_data[j] & 0x08) << 1) |
		    ((line_data[j] & 0x04) << 3) |
		    ((line_data[j] & 0x02) << 5) |
		    ((line_data[j] & 0x01) << 7);
	    }
	    break;
	case 4:
	    for (j = 0; j < s; j++)
	    {
		line_data[j] = (line_data[j] >> 4) | (line_data[j] << 4);
	    }
	    break;
	case 16:
	    for (j = 0; j + 2 <= s; j += 2)
	    {
		char t1 = line_data[j + 0];
		char t2 = line_data[j + 1];

		line_data[j + 1] = t1;
		line_data[j + 0] = t2;
	    }
	    break;
	case 24:
	    for (j = 0; j + 3 <= s; j += 3)
	    {
		char t1 = line_data[j + 0];
		char t2 = line_data[j + 1];
		char t3 = line_data[j + 2];

		line_data[j + 2] = t1;
		line_data[j + 1] = t2;
		line_data[j + 0] = t3;
	    }
	    break;
	case 32:
	    for (j = 0; j + 4 <= s; j += 4)
	    {
		char t1 = line_data[j + 0];
		char t2 = line_data[j + 1];
		char t3 = line_data[j + 2];
		char t4 = line_data[j + 3];

		line_data[j + 3] = t1;
		line_data[j + 2] = t2;
		line_data[j + 1] = t3;
		line_data[j + 0] = t4;
	    }
	    break;
	default:
	    assert (FALSE);
	    break;
	}
    }
}

#define N_LEADING_PROTECTED	10
#define N_TRAILING_PROTECTED	10

typedef struct
{
    void *addr;
    uint32_t len;
    uint8_t *trailing;
    int n_bytes;
} info_t;

#if FENCE_MALLOC_ACTIVE

unsigned long
fence_get_page_size ()
{
    /* You can fake a page size here, if you want to test e.g. 64 kB
     * pages on a 4 kB page system. Just put a multiplier below.
     */
    return getpagesize ();
}

/* This is apparently necessary on at least OS X */
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

void *
fence_malloc (int64_t len)
{
    unsigned long page_size = fence_get_page_size ();
    unsigned long page_mask = page_size - 1;
    uint32_t n_payload_bytes = (len + page_mask) & ~page_mask;
    uint32_t n_bytes =
	(page_size * (N_LEADING_PROTECTED + N_TRAILING_PROTECTED + 2) +
	 n_payload_bytes) & ~page_mask;
    uint8_t *initial_page;
    uint8_t *leading_protected;
    uint8_t *trailing_protected;
    uint8_t *payload;
    uint8_t *addr;

    if (len < 0)
	abort();
    
    addr = mmap (NULL, n_bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
		 -1, 0);

    if (addr == MAP_FAILED)
    {
	printf ("mmap failed on %lld %u\n", (long long int)len, n_bytes);
	return NULL;
    }

    initial_page = (uint8_t *)(((uintptr_t)addr + page_mask) & ~page_mask);
    leading_protected = initial_page + page_size;
    payload = leading_protected + N_LEADING_PROTECTED * page_size;
    trailing_protected = payload + n_payload_bytes;

    ((info_t *)initial_page)->addr = addr;
    ((info_t *)initial_page)->len = len;
    ((info_t *)initial_page)->trailing = trailing_protected;
    ((info_t *)initial_page)->n_bytes = n_bytes;

    if ((mprotect (leading_protected, N_LEADING_PROTECTED * page_size,
		  PROT_NONE) == -1) ||
	(mprotect (trailing_protected, N_TRAILING_PROTECTED * page_size,
		  PROT_NONE) == -1))
    {
	munmap (addr, n_bytes);
	return NULL;
    }

    return payload;
}

void
fence_free (void *data)
{
    uint32_t page_size = fence_get_page_size ();
    uint8_t *payload = data;
    uint8_t *leading_protected = payload - N_LEADING_PROTECTED * page_size;
    uint8_t *initial_page = leading_protected - page_size;
    info_t *info = (info_t *)initial_page;

    munmap (info->addr, info->n_bytes);
}

static void
fence_image_destroy (pixman_image_t *image, void *data)
{
    fence_free (data);
}

/* Create an image with fence pages.
 *
 * Creates an image, where the data area is allocated with fence_malloc ().
 * Each row has an additional page in the stride.
 *
 * min_width is only a minimum width for the image. The width is aligned up
 * for the row size to be divisible by both page size and pixel size.
 *
 * If stride_fence is true, the additional page on each row will be
 * armed to cause SIGSEGV or SIGBUS on all accesses. This should catch
 * all accesses outside the valid row pixels.
 */
pixman_image_t *
fence_image_create_bits (pixman_format_code_t format,
                         int min_width,
                         int height,
                         pixman_bool_t stride_fence)
{
    unsigned page_size = fence_get_page_size ();
    unsigned page_mask = page_size - 1;
    unsigned bitspp = PIXMAN_FORMAT_BPP (format);
    unsigned bits_boundary;
    unsigned row_bits;
    int width;       /* pixels */
    unsigned stride; /* bytes */
    void *pixels;
    pixman_image_t *image;
    int i;

    /* must be power of two */
    assert (page_size && (page_size & page_mask) == 0);

    if (bitspp < 1 || min_width < 1 || height < 1)
        abort ();

    /* least common multiple between page size * 8 and bitspp */
    bits_boundary = bitspp;
    while (! (bits_boundary & 1))
        bits_boundary >>= 1;
    bits_boundary *= page_size * 8;

    /* round up to bits_boundary */
    row_bits = ROUND_UP ( (unsigned)min_width * bitspp, bits_boundary);
    width = row_bits / bitspp;

    stride = row_bits / 8;
    if (stride_fence)
        stride += page_size; /* add fence page */

    if (UINT_MAX / stride < (unsigned)height)
        abort ();

    pixels = fence_malloc (stride * (unsigned)height);
    if (!pixels)
        return NULL;

    if (stride_fence)
    {
        uint8_t *guard = (uint8_t *)pixels + stride - page_size;

        /* arm row end fence pages */
        for (i = 0; i < height; i++)
        {
            if (mprotect (guard + i * stride, page_size, PROT_NONE) == -1)
                goto out_fail;
        }
    }

    assert (width >= min_width);

    image = pixman_image_create_bits_no_clear (format, width, height,
                                               pixels, stride);
    if (!image)
        goto out_fail;

    pixman_image_set_destroy_function (image, fence_image_destroy, pixels);

    return image;

out_fail:
    fence_free (pixels);

    return NULL;
}

#else /* FENCE_MALLOC_ACTIVE */

void *
fence_malloc (int64_t len)
{
    return malloc (len);
}

void
fence_free (void *data)
{
    free (data);
}

pixman_image_t *
fence_image_create_bits (pixman_format_code_t format,
                         int min_width,
                         int height,
                         pixman_bool_t stride_fence)
{
    return pixman_image_create_bits (format, min_width, height, NULL, 0);
    /* Implicitly allocated storage does not need a destroy function
     * to get freed on refcount hitting zero.
     */
}

unsigned long
fence_get_page_size ()
{
    return 0;
}

#endif /* FENCE_MALLOC_ACTIVE */

uint8_t *
make_random_bytes (int n_bytes)
{
    uint8_t *bytes = fence_malloc (n_bytes);

    if (!bytes)
	return NULL;

    prng_randmemset (bytes, n_bytes, 0);

    return bytes;
}

float *
make_random_floats (int n_bytes)
{
    uint8_t *bytes = fence_malloc (n_bytes);
    float *vals = (float *)bytes;

    if (!bytes)
	return 0;

    for (n_bytes /= 4; n_bytes; vals++, n_bytes--)
	*vals = (float)rand() / (float)RAND_MAX;

    return (float *)bytes;
}

void
a8r8g8b8_to_rgba_np (uint32_t *dst, uint32_t *src, int n_pixels)
{
    uint8_t *dst8 = (uint8_t *)dst;
    int i;

    for (i = 0; i < n_pixels; ++i)
    {
	uint32_t p = src[i];
	uint8_t a, r, g, b;

	a = (p & 0xff000000) >> 24;
	r = (p & 0x00ff0000) >> 16;
	g = (p & 0x0000ff00) >> 8;
	b = (p & 0x000000ff) >> 0;

	if (a != 0)
	{
#define DIVIDE(c, a)							\
	    do								\
	    {								\
		int t = ((c) * 255) / a;				\
		(c) = t < 0? 0 : t > 255? 255 : t;			\
	    } while (0)

	    DIVIDE (r, a);
	    DIVIDE (g, a);
	    DIVIDE (b, a);
	}

	*dst8++ = r;
	*dst8++ = g;
	*dst8++ = b;
	*dst8++ = a;
    }
}

#ifdef HAVE_LIBPNG

pixman_bool_t
write_png (pixman_image_t *image, const char *filename)
{
    int width = pixman_image_get_width (image);
    int height = pixman_image_get_height (image);
    int stride = width * 4;
    uint32_t *data = malloc (height * stride);
    pixman_image_t *copy;
    png_struct *write_struct;
    png_info *info_struct;
    pixman_bool_t result = FALSE;
    FILE *f = fopen (filename, "wb");
    png_bytep *row_pointers;
    int i;

    if (!f)
	return FALSE;

    row_pointers = malloc (height * sizeof (png_bytep));

    copy = pixman_image_create_bits (
	PIXMAN_a8r8g8b8, width, height, data, stride);

    pixman_image_composite32 (
	PIXMAN_OP_SRC, image, NULL, copy, 0, 0, 0, 0, 0, 0, width, height);

    a8r8g8b8_to_rgba_np (data, data, height * width);

    for (i = 0; i < height; ++i)
	row_pointers[i] = (png_bytep)(data + i * width);

    if (!(write_struct = png_create_write_struct (
	      PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
	goto out1;

    if (!(info_struct = png_create_info_struct (write_struct)))
	goto out2;

    png_init_io (write_struct, f);

    png_set_IHDR (write_struct, info_struct, width, height,
		  8, PNG_COLOR_TYPE_RGB_ALPHA,
		  PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
		  PNG_FILTER_TYPE_BASE);

    png_write_info (write_struct, info_struct);

    png_write_image (write_struct, row_pointers);

    png_write_end (write_struct, NULL);

    result = TRUE;

out2:
    png_destroy_write_struct (&write_struct, &info_struct);

out1:
    if (fclose (f) != 0)
	result = FALSE;

    pixman_image_unref (copy);
    free (row_pointers);
    free (data);
    return result;
}

#else /* no libpng */

pixman_bool_t
write_png (pixman_image_t *image, const char *filename)
{
    return FALSE;
}

#endif

static void
color8_to_color16 (uint32_t color8, pixman_color_t *color16)
{
    color16->alpha = ((color8 & 0xff000000) >> 24);
    color16->red =   ((color8 & 0x00ff0000) >> 16);
    color16->green = ((color8 & 0x0000ff00) >> 8);
    color16->blue =  ((color8 & 0x000000ff) >> 0);

    color16->alpha |= color16->alpha << 8;
    color16->red   |= color16->red << 8;
    color16->blue  |= color16->blue << 8;
    color16->green |= color16->green << 8;
}

void
draw_checkerboard (pixman_image_t *image,
		   int check_size,
		   uint32_t color1, uint32_t color2)
{
    pixman_color_t check1, check2;
    pixman_image_t *c1, *c2;
    int n_checks_x, n_checks_y;
    int i, j;

    color8_to_color16 (color1, &check1);
    color8_to_color16 (color2, &check2);
    
    c1 = pixman_image_create_solid_fill (&check1);
    c2 = pixman_image_create_solid_fill (&check2);

    n_checks_x = (
	pixman_image_get_width (image) + check_size - 1) / check_size;
    n_checks_y = (
	pixman_image_get_height (image) + check_size - 1) / check_size;

    for (j = 0; j < n_checks_y; j++)
    {
	for (i = 0; i < n_checks_x; i++)
	{
	    pixman_image_t *src;

	    if (((i ^ j) & 1))
		src = c1;
	    else
		src = c2;

	    pixman_image_composite32 (PIXMAN_OP_SRC, src, NULL, image,
				      0, 0, 0, 0,
				      i * check_size, j * check_size,
				      check_size, check_size);
	}
    }
}

static uint32_t
call_test_function (uint32_t    (*test_function)(int testnum, int verbose),
		    int		testnum,
		    int		verbose)
{
    uint32_t retval;

#if defined (__GNUC__) && defined (_WIN32) && (defined (__i386) || defined (__i386__))
    __asm__ (
	/* Deliberately avoid aligning the stack to 16 bytes */
	"pushl	%1\n\t"
	"pushl	%2\n\t"
	"call	*%3\n\t"
	"addl	$8, %%esp\n\t"
	: "=a" (retval)
	: "r" (verbose),
	  "r" (testnum),
	  "r" (test_function)
	: "edx", "ecx"); /* caller save registers */
#else
    retval = test_function (testnum, verbose);
#endif

    return retval;
}

/*
 * A function, which can be used as a core part of the test programs,
 * intended to detect various problems with the help of fuzzing input
 * to pixman API (according to some templates, aka "smart" fuzzing).
 * Some general information about such testing can be found here:
 * http://en.wikipedia.org/wiki/Fuzz_testing
 *
 * It may help detecting:
 *  - crashes on bad handling of valid or reasonably invalid input to
 *    pixman API.
 *  - deviations from the behavior of older pixman releases.
 *  - deviations from the behavior of the same pixman release, but
 *    configured in a different way (for example with SIMD optimizations
 *    disabled), or running on a different OS or hardware.
 *
 * The test is performed by calling a callback function a huge number
 * of times. The callback function is expected to run some snippet of
 * pixman code with pseudorandom variations to the data feeded to
 * pixman API. A result of running each callback function should be
 * some deterministic value which depends on test number (test number
 * can be used as a seed for PRNG). When 'verbose' argument is nonzero,
 * callback function is expected to print to stdout some information
 * about what it does.
 *
 * Return values from many small tests are accumulated together and
 * used as final checksum, which can be compared to some expected
 * value. Running the tests not individually, but in a batch helps
 * to reduce process start overhead and also allows to parallelize
 * testing and utilize multiple CPU cores.
 *
 * The resulting executable can be run without any arguments. In
 * this case it runs a batch of tests starting from 1 and up to
 * 'default_number_of_iterations'. The resulting checksum is
 * compared with 'expected_checksum' and FAIL or PASS verdict
 * depends on the result of this comparison.
 *
 * If the executable is run with 2 numbers provided as command line
 * arguments, they specify the starting and ending numbers for a test
 * batch.
 *
 * If the executable is run with only one number provided as a command
 * line argument, then this number is used to call the callback function
 * once, and also with verbose flag set.
 */
int
fuzzer_test_main (const char *test_name,
		  int         default_number_of_iterations,
		  uint32_t    expected_checksum,
		  uint32_t    (*test_function)(int testnum, int verbose),
		  int         argc,
		  const char *argv[])
{
    int i, n1 = 1, n2 = 0;
    uint32_t checksum = 0;
    int verbose = getenv ("VERBOSE") != NULL;

    if (argc >= 3)
    {
	n1 = atoi (argv[1]);
	n2 = atoi (argv[2]);
	if (n2 < n1)
	{
	    printf ("invalid test range\n");
	    return 1;
	}
    }
    else if (argc >= 2)
    {
	n2 = atoi (argv[1]);

	checksum = call_test_function (test_function, n2, 1);

	printf ("%d: checksum=%08X\n", n2, checksum);
	return 0;
    }
    else
    {
	n1 = 1;
	n2 = default_number_of_iterations;
    }

#ifdef USE_OPENMP
    #pragma omp parallel for reduction(+:checksum) default(none) \
					shared(n1, n2, test_function, verbose)
#endif
    for (i = n1; i <= n2; i++)
    {
	uint32_t crc = call_test_function (test_function, i, 0);
	if (verbose)
	    printf ("%d: %08X\n", i, crc);
	checksum += crc;
    }

    if (n1 == 1 && n2 == default_number_of_iterations)
    {
	if (checksum == expected_checksum)
	{
	    printf ("%s test passed (checksum=%08X)\n",
		    test_name, checksum);
	}
	else
	{
	    printf ("%s test failed! (checksum=%08X, expected %08X)\n",
		    test_name, checksum, expected_checksum);
	    return 1;
	}
    }
    else
    {
	printf ("%d-%d: checksum=%08X\n", n1, n2, checksum);
    }

    return 0;
}

/* Try to obtain current time in seconds */
double
gettime (void)
{
#ifdef HAVE_GETTIMEOFDAY
    struct timeval tv;

    gettimeofday (&tv, NULL);
    return (double)((int64_t)tv.tv_sec * 1000000 + tv.tv_usec) / 1000000.;
#else
    return (double)clock() / (double)CLOCKS_PER_SEC;
#endif
}

uint32_t
get_random_seed (void)
{
    union { double d; uint32_t u32; } t;
    t.d = gettime();
    prng_srand (t.u32);

    return prng_rand ();
}

#ifdef HAVE_SIGACTION
#ifdef HAVE_ALARM
static const char *global_msg;

static void
on_alarm (int signo)
{
    printf ("%s\n", global_msg);
    exit (1);
}
#endif
#endif

void
fail_after (int seconds, const char *msg)
{
#ifdef HAVE_SIGACTION
#ifdef HAVE_ALARM
    struct sigaction action;

    global_msg = msg;

    memset (&action, 0, sizeof (action));
    action.sa_handler = on_alarm;

    alarm (seconds);

    sigaction (SIGALRM, &action, NULL);
#endif
#endif
}

void
enable_divbyzero_exceptions (void)
{
#ifdef HAVE_FENV_H
#ifdef HAVE_FEENABLEEXCEPT
#ifdef HAVE_FEDIVBYZERO
    feenableexcept (FE_DIVBYZERO);
#endif
#endif
#endif
}

void
enable_invalid_exceptions (void)
{
#ifdef HAVE_FENV_H
#ifdef HAVE_FEENABLEEXCEPT
#ifdef FE_INVALID
    feenableexcept (FE_INVALID);
#endif
#endif
#endif
}

void *
aligned_malloc (size_t align, size_t size)
{
    void *result;

#ifdef HAVE_POSIX_MEMALIGN
    if (posix_memalign (&result, align, size) != 0)
      result = NULL;
#else
    result = malloc (size);
#endif

    return result;
}

#define CONVERT_15(c, is_rgb)						\
    (is_rgb?								\
     ((((c) >> 3) & 0x001f) |						\
      (((c) >> 6) & 0x03e0) |						\
      (((c) >> 9) & 0x7c00)) :						\
     (((((c) >> 16) & 0xff) * 153 +					\
       (((c) >>  8) & 0xff) * 301 +					\
       (((c)      ) & 0xff) * 58) >> 2))

double
convert_srgb_to_linear (double c)
{
    if (c <= 0.04045)
        return c / 12.92;
    else
        return pow ((c + 0.055) / 1.055, 2.4);
}

double
convert_linear_to_srgb (double c)
{
    if (c <= 0.0031308)
        return c * 12.92;
    else
        return 1.055 * pow (c, 1.0/2.4) - 0.055;
}

void
initialize_palette (pixman_indexed_t *palette, uint32_t depth, int is_rgb)
{
    int i;
    uint32_t mask = (1 << depth) - 1;

    for (i = 0; i < 32768; ++i)
	palette->ent[i] = prng_rand() & mask;

    memset (palette->rgba, 0, sizeof (palette->rgba));

    for (i = 0; i < mask + 1; ++i)
    {
	uint32_t rgba24;
 	pixman_bool_t retry;
	uint32_t i15;

	/* We filled the rgb->index map with random numbers, but we
	 * do need the ability to round trip, that is if some indexed
	 * color expands to an argb24, then the 15 bit version of that
	 * color must map back to the index. Anything else, we don't
	 * care about too much.
	 */
	do
	{
	    uint32_t old_idx;

	    rgba24 = prng_rand();
	    i15 = CONVERT_15 (rgba24, is_rgb);

	    old_idx = palette->ent[i15];
	    if (CONVERT_15 (palette->rgba[old_idx], is_rgb) == i15)
		retry = 1;
	    else
		retry = 0;
	} while (retry);

	palette->rgba[i] = rgba24;
	palette->ent[i15] = i;
    }

    for (i = 0; i < mask + 1; ++i)
    {
	assert (palette->ent[CONVERT_15 (palette->rgba[i], is_rgb)] == i);
    }
}

struct operator_entry {
    pixman_op_t		 op;
    const char		*name;
    pixman_bool_t	 is_alias;
};

typedef struct operator_entry operator_entry_t;

static const operator_entry_t op_list[] =
{
#define ENTRY(op)							\
    { PIXMAN_OP_##op, "PIXMAN_OP_" #op, FALSE }
#define ALIAS(op, nam)							\
    { PIXMAN_OP_##op, nam, TRUE }

    /* operator_name () will return the first hit in this table,
     * so keep the list properly ordered between entries and aliases.
     * Aliases are not listed by list_operators ().
     */

    ENTRY (CLEAR),
    ENTRY (SRC),
    ENTRY (DST),
    ENTRY (OVER),
    ENTRY (OVER_REVERSE),
    ALIAS (OVER_REVERSE,		"overrev"),
    ENTRY (IN),
    ENTRY (IN_REVERSE),
    ALIAS (IN_REVERSE,			"inrev"),
    ENTRY (OUT),
    ENTRY (OUT_REVERSE),
    ALIAS (OUT_REVERSE,			"outrev"),
    ENTRY (ATOP),
    ENTRY (ATOP_REVERSE),
    ALIAS (ATOP_REVERSE,		"atoprev"),
    ENTRY (XOR),
    ENTRY (ADD),
    ENTRY (SATURATE),

    ENTRY (DISJOINT_CLEAR),
    ENTRY (DISJOINT_SRC),
    ENTRY (DISJOINT_DST),
    ENTRY (DISJOINT_OVER),
    ENTRY (DISJOINT_OVER_REVERSE),
    ENTRY (DISJOINT_IN),
    ENTRY (DISJOINT_IN_REVERSE),
    ENTRY (DISJOINT_OUT),
    ENTRY (DISJOINT_OUT_REVERSE),
    ENTRY (DISJOINT_ATOP),
    ENTRY (DISJOINT_ATOP_REVERSE),
    ENTRY (DISJOINT_XOR),

    ENTRY (CONJOINT_CLEAR),
    ENTRY (CONJOINT_SRC),
    ENTRY (CONJOINT_DST),
    ENTRY (CONJOINT_OVER),
    ENTRY (CONJOINT_OVER_REVERSE),
    ENTRY (CONJOINT_IN),
    ENTRY (CONJOINT_IN_REVERSE),
    ENTRY (CONJOINT_OUT),
    ENTRY (CONJOINT_OUT_REVERSE),
    ENTRY (CONJOINT_ATOP),
    ENTRY (CONJOINT_ATOP_REVERSE),
    ENTRY (CONJOINT_XOR),

    ENTRY (MULTIPLY),
    ENTRY (SCREEN),
    ENTRY (OVERLAY),
    ENTRY (DARKEN),
    ENTRY (LIGHTEN),
    ENTRY (COLOR_DODGE),
    ENTRY (COLOR_BURN),
    ENTRY (HARD_LIGHT),
    ENTRY (SOFT_LIGHT),
    ENTRY (DIFFERENCE),
    ENTRY (EXCLUSION),
    ENTRY (HSL_HUE),
    ENTRY (HSL_SATURATION),
    ENTRY (HSL_COLOR),
    ENTRY (HSL_LUMINOSITY),

    ALIAS (NONE, "<invalid operator 'none'>")

#undef ENTRY
#undef ALIAS
};

typedef struct {
    pixman_dither_t	 dither;
    const char		*name;
    pixman_bool_t	 is_alias;
} dither_entry_t;

static const dither_entry_t dither_list[] =
{
#define ENTRY(dither)							\
    { PIXMAN_DITHER_##dither, "PIXMAN_DITHER_" #dither, FALSE }
#define ALIAS(dither, nam)							\
    { PIXMAN_DITHER_##dither, nam, TRUE }

    /* dither_name () will return the first hit in this table,
     * so keep the list properly ordered between entries and aliases.
     * Aliases are not listed by list_dithers ().
     */

    ENTRY (ORDERED_BAYER_8),
    ENTRY (ORDERED_BLUE_NOISE_64),
    ENTRY (NONE),

#undef ENTRY
#undef ALIAS
};

struct format_entry
{
    pixman_format_code_t format;
    const char		*name;
    pixman_bool_t	 is_alias;
};

typedef struct format_entry format_entry_t;

static const format_entry_t format_list[] =
{
#define ENTRY(f)							\
    { PIXMAN_##f, #f, FALSE }
#define ALIAS(f, nam)							\
    { PIXMAN_##f, nam, TRUE }

    /* format_name () will return the first hit in this table,
     * so keep the list properly ordered between entries and aliases.
     * Aliases are not listed by list_formats ().
     */

/* 128bpp formats */
    ENTRY (rgba_float),
/* 96bpp formats */
    ENTRY (rgb_float),

/* 32bpp formats */
    ENTRY (a8r8g8b8),
    ALIAS (a8r8g8b8,		"8888"),
    ENTRY (x8r8g8b8),
    ALIAS (x8r8g8b8,		"x888"),
    ENTRY (a8b8g8r8),
    ENTRY (x8b8g8r8),
    ENTRY (b8g8r8a8),
    ENTRY (b8g8r8x8),
    ENTRY (r8g8b8a8),
    ENTRY (r8g8b8x8),
    ENTRY (x14r6g6b6),
    ENTRY (x2r10g10b10),
    ALIAS (x2r10g10b10,		"2x10"),
    ENTRY (a2r10g10b10),
    ALIAS (a2r10g10b10,		"2a10"),
    ENTRY (x2b10g10r10),
    ENTRY (a2b10g10r10),

/* sRGB formats */
    ENTRY (a8r8g8b8_sRGB),
    ENTRY (r8g8b8_sRGB),

/* 24bpp formats */
    ENTRY (r8g8b8),
    ALIAS (r8g8b8,		"0888"),
    ENTRY (b8g8r8),

/* 16 bpp formats */
    ENTRY (r5g6b5),
    ALIAS (r5g6b5,		"0565"),
    ENTRY (b5g6r5),

    ENTRY (a1r5g5b5),
    ALIAS (a1r5g5b5,		"1555"),
    ENTRY (x1r5g5b5),
    ENTRY (a1b5g5r5),
    ENTRY (x1b5g5r5),
    ENTRY (a4r4g4b4),
    ALIAS (a4r4g4b4,		"4444"),
    ENTRY (x4r4g4b4),
    ENTRY (a4b4g4r4),
    ENTRY (x4b4g4r4),

/* 8bpp formats */
    ENTRY (a8),
    ALIAS (a8,			"8"),
    ENTRY (r3g3b2),
    ENTRY (b2g3r3),
    ENTRY (a2r2g2b2),
    ALIAS (a2r2g2b2,		"2222"),
    ENTRY (a2b2g2r2),

    ALIAS (c8,			"x4c4 / c8"),
    /* ENTRY (c8), */
    ALIAS (g8,			"x4g4 / g8"),
    /* ENTRY (g8), */

    ENTRY (x4a4),

    /* These format codes are identical to c8 and g8, respectively. */
    /* ENTRY (x4c4), */
    /* ENTRY (x4g4), */

/* 4 bpp formats */
    ENTRY (a4),
    ENTRY (r1g2b1),
    ENTRY (b1g2r1),
    ENTRY (a1r1g1b1),
    ENTRY (a1b1g1r1),

    ALIAS (c4,			"c4"),
    /* ENTRY (c4), */
    ALIAS (g4,			"g4"),
    /* ENTRY (g4), */

/* 1bpp formats */
    ENTRY (a1),

    ALIAS (g1,			"g1"),
    /* ENTRY (g1), */

/* YUV formats */
    ALIAS (yuy2,		"yuy2"),
    /* ENTRY (yuy2), */
    ALIAS (yv12,		"yv12"),
    /* ENTRY (yv12), */

/* Fake formats, not in pixman_format_code_t enum */
    ALIAS (null,		"null"),
    ALIAS (solid,		"solid"),
    ALIAS (solid,		"n"),
    ALIAS (pixbuf,		"pixbuf"),
    ALIAS (rpixbuf,		"rpixbuf"),
    ALIAS (unknown,		"unknown"),

#undef ENTRY
#undef ALIAS
};

pixman_format_code_t
format_from_string (const char *s)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (format_list); ++i)
    {
        const format_entry_t *ent = &format_list[i];

        if (strcasecmp (ent->name, s) == 0)
            return ent->format;
    }

    return PIXMAN_null;
}

static void
emit (const char *s, int *n_chars)
{
    *n_chars += printf ("%s,", s);
    if (*n_chars > 60)
    {
        printf ("\n    ");
        *n_chars = 0;
    }
    else
    {
        printf (" ");
        (*n_chars)++;
    }
}

void
list_formats (void)
{
    int n_chars;
    int i;

    printf ("Formats:\n    ");

    n_chars = 0;
    for (i = 0; i < ARRAY_LENGTH (format_list); ++i)
    {
        const format_entry_t *ent = &format_list[i];

        if (ent->is_alias)
            continue;

        emit (ent->name, &n_chars);
    }

    printf ("\n\n");
}

void
list_operators (void)
{
    char short_name [128] = { 0 };
    int i, n_chars;

    printf ("Operators:\n    ");

    n_chars = 0;
    for (i = 0; i < ARRAY_LENGTH (op_list); ++i)
    {
        const operator_entry_t *ent = &op_list[i];
        int j;

        if (ent->is_alias)
            continue;

        snprintf (short_name, sizeof (short_name) - 1, "%s",
                  ent->name + strlen ("PIXMAN_OP_"));

        for (j = 0; short_name[j] != '\0'; ++j)
            short_name[j] = tolower (short_name[j]);

        emit (short_name, &n_chars);
    }

    printf ("\n\n");
}

void
list_dithers (void)
{
    int n_chars;
    int i;

    printf ("Dithers:\n    ");

    n_chars = 0;
    for (i = 0; i < ARRAY_LENGTH (dither_list); ++i)
    {
        const dither_entry_t *ent = &dither_list[i];

        if (ent->is_alias)
            continue;

        emit (ent->name, &n_chars);
    }

    printf ("\n\n");
}

pixman_op_t
operator_from_string (const char *s)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (op_list); ++i)
    {
        const operator_entry_t *ent = &op_list[i];

        if (ent->is_alias)
        {
            if (strcasecmp (ent->name, s) == 0)
                return ent->op;
        }
        else
        {
            if (strcasecmp (ent->name + strlen ("PIXMAN_OP_"), s) == 0)
                return ent->op;
        }
    }

    return PIXMAN_OP_NONE;
}

pixman_dither_t
dither_from_string (const char *s)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (dither_list); ++i)
    {
        const dither_entry_t *ent = &dither_list[i];

        if (strcasecmp (ent->name, s) == 0)
            return ent->dither;
    }

    return PIXMAN_DITHER_NONE;
}

const char *
operator_name (pixman_op_t op)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (op_list); ++i)
    {
        const operator_entry_t *ent = &op_list[i];

        if (ent->op == op)
            return ent->name;
    }

    return "<unknown operator>";
}

const char *
format_name (pixman_format_code_t format)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (format_list); ++i)
    {
        const format_entry_t *ent = &format_list[i];

        if (ent->format == format)
            return ent->name;
    }

    return "<unknown format>";
};

const char *
dither_name (pixman_dither_t dither)
{
    int i;

    for (i = 0; i < ARRAY_LENGTH (dither_list); ++i)
    {
	const dither_entry_t *ent = &dither_list[i];

	if (ent->dither == dither)
	    return ent->name;
    }

    return "<unknown dither>";
}

#define IS_ZERO(f)     (-DBL_MIN < (f) && (f) < DBL_MIN)

typedef double (* blend_func_t) (double as, double s, double ad, double d);

static force_inline double
blend_multiply (double sa, double s, double da, double d)
{
    return d * s;
}

static force_inline double
blend_screen (double sa, double s, double da, double d)
{
    return d * sa + s * da - s * d;
}

static force_inline double
blend_overlay (double sa, double s, double da, double d)
{
    if (2 * d < da)
        return 2 * s * d;
    else
        return sa * da - 2 * (da - d) * (sa - s);
}

static force_inline double
blend_darken (double sa, double s, double da, double d)
{
    s = s * da;
    d = d * sa;

    if (s > d)
        return d;
    else
        return s;
}

static force_inline double
blend_lighten (double sa, double s, double da, double d)
{
    s = s * da;
    d = d * sa;

    if (s > d)
        return s;
    else
        return d;
}

static force_inline double
blend_color_dodge (double sa, double s, double da, double d)
{
    if (IS_ZERO (d))
        return 0.0f;
    else if (d * sa >= sa * da - s * da)
        return sa * da;
    else if (IS_ZERO (sa - s))
        return sa * da;
    else
        return sa * sa * d / (sa - s);
}

static force_inline double
blend_color_burn (double sa, double s, double da, double d)
{
    if (d >= da)
        return sa * da;
    else if (sa * (da - d) >= s * da)
        return 0.0f;
    else if (IS_ZERO (s))
        return 0.0f;
    else
        return sa * (da - sa * (da - d) / s);
}

static force_inline double
blend_hard_light (double sa, double s, double da, double d)
{
    if (2 * s < sa)
        return 2 * s * d;
    else
        return sa * da - 2 * (da - d) * (sa - s);
}

static force_inline double
blend_soft_light (double sa, double s, double da, double d)
{
    if (2 * s <= sa)
    {
        if (IS_ZERO (da))
            return d * sa;
        else
            return d * sa - d * (da - d) * (sa - 2 * s) / da;
    }
    else
    {
        if (IS_ZERO (da))
        {
	    return d * sa;
        }
        else
        {
            if (4 * d <= da)
                return d * sa + (2 * s - sa) * d * ((16 * d / da - 12) * d / da + 3);
            else
                return d * sa + (sqrt (d * da) - d) * (2 * s - sa);
        }
    }
}

static force_inline double
blend_difference (double sa, double s, double da, double d)
{
    double dsa = d * sa;
    double sda = s * da;

    if (sda < dsa)
        return dsa - sda;
    else
        return sda - dsa;
}

static force_inline double
blend_exclusion (double sa, double s, double da, double d)
{
    return s * da + d * sa - 2 * d * s;
}

static double
clamp (double d)
{
    if (d > 1.0)
	return 1.0;
    else if (d < 0.0)
	return 0.0;
    else
	return d;
}

static double
blend_channel (double as, double s, double ad, double d,
                   blend_func_t blend)
{
    return clamp ((1 - ad) * s + (1 - as) * d + blend (as, s, ad, d));
}

static double
calc_op (pixman_op_t op, double src, double dst, double srca, double dsta)
{
#define mult_chan(src, dst, Fa, Fb) MIN ((src) * (Fa) + (dst) * (Fb), 1.0)

    double Fa, Fb;

    switch (op)
    {
    case PIXMAN_OP_CLEAR:
    case PIXMAN_OP_DISJOINT_CLEAR:
    case PIXMAN_OP_CONJOINT_CLEAR:
	return mult_chan (src, dst, 0.0, 0.0);

    case PIXMAN_OP_SRC:
    case PIXMAN_OP_DISJOINT_SRC:
    case PIXMAN_OP_CONJOINT_SRC:
	return mult_chan (src, dst, 1.0, 0.0);

    case PIXMAN_OP_DST:
    case PIXMAN_OP_DISJOINT_DST:
    case PIXMAN_OP_CONJOINT_DST:
	return mult_chan (src, dst, 0.0, 1.0);

    case PIXMAN_OP_OVER:
	return mult_chan (src, dst, 1.0, 1.0 - srca);

    case PIXMAN_OP_OVER_REVERSE:
	return mult_chan (src, dst, 1.0 - dsta, 1.0);

    case PIXMAN_OP_IN:
	return mult_chan (src, dst, dsta, 0.0);

    case PIXMAN_OP_IN_REVERSE:
	return mult_chan (src, dst, 0.0, srca);

    case PIXMAN_OP_OUT:
	return mult_chan (src, dst, 1.0 - dsta, 0.0);

    case PIXMAN_OP_OUT_REVERSE:
	return mult_chan (src, dst, 0.0, 1.0 - srca);

    case PIXMAN_OP_ATOP:
	return mult_chan (src, dst, dsta, 1.0 - srca);

    case PIXMAN_OP_ATOP_REVERSE:
	return mult_chan (src, dst, 1.0 - dsta,  srca);

    case PIXMAN_OP_XOR:
	return mult_chan (src, dst, 1.0 - dsta, 1.0 - srca);

    case PIXMAN_OP_ADD:
	return mult_chan (src, dst, 1.0, 1.0);

    case PIXMAN_OP_SATURATE:
    case PIXMAN_OP_DISJOINT_OVER_REVERSE:
	if (srca == 0.0)
	    Fa = 1.0;
	else
	    Fa = MIN (1.0, (1.0 - dsta) / srca);
	return mult_chan (src, dst, Fa, 1.0);

    case PIXMAN_OP_DISJOINT_OVER:
	if (dsta == 0.0)
	    Fb = 1.0;
	else
	    Fb = MIN (1.0, (1.0 - srca) / dsta);
	return mult_chan (src, dst, 1.0, Fb);

    case PIXMAN_OP_DISJOINT_IN:
	if (srca == 0.0)
	    Fa = 0.0;
	else
	    Fa = MAX (0.0, 1.0 - (1.0 - dsta) / srca);
	return mult_chan (src, dst, Fa, 0.0);

    case PIXMAN_OP_DISJOINT_IN_REVERSE:
	if (dsta == 0.0)
	    Fb = 0.0;
	else
	    Fb = MAX (0.0, 1.0 - (1.0 - srca) / dsta);
	return mult_chan (src, dst, 0.0, Fb);

    case PIXMAN_OP_DISJOINT_OUT:
	if (srca == 0.0)
	    Fa = 1.0;
	else
	    Fa = MIN (1.0, (1.0 - dsta) / srca);
	return mult_chan (src, dst, Fa, 0.0);

    case PIXMAN_OP_DISJOINT_OUT_REVERSE:
	if (dsta == 0.0)
	    Fb = 1.0;
	else
	    Fb = MIN (1.0, (1.0 - srca) / dsta);
	return mult_chan (src, dst, 0.0, Fb);

    case PIXMAN_OP_DISJOINT_ATOP:
	if (srca == 0.0)
	    Fa = 0.0;
	else
	    Fa = MAX (0.0, 1.0 - (1.0 - dsta) / srca);
	if (dsta == 0.0)
	    Fb = 1.0;
	else
	    Fb = MIN (1.0, (1.0 - srca) / dsta);
	return mult_chan (src, dst, Fa, Fb);

    case PIXMAN_OP_DISJOINT_ATOP_REVERSE:
	if (srca == 0.0)
	    Fa = 1.0;
	else
	    Fa = MIN (1.0, (1.0 - dsta) / srca);
	if (dsta == 0.0)
	    Fb = 0.0;
	else
	    Fb = MAX (0.0, 1.0 - (1.0 - srca) / dsta);
	return mult_chan (src, dst, Fa, Fb);

    case PIXMAN_OP_DISJOINT_XOR:
	if (srca == 0.0)
	    Fa = 1.0;
	else
	    Fa = MIN (1.0, (1.0 - dsta) / srca);
	if (dsta == 0.0)
	    Fb = 1.0;
	else
	    Fb = MIN (1.0, (1.0 - srca) / dsta);
	return mult_chan (src, dst, Fa, Fb);

    case PIXMAN_OP_CONJOINT_OVER:
	if (dsta == 0.0)
	    Fb = 0.0;
	else
	    Fb = MAX (0.0, 1.0 - srca / dsta);
	return mult_chan (src, dst, 1.0, Fb);

    case PIXMAN_OP_CONJOINT_OVER_REVERSE:
	if (srca == 0.0)
	    Fa = 0.0;
	else
	    Fa = MAX (0.0, 1.0 - dsta / srca);
	return mult_chan (src, dst, Fa, 1.0);

    case PIXMAN_OP_CONJOINT_IN:
	if (srca == 0.0)
	    Fa = 1.0;
	else
	    Fa = MIN (1.0, dsta / srca);
	return mult_chan (src, dst, Fa, 0.0);

    case PIXMAN_OP_CONJOINT_IN_REVERSE:
	if (dsta == 0.0)
	    Fb = 1.0;
	else
	    Fb = MIN (1.0, srca / dsta);
	return mult_chan (src, dst, 0.0, Fb);

    case PIXMAN_OP_CONJOINT_OUT:
	if (srca == 0.0)
	    Fa = 0.0;
	else
	    Fa = MAX (0.0, 1.0 - dsta / srca);
	return mult_chan (src, dst, Fa, 0.0);

    case PIXMAN_OP_CONJOINT_OUT_REVERSE:
	if (dsta == 0.0)
	    Fb = 0.0;
	else
	    Fb = MAX (0.0, 1.0 - srca / dsta);
	return mult_chan (src, dst, 0.0, Fb);

    case PIXMAN_OP_CONJOINT_ATOP:
	if (srca == 0.0)
	    Fa = 1.0;
	else
	    Fa = MIN (1.0, dsta / srca);
	if (dsta == 0.0)
	    Fb = 0.0;
	else
	    Fb = MAX (0.0, 1.0 - srca / dsta);
	return mult_chan (src, dst, Fa, Fb);

    case PIXMAN_OP_CONJOINT_ATOP_REVERSE:
	if (srca == 0.0)
	    Fa = 0.0;
	else
	    Fa = MAX (0.0, 1.0 - dsta / srca);
	if (dsta == 0.0)
	    Fb = 1.0;
	else
	    Fb = MIN (1.0, srca / dsta);
	return mult_chan (src, dst, Fa, Fb);

    case PIXMAN_OP_CONJOINT_XOR:
	if (srca == 0.0)
	    Fa = 0.0;
	else
	    Fa = MAX (0.0, 1.0 - dsta / srca);
	if (dsta == 0.0)
	    Fb = 0.0;
	else
	    Fb = MAX (0.0, 1.0 - srca / dsta);
	return mult_chan (src, dst, Fa, Fb);

    case PIXMAN_OP_MULTIPLY:
    case PIXMAN_OP_SCREEN:
    case PIXMAN_OP_OVERLAY:
    case PIXMAN_OP_DARKEN:
    case PIXMAN_OP_LIGHTEN:
    case PIXMAN_OP_COLOR_DODGE:
    case PIXMAN_OP_COLOR_BURN:
    case PIXMAN_OP_HARD_LIGHT:
    case PIXMAN_OP_SOFT_LIGHT:
    case PIXMAN_OP_DIFFERENCE:
    case PIXMAN_OP_EXCLUSION:
    case PIXMAN_OP_HSL_HUE:
    case PIXMAN_OP_HSL_SATURATION:
    case PIXMAN_OP_HSL_COLOR:
    case PIXMAN_OP_HSL_LUMINOSITY:
    default:
	abort();
	return 0; /* silence MSVC */
    }
#undef mult_chan
}

void
do_composite (pixman_op_t op,
	      const color_t *src,
	      const color_t *mask,
	      const color_t *dst,
	      color_t *result,
	      pixman_bool_t component_alpha)
{
    color_t srcval, srcalpha;

    static const blend_func_t blend_funcs[] =
    {
        blend_multiply,
        blend_screen,
        blend_overlay,
        blend_darken,
        blend_lighten,
        blend_color_dodge,
        blend_color_burn,
        blend_hard_light,
        blend_soft_light,
        blend_difference,
        blend_exclusion,
    };

    if (mask == NULL)
    {
	srcval = *src;

	srcalpha.r = src->a;
	srcalpha.g = src->a;
	srcalpha.b = src->a;
	srcalpha.a = src->a;
    }
    else if (component_alpha)
    {
	srcval.r = src->r * mask->r;
	srcval.g = src->g * mask->g;
	srcval.b = src->b * mask->b;
	srcval.a = src->a * mask->a;

	srcalpha.r = src->a * mask->r;
	srcalpha.g = src->a * mask->g;
	srcalpha.b = src->a * mask->b;
	srcalpha.a = src->a * mask->a;
    }
    else
    {
	srcval.r = src->r * mask->a;
	srcval.g = src->g * mask->a;
	srcval.b = src->b * mask->a;
	srcval.a = src->a * mask->a;

	srcalpha.r = src->a * mask->a;
	srcalpha.g = src->a * mask->a;
	srcalpha.b = src->a * mask->a;
	srcalpha.a = src->a * mask->a;
    }

    if (op >= PIXMAN_OP_MULTIPLY)
    {
        blend_func_t func = blend_funcs[op - PIXMAN_OP_MULTIPLY];

	result->a = srcalpha.a + dst->a - srcalpha.a * dst->a;
	result->r = blend_channel (srcalpha.r, srcval.r, dst->a, dst->r, func);
	result->g = blend_channel (srcalpha.g, srcval.g, dst->a, dst->g, func);
	result->b = blend_channel (srcalpha.b, srcval.b, dst->a, dst->b, func);
    }
    else
    {
        result->r = calc_op (op, srcval.r, dst->r, srcalpha.r, dst->a);
        result->g = calc_op (op, srcval.g, dst->g, srcalpha.g, dst->a);
        result->b = calc_op (op, srcval.b, dst->b, srcalpha.b, dst->a);
        result->a = calc_op (op, srcval.a, dst->a, srcalpha.a, dst->a);
    }
}

static double
round_channel (double p, int m)
{
    int t;
    double r;

    t = p * ((1 << m));
    t -= t >> m;

    r = t / (double)((1 << m) - 1);

    return r;
}

void
round_color (pixman_format_code_t format, color_t *color)
{
    if (PIXMAN_FORMAT_R (format) == 0)
    {
	color->r = 0.0;
	color->g = 0.0;
	color->b = 0.0;
    }
    else
    {
	color->r = round_channel (color->r, PIXMAN_FORMAT_R (format));
	color->g = round_channel (color->g, PIXMAN_FORMAT_G (format));
	color->b = round_channel (color->b, PIXMAN_FORMAT_B (format));
    }

    if (PIXMAN_FORMAT_A (format) == 0)
	color->a = 1;
    else
	color->a = round_channel (color->a, PIXMAN_FORMAT_A (format));
}

/* The acceptable deviation in units of [0.0, 1.0]
 */
#define DEVIATION (0.0128)

/* Check whether @pixel is a valid quantization of the a, r, g, b
 * parameters. Some slack is permitted.
 */
void
pixel_checker_init (pixel_checker_t *checker, pixman_format_code_t format)
{
    assert (PIXMAN_FORMAT_VIS (format));

    checker->format = format;

    if (format == PIXMAN_rgba_float ||
	format == PIXMAN_rgb_float)
	return;

    switch (PIXMAN_FORMAT_TYPE (format))
    {
    case PIXMAN_TYPE_A:
	checker->bs = 0;
	checker->gs = 0;
	checker->rs = 0;
	checker->as = 0;
	break;

    case PIXMAN_TYPE_ARGB:
    case PIXMAN_TYPE_ARGB_SRGB:
	checker->bs = 0;
	checker->gs = checker->bs + PIXMAN_FORMAT_B (format);
	checker->rs = checker->gs + PIXMAN_FORMAT_G (format);
	checker->as = checker->rs + PIXMAN_FORMAT_R (format);
	break;

    case PIXMAN_TYPE_ABGR:
	checker->rs = 0;
	checker->gs = checker->rs + PIXMAN_FORMAT_R (format);
	checker->bs = checker->gs + PIXMAN_FORMAT_G (format);
	checker->as = checker->bs + PIXMAN_FORMAT_B (format);
	break;

    case PIXMAN_TYPE_BGRA:
	/* With BGRA formats we start counting at the high end of the pixel */
	checker->bs = PIXMAN_FORMAT_BPP (format) - PIXMAN_FORMAT_B (format);
	checker->gs = checker->bs - PIXMAN_FORMAT_B (format);
	checker->rs = checker->gs - PIXMAN_FORMAT_G (format);
	checker->as = checker->rs - PIXMAN_FORMAT_R (format);
	break;

    case PIXMAN_TYPE_RGBA:
	/* With BGRA formats we start counting at the high end of the pixel */
	checker->rs = PIXMAN_FORMAT_BPP (format) - PIXMAN_FORMAT_R (format);
	checker->gs = checker->rs - PIXMAN_FORMAT_R (format);
	checker->bs = checker->gs - PIXMAN_FORMAT_G (format);
	checker->as = checker->bs - PIXMAN_FORMAT_B (format);
	break;

    default:
	assert (0);
	break;
    }

    checker->am = ((1U << PIXMAN_FORMAT_A (format)) - 1) << checker->as;
    checker->rm = ((1U << PIXMAN_FORMAT_R (format)) - 1) << checker->rs;
    checker->gm = ((1U << PIXMAN_FORMAT_G (format)) - 1) << checker->gs;
    checker->bm = ((1U << PIXMAN_FORMAT_B (format)) - 1) << checker->bs;

    checker->aw = PIXMAN_FORMAT_A (format);
    checker->rw = PIXMAN_FORMAT_R (format);
    checker->gw = PIXMAN_FORMAT_G (format);
    checker->bw = PIXMAN_FORMAT_B (format);

    checker->ad = DEVIATION;
    checker->rd = DEVIATION;
    checker->gd = DEVIATION;
    checker->bd = DEVIATION;
}

/* When dithering is enabled, we allow one extra pixel of tolerance
 */
void
pixel_checker_allow_dither (pixel_checker_t *checker)
{
    checker->ad += 1 / (double)((1 << checker->aw) - 1);
    checker->rd += 1 / (double)((1 << checker->rw) - 1);
    checker->gd += 1 / (double)((1 << checker->gw) - 1);
    checker->bd += 1 / (double)((1 << checker->bw) - 1);
}

static void
pixel_checker_require_uint32_format (const pixel_checker_t *checker)
{
    assert (checker->format != PIXMAN_rgba_float &&
	    checker->format != PIXMAN_rgb_float);
}

void
pixel_checker_split_pixel (const pixel_checker_t *checker, uint32_t pixel,
			   int *a, int *r, int *g, int *b)
{
    pixel_checker_require_uint32_format(checker);

    *a = (pixel & checker->am) >> checker->as;
    *r = (pixel & checker->rm) >> checker->rs;
    *g = (pixel & checker->gm) >> checker->gs;
    *b = (pixel & checker->bm) >> checker->bs;
}

void
pixel_checker_get_masks (const pixel_checker_t *checker,
                         uint32_t              *am,
                         uint32_t              *rm,
                         uint32_t              *gm,
                         uint32_t              *bm)
{
    pixel_checker_require_uint32_format(checker);

    if (am)
        *am = checker->am;
    if (rm)
        *rm = checker->rm;
    if (gm)
        *gm = checker->gm;
    if (bm)
        *bm = checker->bm;
}

void
pixel_checker_convert_pixel_to_color (const pixel_checker_t *checker,
                                      uint32_t pixel, color_t *color)
{
    int a, r, g, b;

    pixel_checker_require_uint32_format(checker);

    pixel_checker_split_pixel (checker, pixel, &a, &r, &g, &b);

    if (checker->am == 0)
        color->a = 1.0;
    else
        color->a = a / (double)(checker->am >> checker->as);

    if (checker->rm == 0)
        color->r = 0.0;
    else
        color->r = r / (double)(checker->rm >> checker->rs);

    if (checker->gm == 0)
        color->g = 0.0;
    else
        color->g = g / (double)(checker->gm >> checker->gs);

    if (checker->bm == 0)
        color->b = 0.0;
    else
        color->b = b / (double)(checker->bm >> checker->bs);

    if (PIXMAN_FORMAT_TYPE (checker->format) == PIXMAN_TYPE_ARGB_SRGB)
    {
	color->r = convert_srgb_to_linear (color->r);
	color->g = convert_srgb_to_linear (color->g);
	color->b = convert_srgb_to_linear (color->b);
    }
}

static int32_t
convert (double v, uint32_t width, uint32_t mask, uint32_t shift, double def)
{
    int32_t r;

    if (!mask)
	v = def;

    r = (v * ((mask >> shift) + 1));
    r -= r >> width;

    return r;
}

static void
get_limits (const pixel_checker_t *checker, double sign,
	    color_t *color,
	    int *ao, int *ro, int *go, int *bo)
{
    color_t tmp;

    if (PIXMAN_FORMAT_TYPE (checker->format) == PIXMAN_TYPE_ARGB_SRGB)
    {
	tmp.a = color->a;
	tmp.r = convert_linear_to_srgb (color->r);
	tmp.g = convert_linear_to_srgb (color->g);
	tmp.b = convert_linear_to_srgb (color->b);

	color = &tmp;
    }
    
    *ao = convert (color->a + sign * checker->ad,
		   checker->aw, checker->am, checker->as, 1.0);
    *ro = convert (color->r + sign * checker->rd,
		   checker->rw, checker->rm, checker->rs, 0.0);
    *go = convert (color->g + sign * checker->gd,
		   checker->gw, checker->gm, checker->gs, 0.0);
    *bo = convert (color->b + sign * checker->bd,
		   checker->bw, checker->bm, checker->bs, 0.0);
}

void
pixel_checker_get_max (const pixel_checker_t *checker, color_t *color,
		       int *am, int *rm, int *gm, int *bm)
{
    pixel_checker_require_uint32_format(checker);

    get_limits (checker, 1, color, am, rm, gm, bm);
}

void
pixel_checker_get_min (const pixel_checker_t *checker, color_t *color,
		       int *am, int *rm, int *gm, int *bm)
{
    pixel_checker_require_uint32_format(checker);

    get_limits (checker, - 1, color, am, rm, gm, bm);
}

pixman_bool_t
pixel_checker_check (const pixel_checker_t *checker, uint32_t pixel,
		     color_t *color)
{
    int32_t a_lo, a_hi, r_lo, r_hi, g_lo, g_hi, b_lo, b_hi;
    int32_t ai, ri, gi, bi;
    pixman_bool_t result;

    pixel_checker_require_uint32_format(checker);

    pixel_checker_get_min (checker, color, &a_lo, &r_lo, &g_lo, &b_lo);
    pixel_checker_get_max (checker, color, &a_hi, &r_hi, &g_hi, &b_hi);
    pixel_checker_split_pixel (checker, pixel, &ai, &ri, &gi, &bi);

    result =
	a_lo <= ai && ai <= a_hi	&&
	r_lo <= ri && ri <= r_hi	&&
	g_lo <= gi && gi <= g_hi	&&
	b_lo <= bi && bi <= b_hi;

    return result;
}

static void
color_limits (const pixel_checker_t *checker,
	      double limit, const color_t *color, color_t *out)
{
    if (PIXMAN_FORMAT_A(checker->format))
	out->a = color->a + limit;
    else
	out->a = 1.;

    out->r = color->r + limit;
    out->g = color->g + limit;
    out->b = color->b + limit;
}

pixman_bool_t
pixel_checker_check_color (const pixel_checker_t *checker,
			   const color_t *actual, const color_t *reference)
{
    color_t min, max;
    pixman_bool_t result;

    color_limits(checker, -DEVIATION, reference, &min);
    color_limits(checker, DEVIATION, reference, &max);

    result =
	actual->a >= min.a && actual->a <= max.a &&
	actual->r >= min.r && actual->r <= max.r &&
	actual->g >= min.g && actual->g <= max.g &&
	actual->b >= min.b && actual->b <= max.b;

    return result;
}
