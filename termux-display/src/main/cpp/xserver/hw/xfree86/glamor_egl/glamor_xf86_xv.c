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

/** @file glamor_xf86_xv.c
 *
 * This implements the XF86 XV interface, and calls into glamor core
 * for its support of the suspiciously similar XF86 and Kdrive
 * device-dependent XV interfaces.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#define GLAMOR_FOR_XORG
#include "glamor_priv.h"

#include <X11/extensions/Xv.h>
#include "fourcc.h"

#define NUM_FORMATS 4

static XF86VideoFormatRec Formats[NUM_FORMATS] = {
    {15, TrueColor}, {16, TrueColor}, {24, TrueColor}, {30, TrueColor}
};

static void
glamor_xf86_xv_stop_video(ScrnInfoPtr pScrn, void *data, Bool cleanup)
{
    if (!cleanup)
        return;

    glamor_xv_stop_video(data);
}

static int
glamor_xf86_xv_set_port_attribute(ScrnInfoPtr pScrn,
                                  Atom attribute, INT32 value, void *data)
{
    return glamor_xv_set_port_attribute(data, attribute, value);
}

static int
glamor_xf86_xv_get_port_attribute(ScrnInfoPtr pScrn,
                                  Atom attribute, INT32 *value, void *data)
{
    return glamor_xv_get_port_attribute(data, attribute, value);
}

static void
glamor_xf86_xv_query_best_size(ScrnInfoPtr pScrn,
                               Bool motion,
                               short vid_w, short vid_h,
                               short drw_w, short drw_h,
                               unsigned int *p_w, unsigned int *p_h, void *data)
{
    *p_w = drw_w;
    *p_h = drw_h;
}

static int
glamor_xf86_xv_query_image_attributes(ScrnInfoPtr pScrn,
                                      int id,
                                      unsigned short *w, unsigned short *h,
                                      int *pitches, int *offsets)
{
    return glamor_xv_query_image_attributes(id, w, h, pitches, offsets);
}

static int
glamor_xf86_xv_put_image(ScrnInfoPtr pScrn,
                    short src_x, short src_y,
                    short drw_x, short drw_y,
                    short src_w, short src_h,
                    short drw_w, short drw_h,
                    int id,
                    unsigned char *buf,
                    short width,
                    short height,
                    Bool sync,
                    RegionPtr clipBoxes, void *data, DrawablePtr pDrawable)
{
    return glamor_xv_put_image(data, pDrawable,
                               src_x, src_y,
                               drw_x, drw_y,
                               src_w, src_h,
                               drw_w, drw_h,
                               id, buf, width, height, sync, clipBoxes);
}

static XF86VideoEncodingRec DummyEncodingGLAMOR[1] = {
    {
     0,
     "XV_IMAGE",
     8192, 8192,
     {1, 1}
     }
};

XF86VideoAdaptorPtr
glamor_xv_init(ScreenPtr screen, int num_texture_ports)
{
    glamor_port_private *port_priv;
    XF86VideoAdaptorPtr adapt;
    int i;

    glamor_xv_core_init(screen);

    adapt = calloc(1, sizeof(XF86VideoAdaptorRec) + num_texture_ports *
                   (sizeof(glamor_port_private) + sizeof(DevUnion)));
    if (adapt == NULL)
        return NULL;

    adapt->type = XvWindowMask | XvInputMask | XvImageMask;
    adapt->flags = 0;
    adapt->name = "GLAMOR Textured Video";
    adapt->nEncodings = 1;
    adapt->pEncodings = DummyEncodingGLAMOR;

    adapt->nFormats = NUM_FORMATS;
    adapt->pFormats = Formats;
    adapt->nPorts = num_texture_ports;
    adapt->pPortPrivates = (DevUnion *) (&adapt[1]);

    adapt->pAttributes = glamor_xv_attributes;
    adapt->nAttributes = glamor_xv_num_attributes;

    port_priv =
        (glamor_port_private *) (&adapt->pPortPrivates[num_texture_ports]);
    adapt->pImages = glamor_xv_images;
    adapt->nImages = glamor_xv_num_images;
    adapt->PutVideo = NULL;
    adapt->PutStill = NULL;
    adapt->GetVideo = NULL;
    adapt->GetStill = NULL;
    adapt->StopVideo = glamor_xf86_xv_stop_video;
    adapt->SetPortAttribute = glamor_xf86_xv_set_port_attribute;
    adapt->GetPortAttribute = glamor_xf86_xv_get_port_attribute;
    adapt->QueryBestSize = glamor_xf86_xv_query_best_size;
    adapt->PutImage = glamor_xf86_xv_put_image;
    adapt->ReputImage = NULL;
    adapt->QueryImageAttributes = glamor_xf86_xv_query_image_attributes;

    for (i = 0; i < num_texture_ports; i++) {
        glamor_port_private *pPriv = &port_priv[i];

        pPriv->brightness = 0;
        pPriv->contrast = 0;
        pPriv->saturation = 0;
        pPriv->hue = 0;
        pPriv->gamma = 1000;
        pPriv->transform_index = 0;

        REGION_NULL(pScreen, &pPriv->clip);

        adapt->pPortPrivates[i].ptr = (void *) (pPriv);
    }
    return adapt;
}
