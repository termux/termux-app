/*

Copyright 1985, 1987, 1994, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

        /* the new monsters ate the old ones */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "XKBlib.h"
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <ctype.h>

#include <X11/extensions/XKBproto.h>
#include "XKBlibint.h"

#define AllMods (ShiftMask|LockMask|ControlMask| \
                 Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask)

static int _XkbLoadDpy(Display *dpy);

struct _XKeytrans {
    struct _XKeytrans *next;    /* next on list */
    char *string;               /* string to return when the time comes */
    int len;                    /* length of string (since NULL is legit) */
    KeySym key;                 /* keysym rebound */
    unsigned int state;         /* modifier state */
    KeySym *modifiers;          /* modifier keysyms you want */
    int mlen;                   /* length of modifier list */
};

KeySym
XkbKeycodeToKeysym(Display *dpy,
#if NeedWidePrototypes
                   unsigned int kc,
#else
                   KeyCode kc,
#endif
                   int group,
                   int level)
{
    XkbDescRec *xkb;

    if (_XkbUnavailable(dpy))
        return NoSymbol;

    _XkbCheckPendingRefresh(dpy, dpy->xkb_info);

    xkb = dpy->xkb_info->desc;
    if ((kc < xkb->min_key_code) || (kc > xkb->max_key_code))
        return NoSymbol;

    if ((group < 0) || (level < 0) || (group >= XkbKeyNumGroups(xkb, kc)))
        return NoSymbol;
    if (level >= XkbKeyGroupWidth(xkb, kc, group)) {
        /* for compatibility with the core protocol, _always_ allow  */
        /* two symbols in the first two groups.   If either of the   */
        /* two is of type ONE_LEVEL, just replicate the first symbol */
        if ((group > XkbGroup2Index) || (XkbKeyGroupWidth(xkb, kc, group) != 1)
            || (level != 1)) {
            return NoSymbol;
        }
        level = 0;
    }
    return XkbKeySymEntry(xkb, kc, level, group);
}

KeySym
XKeycodeToKeysym(Display *dpy,
#if NeedWidePrototypes
                 unsigned int kc,
#else
                 KeyCode kc,
#endif
                 int col)
{
    XkbDescRec *xkb;

    if (_XkbUnavailable(dpy))
        return _XKeycodeToKeysym(dpy, kc, col);

    _XkbCheckPendingRefresh(dpy, dpy->xkb_info);

    xkb = dpy->xkb_info->desc;
    if ((kc < xkb->min_key_code) || (kc > xkb->max_key_code))
        return NoSymbol;

    if (col > 3) {
        int firstSym, nGrp, grp;

        firstSym = 4;
        nGrp = XkbKeyNumGroups(xkb, kc);
        for (grp = 0; grp < nGrp; grp++) {
            int width = XkbKeyGroupWidth(xkb, kc, grp);
            int skip = 0;
            if (grp < 2) {
                /* Skip the first two symbols in the first two groups, since we
                 * return them below for indexes 0-3. */
                skip = 2;
                width -= skip;
                if (width < 0)
                    width = 0;
            }
            if (col < firstSym + width)
                return XkbKeycodeToKeysym(dpy, kc, grp, col - firstSym + skip);
            firstSym += width;
        }
        return NoSymbol;
    }
    return XkbKeycodeToKeysym(dpy, kc, (col >> 1), (col & 1));
}

KeyCode
XKeysymToKeycode(Display *dpy, KeySym ks)
{
    register int i, j, gotOne;

    if (_XkbUnavailable(dpy))
        return _XKeysymToKeycode(dpy, ks);
    _XkbCheckPendingRefresh(dpy, dpy->xkb_info);

    j = 0;
    do {
        register XkbDescRec *xkb = dpy->xkb_info->desc;
        gotOne = 0;
        for (i = dpy->min_keycode; i <= dpy->max_keycode; i++) {
            if (j < (int) XkbKeyNumSyms(xkb, i)) {
                gotOne = 1;
                if ((XkbKeySym(xkb, i, j) == ks))
                    return i;
            }
        }
        j++;
    } while (gotOne);
    return 0;
}

static int
_XkbComputeModmap(Display *dpy)
{
    register XkbDescPtr xkb;

    xkb = dpy->xkb_info->desc;
    if (XkbGetUpdatedMap(dpy, XkbModifierMapMask, xkb) == Success)
        return 1;
    return 0;
}

unsigned
XkbKeysymToModifiers(Display *dpy, KeySym ks)
{
    XkbDescRec *xkb;
    register int i, j;
    register KeySym *pSyms;
    CARD8 mods;

    if (_XkbUnavailable(dpy))
        return _XKeysymToModifiers(dpy, ks);
    _XkbCheckPendingRefresh(dpy, dpy->xkb_info);

    if (_XkbNeedModmap(dpy->xkb_info) && (!_XkbComputeModmap(dpy)))
        return _XKeysymToModifiers(dpy, ks);

    xkb = dpy->xkb_info->desc;
    mods = 0;
    for (i = xkb->min_key_code; i <= (int) xkb->max_key_code; i++) {
        pSyms = XkbKeySymsPtr(xkb, i);
        for (j = XkbKeyNumSyms(xkb, i) - 1; j >= 0; j--) {
            if (pSyms[j] == ks) {
                mods |= xkb->map->modmap[i];
                break;
            }
        }
    }
    return mods;
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

KeySym
XLookupKeysym(register XKeyEvent * event, int col)
{
    Display *dpy = event->display;

    if (_XkbUnavailable(dpy))
        return _XLookupKeysym(event, col);
    _XkbCheckPendingRefresh(dpy, dpy->xkb_info);

    return XKeycodeToKeysym(dpy, event->keycode, col);
}

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

   /*
    * Not a public entry point -- XkbTranslateKey is an obsolete name
    * that is preserved here so that functions linked against the old
    * version will continue to work in a shared library environment.
    */
int
XkbTranslateKey(register Display *dpy,
                KeyCode key,
                register unsigned int mods,
                unsigned int *mods_rtrn,
                KeySym *keysym_rtrn);

int
XkbTranslateKey(register Display *dpy,
                KeyCode key,
                register unsigned int mods,
                unsigned int *mods_rtrn,
                KeySym *keysym_rtrn)
{
    return XkbLookupKeySym(dpy, key, mods, mods_rtrn, keysym_rtrn);
}

Bool
XkbLookupKeySym(register Display *dpy,
                KeyCode key,
                register unsigned int mods,
                unsigned int *mods_rtrn,
                KeySym *keysym_rtrn)
{
    if (_XkbUnavailable(dpy))
        return _XTranslateKey(dpy, key, mods, mods_rtrn, keysym_rtrn);
    _XkbCheckPendingRefresh(dpy, dpy->xkb_info);
    return XkbTranslateKeyCode(dpy->xkb_info->desc, key, mods, mods_rtrn,
                               keysym_rtrn);
}

Bool
XkbTranslateKeyCode(register XkbDescPtr xkb,
                    KeyCode key,
                    register unsigned int mods,
                    unsigned int *mods_rtrn,
                    KeySym *keysym_rtrn)
{
    XkbKeyTypeRec *type;
    int col, nKeyGroups;
    unsigned preserve, effectiveGroup;
    KeySym *syms;

    if (mods_rtrn != NULL)
        *mods_rtrn = 0;

    nKeyGroups = XkbKeyNumGroups(xkb, key);
    if ((!XkbKeycodeInRange(xkb, key)) || (nKeyGroups == 0)) {
        if (keysym_rtrn != NULL)
            *keysym_rtrn = NoSymbol;
        return False;
    }

    syms = XkbKeySymsPtr(xkb, key);

    /* find the offset of the effective group */
    col = 0;
    effectiveGroup = XkbGroupForCoreState(mods);
    if (effectiveGroup >= nKeyGroups) {
        unsigned groupInfo = XkbKeyGroupInfo(xkb, key);

        switch (XkbOutOfRangeGroupAction(groupInfo)) {
        default:
            effectiveGroup %= nKeyGroups;
            break;
        case XkbClampIntoRange:
            effectiveGroup = nKeyGroups - 1;
            break;
        case XkbRedirectIntoRange:
            effectiveGroup = XkbOutOfRangeGroupNumber(groupInfo);
            if (effectiveGroup >= nKeyGroups)
                effectiveGroup = 0;
            break;
        }
    }
    col = effectiveGroup * XkbKeyGroupsWidth(xkb, key);
    type = XkbKeyKeyType(xkb, key, effectiveGroup);

    preserve = 0;
    if (type->map) {  /* find the column (shift level) within the group */
        register int i;
        register XkbKTMapEntryPtr entry;

        for (i = 0, entry = type->map; i < type->map_count; i++, entry++) {
            if ((entry->active) &&
                ((mods & type->mods.mask) == entry->mods.mask)) {
                col += entry->level;
                if (type->preserve)
                    preserve = type->preserve[i].mask;
                break;
            }
        }
    }

    if (keysym_rtrn != NULL)
        *keysym_rtrn = syms[col];
    if (mods_rtrn) {
        *mods_rtrn = type->mods.mask & (~preserve);
        /* The Motif VTS doesn't get the help callback called if help
         * is bound to Shift+<whatever>, and it appears as though it
         * is XkbTranslateKeyCode that is causing the problem.  The
         * core X version of XTranslateKey always OR's in ShiftMask
         * and LockMask for mods_rtrn, so this "fix" keeps this behavior
         * and solves the VTS problem.
         */
        if ((xkb->dpy) && (xkb->dpy->xkb_info) &&
            (xkb->dpy->xkb_info->
             xlib_ctrls & XkbLC_AlwaysConsumeShiftAndLock)) {
            *mods_rtrn |= (ShiftMask | LockMask);
        }
    }
    return (syms[col] != NoSymbol);
}

Status
XkbRefreshKeyboardMapping(register XkbMapNotifyEvent * event)
{
    Display *dpy = event->display;
    XkbInfoPtr xkbi;

    if (_XkbUnavailable(dpy)) {
        _XRefreshKeyboardMapping((XMappingEvent *) event);
        return Success;
    }
    xkbi = dpy->xkb_info;

    if (((event->type & 0x7f) - xkbi->codes->first_event) != XkbEventCode)
        return BadMatch;
    if (event->xkb_type == XkbNewKeyboardNotify) {
        _XkbReloadDpy(dpy);
        return Success;
    }
    if (event->xkb_type == XkbMapNotify) {
        XkbMapChangesRec changes;
        Status rtrn;

        if (xkbi->flags & XkbMapPending)
            changes = xkbi->changes;
        else
            bzero(&changes, sizeof(changes));
        XkbNoteMapChanges(&changes, event, XKB_XLIB_MAP_MASK);
        if ((rtrn = XkbGetMapChanges(dpy, xkbi->desc, &changes)) != Success) {
#ifdef DEBUG
            fprintf(stderr, "Internal Error! XkbGetMapChanges failed:\n");
#endif
            xkbi->changes = changes;
        }
        else if (xkbi->flags & XkbMapPending) {
            xkbi->flags &= ~XkbMapPending;
            bzero(&xkbi->changes, sizeof(XkbMapChangesRec));
        }
        return rtrn;
    }
    return BadMatch;
}

int
XRefreshKeyboardMapping(register XMappingEvent * event)
{
    XkbEvent *xkbevent = (XkbEvent *) event;
    Display *dpy = event->display;
    XkbMapChangesRec changes;
    XkbInfoPtr xkbi;

    /* always do this for input methods, which still use the old keymap */
    (void) _XRefreshKeyboardMapping(event);

    if (_XkbUnavailable(dpy))
        return 1;

    xkbi = dpy->xkb_info;

    if (((event->type & 0x7f) - xkbi->codes->first_event) == XkbEventCode)
        return XkbRefreshKeyboardMapping(&xkbevent->map);

    if (xkbi->flags & XkbXlibNewKeyboard) {
        _XkbReloadDpy(dpy);
        return 1;
    }

    if ((xkbi->flags & XkbMapPending) || (event->request == MappingKeyboard)) {
        if (xkbi->flags & XkbMapPending) {
            changes = xkbi->changes;
            _XkbNoteCoreMapChanges(&changes, event, XKB_XLIB_MAP_MASK);
        }
        else {
            bzero(&changes, sizeof(changes));
            changes.changed = XkbKeySymsMask;
            if (xkbi->desc->min_key_code < xkbi->desc->max_key_code) {
                changes.first_key_sym = xkbi->desc->min_key_code;
                changes.num_key_syms = xkbi->desc->max_key_code -
                    xkbi->desc->min_key_code + 1;
            }
            else {
                changes.first_key_sym = event->first_keycode;
                changes.num_key_syms = event->count;
            }
        }

        if (XkbGetMapChanges(dpy, xkbi->desc, &changes) != Success) {
#ifdef DEBUG
            fprintf(stderr, "Internal Error! XkbGetMapChanges failed:\n");
            if (changes.changed & XkbKeyTypesMask) {
                int first = changes.first_type;
                int last = changes.first_type + changes.num_types - 1;

                fprintf(stderr, "       types:  %d..%d\n", first, last);
            }
            if (changes.changed & XkbKeySymsMask) {
                int first = changes.first_key_sym;
                int last = changes.first_key_sym + changes.num_key_syms - 1;

                fprintf(stderr, "     symbols:  %d..%d\n", first, last);
            }
            if (changes.changed & XkbKeyActionsMask) {
                int first = changes.first_key_act;
                int last = changes.first_key_act + changes.num_key_acts - 1;

                fprintf(stderr, "     acts:  %d..%d\n", first, last);
            }
            if (changes.changed & XkbKeyBehaviorsMask) {
                int first = changes.first_key_behavior;
                int last = first + changes.num_key_behaviors - 1;

                fprintf(stderr, "   behaviors:  %d..%d\n", first, last);
            }
            if (changes.changed & XkbVirtualModsMask) {
                fprintf(stderr, "virtual mods: 0x%04x\n", changes.vmods);
            }
            if (changes.changed & XkbExplicitComponentsMask) {
                int first = changes.first_key_explicit;
                int last = first + changes.num_key_explicit - 1;

                fprintf(stderr, "    explicit:  %d..%d\n", first, last);
            }
#endif
        }
        LockDisplay(dpy);
        if (xkbi->flags & XkbMapPending) {
            xkbi->flags &= ~XkbMapPending;
            bzero(&xkbi->changes, sizeof(XkbMapChangesRec));
        }
        UnlockDisplay(dpy);
    }
    if (event->request == MappingModifier) {
        LockDisplay(dpy);
        if (xkbi->desc->map->modmap) {
            _XkbFree(xkbi->desc->map->modmap);
            xkbi->desc->map->modmap = NULL;
        }
        if (dpy->key_bindings) {
            register struct _XKeytrans *p;

            for (p = dpy->key_bindings; p; p = p->next) {
                register int i;

                p->state = 0;
                if (p->mlen > 0) {
                    for (i = 0; i < p->mlen; i++) {
                        p->state |= XkbKeysymToModifiers(dpy, p->modifiers[i]);
                    }
                    if (p->state)
                        p->state &= AllMods;
                    else
                        p->state = AnyModifier;
                }
            }
        }
        UnlockDisplay(dpy);
    }
    return 1;
}

static int
_XkbLoadDpy(Display *dpy)
{
    XkbInfoPtr xkbi;
    unsigned query, oldEvents;
    XkbDescRec *desc;

    if (!XkbUseExtension(dpy, NULL, NULL))
        return 0;

    xkbi = dpy->xkb_info;
    query = XkbAllClientInfoMask;
    desc = XkbGetMap(dpy, query, XkbUseCoreKbd);
    if (!desc) {
#ifdef DEBUG
        fprintf(stderr, "Warning! XkbGetMap failed!\n");
#endif
        return 0;
    }
    LockDisplay(dpy);
    xkbi->desc = desc;

    UnlockDisplay(dpy);
    oldEvents = xkbi->selected_events;
    if (!(xkbi->xlib_ctrls & XkbLC_IgnoreNewKeyboards)) {
        XkbSelectEventDetails(dpy, xkbi->desc->device_spec,
                              XkbNewKeyboardNotify,
                              XkbNKN_KeycodesMask | XkbNKN_DeviceIDMask,
                              XkbNKN_KeycodesMask | XkbNKN_DeviceIDMask);
    }
    XkbSelectEventDetails(dpy, xkbi->desc->device_spec, XkbMapNotify,
                          XkbAllClientInfoMask, XkbAllClientInfoMask);
    LockDisplay(dpy);
    xkbi->selected_events = oldEvents;
    UnlockDisplay(dpy);
    return 1;
}

void
_XkbReloadDpy(Display *dpy)
{
    XkbInfoPtr xkbi;
    XkbDescRec *desc;
    unsigned oldDeviceID;

    if (_XkbUnavailable(dpy))
        return;

    xkbi = dpy->xkb_info;
    LockDisplay(dpy);
    if (xkbi->desc) {
        oldDeviceID = xkbi->desc->device_spec;
        XkbFreeKeyboard(xkbi->desc, XkbAllComponentsMask, True);
        xkbi->desc = NULL;
        xkbi->flags &= ~(XkbMapPending | XkbXlibNewKeyboard);
        xkbi->changes.changed = 0;
    }
    else
        oldDeviceID = XkbUseCoreKbd;
    UnlockDisplay(dpy);
    desc = XkbGetMap(dpy, XkbAllClientInfoMask, XkbUseCoreKbd);
    if (!desc)
        return;
    LockDisplay(dpy);
    xkbi->desc = desc;
    UnlockDisplay(dpy);

    if (desc->device_spec != oldDeviceID) {
        /* transfer(?) event masks here */
#ifdef NOTYET
        unsigned oldEvents;

        oldEvents = xkbi->selected_events;
        XkbSelectEventDetails(dpy, xkbi->desc->device_spec, XkbMapNotify,
                              XkbAllMapComponentsMask, XkbAllClientInfoMask);
        LockDisplay(dpy);
        xkbi->selected_events = oldEvents;
        UnlockDisplay(dpy);
#endif
    }
    return;
}

int
XkbTranslateKeySym(Display *dpy,
                   KeySym *sym_rtrn,
                   unsigned int mods,
                   char *buffer,
                   int nbytes,
                   int *extra_rtrn)
{
    register XkbInfoPtr xkb;
    XkbKSToMBFunc cvtr;
    XPointer priv;
    char tmp[4];
    int n;

    xkb = dpy->xkb_info;
    if (!xkb->cvt.KSToMB) {
        _XkbGetConverters(_XkbGetCharset(), &xkb->cvt);
        _XkbGetConverters("ISO8859-1", &xkb->latin1cvt);
    }

    if (extra_rtrn)
        *extra_rtrn = 0;

    if ((buffer == NULL) || (nbytes == 0)) {
        buffer = tmp;
        nbytes = 4;
    }

    /* see if symbol rebound, if so, return that string. */
    n = XkbLookupKeyBinding(dpy, *sym_rtrn, mods, buffer, nbytes, extra_rtrn);
    if (n)
        return n;

    if (nbytes > 0)
        buffer[0] = '\0';

    if (xkb->cvt.KSToUpper && (mods & LockMask)) {
        *sym_rtrn = (*xkb->cvt.KSToUpper) (*sym_rtrn);
    }
    if (xkb->xlib_ctrls & XkbLC_ForceLatin1Lookup) {
        cvtr = xkb->latin1cvt.KSToMB;
        priv = xkb->latin1cvt.KSToMBPriv;
    }
    else {
        cvtr = xkb->cvt.KSToMB;
        priv = xkb->cvt.KSToMBPriv;
    }

    n = (*cvtr) (priv, *sym_rtrn, buffer, nbytes, extra_rtrn);

    if ((!xkb->cvt.KSToUpper) && (mods & LockMask)) {
        register int i;
        int change;

        for (i = change = 0; i < n; i++) {
            char ch = toupper(buffer[i]);
            change = (change || (buffer[i] != ch));
            buffer[i] = ch;
        }
        if (change) {
            if (n == 1)
                *sym_rtrn =
                    (*xkb->cvt.MBToKS) (xkb->cvt.MBToKSPriv, buffer, n, NULL);
            else
                *sym_rtrn = NoSymbol;
        }
    }

    if (mods & ControlMask) {
        if (n == 1) {
            buffer[0] = XkbToControl(buffer[0]);
            if (nbytes > 1)
                buffer[1] = '\0';
            return 1;
        }
        if (nbytes > 0)
            buffer[0] = '\0';
        return 0;
    }
    return n;
}

int
XLookupString(register XKeyEvent *event,
              char *buffer,
              int nbytes,
              KeySym *keysym,
              XComposeStatus *status)
{
    KeySym dummy;
    int rtrnLen;
    unsigned int new_mods;
    Display *dpy = event->display;

    if (keysym == NULL)
        keysym = &dummy;
    if (!XkbLookupKeySym(dpy, event->keycode, event->state, &new_mods, keysym))
        return 0;
    new_mods = (event->state & (~new_mods));

    /* find the group where a symbol can be converted to control one */
    if (new_mods & ControlMask && *keysym > 0x7F &&
        (dpy->xkb_info->xlib_ctrls & XkbLC_ControlFallback)) {
        XKeyEvent tmp_ev = *event;
        KeySym tmp_keysym;
        unsigned int tmp_new_mods;

        if (_XkbUnavailable(dpy)) {
            tmp_ev.state = event->state ^ dpy->mode_switch;
            if (XkbLookupKeySym(dpy, tmp_ev.keycode, tmp_ev.state,
                                &tmp_new_mods, &tmp_keysym) &&
                tmp_keysym != NoSymbol && tmp_keysym < 0x80) {
                *keysym = tmp_keysym;
            }
        }
        else {
            int n = XkbKeyNumGroups(dpy->xkb_info->desc, tmp_ev.keycode);
            int i;

            for (i = 0; i < n; i++) {
                if (XkbGroupForCoreState(event->state) == i)
                    continue;
                tmp_ev.state = XkbBuildCoreState(tmp_ev.state, i);
                if (XkbLookupKeySym(dpy, tmp_ev.keycode, tmp_ev.state,
                                    &tmp_new_mods, &tmp_keysym) &&
                    tmp_keysym != NoSymbol && tmp_keysym < 0x80) {
                    *keysym = tmp_keysym;
                    new_mods = (event->state & (~tmp_new_mods));
                    break;
                }
            }
        }
    }

    /* We *should* use the new_mods (which does not contain any modifiers */
    /* that were used to compute the symbol here, but pre-XKB XLookupString */
    /* did not and we have to remain compatible.  Sigh. */
    if (_XkbUnavailable(dpy) ||
        (dpy->xkb_info->xlib_ctrls & XkbLC_ConsumeLookupMods) == 0)
        new_mods = event->state;

    rtrnLen = XkbLookupKeyBinding(dpy, *keysym, new_mods, buffer, nbytes, NULL);
    if (rtrnLen > 0)
        return rtrnLen;

    return XkbTranslateKeySym(dpy, keysym, new_mods, buffer, nbytes, NULL);
}


int
XkbLookupKeyBinding(Display *dpy,
                    register KeySym sym,
                    unsigned int mods,
                    char *buffer,
                    int nbytes,
                    int *extra_rtrn)
{
    register struct _XKeytrans *p;

    if (extra_rtrn)
        *extra_rtrn = 0;
    for (p = dpy->key_bindings; p; p = p->next) {
        if (((mods & AllMods) == p->state) && (sym == p->key)) {
            int tmp = p->len;

            if (tmp > nbytes) {
                if (extra_rtrn)
                    *extra_rtrn = (tmp - nbytes);
                tmp = nbytes;
            }
            memcpy(buffer, p->string, (size_t) tmp);
            if (tmp < nbytes)
                buffer[tmp] = '\0';
            return tmp;
        }
    }
    return 0;
}

char
XkbToControl(char ch)
{
    register char c = ch;

    if ((c >= '@' && c < '\177') || c == ' ')
        c &= 0x1F;
    else if (c == '2')
        c = '\000';
    else if (c >= '3' && c <= '7')
        c -= ('3' - '\033');
    else if (c == '8')
        c = '\177';
    else if (c == '/')
        c = '_' & 0x1F;
    return c;
}
