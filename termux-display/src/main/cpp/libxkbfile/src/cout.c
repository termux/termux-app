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
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBgeom.h>

#include "XKMformat.h"
#include "XKBfileInt.h"

#define lowbit(x)       ((x) & (-(x)))

static Bool
WriteCHdrVMods(FILE *file, Display *dpy, XkbDescPtr xkb)
{
    register int i, nOut;

    if ((!xkb) || (!xkb->names))
        return False;
    for (i = nOut = 0; i < XkbNumVirtualMods; i++) {
        if (xkb->names->vmods[i] != None) {
            fprintf(file, "%s#define	vmod_%s	%d\n",
                    (nOut < 1 ? "\n" : ""),
                    XkbAtomText(dpy, xkb->names->vmods[i], XkbCFile), i);
            nOut++;
        }
    }
    for (i = nOut = 0; i < XkbNumVirtualMods; i++) {
        if (xkb->names->vmods[i] != None) {
            fprintf(file, "%s#define	vmod_%sMask	(1<<%d)\n",
                    (nOut < 1 ? "\n" : ""),
                    XkbAtomText(dpy, xkb->names->vmods[i], XkbCFile), i);
            nOut++;
        }
    }
    if (nOut > 0)
        fprintf(file, "\n");
    return True;
}

static Bool
WriteCHdrKeycodes(FILE * file, XkbDescPtr xkb)
{
    Atom                kcName;
    register unsigned   i;
    char                buf[8];

    if ((!xkb) || (!xkb->names) || (!xkb->names->keys)) {
        _XkbLibError(_XkbErrMissingNames, "WriteCHdrKeycodes", 0);
        return False;
    }
    kcName = xkb->names->keycodes;
    buf[4] = '\0';
    if (xkb->names->keycodes != None)
        fprintf(file, "/* keycodes name is \"%s\" */\n",
                XkbAtomText(xkb->dpy, kcName, XkbMessage));
    fprintf(file, "static XkbKeyNameRec	keyNames[NUM_KEYS]= {\n");
    for (i = 0; i <= xkb->max_key_code; i++) {
        snprintf(buf, sizeof(buf), "\"%s\"",
                 XkbKeyNameText(xkb->names->keys[i].name, XkbCFile));
        if (i != xkb->max_key_code) {
            fprintf(file, "    {  %6s  },", buf);
            if ((i & 3) == 3)
                fprintf(file, "\n");
        }
        else {
            fprintf(file, "    {  %6s  }\n", buf);
        }
    }
    fprintf(file, "};\n");
    return True;
}

static void
WriteTypePreserve(FILE *        file,
                  Display *     dpy,
                  char *        prefix,
                  XkbDescPtr    xkb,
                  XkbKeyTypePtr type)
{
    register unsigned i;
    XkbModsPtr pre;

    fprintf(file, "static XkbModsRec preserve_%s[%d]= {\n", prefix,
            type->map_count);
    for (i = 0, pre = type->preserve; i < type->map_count; i++, pre++) {
        if (i != 0)
            fprintf(file, ",\n");
        fprintf(file, "    {   %15s, ", XkbModMaskText(pre->mask, XkbCFile));
        fprintf(file, "%15s, ", XkbModMaskText(pre->real_mods, XkbCFile));
        fprintf(file, "%15s }",
                XkbVModMaskText(dpy, xkb, 0, pre->vmods, XkbCFile));
    }
    fprintf(file, "\n};\n");
    return;
}

static void
WriteTypeInitFunc(FILE *file, Display *dpy, XkbDescPtr xkb)
{
    register unsigned   i, n;
    XkbKeyTypePtr       type;
    Atom *              names;
    char *              prefix = NULL;

    fprintf(file, "\n\nstatic void\n");
    fprintf(file, "initTypeNames(DPYTYPE dpy)\n");
    fprintf(file, "{\n");
    for (i = 0, type = xkb->map->types; i < xkb->map->num_types; i++, type++) {
        if (!(prefix = strdup(XkbAtomText(dpy, type->name, XkbCFile)))) {
            _XkbLibError(_XkbErrBadAlloc, "WriteTypeInitFunc", 0);
            fprintf(file, "#error XkbErrBadAlloc WriteTypeInitFunc\n");
            break;
        }
        if (type->name != None)
            fprintf(file, "    dflt_types[%d].name= GET_ATOM(dpy,\"%s\");\n",
                    i, XkbAtomText(dpy, type->name, XkbCFile));
        names = type->level_names;
        if (names != NULL) {
            char *tmp;

            for (n = 0; n < type->num_levels; n++) {
                if (names[n] == None)
                    continue;
                tmp = XkbAtomText(dpy, names[n], XkbCFile);
                if (tmp == NULL)
                    continue;
                fprintf(file, "    lnames_%s[%d]=	", prefix, n);
                fprintf(file, "GET_ATOM(dpy,\"%s\");\n", tmp);
            }
        }
        free(prefix);
        prefix = NULL;
    }
    fprintf(file, "}\n");
    return;
}

static Bool
WriteCHdrKeyTypes(FILE *file, Display *dpy, XkbDescPtr xkb)
{
    register unsigned   i, n;
    XkbClientMapPtr     map;
    XkbKeyTypePtr       type;
    char *              prefix = NULL;

    if ((!xkb) || (!xkb->map) || (!xkb->map->types)) {
        _XkbLibError(_XkbErrMissingTypes, "WriteCHdrKeyTypes", 0);
        return False;
    }
    if (xkb->map->num_types < XkbNumRequiredTypes) {
        _XkbLibError(_XkbErrMissingReqTypes, "WriteCHdrKeyTypes", 0);
        return 0;
    }
    map = xkb->map;
    if ((xkb->names != NULL) && (xkb->names->types != None)) {
        fprintf(file, "/* types name is \"%s\" */\n",
                XkbAtomText(dpy, xkb->names->types, XkbCFile));
    }
    for (i = 0, type = map->types; i < map->num_types; i++, type++) {
        if (!(prefix = strdup(XkbAtomText(dpy, type->name, XkbCFile)))) {
            _XkbLibError(_XkbErrBadAlloc, "WriteCHdrKeyTypes", 0);
            return False;
        }

        if (type->map_count > 0) {
            XkbKTMapEntryPtr entry;

            entry = type->map;
            fprintf(file, "static XkbKTMapEntryRec map_%s[%d]= {\n", prefix,
                    type->map_count);
            for (n = 0; n < (unsigned) type->map_count; n++, entry++) {
                if (n != 0)
                    fprintf(file, ",\n");
                fprintf(file, "    { %d, %6d, { %15s, %15s, %15s } }",
                        entry->active,
                        entry->level,
                        XkbModMaskText(entry->mods.mask, XkbCFile),
                        XkbModMaskText(entry->mods.real_mods, XkbCFile),
                        XkbVModMaskText(dpy, xkb, 0, entry->mods.vmods,
                                        XkbCFile));
            }
            fprintf(file, "\n};\n");

            if (type->preserve)
                WriteTypePreserve(file, dpy, prefix, xkb, type);
        }
        if (type->level_names != NULL) {
            fprintf(file, "static Atom lnames_%s[%d];\n", prefix,
                    type->num_levels);
        }
        fprintf(file, "\n");
        free(prefix);
        prefix = NULL;
    }
    fprintf(file, "static XkbKeyTypeRec dflt_types[]= {\n");
    for (i = 0, type = map->types; i < (unsigned) map->num_types; i++, type++) {
        if (!(prefix = strdup(XkbAtomText(dpy, type->name, XkbCFile)))) {
            _XkbLibError(_XkbErrBadAlloc, "WriteCHdrKeyTypes", 0);
            return False;
        }
        if (i != 0)
            fprintf(file, ",\n");
        fprintf(file, "    {\n	{ %15s, %15s, %15s },\n",
                XkbModMaskText(type->mods.mask, XkbCFile),
                XkbModMaskText(type->mods.real_mods, XkbCFile),
                XkbVModMaskText(dpy, xkb, 0, type->mods.vmods, XkbCFile));
        fprintf(file, "	%d,\n", type->num_levels);
        fprintf(file, "	%d,", type->map_count);
        if (type->map_count > 0)
            fprintf(file, "	map_%s,", prefix);
        else
            fprintf(file, "	NULL,");
        if (type->preserve)
            fprintf(file, "	preserve_%s,\n", prefix);
        else
            fprintf(file, "	NULL,\n");
        if (type->level_names != NULL)
            fprintf(file, "	None,	lnames_%s\n    }", prefix);
        else
            fprintf(file, "	None,	NULL\n    }");
        free(prefix);
        prefix = NULL;
    }
    fprintf(file, "\n};\n");
    fprintf(file,
            "#define num_dflt_types (sizeof(dflt_types)/sizeof(XkbKeyTypeRec))\n");
    WriteTypeInitFunc(file, dpy, xkb);
    return True;
}

static Bool
WriteCHdrCompatMap(FILE *file, Display *dpy, XkbDescPtr xkb)
{
    register unsigned   i;
    XkbCompatMapPtr     compat;
    XkbSymInterpretPtr  interp;

    if ((!xkb) || (!xkb->compat) || (!xkb->compat->sym_interpret)) {
        _XkbLibError(_XkbErrMissingSymInterps, "WriteCHdrInterp", 0);
        return False;
    }
    compat = xkb->compat;
    if ((xkb->names != NULL) && (xkb->names->compat != None)) {
        fprintf(file, "/* compat name is \"%s\" */\n",
                XkbAtomText(dpy, xkb->names->compat, XkbCFile));
    }
    fprintf(file, "static XkbSymInterpretRec dfltSI[%d]= {\n", compat->num_si);
    interp = compat->sym_interpret;
    for (i = 0; i < compat->num_si; i++, interp++) {
        XkbAction *act;

        act = (XkbAction *) &interp->act;
        if (i != 0)
            fprintf(file, ",\n");
        fprintf(file, "    {    %s, ", XkbKeysymText(interp->sym, XkbCFile));
        fprintf(file, "0x%04x,\n", interp->flags);
        fprintf(file, "         %s, ", XkbSIMatchText(interp->match, XkbCFile));
        fprintf(file, "%s,\n", XkbModMaskText(interp->mods, XkbCFile));
        fprintf(file, "         %d,\n", interp->virtual_mod);
        fprintf(file, "       %s }", XkbActionText(dpy, xkb, act, XkbCFile));
    }
    fprintf(file, "\n};\n");
    fprintf(file,
            "#define num_dfltSI (sizeof(dfltSI)/sizeof(XkbSymInterpretRec))\n");
    fprintf(file, "\nstatic XkbCompatMapRec compatMap= {\n");
    fprintf(file, "    dfltSI,\n");
    fprintf(file, "    {   /* group compatibility */\n        ");
    for (i = 0; i < XkbNumKbdGroups; i++) {
        XkbModsPtr gc;

        gc = &xkb->compat->groups[i];
        fprintf(file, "%s{ %12s, %12s, %12s }",
                ((i == 0) ? "" : ",\n        "),
                XkbModMaskText(gc->mask, XkbCFile),
                XkbModMaskText(gc->real_mods, XkbCFile),
                XkbVModMaskText(xkb->dpy, xkb, 0, gc->vmods, XkbCFile));
    }
    fprintf(file, "\n    },\n");
    fprintf(file, "    num_dfltSI, num_dfltSI\n");
    fprintf(file, "};\n\n");
    return True;
}

static Bool
WriteCHdrSymbols(FILE *file, XkbDescPtr xkb)
{
    register unsigned i;

    if ((!xkb) || (!xkb->map) || (!xkb->map->syms) || (!xkb->map->key_sym_map)) {
        _XkbLibError(_XkbErrMissingSymbols, "WriteCHdrSymbols", 0);
        return False;
    }
    fprintf(file, "#define NUM_SYMBOLS	%d\n", xkb->map->num_syms);
    if (xkb->map->num_syms > 0) {
        register KeySym *sym;

        sym = xkb->map->syms;
        fprintf(file, "static KeySym	symCache[NUM_SYMBOLS]= {\n");
        for (i = 0; i < xkb->map->num_syms; i++, sym++) {
            if (i == 0)
                fprintf(file, "    ");
            else if (i % 4 == 0)
                fprintf(file, ",\n    ");
            else
                fprintf(file, ", ");
            fprintf(file, "%15s", XkbKeysymText(*sym, XkbCFile));
        }
        fprintf(file, "\n};\n");
    }
    if (xkb->max_key_code > 0) {
        register XkbSymMapPtr map;

        map = xkb->map->key_sym_map;
        fprintf(file, "static XkbSymMapRec	symMap[NUM_KEYS]= {\n");
        for (i = 0; i <= xkb->max_key_code; i++, map++) {
            if (i == 0)
                fprintf(file, "    ");
            else if ((i & 3) == 0)
                fprintf(file, ",\n    ");
            else
                fprintf(file, ", ");
            fprintf(file, "{ %2d, 0x%x, %3d }",
                    map->kt_index[0], map->group_info, map->offset);
        }
        fprintf(file, "\n};\n");
    }
    return True;
}

static Bool
WriteCHdrClientMap(FILE *file, Display *dpy, XkbDescPtr xkb)
{
    if ((!xkb) || (!xkb->map) || (!xkb->map->syms) || (!xkb->map->key_sym_map)) {
        _XkbLibError(_XkbErrMissingSymbols, "WriteCHdrClientMap", 0);
        return False;
    }
    if (!WriteCHdrKeyTypes(file, dpy, xkb))
        return False;
    if (!WriteCHdrSymbols(file, xkb))
        return False;
    fprintf(file, "static XkbClientMapRec clientMap= {\n");
    fprintf(file, "    NUM_TYPES,   NUM_TYPES,   types, \n");
    fprintf(file, "    NUM_SYMBOLS, NUM_SYMBOLS, symCache, symMap\n");
    fprintf(file, "};\n\n");
    return True;
}

static Bool
WriteCHdrServerMap(FILE *file, Display *dpy, XkbDescPtr xkb)
{
    register unsigned i;

    if ((!xkb) || (!xkb->map) || (!xkb->map->syms) || (!xkb->map->key_sym_map)) {
        _XkbLibError(_XkbErrMissingSymbols, "WriteCHdrServerMap", 0);
        return False;
    }
    if (xkb->server->num_acts > 0) {
        register XkbAnyAction *act;

        act = (XkbAnyAction *) xkb->server->acts;
        fprintf(file, "#define NUM_ACTIONS	%d\n", xkb->server->num_acts);
        fprintf(file, "static XkbAnyAction 	actionCache[NUM_ACTIONS]= {\n");
        for (i = 0; i < xkb->server->num_acts; i++, act++) {
            if (i == 0)
                fprintf(file, "    ");
            else
                fprintf(file, ",\n    ");
            fprintf(file, "%s",
                    XkbActionText(dpy, xkb, (XkbAction *) act, XkbCFile));
        }
        fprintf(file, "\n};\n");
    }
    fprintf(file, "static unsigned short	keyActions[NUM_KEYS]= {\n");
    for (i = 0; i <= xkb->max_key_code; i++) {
        if (i == 0)
            fprintf(file, "    ");
        else if ((i & 0xf) == 0)
            fprintf(file, ",\n    ");
        else
            fprintf(file, ", ");
        fprintf(file, "%2d", xkb->server->key_acts[i]);
    }
    fprintf(file, "\n};\n");
    fprintf(file, "static XkbBehavior behaviors[NUM_KEYS]= {\n");
    for (i = 0; i <= xkb->max_key_code; i++) {
        if (i == 0)
            fprintf(file, "    ");
        else if ((i & 0x3) == 0)
            fprintf(file, ",\n    ");
        else
            fprintf(file, ", ");
        if (xkb->server->behaviors) {
            fprintf(file, "%s",
                    XkbBehaviorText(xkb, &xkb->server->behaviors[i], XkbCFile));
        }
        else
            fprintf(file, "{    0,    0 }");
    }
    fprintf(file, "\n};\n");
    fprintf(file, "static unsigned char explicit_parts[NUM_KEYS]= {\n");
    for (i = 0; i <= xkb->max_key_code; i++) {
        if (i == 0)
            fprintf(file, "    ");
        else if ((i & 0x7) == 0)
            fprintf(file, ",\n    ");
        else
            fprintf(file, ", ");
        if ((xkb->server->explicit == NULL) || (xkb->server->explicit[i] == 0))
            fprintf(file, "   0");
        else
            fprintf(file, "0x%02x", xkb->server->explicit[i]);
    }
    fprintf(file, "\n};\n");
    fprintf(file, "static unsigned short vmodmap[NUM_KEYS]= {\n");
    for (i = 0; i < xkb->max_key_code; i++) {
        if (i == 0)
            fprintf(file, "    ");
        else if ((i & 0x7) == 0)
            fprintf(file, ",\n    ");
        else
            fprintf(file, ", ");
        if ((xkb->server->vmodmap == NULL) || (xkb->server->vmodmap[i] == 0))
            fprintf(file, "     0");
        else
            fprintf(file, "0x%04x", xkb->server->vmodmap[i]);
    }
    fprintf(file, "};\n");
    fprintf(file, "static XkbServerMapRec serverMap= {\n");
    fprintf(file, "    %d, %d, (XkbAction *)actionCache,\n",
            xkb->server->num_acts, xkb->server->num_acts);
    fprintf(file, "    behaviors, keyActions, explicit_parts,\n");
    for (i = 0; i < XkbNumVirtualMods; i++) {
        if (i == 0)
            fprintf(file, "    { ");
        else if (i == 8)
            fprintf(file, ",\n      ");
        else
            fprintf(file, ", ");
        fprintf(file, "%3d", xkb->server->vmods[i]);
    }
    fprintf(file, " },\n");
    fprintf(file, "    vmodmap\n");
    fprintf(file, "};\n\n");
    return True;
}

static Bool
WriteCHdrIndicators(FILE *file, Display *dpy, XkbDescPtr xkb)
{
    register int i, nNames;
    XkbIndicatorMapPtr imap;

    if (xkb->indicators == NULL)
        return True;
    fprintf(file, "static XkbIndicatorRec indicators= {\n");
    fprintf(file, "    0x%lx,\n    {\n",
            (long) xkb->indicators->phys_indicators);
    for (imap = xkb->indicators->maps, i = nNames = 0; i < XkbNumIndicators;
         i++, imap++) {
        fprintf(file, "%s        { 0x%02x, %s, 0x%02x, %s, { %s, ",
                (i != 0 ? ",\n" : ""), imap->flags,
                XkbIMWhichStateMaskText(imap->which_groups, XkbCFile),
                imap->groups, XkbIMWhichStateMaskText(imap->which_mods,
                                                      XkbCFile),
                XkbModMaskText(imap->mods.mask, XkbCFile));
        fprintf(file, " %s, %s }, %s }",
                XkbModMaskText(imap->mods.real_mods, XkbCFile),
                XkbVModMaskText(dpy, xkb, 0, imap->mods.vmods, XkbCFile),
                XkbControlsMaskText(imap->ctrls, XkbCFile));
        if (xkb->names && (xkb->names->indicators[i] != None))
            nNames++;
    }
    fprintf(file, "\n    }\n};\n");
    if (nNames > 0) {
        fprintf(file, "static void\n");
        fprintf(file, "initIndicatorNames(DPYTYPE dpy,XkbDescPtr xkb)\n");
        fprintf(file, "{\n");
        for (i = 0; i < XkbNumIndicators; i++) {
            Atom name;

            if (xkb->names->indicators[i] == None)
                continue;
            name = xkb->names->indicators[i];
            fprintf(file, "    xkb->names->indicators[%2d]=	", i);
            fprintf(file, "GET_ATOM(dpy,\"%s\");\n",
                    XkbAtomText(dpy, name, XkbCFile));
        }
        fprintf(file, "}\n");
    }
    return True;
}

static Bool
WriteCHdrGeomProps(FILE *file, XkbDescPtr xkb, XkbGeometryPtr geom)
{
    if (geom->num_properties > 0) {
        register int i;

        fprintf(file, "\nstatic XkbPropertyRec g_props[%d]= {\n",
                geom->num_properties);
        for (i = 0; i < geom->num_properties; i++) {
            fprintf(file, "%s	{	\"%s\", \"%s\"	}",
                    (i == 0 ? "" : ",\n"),
                    XkbStringText(geom->properties[i].name, XkbCFile),
                    XkbStringText(geom->properties[i].value, XkbCFile));
        }
        fprintf(file, "\n};\n");
    }
    return True;
}

static Bool
WriteCHdrGeomColors(FILE *file, XkbDescPtr xkb, XkbGeometryPtr geom)
{
    if (geom->num_colors > 0) {
        register int i;

        fprintf(file, "\nstatic XkbColorRec g_colors[%d]= {\n",
                geom->num_colors);
        for (i = 0; i < geom->num_colors; i++) {
            fprintf(file, "%s	{	%3d, \"%s\"	}",
                    (i == 0 ? "" : ",\n"), geom->colors[i].pixel,
                    XkbStringText(geom->colors[i].spec, XkbCFile));
        }
        fprintf(file, "\n};\n");
    }
    return True;
}

static Bool
WriteCHdrGeomOutlines(FILE *file, int nOL, XkbOutlinePtr ol, int shapeNdx)
{
    register int o, p;

    for (o = 0; o < nOL; o++) {
        fprintf(file, "\nstatic XkbPointRec pts_sh%02do%02d[]= {\n", shapeNdx,
                o);
        for (p = 0; p < ol[o].num_points; p++) {
            if (p == 0)
                fprintf(file, "	");
            else if ((p & 0x3) == 0)
                fprintf(file, ",\n	");
            else
                fprintf(file, ", ");
            fprintf(file, "{ %4d, %4d }", ol[o].points[p].x, ol[o].points[p].y);
        }
        fprintf(file, "\n};");
    }
    fprintf(file, "\n\nstatic XkbOutlineRec ol_sh%02d[]= {\n", shapeNdx);
    for (o = 0; o < nOL; o++) {
        fprintf(file, "%s	{ %d,	%d,	%d,	pts_sh%02do%02d	}",
                (o == 0 ? "" : ",\n"),
                ol[o].num_points, ol[o].num_points,
                ol[o].corner_radius, shapeNdx, o);
    }
    fprintf(file, "\n};\n");
    return True;
}

static Bool
WriteCHdrGeomShapes(FILE *file, XkbDescPtr xkb, XkbGeometryPtr geom)
{
    register int s;
    register XkbShapePtr shape;

    for (s = 0, shape = geom->shapes; s < geom->num_shapes; s++, shape++) {
        WriteCHdrGeomOutlines(file, shape->num_outlines, shape->outlines, s);
    }
    fprintf(file, "\n\nstatic XkbShapeRec g_shapes[%d]= {\n", geom->num_shapes);
    for (s = 0, shape = geom->shapes; s < geom->num_shapes; s++, shape++) {
        fprintf(file, "%s	{ None, %3d, %3d, ol_sh%02d, ",
                (s == 0 ? "" : ",\n"), shape->num_outlines,
                shape->num_outlines, s);
        if (shape->approx) {
            fprintf(file, "&ol_sh%02d[%2d],	", s,
                    XkbOutlineIndex(shape, shape->approx));
        }
        else
            fprintf(file, "        NULL,	");
        if (shape->primary) {
            fprintf(file, "&ol_sh%02d[%2d],\n", s,
                    XkbOutlineIndex(shape, shape->primary));
        }
        else
            fprintf(file, "        NULL,\n");
        fprintf(file,
                "					{ %4d, %4d, %4d, %4d } }",
                shape->bounds.x1, shape->bounds.y1,
                shape->bounds.x2, shape->bounds.y2);
    }
    fprintf(file, "\n};\n");
    return True;
}

static Bool
WriteCHdrGeomDoodads(FILE *             file,
                     XkbDescPtr         xkb,
                     XkbGeometryPtr     geom,
                     XkbSectionPtr      section,
                     int                section_num)
{
    int                 nd, d;
    XkbDoodadPtr        doodad;
    Display *           dpy;

    dpy = xkb->dpy;
    if (section == NULL) {
        if (geom->num_doodads > 0) {
            fprintf(file, "static XkbDoodadRec g_doodads[%d];\n",
                    geom->num_doodads);
        }
        fprintf(file, "static void\n");
        fprintf(file, "_InitGeomDoodads(DPYTYPE dpy,XkbGeometryPtr geom)\n");
        fprintf(file, "{\n");
        fprintf(file, "register XkbDoodadPtr doodads;\n\n");
        fprintf(file, "    doodads= geom->doodads;\n");
        nd = geom->num_doodads;
        doodad = geom->doodads;
    }
    else {
        if (section->num_doodads > 0) {
            fprintf(file, "static XkbDoodadRec doodads_s%02d[%d];\n",
                    section_num, section->num_doodads);
        }
        fprintf(file, "static void\n");
        fprintf(file, "_InitS%02dDoodads(", section_num);
        fprintf(file, "    DPYTYPE		dpy,\n");
        fprintf(file, "    XkbGeometryPtr 	geom,\n");
        fprintf(file, "    XkbSectionPtr 	section)\n");
        fprintf(file, "{\n");
        fprintf(file, "register XkbDoodadPtr doodads;\n\n");
        fprintf(file, "    doodads= section->doodads;\n");
        nd = geom->num_doodads;
        doodad = geom->doodads;
    }
    for (d = 0; d < nd; d++, doodad++) {
        if (d != 0)
            fprintf(file, "\n");
        fprintf(file, "    doodads[%d].any.name= GET_ATOM(dpy,\"%s\");\n",
                d, XkbAtomText(dpy, doodad->any.name, XkbCFile));
        fprintf(file, "    doodads[%d].any.type= %s;\n",
                d, XkbDoodadTypeText(doodad->any.type, XkbCFile));
        fprintf(file, "    doodads[%d].any.priority= %d;\n",
                d, doodad->any.priority);
        fprintf(file, "    doodads[%d].any.top= %d;\n", d, doodad->any.top);
        fprintf(file, "    doodads[%d].any.left= %d;\n", d, doodad->any.left);
        fprintf(file, "    doodads[%d].any.angle= %d;\n", d, doodad->any.angle);
        switch (doodad->any.type) {
        case XkbOutlineDoodad:
        case XkbSolidDoodad:
            fprintf(file, "    doodads[%d].shape.color_ndx= %d;\n",
                    d, doodad->shape.color_ndx);
            fprintf(file, "    doodads[%d].shape.shape_ndx= %d;\n",
                    d, doodad->shape.shape_ndx);
            break;
        case XkbTextDoodad:
            fprintf(file, "    doodads[%d].text.width= %d;\n",
                    d, doodad->text.width);
            fprintf(file, "    doodads[%d].text.height= %d;\n",
                    d, doodad->text.height);
            fprintf(file, "    doodads[%d].text.color_ndx= %d;\n",
                    d, doodad->text.color_ndx);
            fprintf(file, "    doodads[%d].text.text= \"%s\";\n",
                    d, XkbStringText(doodad->text.text, XkbCFile));
            fprintf(file, "    doodads[%d].text.font= \"%s\";\n",
                    d, XkbStringText(doodad->text.font, XkbCFile));
            break;
        case XkbIndicatorDoodad:
            fprintf(file, "    doodads[%d].indicator.shape_ndx= %d;\n",
                    d, doodad->indicator.shape_ndx);
            fprintf(file, "    doodads[%d].indicator.on_color_ndx= %d;\n",
                    d, doodad->indicator.on_color_ndx);
            fprintf(file, "    doodads[%d].indicator.off_color_ndx= %d;\n",
                    d, doodad->indicator.off_color_ndx);
            break;
        case XkbLogoDoodad:
            fprintf(file, "    doodads[%d].logo.color_ndx= %d;\n",
                    d, doodad->logo.color_ndx);
            fprintf(file, "    doodads[%d].logo.shape_ndx= %d;\n",
                    d, doodad->logo.shape_ndx);
            fprintf(file, "    doodads[%d].logo.logo_name= \"%s\";\n",
                    d, XkbStringText(doodad->logo.logo_name, XkbCFile));
            break;
        }
    }
    fprintf(file, "}\n");
    return True;
}

static Bool
WriteCHdrGeomOverlays(FILE *            file,
                      XkbDescPtr        xkb,
                      XkbSectionPtr     section,
                      int               section_num)
{
    register int        o, r, k;
    XkbOverlayPtr       ol;
    XkbOverlayRowPtr    row;
    XkbOverlayKeyPtr    key;

    if (section->num_overlays < 1)
        return True;
    for (o = 0, ol = section->overlays; o < section->num_overlays; o++, ol++) {
        for (r = 0, row = ol->rows; r < ol->num_rows; r++, row++) {
            fprintf(file, "static XkbOverlayKeyRec olkeys_s%02dr%02d[%d]= {\n",
                    section_num, r, row->num_keys);
            for (k = 0, key = row->keys; k < row->num_keys; k++, key++) {
                fprintf(file, "%s	{ {\"%s\"},	{\"%s\"}	}",
                        (k == 0 ? "" : ",\n"),
                        XkbKeyNameText(key->over.name, XkbCFile),
                        XkbKeyNameText(key->under.name, XkbCFile));
            }
            fprintf(file, "\n};\n");
        }
        fprintf(file, "static XkbOverlayRowRec olrows_s%02d[%d]= {\n",
                section_num, section->num_rows);
        for (r = 0, row = ol->rows; r < ol->num_rows; r++, row++) {
            fprintf(file, "%s	{ %4d, %4d, %4d, olkeys_s%02dr%02d }",
                    (r == 0 ? "" : ",\n"),
                    row->row_under, row->num_keys, row->num_keys,
                    section_num, r);
        }
        fprintf(file, "\n};\n");
    }
    fprintf(file, "static XkbOverlayRec overlays_s%02d[%d]= {\n", section_num,
            section->num_overlays);
    for (o = 0, ol = section->overlays; o < section->num_overlays; o++, ol++) {
        fprintf(file, "%s	{\n", (o == 0 ? "" : ",\n"));
        fprintf(file, "	    None, 	/* name */\n");
        fprintf(file, "	    NULL,	/* section_under */\n");
        fprintf(file, "	    %4d,	/* num_rows */\n", ol->num_rows);
        fprintf(file, "	    %4d,	/* sz_rows */\n", ol->num_rows);
        fprintf(file, "	    olrows_s%02d,\n", section_num);
        fprintf(file, "	    NULL	/* bounds */\n");
        fprintf(file, "	}");
    }
    fprintf(file, "\n};\n");
    fprintf(file, "static void\n");
    fprintf(file, "_InitS%02dOverlay(", section_num);
    fprintf(file, "    DPYTYPE		dpy,\n");
    fprintf(file, "    XkbGeometryPtr 	geom,\n");
    fprintf(file, "    XkbSectionPtr 	section)\n");
    fprintf(file, "{\n");
    fprintf(file, "XkbOverlayPtr	ol;\n\n");
    fprintf(file, "    ol= section->overlays;\n");
    for (o = 0, ol = section->overlays; o < section->num_overlays; o++, ol++) {
        fprintf(file, "    ol[%2d].name= GET_ATOM(dpy,\"%s\");\n",
                o, XkbAtomText(xkb->dpy, ol->name, XkbCFile));
        fprintf(file, "    ol[%2d].section_under= section;\n", o);
    }
    fprintf(file, "}\n");
    return True;
}

static Bool
WriteCHdrGeomRows(FILE *        file,
                  XkbDescPtr    xkb,
                  XkbSectionPtr section,
                  int           section_num)
{
    register int        k, r;
    register XkbRowPtr  row;
    register XkbKeyPtr  key;

    for (r = 0, row = section->rows; r < section->num_rows; r++, row++) {
        fprintf(file, "static XkbKeyRec keys_s%02dr%02d[]= {\n",
                section_num, r);
        for (k = 0, key = row->keys; k < row->num_keys; k++, key++) {
            fprintf(file, "%s	{ { \"%s\" },	%4d, %4d, %4d }",
                    (k == 0 ? "" : ",\n"),
                    XkbKeyNameText(key->name.name, XkbCFile),
                    key->gap, key->shape_ndx, key->color_ndx);
        }
        fprintf(file, "\n};\n");
    }
    fprintf(file, "static XkbRowRec rows_s%02d[]= {\n", section_num);
    for (r = 0, row = section->rows; r < section->num_rows; r++, row++) {
        fprintf(file, "%s	{ %4d, %4d, %2d, %2d, %1d, keys_s%02dr%02d, ",
                (r == 0 ? "" : ",\n"),
                row->top, row->left, row->num_keys, row->num_keys,
                (row->vertical != 0), section_num, r);
        fprintf(file, " { %4d, %4d, %4d, %4d } }",
                row->bounds.x1, row->bounds.y1, row->bounds.x2, row->bounds.y2);
    }
    fprintf(file, "\n};\n");
    return True;
}

static Bool
WriteCHdrGeomSections(FILE *file, XkbDescPtr xkb, XkbGeometryPtr geom)
{
    register int s;
    register XkbSectionPtr section;

    for (s = 0, section = geom->sections; s < geom->num_sections;
         s++, section++) {
        WriteCHdrGeomRows(file, xkb, section, s);
        if (section->num_overlays > 0)
            WriteCHdrGeomOverlays(file, xkb, section, s);
    }
    fprintf(file, "\nstatic XkbSectionRec g_sections[%d]= {\n",
            geom->num_sections);
    for (s = 0, section = geom->sections; s < geom->num_sections;
         s++, section++) {
        if (s != 0)
            fprintf(file, ",\n");
        fprintf(file, "	{\n	    None, /* name */\n");
        fprintf(file, "	    %4d, /* priority */\n", section->priority);
        fprintf(file, "	    %4d, /* top */\n", section->top);
        fprintf(file, "	    %4d, /* left */\n", section->left);
        fprintf(file, "	    %4d, /* width */\n", section->width);
        fprintf(file, "	    %4d, /* height */\n", section->height);
        fprintf(file, "	    %4d, /* angle */\n", section->angle);
        fprintf(file, "	    %4d, /* num_rows */\n", section->num_rows);
        fprintf(file, "	    %4d, /* num_doodads */\n", section->num_doodads);
        fprintf(file, "	    %4d, /* num_overlays */\n", section->num_overlays);
        fprintf(file, "	    %4d, /* sz_rows */\n", section->num_rows);
        fprintf(file, "	    %4d, /* sz_doodads */\n", section->num_doodads);
        fprintf(file, "	    %4d, /* sz_overlays */\n", section->num_overlays);
        if (section->num_rows > 0)
            fprintf(file, "	    rows_s%02d,\n", s);
        else
            fprintf(file, "	    NULL, /* rows */\n");
        if (section->num_doodads > 0)
            fprintf(file, "	    doodads_s%02d,\n", s);
        else
            fprintf(file, "	    NULL, /* doodads */\n");
        fprintf(file, "	    { %4d, %4d, %4d, %4d }, /* bounds */\n",
                section->bounds.x1, section->bounds.y1,
                section->bounds.x2, section->bounds.y2);
        if (section->num_overlays > 0)
            fprintf(file, "	    overlays_s%02d\n", s);
        else
            fprintf(file, "	    NULL /* overlays */\n");
        fprintf(file, "	}");
    }
    fprintf(file, "\n};\n");
    fprintf(file, "\nstatic Bool\n");
    fprintf(file, "_InitSections(DPYTYPE dpy,XkbGeometryPtr geom)\n");
    fprintf(file, "{\nXkbSectionPtr	sections;\n\n");
    fprintf(file, "    sections= geom->sections;\n");
    for (s = 0, section = geom->sections; s < geom->num_sections;
         s++, section++) {
        if (section->num_doodads > 0) {
            fprintf(file, "    _InitS%02dDoodads(dpy,geom,&sections[%d]);\n",
                    s, s);
        }
        if (section->num_overlays > 0) {
            fprintf(file, "    _InitS%02dOverlays(dpy,geom,&sections[%d]);\n",
                    s, s);
        }
    }
    fprintf(file, "}\n");
    return True;
}

static Bool
WriteCHdrGeomAliases(FILE *file, XkbDescPtr xkb, XkbGeometryPtr geom)
{
    if (geom->num_key_aliases > 0) {
        register int i;

        fprintf(file, "\nstatic XkbKeyAliasRec g_aliases[%d]= {\n",
                geom->num_key_aliases);
        for (i = 0; i < geom->num_key_aliases; i++) {
            fprintf(file, "%s	{	\"%s\", \"%s\"	}",
                    (i == 0 ? "" : ",\n"),
                    XkbKeyNameText(geom->key_aliases[i].real, XkbCFile),
                    XkbKeyNameText(geom->key_aliases[i].alias, XkbCFile));
        }
        fprintf(file, "\n};\n");
    }
    return True;
}

static Bool
WriteCHdrGeometry(FILE *file, XkbDescPtr xkb)
{
    XkbGeometryPtr geom;
    register int i;

    if ((!xkb) || (!xkb->geom)) {
        _XkbLibError(_XkbErrMissingGeometry, "WriteCHdrGeometry", 0);
        return False;
    }
    geom = xkb->geom;
    WriteCHdrGeomProps(file, xkb, geom);
    WriteCHdrGeomColors(file, xkb, geom);
    WriteCHdrGeomShapes(file, xkb, geom);
    WriteCHdrGeomSections(file, xkb, geom);
    WriteCHdrGeomDoodads(file, xkb, geom, NULL, 0);
    WriteCHdrGeomAliases(file, xkb, geom);
    fprintf(file, "\nstatic XkbGeometryRec	geom= {\n");
    fprintf(file, "	None,			/* name */\n");
    fprintf(file, "	%d, %d,		/* width, height */\n",
            geom->width_mm, geom->height_mm);
    if (geom->label_font)
        fprintf(file, "	\"%s\",/* label font */\n",
                XkbStringText(geom->label_font, XkbCFile));
    else
        fprintf(file, "	NULL,		/* label font */\n");
    if (geom->label_color)
        fprintf(file, "	&g_colors[%d],		/* label color */\n",
                XkbGeomColorIndex(geom, geom->label_color));
    else
        fprintf(file, "	NULL,			/* label color */\n");
    if (geom->base_color)
        fprintf(file, "	&g_colors[%d],		/* base color */\n",
                XkbGeomColorIndex(geom, geom->base_color));
    else
        fprintf(file, "	NULL,			/* base color */\n");
    fprintf(file,
            "	%d,	%d,	%d,	/*  sz: props, colors, shapes */\n",
            geom->num_properties, geom->num_colors, geom->num_shapes);
    fprintf(file,
            "	%d,	%d,	%d,	/*  sz: sections, doodads, aliases */\n",
            geom->num_sections, geom->num_doodads, geom->num_key_aliases);
    fprintf(file,
            "	%d,	%d,	%d,	/* num: props, colors, shapes */\n",
            geom->num_properties, geom->num_colors, geom->num_shapes);
    fprintf(file,
            "	%d,	%d,	%d,	/* num: sections, doodads, aliases */\n",
            geom->num_sections, geom->num_doodads, geom->num_key_aliases);
    fprintf(file, "	%s,	%s,	%s,\n",
            (geom->num_properties > 0 ? "g_props" : "NULL"),
            (geom->num_colors > 0 ? "g_colors" : "NULL"),
            (geom->num_shapes > 0 ? "g_shapes" : "NULL"));
    fprintf(file, "	%s,	%s,	%s\n",
            (geom->num_sections > 0 ? "g_sections" : "NULL"),
            (geom->num_doodads > 0 ? "g_doodads" : "NULL"),
            (geom->num_key_aliases > 0 ? "g_aliases" : "NULL"));
    fprintf(file, "};\n\n");
    fprintf(file, "static Bool\n");
    fprintf(file, "_InitHdrGeom(DPYTYPE dpy,XkbGeometryPtr geom)\n");
    fprintf(file, "{\n");
    if (geom->name != None) {
        fprintf(file, "    geom->name= GET_ATOM(dpy,\"%s\");\n",
                XkbAtomText(xkb->dpy, geom->name, XkbCFile));
    }
    for (i = 0; i < geom->num_shapes; i++) {
        fprintf(file, "    geom->shapes[%2d].name= GET_ATOM(dpy,\"%s\");\n", i,
                XkbAtomText(xkb->dpy, geom->shapes[i].name, XkbCFile));
    }
    if (geom->num_doodads > 0)
        fprintf(file, "    _InitGeomDoodads(dpy,geom);\n");
    fprintf(file, "    _InitSections(dpy,geom);\n");
    fprintf(file, "}\n\n");
    return True;
}

static Bool
WriteCHdrGeomFile(FILE *file, XkbFileInfo *result)
{
    Bool ok;

    ok = WriteCHdrGeometry(file, result->xkb);
    return ok;
}

static Bool
WriteCHdrLayout(FILE *file, XkbFileInfo *result)
{
    Bool ok;
    XkbDescPtr xkb;

    xkb = result->xkb;
    ok = WriteCHdrVMods(file, xkb->dpy, xkb);
    ok = WriteCHdrKeycodes(file, xkb) && ok;
    ok = WriteCHdrSymbols(file, xkb) && ok;
    ok = WriteCHdrGeometry(file, xkb) && ok;
    return ok;
}

static Bool
WriteCHdrSemantics(FILE *file, XkbFileInfo *result)
{
    Bool ok;
    XkbDescPtr xkb;

    xkb = result->xkb;
    ok = WriteCHdrVMods(file, xkb->dpy, xkb);
    ok = WriteCHdrKeyTypes(file, xkb->dpy, xkb) && ok;
    ok = WriteCHdrCompatMap(file, xkb->dpy, xkb) && ok;
    ok = WriteCHdrIndicators(file, xkb->dpy, xkb) && ok;
    return ok;
}

static Bool
WriteCHdrKeymap(FILE *file, XkbFileInfo *result)
{
    Bool ok;
    XkbDescPtr xkb;

    xkb = result->xkb;
    ok = WriteCHdrVMods(file, xkb->dpy, xkb);
    ok = ok && WriteCHdrKeycodes(file, xkb);
    ok = ok && WriteCHdrClientMap(file, xkb->dpy, xkb);
    ok = ok && WriteCHdrServerMap(file, xkb->dpy, xkb);
    ok = ok && WriteCHdrCompatMap(file, xkb->dpy, xkb);
    ok = WriteCHdrIndicators(file, xkb->dpy, xkb) && ok;
    ok = ok && WriteCHdrGeometry(file, xkb);
    return ok;
}

Bool
XkbWriteCFile(FILE *out, char *name, XkbFileInfo *result)
{
    Bool ok;
    XkbDescPtr xkb;

    Bool (*func) (FILE *        /* file */,
                  XkbFileInfo * /* result */
        );

    switch (result->type) {
    case XkmSemanticsFile:
        func = WriteCHdrSemantics;
        break;
    case XkmLayoutFile:
        func = WriteCHdrLayout;
        break;
    case XkmKeymapFile:
        func = WriteCHdrKeymap;
        break;
    case XkmGeometryIndex:
    case XkmGeometryFile:
        func = WriteCHdrGeomFile;
        break;
    default:
        _XkbLibError(_XkbErrIllegalContents, "XkbWriteCFile", result->type);
        return False;
    }
    xkb = result->xkb;
    if (out == NULL) {
        _XkbLibError(_XkbErrFileCannotOpen, "XkbWriteCFile", 0);
        ok = False;
    }
    else {
        char *tmp, *hdrdef;

        tmp = (char *) strrchr(name, '/');
        if (tmp == NULL)
            tmp = name;
        else
            tmp++;
        hdrdef = strdup(tmp);
        if (hdrdef) {
            tmp = hdrdef;
            while (*tmp) {
                if (islower(*tmp))
                    *tmp = toupper(*tmp);
                else if (!isalnum(*tmp))
                    *tmp = '_';
                tmp++;
            }
            fprintf(out, "/* This file generated automatically by xkbcomp */\n");
            fprintf(out, "/* DO  NOT EDIT */\n");
            fprintf(out, "#ifndef %s\n", hdrdef);
            fprintf(out, "#define %s 1\n\n", hdrdef);
        }
        fprintf(out, "#ifndef XKB_IN_SERVER\n");
        fprintf(out, "#define GET_ATOM(d,s)	XInternAtom(d,s,0)\n");
        fprintf(out, "#define DPYTYPE	Display *\n");
        fprintf(out, "#else\n");
        fprintf(out, "#define GET_ATOM(d,s)	MakeAtom(s,strlen(s),1)\n");
        fprintf(out, "#define DPYTYPE	char *\n");
        fprintf(out, "#endif\n");
        fprintf(out, "#define NUM_KEYS	%d\n", xkb->max_key_code + 1);
        ok = (*func) (out, result);
        if (hdrdef) {
            fprintf(out, "#endif /* %s */\n", hdrdef);
            free(hdrdef);
        }
    }

    if (!ok) {
        return False;
    }
    return True;
}
