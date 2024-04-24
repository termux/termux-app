/* Common utility code for interacting with libXfont from test utilities */

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <X11/Xfuncproto.h>
#include <X11/fonts/font.h>
#include <X11/fonts/fontstruct.h>
#include <X11/fonts/fontmisc.h>
#include <X11/fonts/libxfont2.h>

/* Returns pointer to array of functions for each type of font_path_entry
 * handler, and puts count of entries in that array into fpe_function_count.
 * Must be called before init_font_paths().
 */
extern xfont2_fpe_funcs_rec const **init_font_handlers(int *fpe_function_count);

/* Returns pointer to array of FontPathElement structs for each font path
 * entry passed in.  num_fpes must be set to the number of entries in the
 * font_paths array when called - will be set to the number of entries in
 * the returned array.   May be called with (NULL, 0) to use default font
 * path of "catalogue:/etc/X11/fontpath.d" & "built-ins".
 */
extern FontPathElementPtr *init_font_paths(const char * const *font_paths,
					   int *num_fpes);
