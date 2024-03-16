/*
 * Copyright © 2000 SuSE, Inc.
 * Copyright © 2007 Red Hat, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pixman-private.h"

#if defined(USE_X86_MMX) || defined (USE_SSE2) || defined (USE_SSSE3)

/* The CPU detection code needs to be in a file not compiled with
 * "-mmmx -msse", as gcc would generate CMOV instructions otherwise
 * that would lead to SIGILL instructions on old CPUs that don't have
 * it.
 */

typedef enum
{
    X86_MMX			= (1 << 0),
    X86_MMX_EXTENSIONS		= (1 << 1),
    X86_SSE			= (1 << 2) | X86_MMX_EXTENSIONS,
    X86_SSE2			= (1 << 3),
    X86_CMOV			= (1 << 4),
    X86_SSSE3			= (1 << 5)
} cpu_features_t;

#ifdef HAVE_GETISAX

#include <sys/auxv.h>

static cpu_features_t
detect_cpu_features (void)
{
    cpu_features_t features = 0;
    unsigned int result = 0;

    if (getisax (&result, 1))
    {
	if (result & AV_386_CMOV)
	    features |= X86_CMOV;
	if (result & AV_386_MMX)
	    features |= X86_MMX;
	if (result & AV_386_AMD_MMX)
	    features |= X86_MMX_EXTENSIONS;
	if (result & AV_386_SSE)
	    features |= X86_SSE;
	if (result & AV_386_SSE2)
	    features |= X86_SSE2;
	if (result & AV_386_SSSE3)
	    features |= X86_SSSE3;
    }

    return features;
}

#else

#define _PIXMAN_X86_64							\
    (defined(__amd64__) || defined(__x86_64__) || defined(_M_AMD64))

static pixman_bool_t
have_cpuid (void)
{
#if _PIXMAN_X86_64 || defined (_MSC_VER)

    return TRUE;

#elif defined (__GNUC__)
    uint32_t result;

    __asm__ volatile (
        "pushf"				"\n\t"
        "pop %%eax"			"\n\t"
        "mov %%eax, %%ecx"		"\n\t"
        "xor $0x00200000, %%eax"	"\n\t"
        "push %%eax"			"\n\t"
        "popf"				"\n\t"
        "pushf"				"\n\t"
        "pop %%eax"			"\n\t"
        "xor %%ecx, %%eax"		"\n\t"
	"mov %%eax, %0"			"\n\t"
	: "=r" (result)
	:
	: "%eax", "%ecx");

    return !!result;

#else
#error "Unknown compiler"
#endif
}

static void
pixman_cpuid (uint32_t feature,
	      uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
#if defined (__GNUC__)

#if _PIXMAN_X86_64
    __asm__ volatile (
        "cpuid"				"\n\t"
	: "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d)
	: "a" (feature));
#else
    /* On x86-32 we need to be careful about the handling of %ebx
     * and %esp. We can't declare either one as clobbered
     * since they are special registers (%ebx is the "PIC
     * register" holding an offset to global data, %esp the
     * stack pointer), so we need to make sure that %ebx is
     * preserved, and that %esp has its original value when
     * accessing the output operands.
     */
    __asm__ volatile (
	"xchg %%ebx, %1"		"\n\t"
	"cpuid"				"\n\t"
	"xchg %%ebx, %1"		"\n\t"
	: "=a" (*a), "=r" (*b), "=c" (*c), "=d" (*d)
	: "a" (feature));
#endif

#elif defined (_MSC_VER)
    int info[4];

    __cpuid (info, feature);

    *a = info[0];
    *b = info[1];
    *c = info[2];
    *d = info[3];
#else
#error Unknown compiler
#endif
}

static cpu_features_t
detect_cpu_features (void)
{
    uint32_t a, b, c, d;
    cpu_features_t features = 0;

    if (!have_cpuid())
	return features;

    /* Get feature bits */
    pixman_cpuid (0x01, &a, &b, &c, &d);
    if (d & (1 << 15))
	features |= X86_CMOV;
    if (d & (1 << 23))
	features |= X86_MMX;
    if (d & (1 << 25))
	features |= X86_SSE;
    if (d & (1 << 26))
	features |= X86_SSE2;
    if (c & (1 << 9))
	features |= X86_SSSE3;

    /* Check for AMD specific features */
    if ((features & X86_MMX) && !(features & X86_SSE))
    {
	char vendor[13];

	/* Get vendor string */
	memset (vendor, 0, sizeof vendor);

	pixman_cpuid (0x00, &a, &b, &c, &d);
	memcpy (vendor + 0, &b, 4);
	memcpy (vendor + 4, &d, 4);
	memcpy (vendor + 8, &c, 4);

	if (strcmp (vendor, "AuthenticAMD") == 0 ||
	    strcmp (vendor, "HygonGenuine") == 0 ||
	    strcmp (vendor, "Geode by NSC") == 0)
	{
	    pixman_cpuid (0x80000000, &a, &b, &c, &d);
	    if (a >= 0x80000001)
	    {
		pixman_cpuid (0x80000001, &a, &b, &c, &d);

		if (d & (1 << 22))
		    features |= X86_MMX_EXTENSIONS;
	    }
	}
    }

    return features;
}

#endif

static pixman_bool_t
have_feature (cpu_features_t feature)
{
    static pixman_bool_t initialized;
    static cpu_features_t features;

    if (!initialized)
    {
	features = detect_cpu_features();
	initialized = TRUE;
    }

    return (features & feature) == feature;
}

#endif

pixman_implementation_t *
_pixman_x86_get_implementations (pixman_implementation_t *imp)
{
#define MMX_BITS  (X86_MMX | X86_MMX_EXTENSIONS)
#define SSE2_BITS (X86_MMX | X86_MMX_EXTENSIONS | X86_SSE | X86_SSE2)
#define SSSE3_BITS (X86_SSE | X86_SSE2 | X86_SSSE3)

#ifdef USE_X86_MMX
    if (!_pixman_disabled ("mmx") && have_feature (MMX_BITS))
	imp = _pixman_implementation_create_mmx (imp);
#endif

#ifdef USE_SSE2
    if (!_pixman_disabled ("sse2") && have_feature (SSE2_BITS))
	imp = _pixman_implementation_create_sse2 (imp);
#endif

#ifdef USE_SSSE3
    if (!_pixman_disabled ("ssse3") && have_feature (SSSE3_BITS))
	imp = _pixman_implementation_create_ssse3 (imp);
#endif

    return imp;
}
