/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <GL/glxtokens.h>
#include <string.h>
#include <windowstr.h>
#include <os.h>
#include <colormapst.h>

#include "extinit.h"
#include "privates.h"
#include "glxserver.h"
#include "glxutil.h"
#include "glxext.h"
#include "protocol-versions.h"

#ifdef COMPOSITE
#include "compositeext.h"
#endif

static DevPrivateKeyRec glxScreenPrivateKeyRec;

#define glxScreenPrivateKey (&glxScreenPrivateKeyRec)

const char GLServerVersion[] = "1.4";
static const char GLServerExtensions[] =
    "GL_ARB_depth_texture "
    "GL_ARB_draw_buffers "
    "GL_ARB_fragment_program "
    "GL_ARB_fragment_program_shadow "
    "GL_ARB_imaging "
    "GL_ARB_multisample "
    "GL_ARB_multitexture "
    "GL_ARB_occlusion_query "
    "GL_ARB_point_parameters "
    "GL_ARB_point_sprite "
    "GL_ARB_shadow "
    "GL_ARB_shadow_ambient "
    "GL_ARB_texture_border_clamp "
    "GL_ARB_texture_compression "
    "GL_ARB_texture_cube_map "
    "GL_ARB_texture_env_add "
    "GL_ARB_texture_env_combine "
    "GL_ARB_texture_env_crossbar "
    "GL_ARB_texture_env_dot3 "
    "GL_ARB_texture_mirrored_repeat "
    "GL_ARB_texture_non_power_of_two "
    "GL_ARB_transpose_matrix "
    "GL_ARB_vertex_program "
    "GL_ARB_window_pos "
    "GL_EXT_abgr "
    "GL_EXT_bgra "
    "GL_EXT_blend_color "
    "GL_EXT_blend_equation_separate "
    "GL_EXT_blend_func_separate "
    "GL_EXT_blend_logic_op "
    "GL_EXT_blend_minmax "
    "GL_EXT_blend_subtract "
    "GL_EXT_clip_volume_hint "
    "GL_EXT_copy_texture "
    "GL_EXT_draw_range_elements "
    "GL_EXT_fog_coord "
    "GL_EXT_framebuffer_object "
    "GL_EXT_multi_draw_arrays "
    "GL_EXT_packed_pixels "
    "GL_EXT_paletted_texture "
    "GL_EXT_point_parameters "
    "GL_EXT_polygon_offset "
    "GL_EXT_rescale_normal "
    "GL_EXT_secondary_color "
    "GL_EXT_separate_specular_color "
    "GL_EXT_shadow_funcs "
    "GL_EXT_shared_texture_palette "
    "GL_EXT_stencil_two_side "
    "GL_EXT_stencil_wrap "
    "GL_EXT_subtexture "
    "GL_EXT_texture "
    "GL_EXT_texture3D "
    "GL_EXT_texture_compression_dxt1 "
    "GL_EXT_texture_compression_s3tc "
    "GL_EXT_texture_edge_clamp "
    "GL_EXT_texture_env_add "
    "GL_EXT_texture_env_combine "
    "GL_EXT_texture_env_dot3 "
    "GL_EXT_texture_filter_anisotropic "
    "GL_EXT_texture_lod "
    "GL_EXT_texture_lod_bias "
    "GL_EXT_texture_mirror_clamp "
    "GL_EXT_texture_object "
    "GL_EXT_texture_rectangle "
    "GL_EXT_vertex_array "
    "GL_3DFX_texture_compression_FXT1 "
    "GL_APPLE_packed_pixels "
    "GL_ATI_draw_buffers "
    "GL_ATI_texture_env_combine3 "
    "GL_ATI_texture_mirror_once "
    "GL_HP_occlusion_test "
    "GL_IBM_texture_mirrored_repeat "
    "GL_INGR_blend_func_separate "
    "GL_MESA_pack_invert "
    "GL_MESA_ycbcr_texture "
    "GL_NV_blend_square "
    "GL_NV_depth_clamp "
    "GL_NV_fog_distance "
    "GL_NV_fragment_program_option "
    "GL_NV_fragment_program2 "
    "GL_NV_light_max_exponent "
    "GL_NV_multisample_filter_hint "
    "GL_NV_point_sprite "
    "GL_NV_texgen_reflection "
    "GL_NV_texture_compression_vtc "
    "GL_NV_texture_env_combine4 "
    "GL_NV_texture_expand_normal "
    "GL_NV_texture_rectangle "
    "GL_NV_vertex_program2_option "
    "GL_NV_vertex_program3 "
    "GL_OES_compressed_paletted_texture "
    "GL_SGI_color_matrix "
    "GL_SGI_color_table "
    "GL_SGIS_generate_mipmap "
    "GL_SGIS_multisample "
    "GL_SGIS_point_parameters "
    "GL_SGIS_texture_border_clamp "
    "GL_SGIS_texture_edge_clamp "
    "GL_SGIS_texture_lod "
    "GL_SGIX_depth_texture "
    "GL_SGIX_shadow "
    "GL_SGIX_shadow_ambient "
    "GL_SUN_slice_accum ";

static Bool
glxCloseScreen(ScreenPtr pScreen)
{
    __GLXscreen *pGlxScreen = glxGetScreen(pScreen);

    pScreen->CloseScreen = pGlxScreen->CloseScreen;

    pGlxScreen->destroy(pGlxScreen);

    return pScreen->CloseScreen(pScreen);
}

__GLXscreen *
glxGetScreen(ScreenPtr pScreen)
{
    return dixLookupPrivate(&pScreen->devPrivates, glxScreenPrivateKey);
}

GLint
glxConvertToXVisualType(int visualType)
{
    static const int x_visual_types[] = {
        TrueColor, DirectColor,
        PseudoColor, StaticColor,
        GrayScale, StaticGray
    };

    return ((unsigned) (visualType - GLX_TRUE_COLOR) < 6)
        ? x_visual_types[visualType - GLX_TRUE_COLOR] : -1;
}

/* This code inspired by composite/compinit.c.  We could move this to
 * mi/ and share it with composite.*/

static VisualPtr
AddScreenVisuals(ScreenPtr pScreen, int count, int d)
{
    int i;
    DepthPtr depth;

    depth = NULL;
    for (i = 0; i < pScreen->numDepths; i++) {
        if (pScreen->allowedDepths[i].depth == d) {
            depth = &pScreen->allowedDepths[i];
            break;
        }
    }
    if (depth == NULL)
        return NULL;

    if (ResizeVisualArray(pScreen, count, depth) == FALSE)
        return NULL;

    /* Return a pointer to the first of the added visuals. */
    return pScreen->visuals + pScreen->numVisuals - count;
}

static int
findFirstSet(unsigned int v)
{
    int i;

    for (i = 0; i < 32; i++)
        if (v & (1 << i))
            return i;

    return -1;
}

static void
initGlxVisual(VisualPtr visual, __GLXconfig * config)
{
    int maxBits;

    maxBits = max(config->redBits, max(config->greenBits, config->blueBits));

    config->visualID = visual->vid;
    visual->class = glxConvertToXVisualType(config->visualType);
    visual->bitsPerRGBValue = maxBits;
    visual->ColormapEntries = 1 << maxBits;
    visual->nplanes = config->redBits + config->greenBits + config->blueBits;

    visual->redMask = config->redMask;
    visual->greenMask = config->greenMask;
    visual->blueMask = config->blueMask;
    visual->offsetRed = findFirstSet(config->redMask);
    visual->offsetGreen = findFirstSet(config->greenMask);
    visual->offsetBlue = findFirstSet(config->blueMask);
}

static __GLXconfig *
pickFBConfig(__GLXscreen * pGlxScreen, VisualPtr visual)
{
    __GLXconfig *best = NULL, *config;
    int best_score = 0;

    for (config = pGlxScreen->fbconfigs; config != NULL; config = config->next) {
        int score = 0;

        if (config->redMask != visual->redMask ||
            config->greenMask != visual->greenMask ||
            config->blueMask != visual->blueMask)
            continue;
        if (config->visualRating != GLX_NONE)
            continue;
        /* Ignore multisampled configs */
        if (config->sampleBuffers)
            continue;
        if (glxConvertToXVisualType(config->visualType) != visual->class)
            continue;
        /* If it's the 32-bit RGBA visual, demand a 32-bit fbconfig. */
        if (visual->nplanes == 32 && config->rgbBits != 32)
            continue;
        /* If it's the 32-bit RGBA visual, do not pick sRGB capable config.
         * This can cause issues with compositors that are not sRGB aware.
         */
        if (visual->nplanes == 32 && config->sRGBCapable == GL_TRUE)
            continue;
        /* Can't use the same FBconfig for multiple X visuals.  I think. */
        if (config->visualID != 0)
            continue;
#ifdef COMPOSITE
        if (!noCompositeExtension) {
            /* Use only duplicated configs for compIsAlternateVisuals */
            if (!!compIsAlternateVisual(pGlxScreen->pScreen, visual->vid) !=
                !!config->duplicatedForComp)
                continue;
        }
#endif
        /*
         * If possible, use the same swapmethod for all built-in visual
         * fbconfigs, to avoid getting the 32-bit composite visual when
         * requesting, for example, a SWAP_COPY fbconfig.
         */
        if (config->swapMethod == GLX_SWAP_UNDEFINED_OML)
            score += 32;
        if (config->swapMethod == GLX_SWAP_EXCHANGE_OML)
            score += 16;
        if (config->doubleBufferMode > 0)
            score += 8;
        if (config->depthBits > 0)
            score += 4;
        if (config->stencilBits > 0)
            score += 2;
        if (config->alphaBits > 0)
            score++;

        if (score > best_score) {
            best = config;
            best_score = score;
        }
    }

    return best;
}

void
__glXScreenInit(__GLXscreen * pGlxScreen, ScreenPtr pScreen)
{
    __GLXconfig *m;
    __GLXconfig *config;
    int i;

    if (!dixRegisterPrivateKey(&glxScreenPrivateKeyRec, PRIVATE_SCREEN, 0))
        return;

    pGlxScreen->pScreen = pScreen;
    pGlxScreen->GLextensions = strdup(GLServerExtensions);
    pGlxScreen->GLXextensions = NULL;

    pGlxScreen->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = glxCloseScreen;

    i = 0;
    for (m = pGlxScreen->fbconfigs; m != NULL; m = m->next) {
        m->fbconfigID = FakeClientID(0);
        m->visualID = 0;
        i++;
    }
    pGlxScreen->numFBConfigs = i;

    pGlxScreen->visuals =
        calloc(pGlxScreen->numFBConfigs, sizeof(__GLXconfig *));

    /* First, try to choose featureful FBconfigs for the existing X visuals.
     * Note that if multiple X visuals end up with the same FBconfig being
     * chosen, the later X visuals don't get GLX visuals (because we want to
     * prioritize the root visual being GLX).
     */
    for (i = 0; i < pScreen->numVisuals; i++) {
        VisualPtr visual = &pScreen->visuals[i];

        config = pickFBConfig(pGlxScreen, visual);
        if (config) {
            pGlxScreen->visuals[pGlxScreen->numVisuals++] = config;
            config->visualID = visual->vid;
#ifdef COMPOSITE
            if (!noCompositeExtension) {
                if (compIsAlternateVisual(pScreen, visual->vid))
                    config->visualSelectGroup++;
            }
#endif
        }
    }

    /* Then, add new visuals corresponding to all FBconfigs that didn't have
     * an existing, appropriate visual.
     */
    for (config = pGlxScreen->fbconfigs; config != NULL; config = config->next) {
        int depth;

        VisualPtr visual;

        if (config->visualID != 0)
            continue;

        /* Only count RGB bits and not alpha, as we're not trying to create
         * visuals for compositing (that's what the 32-bit composite visual
         * set up above is for.
         */
        depth = config->redBits + config->greenBits + config->blueBits;
#ifdef COMPOSITE
        if (!noCompositeExtension) {
            if (config->duplicatedForComp) {
                    depth += config->alphaBits;
                    config->visualSelectGroup++;
            }
        }
#endif
        /* Make sure that our FBconfig's depth can actually be displayed
         * (corresponds to an existing visual).
         */
        for (i = 0; i < pScreen->numVisuals; i++) {
            if (depth == pScreen->visuals[i].nplanes)
                break;
        }
        /* if it can't, fix up the fbconfig to not advertise window support */
        if (i == pScreen->numVisuals)
            config->drawableType &= ~(GLX_WINDOW_BIT);

        /* fbconfig must support window drawables */
        if (!(config->drawableType & GLX_WINDOW_BIT)) {
            config->visualID = 0;
            continue;
        }

        /* Create a new X visual for our FBconfig. */
        visual = AddScreenVisuals(pScreen, 1, depth);
        if (visual == NULL)
            continue;

#ifdef COMPOSITE
        if (!noCompositeExtension) {
            if (config->duplicatedForComp)
                (void) CompositeRegisterAlternateVisuals(pScreen, &visual->vid, 1);
        }
#endif
        pGlxScreen->visuals[pGlxScreen->numVisuals++] = config;
        initGlxVisual(visual, config);
    }

    dixSetPrivate(&pScreen->devPrivates, glxScreenPrivateKey, pGlxScreen);

    if (pGlxScreen->glvnd)
        __glXEnableExtension(pGlxScreen->glx_enable_bits, "GLX_EXT_libglvnd");

    i = __glXGetExtensionString(pGlxScreen->glx_enable_bits, NULL);
    if (i > 0) {
        pGlxScreen->GLXextensions = xnfalloc(i);
        (void) __glXGetExtensionString(pGlxScreen->glx_enable_bits,
                                       pGlxScreen->GLXextensions);
    }

}

void
__glXScreenDestroy(__GLXscreen * screen)
{
    __GLXconfig *config, *next;

    free(screen->glvnd);
    free(screen->GLXextensions);
    free(screen->GLextensions);
    free(screen->visuals);

    for (config = screen->fbconfigs; config != NULL; config = next) {
        next = config->next;
        free(config);
    }
}
