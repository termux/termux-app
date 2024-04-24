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

#define	DEBUG_VAR debugFlags
#include <stdio.h>
#include "xkbcomp.h"
#include "tokens.h"
#include "expr.h"
#include "misc.h"

#include <X11/extensions/XKB.h>
#include <X11/extensions/XKBstr.h>

#include "vmod.h"

void
InitVModInfo(VModInfo * info, XkbDescPtr xkb)
{
    ClearVModInfo(info, xkb);
    info->errorCount = 0;
    return;
}

void
ClearVModInfo(VModInfo * info, XkbDescPtr xkb)
{
    if (XkbAllocNames(xkb, XkbVirtualModNamesMask, 0, 0) != Success)
        return;
    if (XkbAllocServerMap(xkb, XkbVirtualModsMask, 0) != Success)
        return;
    info->xkb = xkb;
    info->newlyDefined = info->defined = info->available = 0;
    if (xkb && xkb->names)
    {
        int i, bit;

        for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1)
        {
            if (xkb->names->vmods[i] != None)
                info->defined |= bit;
        }
    }
    return;
}

/***====================================================================***/

/**
 * Handle one entry in the virtualModifiers line (e.g. NumLock).
 * If the entry is e.g. NumLock=Mod1, stmt->value is not NULL, and the
 * XkbServerMap's vmod is set to the given modifier. Otherwise, the vmod is 0.
 *
 * @param stmt The statement specifying the name and (if any the value).
 * @param mergeMode Merge strategy (e.g. MergeOverride)
 */
Bool
HandleVModDef(const VModDef *stmt, unsigned mergeMode, VModInfo *info)
{
    int i, bit, nextFree;
    ExprResult mod;
    XkbServerMapPtr srv;
    XkbNamesPtr names;
    Atom stmtName;

    srv = info->xkb->server;
    names = info->xkb->names;
    stmtName =
        XkbInternAtom(info->xkb->dpy, XkbAtomGetString(NULL, stmt->name),
                      False);
    for (i = 0, bit = 1, nextFree = -1; i < XkbNumVirtualMods; i++, bit <<= 1)
    {
        if (info->defined & bit)
        {
            if (names->vmods[i] == stmtName)
            {                   /* already defined */
                info->available |= bit;
                if (stmt->value == NULL)
                    return True;
                else
                {
                    char *str1;
                    const char *str2 = "";
                    if (!ExprResolveModMask(stmt->value, &mod, NULL, NULL))
                    {
                        str1 = XkbAtomText(NULL, stmt->name, XkbMessage);
                        ACTION("Declaration of %s ignored\n", str1);
                        return False;
                    }
                    if (mod.uval == srv->vmods[i])
                        return True;

                    str1 = XkbAtomText(NULL, stmt->name, XkbMessage);
                    WARN("Virtual modifier %s multiply defined\n", str1);
                    str1 = XkbModMaskText(srv->vmods[i], XkbCFile);
                    if (mergeMode == MergeOverride)
                    {
                        str2 = str1;
                        str1 = XkbModMaskText(mod.uval, XkbCFile);
                    }
                    ACTION("Using %s, ignoring %s\n", str1, str2);
                    if (mergeMode == MergeOverride)
                        srv->vmods[i] = mod.uval;
                    return True;
                }
            }
        }
        else if (nextFree < 0)
            nextFree = i;
    }
    if (nextFree < 0)
    {
        ERROR("Too many virtual modifiers defined (maximum %d)\n",
               XkbNumVirtualMods);
        ACTION("Exiting\n");
        return False;
    }
    info->defined |= (1U << nextFree);
    info->newlyDefined |= (1U << nextFree);
    info->available |= (1U << nextFree);
    names->vmods[nextFree] = stmtName;
    if (stmt->value == NULL)
        return True;
    if (ExprResolveModMask(stmt->value, &mod, NULL, NULL))
    {
        srv->vmods[nextFree] = mod.uval;
        return True;
    }
    ACTION("Declaration of %s ignored\n",
            XkbAtomText(NULL, stmt->name, XkbMessage));
    return False;
}

/**
 * Returns the index of the given modifier in the xkb->names->vmods array.
 *
 * @param priv Pointer to the xkb data structure.
 * @param elem Must be None, otherwise return False.
 * @param field The Atom of the modifier's name (e.g. Atom for LAlt)
 * @param type Must be TypeInt, otherwise return False.
 * @param val_rtrn Set to the index of the modifier that matches.
 *
 * @return True on success, False otherwise. If False is returned, val_rtrn is
 * undefined.
 */
static int
LookupVModIndex(XPointer priv,
                Atom elem, Atom field, unsigned type, ExprResult * val_rtrn)
{
    int i;
    char *fieldStr;
    XkbDescPtr xkb;

    xkb = (XkbDescPtr) priv;
    if ((xkb == NULL) || (xkb->names == NULL) || (elem != None)
        || (type != TypeInt))
    {
        return False;
    }
    /* get the actual name */
    fieldStr = XkbAtomGetString(xkb->dpy, field);
    if (fieldStr == NULL)
        return False;
    /* For each named modifier, get the name and compare it to the one passed
     * in. If we get a match, return the index of the modifier.
     * The order of modifiers is the same as in the virtual_modifiers line in
     * the xkb_types section.
     */
    for (i = 0; i < XkbNumVirtualMods; i++)
    {
        char *modStr = XkbAtomGetString(xkb->dpy, xkb->names->vmods[i]);
        if ((modStr != NULL) && (uStrCaseCmp(fieldStr, modStr) == 0))
        {
            val_rtrn->uval = i;
            return True;
        }
    }
    return False;
}

/**
 * Get the mask for the given modifier and set val_rtrn.uval to the mask.
 * Note that the mask returned is always > 512.
 *
 * @param priv Pointer to xkb data structure.
 * @param val_rtrn Set to the mask returned.
 *
 * @return True on success, False otherwise. If False is returned, val_rtrn is
 * undefined.
 */
int
LookupVModMask(XPointer priv,
               Atom elem, Atom field, unsigned type, ExprResult * val_rtrn)
{
    if (LookupVModIndex(priv, elem, field, type, val_rtrn))
    {
        unsigned ndx = val_rtrn->uval;
        val_rtrn->uval = (1U << (XkbNumModifiers + ndx));
        return True;
    }
    return False;
}

int
FindKeypadVMod(XkbDescPtr xkb)
{
    if (xkb) {
        Atom name = XkbInternAtom(xkb->dpy, "NumLock", False);
        ExprResult rtrn;

        if (LookupVModIndex((XPointer) xkb, None, name, TypeInt, &rtrn))
        {
            return rtrn.ival;
        }
    }
    return -1;
}

Bool
ResolveVirtualModifier(const ExprDef *def, ExprResult *val_rtrn,
                       const VModInfo *info)
{
    XkbNamesPtr names;

    names = info->xkb->names;
    if (def->op == ExprIdent)
    {
        int i, bit;
        for (i = 0, bit = 1; i < XkbNumVirtualMods; i++, bit <<= 1)
        {
            char *str1, *str2;
            str1 = XkbAtomGetString(info->xkb->dpy, names->vmods[i]);
            str2 = XkbAtomGetString(NULL, def->value.str);
            if ((info->available & bit) && (uStrCaseCmp(str1, str2) == Equal))
            {
                val_rtrn->uval = i;
                return True;
            }
        }
    }
    if (ExprResolveInteger(def, val_rtrn, NULL, NULL))
    {
        if (val_rtrn->uval < XkbNumVirtualMods)
            return True;
        ERROR("Illegal virtual modifier %d (must be 0..%d inclusive)\n",
               val_rtrn->uval, XkbNumVirtualMods - 1);
    }
    return False;
}
