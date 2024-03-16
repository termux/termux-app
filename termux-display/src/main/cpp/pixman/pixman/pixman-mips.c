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

#if defined(USE_MIPS_DSPR2) || defined(USE_LOONGSON_MMI)

#include <string.h>
#include <stdlib.h>

static pixman_bool_t
have_feature (const char *search_string)
{
#if defined (__linux__) /* linux ELF */
    /* Simple detection of MIPS features at runtime for Linux.
     * It is based on /proc/cpuinfo, which reveals hardware configuration
     * to user-space applications.  According to MIPS (early 2010), no similar
     * facility is universally available on the MIPS architectures, so it's up
     * to individual OSes to provide such.
     */
    const char *file_name = "/proc/cpuinfo";
    char cpuinfo_line[256];
    FILE *f = NULL;

    if ((f = fopen (file_name, "r")) == NULL)
        return FALSE;

    while (fgets (cpuinfo_line, sizeof (cpuinfo_line), f) != NULL)
    {
        if (strstr (cpuinfo_line, search_string) != NULL)
        {
            fclose (f);
            return TRUE;
        }
    }

    fclose (f);
#endif

    /* Did not find string in the proc file, or not Linux ELF. */
    return FALSE;
}

#endif

pixman_implementation_t *
_pixman_mips_get_implementations (pixman_implementation_t *imp)
{
#ifdef USE_LOONGSON_MMI
    /* I really don't know if some Loongson CPUs don't have MMI. */
    if (!_pixman_disabled ("loongson-mmi") && have_feature ("Loongson"))
	imp = _pixman_implementation_create_mmx (imp);
#endif

#ifdef USE_MIPS_DSPR2
    if (!_pixman_disabled ("mips-dspr2"))
    {
	int already_compiling_everything_for_dspr2 = 0;
#if defined(__mips_dsp) && (__mips_dsp_rev >= 2)
	already_compiling_everything_for_dspr2 = 1;
#endif
	if (already_compiling_everything_for_dspr2 ||
	    /* Only currently available MIPS core that supports DSPr2 is 74K. */
	    have_feature ("MIPS 74K"))
	{
	    imp = _pixman_implementation_create_mips_dspr2 (imp);
	}
    }
#endif

    return imp;
}
