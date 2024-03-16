/*
 * Copyright © 2014 Intel Corporation
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
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "kdrive.h"
#include "kxv.h"
#include "ephyr.h"
#include "glamor_priv.h"

#include <X11/extensions/Xv.h>
#include "fourcc.h"

#define NUM_FORMATS 3

static KdVideoFormatRec Formats[NUM_FORMATS] = {
    {15, TrueColor}, {16, TrueColor}, {24, TrueColor}
};

static void
ephyr_glamor_xv_stop_video(KdScreenInfo *screen, void *data, Bool cleanup)
{
    if (!cleanup)
        return;

    glamor_xv_stop_video(data);
}

static int
ephyr_glamor_xv_set_port_attribute(KdScreenInfo *screen,
                                   Atom attribute, INT32 value, void *data)
{
    return glamor_xv_set_port_attribute(data, attribute, value);
}

static int
ephyr_glamor_xv_get_port_attribute(KdScreenInfo *screen,
                                   Atom attribute, INT32 *value, void *data)
{
    return glamor_xv_get_port_attribute(data, attribute, value);
}

static void
ephyr_glamor_xv_query_best_size(KdScreenInfo *screen,
                                Bool motion,
                                short vid_w, short vid_h,
                                short drw_w, short drw_h,
                                unsigned int *p_w, unsigned int *p_h,
                                void *data)
{
    *p_w = drw_w;
    *p_h = drw_h;
}

static int
ephyr_glamor_xv_query_image_attributes(KdScreenInfo *screen,
                                       int id,
                                       unsigned short *w, unsigned short *h,
                                       int *pitches, int *offsets)
{
    return glamor_xv_query_image_attributes(id, w, h, pitches, offsets);
}

static int
ephyr_glamor_xv_put_image(KdScreenInfo *screen,
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
                          RegionPtr clipBoxes, void *data)
{
    return glamor_xv_put_image(data, pDrawable,
                               src_x, src_y,
                               drw_x, drw_y,
                               src_w, src_h,
                               drw_w, drw_h,
                               id, buf, width, height, sync, clipBoxes);
}

void
ephyr_glamor_xv_init(ScreenPtr screen)
{
    KdVideoAdaptorRec *adaptor;
    glamor_port_private *port_privates;
    KdVideoEncodingRec encoding = {
        0,
        "XV_IMAGE",
        /* These sizes should probably be GL_MAX_TEXTURE_SIZE instead
         * of 2048, but our context isn't set up yet.
         */
        2048, 2048,
        {1, 1}
    };
    int i;

    glamor_xv_core_init(screen);

    adaptor = xnfcalloc(1, sizeof(*adaptor));

    adaptor->name = "glamor textured video";
    adaptor->type = XvWindowMask | XvInputMask | XvImageMask;
    adaptor->flags = 0;
    adaptor->nEncodings = 1;
    adaptor->pEncodings = &encoding;

    adaptor->pFormats = Formats;
    adaptor->nFormats = NUM_FORMATS;

    adaptor->nPorts = 16; /* Some absurd number */
    port_privates = xnfcalloc(adaptor->nPorts,
                              sizeof(glamor_port_private));
    adaptor->pPortPrivates = xnfcalloc(adaptor->nPorts,
                                       sizeof(glamor_port_private *));
    for (i = 0; i < adaptor->nPorts; i++) {
        adaptor->pPortPrivates[i].ptr = &port_privates[i];
        glamor_xv_init_port(&port_privates[i]);
    }

    adaptor->pAttributes = glamor_xv_attributes;
    adaptor->nAttributes = glamor_xv_num_attributes;

    adaptor->pImages = glamor_xv_images;
    adaptor->nImages = glamor_xv_num_images;

    adaptor->StopVideo = ephyr_glamor_xv_stop_video;
    adaptor->SetPortAttribute = ephyr_glamor_xv_set_port_attribute;
    adaptor->GetPortAttribute = ephyr_glamor_xv_get_port_attribute;
    adaptor->QueryBestSize = ephyr_glamor_xv_query_best_size;
    adaptor->PutImage = ephyr_glamor_xv_put_image;
    adaptor->QueryImageAttributes = ephyr_glamor_xv_query_image_attributes;

    KdXVScreenInit(screen, adaptor, 1);
}
