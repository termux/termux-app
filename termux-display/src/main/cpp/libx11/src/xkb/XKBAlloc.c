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
#include "XKBlibint.h"


/***===================================================================***/

/*ARGSUSED*/
Status
XkbAllocCompatMap(XkbDescPtr xkb, unsigned which, unsigned nSI)
{
    XkbCompatMapPtr compat;

    if (!xkb)
        return BadMatch;
    if (xkb->compat) {
        if (xkb->compat->size_si >= nSI)
            return Success;
        compat = xkb->compat;
        compat->size_si = nSI;
        if (compat->sym_interpret == NULL)
            compat->num_si = 0;
        _XkbResizeArray(compat->sym_interpret, compat->num_si,
                        nSI, XkbSymInterpretRec);
        if (compat->sym_interpret == NULL) {
            compat->size_si = compat->num_si = 0;
            return BadAlloc;
        }
        return Success;
    }
    compat = _XkbTypedCalloc(1, XkbCompatMapRec);
    if (compat == NULL)
        return BadAlloc;
    if (nSI > 0) {
        compat->sym_interpret = _XkbTypedCalloc(nSI, XkbSymInterpretRec);
        if (!compat->sym_interpret) {
            _XkbFree(compat);
            return BadAlloc;
        }
    }
    compat->size_si = nSI;
    compat->num_si = 0;
    bzero((char *) &compat->groups[0], XkbNumKbdGroups * sizeof(XkbModsRec));
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
        bzero(&compat->groups[0], XkbNumKbdGroups * sizeof(XkbModsRec));
    if (which & XkbSymInterpMask) {
        if ((compat->sym_interpret) && (compat->size_si > 0))
            _XkbFree(compat->sym_interpret);
        compat->size_si = compat->num_si = 0;
        compat->sym_interpret = NULL;
    }
    if (freeMap) {
        _XkbFree(compat);
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
        xkb->names = _XkbTypedCalloc(1, XkbNamesRec);
        if (xkb->names == NULL)
            return BadAlloc;
    }
    names = xkb->names;
    if ((which & XkbKTLevelNamesMask) && (xkb->map != NULL) &&
        (xkb->map->types != NULL)) {
        register int i;
        XkbKeyTypePtr type = xkb->map->types;

        for (i = 0; i < xkb->map->num_types; i++, type++) {
            if (type->level_names == NULL) {
                type->level_names = _XkbTypedCalloc(type->num_levels, Atom);
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
        names->keys = _XkbTypedCalloc((xkb->max_key_code + 1), XkbKeyNameRec);
        if (names->keys == NULL)
            return BadAlloc;
    }
    if ((which & XkbKeyAliasesMask) && (nTotalAliases > 0)) {
        if ((names->key_aliases == NULL) ||
            (nTotalAliases > names->num_key_aliases)) {
            _XkbResizeArray(names->key_aliases, names->num_key_aliases,
                            nTotalAliases, XkbKeyAliasRec);
        }
        if (names->key_aliases == NULL) {
            names->num_key_aliases = 0;
            return BadAlloc;
        }
        names->num_key_aliases = nTotalAliases;
    }
    if ((which & XkbRGNamesMask) && (nTotalRG > 0)) {
        if ((names->radio_groups == NULL) || (nTotalRG > names->num_rg)) {
            _XkbResizeArray(names->radio_groups, names->num_rg, nTotalRG, Atom);
        }
        if (names->radio_groups == NULL) {
            names->num_rg = 0;
            return BadAlloc;
        }
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
                    _XkbFree(type->level_names);
                    type->level_names = NULL;
            }
        }
    }
    if (which & XkbKeyNamesMask) {
        _XkbFree(names->keys);
        names->keys = NULL;
        names->num_keys = 0;
    }
    if (which & XkbKeyAliasesMask) {
        _XkbFree(names->key_aliases);
        names->key_aliases = NULL;
        names->num_key_aliases = 0;
    }
    if (which & XkbRGNamesMask) {
        _XkbFree(names->radio_groups);
        names->radio_groups = NULL;
        names->num_rg = 0;
    }
    if (freeMap) {
        _XkbFree(names);
        xkb->names = NULL;
    }
    return;
}

/***===================================================================***/

/*ARGSUSED*/
Status
XkbAllocControls(XkbDescPtr xkb, unsigned which)
{
    if (xkb == NULL)
        return BadMatch;

    if (xkb->ctrls == NULL) {
        xkb->ctrls = _XkbTypedCalloc(1, XkbControlsRec);
        if (!xkb->ctrls)
            return BadAlloc;
    }
    return Success;
}

/*ARGSUSED*/
void
XkbFreeControls(XkbDescPtr xkb, unsigned which, Bool freeMap)
{
    if (freeMap && (xkb != NULL) && (xkb->ctrls != NULL)) {
        _XkbFree(xkb->ctrls);
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
        xkb->indicators = _XkbTypedCalloc(1, XkbIndicatorRec);
        if (!xkb->indicators)
            return BadAlloc;
    }
    return Success;
}

void
XkbFreeIndicatorMaps(XkbDescPtr xkb)
{
    if ((xkb != NULL) && (xkb->indicators != NULL)) {
        _XkbFree(xkb->indicators);
        xkb->indicators = NULL;
    }
    return;
}

/***====================================================================***/

XkbDescRec *
XkbAllocKeyboard(void)
{
    XkbDescRec *xkb;

    xkb = _XkbTypedCalloc(1, XkbDescRec);
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
        XkbFreeClientMap(xkb, XkbAllClientInfoMask, True);
    if (which & XkbServerMapMask)
        XkbFreeServerMap(xkb, XkbAllServerInfoMask, True);
    if (which & XkbCompatMapMask)
        XkbFreeCompatMap(xkb, XkbAllCompatMask, True);
    if (which & XkbIndicatorMapMask)
        XkbFreeIndicatorMaps(xkb);
    if (which & XkbNamesMask)
        XkbFreeNames(xkb, XkbAllNamesMask, True);
    if ((which & XkbGeometryMask) && (xkb->geom != NULL))
        XkbFreeGeometry(xkb->geom, XkbGeomAllMask, True);
    if (which & XkbControlsMask)
        XkbFreeControls(xkb, XkbAllControlsMask, True);
    if (freeAll)
        _XkbFree(xkb);
    return;
}

/***====================================================================***/

XkbDeviceLedInfoPtr
XkbAddDeviceLedInfo(XkbDeviceInfoPtr devi, unsigned ledClass, unsigned ledId)
{
    XkbDeviceLedInfoPtr devli;
    register int i;

    if ((!devi) || (!XkbSingleXIClass(ledClass)) || (!XkbSingleXIId(ledId)))
        return NULL;
    for (i = 0, devli = devi->leds; i < devi->num_leds; i++, devli++) {
        if ((devli->led_class == ledClass) && (devli->led_id == ledId))
            return devli;
    }
    if (devi->num_leds >= devi->sz_leds) {
        if (devi->sz_leds > 0)
            devi->sz_leds *= 2;
        else
            devi->sz_leds = 1;
        _XkbResizeArray(devi->leds, devi->num_leds, devi->sz_leds,
                        XkbDeviceLedInfoRec);
        if (!devi->leds) {
            devi->sz_leds = devi->num_leds = 0;
            return NULL;
        }
        i = devi->num_leds;
        for (devli = &devi->leds[i]; i < devi->sz_leds; i++, devli++) {
            bzero(devli, sizeof(XkbDeviceLedInfoRec));
            devli->led_class = XkbXINone;
            devli->led_id = XkbXINone;
        }
    }
    devli = &devi->leds[devi->num_leds++];
    bzero(devli, sizeof(XkbDeviceLedInfoRec));
    devli->led_class = ledClass;
    devli->led_id = ledId;
    return devli;
}

Status
XkbResizeDeviceButtonActions(XkbDeviceInfoPtr devi, unsigned newTotal)
{
    if ((!devi) || (newTotal > 255))
        return BadValue;
    if ((devi->btn_acts != NULL) && (newTotal == devi->num_btns))
        return Success;
    if (newTotal == 0) {
        if (devi->btn_acts != NULL) {
            _XkbFree(devi->btn_acts);
            devi->btn_acts = NULL;
        }
        devi->num_btns = 0;
        return Success;
    }
    _XkbResizeArray(devi->btn_acts, devi->num_btns, newTotal, XkbAction);
    if (devi->btn_acts == NULL) {
        devi->num_btns = 0;
        return BadAlloc;
    }
    if (newTotal > devi->num_btns) {
        XkbAction *act;

        act = &devi->btn_acts[devi->num_btns];
        bzero((char *) act, (newTotal - devi->num_btns) * sizeof(XkbAction));
    }
    devi->num_btns = newTotal;
    return Success;
}

/*ARGSUSED*/
XkbDeviceInfoPtr
XkbAllocDeviceInfo(unsigned deviceSpec, unsigned nButtons, unsigned szLeds)
{
    XkbDeviceInfoPtr devi;

    devi = _XkbTypedCalloc(1, XkbDeviceInfoRec);
    if (devi != NULL) {
        devi->device_spec = deviceSpec;
        devi->has_own_state = False;
        devi->num_btns = 0;
        devi->btn_acts = NULL;
        if (nButtons > 0) {
            devi->num_btns = nButtons;
            devi->btn_acts = _XkbTypedCalloc(nButtons, XkbAction);
            if (!devi->btn_acts) {
                _XkbFree(devi);
                return NULL;
            }
        }
        devi->dflt_kbd_fb = XkbXINone;
        devi->dflt_led_fb = XkbXINone;
        devi->num_leds = 0;
        devi->sz_leds = 0;
        devi->leds = NULL;
        if (szLeds > 0) {
            devi->sz_leds = szLeds;
            devi->leds = _XkbTypedCalloc(szLeds, XkbDeviceLedInfoRec);
            if (!devi->leds) {
                _XkbFree(devi->btn_acts);
                _XkbFree(devi);
                return NULL;
            }
        }
    }
    return devi;
}


void
XkbFreeDeviceInfo(XkbDeviceInfoPtr devi, unsigned which, Bool freeDevI)
{
    if (devi) {
        if (freeDevI) {
            which = XkbXI_AllDeviceFeaturesMask;
            if (devi->name) {
                _XkbFree(devi->name);
                devi->name = NULL;
            }
        }
        if ((which & XkbXI_ButtonActionsMask) && (devi->btn_acts)) {
            _XkbFree(devi->btn_acts);
            devi->num_btns = 0;
            devi->btn_acts = NULL;
        }
        if ((which & XkbXI_IndicatorsMask) && (devi->leds)) {
            register int i;

            if ((which & XkbXI_IndicatorsMask) == XkbXI_IndicatorsMask) {
                _XkbFree(devi->leds);
                devi->sz_leds = devi->num_leds = 0;
                devi->leds = NULL;
            }
            else {
                XkbDeviceLedInfoPtr devli;

                for (i = 0, devli = devi->leds; i < devi->num_leds;
                     i++, devli++) {
                    if (which & XkbXI_IndicatorMapsMask)
                        bzero((char *) &devli->maps[0], sizeof(devli->maps));
                    else
                        bzero((char *) &devli->names[0], sizeof(devli->names));
                }
            }
        }
        if (freeDevI)
            _XkbFree(devi);
    }
    return;
}
