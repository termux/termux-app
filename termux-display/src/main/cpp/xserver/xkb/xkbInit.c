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

#include <xkb-config.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <X11/X.h>
#include <X11/Xproto.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include "misc.h"
#include "inputstr.h"
#include "opaque.h"
#include "property.h"
#include "scrnintstr.h"
#define	XKBSRV_NEED_FILE_FUNCS
#include <xkbsrv.h>
#include "xkbgeom.h"
#include <X11/extensions/XKMformat.h>
#include "xkbfile.h"
#include "xkb.h"

#define	CREATE_ATOM(s)	MakeAtom(s,sizeof(s)-1,1)

#if defined(__alpha) || defined(__alpha__)
#define	LED_COMPOSE	2
#define LED_CAPS	3
#define	LED_SCROLL	4
#define	LED_NUM		5
#define	PHYS_LEDS	0x1f
#else
#ifdef __sun
#define LED_NUM		1
#define	LED_SCROLL	2
#define	LED_COMPOSE	3
#define LED_CAPS	4
#define	PHYS_LEDS	0x0f
#else
#define	LED_CAPS	1
#define	LED_NUM		2
#define	LED_SCROLL	3
#define	PHYS_LEDS	0x07
#endif
#endif

#define	MAX_TOC	16
typedef struct _SrvXkmInfo {
    DeviceIntPtr dev;
    FILE *file;
    XkbDescPtr xkb;
} SrvXkmInfo;

/***====================================================================***/

#ifndef XKB_DFLT_RULES_PROP
#define	XKB_DFLT_RULES_PROP	TRUE
#endif

const char *XkbBaseDirectory = XKB_BASE_DIRECTORY;
const char *XkbBinDirectory = XKB_BIN_DIRECTORY;
static int XkbWantAccessX = 0;

static char *XkbRulesDflt = NULL;
static char *XkbModelDflt = NULL;
static char *XkbLayoutDflt = NULL;
static char *XkbVariantDflt = NULL;
static char *XkbOptionsDflt = NULL;

static char *XkbRulesUsed = NULL;
static char *XkbModelUsed = NULL;
static char *XkbLayoutUsed = NULL;
static char *XkbVariantUsed = NULL;
static char *XkbOptionsUsed = NULL;

static XkbDescPtr xkb_cached_map = NULL;

static Bool XkbWantRulesProp = XKB_DFLT_RULES_PROP;

/***====================================================================***/

/**
 * Get the current default XKB rules.
 * Caller must free the data in rmlvo.
 */
void
XkbGetRulesDflts(XkbRMLVOSet * rmlvo)
{
    rmlvo->rules = strdup(XkbRulesDflt ? XkbRulesDflt : XKB_DFLT_RULES);
    rmlvo->model = strdup(XkbModelDflt ? XkbModelDflt : XKB_DFLT_MODEL);
    rmlvo->layout = strdup(XkbLayoutDflt ? XkbLayoutDflt : XKB_DFLT_LAYOUT);
    rmlvo->variant = strdup(XkbVariantDflt ? XkbVariantDflt : XKB_DFLT_VARIANT);
    rmlvo->options = strdup(XkbOptionsDflt ? XkbOptionsDflt : XKB_DFLT_OPTIONS);
}

void
XkbFreeRMLVOSet(XkbRMLVOSet * rmlvo, Bool freeRMLVO)
{
    if (!rmlvo)
        return;

    free(rmlvo->rules);
    free(rmlvo->model);
    free(rmlvo->layout);
    free(rmlvo->variant);
    free(rmlvo->options);

    if (freeRMLVO)
        free(rmlvo);
    else
        memset(rmlvo, 0, sizeof(XkbRMLVOSet));
}

static Bool
XkbWriteRulesProp(void)
{
    int len, out;
    Atom name;
    char *pval;

    len = (XkbRulesUsed ? strlen(XkbRulesUsed) : 0);
    len += (XkbModelUsed ? strlen(XkbModelUsed) : 0);
    len += (XkbLayoutUsed ? strlen(XkbLayoutUsed) : 0);
    len += (XkbVariantUsed ? strlen(XkbVariantUsed) : 0);
    len += (XkbOptionsUsed ? strlen(XkbOptionsUsed) : 0);
    if (len < 1)
        return TRUE;

    len += 5;                   /* trailing NULs */

    name =
        MakeAtom(_XKB_RF_NAMES_PROP_ATOM, strlen(_XKB_RF_NAMES_PROP_ATOM), 1);
    if (name == None) {
        ErrorF("[xkb] Atom error: %s not created\n", _XKB_RF_NAMES_PROP_ATOM);
        return TRUE;
    }
    pval = (char *) malloc(len);
    if (!pval) {
        ErrorF("[xkb] Allocation error: %s proprerty not created\n",
               _XKB_RF_NAMES_PROP_ATOM);
        return TRUE;
    }
    out = 0;
    if (XkbRulesUsed) {
        strcpy(&pval[out], XkbRulesUsed);
        out += strlen(XkbRulesUsed);
    }
    pval[out++] = '\0';
    if (XkbModelUsed) {
        strcpy(&pval[out], XkbModelUsed);
        out += strlen(XkbModelUsed);
    }
    pval[out++] = '\0';
    if (XkbLayoutUsed) {
        strcpy(&pval[out], XkbLayoutUsed);
        out += strlen(XkbLayoutUsed);
    }
    pval[out++] = '\0';
    if (XkbVariantUsed) {
        strcpy(&pval[out], XkbVariantUsed);
        out += strlen(XkbVariantUsed);
    }
    pval[out++] = '\0';
    if (XkbOptionsUsed) {
        strcpy(&pval[out], XkbOptionsUsed);
        out += strlen(XkbOptionsUsed);
    }
    pval[out++] = '\0';
    if (out != len) {
        ErrorF("[xkb] Internal Error! bad size (%d!=%d) for _XKB_RULES_NAMES\n",
               out, len);
    }
    dixChangeWindowProperty(serverClient, screenInfo.screens[0]->root, name,
                            XA_STRING, 8, PropModeReplace, len, pval, TRUE);
    free(pval);
    return TRUE;
}

void
XkbInitRules(XkbRMLVOSet *rmlvo,
             const char *rules,
             const char *model,
             const char *layout,
             const char *variant,
             const char *options)
{
    rmlvo->rules = rules ? xnfstrdup(rules) : NULL;
    rmlvo->model = model ? xnfstrdup(model) : NULL;
    rmlvo->layout = layout ? xnfstrdup(layout) : NULL;
    rmlvo->variant = variant ? xnfstrdup(variant) : NULL;
    rmlvo->options = options ? xnfstrdup(options) : NULL;
}

static void
XkbSetRulesUsed(XkbRMLVOSet * rmlvo)
{
    free(XkbRulesUsed);
    XkbRulesUsed = (rmlvo->rules ? Xstrdup(rmlvo->rules) : NULL);
    free(XkbModelUsed);
    XkbModelUsed = (rmlvo->model ? Xstrdup(rmlvo->model) : NULL);
    free(XkbLayoutUsed);
    XkbLayoutUsed = (rmlvo->layout ? Xstrdup(rmlvo->layout) : NULL);
    free(XkbVariantUsed);
    XkbVariantUsed = (rmlvo->variant ? Xstrdup(rmlvo->variant) : NULL);
    free(XkbOptionsUsed);
    XkbOptionsUsed = (rmlvo->options ? Xstrdup(rmlvo->options) : NULL);
    if (XkbWantRulesProp)
        XkbWriteRulesProp();
    return;
}

void
XkbSetRulesDflts(XkbRMLVOSet * rmlvo)
{
    if (rmlvo->rules) {
        free(XkbRulesDflt);
        XkbRulesDflt = Xstrdup(rmlvo->rules);
    }
    if (rmlvo->model) {
        free(XkbModelDflt);
        XkbModelDflt = Xstrdup(rmlvo->model);
    }
    if (rmlvo->layout) {
        free(XkbLayoutDflt);
        XkbLayoutDflt = Xstrdup(rmlvo->layout);
    }
    if (rmlvo->variant) {
        free(XkbVariantDflt);
        XkbVariantDflt = Xstrdup(rmlvo->variant);
    }
    if (rmlvo->options) {
        free(XkbOptionsDflt);
        XkbOptionsDflt = Xstrdup(rmlvo->options);
    }
    return;
}

void
XkbDeleteRulesUsed(void)
{
    free(XkbRulesUsed);
    XkbRulesUsed = NULL;
    free(XkbModelUsed);
    XkbModelUsed = NULL;
    free(XkbLayoutUsed);
    XkbLayoutUsed = NULL;
    free(XkbVariantUsed);
    XkbVariantUsed = NULL;
    free(XkbOptionsUsed);
    XkbOptionsUsed = NULL;
}

void
XkbDeleteRulesDflts(void)
{
    free(XkbRulesDflt);
    XkbRulesDflt = NULL;
    free(XkbModelDflt);
    XkbModelDflt = NULL;
    free(XkbLayoutDflt);
    XkbLayoutDflt = NULL;
    free(XkbVariantDflt);
    XkbVariantDflt = NULL;
    free(XkbOptionsDflt);
    XkbOptionsDflt = NULL;

    XkbFreeKeyboard(xkb_cached_map, XkbAllComponentsMask, TRUE);
    xkb_cached_map = NULL;
}

#define DIFFERS(a, b) (strcmp((a) ? (a) : "", (b) ? (b) : "") != 0)

static Bool
XkbCompareUsedRMLVO(XkbRMLVOSet * rmlvo)
{
    if (DIFFERS(rmlvo->rules, XkbRulesUsed) ||
        DIFFERS(rmlvo->model, XkbModelUsed) ||
        DIFFERS(rmlvo->layout, XkbLayoutUsed) ||
        DIFFERS(rmlvo->variant, XkbVariantUsed) ||
        DIFFERS(rmlvo->options, XkbOptionsUsed))
        return FALSE;
    return TRUE;
}

#undef DIFFERS

/***====================================================================***/

#include "xkbDflts.h"

static Bool
XkbInitKeyTypes(XkbDescPtr xkb)
{
    if (xkb->defined & XkmTypesMask)
        return TRUE;

    initTypeNames(NULL);
    if (XkbAllocClientMap(xkb, XkbKeyTypesMask, num_dflt_types) != Success)
        return FALSE;
    if (XkbCopyKeyTypes(dflt_types, xkb->map->types, num_dflt_types) != Success) {
        return FALSE;
    }
    xkb->map->size_types = xkb->map->num_types = num_dflt_types;
    return TRUE;
}

static void
XkbInitRadioGroups(XkbSrvInfoPtr xkbi)
{
    xkbi->nRadioGroups = 0;
    xkbi->radioGroups = NULL;
    return;
}

static Status
XkbInitCompatStructs(XkbDescPtr xkb)
{
    register int i;
    XkbCompatMapPtr compat;

    if (xkb->defined & XkmCompatMapMask)
        return TRUE;

    if (XkbAllocCompatMap(xkb, XkbAllCompatMask, num_dfltSI) != Success)
        return BadAlloc;
    compat = xkb->compat;
    if (compat->sym_interpret) {
        compat->num_si = num_dfltSI;
        memcpy((char *) compat->sym_interpret, (char *) dfltSI, sizeof(dfltSI));
    }
    for (i = 0; i < XkbNumKbdGroups; i++) {
        compat->groups[i] = compatMap.groups[i];
        if (compat->groups[i].vmods != 0) {
            unsigned mask;

            mask = XkbMaskForVMask(xkb, compat->groups[i].vmods);
            compat->groups[i].mask = compat->groups[i].real_mods | mask;
        }
        else
            compat->groups[i].mask = compat->groups[i].real_mods;
    }
    return Success;
}

static void
XkbInitSemantics(XkbDescPtr xkb)
{
    XkbInitKeyTypes(xkb);
    XkbInitCompatStructs(xkb);
    return;
}

/***====================================================================***/

static Status
XkbInitNames(XkbSrvInfoPtr xkbi)
{
    XkbDescPtr xkb;
    XkbNamesPtr names;
    Status rtrn;
    Atom unknown;

    xkb = xkbi->desc;
    if ((rtrn = XkbAllocNames(xkb, XkbAllNamesMask, 0, 0)) != Success)
        return rtrn;
    unknown = CREATE_ATOM("unknown");
    names = xkb->names;
    if (names->keycodes == None)
        names->keycodes = unknown;
    if (names->geometry == None)
        names->geometry = unknown;
    if (names->phys_symbols == None)
        names->phys_symbols = unknown;
    if (names->symbols == None)
        names->symbols = unknown;
    if (names->types == None)
        names->types = unknown;
    if (names->compat == None)
        names->compat = unknown;
    if (!(xkb->defined & XkmVirtualModsMask)) {
        if (names->vmods[vmod_NumLock] == None)
            names->vmods[vmod_NumLock] = CREATE_ATOM("NumLock");
        if (names->vmods[vmod_Alt] == None)
            names->vmods[vmod_Alt] = CREATE_ATOM("Alt");
        if (names->vmods[vmod_AltGr] == None)
            names->vmods[vmod_AltGr] = CREATE_ATOM("ModeSwitch");
    }

    if (!(xkb->defined & XkmIndicatorsMask) ||
        !(xkb->defined & XkmGeometryMask)) {
        initIndicatorNames(NULL, xkb);
        if (names->indicators[LED_CAPS - 1] == None)
            names->indicators[LED_CAPS - 1] = CREATE_ATOM("Caps Lock");
        if (names->indicators[LED_NUM - 1] == None)
            names->indicators[LED_NUM - 1] = CREATE_ATOM("Num Lock");
        if (names->indicators[LED_SCROLL - 1] == None)
            names->indicators[LED_SCROLL - 1] = CREATE_ATOM("Scroll Lock");
#ifdef LED_COMPOSE
        if (names->indicators[LED_COMPOSE - 1] == None)
            names->indicators[LED_COMPOSE - 1] = CREATE_ATOM("Compose");
#endif
    }

    if (xkb->geom != NULL)
        names->geometry = xkb->geom->name;
    else
        names->geometry = unknown;

    return Success;
}

static Status
XkbInitIndicatorMap(XkbSrvInfoPtr xkbi)
{
    XkbDescPtr xkb;
    XkbIndicatorPtr map;
    XkbSrvLedInfoPtr sli;

    xkb = xkbi->desc;
    if (XkbAllocIndicatorMaps(xkb) != Success)
        return BadAlloc;

    if (!(xkb->defined & XkmIndicatorsMask)) {
        map = xkb->indicators;
        map->phys_indicators = PHYS_LEDS;
        map->maps[LED_CAPS - 1].flags = XkbIM_NoExplicit;
        map->maps[LED_CAPS - 1].which_mods = XkbIM_UseLocked;
        map->maps[LED_CAPS - 1].mods.mask = LockMask;
        map->maps[LED_CAPS - 1].mods.real_mods = LockMask;

        map->maps[LED_NUM - 1].flags = XkbIM_NoExplicit;
        map->maps[LED_NUM - 1].which_mods = XkbIM_UseLocked;
        map->maps[LED_NUM - 1].mods.mask = 0;
        map->maps[LED_NUM - 1].mods.real_mods = 0;
        map->maps[LED_NUM - 1].mods.vmods = vmod_NumLockMask;

        map->maps[LED_SCROLL - 1].flags = XkbIM_NoExplicit;
        map->maps[LED_SCROLL - 1].which_mods = XkbIM_UseLocked;
        map->maps[LED_SCROLL - 1].mods.mask = Mod3Mask;
        map->maps[LED_SCROLL - 1].mods.real_mods = Mod3Mask;
    }

    sli = XkbFindSrvLedInfo(xkbi->device, XkbDfltXIClass, XkbDfltXIId, 0);
    if (sli)
        XkbCheckIndicatorMaps(xkbi->device, sli, XkbAllIndicatorsMask);

    return Success;
}

static Status
XkbInitControls(DeviceIntPtr pXDev, XkbSrvInfoPtr xkbi)
{
    XkbDescPtr xkb;
    XkbControlsPtr ctrls;

    xkb = xkbi->desc;
    /* 12/31/94 (ef) -- XXX! Should check if controls loaded from file */
    if (XkbAllocControls(xkb, XkbAllControlsMask) != Success)
        FatalError("Couldn't allocate keyboard controls\n");
    ctrls = xkb->ctrls;
    if (!(xkb->defined & XkmSymbolsMask))
        ctrls->num_groups = 1;
    ctrls->groups_wrap = XkbSetGroupInfo(1, XkbWrapIntoRange, 0);
    ctrls->internal.mask = 0;
    ctrls->internal.real_mods = 0;
    ctrls->internal.vmods = 0;
    ctrls->ignore_lock.mask = 0;
    ctrls->ignore_lock.real_mods = 0;
    ctrls->ignore_lock.vmods = 0;
    ctrls->enabled_ctrls = XkbAccessXTimeoutMask | XkbRepeatKeysMask |
        XkbMouseKeysAccelMask | XkbAudibleBellMask | XkbIgnoreGroupLockMask;
    if (XkbWantAccessX)
        ctrls->enabled_ctrls |= XkbAccessXKeysMask;
    AccessXInit(pXDev);
    return Success;
}

static Status
XkbInitOverlayState(XkbSrvInfoPtr xkbi)
{
    memset(xkbi->overlay_perkey_state, 0, sizeof(xkbi->overlay_perkey_state));
    return Success;
}

static Bool
InitKeyboardDeviceStructInternal(DeviceIntPtr dev, XkbRMLVOSet * rmlvo,
                                 const char *keymap, int keymap_length,
                                 BellProcPtr bell_func, KbdCtrlProcPtr ctrl_func)
{
    int i;
    unsigned int check;
    XkbSrvInfoPtr xkbi;
    XkbDescPtr xkb;
    XkbSrvLedInfoPtr sli;
    XkbChangesRec changes;
    XkbEventCauseRec cause;
    XkbRMLVOSet rmlvo_dflts = { NULL };

    BUG_RETURN_VAL(dev == NULL, FALSE);
    BUG_RETURN_VAL(dev->key != NULL, FALSE);
    BUG_RETURN_VAL(dev->kbdfeed != NULL, FALSE);
    BUG_RETURN_VAL(rmlvo && keymap, FALSE);

    if (!rmlvo && !keymap) {
        rmlvo = &rmlvo_dflts;
        XkbGetRulesDflts(rmlvo);
    }

    memset(&changes, 0, sizeof(changes));
    XkbSetCauseUnknown(&cause);

    dev->key = calloc(1, sizeof(*dev->key));
    if (!dev->key) {
        ErrorF("XKB: Failed to allocate key class\n");
        return FALSE;
    }
    dev->key->sourceid = dev->id;

    dev->kbdfeed = calloc(1, sizeof(*dev->kbdfeed));
    if (!dev->kbdfeed) {
        ErrorF("XKB: Failed to allocate key feedback class\n");
        goto unwind_key;
    }

    xkbi = calloc(1, sizeof(*xkbi));
    if (!xkbi) {
        ErrorF("XKB: Failed to allocate XKB info\n");
        goto unwind_kbdfeed;
    }
    dev->key->xkbInfo = xkbi;

    if (xkb_cached_map && (keymap || (rmlvo && !XkbCompareUsedRMLVO(rmlvo)))) {
        XkbFreeKeyboard(xkb_cached_map, XkbAllComponentsMask, TRUE);
        xkb_cached_map = NULL;
    }

    if (xkb_cached_map)
        LogMessageVerb(X_INFO, 4, "XKB: Reusing cached keymap\n");
    else {
        if (rmlvo)
            xkb_cached_map = XkbCompileKeymap(dev, rmlvo);
        else
            xkb_cached_map = XkbCompileKeymapFromString(dev, keymap, keymap_length);

        if (!xkb_cached_map) {
            ErrorF("XKB: Failed to compile keymap\n");
            goto unwind_info;
        }
    }

    xkb = XkbAllocKeyboard();
    if (!xkb) {
        ErrorF("XKB: Failed to allocate keyboard description\n");
        goto unwind_info;
    }

    if (!XkbCopyKeymap(xkb, xkb_cached_map)) {
        ErrorF("XKB: Failed to copy keymap\n");
        goto unwind_desc;
    }
    xkb->defined = xkb_cached_map->defined;
    xkb->flags = xkb_cached_map->flags;
    xkb->device_spec = xkb_cached_map->device_spec;
    xkbi->desc = xkb;

    if (xkb->min_key_code == 0)
        xkb->min_key_code = 8;
    if (xkb->max_key_code == 0)
        xkb->max_key_code = 255;

    i = XkbNumKeys(xkb) / 3 + 1;
    if (XkbAllocClientMap(xkb, XkbAllClientInfoMask, 0) != Success)
        goto unwind_desc;
    if (XkbAllocServerMap(xkb, XkbAllServerInfoMask, i) != Success)
        goto unwind_desc;

    xkbi->dfltPtrDelta = 1;
    xkbi->device = dev;

    XkbInitSemantics(xkb);
    XkbInitNames(xkbi);
    XkbInitRadioGroups(xkbi);

    XkbInitControls(dev, xkbi);

    XkbInitIndicatorMap(xkbi);

    XkbInitOverlayState(xkbi);

    XkbUpdateActions(dev, xkb->min_key_code, XkbNumKeys(xkb), &changes,
                     &check, &cause);

    if (!dev->focus)
        InitFocusClassDeviceStruct(dev);

    xkbi->kbdProc = ctrl_func;
    dev->kbdfeed->BellProc = bell_func;
    dev->kbdfeed->CtrlProc = XkbDDXKeybdCtrlProc;

    dev->kbdfeed->ctrl = defaultKeyboardControl;
    if (dev->kbdfeed->ctrl.autoRepeat)
        xkb->ctrls->enabled_ctrls |= XkbRepeatKeysMask;

    memcpy(dev->kbdfeed->ctrl.autoRepeats, xkb->ctrls->per_key_repeat,
           XkbPerKeyBitArraySize);

    sli = XkbFindSrvLedInfo(dev, XkbDfltXIClass, XkbDfltXIId, 0);
    if (sli)
        XkbCheckIndicatorMaps(dev, sli, XkbAllIndicatorsMask);
    else
        DebugF("XKB: No indicator feedback in XkbFinishInit!\n");

    dev->kbdfeed->CtrlProc(dev, &dev->kbdfeed->ctrl);

    if (rmlvo) {
        XkbSetRulesDflts(rmlvo);
        XkbSetRulesUsed(rmlvo);
    }
    XkbFreeRMLVOSet(&rmlvo_dflts, FALSE);

    return TRUE;

 unwind_desc:
    XkbFreeKeyboard(xkb, 0, TRUE);
 unwind_info:
    free(xkbi);
    dev->key->xkbInfo = NULL;
 unwind_kbdfeed:
    free(dev->kbdfeed);
    dev->kbdfeed = NULL;
 unwind_key:
    free(dev->key);
    dev->key = NULL;
    return FALSE;
}

_X_EXPORT Bool
InitKeyboardDeviceStruct(DeviceIntPtr dev, XkbRMLVOSet * rmlvo,
                         BellProcPtr bell_func, KbdCtrlProcPtr ctrl_func)
{
    return InitKeyboardDeviceStructInternal(dev, rmlvo,
                                            NULL, 0, bell_func, ctrl_func);
}

_X_EXPORT Bool
InitKeyboardDeviceStructFromString(DeviceIntPtr dev,
                                   const char *keymap, int keymap_length,
                                   BellProcPtr bell_func, KbdCtrlProcPtr ctrl_func)
{
    return InitKeyboardDeviceStructInternal(dev, NULL,
                                            keymap, keymap_length,
                                            bell_func, ctrl_func);
}

/***====================================================================***/

        /*
         * Be very careful about what does and doesn't get freed by this
         * function.  To reduce fragmentation, XkbInitDevice allocates a
         * single huge block per device and divides it up into most of the
         * fixed-size structures for the device.   Don't free anything that
         * is part of this larger block.
         */
void
XkbFreeInfo(XkbSrvInfoPtr xkbi)
{
    free(xkbi->radioGroups);
    xkbi->radioGroups = NULL;
    if (xkbi->mouseKeyTimer) {
        TimerFree(xkbi->mouseKeyTimer);
        xkbi->mouseKeyTimer = NULL;
    }
    if (xkbi->slowKeysTimer) {
        TimerFree(xkbi->slowKeysTimer);
        xkbi->slowKeysTimer = NULL;
    }
    if (xkbi->bounceKeysTimer) {
        TimerFree(xkbi->bounceKeysTimer);
        xkbi->bounceKeysTimer = NULL;
    }
    if (xkbi->repeatKeyTimer) {
        TimerFree(xkbi->repeatKeyTimer);
        xkbi->repeatKeyTimer = NULL;
    }
    if (xkbi->krgTimer) {
        TimerFree(xkbi->krgTimer);
        xkbi->krgTimer = NULL;
    }
    xkbi->beepType = _BEEP_NONE;
    if (xkbi->beepTimer) {
        TimerFree(xkbi->beepTimer);
        xkbi->beepTimer = NULL;
    }
    if (xkbi->desc) {
        XkbFreeKeyboard(xkbi->desc, XkbAllComponentsMask, TRUE);
        xkbi->desc = NULL;
    }
    free(xkbi);
    return;
}

/***====================================================================***/

extern int XkbDfltRepeatDelay;
extern int XkbDfltRepeatInterval;

extern unsigned short XkbDfltAccessXTimeout;
extern unsigned int XkbDfltAccessXTimeoutMask;
extern unsigned int XkbDfltAccessXFeedback;
extern unsigned short XkbDfltAccessXOptions;

int
XkbProcessArguments(int argc, char *argv[], int i)
{
    if (strncmp(argv[i], "-xkbdir", 7) == 0) {
        if (++i < argc) {
#if !defined(WIN32) && !defined(__CYGWIN__)
            if (getuid() != geteuid()) {
                LogMessage(X_WARNING,
                           "-xkbdir is not available for setuid X servers\n");
                return -1;
            }
            else
#endif
            {
                if (strlen(argv[i]) < PATH_MAX) {
                    XkbBaseDirectory = argv[i];
                    return 2;
                }
                else {
                    LogMessage(X_ERROR, "-xkbdir pathname too long\n");
                    return -1;
                }
            }
        }
        else {
            return -1;
        }
    }
    else if ((strncmp(argv[i], "-accessx", 8) == 0) ||
             (strncmp(argv[i], "+accessx", 8) == 0)) {
        int j = 1;

        if (argv[i][0] == '-')
            XkbWantAccessX = 0;
        else {
            XkbWantAccessX = 1;

            if (((i + 1) < argc) && (isdigit(argv[i + 1][0]))) {
                XkbDfltAccessXTimeout = atoi(argv[++i]);
                j++;

                if (((i + 1) < argc) && (isdigit(argv[i + 1][0]))) {
                    /*
                     * presumption that the reasonably useful range of
                     * values fits in 0..MAXINT since SunOS 4 doesn't
                     * have strtoul.
                     */
                    XkbDfltAccessXTimeoutMask = (unsigned int)
                        strtol(argv[++i], NULL, 16);
                    j++;
                }
                if (((i + 1) < argc) && (isdigit(argv[i + 1][0]))) {
                    if (argv[++i][0] == '1')
                        XkbDfltAccessXFeedback = XkbAccessXFeedbackMask;
                    else
                        XkbDfltAccessXFeedback = 0;
                    j++;
                }
                if (((i + 1) < argc) && (isdigit(argv[i + 1][0]))) {
                    XkbDfltAccessXOptions = (unsigned short)
                        strtol(argv[++i], NULL, 16);
                    j++;
                }
            }
        }
        return j;
    }
    if ((strcmp(argv[i], "-ardelay") == 0) || (strcmp(argv[i], "-ar1") == 0)) { /* -ardelay int */
        if (++i >= argc)
            UseMsg();
        else
            XkbDfltRepeatDelay = (long) atoi(argv[i]);
        return 2;
    }
    if ((strcmp(argv[i], "-arinterval") == 0) || (strcmp(argv[i], "-ar2") == 0)) {      /* -arinterval int */
        if (++i >= argc)
            UseMsg();
        else
            XkbDfltRepeatInterval = (long) atoi(argv[i]);
        return 2;
    }
    return 0;
}

void
XkbUseMsg(void)
{
    ErrorF
        ("[+-]accessx [ timeout [ timeout_mask [ feedback [ options_mask] ] ] ]\n");
    ErrorF("                       enable/disable accessx key sequences\n");
    ErrorF("-ardelay               set XKB autorepeat delay\n");
    ErrorF("-arinterval            set XKB autorepeat interval\n");
}
