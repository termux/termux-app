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

#ifndef _GLAMOR_PROGRAM_H_
#define _GLAMOR_PROGRAM_H_

typedef enum {
    glamor_program_location_none = 0,
    glamor_program_location_fg = 1,
    glamor_program_location_bg = 2,
    glamor_program_location_fillsamp = 4,
    glamor_program_location_fillpos = 8,
    glamor_program_location_font = 16,
    glamor_program_location_bitplane = 32,
    glamor_program_location_dash = 64,
    glamor_program_location_atlas = 128,
} glamor_program_location;

typedef enum {
    glamor_program_flag_none = 0,
} glamor_program_flag;

typedef enum {
    glamor_program_alpha_normal,
    glamor_program_alpha_ca_first,
    glamor_program_alpha_ca_second,
    glamor_program_alpha_dual_blend,
    glamor_program_alpha_count
} glamor_program_alpha;

typedef struct _glamor_program glamor_program;

typedef Bool (*glamor_use) (PixmapPtr pixmap, GCPtr gc, glamor_program *prog, void *arg);

typedef Bool (*glamor_use_render) (CARD8 op, PicturePtr src, PicturePtr dst, glamor_program *prog);

typedef struct {
    const char                          *name;
    const int                           version;
    char                                *vs_defines;
    char                                *fs_defines;
    const char                          *vs_vars;
    const char                          *vs_exec;
    const char                          *fs_vars;
    const char                          *fs_exec;
    const glamor_program_location       locations;
    const glamor_program_flag           flags;
    const char                          *source_name;
    glamor_use                          use;
    glamor_use_render                   use_render;
} glamor_facet;

struct _glamor_program {
    GLint                       prog;
    GLint                       failed;
    GLint                       matrix_uniform;
    GLint                       fg_uniform;
    GLint                       bg_uniform;
    GLint                       fill_size_inv_uniform;
    GLint                       fill_offset_uniform;
    GLint                       font_uniform;
    GLint                       bitplane_uniform;
    GLint                       bitmul_uniform;
    GLint                       dash_uniform;
    GLint                       dash_length_uniform;
    GLint                       atlas_uniform;
    glamor_program_location     locations;
    glamor_program_flag         flags;
    glamor_use                  prim_use;
    glamor_use                  fill_use;
    glamor_program_alpha        alpha;
    glamor_use_render           prim_use_render;
    glamor_use_render           fill_use_render;
};

typedef struct {
    glamor_program      progs[4];
} glamor_program_fill;

extern const glamor_facet glamor_fill_solid;

Bool
glamor_build_program(ScreenPtr          screen,
                     glamor_program     *prog,
                     const glamor_facet *prim,
                     const glamor_facet *fill,
                     const char         *combine,
                     const char         *defines);

Bool
glamor_use_program(PixmapPtr            pixmap,
                   GCPtr                gc,
                   glamor_program       *prog,
                   void                 *arg);

glamor_program *
glamor_use_program_fill(PixmapPtr               pixmap,
                        GCPtr                   gc,
                        glamor_program_fill     *program_fill,
                        const glamor_facet      *prim);

typedef enum {
    glamor_program_source_solid,
    glamor_program_source_picture,
    glamor_program_source_1x1_picture,
    glamor_program_source_count,
} glamor_program_source;

typedef struct {
    glamor_program      progs[glamor_program_source_count][glamor_program_alpha_count];
} glamor_program_render;

static inline Bool
glamor_is_component_alpha(PicturePtr mask) {
    if (mask && mask->componentAlpha && PICT_FORMAT_RGB(mask->format))
        return TRUE;
    return FALSE;
}

glamor_program *
glamor_setup_program_render(CARD8                 op,
                            PicturePtr            src,
                            PicturePtr            mask,
                            PicturePtr            dst,
                            glamor_program_render *program_render,
                            const glamor_facet    *prim,
                            const char            *defines);

Bool
glamor_use_program_render(glamor_program        *prog,
                          CARD8                 op,
                          PicturePtr            src,
                          PicturePtr            dst);

#endif /* _GLAMOR_PROGRAM_H_ */
