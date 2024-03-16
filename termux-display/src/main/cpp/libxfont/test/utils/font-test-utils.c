/* Common utility code for interacting with libXfont from test utilities
 *
 * Note that this code is designed for test programs, and thus uses assert()
 * and fatal err() calls in places that real code would do error handling,
 * since the goal is to catch bugs faster, not help users get past problems.
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

/* Based on code from xorg-server/dix/dixfont.c covered by this notice:

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

 */

#include "font-test-utils.h"
#include "src/util/replace.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#ifdef HAVE_ERR_H
#include <err.h>
#endif
#include "src/util/replace.h"
#include <X11/X.h>

static unsigned long server_generation;
static xfont2_fpe_funcs_rec const **fpe_functions;
static int num_fpe_types;

static int
test_client_auth_generation(ClientPtr client)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static Bool
test_client_signal(ClientPtr client)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static void
test_delete_font_client_id(Font id)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static void _X_ATTRIBUTE_PRINTF(1,0)
test_verrorf(const char *f, va_list ap)
{
    vwarn(f, ap);
}

static FontPtr
test_find_old_font(FSID id)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static FontResolutionPtr
test_get_client_resolutions(int *num)
{
    *num = 0;
    return NULL;
}

static int
test_get_default_point_size(void)
{
    return 120;
}

static Font
test_get_new_font_client_id(void)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static uint32_t
test_get_time_in_millis(void)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static int
test_init_fs_handlers(FontPathElementPtr fpe,
		      FontBlockHandlerProcPtr block_handler)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

/* Callback from libXfont when each fpe handler is registered */
static int
test_register_fpe_funcs(const xfont2_fpe_funcs_rec *funcs)
{
    xfont2_fpe_funcs_rec const **new;

    /* grow the list */
    new = reallocarray(fpe_functions, (num_fpe_types + 1),
		       sizeof(xfont2_fpe_funcs_ptr));
    assert (new != NULL);
    fpe_functions = new;

    fpe_functions[num_fpe_types] = funcs;

    return num_fpe_types++;
}

static void
test_remove_fs_handlers(FontPathElementPtr fpe,
			FontBlockHandlerProcPtr block_handler, Bool all)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static void *
test_get_server_client(void)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static int
test_set_font_authorizations(char **authorizations, int *authlen, void *client)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static int
test_store_font_client_font(FontPtr pfont, Font id)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static Atom
test_make_atom(const char *string, unsigned len, int makeit)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static int
test_valid_atom(Atom atom)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static const char *
test_name_for_atom(Atom atom)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static unsigned long
test_get_server_generation(void)
{
    return server_generation;
}

static int
test_add_fs_fd(int fd, FontFdHandlerProcPtr handler, void *data)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static void
test_remove_fs_fd(int fd)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static void
test_adjust_fs_wait_for_delay(void *wt, unsigned long newdelay)
{
    err(BadImplementation, "%s called but not yet implemented", __func__);
}

static const xfont2_client_funcs_rec xfont2_client_funcs = {
    .version = XFONT2_CLIENT_FUNCS_VERSION,
    .client_auth_generation = test_client_auth_generation,
    .client_signal = test_client_signal,
    .delete_font_client_id = test_delete_font_client_id,
    .verrorf = test_verrorf,
    .find_old_font = test_find_old_font,
    .get_client_resolutions = test_get_client_resolutions,
    .get_default_point_size = test_get_default_point_size,
    .get_new_font_client_id = test_get_new_font_client_id,
    .get_time_in_millis = test_get_time_in_millis,
    .init_fs_handlers = test_init_fs_handlers,
    .register_fpe_funcs = test_register_fpe_funcs,
    .remove_fs_handlers = test_remove_fs_handlers,
    .get_server_client = test_get_server_client,
    .set_font_authorizations = test_set_font_authorizations,
    .store_font_client_font = test_store_font_client_font,
    .make_atom = test_make_atom,
    .valid_atom = test_valid_atom,
    .name_for_atom = test_name_for_atom,
    .get_server_generation = test_get_server_generation,
    .add_fs_fd = test_add_fs_fd,
    .remove_fs_fd = test_remove_fs_fd,
    .adjust_fs_wait_for_delay = test_adjust_fs_wait_for_delay,
};


xfont2_fpe_funcs_rec const **
init_font_handlers(int *fpe_function_count)
{
    server_generation++;
    xfont2_init(&xfont2_client_funcs);
    /* make sure our callbacks were called & worked */
    assert (fpe_functions != NULL);
    assert (num_fpe_types > 0);
    *fpe_function_count = num_fpe_types;
    return fpe_functions;
}


/* does the necessary magic to figure out the fpe type */
static int
DetermineFPEType(const char *pathname)
{
    int i;

    /* make sure init_font_handlers was called first */
    assert (num_fpe_types > 0);

    for (i = 0; i < num_fpe_types; i++) {
        if ((*fpe_functions[i]->name_check) (pathname))
            return i;
    }
    return -1;
}


static const char * const default_fpes[] = {
    "catalogue:/etc/X11/fontpath.d",
    "built-ins"
};
#define num_default_fpes  (sizeof(default_fpes) / sizeof(*default_fpes))

FontPathElementPtr *
init_font_paths(const char * const *font_paths, int *num_fpes)
{
    FontPathElementPtr *fpe_list;
    int i;

    /* make sure init_font_handlers was called first */
    assert (num_fpe_types > 0);

    /* Use default if caller didn't supply any */
    if (*num_fpes == 0) {
	font_paths = default_fpes;
	*num_fpes = num_default_fpes;
    }

    fpe_list = calloc(*num_fpes, sizeof(FontPathElementPtr));
    assert(fpe_list != NULL);

    for (i = 0; i < *num_fpes; i++) {
	int result;
	FontPathElementPtr fpe = calloc(1, sizeof(FontPathElementRec));
	assert(fpe != NULL);

	fpe->name = strdup(font_paths[i]);
	assert(fpe->name != NULL);
	fpe->name_length = strlen(fpe->name);
	assert(fpe->name_length > 0);
	/* If path is to fonts.dir file, trim it off and use the full
	   directory path instead.  Simplifies testing with afl. */
	if (fpe->name_length > (int) sizeof("/fonts.dir")) {
	    char *tail = fpe->name + fpe->name_length -
		(sizeof("/fonts.dir") - 1);

	    if (strcmp(tail, "/fonts.dir") == 0) {
		char *fullpath;

		*tail = '\0';
		fullpath = realpath(fpe->name, NULL);
		assert(fullpath != NULL);
		free(fpe->name);
		fpe->name = fullpath;
		fpe->name_length = strlen(fpe->name);
		assert(fpe->name_length > 0);
	    }
	}
	fpe->type = DetermineFPEType(fpe->name);
	if (fpe->type == -1)
	    err(BadFontPath, "Unable to find handler for font path %s",
		fpe->name);
	result = (*fpe_functions[fpe->type]->init_fpe) (fpe);
	if (result != Successful)
	    err(result, "init_fpe failed for font path %s: error %d",
		fpe->name, result);

	printf("Initialized font path element #%d: %s\n", i, fpe->name);
	fpe_list[i] = fpe;
    }
    printf("\n");

    return fpe_list;
}
