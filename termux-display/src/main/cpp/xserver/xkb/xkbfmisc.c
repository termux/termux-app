/************************************************************
 Copyright (c) 1995 by Silicon Graphics Computer Systems, Inc.

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
#include <ctype.h>
#include <stdlib.h>

#include <X11/Xos.h>
#include <X11/Xfuncs.h>
#include <X11/extensions/XKMformat.h>

#include <X11/X.h>
#include <X11/keysym.h>
#include <X11/Xproto.h>
#include "misc.h"
#include "inputstr.h"
#include "dix.h"
#include "xkbstr.h"
#define XKBSRV_NEED_FILE_FUNCS	1
#include <xkbsrv.h>
#include "xkbgeom.h"
#include "xkb.h"

unsigned
_XkbKSCheckCase(KeySym ks)
{
    unsigned set, rtrn;

    set = (ks & (~0xff)) >> 8;
    rtrn = 0;
    switch (set) {
    case 0:                    /* latin 1 */
        if (((ks >= XK_A) && (ks <= XK_Z)) ||
            ((ks >= XK_Agrave) && (ks <= XK_THORN) && (ks != XK_multiply))) {
            rtrn |= _XkbKSUpper;
        }
        if (((ks >= XK_a) && (ks <= XK_z)) ||
            ((ks >= XK_ssharp) && (ks <= XK_ydiaeresis) &&
             (ks != XK_division))) {
            rtrn |= _XkbKSLower;
        }
        break;
    case 1:                    /* latin 2 */
        if (((ks >= XK_Aogonek) && (ks <= XK_Zabovedot) && (ks != XK_breve)) ||
            ((ks >= XK_Racute) && (ks <= XK_Tcedilla))) {
            rtrn |= _XkbKSUpper;
        }
        if (((ks >= XK_aogonek) && (ks <= XK_zabovedot) && (ks != XK_ogonek) &&
             (ks != XK_caron) && (ks != XK_doubleacute)) || ((ks >= XK_racute)
                                                             && (ks <=
                                                                 XK_tcedilla)))
        {
            rtrn |= _XkbKSLower;
        }
        break;
    case 2:                    /* latin 3 */
        if (((ks >= XK_Hstroke) && (ks <= XK_Jcircumflex)) ||
            ((ks >= XK_Cabovedot) && (ks <= XK_Scircumflex))) {
            rtrn |= _XkbKSUpper;
        }
        if (((ks >= XK_hstroke) && (ks <= XK_jcircumflex)) ||
            ((ks >= XK_cabovedot) && (ks <= XK_scircumflex))) {
            rtrn |= _XkbKSLower;
        }
        break;
    case 3:                    /* latin 4 */
        if (((ks >= XK_Rcedilla) && (ks <= XK_Tslash)) ||
            (ks == XK_ENG) || ((ks >= XK_Amacron) && (ks <= XK_Umacron))) {
            rtrn |= _XkbKSUpper;
        }
        if ((ks == XK_kra) ||
            ((ks >= XK_rcedilla) && (ks <= XK_tslash)) ||
            (ks == XK_eng) || ((ks >= XK_amacron) && (ks <= XK_umacron))) {
            rtrn |= _XkbKSLower;
        }
        break;
    case 18:                   /* latin 8 */
        if ((ks == XK_Wcircumflex) ||
            (ks == XK_Ycircumflex) ||
            (ks == XK_Babovedot) ||
            (ks == XK_Dabovedot) ||
            (ks == XK_Fabovedot) ||
            (ks == XK_Mabovedot) ||
            (ks == XK_Pabovedot) ||
            (ks == XK_Sabovedot) ||
            (ks == XK_Tabovedot) ||
            (ks == XK_Wgrave) ||
            (ks == XK_Wacute) || (ks == XK_Wdiaeresis) || (ks == XK_Ygrave)) {
            rtrn |= _XkbKSUpper;
        }
        if ((ks == XK_wcircumflex) ||
            (ks == XK_ycircumflex) ||
            (ks == XK_babovedot) ||
            (ks == XK_dabovedot) ||
            (ks == XK_fabovedot) ||
            (ks == XK_mabovedot) ||
            (ks == XK_pabovedot) ||
            (ks == XK_sabovedot) ||
            (ks == XK_tabovedot) ||
            (ks == XK_wgrave) ||
            (ks == XK_wacute) || (ks == XK_wdiaeresis) || (ks == XK_ygrave)) {
            rtrn |= _XkbKSLower;
        }
        break;
    case 19:                   /* latin 9 */
        if ((ks == XK_OE) || (ks == XK_Ydiaeresis)) {
            rtrn |= _XkbKSUpper;
        }
        if (ks == XK_oe) {
            rtrn |= _XkbKSLower;
        }
        break;
    }
    return rtrn;
}

/***===================================================================***/

static Bool
XkbWriteSectionFromName(FILE * file, const char *sectionName, const char *name)
{
    fprintf(file, "    xkb_%-20s { include \"%s\" };\n", sectionName, name);
    return TRUE;
}

#define	NEED_DESC(n) ((!n)||((n)[0]=='+')||((n)[0]=='|')||(strchr((n),'%')))
#define	COMPLETE(n)  ((n)&&(!NEED_DESC(n)))

/* ARGSUSED */
static void
_AddIncl(FILE * file,
         XkbDescPtr xkb,
         Bool topLevel, Bool showImplicit, int index, void *priv)
{
    if ((priv) && (strcmp((char *) priv, "%") != 0))
        fprintf(file, "    include \"%s\"\n", (char *) priv);
    return;
}

Bool
XkbWriteXKBKeymapForNames(FILE * file,
                          XkbComponentNamesPtr names,
                          XkbDescPtr xkb, unsigned want, unsigned need)
{
    const char *tmp;
    unsigned complete;
    XkbNamesPtr old_names;
    int multi_section;
    unsigned wantNames, wantConfig, wantDflts;

    complete = 0;
    if (COMPLETE(names->keycodes))
        complete |= XkmKeyNamesMask;
    if (COMPLETE(names->types))
        complete |= XkmTypesMask;
    if (COMPLETE(names->compat))
        complete |= XkmCompatMapMask;
    if (COMPLETE(names->symbols))
        complete |= XkmSymbolsMask;
    if (COMPLETE(names->geometry))
        complete |= XkmGeometryMask;
    want |= (complete | need);
    if (want & XkmSymbolsMask)
        want |= XkmKeyNamesMask | XkmTypesMask;

    if (want == 0)
        return FALSE;

    if (xkb) {
        old_names = xkb->names;

        xkb->defined = 0;
        /* Wow would it ever be neat if we didn't need this noise. */
        if (xkb->names && xkb->names->keys)
            xkb->defined |= XkmKeyNamesMask;
        if (xkb->map && xkb->map->types)
            xkb->defined |= XkmTypesMask;
        if (xkb->compat)
            xkb->defined |= XkmCompatMapMask;
        if (xkb->map && xkb->map->num_syms)
            xkb->defined |= XkmSymbolsMask;
        if (xkb->indicators)
            xkb->defined |= XkmIndicatorsMask;
        if (xkb->geom)
            xkb->defined |= XkmGeometryMask;
    }
    else {
        old_names = NULL;
    }

    wantConfig = want & (~complete);
    if (xkb != NULL) {
        if (wantConfig & XkmTypesMask) {
            if ((!xkb->map) || (xkb->map->num_types < XkbNumRequiredTypes))
                wantConfig &= ~XkmTypesMask;
        }
        if (wantConfig & XkmCompatMapMask) {
            if ((!xkb->compat) || (xkb->compat->num_si < 1))
                wantConfig &= ~XkmCompatMapMask;
        }
        if (wantConfig & XkmSymbolsMask) {
            if ((!xkb->map) || (!xkb->map->key_sym_map))
                wantConfig &= ~XkmSymbolsMask;
        }
        if (wantConfig & XkmIndicatorsMask) {
            if (!xkb->indicators)
                wantConfig &= ~XkmIndicatorsMask;
        }
        if (wantConfig & XkmKeyNamesMask) {
            if ((!xkb->names) || (!xkb->names->keys))
                wantConfig &= ~XkmKeyNamesMask;
        }
        if ((wantConfig & XkmGeometryMask) && (!xkb->geom))
            wantConfig &= ~XkmGeometryMask;
    }
    else {
        wantConfig = 0;
    }
    complete |= wantConfig;

    wantDflts = 0;
    wantNames = want & (~complete);
    if ((xkb != NULL) && (old_names != NULL)) {
        if (wantNames & XkmTypesMask) {
            if (old_names->types != None) {
                tmp = NameForAtom(old_names->types);
                names->types = Xstrdup(tmp);
            }
            else {
                wantDflts |= XkmTypesMask;
            }
            complete |= XkmTypesMask;
        }
        if (wantNames & XkmCompatMapMask) {
            if (old_names->compat != None) {
                tmp = NameForAtom(old_names->compat);
                names->compat = Xstrdup(tmp);
            }
            else
                wantDflts |= XkmCompatMapMask;
            complete |= XkmCompatMapMask;
        }
        if (wantNames & XkmSymbolsMask) {
            if (old_names->symbols == None)
                return FALSE;
            tmp = NameForAtom(old_names->symbols);
            names->symbols = Xstrdup(tmp);
            complete |= XkmSymbolsMask;
        }
        if (wantNames & XkmKeyNamesMask) {
            if (old_names->keycodes != None) {
                tmp = NameForAtom(old_names->keycodes);
                names->keycodes = Xstrdup(tmp);
            }
            else
                wantDflts |= XkmKeyNamesMask;
            complete |= XkmKeyNamesMask;
        }
        if (wantNames & XkmGeometryMask) {
            if (old_names->geometry == None)
                return FALSE;
            tmp = NameForAtom(old_names->geometry);
            names->geometry = Xstrdup(tmp);
            complete |= XkmGeometryMask;
            wantNames &= ~XkmGeometryMask;
        }
    }
    if (complete & XkmCompatMapMask)
        complete |= XkmIndicatorsMask | XkmVirtualModsMask;
    else if (complete & (XkmSymbolsMask | XkmTypesMask))
        complete |= XkmVirtualModsMask;
    if (need & (~complete))
        return FALSE;
    if ((complete & XkmSymbolsMask) &&
        ((XkmKeyNamesMask | XkmTypesMask) & (~complete)))
        return FALSE;

    multi_section = 1;
    if (((complete & XkmKeymapRequired) == XkmKeymapRequired) &&
        ((complete & (~XkmKeymapLegal)) == 0)) {
        fprintf(file, "xkb_keymap \"default\" {\n");
    }
    else if (((complete & XkmSemanticsRequired) == XkmSemanticsRequired) &&
             ((complete & (~XkmSemanticsLegal)) == 0)) {
        fprintf(file, "xkb_semantics \"default\" {\n");
    }
    else if (((complete & XkmLayoutRequired) == XkmLayoutRequired) &&
             ((complete & (~XkmLayoutLegal)) == 0)) {
        fprintf(file, "xkb_layout \"default\" {\n");
    }
    else if (XkmSingleSection(complete & (~XkmVirtualModsMask))) {
        multi_section = 0;
    }
    else {
        return FALSE;
    }

    wantNames = complete & (~(wantConfig | wantDflts));
    if (wantConfig & XkmKeyNamesMask)
        XkbWriteXKBKeycodes(file, xkb, FALSE, FALSE, _AddIncl, names->keycodes);
    else if (wantDflts & XkmKeyNamesMask)
        fprintf(stderr, "Default symbols not implemented yet!\n");
    else if (wantNames & XkmKeyNamesMask)
        XkbWriteSectionFromName(file, "keycodes", names->keycodes);

    if (wantConfig & XkmTypesMask)
        XkbWriteXKBKeyTypes(file, xkb, FALSE, FALSE, _AddIncl, names->types);
    else if (wantDflts & XkmTypesMask)
        fprintf(stderr, "Default types not implemented yet!\n");
    else if (wantNames & XkmTypesMask)
        XkbWriteSectionFromName(file, "types", names->types);

    if (wantConfig & XkmCompatMapMask)
        XkbWriteXKBCompatMap(file, xkb, FALSE, FALSE, _AddIncl, names->compat);
    else if (wantDflts & XkmCompatMapMask)
        fprintf(stderr, "Default interps not implemented yet!\n");
    else if (wantNames & XkmCompatMapMask)
        XkbWriteSectionFromName(file, "compatibility", names->compat);

    if (wantConfig & XkmSymbolsMask)
        XkbWriteXKBSymbols(file, xkb, FALSE, FALSE, _AddIncl, names->symbols);
    else if (wantNames & XkmSymbolsMask)
        XkbWriteSectionFromName(file, "symbols", names->symbols);

    if (wantConfig & XkmGeometryMask)
        XkbWriteXKBGeometry(file, xkb, FALSE, FALSE, _AddIncl, names->geometry);
    else if (wantNames & XkmGeometryMask)
        XkbWriteSectionFromName(file, "geometry", names->geometry);

    if (multi_section)
        fprintf(file, "};\n");
    return TRUE;
}

/***====================================================================***/

int
XkbFindKeycodeByName(XkbDescPtr xkb, char *name, Bool use_aliases)
{
    register int i;

    if ((!xkb) || (!xkb->names) || (!xkb->names->keys))
        return 0;
    for (i = xkb->min_key_code; i <= xkb->max_key_code; i++) {
        if (strncmp(xkb->names->keys[i].name, name, XkbKeyNameLength) == 0)
            return i;
    }
    if (!use_aliases)
        return 0;
    if (xkb->geom && xkb->geom->key_aliases) {
        XkbKeyAliasPtr a;

        a = xkb->geom->key_aliases;
        for (i = 0; i < xkb->geom->num_key_aliases; i++, a++) {
            if (strncmp(name, a->alias, XkbKeyNameLength) == 0)
                return XkbFindKeycodeByName(xkb, a->real, FALSE);
        }
    }
    if (xkb->names && xkb->names->key_aliases) {
        XkbKeyAliasPtr a;

        a = xkb->names->key_aliases;
        for (i = 0; i < xkb->names->num_key_aliases; i++, a++) {
            if (strncmp(name, a->alias, XkbKeyNameLength) == 0)
                return XkbFindKeycodeByName(xkb, a->real, FALSE);
        }
    }
    return 0;
}

unsigned
XkbConvertGetByNameComponents(Bool toXkm, unsigned orig)
{
    unsigned rtrn;

    rtrn = 0;
    if (toXkm) {
        if (orig & XkbGBN_TypesMask)
            rtrn |= XkmTypesMask;
        if (orig & XkbGBN_CompatMapMask)
            rtrn |= XkmCompatMapMask;
        if (orig & XkbGBN_SymbolsMask)
            rtrn |= XkmSymbolsMask;
        if (orig & XkbGBN_IndicatorMapMask)
            rtrn |= XkmIndicatorsMask;
        if (orig & XkbGBN_KeyNamesMask)
            rtrn |= XkmKeyNamesMask;
        if (orig & XkbGBN_GeometryMask)
            rtrn |= XkmGeometryMask;
    }
    else {
        if (orig & XkmTypesMask)
            rtrn |= XkbGBN_TypesMask;
        if (orig & XkmCompatMapMask)
            rtrn |= XkbGBN_CompatMapMask;
        if (orig & XkmSymbolsMask)
            rtrn |= XkbGBN_SymbolsMask;
        if (orig & XkmIndicatorsMask)
            rtrn |= XkbGBN_IndicatorMapMask;
        if (orig & XkmKeyNamesMask)
            rtrn |= XkbGBN_KeyNamesMask;
        if (orig & XkmGeometryMask)
            rtrn |= XkbGBN_GeometryMask;
        if (orig != 0)
            rtrn |= XkbGBN_OtherNamesMask;
    }
    return rtrn;
}

/***====================================================================***/

#define	UNMATCHABLE(c)	(((c)=='(')||((c)==')')||((c)=='/'))

Bool
XkbNameMatchesPattern(char *name, char *ptrn)
{
    while (ptrn[0] != '\0') {
        if (name[0] == '\0') {
            if (ptrn[0] == '*') {
                ptrn++;
                continue;
            }
            return FALSE;
        }
        if (ptrn[0] == '?') {
            if (UNMATCHABLE(name[0]))
                return FALSE;
        }
        else if (ptrn[0] == '*') {
            if ((!UNMATCHABLE(name[0])) &&
                XkbNameMatchesPattern(name + 1, ptrn))
                return TRUE;
            return XkbNameMatchesPattern(name, ptrn + 1);
        }
        else if (ptrn[0] != name[0])
            return FALSE;
        name++;
        ptrn++;
    }
    /* if we get here, the pattern is exhausted (-:just like me:-) */
    return name[0] == '\0';
}
