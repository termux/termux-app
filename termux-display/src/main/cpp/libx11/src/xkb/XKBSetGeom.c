/************************************************************
Copyright (c) 1993 by Silicon Graphics Computer Systems, Inc.

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of Silicon Graphics not be
used in advertising or publicity pertaining to distribution
of the software without specific prior written permission.
Silicon Graphics makes no representation about the suitability
of this software for any purpose. It is provided "as is"
without any express or implied warranty.

SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#ifdef DEBUG
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#endif

#include "Xlibint.h"
#include "XKBlibint.h"
#include "X11/extensions/XKBgeom.h"
#include <X11/extensions/XKBproto.h>

#ifndef MINSHORT
#define	MINSHORT	-32768
#endif
#ifndef MAXSHORT
#define	MAXSHORT	32767
#endif

/***====================================================================***/

#define	_SizeCountedString(s)  ((s)?XkbPaddedSize(2+strlen(s)):4)

static char *
_WriteCountedString(char *wire, char *str)
{
    CARD16 len, *pLen;

    len = (CARD16) (str ? strlen(str) : 0);
    pLen = (CARD16 *) wire;
    *pLen = len;
    if (len && str)
        memcpy(&wire[2], str, len);
    wire += XkbPaddedSize(len + 2);
    return wire;
}

static int
_SizeGeomProperties(XkbGeometryPtr geom)
{
    register int i, size;
    XkbPropertyPtr prop;

    for (size = i = 0, prop = geom->properties; i < geom->num_properties;
         i++, prop++) {
        size = (int) ((unsigned) size + _SizeCountedString(prop->name));
        size = (int) ((unsigned) size + _SizeCountedString(prop->value));
    }
    return size;
}

static int
_SizeGeomColors(XkbGeometryPtr geom)
{
    register int i, size;
    register XkbColorPtr color;

    for (i = size = 0, color = geom->colors; i < geom->num_colors; i++, color++) {
        size = (int) ((unsigned) size + _SizeCountedString(color->spec));
    }
    return size;
}

static int
_SizeGeomShapes(XkbGeometryPtr geom)
{
    register int i, size;
    register XkbShapePtr shape;

    for (i = size = 0, shape = geom->shapes; i < geom->num_shapes; i++, shape++) {
        register int n;
        register XkbOutlinePtr ol;

        size += SIZEOF(xkbShapeWireDesc);
        for (n = 0, ol = shape->outlines; n < shape->num_outlines; n++, ol++) {
            size += SIZEOF(xkbOutlineWireDesc);
            size += ol->num_points * SIZEOF(xkbPointWireDesc);
        }
    }
    return size;
}

static int
_SizeGeomDoodads(int num_doodads, XkbDoodadPtr doodad)
{
    register int i, size;

    for (i = size = 0; i < num_doodads; i++, doodad++) {
        size += SIZEOF(xkbAnyDoodadWireDesc);
        if (doodad->any.type == XkbTextDoodad) {
            size = (int) ((unsigned) size + _SizeCountedString(doodad->text.text));
            size = (int) ((unsigned) size + _SizeCountedString(doodad->text.font));
        }
        else if (doodad->any.type == XkbLogoDoodad) {
            size = (int) ((unsigned) size + _SizeCountedString(doodad->logo.logo_name));
        }
    }
    return size;
}

static int
_SizeGeomSections(XkbGeometryPtr geom)
{
    register int i, size;
    XkbSectionPtr section;

    for (i = size = 0, section = geom->sections; i < geom->num_sections;
         i++, section++) {
        size += SIZEOF(xkbSectionWireDesc);
        if (section->rows) {
            int r;
            XkbRowPtr row;

            for (r = 0, row = section->rows; r < section->num_rows; row++, r++) {
                size += SIZEOF(xkbRowWireDesc);
                size += row->num_keys * SIZEOF(xkbKeyWireDesc);
            }
        }
        if (section->doodads)
            size += _SizeGeomDoodads(section->num_doodads, section->doodads);
        if (section->overlays) {
            int o;
            XkbOverlayPtr ol;

            for (o = 0, ol = section->overlays; o < section->num_overlays;
                 o++, ol++) {
                int r;
                XkbOverlayRowPtr row;

                size += SIZEOF(xkbOverlayWireDesc);
                for (r = 0, row = ol->rows; r < ol->num_rows; r++, row++) {
                    size += SIZEOF(xkbOverlayRowWireDesc);
                    size += row->num_keys * SIZEOF(xkbOverlayKeyWireDesc);
                }
            }
        }
    }
    return size;
}

static int
_SizeGeomKeyAliases(XkbGeometryPtr geom)
{
    return geom->num_key_aliases * (2 * XkbKeyNameLength);
}

/***====================================================================***/

static char *
_WriteGeomProperties(char *wire, XkbGeometryPtr geom)
{
    register int i;
    register XkbPropertyPtr prop;

    for (i = 0, prop = geom->properties; i < geom->num_properties; i++, prop++) {
        wire = _WriteCountedString(wire, prop->name);
        wire = _WriteCountedString(wire, prop->value);
    }
    return wire;
}

static char *
_WriteGeomColors(char *wire, XkbGeometryPtr geom)
{
    register int i;
    register XkbColorPtr color;

    for (i = 0, color = geom->colors; i < geom->num_colors; i++, color++) {
        wire = _WriteCountedString(wire, color->spec);
    }
    return wire;
}

static char *
_WriteGeomShapes(char *wire, XkbGeometryPtr geom)
{
    int i;
    XkbShapePtr shape;
    xkbShapeWireDesc *shapeWire;

    for (i = 0, shape = geom->shapes; i < geom->num_shapes; i++, shape++) {
        register int o;
        XkbOutlinePtr ol;
        xkbOutlineWireDesc *olWire;

        shapeWire = (xkbShapeWireDesc *) wire;
        shapeWire->name = shape->name;
        shapeWire->nOutlines = shape->num_outlines;
        if (shape->primary != NULL)
            shapeWire->primaryNdx = XkbOutlineIndex(shape, shape->primary);
        else
            shapeWire->primaryNdx = XkbNoShape;
        if (shape->approx != NULL)
            shapeWire->approxNdx = XkbOutlineIndex(shape, shape->approx);
        else
            shapeWire->approxNdx = XkbNoShape;
        wire = (char *) &shapeWire[1];
        for (o = 0, ol = shape->outlines; o < shape->num_outlines; o++, ol++) {
            register int p;
            XkbPointPtr pt;
            xkbPointWireDesc *ptWire;

            olWire = (xkbOutlineWireDesc *) wire;
            olWire->nPoints = ol->num_points;
            olWire->cornerRadius = ol->corner_radius;
            wire = (char *) &olWire[1];
            ptWire = (xkbPointWireDesc *) wire;
            for (p = 0, pt = ol->points; p < ol->num_points; p++, pt++) {
                ptWire[p].x = pt->x;
                ptWire[p].y = pt->y;
            }
            wire = (char *) &ptWire[ol->num_points];
        }
    }
    return wire;
}

static char *
_WriteGeomDoodads(char *wire, int num_doodads, XkbDoodadPtr doodad)
{
    register int i;

    for (i = 0; i < num_doodads; i++, doodad++) {
        xkbDoodadWireDesc *doodadWire = (xkbDoodadWireDesc *) wire;

        wire = (char *) &doodadWire[1];
        bzero(doodadWire, SIZEOF(xkbDoodadWireDesc));
        doodadWire->any.name = doodad->any.name;
        doodadWire->any.type = doodad->any.type;
        doodadWire->any.priority = doodad->any.priority;
        doodadWire->any.top = doodad->any.top;
        doodadWire->any.left = doodad->any.left;
        doodadWire->any.angle = doodad->any.angle;
        switch (doodad->any.type) {
        case XkbOutlineDoodad:
        case XkbSolidDoodad:
            doodadWire->shape.colorNdx = doodad->shape.color_ndx;
            doodadWire->shape.shapeNdx = doodad->shape.shape_ndx;
            break;
        case XkbTextDoodad:
            doodadWire->text.width = doodad->text.width;
            doodadWire->text.height = doodad->text.height;
            doodadWire->text.colorNdx = doodad->text.color_ndx;
            wire = _WriteCountedString(wire, doodad->text.text);
            wire = _WriteCountedString(wire, doodad->text.font);
            break;
        case XkbIndicatorDoodad:
            doodadWire->indicator.shapeNdx = doodad->indicator.shape_ndx;
            doodadWire->indicator.onColorNdx = doodad->indicator.on_color_ndx;
            doodadWire->indicator.offColorNdx = doodad->indicator.off_color_ndx;
            break;
        case XkbLogoDoodad:
            doodadWire->logo.colorNdx = doodad->logo.color_ndx;
            doodadWire->logo.shapeNdx = doodad->logo.shape_ndx;
            wire = _WriteCountedString(wire, doodad->logo.logo_name);
            break;
        default:
            break;
        }
    }
    return wire;
}

static char *
_WriteGeomOverlay(char *wire, XkbOverlayPtr ol)
{
    register int r;
    XkbOverlayRowPtr row;
    xkbOverlayWireDesc *olWire = (xkbOverlayWireDesc *) wire;

    olWire->name = ol->name;
    olWire->nRows = ol->num_rows;
    wire = (char *) &olWire[1];
    for (r = 0, row = ol->rows; r < ol->num_rows; r++, row++) {
        unsigned int k;
        XkbOverlayKeyPtr key;
        xkbOverlayRowWireDesc *rowWire = (xkbOverlayRowWireDesc *) wire;

        rowWire->rowUnder = row->row_under;
        rowWire->nKeys = row->num_keys;
        wire = (char *) &rowWire[1];
        for (k = 0, key = row->keys; k < row->num_keys; k++, key++) {
            xkbOverlayKeyWireDesc *keyWire = (xkbOverlayKeyWireDesc *) wire;

            memcpy(keyWire->over, key->over.name, XkbKeyNameLength);
            memcpy(keyWire->under, key->under.name, XkbKeyNameLength);
            wire = (char *) &keyWire[1];
        }
    }
    return wire;
}

static char *
_WriteGeomSections(char *wire, XkbGeometryPtr geom)
{
    register int i;
    XkbSectionPtr section;

    for (i = 0, section = geom->sections; i < geom->num_sections;
         i++, section++) {
        xkbSectionWireDesc *sectionWire = (xkbSectionWireDesc *) wire;

        sectionWire->name = section->name;
        sectionWire->top = section->top;
        sectionWire->left = section->left;
        sectionWire->width = section->width;
        sectionWire->height = section->height;
        sectionWire->angle = section->angle;
        sectionWire->priority = section->priority;
        sectionWire->nRows = section->num_rows;
        sectionWire->nDoodads = section->num_doodads;
        sectionWire->nOverlays = section->num_overlays;
        sectionWire->pad = 0;
        wire = (char *) &sectionWire[1];
        if (section->rows) {
            int r;
            XkbRowPtr row;

            for (r = 0, row = section->rows; r < section->num_rows; r++, row++) {
                xkbRowWireDesc *rowWire = (xkbRowWireDesc *) wire;

                rowWire->top = row->top;
                rowWire->left = row->left;
                rowWire->nKeys = row->num_keys;
                rowWire->vertical = row->vertical;
                rowWire->pad = 0;
                wire = (char *) &rowWire[1];
                if (row->keys) {
                    int k;
                    XkbKeyPtr key;
                    xkbKeyWireDesc *keyWire = (xkbKeyWireDesc *) wire;

                    for (k = 0, key = row->keys; k < row->num_keys; k++, key++) {
                        memcpy(keyWire[k].name, key->name.name,
                               XkbKeyNameLength);
                        keyWire[k].gap = key->gap;
                        keyWire[k].shapeNdx = key->shape_ndx;
                        keyWire[k].colorNdx = key->color_ndx;
                    }
                    wire = (char *) &keyWire[row->num_keys];
                }
            }
        }
        if (section->doodads) {
            wire = _WriteGeomDoodads(wire,
                                     section->num_doodads, section->doodads);
        }
        if (section->overlays) {
            register int o;

            for (o = 0; o < section->num_overlays; o++) {
                wire = _WriteGeomOverlay(wire, &section->overlays[o]);
            }
        }
    }
    return wire;
}

static char *
_WriteGeomKeyAliases(char *wire, XkbGeometryPtr geom)
{
    register int sz;

    sz = geom->num_key_aliases * (XkbKeyNameLength * 2);
    if (sz > 0) {
        memcpy(wire, (char *) geom->key_aliases, (size_t)sz);
        wire += sz;
    }
    return wire;
}

/***====================================================================***/

static Status
_SendSetGeometry(Display *dpy, XkbGeometryPtr geom, xkbSetGeometryReq *req)
{
    int sz;
    char *wire, *tbuf;

    sz = 0;
    sz = (int) ((unsigned) (sz + _SizeCountedString(geom->label_font)));
    sz += _SizeGeomProperties(geom);
    sz += _SizeGeomColors(geom);
    sz += _SizeGeomShapes(geom);
    sz += _SizeGeomSections(geom);
    sz += _SizeGeomDoodads(geom->num_doodads, geom->doodads);
    sz += _SizeGeomKeyAliases(geom);
    req->length += (sz / 4);
    if (sz < (dpy->bufmax - dpy->buffer)) {
        BufAlloc(char *, wire, sz);
        tbuf = NULL;
    }
    else {
        tbuf = _XAllocTemp(dpy, sz);
        if (!tbuf)
            return BadAlloc;
        wire = tbuf;
    }
    wire = _WriteCountedString(wire, geom->label_font);
    if (geom->num_properties > 0)
        wire = _WriteGeomProperties(wire, geom);
    if (geom->num_colors > 0)
        wire = _WriteGeomColors(wire, geom);
    if (geom->num_shapes > 0)
        wire = _WriteGeomShapes(wire, geom);
    if (geom->num_sections > 0)
        wire = _WriteGeomSections(wire, geom);
    if (geom->num_doodads > 0)
        wire = _WriteGeomDoodads(wire, geom->num_doodads, geom->doodads);
    if (geom->num_key_aliases > 0)
        wire = _WriteGeomKeyAliases(wire, geom);
    if (tbuf != NULL) {
        Data(dpy, tbuf, sz);
        _XFreeTemp(dpy, tbuf, sz);
    }
    return Success;
}

/***====================================================================***/

Status
XkbSetGeometry(Display *dpy, unsigned deviceSpec, XkbGeometryPtr geom)
{
    xkbSetGeometryReq *req;
    Status ret;

    if ((!geom) || (dpy->flags & XlibDisplayNoXkb) ||
        (!dpy->xkb_info && !XkbUseExtension(dpy, NULL, NULL)))
        return BadAccess;

    LockDisplay(dpy);
    GetReq(kbSetGeometry, req);
    req->reqType = dpy->xkb_info->codes->major_opcode;
    req->xkbReqType = X_kbSetGeometry;
    req->deviceSpec = deviceSpec;
    req->nShapes = geom->num_shapes;
    req->nSections = geom->num_sections;
    req->name = geom->name;
    req->widthMM = geom->width_mm;
    req->heightMM = geom->height_mm;
    req->nProperties = geom->num_properties;
    req->nColors = geom->num_colors;
    req->nDoodads = geom->num_doodads;
    req->nKeyAliases = geom->num_key_aliases;
    req->baseColorNdx = (geom->base_color - geom->colors);
    req->labelColorNdx = (geom->label_color - geom->colors);

    ret = _SendSetGeometry(dpy, geom, req);
    UnlockDisplay(dpy);
    SyncHandle();
    return ret;
}
