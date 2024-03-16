/*
 * Copyright © 2008 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */
#ifndef GLAMOR_PRIV_H
#define GLAMOR_PRIV_H

#include "dix-config.h"

#include "glamor.h"
#include "xvdix.h"

#if XSYNC
#include "misyncshm.h"
#include "misyncstr.h"
#endif

#include <epoxy/gl.h>
#ifdef GLAMOR_HAS_GBM
#define MESA_EGL_NO_X11_HEADERS
#define EGL_NO_X11
#include <epoxy/egl.h>
#endif

#define GLAMOR_DEFAULT_PRECISION  \
    "#ifdef GL_ES\n"              \
    "precision mediump float;\n"  \
    "#endif\n"

#include "glyphstr.h"

#include "glamor_debug.h"
#include "glamor_context.h"
#include "glamor_program.h"

#include <list.h>

struct glamor_pixmap_private;

typedef struct glamor_composite_shader {
    GLuint prog;
    GLint dest_to_dest_uniform_location;
    GLint dest_to_source_uniform_location;
    GLint dest_to_mask_uniform_location;
    GLint source_uniform_location;
    GLint mask_uniform_location;
    GLint source_wh;
    GLint mask_wh;
    GLint source_repeat_mode;
    GLint mask_repeat_mode;
    union {
        float source_solid_color[4];
        struct {
            PixmapPtr source_pixmap;
            PicturePtr source;
        };
    };

    union {
        float mask_solid_color[4];
        struct {
            PixmapPtr mask_pixmap;
            PicturePtr mask;
        };
    };
} glamor_composite_shader;

enum ca_state {
    CA_NONE,
    CA_TWO_PASS,
    CA_DUAL_BLEND,
};

enum shader_source {
    SHADER_SOURCE_SOLID,
    SHADER_SOURCE_TEXTURE,
    SHADER_SOURCE_TEXTURE_ALPHA,
    SHADER_SOURCE_COUNT,
};

enum shader_mask {
    SHADER_MASK_NONE,
    SHADER_MASK_SOLID,
    SHADER_MASK_TEXTURE,
    SHADER_MASK_TEXTURE_ALPHA,
    SHADER_MASK_COUNT,
};

enum shader_dest_swizzle {
    SHADER_DEST_SWIZZLE_DEFAULT,
    SHADER_DEST_SWIZZLE_ALPHA_TO_RED,
    SHADER_DEST_SWIZZLE_COUNT,
};

struct shader_key {
    enum shader_source source;
    enum shader_mask mask;
    glamor_program_alpha in;
    enum shader_dest_swizzle dest_swizzle;
};

struct blendinfo {
    Bool dest_alpha;
    Bool source_alpha;
    GLenum source_blend;
    GLenum dest_blend;
};

typedef struct {
    INT16 x_src;
    INT16 y_src;
    INT16 x_mask;
    INT16 y_mask;
    INT16 x_dst;
    INT16 y_dst;
    INT16 width;
    INT16 height;
} glamor_composite_rect_t;

enum glamor_vertex_type {
    GLAMOR_VERTEX_POS,
    GLAMOR_VERTEX_SOURCE,
    GLAMOR_VERTEX_MASK
};

enum gradient_shader {
    SHADER_GRADIENT_LINEAR,
    SHADER_GRADIENT_RADIAL,
    SHADER_GRADIENT_CONICAL,
    SHADER_GRADIENT_COUNT,
};

struct glamor_screen_private;
struct glamor_pixmap_private;

#define GLAMOR_COMPOSITE_VBO_VERT_CNT (64*1024)

struct glamor_format {
    /** X Server's "depth" value */
    int depth;
    /** GL internalformat for creating textures of this type */
    GLenum internalformat;
    /** GL format transferring pixels in/out of textures of this type. */
    GLenum format;
    /** GL type transferring pixels in/out of textures of this type. */
    GLenum type;
    /* Render PICT_* matching GL's channel layout for pixels
     * transferred using format/type.
     */
    CARD32 render_format;
    /**
     * Whether rendering is supported in GL at all (i.e. without pixel data conversion
     * just before upload)
     */
    Bool rendering_supported;
};

struct glamor_saved_procs {
    CloseScreenProcPtr close_screen;
    CreateGCProcPtr create_gc;
    CreatePixmapProcPtr create_pixmap;
    DestroyPixmapProcPtr destroy_pixmap;
    GetSpansProcPtr get_spans;
    GetImageProcPtr get_image;
    CompositeProcPtr composite;
    CompositeRectsProcPtr composite_rects;
    TrapezoidsProcPtr trapezoids;
    GlyphsProcPtr glyphs;
    ChangeWindowAttributesProcPtr change_window_attributes;
    CopyWindowProcPtr copy_window;
    BitmapToRegionProcPtr bitmap_to_region;
    TrianglesProcPtr triangles;
    AddTrapsProcPtr addtraps;
#if XSYNC
    SyncScreenFuncsRec sync_screen_funcs;
#endif
    ScreenBlockHandlerProcPtr block_handler;
};

typedef struct glamor_screen_private {
    Bool is_gles;
    int glsl_version;
    Bool has_pack_invert;
    Bool has_fbo_blit;
    Bool has_map_buffer_range;
    Bool has_buffer_storage;
    Bool has_khr_debug;
    Bool has_mesa_tile_raster_order;
    Bool has_nv_texture_barrier;
    Bool has_pack_subimage;
    Bool has_unpack_subimage;
    Bool has_rw_pbo;
    Bool use_quads;
    Bool has_dual_blend;
    Bool has_clear_texture;
    Bool has_texture_swizzle;
    Bool is_core_profile;
    Bool can_copyplane;
    Bool use_gpu_shader4;
    int max_fbo_size;

    /**
     * Stores information about supported formats. Note, that this list contains all
     * supported pixel formats, including these that are not supported on GL side
     * directly, but are converted to another format instead.
     */
    struct glamor_format formats[33];
    struct glamor_format cbcr_format;

    /* glamor point shader */
    glamor_program point_prog;

    /* glamor spans shaders */
    glamor_program_fill fill_spans_program;

    /* glamor rect shaders */
    glamor_program_fill poly_fill_rect_program;

    /* glamor glyphblt shaders */
    glamor_program_fill poly_glyph_blt_progs;

    /* glamor text shaders */
    glamor_program_fill poly_text_progs;
    glamor_program      te_text_prog;
    glamor_program      image_text_prog;

    /* glamor copy shaders */
    glamor_program      copy_area_prog;
    glamor_program      copy_plane_prog;

    /* glamor line shader */
    glamor_program_fill poly_line_program;

    /* glamor segment shaders */
    glamor_program_fill poly_segment_program;

    /*  glamor dash line shader */
    glamor_program_fill on_off_dash_line_progs;
    glamor_program      double_dash_line_prog;

    /* glamor composite_glyphs shaders */
    glamor_program_render       glyphs_program;
    struct glamor_glyph_atlas   *glyph_atlas_a;
    struct glamor_glyph_atlas   *glyph_atlas_argb;
    int                         glyph_atlas_dim;
    int                         glyph_max_dim;
    char                        *glyph_defines;

    /** Vertex buffer for all GPU rendering. */
    GLuint vao;
    GLuint vbo;
    /** Next offset within the VBO that glamor_get_vbo_space() will use. */
    int vbo_offset;
    int vbo_size;
    Bool vbo_mapped;
    /**
     * Pointer to glamor_get_vbo_space()'s current VBO mapping.
     *
     * Note that this is not necessarily equal to the pointer returned
     * by glamor_get_vbo_space(), so it can't be used in place of that.
     */
    char *vb;
    int vb_stride;

    /** Cached index buffer for translating GL_QUADS to triangles. */
    GLuint ib;
    /** Index buffer type: GL_UNSIGNED_SHORT or GL_UNSIGNED_INT */
    GLenum ib_type;
    /** Number of quads the index buffer has indices for. */
    unsigned ib_size;

    Bool has_source_coords, has_mask_coords;
    int render_nr_quads;
    glamor_composite_shader composite_shader[SHADER_SOURCE_COUNT]
        [SHADER_MASK_COUNT]
        [glamor_program_alpha_count]
        [SHADER_DEST_SWIZZLE_COUNT];

    /* glamor gradient, 0 for small nstops, 1 for
       large nstops and 2 for dynamic generate. */
    GLint gradient_prog[SHADER_GRADIENT_COUNT][3];
    int linear_max_nstops;
    int radial_max_nstops;

    struct glamor_saved_procs saved_procs;
    GetDrawableModifiersFuncPtr get_drawable_modifiers;
    int flags;
    ScreenPtr screen;
    int dri3_enabled;

    Bool suppress_gl_out_of_memory_logging;
    Bool logged_any_fbo_allocation_failure;
    Bool logged_any_pbo_allocation_failure;

    /* xv */
    glamor_program xv_prog;

    struct glamor_context ctx;
} glamor_screen_private;

typedef enum glamor_access {
    GLAMOR_ACCESS_RO,
    GLAMOR_ACCESS_RW,
} glamor_access_t;

enum glamor_fbo_state {
    /** There is no storage attached to the pixmap. */
    GLAMOR_FBO_UNATTACHED,
    /**
     * The pixmap has FBO storage attached, but devPrivate.ptr doesn't
     * point at anything.
     */
    GLAMOR_FBO_NORMAL,
};

typedef struct glamor_pixmap_fbo {
    GLuint tex; /**< GL texture name */
    GLuint fb; /**< GL FBO name */
    int width; /**< width in pixels */
    int height; /**< height in pixels */
    Bool is_red;
} glamor_pixmap_fbo;

typedef struct glamor_pixmap_clipped_regions {
    int block_idx;
    RegionPtr region;
} glamor_pixmap_clipped_regions;

typedef struct glamor_pixmap_private {
    glamor_pixmap_type_t type;
    enum glamor_fbo_state gl_fbo;
    /**
     * If devPrivate.ptr is non-NULL (meaning we're within
     * glamor_prepare_access), determies whether we should re-upload
     * that data on glamor_finish_access().
     */
    glamor_access_t map_access;
    glamor_pixmap_fbo *fbo;
    /** current fbo's coords in the whole pixmap. */
    BoxRec box;
    GLuint pbo;
    RegionRec prepare_region;
    Bool prepared;
#ifdef GLAMOR_HAS_GBM
    EGLImageKHR image;
    Bool used_modifiers;
#endif
    /** block width of this large pixmap. */
    int block_w;
    /** block height of this large pixmap. */
    int block_h;

    /** block_wcnt: block count in one block row. */
    int block_wcnt;
    /** block_hcnt: block count in one block column. */
    int block_hcnt;

    /**
     * The list of boxes for the bounds of the FBOs making up the
     * pixmap.
     *
     * For a 2048x2048 pixmap with GL FBO size limits of 1024x1024:
     *
     * ******************
     * *  fbo0 * fbo1   *
     * *       *        *
     * ******************
     * *  fbo2 * fbo3   *
     * *       *        *
     * ******************
     *
     * box[0] = {0,0,1024,1024}
     * box[1] = {1024,0,2048,2048}
     * ...
     */
    BoxPtr box_array;

    /**
     * Array of fbo structs containing the actual GL texture/fbo
     * names.
     */
    glamor_pixmap_fbo **fbo_array;

    Bool is_cbcr;
} glamor_pixmap_private;

extern DevPrivateKeyRec glamor_pixmap_private_key;

static inline glamor_pixmap_private *
glamor_get_pixmap_private(PixmapPtr pixmap)
{
    if (pixmap == NULL)
        return NULL;

    return dixLookupPrivate(&pixmap->devPrivates, &glamor_pixmap_private_key);
}

/*
 * Returns TRUE if pixmap has no image object
 */
static inline Bool
glamor_pixmap_drm_only(PixmapPtr pixmap)
{
    glamor_pixmap_private *priv = glamor_get_pixmap_private(pixmap);

    return priv->type == GLAMOR_DRM_ONLY;
}

/*
 * Returns TRUE if pixmap is plain memory (not a GL object at all)
 */
static inline Bool
glamor_pixmap_is_memory(PixmapPtr pixmap)
{
    glamor_pixmap_private *priv = glamor_get_pixmap_private(pixmap);

    return priv->type == GLAMOR_MEMORY;
}

/*
 * Returns TRUE if pixmap requires multiple textures to hold it
 */
static inline Bool
glamor_pixmap_priv_is_large(glamor_pixmap_private *priv)
{
    return priv->block_wcnt > 1 || priv->block_hcnt > 1;
}

static inline Bool
glamor_pixmap_priv_is_small(glamor_pixmap_private *priv)
{
    return priv->block_wcnt <= 1 && priv->block_hcnt <= 1;
}

static inline Bool
glamor_pixmap_is_large(PixmapPtr pixmap)
{
    glamor_pixmap_private *priv = glamor_get_pixmap_private(pixmap);

    return glamor_pixmap_priv_is_large(priv);
}
/*
 * Returns TRUE if pixmap has an FBO
 */
static inline Bool
glamor_pixmap_has_fbo(PixmapPtr pixmap)
{
    glamor_pixmap_private *priv = glamor_get_pixmap_private(pixmap);

    return priv->gl_fbo == GLAMOR_FBO_NORMAL;
}

static inline void
glamor_set_pixmap_fbo_current(glamor_pixmap_private *priv, int idx)
{
    if (glamor_pixmap_priv_is_large(priv)) {
        priv->fbo = priv->fbo_array[idx];
        priv->box = priv->box_array[idx];
    }
}

static inline glamor_pixmap_fbo *
glamor_pixmap_fbo_at(glamor_pixmap_private *priv, int box)
{
    assert(box < priv->block_wcnt * priv->block_hcnt);
    return priv->fbo_array[box];
}

static inline BoxPtr
glamor_pixmap_box_at(glamor_pixmap_private *priv, int box)
{
    assert(box < priv->block_wcnt * priv->block_hcnt);
    return &priv->box_array[box];
}

static inline int
glamor_pixmap_wcnt(glamor_pixmap_private *priv)
{
    return priv->block_wcnt;
}

static inline int
glamor_pixmap_hcnt(glamor_pixmap_private *priv)
{
    return priv->block_hcnt;
}

#define glamor_pixmap_loop(priv, box_index)                            \
    for (box_index = 0; box_index < glamor_pixmap_hcnt(priv) *         \
             glamor_pixmap_wcnt(priv); box_index++)                    \

/* GC private structure. Currently holds only any computed dash pixmap */

typedef struct {
    PixmapPtr   dash;
    PixmapPtr   stipple;
    DamagePtr   stipple_damage;
} glamor_gc_private;

extern DevPrivateKeyRec glamor_gc_private_key;
extern DevPrivateKeyRec glamor_screen_private_key;

extern glamor_screen_private *
glamor_get_screen_private(ScreenPtr screen);

extern void
glamor_set_screen_private(ScreenPtr screen, glamor_screen_private *priv);

static inline glamor_gc_private *
glamor_get_gc_private(GCPtr gc)
{
    return dixLookupPrivate(&gc->devPrivates, &glamor_gc_private_key);
}

/**
 * Returns TRUE if the given planemask covers all the significant bits in the
 * pixel values for pDrawable.
 */
static inline Bool
glamor_pm_is_solid(int depth, unsigned long planemask)
{
    return (planemask & FbFullMask(depth)) ==
        FbFullMask(depth);
}

extern int glamor_debug_level;

/* glamor.c */
PixmapPtr glamor_get_drawable_pixmap(DrawablePtr drawable);

glamor_pixmap_fbo *glamor_pixmap_detach_fbo(glamor_pixmap_private *
                                            pixmap_priv);
void glamor_pixmap_attach_fbo(PixmapPtr pixmap, glamor_pixmap_fbo *fbo);
glamor_pixmap_fbo *glamor_create_fbo_from_tex(glamor_screen_private *
                                              glamor_priv, PixmapPtr pixmap,
                                              int w, int h, GLint tex,
                                              int flag);
glamor_pixmap_fbo *glamor_create_fbo(glamor_screen_private *glamor_priv,
                                     PixmapPtr pixmap, int w, int h, int flag);
void glamor_destroy_fbo(glamor_screen_private *glamor_priv,
                        glamor_pixmap_fbo *fbo);
void glamor_pixmap_destroy_fbo(PixmapPtr pixmap);
Bool glamor_pixmap_fbo_fixup(ScreenPtr screen, PixmapPtr pixmap);
void glamor_pixmap_clear_fbo(glamor_screen_private *glamor_priv, glamor_pixmap_fbo *fbo,
                             const struct glamor_format *pixmap_format);

const struct glamor_format *glamor_format_for_pixmap(PixmapPtr pixmap);

/* Return whether 'picture' is alpha-only */
static inline Bool glamor_picture_is_alpha(PicturePtr picture)
{
    return picture->format == PICT_a1 || picture->format == PICT_a8;
}

/* Return whether 'picture' is storing alpha bits in the red channel */
static inline Bool
glamor_picture_red_is_alpha(PicturePtr picture)
{
    /* True when the picture is alpha only and the screen is using GL_RED for alpha pictures */
    return glamor_picture_is_alpha(picture) &&
        glamor_get_screen_private(picture->pDrawable->pScreen)->formats[8].format == GL_RED;
}

void glamor_bind_texture(glamor_screen_private *glamor_priv,
                         GLenum texture,
                         glamor_pixmap_fbo *fbo,
                         Bool destination_red);

glamor_pixmap_fbo *glamor_create_fbo_array(glamor_screen_private *glamor_priv,
                                           PixmapPtr pixmap,
                                           int flag, int block_w, int block_h,
                                           glamor_pixmap_private *);

void glamor_gldrawarrays_quads_using_indices(glamor_screen_private *glamor_priv,
                                             unsigned count);

/* glamor_core.c */
Bool glamor_get_drawable_location(const DrawablePtr drawable);
void glamor_get_drawable_deltas(DrawablePtr drawable, PixmapPtr pixmap,
                                int *x, int *y);
GLint glamor_compile_glsl_prog(GLenum type, const char *source);
void glamor_link_glsl_prog(ScreenPtr screen, GLint prog,
                           const char *format, ...) _X_ATTRIBUTE_PRINTF(3,4);
void glamor_get_color_4f_from_pixel(PixmapPtr pixmap,
                                    unsigned long fg_pixel, GLfloat *color);

int glamor_set_destination_pixmap(PixmapPtr pixmap);
int glamor_set_destination_pixmap_priv(glamor_screen_private *glamor_priv, PixmapPtr pixmap, glamor_pixmap_private *pixmap_priv);
void glamor_set_destination_pixmap_fbo(glamor_screen_private *glamor_priv, glamor_pixmap_fbo *, int, int, int, int);

/* nc means no check. caller must ensure this pixmap has valid fbo.
 * usually use the GLAMOR_PIXMAP_PRIV_HAS_FBO firstly.
 * */
void glamor_set_destination_pixmap_priv_nc(glamor_screen_private *glamor_priv, PixmapPtr pixmap, glamor_pixmap_private *pixmap_priv);

Bool glamor_set_alu(ScreenPtr screen, unsigned char alu);
Bool glamor_set_planemask(int depth, unsigned long planemask);
RegionPtr glamor_bitmap_to_region(PixmapPtr pixmap);

void
glamor_track_stipple(GCPtr gc);

/* glamor_render.c */
Bool glamor_composite_clipped_region(CARD8 op,
                                     PicturePtr source,
                                     PicturePtr mask,
                                     PicturePtr dest,
                                     PixmapPtr source_pixmap,
                                     PixmapPtr mask_pixmap,
                                     PixmapPtr dest_pixmap,
                                     RegionPtr region,
                                     int x_source,
                                     int y_source,
                                     int x_mask, int y_mask,
                                     int x_dest, int y_dest);

void glamor_composite(CARD8 op,
                      PicturePtr pSrc,
                      PicturePtr pMask,
                      PicturePtr pDst,
                      INT16 xSrc,
                      INT16 ySrc,
                      INT16 xMask,
                      INT16 yMask,
                      INT16 xDst, INT16 yDst, CARD16 width, CARD16 height);

void glamor_composite_rects(CARD8 op,
                            PicturePtr pDst,
                            xRenderColor *color, int nRect, xRectangle *rects);

/* glamor_trapezoid.c */
void glamor_trapezoids(CARD8 op,
                       PicturePtr src, PicturePtr dst,
                       PictFormatPtr mask_format, INT16 x_src, INT16 y_src,
                       int ntrap, xTrapezoid *traps);

/* glamor_gradient.c */
void glamor_init_gradient_shader(ScreenPtr screen);
PicturePtr glamor_generate_linear_gradient_picture(ScreenPtr screen,
                                                   PicturePtr src_picture,
                                                   int x_source, int y_source,
                                                   int width, int height,
                                                   PictFormatShort format);
PicturePtr glamor_generate_radial_gradient_picture(ScreenPtr screen,
                                                   PicturePtr src_picture,
                                                   int x_source, int y_source,
                                                   int width, int height,
                                                   PictFormatShort format);

/* glamor_triangles.c */
void glamor_triangles(CARD8 op,
                      PicturePtr pSrc,
                      PicturePtr pDst,
                      PictFormatPtr maskFormat,
                      INT16 xSrc, INT16 ySrc, int ntris, xTriangle * tris);

/* glamor_pixmap.c */

void glamor_pixmap_init(ScreenPtr screen);
void glamor_pixmap_fini(ScreenPtr screen);

/* glamor_vbo.c */

void glamor_init_vbo(ScreenPtr screen);
void glamor_fini_vbo(ScreenPtr screen);

void *
glamor_get_vbo_space(ScreenPtr screen, unsigned size, char **vbo_offset);

void
glamor_put_vbo_space(ScreenPtr screen);

/**
 * According to the flag,
 * if the flag is GLAMOR_CREATE_FBO_NO_FBO then just ensure
 * the fbo has a valid texture. Otherwise, it will ensure
 * the fbo has valid texture and attach to a valid fb.
 * If the fbo already has a valid glfbo then do nothing.
 */
Bool glamor_pixmap_ensure_fbo(PixmapPtr pixmap, int flag);

glamor_pixmap_clipped_regions *
glamor_compute_clipped_regions(PixmapPtr pixmap,
                               RegionPtr region, int *clipped_nbox,
                               int repeat_type, int reverse,
                               int upsidedown);

glamor_pixmap_clipped_regions *
glamor_compute_clipped_regions_ext(PixmapPtr pixmap,
                                   RegionPtr region, int *n_region,
                                   int inner_block_w, int inner_block_h,
                                   int reverse, int upsidedown);

Bool glamor_composite_largepixmap_region(CARD8 op,
                                         PicturePtr source,
                                         PicturePtr mask,
                                         PicturePtr dest,
                                         PixmapPtr source_pixmap,
                                         PixmapPtr mask_pixmap,
                                         PixmapPtr dest_pixmap,
                                         RegionPtr region, Bool force_clip,
                                         INT16 x_source,
                                         INT16 y_source,
                                         INT16 x_mask,
                                         INT16 y_mask,
                                         INT16 x_dest, INT16 y_dest,
                                         CARD16 width, CARD16 height);

/**
 * Upload a picture to gl texture. Similar to the
 * glamor_upload_pixmap_to_texture. Used in rendering.
 **/
Bool glamor_upload_picture_to_texture(PicturePtr picture);

void glamor_add_traps(PicturePtr pPicture,
                      INT16 x_off, INT16 y_off, int ntrap, xTrap *traps);

/* glamor_text.c */
int glamor_poly_text8(DrawablePtr pDrawable, GCPtr pGC,
                      int x, int y, int count, char *chars);

int glamor_poly_text16(DrawablePtr pDrawable, GCPtr pGC,
                       int x, int y, int count, unsigned short *chars);

void glamor_image_text8(DrawablePtr pDrawable, GCPtr pGC,
                        int x, int y, int count, char *chars);

void glamor_image_text16(DrawablePtr pDrawable, GCPtr pGC,
                         int x, int y, int count, unsigned short *chars);

/* glamor_spans.c */
void
glamor_fill_spans(DrawablePtr drawable,
                  GCPtr gc,
                  int n, DDXPointPtr points, int *widths, int sorted);

void
glamor_get_spans(DrawablePtr drawable, int wmax,
                 DDXPointPtr points, int *widths, int count, char *dst);

void
glamor_set_spans(DrawablePtr drawable, GCPtr gc, char *src,
                 DDXPointPtr points, int *widths, int numPoints, int sorted);

/* glamor_rects.c */
void
glamor_poly_fill_rect(DrawablePtr drawable,
                      GCPtr gc, int nrect, xRectangle *prect);

/* glamor_image.c */
void
glamor_put_image(DrawablePtr drawable, GCPtr gc, int depth, int x, int y,
                 int w, int h, int leftPad, int format, char *bits);

void
glamor_get_image(DrawablePtr pDrawable, int x, int y, int w, int h,
                 unsigned int format, unsigned long planeMask, char *d);

/* glamor_dash.c */
Bool
glamor_poly_lines_dash_gl(DrawablePtr drawable, GCPtr gc,
                          int mode, int n, DDXPointPtr points);

Bool
glamor_poly_segment_dash_gl(DrawablePtr drawable, GCPtr gc,
                            int nseg, xSegment *segs);

/* glamor_lines.c */
void
glamor_poly_lines(DrawablePtr drawable, GCPtr gc,
                  int mode, int n, DDXPointPtr points);

/*  glamor_segs.c */
void
glamor_poly_segment(DrawablePtr drawable, GCPtr gc,
                    int nseg, xSegment *segs);

/* glamor_copy.c */
void
glamor_copy(DrawablePtr src,
            DrawablePtr dst,
            GCPtr gc,
            BoxPtr box,
            int nbox,
            int dx,
            int dy,
            Bool reverse,
            Bool upsidedown,
            Pixel bitplane,
            void *closure);

RegionPtr
glamor_copy_area(DrawablePtr src, DrawablePtr dst, GCPtr gc,
                 int srcx, int srcy, int width, int height, int dstx, int dsty);

RegionPtr
glamor_copy_plane(DrawablePtr src, DrawablePtr dst, GCPtr gc,
                  int srcx, int srcy, int width, int height, int dstx, int dsty,
                  unsigned long bitplane);

/* glamor_glyphblt.c */
void glamor_image_glyph_blt(DrawablePtr pDrawable, GCPtr pGC,
                            int x, int y, unsigned int nglyph,
                            CharInfoPtr *ppci, void *pglyphBase);

void glamor_poly_glyph_blt(DrawablePtr pDrawable, GCPtr pGC,
                           int x, int y, unsigned int nglyph,
                           CharInfoPtr *ppci, void *pglyphBase);

void glamor_push_pixels(GCPtr pGC, PixmapPtr pBitmap,
                        DrawablePtr pDrawable, int w, int h, int x, int y);

void glamor_poly_point(DrawablePtr pDrawable, GCPtr pGC, int mode, int npt,
                       DDXPointPtr ppt);

void glamor_composite_rectangles(CARD8 op,
                                 PicturePtr dst,
                                 xRenderColor *color,
                                 int num_rects, xRectangle *rects);

/* glamor_composite_glyphs.c */
Bool
glamor_composite_glyphs_init(ScreenPtr pScreen);

void
glamor_composite_glyphs_fini(ScreenPtr pScreen);

void
glamor_composite_glyphs(CARD8 op,
                        PicturePtr src,
                        PicturePtr dst,
                        PictFormatPtr mask_format,
                        INT16 x_src,
                        INT16 y_src, int nlist,
                        GlyphListPtr list, GlyphPtr *glyphs);

/* glamor_sync.c */
Bool
glamor_sync_init(ScreenPtr screen);

void
glamor_sync_close(ScreenPtr screen);

/* glamor_util.c */
void
glamor_solid(PixmapPtr pixmap, int x, int y, int width, int height,
             unsigned long fg_pixel);

void
glamor_solid_boxes(PixmapPtr pixmap,
                   BoxPtr box, int nbox, unsigned long fg_pixel);


/* glamor_xv */
typedef struct {
    uint32_t transform_index;
    uint32_t gamma;             /* gamma value x 1000 */
    int brightness;
    int saturation;
    int hue;
    int contrast;

    DrawablePtr pDraw;
    PixmapPtr pPixmap;
    uint32_t src_pitch;
    uint8_t *src_addr;
    int src_w, src_h, dst_w, dst_h;
    int src_x, src_y, drw_x, drw_y;
    int w, h;
    RegionRec clip;
    PixmapPtr src_pix[3];       /* y, u, v for planar */
    int src_pix_w, src_pix_h;
} glamor_port_private;

extern XvAttributeRec glamor_xv_attributes[];
extern int glamor_xv_num_attributes;
extern XvImageRec glamor_xv_images[];
extern int glamor_xv_num_images;

void glamor_xv_init_port(glamor_port_private *port_priv);
void glamor_xv_stop_video(glamor_port_private *port_priv);
int glamor_xv_set_port_attribute(glamor_port_private *port_priv,
                                 Atom attribute, INT32 value);
int glamor_xv_get_port_attribute(glamor_port_private *port_priv,
                                 Atom attribute, INT32 *value);
int glamor_xv_query_image_attributes(int id,
                                     unsigned short *w, unsigned short *h,
                                     int *pitches, int *offsets);
int glamor_xv_put_image(glamor_port_private *port_priv,
                        DrawablePtr pDrawable,
                        short src_x, short src_y,
                        short drw_x, short drw_y,
                        short src_w, short src_h,
                        short drw_w, short drw_h,
                        int id,
                        unsigned char *buf,
                        short width,
                        short height,
                        Bool sync,
                        RegionPtr clipBoxes);
void glamor_xv_core_init(ScreenPtr screen);
void glamor_xv_render(glamor_port_private *port_priv, int id);

#include "glamor_utils.h"

#if 0
#define MAX_FBO_SIZE 32         /* For test purpose only. */
#endif

#include "glamor_font.h"

#define GLAMOR_MIN_ALU_INSTRUCTIONS 128 /* Minimum required number of native ALU instructions */

#endif                          /* GLAMOR_PRIV_H */
