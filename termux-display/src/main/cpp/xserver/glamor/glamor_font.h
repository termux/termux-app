/*
 * Copyright © 2014 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifndef _GLAMOR_FONT_H_
#define _GLAMOR_FONT_H_

typedef struct {
    Bool        realized;
    CharInfoPtr default_char;
    CARD8       default_row;
    CARD8       default_col;

    GLuint      texture_id;
    GLuint      row_width;
    CARD16      glyph_width_bytes;
    CARD16      glyph_width_pixels;
    CARD16      glyph_height;

} glamor_font_t;

glamor_font_t *
glamor_font_get(ScreenPtr screen, FontPtr font);

Bool
glamor_font_init(ScreenPtr screen);

void
glamor_fini_glyph_shader(ScreenPtr screen);

#endif /* _GLAMOR_FONT_H_ */
