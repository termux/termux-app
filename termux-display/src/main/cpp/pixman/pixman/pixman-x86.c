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
#include <pixman-config.h>
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

#if defined (__GNUC__)
#include <cpuid.h>
#endif

static void
pixman_cpuid (uint32_t feature,
	      uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
#if defined (__GNUC__)
    *a = *b = *c = *d = 0;
    __get_cpuid(feature, a, b, c, d);
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
