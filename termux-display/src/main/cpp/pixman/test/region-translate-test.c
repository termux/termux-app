#include <assert.h>
#include "utils.h"

/* Pixman had a bug where 32bit regions where clipped to 16bit sizes when
 * pixman_region32_translate() was called. This test exercises that bug.
 */

#define LARGE 32000

int
main (int argc, char **argv)
{
  pixman_box32_t rect = { -LARGE, -LARGE, LARGE, LARGE };
  pixman_region32_t r1, r2;

  pixman_region32_init_rects (&r1, &rect, 1);
  pixman_region32_init_rect (&r2, rect.x1, rect.y1, rect.x2 - rect.x1, rect.y2 - rect.y1);

  assert (pixman_region32_equal (&r1,  &r2));

  pixman_region32_translate (&r1, -LARGE, LARGE);
  pixman_region32_translate (&r1, LARGE, -LARGE);

  assert (pixman_region32_equal (&r1,  &r2));

  pixman_region32_fini (&r1);
  pixman_region32_fini (&r2);

  return 0;
}
