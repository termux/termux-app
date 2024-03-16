#define COMPONENT_SIZE 8
#define MASK 0xff
#define ONE_HALF 0x80

#define A_SHIFT 8 * 3
#define R_SHIFT 8 * 2
#define G_SHIFT 8
#define A_MASK 0xff000000
#define R_MASK 0xff0000
#define G_MASK 0xff00

#define RB_MASK 0xff00ff
#define AG_MASK 0xff00ff00
#define RB_ONE_HALF 0x800080
#define RB_MASK_PLUS_ONE 0x1000100

#define ALPHA_8(x) ((x) >> A_SHIFT)
#define RED_8(x) (((x) >> R_SHIFT) & MASK)
#define GREEN_8(x) (((x) >> G_SHIFT) & MASK)
#define BLUE_8(x) ((x) & MASK)

/*
 * ARMv6 has UQADD8 instruction, which implements unsigned saturated
 * addition for 8-bit values packed in 32-bit registers. It is very useful
 * for UN8x4_ADD_UN8x4, UN8_rb_ADD_UN8_rb and ADD_UN8 macros (which would
 * otherwise need a lot of arithmetic operations to simulate this operation).
 * Since most of the major ARM linux distros are built for ARMv7, we are
 * much less dependent on runtime CPU detection and can get practical
 * benefits from conditional compilation here for a lot of users.
 */

#if defined(USE_GCC_INLINE_ASM) && defined(__arm__) && \
    !defined(__aarch64__) && (!defined(__thumb__) || defined(__thumb2__))
#if defined(__ARM_ARCH_6__)   || defined(__ARM_ARCH_6J__)  || \
    defined(__ARM_ARCH_6K__)  || defined(__ARM_ARCH_6Z__)  || \
    defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6T2__) || \
    defined(__ARM_ARCH_6M__)  || defined(__ARM_ARCH_7__)   || \
    defined(__ARM_ARCH_7A__)  || defined(__ARM_ARCH_7R__)  || \
    defined(__ARM_ARCH_7M__)  || defined(__ARM_ARCH_7EM__)

static force_inline uint32_t
un8x4_add_un8x4 (uint32_t x, uint32_t y)
{
    uint32_t t;
    asm ("uqadd8 %0, %1, %2" : "=r" (t) : "%r" (x), "r" (y));
    return t;
}

#define UN8x4_ADD_UN8x4(x, y) \
    ((x) = un8x4_add_un8x4 ((x), (y)))

#define UN8_rb_ADD_UN8_rb(x, y, t) \
    ((t) = un8x4_add_un8x4 ((x), (y)), (x) = (t))

#define ADD_UN8(x, y, t) \
    ((t) = (x), un8x4_add_un8x4 ((t), (y)))

#endif
#endif

/*****************************************************************************/

/*
 * Helper macros.
 */

#define MUL_UN8(a, b, t)						\
    ((t) = (a) * (uint16_t)(b) + ONE_HALF, ((((t) >> G_SHIFT ) + (t) ) >> G_SHIFT ))

#define DIV_UN8(a, b)							\
    (((uint16_t) (a) * MASK + ((b) / 2)) / (b))

#ifndef ADD_UN8
#define ADD_UN8(x, y, t)				     \
    ((t) = (x) + (y),					     \
     (uint32_t) (uint8_t) ((t) | (0 - ((t) >> G_SHIFT))))
#endif

#define DIV_ONE_UN8(x)							\
    (((x) + ONE_HALF + (((x) + ONE_HALF) >> G_SHIFT)) >> G_SHIFT)

/*
 * The methods below use some tricks to be able to do two color
 * components at the same time.
 */

/*
 * x_rb = (x_rb * a) / 255
 */
#define UN8_rb_MUL_UN8(x, a, t)						\
    do									\
    {									\
	t  = ((x) & RB_MASK) * (a);					\
	t += RB_ONE_HALF;						\
	x = (t + ((t >> G_SHIFT) & RB_MASK)) >> G_SHIFT;		\
	x &= RB_MASK;							\
    } while (0)

/*
 * x_rb = min (x_rb + y_rb, 255)
 */
#ifndef UN8_rb_ADD_UN8_rb
#define UN8_rb_ADD_UN8_rb(x, y, t)					\
    do									\
    {									\
	t = ((x) + (y));						\
	t |= RB_MASK_PLUS_ONE - ((t >> G_SHIFT) & RB_MASK);		\
	x = (t & RB_MASK);						\
    } while (0)
#endif

/*
 * x_rb = (x_rb * a_rb) / 255
 */
#define UN8_rb_MUL_UN8_rb(x, a, t)					\
    do									\
    {									\
	t  = (x & MASK) * (a & MASK);					\
	t |= (x & R_MASK) * ((a >> R_SHIFT) & MASK);			\
	t += RB_ONE_HALF;						\
	t = (t + ((t >> G_SHIFT) & RB_MASK)) >> G_SHIFT;		\
	x = t & RB_MASK;						\
    } while (0)

/*
 * x_c = (x_c * a) / 255
 */
#define UN8x4_MUL_UN8(x, a)						\
    do									\
    {									\
	uint32_t r1__, r2__, t__;					\
									\
	r1__ = (x);							\
	UN8_rb_MUL_UN8 (r1__, (a), t__);				\
									\
	r2__ = (x) >> G_SHIFT;						\
	UN8_rb_MUL_UN8 (r2__, (a), t__);				\
									\
	(x) = r1__ | (r2__ << G_SHIFT);					\
    } while (0)

/*
 * x_c = (x_c * a) / 255 + y_c
 */
#define UN8x4_MUL_UN8_ADD_UN8x4(x, a, y)				\
    do									\
    {									\
	uint32_t r1__, r2__, r3__, t__;					\
									\
	r1__ = (x);							\
	r2__ = (y) & RB_MASK;						\
	UN8_rb_MUL_UN8 (r1__, (a), t__);				\
	UN8_rb_ADD_UN8_rb (r1__, r2__, t__);				\
									\
	r2__ = (x) >> G_SHIFT;						\
	r3__ = ((y) >> G_SHIFT) & RB_MASK;				\
	UN8_rb_MUL_UN8 (r2__, (a), t__);				\
	UN8_rb_ADD_UN8_rb (r2__, r3__, t__);				\
									\
	(x) = r1__ | (r2__ << G_SHIFT);					\
    } while (0)

/*
 * x_c = (x_c * a + y_c * b) / 255
 */
#define UN8x4_MUL_UN8_ADD_UN8x4_MUL_UN8(x, a, y, b)			\
    do									\
    {									\
	uint32_t r1__, r2__, r3__, t__;					\
									\
	r1__ = (x);							\
	r2__ = (y);							\
	UN8_rb_MUL_UN8 (r1__, (a), t__);				\
	UN8_rb_MUL_UN8 (r2__, (b), t__);				\
	UN8_rb_ADD_UN8_rb (r1__, r2__, t__);				\
									\
	r2__ = ((x) >> G_SHIFT);					\
	r3__ = ((y) >> G_SHIFT);					\
	UN8_rb_MUL_UN8 (r2__, (a), t__);				\
	UN8_rb_MUL_UN8 (r3__, (b), t__);				\
	UN8_rb_ADD_UN8_rb (r2__, r3__, t__);				\
									\
	(x) = r1__ | (r2__ << G_SHIFT);					\
    } while (0)

/*
 * x_c = (x_c * a_c) / 255
 */
#define UN8x4_MUL_UN8x4(x, a)						\
    do									\
    {									\
	uint32_t r1__, r2__, r3__, t__;					\
									\
	r1__ = (x);							\
	r2__ = (a);							\
	UN8_rb_MUL_UN8_rb (r1__, r2__, t__);				\
									\
	r2__ = (x) >> G_SHIFT;						\
	r3__ = (a) >> G_SHIFT;						\
	UN8_rb_MUL_UN8_rb (r2__, r3__, t__);				\
									\
	(x) = r1__ | (r2__ << G_SHIFT);					\
    } while (0)

/*
 * x_c = (x_c * a_c) / 255 + y_c
 */
#define UN8x4_MUL_UN8x4_ADD_UN8x4(x, a, y)				\
    do									\
    {									\
	uint32_t r1__, r2__, r3__, t__;					\
									\
	r1__ = (x);							\
	r2__ = (a);							\
	UN8_rb_MUL_UN8_rb (r1__, r2__, t__);				\
	r2__ = (y) & RB_MASK;						\
	UN8_rb_ADD_UN8_rb (r1__, r2__, t__);				\
									\
	r2__ = ((x) >> G_SHIFT);					\
	r3__ = ((a) >> G_SHIFT);					\
	UN8_rb_MUL_UN8_rb (r2__, r3__, t__);				\
	r3__ = ((y) >> G_SHIFT) & RB_MASK;				\
	UN8_rb_ADD_UN8_rb (r2__, r3__, t__);				\
									\
	(x) = r1__ | (r2__ << G_SHIFT);					\
    } while (0)

/*
 * x_c = (x_c * a_c + y_c * b) / 255
 */
#define UN8x4_MUL_UN8x4_ADD_UN8x4_MUL_UN8(x, a, y, b)			\
    do									\
    {									\
	uint32_t r1__, r2__, r3__, t__;					\
									\
	r1__ = (x);							\
	r2__ = (a);							\
	UN8_rb_MUL_UN8_rb (r1__, r2__, t__);				\
	r2__ = (y);							\
	UN8_rb_MUL_UN8 (r2__, (b), t__);				\
	UN8_rb_ADD_UN8_rb (r1__, r2__, t__);				\
									\
	r2__ = (x) >> G_SHIFT;						\
	r3__ = (a) >> G_SHIFT;						\
	UN8_rb_MUL_UN8_rb (r2__, r3__, t__);				\
	r3__ = (y) >> G_SHIFT;						\
	UN8_rb_MUL_UN8 (r3__, (b), t__);				\
	UN8_rb_ADD_UN8_rb (r2__, r3__, t__);				\
									\
	x = r1__ | (r2__ << G_SHIFT);					\
    } while (0)

/*
  x_c = min(x_c + y_c, 255)
*/
#ifndef UN8x4_ADD_UN8x4
#define UN8x4_ADD_UN8x4(x, y)						\
    do									\
    {									\
	uint32_t r1__, r2__, r3__, t__;					\
									\
	r1__ = (x) & RB_MASK;						\
	r2__ = (y) & RB_MASK;						\
	UN8_rb_ADD_UN8_rb (r1__, r2__, t__);				\
									\
	r2__ = ((x) >> G_SHIFT) & RB_MASK;				\
	r3__ = ((y) >> G_SHIFT) & RB_MASK;				\
	UN8_rb_ADD_UN8_rb (r2__, r3__, t__);				\
									\
	x = r1__ | (r2__ << G_SHIFT);					\
    } while (0)
#endif
