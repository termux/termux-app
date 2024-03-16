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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <stdio.h>
#include "Xlibint.h"
#include "XKBlibint.h"
#include "X11/extensions/XKBgeom.h"
#include <X11/extensions/XKBproto.h>

/***====================================================================***/

static void
_XkbFreeGeomLeafElems(Bool freeAll,
                      int first,
                      int count,
                      unsigned short *num_inout,
                      unsigned short *sz_inout,
                      char **elems,
                      unsigned int elem_sz)
{
    if ((freeAll) || (*elems == NULL)) {
        *num_inout = *sz_inout = 0;
        if (*elems != NULL) {
            _XkbFree(*elems);
            *elems = NULL;
        }
        return;
    }

    if ((first >= (*num_inout)) || (first < 0) || (count < 1))
        return;

    if (first + count >= (*num_inout)) {
        /* truncating the array is easy */
        (*num_inout) = first;
    }
    else {
        char *ptr;
        int extra;

        ptr = *elems;
        extra = ((*num_inout) - (first + count)) * elem_sz;
        if (extra > 0)
            memmove(&ptr[(unsigned) first * elem_sz], &ptr[(unsigned)(first + count) * elem_sz],
                    (size_t) extra);
        (*num_inout) -= count;
    }
    return;
}

typedef void (*ContentsClearFunc) (
    char *       /* priv */
);

static void
_XkbFreeGeomNonLeafElems(Bool freeAll,
                         int first,
                         int count,
                         unsigned short *num_inout,
                         unsigned short *sz_inout,
                         char **elems,
                         unsigned int elem_sz,
                         ContentsClearFunc freeFunc)
{
    register int i;
    register char *ptr;

    if (freeAll) {
        first = 0;
        count = (*num_inout);
    }
    else if ((first >= (*num_inout)) || (first < 0) || (count < 1))
        return;
    else if (first + count > (*num_inout))
        count = (*num_inout) - first;
    if (*elems == NULL)
        return;

    if (freeFunc) {
        ptr = *elems;
        ptr += first * elem_sz;
        for (i = 0; i < count; i++) {
            (*freeFunc) (ptr);
            ptr += elem_sz;
        }
    }
    if (freeAll) {
        (*num_inout) = (*sz_inout) = 0;
        if (*elems) {
            _XkbFree(*elems);
            *elems = NULL;
        }
    }
    else if (first + count >= (*num_inout))
        *num_inout = first;
    else {
        i = ((*num_inout) - (first + count)) * elem_sz;
        ptr = *elems;
        memmove(&ptr[(unsigned) first * elem_sz], &ptr[(unsigned)(first + count) * elem_sz], (size_t) i);
        (*num_inout) -= count;
    }
    return;
}

/***====================================================================***/

static void
_XkbClearProperty(char *prop_in)
{
    XkbPropertyPtr prop = (XkbPropertyPtr) prop_in;

    if (prop->name) {
        _XkbFree(prop->name);
        prop->name = NULL;
    }
    if (prop->value) {
        _XkbFree(prop->value);
        prop->value = NULL;
    }
    return;
}

void
XkbFreeGeomProperties(XkbGeometryPtr geom, int first, int count, Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll, first, count,
                             &geom->num_properties, &geom->sz_properties,
                             (char **) &geom->properties,
                             sizeof(XkbPropertyRec), _XkbClearProperty);
    return;
}

/***====================================================================***/

void
XkbFreeGeomKeyAliases(XkbGeometryPtr geom, int first, int count, Bool freeAll)
{
    _XkbFreeGeomLeafElems(freeAll, first, count,
                          &geom->num_key_aliases, &geom->sz_key_aliases,
                          (char **) &geom->key_aliases,
                          sizeof(XkbKeyAliasRec));
    return;
}

/***====================================================================***/

static void
_XkbClearColor(char *color_in)
{
    XkbColorPtr color = (XkbColorPtr) color_in;

    _XkbFree(color->spec);
    return;
}

void
XkbFreeGeomColors(XkbGeometryPtr geom, int first, int count, Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll, first, count,
                             &geom->num_colors, &geom->sz_colors,
                             (char **) &geom->colors,
                             sizeof(XkbColorRec), _XkbClearColor);
    return;
}

/***====================================================================***/

void
XkbFreeGeomPoints(XkbOutlinePtr outline, int first, int count, Bool freeAll)
{
    _XkbFreeGeomLeafElems(freeAll, first, count,
                          &outline->num_points, &outline->sz_points,
                          (char **) &outline->points,
                          sizeof(XkbPointRec));
    return;
}

/***====================================================================***/

static void
_XkbClearOutline(char *outline_in)
{
    XkbOutlinePtr outline = (XkbOutlinePtr) outline_in;

    if (outline->points != NULL)
        XkbFreeGeomPoints(outline, 0, outline->num_points, True);
    return;
}

void
XkbFreeGeomOutlines(XkbShapePtr shape, int first, int count, Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll, first, count,
                             &shape->num_outlines, &shape->sz_outlines,
                             (char **) &shape->outlines,
                             sizeof(XkbOutlineRec), _XkbClearOutline);

    return;
}

/***====================================================================***/

static void
_XkbClearShape(char *shape_in)
{
    XkbShapePtr shape = (XkbShapePtr) shape_in;

    if (shape->outlines)
        XkbFreeGeomOutlines(shape, 0, shape->num_outlines, True);
    return;
}

void
XkbFreeGeomShapes(XkbGeometryPtr geom, int first, int count, Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll, first, count,
                             &geom->num_shapes, &geom->sz_shapes,
                             (char **) &geom->shapes,
                             sizeof(XkbShapeRec), _XkbClearShape);
    return;
}

/***====================================================================***/

void
XkbFreeGeomOverlayKeys(XkbOverlayRowPtr row, int first, int count, Bool freeAll)
{
    _XkbFreeGeomLeafElems(freeAll, first, count,
                          &row->num_keys, &row->sz_keys,
                          (char **) &row->keys,
                          sizeof(XkbOverlayKeyRec));
    return;
}

/***====================================================================***/

static void
_XkbClearOverlayRow(char *row_in)
{
    XkbOverlayRowPtr row = (XkbOverlayRowPtr) row_in;

    if (row->keys != NULL)
        XkbFreeGeomOverlayKeys(row, 0, row->num_keys, True);
    return;
}

void
XkbFreeGeomOverlayRows(XkbOverlayPtr overlay, int first, int count,
                       Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll, first, count,
                             &overlay->num_rows, &overlay->sz_rows,
                             (char **) &overlay->rows,
                             sizeof(XkbOverlayRowRec), _XkbClearOverlayRow);
    return;
}

/***====================================================================***/

static void
_XkbClearOverlay(char *overlay_in)
{
    XkbOverlayPtr overlay = (XkbOverlayPtr) overlay_in;

    if (overlay->rows != NULL)
        XkbFreeGeomOverlayRows(overlay, 0, overlay->num_rows, True);
    return;
}

void
XkbFreeGeomOverlays(XkbSectionPtr section, int first, int count, Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll, first, count,
                             &section->num_overlays, &section->sz_overlays,
                             (char **) &section->overlays,
                             sizeof(XkbOverlayRec), _XkbClearOverlay);
    return;
}

/***====================================================================***/

void
XkbFreeGeomKeys(XkbRowPtr row, int first, int count, Bool freeAll)
{
    _XkbFreeGeomLeafElems(freeAll, first, count,
                          &row->num_keys, &row->sz_keys,
                          (char **) &row->keys,
                          sizeof(XkbKeyRec));
    return;
}

/***====================================================================***/

static void
_XkbClearRow(char *row_in)
{
    XkbRowPtr row = (XkbRowPtr) row_in;

    if (row->keys != NULL)
        XkbFreeGeomKeys(row, 0, row->num_keys, True);
    return;
}

void
XkbFreeGeomRows(XkbSectionPtr section, int first, int count, Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll, first, count,
                             &section->num_rows, &section->sz_rows,
                             (char **) &section->rows,
                             sizeof(XkbRowRec), _XkbClearRow);
}

/***====================================================================***/

static void
_XkbClearSection(char *section_in)
{
    XkbSectionPtr section = (XkbSectionPtr) section_in;

    if (section->rows != NULL)
        XkbFreeGeomRows(section, 0, section->num_rows, True);
    if (section->doodads != NULL) {
        XkbFreeGeomDoodads(section->doodads, section->num_doodads, True);
        section->doodads = NULL;
    }
    return;
}

void
XkbFreeGeomSections(XkbGeometryPtr geom, int first, int count, Bool freeAll)
{
    _XkbFreeGeomNonLeafElems(freeAll, first, count,
                             &geom->num_sections, &geom->sz_sections,
                             (char **) &geom->sections,
                             sizeof(XkbSectionRec), _XkbClearSection);
    return;
}

/***====================================================================***/

static void
_XkbClearDoodad(char *doodad_in)
{
    XkbDoodadPtr doodad = (XkbDoodadPtr) doodad_in;

    switch (doodad->any.type) {
    case XkbTextDoodad:
    {
        if (doodad->text.text != NULL) {
            _XkbFree(doodad->text.text);
            doodad->text.text = NULL;
        }
        if (doodad->text.font != NULL) {
            _XkbFree(doodad->text.font);
            doodad->text.font = NULL;
        }
    }
        break;
    case XkbLogoDoodad:
    {
        if (doodad->logo.logo_name != NULL) {
            _XkbFree(doodad->logo.logo_name);
            doodad->logo.logo_name = NULL;
        }
    }
        break;
    }
    return;
}

void
XkbFreeGeomDoodads(XkbDoodadPtr doodads, int nDoodads, Bool freeAll)
{
    register int i;
    register XkbDoodadPtr doodad;

    if (doodads) {
        for (i = 0, doodad = doodads; i < nDoodads; i++, doodad++) {
            _XkbClearDoodad((char *) doodad);
        }
        if (freeAll)
            _XkbFree(doodads);
    }
    return;
}

void
XkbFreeGeometry(XkbGeometryPtr geom, unsigned which, Bool freeMap)
{
    if (geom == NULL)
        return;
    if (freeMap)
        which = XkbGeomAllMask;
    if ((which & XkbGeomPropertiesMask) && (geom->properties != NULL))
        XkbFreeGeomProperties(geom, 0, geom->num_properties, True);
    if ((which & XkbGeomColorsMask) && (geom->colors != NULL))
        XkbFreeGeomColors(geom, 0, geom->num_colors, True);
    if ((which & XkbGeomShapesMask) && (geom->shapes != NULL))
        XkbFreeGeomShapes(geom, 0, geom->num_shapes, True);
    if ((which & XkbGeomSectionsMask) && (geom->sections != NULL))
        XkbFreeGeomSections(geom, 0, geom->num_sections, True);
    if ((which & XkbGeomDoodadsMask) && (geom->doodads != NULL)) {
        XkbFreeGeomDoodads(geom->doodads, geom->num_doodads, True);
        geom->doodads = NULL;
        geom->num_doodads = geom->sz_doodads = 0;
    }
    if ((which & XkbGeomKeyAliasesMask) && (geom->key_aliases != NULL))
        XkbFreeGeomKeyAliases(geom, 0, geom->num_key_aliases, True);
    if (freeMap) {
        if (geom->label_font != NULL) {
            _XkbFree(geom->label_font);
            geom->label_font = NULL;
        }
        _XkbFree(geom);
    }
    return;
}

/***====================================================================***/

static Status
_XkbGeomAlloc(XPointer *old,
              unsigned short *num,
              unsigned short *total,
              int num_new,
              size_t sz_elem)
{
    if (num_new < 1)
        return Success;
    if ((*old) == NULL)
        *num = *total = 0;

    if ((*num) + num_new <= (*total))
        return Success;

    *total = (*num) + num_new;
    if ((*old) != NULL)
        (*old) = (XPointer) _XkbRealloc((*old), (*total) * sz_elem);
    else
        (*old) = (XPointer) _XkbCalloc((*total), sz_elem);
    if ((*old) == NULL) {
        *total = *num = 0;
        return BadAlloc;
    }

    if (*num > 0) {
        char *tmp = (char *) (*old);
        bzero(&tmp[sz_elem * (*num)], (num_new * sz_elem));
    }
    return Success;
}

#define _XkbAllocProps(g, n) _XkbGeomAlloc((XPointer *)&(g)->properties, \
                                &(g)->num_properties, &(g)->sz_properties, \
                                (n), sizeof(XkbPropertyRec))
#define _XkbAllocColors(g, n) _XkbGeomAlloc((XPointer *)&(g)->colors, \
                                &(g)->num_colors, &(g)->sz_colors, \
                                (n), sizeof(XkbColorRec))
#define _XkbAllocShapes(g, n) _XkbGeomAlloc((XPointer *)&(g)->shapes, \
                                &(g)->num_shapes, &(g)->sz_shapes, \
                                (n), sizeof(XkbShapeRec))
#define _XkbAllocSections(g, n) _XkbGeomAlloc((XPointer *)&(g)->sections, \
                                &(g)->num_sections, &(g)->sz_sections, \
                                (n), sizeof(XkbSectionRec))
#define _XkbAllocDoodads(g, n) _XkbGeomAlloc((XPointer *)&(g)->doodads, \
                                &(g)->num_doodads, &(g)->sz_doodads, \
                                (n), sizeof(XkbDoodadRec))
#define _XkbAllocKeyAliases(g, n) _XkbGeomAlloc((XPointer *)&(g)->key_aliases, \
                                &(g)->num_key_aliases, &(g)->sz_key_aliases, \
                                (n), sizeof(XkbKeyAliasRec))

#define _XkbAllocOutlines(s, n) _XkbGeomAlloc((XPointer *)&(s)->outlines, \
                                &(s)->num_outlines, &(s)->sz_outlines, \
                                (n), sizeof(XkbOutlineRec))
#define _XkbAllocRows(s, n) _XkbGeomAlloc((XPointer *)&(s)->rows, \
                                &(s)->num_rows, &(s)->sz_rows, \
                                (n), sizeof(XkbRowRec))
#define _XkbAllocPoints(o, n) _XkbGeomAlloc((XPointer *)&(o)->points, \
                                &(o)->num_points, &(o)->sz_points, \
                                (n), sizeof(XkbPointRec))
#define _XkbAllocKeys(r, n) _XkbGeomAlloc((XPointer *)&(r)->keys, \
                                &(r)->num_keys, &(r)->sz_keys, \
                                (n), sizeof(XkbKeyRec))
#define _XkbAllocOverlays(s, n) _XkbGeomAlloc((XPointer *)&(s)->overlays, \
                                &(s)->num_overlays, &(s)->sz_overlays, \
                                (n), sizeof(XkbOverlayRec))
#define _XkbAllocOverlayRows(o, n) _XkbGeomAlloc((XPointer *)&(o)->rows, \
                                &(o)->num_rows, &(o)->sz_rows, \
                                (n), sizeof(XkbOverlayRowRec))
#define _XkbAllocOverlayKeys(r, n) _XkbGeomAlloc((XPointer *)&(r)->keys, \
                                &(r)->num_keys, &(r)->sz_keys, \
                                (n), sizeof(XkbOverlayKeyRec))

Status
XkbAllocGeomProps(XkbGeometryPtr geom, int nProps)
{
    return _XkbAllocProps(geom, nProps);
}

Status
XkbAllocGeomColors(XkbGeometryPtr geom, int nColors)
{
    return _XkbAllocColors(geom, nColors);
}

Status
XkbAllocGeomKeyAliases(XkbGeometryPtr geom, int nKeyAliases)
{
    return _XkbAllocKeyAliases(geom, nKeyAliases);
}

Status
XkbAllocGeomShapes(XkbGeometryPtr geom, int nShapes)
{
    return _XkbAllocShapes(geom, nShapes);
}

Status
XkbAllocGeomSections(XkbGeometryPtr geom, int nSections)
{
    return _XkbAllocSections(geom, nSections);
}

Status
XkbAllocGeomOverlays(XkbSectionPtr section, int nOverlays)
{
    return _XkbAllocOverlays(section, nOverlays);
}

Status
XkbAllocGeomOverlayRows(XkbOverlayPtr overlay, int nRows)
{
    return _XkbAllocOverlayRows(overlay, nRows);
}

Status
XkbAllocGeomOverlayKeys(XkbOverlayRowPtr row, int nKeys)
{
    return _XkbAllocOverlayKeys(row, nKeys);
}

Status
XkbAllocGeomDoodads(XkbGeometryPtr geom, int nDoodads)
{
    return _XkbAllocDoodads(geom, nDoodads);
}

Status
XkbAllocGeomSectionDoodads(XkbSectionPtr section, int nDoodads)
{
    return _XkbAllocDoodads(section, nDoodads);
}

Status
XkbAllocGeomOutlines(XkbShapePtr shape, int nOL)
{
    return _XkbAllocOutlines(shape, nOL);
}

Status
XkbAllocGeomRows(XkbSectionPtr section, int nRows)
{
    return _XkbAllocRows(section, nRows);
}

Status
XkbAllocGeomPoints(XkbOutlinePtr ol, int nPts)
{
    return _XkbAllocPoints(ol, nPts);
}

Status
XkbAllocGeomKeys(XkbRowPtr row, int nKeys)
{
    return _XkbAllocKeys(row, nKeys);
}

Status
XkbAllocGeometry(XkbDescPtr xkb, XkbGeometrySizesPtr sizes)
{
    XkbGeometryPtr geom;
    Status rtrn;

    if (xkb->geom == NULL) {
        xkb->geom = _XkbTypedCalloc(1, XkbGeometryRec);
        if (!xkb->geom)
            return BadAlloc;
    }
    geom = xkb->geom;
    if ((sizes->which & XkbGeomPropertiesMask) &&
        ((rtrn = _XkbAllocProps(geom, sizes->num_properties)) != Success)) {
        goto BAIL;
    }
    if ((sizes->which & XkbGeomColorsMask) &&
        ((rtrn = _XkbAllocColors(geom, sizes->num_colors)) != Success)) {
        goto BAIL;
    }
    if ((sizes->which & XkbGeomShapesMask) &&
        ((rtrn = _XkbAllocShapes(geom, sizes->num_shapes)) != Success)) {
        goto BAIL;
    }
    if ((sizes->which & XkbGeomSectionsMask) &&
        ((rtrn = _XkbAllocSections(geom, sizes->num_sections)) != Success)) {
        goto BAIL;
    }
    if ((sizes->which & XkbGeomDoodadsMask) &&
        ((rtrn = _XkbAllocDoodads(geom, sizes->num_doodads)) != Success)) {
        goto BAIL;
    }
    if ((sizes->which & XkbGeomKeyAliasesMask) &&
        ((rtrn = _XkbAllocKeyAliases(geom, sizes->num_key_aliases))
         != Success)) {
        goto BAIL;
    }
    return Success;
 BAIL:
    XkbFreeGeometry(geom, XkbGeomAllMask, True);
    xkb->geom = NULL;
    return rtrn;
}

/***====================================================================***/

XkbPropertyPtr
XkbAddGeomProperty(XkbGeometryPtr geom, _Xconst char *name, _Xconst char *value)
{
    register int i;
    register XkbPropertyPtr prop;

    if ((!geom) || (!name) || (!value))
        return NULL;
    for (i = 0, prop = geom->properties; i < geom->num_properties; i++, prop++) {
        if ((prop->name) && (strcmp(name, prop->name) == 0)) {
            _XkbFree(prop->value);
            prop->value = strdup(value);
            return prop;
        }
    }
    if ((geom->num_properties >= geom->sz_properties) &&
        (_XkbAllocProps(geom, 1) != Success)) {
        return NULL;
    }
    prop = &geom->properties[geom->num_properties];
    prop->name = strdup(name);
    if (!prop->name)
        return NULL;
    prop->value = strdup(value);
    if (!prop->value) {
        _XkbFree(prop->name);
        prop->name = NULL;
        return NULL;
    }
    geom->num_properties++;
    return prop;
}

XkbKeyAliasPtr
XkbAddGeomKeyAlias(XkbGeometryPtr geom, _Xconst char *aliasStr,
                   _Xconst char *realStr)
{
    register int i;
    register XkbKeyAliasPtr alias;

    if ((!geom) || (!aliasStr) || (!realStr) || (!aliasStr[0]) || (!realStr[0]))
        return NULL;
    for (i = 0, alias = geom->key_aliases; i < geom->num_key_aliases;
         i++, alias++) {
        if (strncmp(alias->alias, aliasStr, XkbKeyNameLength) == 0) {
            bzero(alias->real, XkbKeyNameLength);
            strncpy(alias->real, realStr, XkbKeyNameLength);
            return alias;
        }
    }
    if ((geom->num_key_aliases >= geom->sz_key_aliases) &&
        (_XkbAllocKeyAliases(geom, 1) != Success)) {
        return NULL;
    }
    alias = &geom->key_aliases[geom->num_key_aliases];
    bzero(alias, sizeof(XkbKeyAliasRec));
    strncpy(alias->alias, aliasStr, XkbKeyNameLength);
    strncpy(alias->real, realStr, XkbKeyNameLength);
    geom->num_key_aliases++;
    return alias;
}

XkbColorPtr
XkbAddGeomColor(XkbGeometryPtr geom, _Xconst char *spec, unsigned int pixel)
{
    register int i;
    register XkbColorPtr color;

    if ((!geom) || (!spec))
        return NULL;
    for (i = 0, color = geom->colors; i < geom->num_colors; i++, color++) {
        if ((color->spec) && (strcmp(color->spec, spec) == 0)) {
            color->pixel = pixel;
            return color;
        }
    }
    if ((geom->num_colors >= geom->sz_colors) &&
        (_XkbAllocColors(geom, 1) != Success)) {
        return NULL;
    }
    color = &geom->colors[geom->num_colors];
    color->pixel = pixel;
    color->spec = strdup(spec);
    if (!color->spec)
        return NULL;
    geom->num_colors++;
    return color;
}

XkbOutlinePtr
XkbAddGeomOutline(XkbShapePtr shape, int sz_points)
{
    XkbOutlinePtr outline;

    if ((!shape) || (sz_points < 0))
        return NULL;
    if ((shape->num_outlines >= shape->sz_outlines) &&
        (_XkbAllocOutlines(shape, 1) != Success)) {
        return NULL;
    }
    outline = &shape->outlines[shape->num_outlines];
    bzero(outline, sizeof(XkbOutlineRec));
    if ((sz_points > 0) && (_XkbAllocPoints(outline, sz_points) != Success))
        return NULL;
    shape->num_outlines++;
    return outline;
}

XkbShapePtr
XkbAddGeomShape(XkbGeometryPtr geom, Atom name, int sz_outlines)
{
    XkbShapePtr shape;
    register int i;

    if ((!geom) || (!name) || (sz_outlines < 0))
        return NULL;
    if (geom->num_shapes > 0) {
        for (shape = geom->shapes, i = 0; i < geom->num_shapes; i++, shape++) {
            if (name == shape->name)
                return shape;
        }
    }
    if ((geom->num_shapes >= geom->sz_shapes) &&
        (_XkbAllocShapes(geom, 1) != Success))
        return NULL;
    shape = &geom->shapes[geom->num_shapes];
    bzero(shape, sizeof(XkbShapeRec));
    if ((sz_outlines > 0) && (_XkbAllocOutlines(shape, sz_outlines) != Success))
        return NULL;
    shape->name = name;
    shape->primary = shape->approx = NULL;
    geom->num_shapes++;
    return shape;
}

XkbKeyPtr
XkbAddGeomKey(XkbRowPtr row)
{
    XkbKeyPtr key;

    if (!row)
        return NULL;
    if ((row->num_keys >= row->sz_keys) && (_XkbAllocKeys(row, 1) != Success))
        return NULL;
    key = &row->keys[row->num_keys++];
    bzero(key, sizeof(XkbKeyRec));
    return key;
}

XkbRowPtr
XkbAddGeomRow(XkbSectionPtr section, int sz_keys)
{
    XkbRowPtr row;

    if ((!section) || (sz_keys < 0))
        return NULL;
    if ((section->num_rows >= section->sz_rows) &&
        (_XkbAllocRows(section, 1) != Success))
        return NULL;
    row = &section->rows[section->num_rows];
    bzero(row, sizeof(XkbRowRec));
    if ((sz_keys > 0) && (_XkbAllocKeys(row, sz_keys) != Success))
        return NULL;
    section->num_rows++;
    return row;
}

XkbSectionPtr
XkbAddGeomSection(XkbGeometryPtr geom,
                  Atom name,
                  int sz_rows,
                  int sz_doodads,
                  int sz_over)
{
    register int i;
    XkbSectionPtr section;

    if ((!geom) || (name == None) || (sz_rows < 0))
        return NULL;
    for (i = 0, section = geom->sections; i < geom->num_sections;
         i++, section++) {
        if (section->name != name)
            continue;
        if (((sz_rows > 0) && (_XkbAllocRows(section, sz_rows) != Success)) ||
            ((sz_doodads > 0) &&
             (_XkbAllocDoodads(section, sz_doodads) != Success)) ||
            ((sz_over > 0) && (_XkbAllocOverlays(section, sz_over) != Success)))
            return NULL;
        return section;
    }
    if ((geom->num_sections >= geom->sz_sections) &&
        (_XkbAllocSections(geom, 1) != Success))
        return NULL;
    section = &geom->sections[geom->num_sections];
    if ((sz_rows > 0) && (_XkbAllocRows(section, sz_rows) != Success))
        return NULL;
    if ((sz_doodads > 0) && (_XkbAllocDoodads(section, sz_doodads) != Success)) {
        if (section->rows) {
            _XkbFree(section->rows);
            section->rows = NULL;
            section->sz_rows = section->num_rows = 0;
        }
        return NULL;
    }
    section->name = name;
    geom->num_sections++;
    return section;
}

XkbDoodadPtr
XkbAddGeomDoodad(XkbGeometryPtr geom, XkbSectionPtr section, Atom name)
{
    XkbDoodadPtr old, doodad;
    register int i, nDoodads;

    if ((!geom) || (name == None))
        return NULL;
    if ((section != NULL) && (section->num_doodads > 0)) {
        old = section->doodads;
        nDoodads = section->num_doodads;
    }
    else {
        old = geom->doodads;
        nDoodads = geom->num_doodads;
    }
    for (i = 0, doodad = old; i < nDoodads; i++, doodad++) {
        if (doodad->any.name == name)
            return doodad;
    }
    if (section) {
        if ((section->num_doodads >= geom->sz_doodads) &&
            (_XkbAllocDoodads(section, 1) != Success)) {
            return NULL;
        }
        doodad = &section->doodads[section->num_doodads++];
    }
    else {
        if ((geom->num_doodads >= geom->sz_doodads) &&
            (_XkbAllocDoodads(geom, 1) != Success))
            return NULL;
        doodad = &geom->doodads[geom->num_doodads++];
    }
    bzero(doodad, sizeof(XkbDoodadRec));
    doodad->any.name = name;
    return doodad;
}

XkbOverlayKeyPtr
XkbAddGeomOverlayKey(XkbOverlayPtr overlay,
                     XkbOverlayRowPtr row,
                     _Xconst char *over,
                     _Xconst char *under)
{
    register int i;
    XkbOverlayKeyPtr key;
    XkbSectionPtr section;
    XkbRowPtr row_under;
    Bool found;

    if ((!overlay) || (!row) || (!over) || (!under))
        return NULL;
    section = overlay->section_under;
    if (row->row_under >= section->num_rows)
        return NULL;
    row_under = &section->rows[row->row_under];
    for (i = 0, found = False; i < row_under->num_keys; i++) {
        if (strncmp(under, row_under->keys[i].name.name, XkbKeyNameLength) == 0) {
            found = True;
            break;
        }
    }
    if (!found)
        return NULL;
    if ((row->num_keys >= row->sz_keys) &&
        (_XkbAllocOverlayKeys(row, 1) != Success))
        return NULL;
    key = &row->keys[row->num_keys];
    strncpy(key->under.name, under, XkbKeyNameLength);
    strncpy(key->over.name, over, XkbKeyNameLength);
    row->num_keys++;
    return key;
}

XkbOverlayRowPtr
XkbAddGeomOverlayRow(XkbOverlayPtr overlay, int row_under, int sz_keys)
{
    register int i;
    XkbOverlayRowPtr row;

    if ((!overlay) || (sz_keys < 0))
        return NULL;
    if (row_under >= overlay->section_under->num_rows)
        return NULL;
    for (i = 0; i < overlay->num_rows; i++) {
        if (overlay->rows[i].row_under == row_under) {
            row = &overlay->rows[i];
            if ((row->sz_keys < sz_keys) &&
                (_XkbAllocOverlayKeys(row, sz_keys) != Success)) {
                return NULL;
            }
            return &overlay->rows[i];
        }
    }
    if ((overlay->num_rows >= overlay->sz_rows) &&
        (_XkbAllocOverlayRows(overlay, 1) != Success))
        return NULL;
    row = &overlay->rows[overlay->num_rows];
    bzero(row, sizeof(XkbOverlayRowRec));
    if ((sz_keys > 0) && (_XkbAllocOverlayKeys(row, sz_keys) != Success))
        return NULL;
    row->row_under = row_under;
    overlay->num_rows++;
    return row;
}

XkbOverlayPtr
XkbAddGeomOverlay(XkbSectionPtr section, Atom name, int sz_rows)
{
    register int i;
    XkbOverlayPtr overlay;

    if ((!section) || (name == None) || (sz_rows == 0))
        return NULL;

    for (i = 0, overlay = section->overlays; i < section->num_overlays;
         i++, overlay++) {
        if (overlay->name == name) {
            if ((sz_rows > 0) &&
                (_XkbAllocOverlayRows(overlay, sz_rows) != Success))
                return NULL;
            return overlay;
        }
    }
    if ((section->num_overlays >= section->sz_overlays) &&
        (_XkbAllocOverlays(section, 1) != Success))
        return NULL;
    overlay = &section->overlays[section->num_overlays];
    if ((sz_rows > 0) && (_XkbAllocOverlayRows(overlay, sz_rows) != Success))
        return NULL;
    overlay->name = name;
    overlay->section_under = section;
    section->num_overlays++;
    return overlay;
}
