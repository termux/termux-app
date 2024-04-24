/* The gcc-provided loongson intrinsic functions are way too fucking broken
 * to be of any use, otherwise I'd use them.
 *
 * - The hardware instructions are very similar to MMX or iwMMXt. Certainly
 *   close enough that they could have implemented the _mm_*-style intrinsic
 *   interface and had a ton of optimized code available to them. Instead they
 *   implemented something much, much worse.
 *
 * - pshuf takes a dead first argument, causing extra instructions to be
 *   generated.
 *
 * - There are no 64-bit shift or logical intrinsics, which means you have
 *   to implement them with inline assembly, but this is a nightmare because
 *   gcc doesn't understand that the integer vector datatypes are actually in
 *   floating-point registers, so you end up with braindead code like
 *
 *	punpcklwd	$f9,$f9,$f5
 *	    dmtc1	v0,$f8
 *	punpcklwd	$f19,$f19,$f5
 *	    dmfc1	t9,$f9
 *	    dmtc1	v0,$f9
 *	    dmtc1	t9,$f20
 *	    dmfc1	s0,$f19
 *	punpcklbh	$f20,$f20,$f2
 *
 *   where crap just gets copied back and forth between integer and floating-
 *   point registers ad nauseum.
 *
 * Instead of trying to workaround the problems from these crap intrinsics, I
 * just implement the _mm_* intrinsics needed for pixman-mmx.c using inline
 * assembly.
 */

#include <stdint.h>

/* vectors are stored in 64-bit floating-point registers */
typedef double __m64;
/* having a 32-bit datatype allows us to use 32-bit loads in places like load8888 */
typedef float  __m32;

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_setzero_si64 (void)
{
	return 0.0;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_add_pi16 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("paddh %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_add_pi32 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("paddw %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_adds_pu16 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("paddush %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_adds_pu8 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("paddusb %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_and_si64 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("and %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_cmpeq_pi32 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("pcmpeqw %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline void __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_empty (void)
{

}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_madd_pi16 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("pmaddhw %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_mulhi_pu16 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("pmulhuh %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_mullo_pi16 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("pmullh %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_or_si64 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("or %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_packs_pu16 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("packushb %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_packs_pi32 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("packsswh %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

#define _MM_SHUFFLE(fp3,fp2,fp1,fp0) \
 (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | (fp0))
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_set_pi16 (uint16_t __w3, uint16_t __w2, uint16_t __w1, uint16_t __w0)
{
	if (__builtin_constant_p (__w3) &&
	    __builtin_constant_p (__w2) &&
	    __builtin_constant_p (__w1) &&
	    __builtin_constant_p (__w0))
	{
		uint64_t val = ((uint64_t)__w3 << 48)
			     | ((uint64_t)__w2 << 32)
			     | ((uint64_t)__w1 << 16)
			     | ((uint64_t)__w0 <<  0);
		return *(__m64 *)&val;
	}
	else if (__w3 == __w2 && __w2 == __w1 && __w1 == __w0)
	{
		/* TODO: handle other cases */
		uint64_t val = __w3;
		uint64_t imm = _MM_SHUFFLE (0, 0, 0, 0);
		__m64 ret;
		asm("pshufh %0, %1, %2\n\t"
		    : "=f" (ret)
		    : "f" (*(__m64 *)&val), "f" (*(__m64 *)&imm)
		);
		return ret;
	} else {
		uint64_t val = ((uint64_t)__w3 << 48)
			     | ((uint64_t)__w2 << 32)
			     | ((uint64_t)__w1 << 16)
			     | ((uint64_t)__w0 <<  0);
		return *(__m64 *)&val;
	}
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_set_pi32 (unsigned __i1, unsigned __i0)
{
	if (__builtin_constant_p (__i1) &&
	    __builtin_constant_p (__i0))
	{
		uint64_t val = ((uint64_t)__i1 << 32)
			     | ((uint64_t)__i0 <<  0);
		return *(__m64 *)&val;
	}
	else if (__i1 == __i0)
	{
		uint64_t imm = _MM_SHUFFLE (1, 0, 1, 0);
		__m64 ret;
		asm("pshufh %0, %1, %2\n\t"
		    : "=f" (ret)
		    : "f" (*(__m32 *)&__i1), "f" (*(__m64 *)&imm)
		);
		return ret;
	} else {
		uint64_t val = ((uint64_t)__i1 << 32)
			     | ((uint64_t)__i0 <<  0);
		return *(__m64 *)&val;
	}
}
#undef _MM_SHUFFLE

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_shuffle_pi16 (__m64 __m, int64_t __n)
{
	__m64 ret;
	asm("pshufh %0, %1, %2\n\t"
	    : "=f" (ret)
	    : "f" (__m), "f" (*(__m64 *)&__n)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_slli_pi16 (__m64 __m, int64_t __count)
{
	__m64 ret;
	asm("psllh  %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m), "f" (*(__m64 *)&__count)
	);
	return ret;
}
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_slli_si64 (__m64 __m, int64_t __count)
{
	__m64 ret;
	asm("dsll  %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m), "f" (*(__m64 *)&__count)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_srli_pi16 (__m64 __m, int64_t __count)
{
	__m64 ret;
	asm("psrlh %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m), "f" (*(__m64 *)&__count)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_srli_pi32 (__m64 __m, int64_t __count)
{
	__m64 ret;
	asm("psrlw %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m), "f" (*(__m64 *)&__count)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_srli_si64 (__m64 __m, int64_t __count)
{
	__m64 ret;
	asm("dsrl  %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m), "f" (*(__m64 *)&__count)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_sub_pi16 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("psubh %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_unpackhi_pi8 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("punpckhbh %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_unpackhi_pi16 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("punpckhhw %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_unpacklo_pi8 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("punpcklbh %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

/* Since punpcklbh doesn't care about the high 32-bits, we use the __m32 datatype which
 * allows load8888 to use 32-bit loads */
extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_unpacklo_pi8_f (__m32 __m1, __m64 __m2)
{
	__m64 ret;
	asm("punpcklbh %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_unpacklo_pi16 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("punpcklhw %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
_mm_xor_si64 (__m64 __m1, __m64 __m2)
{
	__m64 ret;
	asm("xor %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
loongson_extract_pi16 (__m64 __m, int64_t __pos)
{
	__m64 ret;
	asm("pextrh %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m), "f" (*(__m64 *)&__pos)
	);
	return ret;
}

extern __inline __m64 __attribute__((__gnu_inline__, __always_inline__, __artificial__))
loongson_insert_pi16 (__m64 __m1, __m64 __m2, int64_t __pos)
{
	__m64 ret;
	asm("pinsrh_%3 %0, %1, %2\n\t"
	   : "=f" (ret)
	   : "f" (__m1), "f" (__m2), "i" (__pos)
	);
	return ret;
}
