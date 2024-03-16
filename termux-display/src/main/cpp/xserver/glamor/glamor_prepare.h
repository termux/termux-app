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

#ifndef _GLAMOR_PREPARE_H_
#define _GLAMOR_PREPARE_H_

Bool
glamor_prepare_access(DrawablePtr drawable, glamor_access_t access);

Bool
glamor_prepare_access_box(DrawablePtr drawable, glamor_access_t access,
                         int x, int y, int w, int h);

void
glamor_finish_access(DrawablePtr drawable);

Bool
glamor_prepare_access_picture(PicturePtr picture, glamor_access_t access);

Bool
glamor_prepare_access_picture_box(PicturePtr picture, glamor_access_t access,
                        int x, int y, int w, int h);

void
glamor_finish_access_picture(PicturePtr picture);

Bool
glamor_prepare_access_gc(GCPtr gc);

void
glamor_finish_access_gc(GCPtr gc);

#endif /* _GLAMOR_PREPARE_H_ */
