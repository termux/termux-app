/*
 * Copyright © 2013 Red Hat
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
 *      Dave Airlie <airlied@redhat.com>
 *
 * some code is derived from the xf86-video-ati radeon driver, mainly
 * the calculations.
 */

/** @file glamor_xv.c
 *
 * Xv acceleration implementation
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "glamor_priv.h"
#include "glamor_transform.h"
#include "glamor_transfer.h"

#include <X11/extensions/Xv.h>
#include <fourcc.h>
/* Reference color space transform data */
typedef struct tagREF_TRANSFORM {
    float RefLuma;
    float RefRCb;
    float RefRCr;
    float RefGCb;
    float RefGCr;
    float RefBCb;
    float RefBCr;
} REF_TRANSFORM;

#define RTFSaturation(a)   (1.0 + ((a)*1.0)/1000.0)
#define RTFBrightness(a)   (((a)*1.0)/2000.0)
#define RTFIntensity(a)   (((a)*1.0)/2000.0)
#define RTFContrast(a)   (1.0 + ((a)*1.0)/1000.0)
#define RTFHue(a)   (((a)*3.1416)/1000.0)

static const glamor_facet glamor_facet_xv_planar_2 = {
    .name = "xv_planar_2",

    .version = 120,

    .source_name = "v_texcoord0",
    .vs_vars = ("attribute vec2 position;\n"
                "attribute vec2 v_texcoord0;\n"
                "varying vec2 tcs;\n"),
    .vs_exec = (GLAMOR_POS(gl_Position, position)
                "        tcs = v_texcoord0;\n"),

    .fs_vars = ("uniform sampler2D y_sampler;\n"
                "uniform sampler2D u_sampler;\n"
                "uniform vec4 offsetyco;\n"
                "uniform vec4 ucogamma;\n"
                "uniform vec4 vco;\n"
                "varying vec2 tcs;\n"),
    .fs_exec = (
                "        float sample;\n"
                "        vec2 sample_uv;\n"
                "        vec4 temp1;\n"
                "        sample = texture2D(y_sampler, tcs).w;\n"
                "        temp1.xyz = offsetyco.www * vec3(sample) + offsetyco.xyz;\n"
                "        sample_uv = texture2D(u_sampler, tcs).xy;\n"
                "        temp1.xyz = ucogamma.xyz * vec3(sample_uv.x) + temp1.xyz;\n"
                "        temp1.xyz = clamp(vco.xyz * vec3(sample_uv.y) + temp1.xyz, 0.0, 1.0);\n"
                "        temp1.w = 1.0;\n"
                "        gl_FragColor = temp1;\n"
                ),
};

static const glamor_facet glamor_facet_xv_planar_3 = {
    .name = "xv_planar_3",

    .version = 120,

    .source_name = "v_texcoord0",
    .vs_vars = ("attribute vec2 position;\n"
                "attribute vec2 v_texcoord0;\n"
                "varying vec2 tcs;\n"),
    .vs_exec = (GLAMOR_POS(gl_Position, position)
                "        tcs = v_texcoord0;\n"),

    .fs_vars = ("uniform sampler2D y_sampler;\n"
                "uniform sampler2D u_sampler;\n"
                "uniform sampler2D v_sampler;\n"
                "uniform vec4 offsetyco;\n"
                "uniform vec4 ucogamma;\n"
                "uniform vec4 vco;\n"
                "varying vec2 tcs;\n"),
    .fs_exec = (
                "        float sample;\n"
                "        vec4 temp1;\n"
                "        sample = texture2D(y_sampler, tcs).w;\n"
                "        temp1.xyz = offsetyco.www * vec3(sample) + offsetyco.xyz;\n"
                "        sample = texture2D(u_sampler, tcs).w;\n"
                "        temp1.xyz = ucogamma.xyz * vec3(sample) + temp1.xyz;\n"
                "        sample = texture2D(v_sampler, tcs).w;\n"
                "        temp1.xyz = clamp(vco.xyz * vec3(sample) + temp1.xyz, 0.0, 1.0);\n"
                "        temp1.w = 1.0;\n"
                "        gl_FragColor = temp1;\n"
                ),
};

#define MAKE_ATOM(a) MakeAtom(a, sizeof(a) - 1, TRUE)

XvAttributeRec glamor_xv_attributes[] = {
    {XvSettable | XvGettable, -1000, 1000, (char *)"XV_BRIGHTNESS"},
    {XvSettable | XvGettable, -1000, 1000, (char *)"XV_CONTRAST"},
    {XvSettable | XvGettable, -1000, 1000, (char *)"XV_SATURATION"},
    {XvSettable | XvGettable, -1000, 1000, (char *)"XV_HUE"},
    {XvSettable | XvGettable, 0, 1, (char *)"XV_COLORSPACE"},
    {0, 0, 0, NULL}
};
int glamor_xv_num_attributes = ARRAY_SIZE(glamor_xv_attributes) - 1;

Atom glamorBrightness, glamorContrast, glamorSaturation, glamorHue,
    glamorColorspace, glamorGamma;

XvImageRec glamor_xv_images[] = {
    XVIMAGE_YV12,
    XVIMAGE_I420,
    XVIMAGE_NV12
};
int glamor_xv_num_images = ARRAY_SIZE(glamor_xv_images);

static void
glamor_init_xv_shader(ScreenPtr screen, int id)
{
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    GLint sampler_loc;
    const glamor_facet *glamor_facet_xv_planar = NULL;

    switch (id) {
    case FOURCC_YV12:
    case FOURCC_I420:
        glamor_facet_xv_planar = &glamor_facet_xv_planar_3;
        break;
    case FOURCC_NV12:
        glamor_facet_xv_planar = &glamor_facet_xv_planar_2;
        break;
    default:
        break;
    }

    glamor_build_program(screen,
                         &glamor_priv->xv_prog,
                         glamor_facet_xv_planar, NULL, NULL, NULL);

    glUseProgram(glamor_priv->xv_prog.prog);
    sampler_loc = glGetUniformLocation(glamor_priv->xv_prog.prog, "y_sampler");
    glUniform1i(sampler_loc, 0);
    sampler_loc = glGetUniformLocation(glamor_priv->xv_prog.prog, "u_sampler");
    glUniform1i(sampler_loc, 1);

    switch (id) {
    case FOURCC_YV12:
    case FOURCC_I420:
        sampler_loc = glGetUniformLocation(glamor_priv->xv_prog.prog, "v_sampler");
        glUniform1i(sampler_loc, 2);
        break;
    case FOURCC_NV12:
        break;
    default:
        break;
    }

}

#define ClipValue(v,min,max) ((v) < (min) ? (min) : (v) > (max) ? (max) : (v))

void
glamor_xv_stop_video(glamor_port_private *port_priv)
{
}

static void
glamor_xv_free_port_data(glamor_port_private *port_priv)
{
    int i;

    for (i = 0; i < 3; i++) {
        if (port_priv->src_pix[i]) {
            glamor_destroy_pixmap(port_priv->src_pix[i]);
            port_priv->src_pix[i] = NULL;
        }
    }
    RegionUninit(&port_priv->clip);
    RegionNull(&port_priv->clip);
}

int
glamor_xv_set_port_attribute(glamor_port_private *port_priv,
                             Atom attribute, INT32 value)
{
    if (attribute == glamorBrightness)
        port_priv->brightness = ClipValue(value, -1000, 1000);
    else if (attribute == glamorHue)
        port_priv->hue = ClipValue(value, -1000, 1000);
    else if (attribute == glamorContrast)
        port_priv->contrast = ClipValue(value, -1000, 1000);
    else if (attribute == glamorSaturation)
        port_priv->saturation = ClipValue(value, -1000, 1000);
    else if (attribute == glamorGamma)
        port_priv->gamma = ClipValue(value, 100, 10000);
    else if (attribute == glamorColorspace)
        port_priv->transform_index = ClipValue(value, 0, 1);
    else
        return BadMatch;
    return Success;
}

int
glamor_xv_get_port_attribute(glamor_port_private *port_priv,
                             Atom attribute, INT32 *value)
{
    if (attribute == glamorBrightness)
        *value = port_priv->brightness;
    else if (attribute == glamorHue)
        *value = port_priv->hue;
    else if (attribute == glamorContrast)
        *value = port_priv->contrast;
    else if (attribute == glamorSaturation)
        *value = port_priv->saturation;
    else if (attribute == glamorGamma)
        *value = port_priv->gamma;
    else if (attribute == glamorColorspace)
        *value = port_priv->transform_index;
    else
        return BadMatch;

    return Success;
}

int
glamor_xv_query_image_attributes(int id,
                                 unsigned short *w, unsigned short *h,
                                 int *pitches, int *offsets)
{
    int size = 0, tmp;

    if (offsets)
        offsets[0] = 0;
    switch (id) {
    case FOURCC_YV12:
    case FOURCC_I420:
        *w = ALIGN(*w, 2);
        *h = ALIGN(*h, 2);
        size = ALIGN(*w, 4);
        if (pitches)
            pitches[0] = size;
        size *= *h;
        if (offsets)
            offsets[1] = size;
        tmp = ALIGN(*w >> 1, 4);
        if (pitches)
            pitches[1] = pitches[2] = tmp;
        tmp *= (*h >> 1);
        size += tmp;
        if (offsets)
            offsets[2] = size;
        size += tmp;
        break;
    case FOURCC_NV12:
        *w = ALIGN(*w, 2);
        *h = ALIGN(*h, 2);
        size = ALIGN(*w, 4);
        if (pitches)
            pitches[0] = size;
        size *= *h;
        if (offsets)
            offsets[1] = offsets[2] = size;
        tmp = ALIGN(*w, 4);
        if (pitches)
            pitches[1] = pitches[2] = tmp;
        tmp *= (*h >> 1);
        size += tmp;
        break;
    }
    return size;
}

/* Parameters for ITU-R BT.601 and ITU-R BT.709 colour spaces
   note the difference to the parameters used in overlay are due
   to 10bit vs. float calcs */
static REF_TRANSFORM trans[2] = {
    {1.1643, 0.0, 1.5960, -0.3918, -0.8129, 2.0172, 0.0},       /* BT.601 */
    {1.1643, 0.0, 1.7927, -0.2132, -0.5329, 2.1124, 0.0}        /* BT.709 */
};

void
glamor_xv_render(glamor_port_private *port_priv, int id)
{
    ScreenPtr screen = port_priv->pPixmap->drawable.pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(screen);
    PixmapPtr pixmap = port_priv->pPixmap;
    glamor_pixmap_private *pixmap_priv = glamor_get_pixmap_private(pixmap);
    glamor_pixmap_private *src_pixmap_priv[3];
    BoxPtr box = REGION_RECTS(&port_priv->clip);
    int nBox = REGION_NUM_RECTS(&port_priv->clip);
    GLfloat src_xscale[3], src_yscale[3];
    int i;
    const float Loff = -0.0627;
    const float Coff = -0.502;
    float uvcosf, uvsinf;
    float yco;
    float uco[3], vco[3], off[3];
    float bright, cont, gamma;
    int ref = port_priv->transform_index;
    GLint uloc;
    GLfloat *v;
    char *vbo_offset;
    int dst_box_index;

    if (!glamor_priv->xv_prog.prog)
        glamor_init_xv_shader(screen, id);

    cont = RTFContrast(port_priv->contrast);
    bright = RTFBrightness(port_priv->brightness);
    gamma = (float) port_priv->gamma / 1000.0;
    uvcosf = RTFSaturation(port_priv->saturation) * cos(RTFHue(port_priv->hue));
    uvsinf = RTFSaturation(port_priv->saturation) * sin(RTFHue(port_priv->hue));
/* overlay video also does pre-gamma contrast/sat adjust, should we? */

    yco = trans[ref].RefLuma * cont;
    uco[0] = -trans[ref].RefRCr * uvsinf;
    uco[1] = trans[ref].RefGCb * uvcosf - trans[ref].RefGCr * uvsinf;
    uco[2] = trans[ref].RefBCb * uvcosf;
    vco[0] = trans[ref].RefRCr * uvcosf;
    vco[1] = trans[ref].RefGCb * uvsinf + trans[ref].RefGCr * uvcosf;
    vco[2] = trans[ref].RefBCb * uvsinf;
    off[0] = Loff * yco + Coff * (uco[0] + vco[0]) + bright;
    off[1] = Loff * yco + Coff * (uco[1] + vco[1]) + bright;
    off[2] = Loff * yco + Coff * (uco[2] + vco[2]) + bright;
    gamma = 1.0;

    glamor_set_alu(screen, GXcopy);

    for (i = 0; i < 3; i++) {
        if (port_priv->src_pix[i]) {
            src_pixmap_priv[i] =
                glamor_get_pixmap_private(port_priv->src_pix[i]);
            pixmap_priv_get_scale(src_pixmap_priv[i], &src_xscale[i],
                                  &src_yscale[i]);
        } else {
           src_pixmap_priv[i] = NULL;
        }
    }
    glamor_make_current(glamor_priv);
    glUseProgram(glamor_priv->xv_prog.prog);

    uloc = glGetUniformLocation(glamor_priv->xv_prog.prog, "offsetyco");
    glUniform4f(uloc, off[0], off[1], off[2], yco);
    uloc = glGetUniformLocation(glamor_priv->xv_prog.prog, "ucogamma");
    glUniform4f(uloc, uco[0], uco[1], uco[2], gamma);
    uloc = glGetUniformLocation(glamor_priv->xv_prog.prog, "vco");
    glUniform4f(uloc, vco[0], vco[1], vco[2], 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, src_pixmap_priv[0]->fbo->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, src_pixmap_priv[1]->fbo->tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    switch (id) {
    case FOURCC_YV12:
    case FOURCC_I420:
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, src_pixmap_priv[2]->fbo->tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        break;
    case FOURCC_NV12:
        break;
    default:
        break;
    }

    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
    glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

    glEnable(GL_SCISSOR_TEST);

    v = glamor_get_vbo_space(screen, 3 * 4 * sizeof(GLfloat), &vbo_offset);

    /* Set up a single primitive covering the area being drawn.  We'll
     * clip it to port_priv->clip using GL scissors instead of just
     * emitting a GL_QUAD per box, because this way we hopefully avoid
     * diagonal tearing between the two triangles used to rasterize a
     * GL_QUAD.
     */
    i = 0;
    v[i++] = port_priv->drw_x;
    v[i++] = port_priv->drw_y;

    v[i++] = port_priv->drw_x + port_priv->dst_w * 2;
    v[i++] = port_priv->drw_y;

    v[i++] = port_priv->drw_x;
    v[i++] = port_priv->drw_y + port_priv->dst_h * 2;

    v[i++] = t_from_x_coord_x(src_xscale[0], port_priv->src_x);
    v[i++] = t_from_x_coord_y(src_yscale[0], port_priv->src_y);

    v[i++] = t_from_x_coord_x(src_xscale[0], port_priv->src_x +
                              port_priv->src_w * 2);
    v[i++] = t_from_x_coord_y(src_yscale[0], port_priv->src_y);

    v[i++] = t_from_x_coord_x(src_xscale[0], port_priv->src_x);
    v[i++] = t_from_x_coord_y(src_yscale[0], port_priv->src_y +
                              port_priv->src_h * 2);

    glVertexAttribPointer(GLAMOR_VERTEX_POS, 2,
                          GL_FLOAT, GL_FALSE,
                          2 * sizeof(float), vbo_offset);

    glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2,
                          GL_FLOAT, GL_FALSE,
                          2 * sizeof(float), vbo_offset + 6 * sizeof(GLfloat));

    glamor_put_vbo_space(screen);

    /* Now draw our big triangle, clipped to each of the clip boxes. */
    glamor_pixmap_loop(pixmap_priv, dst_box_index) {
        int dst_off_x, dst_off_y;

        glamor_set_destination_drawable(port_priv->pDraw,
                                        dst_box_index,
                                        FALSE, FALSE,
                                        glamor_priv->xv_prog.matrix_uniform,
                                        &dst_off_x, &dst_off_y);

        for (i = 0; i < nBox; i++) {
            int dstx, dsty, dstw, dsth;

            dstx = box[i].x1 + dst_off_x;
            dsty = box[i].y1 + dst_off_y;
            dstw = box[i].x2 - box[i].x1;
            dsth = box[i].y2 - box[i].y1;

            glScissor(dstx, dsty, dstw, dsth);
            glDrawArrays(GL_TRIANGLE_FAN, 0, 3);
        }
    }
    glDisable(GL_SCISSOR_TEST);

    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
    glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

    DamageDamageRegion(port_priv->pDraw, &port_priv->clip);

    glamor_xv_free_port_data(port_priv);
}

int
glamor_xv_put_image(glamor_port_private *port_priv,
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
                    RegionPtr clipBoxes)
{
    ScreenPtr pScreen = pDrawable->pScreen;
    glamor_screen_private *glamor_priv = glamor_get_screen_private(pScreen);
    int srcPitch, srcPitch2;
    int top, nlines;
    int s2offset, s3offset, tmp;
    BoxRec full_box, half_box;

    s2offset = s3offset = srcPitch2 = 0;

    if (!port_priv->src_pix[0] ||
        (width != port_priv->src_pix_w || height != port_priv->src_pix_h) ||
        (port_priv->src_pix[2] && id == FOURCC_NV12) ||
        (!port_priv->src_pix[2] && id != FOURCC_NV12)) {
        int i;

        if (glamor_priv->xv_prog.prog) {
            glDeleteProgram(glamor_priv->xv_prog.prog);
            glamor_priv->xv_prog.prog = 0;
        }

        for (i = 0; i < 3; i++)
            if (port_priv->src_pix[i])
                glamor_destroy_pixmap(port_priv->src_pix[i]);

        port_priv->src_pix[0] =
            glamor_create_pixmap(pScreen, width, height, 8,
                                 GLAMOR_CREATE_FBO_NO_FBO);

        switch (id) {
        case FOURCC_YV12:
        case FOURCC_I420:
            port_priv->src_pix[1] =
                glamor_create_pixmap(pScreen, width >> 1, height >> 1, 8,
                                     GLAMOR_CREATE_FBO_NO_FBO);
            port_priv->src_pix[2] =
                glamor_create_pixmap(pScreen, width >> 1, height >> 1, 8,
                                     GLAMOR_CREATE_FBO_NO_FBO);
            if (!port_priv->src_pix[2])
                return BadAlloc;
            break;
        case FOURCC_NV12:
            port_priv->src_pix[1] =
                glamor_create_pixmap(pScreen, width >> 1, height >> 1, 16,
                                     GLAMOR_CREATE_FBO_NO_FBO |
                                     GLAMOR_CREATE_FORMAT_CBCR);
            port_priv->src_pix[2] = NULL;
            break;
        default:
            return BadMatch;
        }

        port_priv->src_pix_w = width;
        port_priv->src_pix_h = height;

        if (!port_priv->src_pix[0] || !port_priv->src_pix[1])
            return BadAlloc;
    }

    top = (src_y) & ~1;
    nlines = (src_y + src_h) - top;

    switch (id) {
    case FOURCC_YV12:
    case FOURCC_I420:
        srcPitch = ALIGN(width, 4);
        srcPitch2 = ALIGN(width >> 1, 4);
        s2offset = srcPitch * height;
        s3offset = s2offset + (srcPitch2 * ((height + 1) >> 1));
        s2offset += ((top >> 1) * srcPitch2);
        s3offset += ((top >> 1) * srcPitch2);
        if (id == FOURCC_YV12) {
            tmp = s2offset;
            s2offset = s3offset;
            s3offset = tmp;
        }

        full_box.x1 = 0;
        full_box.y1 = 0;
        full_box.x2 = width;
        full_box.y2 = nlines;

        half_box.x1 = 0;
        half_box.y1 = 0;
        half_box.x2 = width >> 1;
        half_box.y2 = (nlines + 1) >> 1;

        glamor_upload_boxes(port_priv->src_pix[0], &full_box, 1,
                            0, 0, 0, 0,
                            buf + (top * srcPitch), srcPitch);

        glamor_upload_boxes(port_priv->src_pix[1], &half_box, 1,
                            0, 0, 0, 0,
                            buf + s2offset, srcPitch2);

        glamor_upload_boxes(port_priv->src_pix[2], &half_box, 1,
                            0, 0, 0, 0,
                            buf + s3offset, srcPitch2);
        break;
    case FOURCC_NV12:
        srcPitch = ALIGN(width, 4);
        s2offset = srcPitch * height;
        s2offset += ((top >> 1) * srcPitch);

        full_box.x1 = 0;
        full_box.y1 = 0;
        full_box.x2 = width;
        full_box.y2 = nlines;

        half_box.x1 = 0;
        half_box.y1 = 0;
        half_box.x2 = width;
        half_box.y2 = (nlines + 1) >> 1;

        glamor_upload_boxes(port_priv->src_pix[0], &full_box, 1,
                            0, 0, 0, 0,
                            buf + (top * srcPitch), srcPitch);

        glamor_upload_boxes(port_priv->src_pix[1], &half_box, 1,
                            0, 0, 0, 0,
                            buf + s2offset, srcPitch);
        break;
    default:
        return BadMatch;
    }

    if (pDrawable->type == DRAWABLE_WINDOW)
        port_priv->pPixmap = pScreen->GetWindowPixmap((WindowPtr) pDrawable);
    else
        port_priv->pPixmap = (PixmapPtr) pDrawable;

    RegionCopy(&port_priv->clip, clipBoxes);

    port_priv->src_x = src_x;
    port_priv->src_y = src_y - top;
    port_priv->src_w = src_w;
    port_priv->src_h = src_h;
    port_priv->dst_w = drw_w;
    port_priv->dst_h = drw_h;
    port_priv->drw_x = drw_x;
    port_priv->drw_y = drw_y;
    port_priv->w = width;
    port_priv->h = height;
    port_priv->pDraw = pDrawable;
    glamor_xv_render(port_priv, id);
    return Success;
}

void
glamor_xv_init_port(glamor_port_private *port_priv)
{
    port_priv->brightness = 0;
    port_priv->contrast = 0;
    port_priv->saturation = 0;
    port_priv->hue = 0;
    port_priv->gamma = 1000;
    port_priv->transform_index = 0;

    REGION_NULL(pScreen, &port_priv->clip);
}

void
glamor_xv_core_init(ScreenPtr screen)
{
    glamorBrightness = MAKE_ATOM("XV_BRIGHTNESS");
    glamorContrast = MAKE_ATOM("XV_CONTRAST");
    glamorSaturation = MAKE_ATOM("XV_SATURATION");
    glamorHue = MAKE_ATOM("XV_HUE");
    glamorGamma = MAKE_ATOM("XV_GAMMA");
    glamorColorspace = MAKE_ATOM("XV_COLORSPACE");
}
