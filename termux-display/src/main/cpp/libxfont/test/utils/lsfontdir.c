/* lsfontdir [<font path entry> ...]
 *
 * Lists entries from fonts.dir file in given directory paths.
 * Defaults to "catalogue:/etc/X11/fontpath.d" & "built-ins" if no paths given.
 */

/*
 * Copyright (c) 2015, 2019, Oracle and/or its affiliates. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "font-test-utils.h"
#include <stdio.h>
#include <assert.h>
#ifdef HAVE_ERR_H
#include <err.h>
#endif
#include "src/util/replace.h"

int
main(int argc, char **argv)
{
    FontPathElementPtr *fpe_list;
    xfont2_fpe_funcs_rec const **fpe_functions;
    int fpe_function_count, fpe_list_count;
    int i, n;

    fpe_functions = init_font_handlers(&fpe_function_count);

    fpe_list_count = argc - 1;
    fpe_list = init_font_paths((const char **) argv + 1, &fpe_list_count);

    for (i = 0; i < fpe_list_count; i++) {
	FontPathElementPtr fpe = fpe_list[i];
	FontNamesPtr names;
	const int max_names_count = 8192;
	const char *pattern = "*";
	int result;

	/* Don't allocate max size up front to allow testing expansion code */
	names = xfont2_make_font_names_record(max_names_count / 16);
	assert(names != NULL);

	result = (*fpe_functions[fpe->type]->list_fonts)
	    (NULL, fpe, pattern, strlen(pattern), max_names_count, names);
	if (result != Successful)
	    err(result, "list_font failed for font path %s: error %d",
		fpe->name, result);

	printf("--- %s:\n", fpe->name);
	for (n = 0 ; n < names->nnames; n++) {
	    printf("%s\n", names->names[n]);
	}

	xfont2_free_font_names(names);
    }

    return 0;
}
