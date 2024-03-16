/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

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
#include <ctype.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include "XKMformat.h"
#include "XKBfileInt.h"

Bool
XkbWriteToServer(XkbFileInfo *result)
{
    XkbDescPtr xkb;
    Display *dpy;

    if ((result == NULL) || (result->xkb == NULL) || (result->xkb->dpy == NULL))
        return False;
    xkb = result->xkb;
    dpy = xkb->dpy;
    if (!XkbSetMap(dpy, XkbAllMapComponentsMask, xkb))
        return False;
    if (!XkbSetIndicatorMap(dpy, ~0, xkb))
        return False;
    if (!XkbSetCompatMap(dpy, XkbAllCompatMask, xkb, True))
        return False;
    if (!XkbSetNames(dpy, XkbAllNamesMask, 0, xkb->map->num_types, xkb))
        return False;
    if (xkb->geom) {
        if (XkbSetGeometry(dpy, xkb->device_spec, xkb->geom) != Success)
            return False;
    }
    return True;
}

unsigned
XkbReadFromServer(Display *dpy, unsigned need, unsigned want,
                  XkbFileInfo *result)
{
    unsigned which = need | want;
    unsigned tmp = 0;

    if ((result == NULL) || (dpy == NULL))
        return which;

    if (which & XkmSymbolsMask)
        tmp = XkbAllMapComponentsMask;
    else if (which & XkmTypesMask)
        tmp = XkbKeyTypesMask;
    if (result->xkb == NULL) {
        result->xkb = XkbGetMap(dpy, tmp, XkbUseCoreKbd);
        if (!result->xkb)
            return which;
        else
            which &= ~(XkmSymbolsMask | XkmTypesMask | XkmVirtualModsMask);
    }
    else if ((tmp) && (XkbGetUpdatedMap(dpy, tmp, result->xkb) == Success))
        which &= ~(XkmSymbolsMask | XkmTypesMask | XkmVirtualModsMask);

    if (which & XkmIndicatorsMask) {
        if (XkbGetIndicatorMap(dpy, XkbAllIndicatorsMask, result->xkb) ==
            Success)
            which &= ~XkmIndicatorsMask;
    }

    if (which & XkmCompatMapMask) {
        if (XkbGetCompatMap(dpy, XkbAllCompatMask, result->xkb) == Success)
            which &= ~XkmCompatMapMask;
    }
    if (which & XkmGeometryMask) {
        if (XkbGetGeometry(dpy, result->xkb) == Success)
            which &= ~XkmGeometryMask;
    }
    XkbGetNames(dpy, XkbAllNamesMask, result->xkb);
    return which;
}

Status
XkbChangeKbdDisplay(Display *newDpy, XkbFileInfo *result)
{
    register int i;
    XkbDescPtr xkb;
    Display *oldDpy;
    Atom *atm;

    if ((result->xkb == NULL) || (result->xkb->dpy == newDpy))
        return Success;
    xkb = result->xkb;
    oldDpy = xkb->dpy;
    if (xkb->names) {
        XkbNamesPtr names = xkb->names;

        names->keycodes = XkbChangeAtomDisplay(oldDpy, newDpy, names->keycodes);
        names->geometry = XkbChangeAtomDisplay(oldDpy, newDpy, names->geometry);
        names->symbols = XkbChangeAtomDisplay(oldDpy, newDpy, names->symbols);
        names->types = XkbChangeAtomDisplay(oldDpy, newDpy, names->types);
        names->compat = XkbChangeAtomDisplay(oldDpy, newDpy, names->compat);
        names->phys_symbols = XkbChangeAtomDisplay(oldDpy, newDpy,
                                                   names->phys_symbols);
        for (i = 0, atm = names->vmods; i < XkbNumVirtualMods; i++, atm++) {
            *atm = XkbChangeAtomDisplay(oldDpy, newDpy, *atm);
        }
        for (i = 0, atm = names->indicators; i < XkbNumIndicators; i++, atm++) {
            *atm = XkbChangeAtomDisplay(oldDpy, newDpy, *atm);
        }
        for (i = 0, atm = names->groups; i < XkbNumKbdGroups; i++, atm++) {
            *atm = XkbChangeAtomDisplay(oldDpy, newDpy, *atm);
        }
        for (i = 0, atm = names->radio_groups; i < names->num_rg; i++, atm++) {
            *atm = XkbChangeAtomDisplay(oldDpy, newDpy, *atm);
        }
    }
    if (xkb->map) {
        register int t;
        XkbKeyTypePtr type;

        for (t = 0, type = xkb->map->types; t < xkb->map->num_types;
             t++, type++) {
            type->name = XkbChangeAtomDisplay(oldDpy, newDpy, type->name);
            if (type->level_names != NULL) {
                for (i = 0, atm = type->level_names; i < type->num_levels;
                     i++, atm++) {
                    *atm = XkbChangeAtomDisplay(oldDpy, newDpy, *atm);
                }
            }
        }
    }
    if (xkb->geom) {
        XkbGeometryPtr geom = xkb->geom;

        geom->name = XkbChangeAtomDisplay(oldDpy, newDpy, geom->name);
        if (geom->shapes) {
            register int s;
            XkbShapePtr shape;

            for (s = 0, shape = geom->shapes; s < geom->num_shapes;
                 s++, shape++) {
                shape->name = XkbChangeAtomDisplay(oldDpy, newDpy, shape->name);
            }
        }
        if (geom->sections) {
            register int s;
            XkbSectionPtr section;

            for (s = 0, section = geom->sections; s < geom->num_sections;
                 s++, section++) {
                section->name =
                    XkbChangeAtomDisplay(oldDpy, newDpy, section->name);
                if (section->doodads) {
                    register int d;
                    XkbDoodadPtr doodad;

                    for (d = 0, doodad = section->doodads;
                         d < section->num_doodads; d++, doodad++) {
                        doodad->any.name =
                            XkbChangeAtomDisplay(oldDpy, newDpy,
                                                 doodad->any.name);
                    }
                }
                if (section->overlays) {
                    register int o;
                    register XkbOverlayPtr ol;

                    for (o = 0, ol = section->overlays;
                         o < section->num_overlays; o++, ol++) {
                        ol->name =
                            XkbChangeAtomDisplay(oldDpy, newDpy, ol->name);
                    }
                }
            }
        }
        if (geom->doodads) {
            register int d;
            XkbDoodadPtr doodad;

            for (d = 0, doodad = geom->doodads; d < geom->num_doodads;
                 d++, doodad++) {
                doodad->any.name =
                    XkbChangeAtomDisplay(oldDpy, newDpy, doodad->any.name);
            }
        }
    }
    xkb->dpy = newDpy;
    return Success;
}
