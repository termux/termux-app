/* Blue noise generation using the void-and-cluster method as described in
 *
 *     The void-and-cluster method for dither array generation
 *     Ulichney, Robert A (1993)
 *
 *     http://cv.ulichney.com/papers/1993-void-cluster.pdf
 *
 * Note that running with openmp (-DUSE_OPENMP) will trigger additional
 * randomness due to computing reductions in parallel, and is not recommended
 * unless generating very large dither arrays.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

/* Booleans and utility functions */

#ifndef TRUE
#   define TRUE 1
#endif

#ifndef FALSE
#   define FALSE 0
#endif

typedef int bool_t;

int
imin (int x, int y)
{
    return x < y ? x : y;
}

/* Memory allocation */
void *
malloc_abc (unsigned int a, unsigned int b, unsigned int c)
{
    if (a >= INT32_MAX / b)
	return NULL;
    else if (a * b >= INT32_MAX / c)
	return NULL;
    else
	return malloc (a * b * c);
}

/* Random number generation */
typedef uint32_t xorwow_state_t[5];

uint32_t
xorwow_next (xorwow_state_t *state)
{
    uint32_t s = (*state)[0],
    t = (*state)[3];
    (*state)[3] = (*state)[2];
    (*state)[2] = (*state)[1];
    (*state)[1] = s;

    t ^= t >> 2;
    t ^= t << 1;
    t ^= s ^ (s << 4);

    (*state)[0] = t;
    (*state)[4] += 362437;

    return t + (*state)[4];
}

float
xorwow_float (xorwow_state_t *s)
{
    return (xorwow_next (s) >> 9) / (float)((1 << 23) - 1);
}

/* Floating point matrices
 *
 * Used to cache the cluster sizes.
 */
typedef struct matrix_t {
    int width;
    int height;
    float *buffer;
} matrix_t;

bool_t
matrix_init (matrix_t *matrix, int width, int height)
{
    float *buffer;

    if (!matrix)
	return FALSE;

    buffer = malloc_abc (width, height, sizeof (float));

    if (!buffer)
	return FALSE;

    matrix->buffer = buffer;
    matrix->width  = width;
    matrix->height = height;

    return TRUE;
}

bool_t
matrix_copy (matrix_t *dst, matrix_t const *src)
{
    float *srcbuf = src->buffer,
	  *srcend = src->buffer + src->width * src->height,
	  *dstbuf = dst->buffer;

    if (dst->width != src->width || dst->height != src->height)
	return FALSE;

    while (srcbuf < srcend)
	*dstbuf++ = *srcbuf++;

    return TRUE;
}

float *
matrix_get (matrix_t *matrix, int x, int y)
{
    return &matrix->buffer[y * matrix->width + x];
}

void
matrix_destroy (matrix_t *matrix)
{
    free (matrix->buffer);
}

/* Binary patterns */
typedef struct pattern_t {
    int width;
    int height;
    bool_t *buffer;
} pattern_t;

bool_t
pattern_init (pattern_t *pattern, int width, int height)
{
    bool_t *buffer;

    if (!pattern)
	return FALSE;

    buffer = malloc_abc (width, height, sizeof (bool_t));

    if (!buffer)
	return FALSE;

    pattern->buffer = buffer;
    pattern->width  = width;
    pattern->height = height;

    return TRUE;
}

bool_t
pattern_copy (pattern_t *dst, pattern_t const *src)
{
    bool_t *srcbuf = src->buffer,
	   *srcend = src->buffer + src->width * src->height,
	   *dstbuf = dst->buffer;

    if (dst->width != src->width || dst->height != src->height)
	return FALSE;

    while (srcbuf < srcend)
	*dstbuf++ = *srcbuf++;

    return TRUE;
}

bool_t *
pattern_get (pattern_t *pattern, int x, int y)
{
    return &pattern->buffer[y * pattern->width + x];
}

void
pattern_fill_white_noise (pattern_t *pattern, float fraction,
			  xorwow_state_t *s)
{
    bool_t *buffer = pattern->buffer;
    bool_t *end    = buffer + (pattern->width * pattern->height);

    while (buffer < end)
	*buffer++ = xorwow_float (s) < fraction;
}

void
pattern_destroy (pattern_t *pattern)
{
    free (pattern->buffer);
}

/* Dither arrays */
typedef struct array_t {
    int width;
    int height;
    uint32_t *buffer;
} array_t;

bool_t
array_init (array_t *array, int width, int height)
{
    uint32_t *buffer;

    if (!array)
	return FALSE;

    buffer = malloc_abc (width, height, sizeof (uint32_t));

    if (!buffer)
	return FALSE;

    array->buffer = buffer;
    array->width  = width;
    array->height = height;

    return TRUE;
}

uint32_t *
array_get (array_t *array, int x, int y)
{
    return &array->buffer[y * array->width + x];
}

bool_t
array_save_ppm (array_t *array, const char *filename)
{
    FILE *f = fopen(filename, "wb");

    int i   = 0;
    int bpp = 2;
    uint8_t buffer[1024];

    if (!f)
	return FALSE;

    if (array->width * array->height - 1 < 256)
	bpp = 1;

    fprintf(f, "P5 %d %d %d\n", array->width, array->height,
	    array->width * array->height - 1);
    while (i < array->width * array->height)
    {
	    int j = 0;
	    for (; j < 1024 / bpp && j < array->width * array->height; ++j)
	    {
		    uint32_t v = array->buffer[i + j];
		    if (bpp == 2)
		    {
			buffer[2 * j] = v & 0xff;
			buffer[2 * j + 1] = (v & 0xff00) >> 8;
		    } else {
			buffer[j] = v;
		    }
	    }

	    fwrite((void *)buffer, bpp, j, f);
	    i += j;
    }

    if (fclose(f) != 0)
	return FALSE;

    return TRUE;
}

bool_t
array_save (array_t *array, const char *filename)
{
    int x, y;
    FILE *f = fopen(filename, "wb");

    if (!f)
	return FALSE;

    fprintf (f, 
"/* WARNING: This file is generated by make-blue-noise.c\n"
" * Please edit that file instead of this one.\n"
" */\n"
"\n"
"#ifndef BLUE_NOISE_%dX%d_H\n"
"#define BLUE_NOISE_%dX%d_H\n"
"\n"
"#include <stdint.h>\n"
"\n", array->width, array->height, array->width, array->height);

    fprintf (f, "static const uint16_t dither_blue_noise_%dx%d[%d] = {\n",
	     array->width, array->height, array->width * array->height);

    for (y = 0; y < array->height; ++y)
    {
	fprintf (f, "    ");
	for (x = 0; x < array->width; ++x)
	{
	    if (x != 0)
		fprintf (f, ", ");

	    fprintf (f, "%d", *array_get (array, x, y));
	}

	fprintf (f, ",\n");
    }
    fprintf (f, "};\n");

    fprintf (f, "\n#endif /* BLUE_NOISE_%dX%d_H */\n",
	     array->width, array->height);

    if (fclose(f) != 0)
	return FALSE;

    return TRUE;
}

void
array_destroy (array_t *array)
{
    free (array->buffer);
}

/* Dither array generation */
bool_t
compute_cluster_sizes (pattern_t *pattern, matrix_t *matrix)
{
    int width  = pattern->width,
	height = pattern->height;

    if (matrix->width != width || matrix->height != height)
	return FALSE;

    int px, py, qx, qy, dx, dy;
    float tsqsi = 2.f * 1.5f * 1.5f;

#ifdef USE_OPENMP
#pragma omp parallel for default (none) \
    private (py, px, qy, qx, dx, dy) \
    shared (height, width, pattern, matrix, tsqsi)
#endif
    for (py = 0; py < height; ++py)
    {
	for (px = 0; px < width; ++px)
	{
	    bool_t pixel = *pattern_get (pattern, px, py);
	    float dist   = 0.f;

	    for (qx = 0; qx < width; ++qx)
	    {
		dx = imin (abs (qx - px), width - abs (qx - px));
		dx = dx * dx;

		for (qy = 0; qy < height; ++qy)
		{
		    dy = imin (abs (qy - py), height - abs (qy - py));
		    dy = dy * dy;

		    dist += (pixel == *pattern_get (pattern, qx, qy))
			* expf (- (dx + dy) / tsqsi);
		}
	    }

	    *matrix_get (matrix, px, py) = dist;
	}
    }

    return TRUE;
}

bool_t
swap_pixel (pattern_t *pattern, matrix_t *matrix, int x, int y)
{
    int width  = pattern->width,
	height = pattern->height;

    bool_t new;

    float f,
          dist  = 0.f,
	  tsqsi = 2.f * 1.5f * 1.5f;

    int px, py, dx, dy;
    bool_t b;

    new = !*pattern_get (pattern, x, y);
    *pattern_get (pattern, x, y) = new;

    if (matrix->width != width || matrix->height != height)
	return FALSE;


#ifdef USE_OPENMP
#pragma omp parallel for reduction (+:dist) default (none) \
    private (px, py, dx, dy, b, f) \
    shared (x, y, width, height, pattern, matrix, new, tsqsi)
#endif
    for (py = 0; py < height; ++py)
    {
	dy = imin (abs (py - y), height - abs (py - y));
	dy = dy * dy;

	for (px = 0; px < width; ++px)
	{
	    dx = imin (abs (px - x), width - abs (px - x));
	    dx = dx * dx;

	    b = (*pattern_get (pattern, px, py) == new);
	    f = expf (- (dx + dy) / tsqsi);
	    *matrix_get (matrix, px, py) += (2 * b - 1) * f;

	    dist += b * f;
	}
    }

    *matrix_get (matrix, x, y) = dist;
    return TRUE;
}

void
largest_cluster (pattern_t *pattern, matrix_t *matrix,
		 bool_t pixel, int *xmax, int *ymax)
{
    int width       = pattern->width,
	height      = pattern->height;

    int   x, y;

    float vmax = -INFINITY;

#ifdef USE_OPENMP
#pragma omp parallel default (none) \
    private (x, y) \
    shared (height, width, pattern, matrix, pixel, xmax, ymax, vmax)
#endif
    {
	int xbest = -1,
	    ybest = -1;

#ifdef USE_OPENMP
	float vbest = -INFINITY;

#pragma omp for reduction (max: vmax) collapse (2)
#endif
	for (y = 0; y < height; ++y)
	{
	    for (x = 0; x < width; ++x)
	    {
		if (*pattern_get (pattern, x, y) != pixel)
		    continue;

		if (*matrix_get (matrix, x, y) > vmax)
		{
		    vmax = *matrix_get (matrix, x, y);
#ifdef USE_OPENMP
		    vbest = vmax;
#endif
		    xbest = x;
		    ybest = y;
		}
	    }
	}

#ifdef USE_OPENMP
#pragma omp barrier
#pragma omp critical
	{
	    if (vmax == vbest)
	    {
		*xmax = xbest;
		*ymax = ybest;
	    }
	}
#else
	*xmax = xbest;
	*ymax = ybest;
#endif
    }

    assert (vmax > -INFINITY);
}

void
generate_initial_binary_pattern (pattern_t *pattern, matrix_t *matrix)
{
    int xcluster = 0,
	ycluster = 0,
	xvoid    = 0,
	yvoid    = 0;

    for (;;)
    {
	largest_cluster (pattern, matrix, TRUE, &xcluster, &ycluster);
	assert (*pattern_get (pattern, xcluster, ycluster) == TRUE);
	swap_pixel (pattern, matrix, xcluster, ycluster);

	largest_cluster (pattern, matrix, FALSE, &xvoid, &yvoid);
	assert (*pattern_get (pattern, xvoid, yvoid) == FALSE);
	swap_pixel (pattern, matrix, xvoid, yvoid);

	if (xcluster == xvoid && ycluster == yvoid)
	    return;
    }
}

bool_t
generate_dither_array (array_t *array,
		       pattern_t const *prototype, matrix_t const *matrix,
		       pattern_t *temp_pattern, matrix_t *temp_matrix)
{
    int width        = prototype->width,
	height       = prototype->height;

    int x, y, rank;

    int initial_rank = 0;

    if (array->width != width || array->height != height)
	return FALSE;

    // Make copies of the prototype and associated sizes matrix since we will
    // trash them
    if (!pattern_copy (temp_pattern, prototype))
	return FALSE;

    if (!matrix_copy (temp_matrix, matrix))
	return FALSE;

    // Compute initial rank
    for (y = 0; y < height; ++y)
    {
	for (x = 0; x < width; ++x)
	{
	    if (*pattern_get (temp_pattern, x, y))
		initial_rank += 1;

	    *array_get (array, x, y) = 0;
	}
    }

    // Phase 1
    for (rank = initial_rank; rank > 0; --rank)
    {
	largest_cluster (temp_pattern, temp_matrix, TRUE, &x, &y);
	swap_pixel (temp_pattern, temp_matrix, x, y);
	*array_get (array, x, y) = rank - 1;
    }

    // Make copies again for phases 2 & 3
    if (!pattern_copy (temp_pattern, prototype))
	return FALSE;

    if (!matrix_copy (temp_matrix, matrix))
	return FALSE;

    // Phase 2 & 3
    for (rank = initial_rank; rank < width * height; ++rank)
    {
	largest_cluster (temp_pattern, temp_matrix, FALSE, &x, &y);
	swap_pixel (temp_pattern, temp_matrix, x, y);
	*array_get (array, x, y) = rank;
    }

    return TRUE;
}

bool_t
generate (int size, xorwow_state_t *s,
	  char const *c_filename, char const *ppm_filename)
{
    bool_t ok = TRUE;

    pattern_t prototype, temp_pattern;
    array_t   array;
    matrix_t  matrix, temp_matrix;

    printf ("Generating %dx%d blue noise...\n", size, size);

    if (!pattern_init (&prototype, size, size))
	return FALSE;

    if (!pattern_init (&temp_pattern, size, size))
    {
	pattern_destroy (&prototype);
	return FALSE;
    }

    if (!matrix_init (&matrix, size, size))
    {
	pattern_destroy (&temp_pattern);
	pattern_destroy (&prototype);
	return FALSE;
    }

    if (!matrix_init (&temp_matrix, size, size))
    {
	matrix_destroy (&matrix);
	pattern_destroy (&temp_pattern);
	pattern_destroy (&prototype);
	return FALSE;
    }

    if (!array_init (&array, size, size))
    {
	matrix_destroy (&temp_matrix);
	matrix_destroy (&matrix);
	pattern_destroy (&temp_pattern);
	pattern_destroy (&prototype);
	return FALSE;
    }

    printf("Filling initial binary pattern with white noise...\n");
    pattern_fill_white_noise (&prototype, .1, s);

    printf("Initializing cluster sizes...\n");
    if (!compute_cluster_sizes (&prototype, &matrix))
    {
	fprintf (stderr, "Error while computing cluster sizes\n");
	ok = FALSE;
	goto out;
    }

    printf("Generating initial binary pattern...\n");
    generate_initial_binary_pattern (&prototype, &matrix);

    printf("Generating dither array...\n");
    if (!generate_dither_array (&array, &prototype, &matrix,
			 &temp_pattern, &temp_matrix))
    {
	fprintf (stderr, "Error while generating dither array\n");
	ok = FALSE;
	goto out;
    }

    printf("Saving dither array...\n");
    if (!array_save (&array, c_filename))
    {
	fprintf (stderr, "Error saving dither array\n");
	ok = FALSE;
	goto out;
    }

#if SAVE_PPM
    if (!array_save_ppm (&array, ppm_filename))
    {
	fprintf (stderr, "Error saving dither array PPM\n");
	ok = FALSE;
	goto out;
    }
#else
    (void)ppm_filename;
#endif

    printf("All done!\n");

out:
    array_destroy (&array);
    matrix_destroy (&temp_matrix);
    matrix_destroy (&matrix);
    pattern_destroy (&temp_pattern);
    pattern_destroy (&prototype);
    return ok;
}

int
main (void)
{
    xorwow_state_t s = {1185956906, 12385940, 983948, 349208051, 901842};

    if (!generate (64, &s, "blue-noise-64x64.h", "blue-noise-64x64.ppm"))
	return -1;

    return 0;
}
