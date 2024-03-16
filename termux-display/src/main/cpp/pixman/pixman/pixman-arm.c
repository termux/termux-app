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

typedef enum
{
    ARM_V7		= (1 << 0),
    ARM_V6		= (1 << 1),
    ARM_VFP		= (1 << 2),
    ARM_NEON		= (1 << 3),
    ARM_IWMMXT		= (1 << 4)
} arm_cpu_features_t;

#if defined(USE_ARM_SIMD) || defined(USE_ARM_NEON) || defined(USE_ARM_IWMMXT)

#if defined(_MSC_VER)

/* Needed for EXCEPTION_ILLEGAL_INSTRUCTION */
#include <windows.h>

extern int pixman_msvc_try_arm_neon_op ();
extern int pixman_msvc_try_arm_simd_op ();

static arm_cpu_features_t
detect_cpu_features (void)
{
    arm_cpu_features_t features = 0;

    __try
    {
	pixman_msvc_try_arm_simd_op ();
	features |= ARM_V6;
    }
    __except (GetExceptionCode () == EXCEPTION_ILLEGAL_INSTRUCTION)
    {
    }

    __try
    {
	pixman_msvc_try_arm_neon_op ();
	features |= ARM_NEON;
    }
    __except (GetExceptionCode () == EXCEPTION_ILLEGAL_INSTRUCTION)
    {
    }

    return features;
}

#elif defined(__APPLE__) && defined(TARGET_OS_IPHONE) /* iOS */

#include "TargetConditionals.h"

static arm_cpu_features_t
detect_cpu_features (void)
{
    arm_cpu_features_t features = 0;

    features |= ARM_V6;

    /* Detection of ARM NEON on iOS is fairly simple because iOS binaries
     * contain separate executable images for each processor architecture.
     * So all we have to do is detect the armv7 architecture build. The
     * operating system automatically runs the armv7 binary for armv7 devices
     * and the armv6 binary for armv6 devices.
     */
#if defined(__ARM_NEON__)
    features |= ARM_NEON;
#endif

    return features;
}

#elif defined(__ANDROID__) || defined(ANDROID) /* Android */

static arm_cpu_features_t
detect_cpu_features (void)
{
    arm_cpu_features_t features = (ARM_V7 | ARM_VFP | ARM_NEON);
    return features;
}

#elif defined (__linux__) /* linux ELF */

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <elf.h>

static arm_cpu_features_t
detect_cpu_features (void)
{
    arm_cpu_features_t features = 0;
    Elf32_auxv_t aux;
    int fd;

    fd = open ("/proc/self/auxv", O_RDONLY);
    if (fd >= 0)
    {
	while (read (fd, &aux, sizeof(Elf32_auxv_t)) == sizeof(Elf32_auxv_t))
	{
	    if (aux.a_type == AT_HWCAP)
	    {
		uint32_t hwcap = aux.a_un.a_val;

		/* hardcode these values to avoid depending on specific
		 * versions of the hwcap header, e.g. HWCAP_NEON
		 */
		if ((hwcap & 64) != 0)
		    features |= ARM_VFP;
		if ((hwcap & 512) != 0)
		    features |= ARM_IWMMXT;
		/* this flag is only present on kernel 2.6.29 */
		if ((hwcap & 4096) != 0)
		    features |= ARM_NEON;
	    }
	    else if (aux.a_type == AT_PLATFORM)
	    {
		const char *plat = (const char*) aux.a_un.a_val;

		if (strncmp (plat, "v7l", 3) == 0)
		    features |= (ARM_V7 | ARM_V6);
		else if (strncmp (plat, "v6l", 3) == 0)
		    features |= ARM_V6;
	    }
	}
	close (fd);
    }

    return features;
}

#elif defined (_3DS) /* 3DS homebrew (devkitARM) */

static arm_cpu_features_t
detect_cpu_features (void)
{
    arm_cpu_features_t features = 0;

    features |= ARM_V6;

    return features;
}

#elif defined (PSP2) || defined (__SWITCH__)
/* Vita (VitaSDK) or Switch (devkitA64) homebrew */

static arm_cpu_features_t
detect_cpu_features (void)
{
    arm_cpu_features_t features = 0;

    features |= ARM_NEON;

    return features;
}

#else /* Unknown */

static arm_cpu_features_t
detect_cpu_features (void)
{
    return 0;
}

#endif /* Linux elf */

static pixman_bool_t
have_feature (arm_cpu_features_t feature)
{
    static pixman_bool_t initialized;
    static arm_cpu_features_t features;

    if (!initialized)
    {
	features = detect_cpu_features();
	initialized = TRUE;
    }

    return (features & feature) == feature;
}

#endif /* USE_ARM_SIMD || USE_ARM_NEON || USE_ARM_IWMMXT */

pixman_implementation_t *
_pixman_arm_get_implementations (pixman_implementation_t *imp)
{
#ifdef USE_ARM_SIMD
    if (!_pixman_disabled ("arm-simd") && have_feature (ARM_V6))
	imp = _pixman_implementation_create_arm_simd (imp);
#endif

#ifdef USE_ARM_IWMMXT
    if (!_pixman_disabled ("arm-iwmmxt") && have_feature (ARM_IWMMXT))
	imp = _pixman_implementation_create_mmx (imp);
#endif

#ifdef USE_ARM_NEON
    if (!_pixman_disabled ("arm-neon") && have_feature (ARM_NEON))
	imp = _pixman_implementation_create_arm_neon (imp);
#endif

#ifdef USE_ARM_A64_NEON
    /* neon is a part of aarch64 */
    if (!_pixman_disabled ("arm-neon"))
        imp = _pixman_implementation_create_arm_neon (imp);
#endif

    return imp;
}
