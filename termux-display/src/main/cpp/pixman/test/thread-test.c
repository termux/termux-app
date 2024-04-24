#include "utils.h"

#if !defined (HAVE_PTHREADS) && !defined (_WIN32)

int main ()
{
    printf ("Skipped thread-test - pthreads or Windows Threads not supported\n");
    return 0;
}

#else

#include <stdlib.h>

#ifdef HAVE_PTHREADS
# include <pthread.h>
#elif defined (_WIN32)
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#endif

#define THREADS 16

typedef struct
{
    int       thread_no;
    uint32_t *dst_buf;
    prng_t    prng_state;
#if defined (_WIN32) && !defined (HAVE_PTHREADS)
    uint32_t  crc32;
#endif
} info_t;

static const pixman_op_t operators[] = 
{
    PIXMAN_OP_SRC,
    PIXMAN_OP_OVER,
    PIXMAN_OP_ADD,
    PIXMAN_OP_CLEAR,
    PIXMAN_OP_SRC,
    PIXMAN_OP_DST,
    PIXMAN_OP_OVER,
    PIXMAN_OP_OVER_REVERSE,
    PIXMAN_OP_IN,
    PIXMAN_OP_IN_REVERSE,
    PIXMAN_OP_OUT,
    PIXMAN_OP_OUT_REVERSE,
    PIXMAN_OP_ATOP,
    PIXMAN_OP_ATOP_REVERSE,
    PIXMAN_OP_XOR,
    PIXMAN_OP_ADD,
    PIXMAN_OP_MULTIPLY,
    PIXMAN_OP_SCREEN,
    PIXMAN_OP_OVERLAY,
    PIXMAN_OP_DARKEN,
    PIXMAN_OP_LIGHTEN,
    PIXMAN_OP_HARD_LIGHT,
    PIXMAN_OP_DIFFERENCE,
    PIXMAN_OP_EXCLUSION,
};

static const pixman_format_code_t formats[] =
{
    PIXMAN_a8r8g8b8,
    PIXMAN_r5g6b5,
    PIXMAN_a8,
    PIXMAN_a4,
    PIXMAN_a1,
    PIXMAN_b5g6r5,
    PIXMAN_r8g8b8a8,
    PIXMAN_a4r4g4b4
};

#define N_ROUNDS 8192

#define RAND_ELT(arr)							\
    arr[prng_rand_r(&info->prng_state) % ARRAY_LENGTH (arr)]

#define DEST_WIDTH (7)

#ifdef HAVE_PTHREADS
static void *
thread (void *data)
#elif defined (_WIN32)
DWORD WINAPI
thread (LPVOID data)
#endif
{
    info_t *info = data;
    uint32_t crc32 = 0x0;
    uint32_t src_buf[64];
    pixman_image_t *dst_img, *src_img;
    int i;

    prng_srand_r (&info->prng_state, info->thread_no);

    for (i = 0; i < N_ROUNDS; ++i)
    {
	pixman_op_t op;
	int rand1, rand2;

	prng_randmemset_r (&info->prng_state, info->dst_buf,
			   DEST_WIDTH * sizeof (uint32_t), 0);
	prng_randmemset_r (&info->prng_state, src_buf,
			   sizeof (src_buf), 0);

	src_img = pixman_image_create_bits (
	    RAND_ELT (formats), 4, 4, src_buf, 16);
	dst_img = pixman_image_create_bits (
	    RAND_ELT (formats), DEST_WIDTH, 1, info->dst_buf,
	    DEST_WIDTH * sizeof (uint32_t));

	image_endian_swap (src_img);
	image_endian_swap (dst_img);
	
	rand2 = prng_rand_r (&info->prng_state) % 4;
	rand1 = prng_rand_r (&info->prng_state) % 4;
	op = RAND_ELT (operators);

	pixman_image_composite32 (
	    op,
	    src_img, NULL, dst_img,
	    rand1, rand2, 0, 0, 0, 0, DEST_WIDTH, 1);

	crc32 = compute_crc32_for_image (crc32, dst_img);

	pixman_image_unref (src_img);
	pixman_image_unref (dst_img);
    }

#ifdef HAVE_PTHREADS
    return (void *)(uintptr_t)crc32;
#elif defined (_WIN32)
    info->crc32 = crc32;
    return 0;
#endif
}

static inline uint32_t
byteswap32 (uint32_t x)
{
    return ((x & ((uint32_t)0xFF << 24)) >> 24) |
           ((x & ((uint32_t)0xFF << 16)) >>  8) |
           ((x & ((uint32_t)0xFF <<  8)) <<  8) |
           ((x & ((uint32_t)0xFF <<  0)) << 24);
}

int
main (void)
{
    uint32_t dest[THREADS * DEST_WIDTH];
    info_t info[THREADS] = { { 0 } };

#ifdef HAVE_PTHREADS
    pthread_t threads[THREADS];
    void *retvals[THREADS];
#elif defined (_WIN32)
    HANDLE  hThreadArray[THREADS];
    DWORD   dwThreadIdArray[THREADS];
#endif

    uint32_t crc32s[THREADS], crc32;
    int i;

    for (i = 0; i < THREADS; ++i)
    {
	info[i].thread_no = i;
	info[i].dst_buf = &dest[i * DEST_WIDTH];
    }

#ifdef HAVE_PTHREADS
    for (i = 0; i < THREADS; ++i)
      pthread_create (&threads[i], NULL, thread, &info[i]);

    for (i = 0; i < THREADS; ++i)
	  pthread_join (threads[i], &retvals[i]);

    for (i = 0; i < THREADS; ++i)
    {
	crc32s[i] = (uintptr_t)retvals[i];

	if (is_little_endian())
	    crc32s[i] = byteswap32 (crc32s[i]);
    }

#elif defined (_WIN32)
    for (i = 0; i < THREADS; ++i)
      {
        hThreadArray[i] = CreateThread(NULL,
                                       0,
                                       thread,
                                       &info[i],
                                       0,
                                       &dwThreadIdArray[i]);
        if (hThreadArray[i] == NULL)
          {
            printf ("Windows thread creation failed!\n");
            return 1;
          }
      }
    for (i = 0; i < THREADS; ++i)
      {
        WaitForSingleObject (hThreadArray[i], INFINITE);
        CloseHandle(hThreadArray[i]);
      }

    for (i = 0; i < THREADS; ++i)
      {
        crc32s[i] = info[i].crc32;

        if (is_little_endian())
          crc32s[i] = byteswap32 (crc32s[i]);
      }
#endif

    crc32 = compute_crc32 (0, crc32s, sizeof crc32s);

#define EXPECTED 0x82C4D9FB

    if (crc32 != EXPECTED)
    {
	printf ("thread-test failed. Got checksum 0x%08X, expected 0x%08X\n",
		crc32, EXPECTED);
	return 1;
    }

    return 0;
}

#endif

