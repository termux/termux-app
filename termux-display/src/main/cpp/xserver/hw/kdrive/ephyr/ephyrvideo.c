/*
 * Xephyr - A kdrive X server that runs in a host X window.
 *          Authored by Matthew Allum <mallum@openedhand.com>
 *
 * Copyright © 2007 OpenedHand Ltd
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of OpenedHand Ltd not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission. OpenedHand Ltd makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * OpenedHand Ltd DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OpenedHand Ltd BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:
 *    Dodji Seketeli <dodji@openedhand.com>
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif
#include <string.h>
#include <X11/extensions/Xv.h>
#include <xcb/xcb.h>
#include <xcb/xcb_aux.h>
#include <xcb/xv.h>
#include "ephyrlog.h"
#include "kdrive.h"
#include "kxv.h"
#include "ephyr.h"
#include "hostx.h"

struct _EphyrXVPriv {
    xcb_xv_query_adaptors_reply_t *host_adaptors;
    KdVideoAdaptorPtr adaptors;
    int num_adaptors;
};
typedef struct _EphyrXVPriv EphyrXVPriv;

struct _EphyrPortPriv {
    int port_number;
    KdVideoAdaptorPtr current_adaptor;
    EphyrXVPriv *xv_priv;
    unsigned char *image_buf;
    int image_buf_size;
    int image_id;
    int drw_x, drw_y, drw_w, drw_h;
    int src_x, src_y, src_w, src_h;
    int image_width, image_height;
};
typedef struct _EphyrPortPriv EphyrPortPriv;

static Bool ephyrLocalAtomToHost(int a_local_atom, int *a_host_atom);

static EphyrXVPriv *ephyrXVPrivNew(void);
static void ephyrXVPrivDelete(EphyrXVPriv * a_this);
static Bool ephyrXVPrivQueryHostAdaptors(EphyrXVPriv * a_this);
static Bool ephyrXVPrivSetAdaptorsHooks(EphyrXVPriv * a_this);
static Bool ephyrXVPrivRegisterAdaptors(EphyrXVPriv * a_this,
                                        ScreenPtr a_screen);

static Bool ephyrXVPrivIsAttrValueValid(XvAttributePtr a_attrs,
                                        int a_attrs_len,
                                        const char *a_attr_name,
                                        int a_attr_value, Bool *a_is_valid);

static Bool ephyrXVPrivGetImageBufSize(int a_port_id,
                                       int a_image_id,
                                       unsigned short a_width,
                                       unsigned short a_height, int *a_size);

static Bool ephyrXVPrivSaveImageToPortPriv(EphyrPortPriv * a_port_priv,
                                           const unsigned char *a_image,
                                           int a_image_len);

static void ephyrStopVideo(KdScreenInfo * a_info,
                           void *a_xv_priv, Bool a_exit);

static int ephyrSetPortAttribute(KdScreenInfo * a_info,
                                 Atom a_attr_name,
                                 int a_attr_value, void *a_port_priv);

static int ephyrGetPortAttribute(KdScreenInfo * a_screen_info,
                                 Atom a_attr_name,
                                 int *a_attr_value, void *a_port_priv);

static void ephyrQueryBestSize(KdScreenInfo * a_info,
                               Bool a_motion,
                               short a_src_w,
                               short a_src_h,
                               short a_drw_w,
                               short a_drw_h,
                               unsigned int *a_prefered_w,
                               unsigned int *a_prefered_h, void *a_port_priv);

static int ephyrPutImage(KdScreenInfo * a_info,
                         DrawablePtr a_drawable,
                         short a_src_x,
                         short a_src_y,
                         short a_drw_x,
                         short a_drw_y,
                         short a_src_w,
                         short a_src_h,
                         short a_drw_w,
                         short a_drw_h,
                         int a_id,
                         unsigned char *a_buf,
                         short a_width,
                         short a_height,
                         Bool a_sync,
                         RegionPtr a_clipping_region, void *a_port_priv);

static int ephyrReputImage(KdScreenInfo * a_info,
                           DrawablePtr a_drawable,
                           short a_drw_x,
                           short a_drw_y,
                           RegionPtr a_clipping_region, void *a_port_priv);

static int ephyrPutVideo(KdScreenInfo * a_info,
                         DrawablePtr a_drawable,
                         short a_vid_x, short a_vid_y,
                         short a_drw_x, short a_drw_y,
                         short a_vid_w, short a_vid_h,
                         short a_drw_w, short a_drw_h,
                         RegionPtr a_clip_region, void *a_port_priv);

static int ephyrGetVideo(KdScreenInfo * a_info,
                         DrawablePtr a_drawable,
                         short a_vid_x, short a_vid_y,
                         short a_drw_x, short a_drw_y,
                         short a_vid_w, short a_vid_h,
                         short a_drw_w, short a_drw_h,
                         RegionPtr a_clip_region, void *a_port_priv);

static int ephyrPutStill(KdScreenInfo * a_info,
                         DrawablePtr a_drawable,
                         short a_vid_x, short a_vid_y,
                         short a_drw_x, short a_drw_y,
                         short a_vid_w, short a_vid_h,
                         short a_drw_w, short a_drw_h,
                         RegionPtr a_clip_region, void *a_port_priv);

static int ephyrGetStill(KdScreenInfo * a_info,
                         DrawablePtr a_drawable,
                         short a_vid_x, short a_vid_y,
                         short a_drw_x, short a_drw_y,
                         short a_vid_w, short a_vid_h,
                         short a_drw_w, short a_drw_h,
                         RegionPtr a_clip_region, void *a_port_priv);

static int ephyrQueryImageAttributes(KdScreenInfo * a_info,
                                     int a_id,
                                     unsigned short *a_w,
                                     unsigned short *a_h,
                                     int *a_pitches, int *a_offsets);
static int s_base_port_id;

/**************
 * <helpers>
 * ************/

static Bool
adaptor_has_flags(const xcb_xv_adaptor_info_t *adaptor, uint32_t flags)
{
    return (adaptor->type & flags) == flags;
}

static Bool
ephyrLocalAtomToHost(int a_local_atom, int *a_host_atom)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_intern_atom_cookie_t cookie;
    xcb_intern_atom_reply_t *reply;
    const char *atom_name = NULL;

    EPHYR_RETURN_VAL_IF_FAIL(a_host_atom, FALSE);

    if (!ValidAtom(a_local_atom))
        return FALSE;

    atom_name = NameForAtom(a_local_atom);

    if (!atom_name)
        return FALSE;

    cookie = xcb_intern_atom(conn, FALSE, strlen(atom_name), atom_name);
    reply = xcb_intern_atom_reply(conn, cookie, NULL);
    if (!reply || reply->atom == None) {
        EPHYR_LOG_ERROR("no atom for string %s defined in host X\n", atom_name);
        return FALSE;
    }

    *a_host_atom = reply->atom;
    free(reply);

    return TRUE;
}

/**************
 *</helpers>
 * ************/

Bool
ephyrInitVideo(ScreenPtr pScreen)
{
    Bool is_ok = FALSE;

    KdScreenPriv(pScreen);
    KdScreenInfo *screen = pScreenPriv->screen;
    static EphyrXVPriv *xv_priv;

    EPHYR_LOG("enter\n");

    if (screen->fb.bitsPerPixel == 8) {
        EPHYR_LOG_ERROR("8 bits depth not supported\n");
        return FALSE;
    }

    if (!hostx_has_extension(&xcb_xv_id)) {
        EPHYR_LOG_ERROR("Host has no XVideo extension\n");
        return FALSE;
    }

    if (!xv_priv) {
        xv_priv = ephyrXVPrivNew();
    }
    if (!xv_priv) {
        EPHYR_LOG_ERROR("failed to create xv_priv\n");
        goto out;
    }

    if (!ephyrXVPrivRegisterAdaptors(xv_priv, pScreen)) {
        EPHYR_LOG_ERROR("failed to register adaptors\n");
        goto out;
    }
    is_ok = TRUE;

 out:
    return is_ok;
}

static EphyrXVPriv *
ephyrXVPrivNew(void)
{
    EphyrXVPriv *xv_priv = NULL;

    EPHYR_LOG("enter\n");

    xv_priv = calloc(1, sizeof(EphyrXVPriv));
    if (!xv_priv) {
        EPHYR_LOG_ERROR("failed to create EphyrXVPriv\n");
        goto error;
    }

    if (!ephyrXVPrivQueryHostAdaptors(xv_priv)) {
        EPHYR_LOG_ERROR("failed to query the host x for xv properties\n");
        goto error;
    }
    if (!ephyrXVPrivSetAdaptorsHooks(xv_priv)) {
        EPHYR_LOG_ERROR("failed to set xv_priv hooks\n");
        goto error;
    }

    EPHYR_LOG("leave\n");
    return xv_priv;

 error:
    if (xv_priv) {
        ephyrXVPrivDelete(xv_priv);
        xv_priv = NULL;
    }
    return NULL;
}

static void
ephyrXVPrivDelete(EphyrXVPriv * a_this)
{
    EPHYR_LOG("enter\n");

    if (!a_this)
        return;
    if (a_this->host_adaptors) {
        free(a_this->host_adaptors);
        a_this->host_adaptors = NULL;
    }
    free(a_this->adaptors);
    a_this->adaptors = NULL;
    free(a_this);
    EPHYR_LOG("leave\n");
}

static Bool
translate_video_encodings(KdVideoAdaptorPtr adaptor,
                          xcb_xv_adaptor_info_t *host_adaptor)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    int i;
    xcb_xv_query_encodings_cookie_t cookie;
    xcb_xv_query_encodings_reply_t *reply;
    xcb_xv_encoding_info_iterator_t encoding_it;

    cookie = xcb_xv_query_encodings(conn, host_adaptor->base_id);
    reply = xcb_xv_query_encodings_reply(conn, cookie, NULL);
    if (!reply)
        return FALSE;

    adaptor->nEncodings = reply->num_encodings;
    adaptor->pEncodings = calloc(adaptor->nEncodings,
                                  sizeof(*adaptor->pEncodings));
    if (!adaptor->pEncodings) {
        free(reply);
        return FALSE;
    }

    encoding_it = xcb_xv_query_encodings_info_iterator(reply);
    for (i = 0; i < adaptor->nEncodings; i++) {
        xcb_xv_encoding_info_t *encoding_info = encoding_it.data;
        KdVideoEncodingPtr encoding = &adaptor->pEncodings[i];

        encoding->id = encoding_info->encoding;
        encoding->name = strndup(xcb_xv_encoding_info_name(encoding_info),
                                 encoding_info->name_size);
        encoding->width = encoding_info->width;
        encoding->height = encoding_info->height;
        encoding->rate.numerator = encoding_info->rate.numerator;
        encoding->rate.denominator = encoding_info->rate.denominator;

        xcb_xv_encoding_info_next(&encoding_it);
    }

    free(reply);
    return TRUE;
}

static Bool
translate_xv_attributes(KdVideoAdaptorPtr adaptor,
                        xcb_xv_adaptor_info_t *host_adaptor)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    int i = 0;
    xcb_xv_attribute_info_iterator_t it;
    xcb_xv_query_port_attributes_cookie_t cookie =
        xcb_xv_query_port_attributes(conn, host_adaptor->base_id);
    xcb_xv_query_port_attributes_reply_t *reply =
        xcb_xv_query_port_attributes_reply(conn, cookie, NULL);

    if (!reply)
        return FALSE;

    adaptor->nAttributes = reply->num_attributes;
    adaptor->pAttributes = calloc(reply->num_attributes,
                                  sizeof(*adaptor->pAttributes));
    if (!adaptor->pAttributes) {
        EPHYR_LOG_ERROR("failed to allocate attributes\n");
        free(reply);
        return FALSE;
    }

    it = xcb_xv_query_port_attributes_attributes_iterator(reply);
    for (i = 0; i < reply->num_attributes; i++) {
        XvAttributePtr attribute = &adaptor->pAttributes[i];

        attribute->flags = it.data->flags;
        attribute->min_value = it.data->min;
        attribute->max_value = it.data->max;
        attribute->name = strndup(xcb_xv_attribute_info_name(it.data),
                                  it.data->size);

        /* make sure atoms of attrs names are created in xephyr */
        MakeAtom(xcb_xv_attribute_info_name(it.data), it.data->size, TRUE);

        xcb_xv_attribute_info_next(&it);
    }

    free(reply);
    return TRUE;
}

static Bool
translate_xv_image_formats(KdVideoAdaptorPtr adaptor,
                           xcb_xv_adaptor_info_t *host_adaptor)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    int i = 0;
    xcb_xv_list_image_formats_cookie_t cookie =
        xcb_xv_list_image_formats(conn, host_adaptor->base_id);
    xcb_xv_list_image_formats_reply_t *reply =
        xcb_xv_list_image_formats_reply(conn, cookie, NULL);
    xcb_xv_image_format_info_t *formats;

    if (!reply)
        return FALSE;

    adaptor->nImages = reply->num_formats;
    adaptor->pImages = calloc(reply->num_formats, sizeof(XvImageRec));
    if (!adaptor->pImages) {
        free(reply);
        return FALSE;
    }

    formats = xcb_xv_list_image_formats_format(reply);
    for (i = 0; i < reply->num_formats; i++) {
        XvImagePtr image = &adaptor->pImages[i];

        image->id = formats[i].id;
        image->type = formats[i].type;
        image->byte_order = formats[i].byte_order;
        memcpy(image->guid, formats[i].guid, 16);
        image->bits_per_pixel = formats[i].bpp;
        image->format = formats[i].format;
        image->num_planes = formats[i].num_planes;
        image->depth = formats[i].depth;
        image->red_mask = formats[i].red_mask;
        image->green_mask = formats[i].green_mask;
        image->blue_mask = formats[i].blue_mask;
        image->y_sample_bits = formats[i].y_sample_bits;
        image->u_sample_bits = formats[i].u_sample_bits;
        image->v_sample_bits = formats[i].v_sample_bits;
        image->horz_y_period = formats[i].vhorz_y_period;
        image->horz_u_period = formats[i].vhorz_u_period;
        image->horz_v_period = formats[i].vhorz_v_period;
        image->vert_y_period = formats[i].vvert_y_period;
        image->vert_u_period = formats[i].vvert_u_period;
        image->vert_v_period = formats[i].vvert_v_period;
        memcpy(image->component_order, formats[i].vcomp_order, 32);
        image->scanline_order = formats[i].vscanline_order;
    }

    free(reply);
    return TRUE;
}

static Bool
ephyrXVPrivQueryHostAdaptors(EphyrXVPriv * a_this)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_screen_t *xscreen = xcb_aux_get_screen(conn, hostx_get_screen());
    int base_port_id = 0, i = 0, port_priv_offset = 0;
    Bool is_ok = FALSE;
    xcb_generic_error_t *e = NULL;
    xcb_xv_adaptor_info_iterator_t it;

    EPHYR_RETURN_VAL_IF_FAIL(a_this, FALSE);

    EPHYR_LOG("enter\n");

    {
        xcb_xv_query_adaptors_cookie_t cookie =
            xcb_xv_query_adaptors(conn, xscreen->root);
        a_this->host_adaptors = xcb_xv_query_adaptors_reply(conn, cookie, &e);
        if (e) {
            free(e);
            EPHYR_LOG_ERROR("failed to query host adaptors\n");
            goto out;
        }
    }

    if (a_this->host_adaptors)
        a_this->num_adaptors = a_this->host_adaptors->num_adaptors;
    if (a_this->num_adaptors <= 0) {
        EPHYR_LOG_ERROR("failed to get number of host adaptors\n");
        goto out;
    }
    EPHYR_LOG("host has %d adaptors\n", a_this->num_adaptors);
    /*
     * copy what we can from adaptors into a_this->adaptors
     */
    if (a_this->num_adaptors) {
        a_this->adaptors = calloc(a_this->num_adaptors,
                                  sizeof(KdVideoAdaptorRec));
        if (!a_this->adaptors) {
            EPHYR_LOG_ERROR("failed to create internal adaptors\n");
            goto out;
        }
    }

    it = xcb_xv_query_adaptors_info_iterator(a_this->host_adaptors);
    for (i = 0; i < a_this->num_adaptors; i++) {
        xcb_xv_adaptor_info_t *cur_host_adaptor = it.data;
        xcb_xv_format_t *format = xcb_xv_adaptor_info_formats(cur_host_adaptor);
        int j = 0;

        a_this->adaptors[i].nPorts = cur_host_adaptor->num_ports;
        if (a_this->adaptors[i].nPorts <= 0) {
            EPHYR_LOG_ERROR("Could not find any port of adaptor %d\n", i);
            continue;
        }
        a_this->adaptors[i].type = cur_host_adaptor->type;
        a_this->adaptors[i].type |= XvWindowMask;
        a_this->adaptors[i].flags =
            VIDEO_OVERLAID_IMAGES | VIDEO_CLIP_TO_VIEWPORT;
        a_this->adaptors[i].name =
            strndup(xcb_xv_adaptor_info_name(cur_host_adaptor),
                    cur_host_adaptor->name_size);
        if (!a_this->adaptors[i].name)
            a_this->adaptors[i].name = strdup("Xephyr Video Overlay");
        base_port_id = cur_host_adaptor->base_id;
        if (base_port_id < 0) {
            EPHYR_LOG_ERROR("failed to get port id for adaptor %d\n", i);
            continue;
        }
        if (!s_base_port_id)
            s_base_port_id = base_port_id;

        if (!translate_video_encodings(&a_this->adaptors[i],
                                       cur_host_adaptor)) {
            EPHYR_LOG_ERROR("failed to get encodings for port port id %d,"
                            " adaptors %d\n", base_port_id, i);
            continue;
        }

        a_this->adaptors[i].nFormats = cur_host_adaptor->num_formats;
        a_this->adaptors[i].pFormats =
            calloc(cur_host_adaptor->num_formats,
                   sizeof(*a_this->adaptors[i].pFormats));
        for (j = 0; j < cur_host_adaptor->num_formats; j++) {
            xcb_visualtype_t *visual =
                xcb_aux_find_visual_by_id(xscreen, format[j].visual);
            a_this->adaptors[i].pFormats[j].depth = format[j].depth;
            a_this->adaptors[i].pFormats[j].class = visual->_class;
        }

        a_this->adaptors[i].pPortPrivates =
            calloc(a_this->adaptors[i].nPorts,
                   sizeof(DevUnion) + sizeof(EphyrPortPriv));
        port_priv_offset = a_this->adaptors[i].nPorts;
        for (j = 0; j < a_this->adaptors[i].nPorts; j++) {
            EphyrPortPriv *port_privs_base =
                (EphyrPortPriv *) &a_this->adaptors[i].
                pPortPrivates[port_priv_offset];
            EphyrPortPriv *port_priv = &port_privs_base[j];

            port_priv->port_number = base_port_id + j;
            port_priv->current_adaptor = &a_this->adaptors[i];
            port_priv->xv_priv = a_this;
            a_this->adaptors[i].pPortPrivates[j].ptr = port_priv;
        }

        if (!translate_xv_attributes(&a_this->adaptors[i], cur_host_adaptor)) {
        {
            EPHYR_LOG_ERROR("failed to get port attribute "
                            "for adaptor %d\n", i);
            continue;
        }
        }

        if (!translate_xv_image_formats(&a_this->adaptors[i], cur_host_adaptor)) {
            EPHYR_LOG_ERROR("failed to get image formats "
                            "for adaptor %d\n", i);
            continue;
        }

        xcb_xv_adaptor_info_next(&it);
    }
    is_ok = TRUE;

 out:
    EPHYR_LOG("leave\n");
    return is_ok;
}

static Bool
ephyrXVPrivSetAdaptorsHooks(EphyrXVPriv * a_this)
{
    int i = 0;
    xcb_xv_adaptor_info_iterator_t it;

    EPHYR_RETURN_VAL_IF_FAIL(a_this, FALSE);

    EPHYR_LOG("enter\n");

    it = xcb_xv_query_adaptors_info_iterator(a_this->host_adaptors);
    for (i = 0; i < a_this->num_adaptors; i++) {
        xcb_xv_adaptor_info_t *cur_host_adaptor = it.data;

        a_this->adaptors[i].ReputImage = ephyrReputImage;
        a_this->adaptors[i].StopVideo = ephyrStopVideo;
        a_this->adaptors[i].SetPortAttribute = ephyrSetPortAttribute;
        a_this->adaptors[i].GetPortAttribute = ephyrGetPortAttribute;
        a_this->adaptors[i].QueryBestSize = ephyrQueryBestSize;
        a_this->adaptors[i].QueryImageAttributes = ephyrQueryImageAttributes;

        if (adaptor_has_flags(cur_host_adaptor,
                              XCB_XV_TYPE_IMAGE_MASK | XCB_XV_TYPE_INPUT_MASK))
            a_this->adaptors[i].PutImage = ephyrPutImage;

        if (adaptor_has_flags(cur_host_adaptor,
                              XCB_XV_TYPE_VIDEO_MASK | XCB_XV_TYPE_INPUT_MASK))
            a_this->adaptors[i].PutVideo = ephyrPutVideo;

        if (adaptor_has_flags(cur_host_adaptor,
                              XCB_XV_TYPE_VIDEO_MASK | XCB_XV_TYPE_OUTPUT_MASK))
            a_this->adaptors[i].GetVideo = ephyrGetVideo;

        if (adaptor_has_flags(cur_host_adaptor,
                              XCB_XV_TYPE_STILL_MASK | XCB_XV_TYPE_INPUT_MASK))
            a_this->adaptors[i].PutStill = ephyrPutStill;

        if (adaptor_has_flags(cur_host_adaptor,
                              XCB_XV_TYPE_STILL_MASK | XCB_XV_TYPE_OUTPUT_MASK))
            a_this->adaptors[i].GetStill = ephyrGetStill;
    }
    EPHYR_LOG("leave\n");
    return TRUE;
}

static Bool
ephyrXVPrivRegisterAdaptors(EphyrXVPriv * a_this, ScreenPtr a_screen)
{
    Bool is_ok = FALSE;

    EPHYR_RETURN_VAL_IF_FAIL(a_this && a_screen, FALSE);

    EPHYR_LOG("enter\n");

    if (!a_this->num_adaptors)
        goto out;

    if (!KdXVScreenInit(a_screen, a_this->adaptors, a_this->num_adaptors)) {
        EPHYR_LOG_ERROR("failed to register adaptors\n");
        goto out;
    }
    EPHYR_LOG("there are  %d registered adaptors\n", a_this->num_adaptors);
    is_ok = TRUE;

 out:

    EPHYR_LOG("leave\n");
    return is_ok;
}

static Bool
ephyrXVPrivIsAttrValueValid(XvAttributePtr a_attrs,
                            int a_attrs_len,
                            const char *a_attr_name,
                            int a_attr_value, Bool *a_is_valid)
{
    int i = 0;

    EPHYR_RETURN_VAL_IF_FAIL(a_attrs && a_attr_name && a_is_valid, FALSE);

    for (i = 0; i < a_attrs_len; i++) {
        if (a_attrs[i].name && strcmp(a_attrs[i].name, a_attr_name))
            continue;
        if (a_attrs[i].min_value > a_attr_value ||
            a_attrs[i].max_value < a_attr_value) {
            *a_is_valid = FALSE;
            EPHYR_LOG_ERROR("attribute was not valid\n"
                            "value:%d. min:%d. max:%d\n",
                            a_attr_value,
                            a_attrs[i].min_value, a_attrs[i].max_value);
        }
        else {
            *a_is_valid = TRUE;
        }
        return TRUE;
    }
    return FALSE;
}

static Bool
ephyrXVPrivGetImageBufSize(int a_port_id,
                           int a_image_id,
                           unsigned short a_width,
                           unsigned short a_height, int *a_size)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_xv_query_image_attributes_cookie_t cookie;
    xcb_xv_query_image_attributes_reply_t *reply;
    Bool is_ok = FALSE;

    EPHYR_RETURN_VAL_IF_FAIL(a_size, FALSE);

    EPHYR_LOG("enter\n");

    cookie = xcb_xv_query_image_attributes(conn,
                                           a_port_id, a_image_id,
                                           a_width, a_height);
    reply = xcb_xv_query_image_attributes_reply(conn, cookie, NULL);
    if (!reply)
        goto out;

    *a_size = reply->data_size;
    is_ok = TRUE;

    free(reply);

 out:
    EPHYR_LOG("leave\n");
    return is_ok;
}

static Bool
ephyrXVPrivSaveImageToPortPriv(EphyrPortPriv * a_port_priv,
                               const unsigned char *a_image_buf,
                               int a_image_len)
{
    Bool is_ok = FALSE;

    EPHYR_LOG("enter\n");

    if (a_port_priv->image_buf_size < a_image_len) {
        unsigned char *buf = NULL;

        buf = realloc(a_port_priv->image_buf, a_image_len);
        if (!buf) {
            EPHYR_LOG_ERROR("failed to realloc image buffer\n");
            goto out;
        }
        a_port_priv->image_buf = buf;
        a_port_priv->image_buf_size = a_image_len;
    }
    memmove(a_port_priv->image_buf, a_image_buf, a_image_len);
    is_ok = TRUE;

 out:
    return is_ok;
    EPHYR_LOG("leave\n");
}

static void
ephyrStopVideo(KdScreenInfo * a_info, void *a_port_priv, Bool a_exit)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    EphyrPortPriv *port_priv = a_port_priv;
    EphyrScrPriv *scrpriv = a_info->driver;

    EPHYR_RETURN_IF_FAIL(port_priv);

    EPHYR_LOG("enter\n");
    xcb_xv_stop_video(conn, port_priv->port_number, scrpriv->win);
    EPHYR_LOG("leave\n");
}

static int
ephyrSetPortAttribute(KdScreenInfo * a_info,
                      Atom a_attr_name, int a_attr_value, void *a_port_priv)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    int res = Success, host_atom = 0;
    EphyrPortPriv *port_priv = a_port_priv;
    Bool is_attr_valid = FALSE;

    EPHYR_RETURN_VAL_IF_FAIL(port_priv, BadMatch);
    EPHYR_RETURN_VAL_IF_FAIL(port_priv->current_adaptor, BadMatch);
    EPHYR_RETURN_VAL_IF_FAIL(port_priv->current_adaptor->pAttributes, BadMatch);
    EPHYR_RETURN_VAL_IF_FAIL(port_priv->current_adaptor->nAttributes, BadMatch);
    EPHYR_RETURN_VAL_IF_FAIL(ValidAtom(a_attr_name), BadMatch);

    EPHYR_LOG("enter, portnum:%d, atomid:%d, attr_name:%s, attr_val:%d\n",
              port_priv->port_number,
              (int) a_attr_name, NameForAtom(a_attr_name), a_attr_value);

    if (!ephyrLocalAtomToHost(a_attr_name, &host_atom)) {
        EPHYR_LOG_ERROR("failed to convert local atom to host atom\n");
        res = BadMatch;
        goto out;
    }

    if (!ephyrXVPrivIsAttrValueValid(port_priv->current_adaptor->pAttributes,
                                     port_priv->current_adaptor->nAttributes,
                                     NameForAtom(a_attr_name),
                                     a_attr_value, &is_attr_valid)) {
        EPHYR_LOG_ERROR("failed to validate attribute %s\n",
                        NameForAtom(a_attr_name));
        /*
           res = BadMatch ;
           goto out ;
         */
    }
    if (!is_attr_valid) {
        EPHYR_LOG_ERROR("attribute %s is not valid\n",
                        NameForAtom(a_attr_name));
        /*
           res = BadMatch ;
           goto out ;
         */
    }

    xcb_xv_set_port_attribute(conn, port_priv->port_number,
                              host_atom, a_attr_value);
    xcb_flush(conn);

    res = Success;
 out:
    EPHYR_LOG("leave\n");
    return res;
}

static int
ephyrGetPortAttribute(KdScreenInfo * a_screen_info,
                      Atom a_attr_name, int *a_attr_value, void *a_port_priv)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    int res = Success, host_atom = 0;
    EphyrPortPriv *port_priv = a_port_priv;
    xcb_generic_error_t *e;
    xcb_xv_get_port_attribute_cookie_t cookie;
    xcb_xv_get_port_attribute_reply_t *reply;

    EPHYR_RETURN_VAL_IF_FAIL(port_priv, BadMatch);
    EPHYR_RETURN_VAL_IF_FAIL(ValidAtom(a_attr_name), BadMatch);

    EPHYR_LOG("enter, portnum:%d, atomid:%d, attr_name:%s\n",
              port_priv->port_number,
              (int) a_attr_name, NameForAtom(a_attr_name));

    if (!ephyrLocalAtomToHost(a_attr_name, &host_atom)) {
        EPHYR_LOG_ERROR("failed to convert local atom to host atom\n");
        res = BadMatch;
        goto out;
    }

    cookie = xcb_xv_get_port_attribute(conn, port_priv->port_number, host_atom);
    reply = xcb_xv_get_port_attribute_reply(conn, cookie, &e);
    if (e) {
        EPHYR_LOG_ERROR ("XvGetPortAttribute() failed: %d \n", e->error_code);
        free(e);
        res = BadMatch;
        goto out;
    }
    *a_attr_value = reply->value;

    free(reply);

    res = Success;
 out:
    EPHYR_LOG("leave\n");
    return res;
}

static void
ephyrQueryBestSize(KdScreenInfo * a_info,
                   Bool a_motion,
                   short a_src_w,
                   short a_src_h,
                   short a_drw_w,
                   short a_drw_h,
                   unsigned int *a_prefered_w,
                   unsigned int *a_prefered_h, void *a_port_priv)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    EphyrPortPriv *port_priv = a_port_priv;
    xcb_xv_query_best_size_cookie_t cookie =
        xcb_xv_query_best_size(conn,
                               port_priv->port_number,
                               a_src_w, a_src_h,
                               a_drw_w, a_drw_h,
                               a_motion);
    xcb_xv_query_best_size_reply_t *reply =
        xcb_xv_query_best_size_reply(conn, cookie, NULL);

    EPHYR_LOG("enter: frame (%dx%d), drw (%dx%d)\n",
              a_src_w, a_src_h, a_drw_w, a_drw_h);

    if (!reply) {
        EPHYR_LOG_ERROR ("XvQueryBestSize() failed\n");
        return;
    }
    *a_prefered_w = reply->actual_width;
    *a_prefered_h = reply->actual_height;
    EPHYR_LOG("actual (%dx%d)\n", *a_prefered_w, *a_prefered_h);
    free(reply);

    EPHYR_LOG("leave\n");
}


static Bool
ephyrHostXVPutImage(KdScreenInfo * a_info,
                    EphyrPortPriv *port_priv,
                    int a_image_id,
                    int a_drw_x,
                    int a_drw_y,
                    int a_drw_w,
                    int a_drw_h,
                    int a_src_x,
                    int a_src_y,
                    int a_src_w,
                    int a_src_h,
                    int a_image_width,
                    int a_image_height,
                    unsigned char *a_buf,
                    BoxPtr a_clip_rects, int a_clip_rect_nums)
{
    EphyrScrPriv *scrpriv = a_info->driver;
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_gcontext_t gc;
    Bool is_ok = TRUE;
    xcb_rectangle_t *rects = NULL;
    int data_len, width, height;
    xcb_xv_query_image_attributes_cookie_t image_attr_cookie;
    xcb_xv_query_image_attributes_reply_t *image_attr_reply;

    EPHYR_RETURN_VAL_IF_FAIL(a_buf, FALSE);

    EPHYR_LOG("enter, num_clip_rects: %d\n", a_clip_rect_nums);

    image_attr_cookie = xcb_xv_query_image_attributes(conn,
                                                      port_priv->port_number,
                                                      a_image_id,
                                                      a_image_width,
                                                      a_image_height);
    image_attr_reply = xcb_xv_query_image_attributes_reply(conn,
                                                           image_attr_cookie,
                                                           NULL);
    if (!image_attr_reply)
        goto out;
    data_len = image_attr_reply->data_size;
    width = image_attr_reply->width;
    height = image_attr_reply->height;
    free(image_attr_reply);

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, scrpriv->win, 0, NULL);

    if (a_clip_rect_nums) {
        int i = 0;
        rects = calloc(a_clip_rect_nums, sizeof(xcb_rectangle_t));
        for (i=0; i < a_clip_rect_nums; i++) {
            rects[i].x = a_clip_rects[i].x1;
            rects[i].y = a_clip_rects[i].y1;
            rects[i].width = a_clip_rects[i].x2 - a_clip_rects[i].x1;
            rects[i].height = a_clip_rects[i].y2 - a_clip_rects[i].y1;
            EPHYR_LOG("(x,y,w,h): (%d,%d,%d,%d)\n",
                      rects[i].x, rects[i].y, rects[i].width, rects[i].height);
        }
        xcb_set_clip_rectangles(conn,
                                XCB_CLIP_ORDERING_YX_BANDED,
                                gc,
                                0,
                                0,
                                a_clip_rect_nums,
                                rects);
	free(rects);
    }
    xcb_xv_put_image(conn,
                     port_priv->port_number,
                     scrpriv->win,
                     gc,
                     a_image_id,
                     a_src_x, a_src_y, a_src_w, a_src_h,
                     a_drw_x, a_drw_y, a_drw_w, a_drw_h,
                     width, height,
                     data_len, a_buf);
    xcb_free_gc(conn, gc);

    is_ok = TRUE;

out:
    EPHYR_LOG("leave\n");
    return is_ok;
}

static int
ephyrPutImage(KdScreenInfo * a_info,
              DrawablePtr a_drawable,
              short a_src_x,
              short a_src_y,
              short a_drw_x,
              short a_drw_y,
              short a_src_w,
              short a_src_h,
              short a_drw_w,
              short a_drw_h,
              int a_id,
              unsigned char *a_buf,
              short a_width,
              short a_height,
              Bool a_sync, RegionPtr a_clipping_region, void *a_port_priv)
{
    EphyrPortPriv *port_priv = a_port_priv;
    Bool is_ok = FALSE;
    int result = BadImplementation, image_size = 0;

    EPHYR_RETURN_VAL_IF_FAIL(a_info && a_info->pScreen, BadValue);
    EPHYR_RETURN_VAL_IF_FAIL(a_drawable, BadValue);

    EPHYR_LOG("enter\n");

    if (!ephyrHostXVPutImage(a_info, port_priv,
                             a_id,
                             a_drw_x, a_drw_y, a_drw_w, a_drw_h,
                             a_src_x, a_src_y, a_src_w, a_src_h,
                             a_width, a_height, a_buf,
                             RegionRects(a_clipping_region),
                             RegionNumRects(a_clipping_region))) {
        EPHYR_LOG_ERROR("EphyrHostXVPutImage() failed\n");
        goto out;
    }

    /*
     * Now save the image so that we can resend it to host it
     * later, in ReputImage.
     */
    if (!ephyrXVPrivGetImageBufSize(port_priv->port_number,
                                    a_id, a_width, a_height, &image_size)) {
        EPHYR_LOG_ERROR("failed to get image size\n");
        /*this is a minor error so we won't get bail out abruptly */
        is_ok = FALSE;
    }
    else {
        is_ok = TRUE;
    }
    if (is_ok) {
        if (!ephyrXVPrivSaveImageToPortPriv(port_priv, a_buf, image_size)) {
            is_ok = FALSE;
        }
        else {
            port_priv->image_id = a_id;
            port_priv->drw_x = a_drw_x;
            port_priv->drw_y = a_drw_y;
            port_priv->drw_w = a_drw_w;
            port_priv->drw_h = a_drw_h;
            port_priv->src_x = a_src_x;
            port_priv->src_y = a_src_y;
            port_priv->src_w = a_src_w;
            port_priv->src_h = a_src_h;
            port_priv->image_width = a_width;
            port_priv->image_height = a_height;
        }
    }
    if (!is_ok) {
        if (port_priv->image_buf) {
            free(port_priv->image_buf);
            port_priv->image_buf = NULL;
            port_priv->image_buf_size = 0;
        }
    }

    result = Success;

 out:
    EPHYR_LOG("leave\n");
    return result;
}

static int
ephyrReputImage(KdScreenInfo * a_info,
                DrawablePtr a_drawable,
                short a_drw_x,
                short a_drw_y, RegionPtr a_clipping_region, void *a_port_priv)
{
    EphyrPortPriv *port_priv = a_port_priv;
    int result = BadImplementation;

    EPHYR_RETURN_VAL_IF_FAIL(a_info->pScreen, FALSE);
    EPHYR_RETURN_VAL_IF_FAIL(a_drawable && port_priv, BadValue);

    EPHYR_LOG("enter\n");

    if (!port_priv->image_buf_size || !port_priv->image_buf) {
        EPHYR_LOG_ERROR("has null image buf in cache\n");
        goto out;
    }
    if (!ephyrHostXVPutImage(a_info,
                             port_priv,
                             port_priv->image_id,
                             a_drw_x, a_drw_y,
                             port_priv->drw_w, port_priv->drw_h,
                             port_priv->src_x, port_priv->src_y,
                             port_priv->src_w, port_priv->src_h,
                             port_priv->image_width, port_priv->image_height,
                             port_priv->image_buf,
                             RegionRects(a_clipping_region),
                             RegionNumRects(a_clipping_region))) {
        EPHYR_LOG_ERROR("ephyrHostXVPutImage() failed\n");
        goto out;
    }

    result = Success;

 out:
    EPHYR_LOG("leave\n");
    return result;
}

static int
ephyrPutVideo(KdScreenInfo * a_info,
              DrawablePtr a_drawable,
              short a_vid_x, short a_vid_y,
              short a_drw_x, short a_drw_y,
              short a_vid_w, short a_vid_h,
              short a_drw_w, short a_drw_h,
              RegionPtr a_clipping_region, void *a_port_priv)
{
    EphyrScrPriv *scrpriv = a_info->driver;
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_gcontext_t gc;
    EphyrPortPriv *port_priv = a_port_priv;

    EPHYR_RETURN_VAL_IF_FAIL(a_info->pScreen, BadValue);
    EPHYR_RETURN_VAL_IF_FAIL(a_drawable && port_priv, BadValue);

    EPHYR_LOG("enter\n");

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, scrpriv->win, 0, NULL);
    xcb_xv_put_video(conn, port_priv->port_number,
                     scrpriv->win, gc,
                     a_vid_x, a_vid_y, a_vid_w, a_vid_h,
                     a_drw_x, a_drw_y, a_drw_w, a_drw_h);
    xcb_free_gc(conn, gc);

    EPHYR_LOG("leave\n");
    return Success;
}

static int
ephyrGetVideo(KdScreenInfo * a_info,
              DrawablePtr a_drawable,
              short a_vid_x, short a_vid_y,
              short a_drw_x, short a_drw_y,
              short a_vid_w, short a_vid_h,
              short a_drw_w, short a_drw_h,
              RegionPtr a_clipping_region, void *a_port_priv)
{
    EphyrScrPriv *scrpriv = a_info->driver;
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_gcontext_t gc;
    EphyrPortPriv *port_priv = a_port_priv;

    EPHYR_RETURN_VAL_IF_FAIL(a_info && a_info->pScreen, BadValue);
    EPHYR_RETURN_VAL_IF_FAIL(a_drawable && port_priv, BadValue);

    EPHYR_LOG("enter\n");

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, scrpriv->win, 0, NULL);
    xcb_xv_get_video(conn, port_priv->port_number,
                     scrpriv->win, gc,
                     a_vid_x, a_vid_y, a_vid_w, a_vid_h,
                     a_drw_x, a_drw_y, a_drw_w, a_drw_h);

    xcb_free_gc(conn, gc);

    EPHYR_LOG("leave\n");
    return Success;
}

static int
ephyrPutStill(KdScreenInfo * a_info,
              DrawablePtr a_drawable,
              short a_vid_x, short a_vid_y,
              short a_drw_x, short a_drw_y,
              short a_vid_w, short a_vid_h,
              short a_drw_w, short a_drw_h,
              RegionPtr a_clipping_region, void *a_port_priv)
{
    EphyrScrPriv *scrpriv = a_info->driver;
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_gcontext_t gc;
    EphyrPortPriv *port_priv = a_port_priv;

    EPHYR_RETURN_VAL_IF_FAIL(a_info && a_info->pScreen, BadValue);
    EPHYR_RETURN_VAL_IF_FAIL(a_drawable && port_priv, BadValue);

    EPHYR_LOG("enter\n");

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, scrpriv->win, 0, NULL);
    xcb_xv_put_still(conn, port_priv->port_number,
                     scrpriv->win, gc,
                     a_vid_x, a_vid_y, a_vid_w, a_vid_h,
                     a_drw_x, a_drw_y, a_drw_w, a_drw_h);
    xcb_free_gc(conn, gc);

    EPHYR_LOG("leave\n");
    return Success;
}

static int
ephyrGetStill(KdScreenInfo * a_info,
              DrawablePtr a_drawable,
              short a_vid_x, short a_vid_y,
              short a_drw_x, short a_drw_y,
              short a_vid_w, short a_vid_h,
              short a_drw_w, short a_drw_h,
              RegionPtr a_clipping_region, void *a_port_priv)
{
    EphyrScrPriv *scrpriv = a_info->driver;
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_gcontext_t gc;
    EphyrPortPriv *port_priv = a_port_priv;

    EPHYR_RETURN_VAL_IF_FAIL(a_info && a_info->pScreen, BadValue);
    EPHYR_RETURN_VAL_IF_FAIL(a_drawable && port_priv, BadValue);

    EPHYR_LOG("enter\n");

    gc = xcb_generate_id(conn);
    xcb_create_gc(conn, gc, scrpriv->win, 0, NULL);
    xcb_xv_get_still(conn, port_priv->port_number,
                     scrpriv->win, gc,
                     a_vid_x, a_vid_y, a_vid_w, a_vid_h,
                     a_drw_x, a_drw_y, a_drw_w, a_drw_h);
    xcb_free_gc(conn, gc);

    EPHYR_LOG("leave\n");
    return Success;
}

static int
ephyrQueryImageAttributes(KdScreenInfo * a_info,
                          int a_id,
                          unsigned short *a_w,
                          unsigned short *a_h, int *a_pitches, int *a_offsets)
{
    xcb_connection_t *conn = hostx_get_xcbconn();
    xcb_xv_query_image_attributes_cookie_t cookie;
    xcb_xv_query_image_attributes_reply_t *reply;
    int image_size = 0;

    EPHYR_RETURN_VAL_IF_FAIL(a_w && a_h, FALSE);

    EPHYR_LOG("enter: dim (%dx%d), pitches: %p, offsets: %p\n",
              *a_w, *a_h, a_pitches, a_offsets);

    cookie = xcb_xv_query_image_attributes(conn,
                                           s_base_port_id, a_id,
                                           *a_w, *a_h);
    reply = xcb_xv_query_image_attributes_reply(conn, cookie, NULL);
    if (!reply)
        goto out;

    *a_w = reply->width;
    *a_h = reply->height;
    if (a_pitches && a_offsets) {
        memcpy(a_pitches, xcb_xv_query_image_attributes_pitches(reply),
               reply->num_planes << 2);
        memcpy(a_offsets, xcb_xv_query_image_attributes_offsets(reply),
               reply->num_planes << 2);
    }
    image_size = reply->data_size;

    free(reply);

    EPHYR_LOG("image size: %d, dim (%dx%d)\n", image_size, *a_w, *a_h);

 out:
    EPHYR_LOG("leave\n");
    return image_size;
}
