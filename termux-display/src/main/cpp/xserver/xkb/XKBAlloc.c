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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <stdio.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "inputstr.h"
#include <xkbsrv.h>
#include "xkbgeom.h"
#include <os.h>
#include <string.h>

/***===================================================================***/

 /*ARGSUSED*/ Status
XkbAllocCompatMap(XkbDescPtr xkb, unsigned which, unsigned nSI)
{
    XkbCompatMapPtr compat;
    XkbSymInterpretRec *prev_interpret;

    if (!xkb)
        return BadMatch;
    if (xkb->compat) {
        if (xkb->compat->size_si >= nSI)
            return Success;
        compat = xkb->compat;
        compat->size_si = nSI;
        if (compat->sym_interpret == NULL)
            compat->num_si = 0;
        prev_interpret = compat->sym_interpret;
        compat->sym_interpret = reallocarray(compat->sym_interpret,
                                             nSI, sizeof(XkbSymInterpretRec));
        if (compat->sym_interpret == NULL) {
            free(prev_interpret);
            compat->size_si = compat->num_si = 0;
            return BadAlloc;
        }
        if (compat->num_si != 0) {
            memset(&compat->sym_interpret[compat->num_si], 0,
                   (compat->size_si -
                    compat->num_si) * sizeof(XkbSymInterpretRec));
        }
        return Success;
    }
    compat = calloc(1, sizeof(XkbCompatMapRec));
    if (compat == NULL)
        return BadAlloc;
    if (nSI > 0) {
        compat->sym_interpret = calloc(nSI, sizeof(XkbSymInterpretRec));
        if (!compat->sym_interpret) {
            free(compat);
            return BadAlloc;
        }
    }
    compat->size_si = nSI;
    compat->num_si = 0;
    memset((char *) &compat->groups[0], 0,
           XkbNumKbdGroups * sizeof(XkbModsRec));
    xkb->compat = compat;
    return Success;
}

void
XkbFreeCompatMap(XkbDescPtr xkb, unsigned which, Bool freeMap)
{
    register XkbCompatMapPtr compat;

    if ((xkb == NULL) || (xkb->compat == NULL))
        return;
    compat = xkb->compat;
    if (freeMap)
        which = XkbAllCompatMask;
    if (which & XkbGroupCompatMask)
        memset((char *) &compat->groups[0], 0,
               XkbNumKbdGroups * sizeof(XkbModsRec));
    if (which & XkbSymInterpMask) {
        if ((compat->sym_interpret) && (compat->size_si > 0))
            free(compat->sym_interpret);
        compat->size_si = compat->num_si = 0;
        compat->sym_interpret = NULL;
    }
    if (freeMap) {
        free(compat);
        xkb->compat = NULL;
    }
    return;
}

/***===================================================================***/

Status
XkbAllocNames(XkbDescPtr xkb, unsigned which, int nTotalRG, int nTotalAliases)
{
    XkbNamesPtr names;

    if (xkb == NULL)
        return BadMatch;
    if (xkb->names == NULL) {
        xkb->names = calloc(1, sizeof(XkbNamesRec));
        if (xkb->names == NULL)
            return BadAlloc;
    }
    names = xkb->names;
    if ((which & XkbKTLevelNamesMask) && (xkb->map != NULL) &&
        (xkb->map->types != NULL)) {
        register int i;
        XkbKeyTypePtr type;

        type = xkb->map->types;
        for (i = 0; i < xkb->map->num_types; i++, type++) {
            if (type->level_names == NULL) {
                type->level_names = calloc(type->num_levels, sizeof(Atom));
                if (type->level_names == NULL)
                    return BadAlloc;
            }
        }
    }
    if ((which & XkbKeyNamesMask) && (names->keys == NULL)) {
        if ((!XkbIsLegalKeycode(xkb->min_key_code)) ||
            (!XkbIsLegalKeycode(xkb->max_key_code)) ||
            (xkb->max_key_code < xkb->min_key_code))
            return BadValue;
        names->keys = calloc((xkb->max_key_code + 1), sizeof(XkbKeyNameRec));
        if (names->keys == NULL)
            return BadAlloc;
    }
    if ((which & XkbKeyAliasesMask) && (nTotalAliases > 0)) {
        if (names->key_aliases == NULL) {
            names->key_aliases = calloc(nTotalAliases, sizeof(XkbKeyAliasRec));
        }
        else if (nTotalAliases > names->num_key_aliases) {
            XkbKeyAliasRec *prev_aliases = names->key_aliases;

            names->key_aliases = reallocarray(names->key_aliases,
                                              nTotalAliases,
                                              sizeof(XkbKeyAliasRec));
            if (names->key_aliases != NULL) {
                memset(&names->key_aliases[names->num_key_aliases], 0,
                       (nTotalAliases -
                        names->num_key_aliases) * sizeof(XkbKeyAliasRec));
            }
            else {
                free(prev_aliases);
            }
        }
        if (names->key_aliases == NULL) {
            names->num_key_aliases = 0;
            return BadAlloc;
        }
        names->num_key_aliases = nTotalAliases;
    }
    if ((which & XkbRGNamesMask) && (nTotalRG > 0)) {
        if (names->radio_groups == NULL) {
            names->radio_groups = calloc(nTotalRG, sizeof(Atom));
        }
        else if (nTotalRG > names->num_rg) {
            Atom *prev_radio_groups = names->radio_groups;

            names->radio_groups = reallocarray(names->radio_groups,
                                               nTotalRG, sizeof(Atom));
            if (names->radio_groups != NULL) {
                memset(&names->radio_groups[names->num_rg], 0,
                       (nTotalRG - names->num_rg) * sizeof(Atom));
            }
            else {
                free(prev_radio_groups);
            }
        }
        if (names->radio_groups == NULL)
            return BadAlloc;
        names->num_rg = nTotalRG;
    }
    return Success;
}

void
XkbFreeNames(XkbDescPtr xkb, unsigned which, Bool freeMap)
{
    XkbNamesPtr names;

    if ((xkb == NULL) || (xkb->names == NULL))
        return;
    names = xkb->names;
    if (freeMap)
        which = XkbAllNamesMask;
    if (which & XkbKTLevelNamesMask) {
        XkbClientMapPtr map = xkb->map;

        if ((map != NULL) && (map->types != NULL)) {
            register int i;
            register XkbKeyTypePtr type;

            type = map->types;
            for (i = 0; i < map->num_types; i++, type++) {
                free(type->level_names);
                type->level_names = NULL;
            }
        }
    }
    if ((which & XkbKeyNamesMask) && (names->keys != NULL)) {
        free(names->keys);
        names->keys = NULL;
        names->num_keys = 0;
    }
    if ((which & XkbKeyAliasesMask) && (names->key_aliases)) {
        free(names->key_aliases);
        names->key_aliases = NULL;
        names->num_key_aliases = 0;
    }
    if ((which & XkbRGNamesMask) && (names->radio_groups)) {
        free(names->radio_groups);
        names->radio_groups = NULL;
        names->num_rg = 0;
    }
    if (freeMap) {
        free(names);
        xkb->names = NULL;
    }
    return;
}

/***===================================================================***/

 /*ARGSUSED*/ Status
XkbAllocControls(XkbDescPtr xkb, unsigned which)
{
    if (xkb == NULL)
        return BadMatch;

    if (xkb->ctrls == NULL) {
        xkb->ctrls = calloc(1, sizeof(XkbControlsRec));
        if (!xkb->ctrls)
            return BadAlloc;
    }
    return Success;
}

 /*ARGSUSED*/ static void
XkbFreeControls(XkbDescPtr xkb, unsigned which, Bool freeMap)
{
    if (freeMap && (xkb != NULL) && (xkb->ctrls != NULL)) {
        free(xkb->ctrls);
        xkb->ctrls = NULL;
    }
    return;
}

/***===================================================================***/

Status
XkbAllocIndicatorMaps(XkbDescPtr xkb)
{
    if (xkb == NULL)
        return BadMatch;
    if (xkb->indicators == NULL) {
        xkb->indicators = calloc(1, sizeof(XkbIndicatorRec));
        if (!xkb->indicators)
            return BadAlloc;
    }
    return Success;
}

static void
XkbFreeIndicatorMaps(XkbDescPtr xkb)
{
    if ((xkb != NULL) && (xkb->indicators != NULL)) {
        free(xkb->indicators);
        xkb->indicators = NULL;
    }
    return;
}

/***====================================================================***/

XkbDescRec *
XkbAllocKeyboard(void)
{
    XkbDescRec *xkb;

    xkb = calloc(1, sizeof(XkbDescRec));
    if (xkb)
        xkb->device_spec = XkbUseCoreKbd;
    return xkb;
}

void
XkbFreeKeyboard(XkbDescPtr xkb, unsigned which, Bool freeAll)
{
    if (xkb == NULL)
        return;
    if (freeAll)
        which = XkbAllComponentsMask;
    if (which & XkbClientMapMask)
        XkbFreeClientMap(xkb, XkbAllClientInfoMask, TRUE);
    if (which & XkbServerMapMask)
        XkbFreeServerMap(xkb, XkbAllServerInfoMask, TRUE);
    if (which & XkbCompatMapMask)
        XkbFreeCompatMap(xkb, XkbAllCompatMask, TRUE);
    if (which & XkbIndicatorMapMask)
        XkbFreeIndicatorMaps(xkb);
    if (which & XkbNamesMask)
        XkbFreeNames(xkb, XkbAllNamesMask, TRUE);
    if ((which & XkbGeometryMask) && (xkb->geom != NULL)) {
        XkbFreeGeometry(xkb->geom, XkbGeomAllMask, TRUE);
        /* PERHAPS BONGHITS etc */
        xkb->geom = NULL;
    }
    if (which & XkbControlsMask)
        XkbFreeControls(xkb, XkbAllControlsMask, TRUE);
    if (freeAll)
        free(xkb);
    return;
}

/***====================================================================***/

void
XkbFreeComponentNames(XkbComponentNamesPtr names, Bool freeNames)
{
    if (names) {
        free(names->keycodes);
        free(names->types);
        free(names->compat);
        free(names->symbols);
        free(names->geometry);
        memset(names, 0, sizeof(XkbComponentNamesRec));
    }
    if (freeNames)
        free(names);
}
