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
#elif defined(HAVE_CONFIG_H)
#include <config.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include <X11/Xos.h>
#include <X11/Xfuncs.h>


#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>
#include "XKMformat.h"
#include "XKBfileInt.h"


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
            ((ks >= XK_agrave) && (ks <= XK_ydiaeresis))) {
            rtrn |= _XkbKSLower;
        }
        break;
    case 1:                    /* latin 2 */
        if (((ks >= XK_Aogonek) && (ks <= XK_Zabovedot) && (ks != XK_breve)) ||
            ((ks >= XK_Racute) && (ks <= XK_Tcedilla))) {
            rtrn |= _XkbKSUpper;
        }
        if (((ks >= XK_aogonek) && (ks <= XK_zabovedot) && (ks != XK_caron)) ||
            ((ks >= XK_racute) && (ks <= XK_tcedilla))) {
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
        if (((ks >= XK_rcedilla) && (ks <= XK_tslash)) ||
            (ks == XK_eng) || ((ks >= XK_amacron) && (ks <= XK_umacron))) {
            rtrn |= _XkbKSLower;
        }
        break;
    case 18:                   /* latin 8 */
        if ((ks == XK_Babovedot) ||
            ((ks >= XK_Dabovedot) && (ks <= XK_Wacute)) ||
            ((ks >= XK_Ygrave) && (ks <= XK_Fabovedot)) ||
            (ks == XK_Mabovedot) ||
            (ks == XK_Pabovedot) ||
            (ks == XK_Sabovedot) ||
            (ks == XK_Wdiaeresis) ||
            ((ks >= XK_Wcircumflex) && (ks <= XK_Ycircumflex))) {
            rtrn |= _XkbKSUpper;
        }
        if ((ks == XK_babovedot) ||
            (ks == XK_dabovedot) ||
            (ks == XK_fabovedot) ||
            (ks == XK_mabovedot) ||
            ((ks >= XK_wgrave) && (ks <= XK_wacute)) ||
            (ks == XK_ygrave) ||
            ((ks >= XK_wdiaeresis) && (ks <= XK_ycircumflex))) {
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

Bool
XkbLookupGroupAndLevel(XkbDescPtr       xkb,
                       int              key,
                       int *            mods_inout,
                       int *            grp_inout,
                       int *            lvl_rtrn)
{
    int nG, eG;

    if ((!xkb) || (!XkbKeycodeInRange(xkb, key)) || (!grp_inout))
        return False;

    nG = XkbKeyNumGroups(xkb, key);
    eG = *grp_inout;

    if (nG == 0) {
        *grp_inout = 0;
        if (lvl_rtrn != NULL)
            *lvl_rtrn = 0;
        return False;
    }
    else if (nG == 1) {
        eG = 0;
    }
    else if (eG >= nG) {
        unsigned gI = XkbKeyGroupInfo(xkb, key);

        switch (XkbOutOfRangeGroupAction(gI)) {
        default:
            eG %= nG;
            break;
        case XkbClampIntoRange:
            eG = nG - 1;
            break;
        case XkbRedirectIntoRange:
            eG = XkbOutOfRangeGroupNumber(gI);
            if (eG >= nG)
                eG = 0;
            break;
        }
    }
    *grp_inout = eG;
    if (mods_inout != NULL) {
        XkbKeyTypePtr type;
        int preserve;

        type = XkbKeyKeyType(xkb, key, eG);
        if (lvl_rtrn != NULL)
            *lvl_rtrn = 0;
        preserve = 0;
        if (type->map) {        /* find the shift level */
            register int i;
            register XkbKTMapEntryPtr entry;

            for (i = 0, entry = type->map; i < type->map_count; i++, entry++) {
                if ((entry->active) &&
                    (((*mods_inout) & type->mods.mask) == entry->mods.mask)) {
                    if (lvl_rtrn != NULL)
                        *lvl_rtrn = entry->level;
                    if (type->preserve)
                        preserve = type->preserve[i].mask;
                    break;
                }
            }
        }
        (*mods_inout) &= ~(type->mods.mask & (~preserve));
    }
    return True;
}

/***===================================================================***/

static Bool
XkbWriteSectionFromName(FILE *file, const char *sectionName, const char *name)
{
    fprintf(file, "    xkb_%-20s { include \"%s\" };\n", sectionName, name);
    return True;
}

#define	NEED_DESC(n) ((!n)||((n)[0]=='+')||((n)[0]=='|')||(strchr((n),'%')))
#define	COMPLETE(n)  ((n)&&(!NEED_DESC(n)))

/* ARGSUSED */
static void
_AddIncl(FILE *         file,
         XkbFileInfo *  result,
         Bool           topLevel,
         Bool           showImplicit,
         int            index,
         void *         priv)
{
    if ((priv) && (strcmp((char *) priv, "%") != 0))
        fprintf(file, "    include \"%s\"\n", (char *) priv);
    return;
}

Bool
XkbWriteXKBKeymapForNames(FILE *                file,
                          XkbComponentNamesPtr  names,
                          Display *             dpy,
                          XkbDescPtr            xkb,
                          unsigned              want,
                          unsigned              need)
{
    char *name, *tmp;
    unsigned complete;
    XkbNamesPtr old_names;
    int multi_section;
    unsigned wantNames, wantConfig, wantDflts;
    XkbFileInfo finfo;

    bzero(&finfo, sizeof(XkbFileInfo));

    complete = 0;
    if ((name = names->keymap) == NULL)
        name = "default";
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
        return False;

    if (xkb != NULL) {
        old_names = xkb->names;
        finfo.type = 0;
        finfo.defined = 0;
        finfo.xkb = xkb;
        if (!XkbDetermineFileType(&finfo, XkbXKBFile, NULL))
            return False;
    }
    else
        old_names = NULL;

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
                tmp = XkbAtomGetString(dpy, old_names->types);
                names->types = tmp;
            }
            else {
                wantDflts |= XkmTypesMask;
            }
            complete |= XkmTypesMask;
        }
        if (wantNames & XkmCompatMapMask) {
            if (old_names->compat != None) {
                tmp = XkbAtomGetString(dpy, old_names->compat);
                names->compat = tmp;
            }
            else
                wantDflts |= XkmCompatMapMask;
            complete |= XkmCompatMapMask;
        }
        if (wantNames & XkmSymbolsMask) {
            if (old_names->symbols == None)
                return False;
            tmp = XkbAtomGetString(dpy, old_names->symbols);
            names->symbols = tmp;
            complete |= XkmSymbolsMask;
        }
        if (wantNames & XkmKeyNamesMask) {
            if (old_names->keycodes != None) {
                tmp = XkbAtomGetString(dpy, old_names->keycodes);
                names->keycodes = tmp;
            }
            else
                wantDflts |= XkmKeyNamesMask;
            complete |= XkmKeyNamesMask;
        }
        if (wantNames & XkmGeometryMask) {
            if (old_names->geometry == None)
                return False;
            tmp = XkbAtomGetString(dpy, old_names->geometry);
            names->geometry = tmp;
            complete |= XkmGeometryMask;
            wantNames &= ~XkmGeometryMask;
        }
    }
    if (complete & XkmCompatMapMask)
        complete |= XkmIndicatorsMask | XkmVirtualModsMask;
    else if (complete & (XkmSymbolsMask | XkmTypesMask))
        complete |= XkmVirtualModsMask;
    if (need & (~complete))
        return False;
    if ((complete & XkmSymbolsMask) &&
        ((XkmKeyNamesMask | XkmTypesMask) & (~complete)))
        return False;

    multi_section = 1;
    if (((complete & XkmKeymapRequired) == XkmKeymapRequired) &&
        ((complete & (~XkmKeymapLegal)) == 0)) {
        fprintf(file, "xkb_keymap \"%s\" {\n", name);
    }
    else if (((complete & XkmSemanticsRequired) == XkmSemanticsRequired) &&
             ((complete & (~XkmSemanticsLegal)) == 0)) {
        fprintf(file, "xkb_semantics \"%s\" {\n", name);
    }
    else if (((complete & XkmLayoutRequired) == XkmLayoutRequired) &&
             ((complete & (~XkmLayoutLegal)) == 0)) {
        fprintf(file, "xkb_layout \"%s\" {\n", name);
    }
    else if (XkmSingleSection(complete & (~XkmVirtualModsMask))) {
        multi_section = 0;
    }
    else {
        return False;
    }

    wantNames = complete & (~(wantConfig | wantDflts));
    name = names->keycodes;
    if (wantConfig & XkmKeyNamesMask)
        XkbWriteXKBKeycodes(file, &finfo, False, False, _AddIncl, name);
    else if (wantDflts & XkmKeyNamesMask)
        fprintf(stderr, "Default symbols not implemented yet!\n");
    else if (wantNames & XkmKeyNamesMask)
        XkbWriteSectionFromName(file, "keycodes", name);

    name = names->types;
    if (wantConfig & XkmTypesMask)
        XkbWriteXKBKeyTypes(file, &finfo, False, False, _AddIncl, name);
    else if (wantDflts & XkmTypesMask)
        fprintf(stderr, "Default types not implemented yet!\n");
    else if (wantNames & XkmTypesMask)
        XkbWriteSectionFromName(file, "types", name);

    name = names->compat;
    if (wantConfig & XkmCompatMapMask)
        XkbWriteXKBCompatMap(file, &finfo, False, False, _AddIncl, name);
    else if (wantDflts & XkmCompatMapMask)
        fprintf(stderr, "Default interps not implemented yet!\n");
    else if (wantNames & XkmCompatMapMask)
        XkbWriteSectionFromName(file, "compatibility", name);

    name = names->symbols;
    if (wantConfig & XkmSymbolsMask)
        XkbWriteXKBSymbols(file, &finfo, False, False, _AddIncl, name);
    else if (wantNames & XkmSymbolsMask)
        XkbWriteSectionFromName(file, "symbols", name);

    name = names->geometry;
    if (wantConfig & XkmGeometryMask)
        XkbWriteXKBGeometry(file, &finfo, False, False, _AddIncl, name);
    else if (wantNames & XkmGeometryMask)
        XkbWriteSectionFromName(file, "geometry", name);

    if (multi_section)
        fprintf(file, "};\n");
    return True;
}

/***====================================================================***/

/*ARGSUSED*/
Status
XkbMergeFile(XkbDescPtr xkb, XkbFileInfo finfo)
{
    return BadImplementation;
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
                return XkbFindKeycodeByName(xkb, a->real, False);
        }
    }
    if (xkb->names && xkb->names->key_aliases) {
        XkbKeyAliasPtr a;

        a = xkb->names->key_aliases;
        for (i = 0; i < xkb->names->num_key_aliases; i++, a++) {
            if (strncmp(name, a->alias, XkbKeyNameLength) == 0)
                return XkbFindKeycodeByName(xkb, a->real, False);
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

unsigned
XkbConvertXkbComponents(Bool toXkm, unsigned orig)
{
    unsigned rtrn;

    rtrn = 0;
    if (toXkm) {
        if (orig & XkbClientMapMask)
            rtrn |= XkmTypesMask | XkmSymbolsMask;
        if (orig & XkbServerMapMask)
            rtrn |= XkmTypesMask | XkmSymbolsMask;
        if (orig & XkbCompatMapMask)
            rtrn |= XkmCompatMapMask;
        if (orig & XkbIndicatorMapMask)
            rtrn |= XkmIndicatorsMask;
        if (orig & XkbNamesMask)
            rtrn |= XkmKeyNamesMask;
        if (orig & XkbGeometryMask)
            rtrn |= XkmGeometryMask;
    }
    else {
        if (orig != 0)
            rtrn |= XkbNamesMask;
        if (orig & XkmTypesMask)
            rtrn |= XkbClientMapMask;
        if (orig & XkmCompatMapMask)
            rtrn |= XkbCompatMapMask | XkbIndicatorMapMask;
        if (orig & XkmSymbolsMask)
            rtrn |= XkbClientMapMask | XkbServerMapMask;
        if (orig & XkmIndicatorsMask)
            rtrn |= XkbIndicatorMapMask;
        if (orig & XkmKeyNamesMask)
            rtrn |= XkbNamesMask | XkbIndicatorMapMask;
        if (orig & XkmGeometryMask)
            rtrn |= XkbGeometryMask;
    }
    return rtrn;
}

Bool
XkbDetermineFileType(XkbFileInfoPtr finfo, int format, int *opts_missing)
{
    unsigned present;
    XkbDescPtr xkb;

    if ((!finfo) || (!finfo->xkb))
        return False;
    if (opts_missing)
        *opts_missing = 0;
    xkb = finfo->xkb;
    present = 0;
    if ((xkb->names) && (xkb->names->keys))
        present |= XkmKeyNamesMask;
    if ((xkb->map) && (xkb->map->types))
        present |= XkmTypesMask;
    if (xkb->compat)
        present |= XkmCompatMapMask;
    if ((xkb->map) && (xkb->map->num_syms > 1))
        present |= XkmSymbolsMask;
    if (xkb->indicators)
        present |= XkmIndicatorsMask;
    if (xkb->geom)
        present |= XkmGeometryMask;
    if (!present)
        return False;
    else
        switch (present) {
        case XkmKeyNamesMask:
            finfo->type = XkmKeyNamesIndex;
            finfo->defined = present;
            return True;
        case XkmTypesMask:
            finfo->type = XkmTypesIndex;
            finfo->defined = present;
            return True;
        case XkmCompatMapMask:
            finfo->type = XkmCompatMapIndex;
            finfo->defined = present;
            return True;
        case XkmSymbolsMask:
            if (format != XkbXKMFile) {
                finfo->type = XkmSymbolsIndex;
                finfo->defined = present;
                return True;
            }
            break;
        case XkmGeometryMask:
            finfo->type = XkmGeometryIndex;
            finfo->defined = present;
            return True;
        }
    if ((present & (~XkmSemanticsLegal)) == 0) {
        if ((XkmSemanticsRequired & present) == XkmSemanticsRequired) {
            if (opts_missing)
                *opts_missing = XkmSemanticsOptional & (~present);
            finfo->type = XkmSemanticsFile;
            finfo->defined = present;
            return True;
        }
    }
    else if ((present & (~XkmLayoutLegal)) == 0) {
        if ((XkmLayoutRequired & present) == XkmLayoutRequired) {
            if (opts_missing)
                *opts_missing = XkmLayoutOptional & (~present);
            finfo->type = XkmLayoutFile;
            finfo->defined = present;
            return True;
        }
    }
    else if ((present & (~XkmKeymapLegal)) == 0) {
        if ((XkmKeymapRequired & present) == XkmKeymapRequired) {
            if (opts_missing)
                *opts_missing = XkmKeymapOptional & (~present);
            finfo->type = XkmKeymapFile;
            finfo->defined = present;
            return True;
        }
    }
    return False;
}

/* all latin-1 alphanumerics, plus parens, slash, minus, underscore and */
/* wildcards */

static unsigned char componentSpecLegal[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xa7, 0xff, 0x83,
    0xfe, 0xff, 0xff, 0x87, 0xfe, 0xff, 0xff, 0x07,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0x7f, 0xff
};

void
XkbEnsureSafeMapName(char *name)
{
    if (name == NULL)
        return;
    while (*name != '\0') {
        if ((componentSpecLegal[(*name) / 8] & (1 << ((*name) % 8))) == 0)
            *name = '_';
        name++;
    }
    return;
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
            return False;
        }
        if (ptrn[0] == '?') {
            if (UNMATCHABLE(name[0]))
                return False;
        }
        else if (ptrn[0] == '*') {
            if ((!UNMATCHABLE(name[0])) &&
                XkbNameMatchesPattern(name + 1, ptrn))
                return True;
            return XkbNameMatchesPattern(name, ptrn + 1);
        }
        else if (ptrn[0] != name[0])
            return False;
        name++;
        ptrn++;
    }
    /* if we get here, the pattern is exhausted (-:just like me:-) */
    return (name[0] == '\0');
}

#ifndef HAVE_STRCASECMP
_X_HIDDEN int
_XkbStrCaseCmp(char *str1, char *str2)
{
    const u_char *us1 = (const u_char *) str1, *us2 = (const u_char *) str2;

    while (tolower(*us1) == tolower(*us2)) {
        if (*us1++ == '\0')
            return (0);
        us2++;
    }

    return (tolower(*us1) - tolower(*us2));
}
#endif
