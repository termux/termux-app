#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include <sys/types.h>
#include "pixman-private.h"

static const pixman_op_t op_list[] =
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
    PIXMAN_OP_SATURATE,
    PIXMAN_OP_DISJOINT_CLEAR,
    PIXMAN_OP_DISJOINT_SRC,
    PIXMAN_OP_DISJOINT_DST,
    PIXMAN_OP_DISJOINT_OVER,
    PIXMAN_OP_DISJOINT_OVER_REVERSE,
    PIXMAN_OP_DISJOINT_IN,
    PIXMAN_OP_DISJOINT_IN_REVERSE,
    PIXMAN_OP_DISJOINT_OUT,
    PIXMAN_OP_DISJOINT_OUT_REVERSE,
    PIXMAN_OP_DISJOINT_ATOP,
    PIXMAN_OP_DISJOINT_ATOP_REVERSE,
    PIXMAN_OP_DISJOINT_XOR,
    PIXMAN_OP_CONJOINT_CLEAR,
    PIXMAN_OP_CONJOINT_SRC,
    PIXMAN_OP_CONJOINT_DST,
    PIXMAN_OP_CONJOINT_OVER,
    PIXMAN_OP_CONJOINT_OVER_REVERSE,
    PIXMAN_OP_CONJOINT_IN,
    PIXMAN_OP_CONJOINT_IN_REVERSE,
    PIXMAN_OP_CONJOINT_OUT,
    PIXMAN_OP_CONJOINT_OUT_REVERSE,
    PIXMAN_OP_CONJOINT_ATOP,
    PIXMAN_OP_CONJOINT_ATOP_REVERSE,
    PIXMAN_OP_CONJOINT_XOR,
    PIXMAN_OP_MULTIPLY,
    PIXMAN_OP_SCREEN,
    PIXMAN_OP_OVERLAY,
    PIXMAN_OP_DARKEN,
    PIXMAN_OP_LIGHTEN,
    PIXMAN_OP_COLOR_DODGE,
    PIXMAN_OP_COLOR_BURN,
    PIXMAN_OP_HARD_LIGHT,
    PIXMAN_OP_DIFFERENCE,
    PIXMAN_OP_EXCLUSION,
    PIXMAN_OP_SOFT_LIGHT,
    PIXMAN_OP_HSL_HUE,
    PIXMAN_OP_HSL_SATURATION,
    PIXMAN_OP_HSL_COLOR,
    PIXMAN_OP_HSL_LUMINOSITY,
};

static float
rand_float (void)
{
    uint32_t u = prng_rand();

    return *(float *)&u;
}

static void
random_floats (argb_t *argb, int width)
{
    int i;

    for (i = 0; i < width; ++i)
    {
	argb_t *p = argb + i;

	p->a = rand_float();
	p->r = rand_float();
	p->g = rand_float();
	p->b = rand_float();
    }
}

#define WIDTH	512

static pixman_combine_float_func_t
lookup_combiner (pixman_implementation_t *imp, pixman_op_t op,
		 pixman_bool_t component_alpha)
{
    pixman_combine_float_func_t f;

    do
    {
	if (component_alpha)
	    f = imp->combine_float_ca[op];
	else
	    f = imp->combine_float[op];
	
	imp = imp->fallback;
    }
    while (!f);

    return f;
}

int
main ()
{
    pixman_implementation_t *impl;
    argb_t *src_bytes = malloc (WIDTH * sizeof (argb_t));
    argb_t *mask_bytes = malloc (WIDTH * sizeof (argb_t));
    argb_t *dest_bytes = malloc (WIDTH * sizeof (argb_t));
    int i;

    enable_divbyzero_exceptions();
    
    impl = _pixman_internal_only_get_implementation();
    
    prng_srand (0);

    for (i = 0; i < ARRAY_LENGTH (op_list); ++i)
    {
	pixman_op_t op = op_list[i];
	pixman_combine_float_func_t combiner;
	int ca;

	for (ca = 0; ca < 2; ++ca)
	{
	    combiner = lookup_combiner (impl, op, ca);

	    random_floats (src_bytes, WIDTH);
	    random_floats (mask_bytes, WIDTH);
	    random_floats (dest_bytes, WIDTH);

	    combiner (impl, op,
		      (float *)dest_bytes,
		      (float *)mask_bytes,
		      (float *)src_bytes,
		      WIDTH);
	}
    }	

    return 0;
}
