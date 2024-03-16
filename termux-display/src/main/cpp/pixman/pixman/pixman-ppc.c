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

#ifdef USE_VMX

/* The CPU detection code needs to be in a file not compiled with
 * "-maltivec -mabi=altivec", as gcc would try to save vector register
 * across function calls causing SIGILL on cpus without Altivec/vmx.
 */
#ifdef __APPLE__
#include <sys/sysctl.h>

static pixman_bool_t
pixman_have_vmx (void)
{
    int error, have_vmx;
    size_t length = sizeof(have_vmx);

    error = sysctlbyname ("hw.optional.altivec", &have_vmx, &length, NULL, 0);

    if (error)
	return FALSE;

    return have_vmx;
}

#elif defined (__OpenBSD__)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>

static pixman_bool_t
pixman_have_vmx (void)
{
    int error, have_vmx;
    int mib[2] = { CTL_MACHDEP, CPU_ALTIVEC };
    size_t length = sizeof(have_vmx);

    error = sysctl (mib, 2, &have_vmx, &length, NULL, 0);

    if (error != 0)
	return FALSE;

    return have_vmx;
}

#elif defined (__FreeBSD__)
#include <machine/cpu.h>
#include <sys/auxv.h>

static pixman_bool_t
pixman_have_vmx (void)
{

    unsigned long cpufeatures;
    int have_vmx;

    if (elf_aux_info(AT_HWCAP, &cpufeatures, sizeof(cpufeatures)))
    return FALSE;

    have_vmx = cpufeatures & PPC_FEATURE_HAS_ALTIVEC;
    return have_vmx;
}

#elif defined (__linux__)

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <linux/auxvec.h>
#include <asm/cputable.h>

static pixman_bool_t
pixman_have_vmx (void)
{
    int have_vmx = FALSE;
    int fd;
    struct
    {
	unsigned long type;
	unsigned long value;
    } aux;

    fd = open ("/proc/self/auxv", O_RDONLY);
    if (fd >= 0)
    {
	while (read (fd, &aux, sizeof (aux)) == sizeof (aux))
	{
	    if (aux.type == AT_HWCAP && (aux.value & PPC_FEATURE_HAS_ALTIVEC))
	    {
		have_vmx = TRUE;
		break;
	    }
	}

	close (fd);
    }

    return have_vmx;
}

#else /* !__APPLE__ && !__OpenBSD__ && !__linux__ */
#include <signal.h>
#include <setjmp.h>

static jmp_buf jump_env;

static void
vmx_test (int        sig,
	  siginfo_t *si,
	  void *     unused)
{
    longjmp (jump_env, 1);
}

static pixman_bool_t
pixman_have_vmx (void)
{
    struct sigaction sa, osa;
    int jmp_result;

    sa.sa_flags = SA_SIGINFO;
    sigemptyset (&sa.sa_mask);
    sa.sa_sigaction = vmx_test;
    sigaction (SIGILL, &sa, &osa);
    jmp_result = setjmp (jump_env);
    if (jmp_result == 0)
    {
	asm volatile ( "vor 0, 0, 0" );
    }
    sigaction (SIGILL, &osa, NULL);
    return (jmp_result == 0);
}

#endif /* __APPLE__ */
#endif /* USE_VMX */

pixman_implementation_t *
_pixman_ppc_get_implementations (pixman_implementation_t *imp)
{
#ifdef USE_VMX
    if (!_pixman_disabled ("vmx") && pixman_have_vmx ())
	imp = _pixman_implementation_create_vmx (imp);
#endif

    return imp;
}
