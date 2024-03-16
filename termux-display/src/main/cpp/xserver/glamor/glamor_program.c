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

#include "glamor_priv.h"
#include "glamor_transform.h"
#include "glamor_program.h"

static Bool
use_solid(PixmapPtr pixmap, GCPtr gc, glamor_program *prog, void *arg)
{
    return glamor_set_solid(pixmap, gc, TRUE, prog->fg_uniform);
}

const glamor_facet glamor_fill_solid = {
    .name = "solid",
    .fs_exec = "       gl_FragColor = fg;\n",
    .locations = glamor_program_location_fg,
    .use = use_solid,
};

static Bool
use_tile(PixmapPtr pixmap, GCPtr gc, glamor_program *prog, void *arg)
{
    return glamor_set_tiled(pixmap, gc, prog->fill_offset_uniform, prog->fill_size_inv_uniform);
}

static const glamor_facet glamor_fill_tile = {
    .name = "tile",
    .vs_exec =  "       fill_pos = (fill_offset + primitive.xy + pos) * fill_size_inv;\n",
    .fs_exec =  "       gl_FragColor = texture2D(sampler, fill_pos);\n",
    .locations = glamor_program_location_fillsamp | glamor_program_location_fillpos,
    .use = use_tile,
};

static Bool
use_stipple(PixmapPtr pixmap, GCPtr gc, glamor_program *prog, void *arg)
{
    return glamor_set_stippled(pixmap, gc, prog->fg_uniform,
                               prog->fill_offset_uniform,
                               prog->fill_size_inv_uniform);
}

static const glamor_facet glamor_fill_stipple = {
    .name = "stipple",
    .vs_exec =  "       fill_pos = (fill_offset + primitive.xy + pos) * fill_size_inv;\n",
    .fs_exec = ("       float a = texture2D(sampler, fill_pos).w;\n"
                "       if (a == 0.0)\n"
                "               discard;\n"
                "       gl_FragColor = fg;\n"),
    .locations = glamor_program_location_fg | glamor_program_location_fillsamp | glamor_program_location_fillpos,
    .use = use_stipple,
};

static Bool
use_opaque_stipple(PixmapPtr pixmap, GCPtr gc, glamor_program *prog, void *arg)
{
    if (!use_stipple(pixmap, gc, prog, arg))
        return FALSE;
    glamor_set_color(pixmap, gc->bgPixel, prog->bg_uniform);
    return TRUE;
}

static const glamor_facet glamor_fill_opaque_stipple = {
    .name = "opaque_stipple",
    .vs_exec =  "       fill_pos = (fill_offset + primitive.xy + pos) * fill_size_inv;\n",
    .fs_exec = ("       float a = texture2D(sampler, fill_pos).w;\n"
                "       if (a == 0.0)\n"
                "               gl_FragColor = bg;\n"
                "       else\n"
                "               gl_FragColor = fg;\n"),
    .locations = glamor_program_location_fg | glamor_program_location_bg | glamor_program_location_fillsamp | glamor_program_location_fillpos,
    .use = use_opaque_stipple
};

static const glamor_facet *glamor_facet_fill[4] = {
    &glamor_fill_solid,
    &glamor_fill_tile,
    &glamor_fill_stipple,
    &glamor_fill_opaque_stipple,
};

typedef struct {
    glamor_program_location     location;
    const char                  *vs_vars;
    const char                  *fs_vars;
} glamor_location_var;

static glamor_location_var location_vars[] = {
    {
        .location = glamor_program_location_fg,
        .fs_vars = "uniform vec4 fg;\n"
    },
    {
        .location = glamor_program_location_bg,
        .fs_vars = "uniform vec4 bg;\n"
    },
    {
        .location = glamor_program_location_fillsamp,
        .fs_vars = "uniform sampler2D sampler;\n"
    },
    {
        .location = glamor_program_location_fillpos,
        .vs_vars = ("uniform vec2 fill_offset;\n"
                    "uniform vec2 fill_size_inv;\n"
                    "varying vec2 fill_pos;\n"),
        .fs_vars = ("varying vec2 fill_pos;\n")
    },
    {
        .location = glamor_program_location_font,
        .fs_vars = "uniform usampler2D font;\n",
    },
    {
        .location = glamor_program_location_bitplane,
        .fs_vars = ("uniform uvec4 bitplane;\n"
                    "uniform vec4 bitmul;\n"),
    },
    {
        .location = glamor_program_location_dash,
        .vs_vars = "uniform float dash_length;\n",
        .fs_vars = "uniform sampler2D dash;\n",
    },
    {
        .location = glamor_program_location_atlas,
        .fs_vars = "uniform sampler2D atlas;\n",
    },
};

static char *
add_var(char *cur, const char *add)
{
    char *new;

    if (!add)
        return cur;

    new = realloc(cur, strlen(cur) + strlen(add) + 1);
    if (!new) {
        free(cur);
        return NULL;
    }
    strcat(new, add);
    return new;
}

static char *
vs_location_vars(glamor_program_location locations)
{
    int l;
    char *vars = strdup("");

    for (l = 0; vars && l < ARRAY_SIZE(location_vars); l++)
        if (locations & location_vars[l].location)
            vars = add_var(vars, location_vars[l].vs_vars);
    return vars;
}

static char *
fs_location_vars(glamor_program_location locations)
{
    int l;
    char *vars = strdup("");

    for (l = 0; vars && l < ARRAY_SIZE(location_vars); l++)
        if (locations & location_vars[l].location)
            vars = add_var(vars, location_vars[l].fs_vars);
    return vars;
}

static const char vs_template[] =
    "%s"                                /* version */
    "%s"                                /* exts */
    "%s"                                /* defines */
    "%s"                                /* prim vs_vars */
    "%s"                                /* fill vs_vars */
    "%s"                                /* location vs_vars */
    GLAMOR_DECLARE_MATRIX
    "void main() {\n"
    "%s"                                /* prim vs_exec, outputs 'pos' and gl_Position */
    "%s"                                /* fill vs_exec */
    "}\n";

static const char fs_template[] =
    "%s"                                /* version */
    "%s"                                /* exts */
    GLAMOR_DEFAULT_PRECISION
    "%s"                                /* defines */
    "%s"                                /* prim fs_vars */
    "%s"                                /* fill fs_vars */
    "%s"                                /* location fs_vars */
    "void main() {\n"
    "%s"                                /* prim fs_exec */
    "%s"                                /* fill fs_exec */
    "%s"                                /* combine */
    "}\n";

static const char *
str(const char *s)
{
    if (!s)
        return "";
    return s;
}

static const glamor_facet facet_null_fill = {
    .name = ""
};

#define DBG 0

static GLint
glamor_get_uniform(glamor_program               *prog,
                   glamor_program_location      location,
                   const char                   *name)
{
    GLint uniform;
    if (location && (prog->locations & location) == 0)
        return -2;
    uniform = glGetUniformLocation(prog->prog, name);
#if DBG
    ErrorF("%s uniform %d\n", name, uniform);
#endif
    return uniform;
}

Bool
glamor_build_program(ScreenPtr          screen,
                     glamor_program     *prog,
                     const glamor_facet *prim,
                     const glamor_facet *fill,
                     const char         *combine,
                     const char         *defines)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);

    glamor_program_location     locations = prim->locations;
    glamor_program_flag         flags = prim->flags;

    int                         version = prim->version;
    char                        *version_string = NULL;

    char                        *fs_vars = NULL;
    char                        *vs_vars = NULL;

    char                        *vs_prog_string = NULL;
    char                        *fs_prog_string = NULL;

    GLint                       fs_prog, vs_prog;
    Bool                        gpu_shader4 = FALSE;

    if (!fill)
        fill = &facet_null_fill;

    locations |= fill->locations;
    flags |= fill->flags;
    version = MAX(version, fill->version);

    if (version > glamor_priv->glsl_version) {
        if (version == 130 && !glamor_priv->use_gpu_shader4)
            goto fail;
        else {
            version = 120;
            gpu_shader4 = TRUE;
        }
    }

    vs_vars = vs_location_vars(locations);
    fs_vars = fs_location_vars(locations);

    if (!vs_vars)
        goto fail;
    if (!fs_vars)
        goto fail;

    if (version) {
        if (asprintf(&version_string, "#version %d\n", version) < 0)
            version_string = NULL;
        if (!version_string)
            goto fail;
    }

    if (asprintf(&vs_prog_string,
                 vs_template,
                 str(version_string),
                 gpu_shader4 ? "#extension GL_EXT_gpu_shader4 : require\n" : "",
                 str(defines),
                 str(prim->vs_vars),
                 str(fill->vs_vars),
                 vs_vars,
                 str(prim->vs_exec),
                 str(fill->vs_exec)) < 0)
        vs_prog_string = NULL;

    if (asprintf(&fs_prog_string,
                 fs_template,
                 str(version_string),
                 gpu_shader4 ? "#extension GL_EXT_gpu_shader4 : require\n#define texelFetch texelFetch2D\n#define uint unsigned int\n" : "",
                 str(defines),
                 str(prim->fs_vars),
                 str(fill->fs_vars),
                 fs_vars,
                 str(prim->fs_exec),
                 str(fill->fs_exec),
                 str(combine)) < 0)
        fs_prog_string = NULL;

    if (!vs_prog_string || !fs_prog_string)
        goto fail;

    prog->prog = glCreateProgram();
#if DBG
    ErrorF("\n\tProgram %d for %s %s\n\tVertex shader:\n\n\t================\n%s\n\n\tFragment Shader:\n\n%s\t================\n",
           prog->prog, prim->name, fill->name, vs_prog_string, fs_prog_string);
#endif

    prog->flags = flags;
    prog->locations = locations;
    prog->prim_use = prim->use;
    prog->prim_use_render = prim->use_render;
    prog->fill_use = fill->use;
    prog->fill_use_render = fill->use_render;

    vs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER, vs_prog_string);
    fs_prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER, fs_prog_string);
    free(vs_prog_string);
    free(fs_prog_string);
    glAttachShader(prog->prog, vs_prog);
    glDeleteShader(vs_prog);
    glAttachShader(prog->prog, fs_prog);
    glDeleteShader(fs_prog);
    glBindAttribLocation(prog->prog, GLAMOR_VERTEX_POS, "primitive");

    if (prim->source_name) {
#if DBG
        ErrorF("Bind GLAMOR_VERTEX_SOURCE to %s\n", prim->source_name);
#endif
        glBindAttribLocation(prog->prog, GLAMOR_VERTEX_SOURCE, prim->source_name);
    }
    if (prog->alpha == glamor_program_alpha_dual_blend) {
        glBindFragDataLocationIndexed(prog->prog, 0, 0, "color0");
        glBindFragDataLocationIndexed(prog->prog, 0, 1, "color1");
    }

    glamor_link_glsl_prog(screen, prog->prog, "%s_%s", prim->name, fill->name);

    prog->matrix_uniform = glamor_get_uniform(prog, glamor_program_location_none, "v_matrix");
    prog->fg_uniform = glamor_get_uniform(prog, glamor_program_location_fg, "fg");
    prog->bg_uniform = glamor_get_uniform(prog, glamor_program_location_bg, "bg");
    prog->fill_offset_uniform = glamor_get_uniform(prog, glamor_program_location_fillpos, "fill_offset");
    prog->fill_size_inv_uniform = glamor_get_uniform(prog, glamor_program_location_fillpos, "fill_size_inv");
    prog->font_uniform = glamor_get_uniform(prog, glamor_program_location_font, "font");
    prog->bitplane_uniform = glamor_get_uniform(prog, glamor_program_location_bitplane, "bitplane");
    prog->bitmul_uniform = glamor_get_uniform(prog, glamor_program_location_bitplane, "bitmul");
    prog->dash_uniform = glamor_get_uniform(prog, glamor_program_location_dash, "dash");
    prog->dash_length_uniform = glamor_get_uniform(prog, glamor_program_location_dash, "dash_length");
    prog->atlas_uniform = glamor_get_uniform(prog, glamor_program_location_atlas, "atlas");

    free(version_string);
    free(fs_vars);
    free(vs_vars);
    return TRUE;
fail:
    prog->failed = 1;
    if (prog->prog) {
        glDeleteProgram(prog->prog);
        prog->prog = 0;
    }
    free(vs_prog_string);
    free(fs_prog_string);
    free(version_string);
    free(fs_vars);
    free(vs_vars);
    return FALSE;
}

Bool
glamor_use_program(PixmapPtr            pixmap,
                   GCPtr                gc,
                   glamor_program       *prog,
                   void                 *arg)
{
    glUseProgram(prog->prog);

    if (prog->prim_use && !prog->prim_use(pixmap, gc, prog, arg))
        return FALSE;

    if (prog->fill_use && !prog->fill_use(pixmap, gc, prog, arg))
        return FALSE;

    return TRUE;
}

glamor_program *
glamor_use_program_fill(PixmapPtr               pixmap,
                        GCPtr                   gc,
                        glamor_program_fill     *program_fill,
                        const glamor_facet      *prim)
{
    ScreenPtr                   screen = pixmap->drawable.pScreen;
    glamor_program              *prog = &program_fill->progs[gc->fillStyle];

    int                         fill_style = gc->fillStyle;
    const glamor_facet          *fill;

    if (prog->failed)
        return FALSE;

    if (!prog->prog) {
        fill = glamor_facet_fill[fill_style];
        if (!fill)
            return NULL;

        if (!glamor_build_program(screen, prog, prim, fill, NULL, NULL))
            return NULL;
    }

    if (!glamor_use_program(pixmap, gc, prog, NULL))
        return NULL;

    return prog;
}

static struct blendinfo composite_op_info[] = {
    [PictOpClear] = {0, 0, GL_ZERO, GL_ZERO},
    [PictOpSrc] = {0, 0, GL_ONE, GL_ZERO},
    [PictOpDst] = {0, 0, GL_ZERO, GL_ONE},
    [PictOpOver] = {0, 1, GL_ONE, GL_ONE_MINUS_SRC_ALPHA},
    [PictOpOverReverse] = {1, 0, GL_ONE_MINUS_DST_ALPHA, GL_ONE},
    [PictOpIn] = {1, 0, GL_DST_ALPHA, GL_ZERO},
    [PictOpInReverse] = {0, 1, GL_ZERO, GL_SRC_ALPHA},
    [PictOpOut] = {1, 0, GL_ONE_MINUS_DST_ALPHA, GL_ZERO},
    [PictOpOutReverse] = {0, 1, GL_ZERO, GL_ONE_MINUS_SRC_ALPHA},
    [PictOpAtop] = {1, 1, GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA},
    [PictOpAtopReverse] = {1, 1, GL_ONE_MINUS_DST_ALPHA, GL_SRC_ALPHA},
    [PictOpXor] = {1, 1, GL_ONE_MINUS_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA},
    [PictOpAdd] = {0, 0, GL_ONE, GL_ONE},
};

static void
glamor_set_blend(CARD8 op, glamor_program_alpha alpha, PicturePtr dst)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(dst->pDrawable->pScreen);
    GLenum src_blend, dst_blend;
    struct blendinfo *op_info;

    switch (alpha) {
    case glamor_program_alpha_ca_first:
        op = PictOpOutReverse;
        break;
    case glamor_program_alpha_ca_second:
        op = PictOpAdd;
        break;
    default:
        break;
    }

    if (!glamor_priv->is_gles)
        glDisable(GL_COLOR_LOGIC_OP);

    if (op == PictOpSrc)
        return;

    op_info = &composite_op_info[op];

    src_blend = op_info->source_blend;
    dst_blend = op_info->dest_blend;

    /* If there's no dst alpha channel, adjust the blend op so that we'll treat
     * it as always 1.
     */
    if (PICT_FORMAT_A(dst->format) == 0 && op_info->dest_alpha) {
        if (src_blend == GL_DST_ALPHA)
            src_blend = GL_ONE;
        else if (src_blend == GL_ONE_MINUS_DST_ALPHA)
            src_blend = GL_ZERO;
    }

    /* Set up the source alpha value for blending in component alpha mode. */
    if (alpha == glamor_program_alpha_dual_blend) {
        switch (dst_blend) {
        case GL_SRC_ALPHA:
            dst_blend = GL_SRC1_COLOR;
            break;
        case GL_ONE_MINUS_SRC_ALPHA:
            dst_blend = GL_ONE_MINUS_SRC1_COLOR;
            break;
        }
    } else if (alpha != glamor_program_alpha_normal) {
        switch (dst_blend) {
        case GL_SRC_ALPHA:
            dst_blend = GL_SRC_COLOR;
            break;
        case GL_ONE_MINUS_SRC_ALPHA:
            dst_blend = GL_ONE_MINUS_SRC_COLOR;
            break;
        }
    }

    glEnable(GL_BLEND);
    glBlendFunc(src_blend, dst_blend);
}

static Bool
use_source_solid(CARD8 op, PicturePtr src, PicturePtr dst, glamor_program *prog)
{
    PictSolidFill *solid = &src->pSourcePict->solidFill;
    float color[4];

    glamor_get_rgba_from_color(&solid->fullcolor, color);
    glamor_set_blend(op, prog->alpha, dst);
    glUniform4fv(prog->fg_uniform, 1, color);

    return TRUE;
}

static const glamor_facet glamor_source_solid = {
    .name = "render_solid",
    .fs_exec = "       vec4 source = fg;\n",
    .locations = glamor_program_location_fg,
    .use_render = use_source_solid,
};

static Bool
use_source_picture(CARD8 op, PicturePtr src, PicturePtr dst, glamor_program *prog)
{
    glamor_set_blend(op, prog->alpha, dst);

    return glamor_set_texture((PixmapPtr) src->pDrawable,
                              glamor_picture_red_is_alpha(dst),
                              0, 0,
                              prog->fill_offset_uniform,
                              prog->fill_size_inv_uniform);
}

static const glamor_facet glamor_source_picture = {
    .name = "render_picture",
    .vs_exec =  "       fill_pos = (fill_offset + primitive.xy + pos) * fill_size_inv;\n",
    .fs_exec =  "       vec4 source = texture2D(sampler, fill_pos);\n",
    .locations = glamor_program_location_fillsamp | glamor_program_location_fillpos,
    .use_render = use_source_picture,
};

static Bool
use_source_1x1_picture(CARD8 op, PicturePtr src, PicturePtr dst, glamor_program *prog)
{
    glamor_set_blend(op, prog->alpha, dst);

    return glamor_set_texture_pixmap((PixmapPtr) src->pDrawable,
                                     glamor_picture_red_is_alpha(dst));
}

static const glamor_facet glamor_source_1x1_picture = {
    .name = "render_picture",
    .fs_exec =  "       vec4 source = texture2D(sampler, vec2(0.5));\n",
    .locations = glamor_program_location_fillsamp,
    .use_render = use_source_1x1_picture,
};

static const glamor_facet *glamor_facet_source[glamor_program_source_count] = {
    [glamor_program_source_solid] = &glamor_source_solid,
    [glamor_program_source_picture] = &glamor_source_picture,
    [glamor_program_source_1x1_picture] = &glamor_source_1x1_picture,
};

static const char *glamor_combine[] = {
    [glamor_program_alpha_normal]    = "       gl_FragColor = source * mask.a;\n",
    [glamor_program_alpha_ca_first]  = "       gl_FragColor = source.a * mask;\n",
    [glamor_program_alpha_ca_second] = "       gl_FragColor = source * mask;\n",
    [glamor_program_alpha_dual_blend] = "      color0 = source * mask;\n"
                                        "      color1 = source.a * mask;\n"
};

static Bool
glamor_setup_one_program_render(ScreenPtr               screen,
                                glamor_program          *prog,
                                glamor_program_source   source_type,
                                glamor_program_alpha    alpha,
                                const glamor_facet      *prim,
                                const char              *defines)
{
    if (prog->failed)
        return FALSE;

    if (!prog->prog) {
        const glamor_facet      *fill = glamor_facet_source[source_type];

        if (!fill)
            return FALSE;

        prog->alpha = alpha;
        if (!glamor_build_program(screen, prog, prim, fill, glamor_combine[alpha], defines))
            return FALSE;
    }

    return TRUE;
}

glamor_program *
glamor_setup_program_render(CARD8                 op,
                            PicturePtr            src,
                            PicturePtr            mask,
                            PicturePtr            dst,
                            glamor_program_render *program_render,
                            const glamor_facet    *prim,
                            const char            *defines)
{
    ScreenPtr                   screen = dst->pDrawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    glamor_program_alpha        alpha;
    glamor_program_source       source_type;
    glamor_program              *prog;

    if (op > ARRAY_SIZE(composite_op_info))
        return NULL;

    if (glamor_is_component_alpha(mask)) {
        if (glamor_priv->has_dual_blend) {
            alpha = glamor_program_alpha_dual_blend;
        } else {
            /* This only works for PictOpOver */
            if (op != PictOpOver)
                return NULL;

            alpha = glamor_program_alpha_ca_first;
        }
    } else
        alpha = glamor_program_alpha_normal;

    if (src->pDrawable) {

        /* Can't do transforms, alphamaps or sourcing from non-pixmaps yet */
        if (src->transform || src->alphaMap || src->pDrawable->type != DRAWABLE_PIXMAP)
            return NULL;

        if (src->pDrawable->width == 1 && src->pDrawable->height == 1 && src->repeat)
            source_type = glamor_program_source_1x1_picture;
        else
            source_type = glamor_program_source_picture;
    } else {
        SourcePictPtr   sp = src->pSourcePict;
        if (!sp)
            return NULL;
        switch (sp->type) {
        case SourcePictTypeSolidFill:
            source_type = glamor_program_source_solid;
            break;
        default:
            return NULL;
        }
    }

    prog = &program_render->progs[source_type][alpha];
    if (!glamor_setup_one_program_render(screen, prog, source_type, alpha, prim, defines))
        return NULL;

    if (alpha == glamor_program_alpha_ca_first) {

	  /* Make sure we can also build the second program before
	   * deciding to use this path.
	   */
	  if (!glamor_setup_one_program_render(screen,
					       &program_render->progs[source_type][glamor_program_alpha_ca_second],
					       source_type, glamor_program_alpha_ca_second, prim,
					       defines))
	      return NULL;
    }
    return prog;
}

Bool
glamor_use_program_render(glamor_program        *prog,
                          CARD8                 op,
                          PicturePtr            src,
                          PicturePtr            dst)
{
    glUseProgram(prog->prog);

    if (prog->prim_use_render && !prog->prim_use_render(op, src, dst, prog))
        return FALSE;

    if (prog->fill_use_render && !prog->fill_use_render(op, src, dst, prog))
        return FALSE;
    return TRUE;
}
