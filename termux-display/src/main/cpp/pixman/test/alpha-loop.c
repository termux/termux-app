#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#define WIDTH 400
#define HEIGHT 200

int
main (int argc, char **argv)
{
    pixman_image_t *a, *d, *s;
    uint8_t *alpha;
    uint32_t *src, *dest;

    prng_srand (0);

    alpha = make_random_bytes (WIDTH * HEIGHT);
    src = (uint32_t *)make_random_bytes (WIDTH * HEIGHT * 4);
    dest = (uint32_t *)make_random_bytes (WIDTH * HEIGHT * 4);

    a = pixman_image_create_bits (PIXMAN_a8, WIDTH, HEIGHT, (uint32_t *)alpha, WIDTH);
    d = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, dest, WIDTH * 4);
    s = pixman_image_create_bits (PIXMAN_a2r10g10b10, WIDTH, HEIGHT, src, WIDTH * 4);

    fail_after (5, "Infinite loop detected: 5 seconds without progress\n");

    pixman_image_set_alpha_map (s, a, 0, 0);
    pixman_image_set_alpha_map (a, s, 0, 0);

    pixman_image_composite (PIXMAN_OP_SRC, s, NULL, d, 0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);

    pixman_image_unref (s);

    return 0;
}
