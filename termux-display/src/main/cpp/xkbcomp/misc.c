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

#include "xkbcomp.h"
#include "xkbpath.h"
#include "tokens.h"
#include "keycodes.h"
#include "misc.h"
#include <X11/keysym.h>
#include "parseutils.h"

#include <X11/extensions/XKBgeom.h>

/***====================================================================***/

/**
 * Open the file given in the include statement and parse it's content.
 * If the statement defines a specific map to use, this map is returned in
 * file_rtrn. Otherwise, the default map is returned.
 *
 * @param stmt The include statement, specifying the file name to look for.
 * @param file_type Type of file (XkmKeyNamesIdx, etc.)
 * @param file_rtrn Returns the key map to be used.
 * @param merge_rtrn Always returns stmt->merge.
 *
 * @return True on success or False otherwise.
 */
Bool
ProcessIncludeFile(IncludeStmt * stmt,
                   unsigned file_type,
                   XkbFile ** file_rtrn, unsigned *merge_rtrn)
{
    XkbFile *rtrn, *mapToUse;
    char oldFile[1024] = {0};
    int oldLine = lineNum;

    rtrn = XkbFindFileInCache(stmt->file, file_type, &stmt->path);
    if (rtrn == NULL)
    {
        /* file not in cache, open it, parse it and store it in cache for next
           time. */
        FILE *file = XkbFindFileInPath(stmt->file, file_type, &stmt->path);
        if (file == NULL)
        {
            ERROR("Can't find file \"%s\" for %s include\n", stmt->file,
                   XkbDirectoryForInclude(file_type));
            ACTION("Exiting\n");
            return False;
        }
        strcpy(oldFile, scanFile);
        oldLine = lineNum;
        setScanState(stmt->file, 1);
#ifdef DEBUG
        if (debugFlags & 2)
            INFO("About to parse include file %s\n", stmt->file);
#endif
        /* parse the file */
        if ((XKBParseFile(file, &rtrn) == 0) || (rtrn == NULL))
        {
            setScanState(oldFile, oldLine);
            ERROR("Error interpreting include file \"%s\"\n", stmt->file);
            ACTION("Exiting\n");
            fclose(file);
            return False;
        }
        fclose(file);
        XkbAddFileToCache(stmt->file, file_type, stmt->path, rtrn);
    }

    /*
     * A single file may contain several maps. Here's how we choose the map:
     * - If a specific map was requested, look for it exclusively.
     * - Otherwise, if the file only contains a single map, return it.
     * - Otherwise, if the file has maps tagged as default, return the
     *   first one.
     * - If all fails, return the first map in the file.
     */
    mapToUse = rtrn;
    if (stmt->map != NULL)
    {
        while ((mapToUse) && ((!uStringEqual(mapToUse->name, stmt->map)) ||
                              (mapToUse->type != file_type)))
        {
            mapToUse = (XkbFile *) mapToUse->common.next;
        }
        if (!mapToUse)
        {
            ERROR("No %s named \"%s\" in the include file \"%s\"\n",
                   XkbConfigText(file_type, XkbMessage), stmt->map,
                   stmt->file);
            ACTION("Exiting\n");
            return False;
        }
    }
    else if (rtrn->common.next != NULL)
    {
        while ((mapToUse) && !(mapToUse->flags & XkbLC_Default))
        {
            mapToUse = (XkbFile *) mapToUse->common.next;
        }
        if (!mapToUse)
        {
            if (warningLevel > 5)
            {
                WARN("No map in include statement, but \"%s\" contains several without a default map\n",
                      stmt->file);
                ACTION("Using first defined map, \"%s\"\n", rtrn->name);
            }
            mapToUse = rtrn;
        }
    }

    setScanState(oldFile, oldLine);
    if (mapToUse->type != file_type)
    {
        ERROR("Include file wrong type (expected %s, got %s)\n",
               XkbConfigText(file_type, XkbMessage),
               XkbConfigText(mapToUse->type, XkbMessage));
        ACTION("Include file \"%s\" ignored\n", stmt->file);
        return False;
    }
    /* FIXME: we have to check recursive includes here (or somewhere) */

    mapToUse->compiled = True;
    *file_rtrn = mapToUse;
    *merge_rtrn = stmt->merge;
    return True;
}

/***====================================================================***/

int
ReportNotArray(const char *type, const char *field, const char *name)
{
    ERROR("The %s %s field is not an array\n", type, field);
    ACTION("Ignoring illegal assignment in %s\n", name);
    return False;
}

int
ReportShouldBeArray(const char *type, const char *field, char *name)
{
    ERROR("Missing subscript for %s %s\n", type, field);
    ACTION("Ignoring illegal assignment in %s\n", name);
    return False;
}

int
ReportBadType(const char *type, const char *field,
              const char *name, const char *wanted)
{
    ERROR("The %s %s field must be a %s\n", type, field, wanted);
    ACTION("Ignoring illegal assignment in %s\n", name);
    return False;
}

#if 0
int
ReportBadIndexType(char *type, char *field, char *name, char *wanted)
{
    ERROR("Index for the %s %s field must be a %s\n", type, field, wanted);
    ACTION("Ignoring assignment to illegal field in %s\n", name);
    return False;
}
#endif

int
ReportBadField(const char *type, const char *field, const char *name)
{
    ERROR("Unknown %s field %s in %s\n", type, field, name);
    ACTION("Ignoring assignment to unknown field in %s\n", name);
    return False;
}

#if 0
int
ReportMultipleDefs(char *type, char *field, char *name)
{
    WARN("Multiple definitions of %s in %s \"%s\"\n", field, type, name);
    ACTION("Using last definition\n");
    return False;
}
#endif

/***====================================================================***/

Bool
UseNewField(unsigned field, const CommonInfo *oldDefs,
            const CommonInfo *newDefs, unsigned *pCollide)
{
    Bool useNew;

    useNew = False;
    if (oldDefs->defined & field)
    {
        if (newDefs->defined & field)
        {
            if (((oldDefs->fileID == newDefs->fileID)
                 && (warningLevel > 0)) || (warningLevel > 9))
            {
                *pCollide |= field;
            }
            if (newDefs->merge != MergeAugment)
                useNew = True;
        }
    }
    else if (newDefs->defined & field)
        useNew = True;
    return useNew;
}

#if 0
static Bool
MergeNewField(unsigned field, const CommonInfo *oldDefs,
            const CommonInfo *newDefs, unsigned *pCollide)
{
    if ((oldDefs->defined & field) && (newDefs->defined & field))
    {
        if (((oldDefs->fileID == newDefs->fileID) && (warningLevel > 0)) ||
            (warningLevel > 9))
        {
            *pCollide |= field;
        }
        if (newDefs->merge == MergeAugment)
            return True;
    }
    return False;
}
#endif

XPointer
ClearCommonInfo(CommonInfo * cmn)
{
    if (cmn != NULL)
    {
        CommonInfo *this, *next;
        for (this = cmn; this != NULL; this = next)
        {
            next = this->next;
            free(this);
        }
    }
    return NULL;
}

XPointer
AddCommonInfo(CommonInfo * old, CommonInfo * new)
{
    CommonInfo *first;

    first = old;
    while (old && old->next)
    {
        old = old->next;
    }
    new->next = NULL;
    if (old)
    {
        old->next = new;
        return (XPointer) first;
    }
    return (XPointer) new;
}

/***====================================================================***/

typedef struct _KeyNameDesc
{
    KeySym level1;
    KeySym level2;
    char name[5];
    Bool used;
} KeyNameDesc;

static KeyNameDesc dfltKeys[] = {
    {XK_Escape, NoSymbol, "ESC\0", 0},
    {XK_quoteleft, XK_asciitilde, "TLDE", 0},
    {XK_1, XK_exclam, "AE01", 0},
    {XK_2, XK_at, "AE02", 0},
    {XK_3, XK_numbersign, "AE03", 0},
    {XK_4, XK_dollar, "AE04", 0},
    {XK_5, XK_percent, "AE05", 0},
    {XK_6, XK_asciicircum, "AE06", 0},
    {XK_7, XK_ampersand, "AE07", 0},
    {XK_8, XK_asterisk, "AE08", 0},
    {XK_9, XK_parenleft, "AE09", 0},
    {XK_0, XK_parenright, "AE10", 0},
    {XK_minus, XK_underscore, "AE11", 0},
    {XK_equal, XK_plus, "AE12", 0},
    {XK_BackSpace, NoSymbol, "BKSP", 0},
    {XK_Tab, NoSymbol, "TAB\0", 0},
    {XK_q, XK_Q, "AD01", 0},
    {XK_w, XK_W, "AD02", 0},
    {XK_e, XK_E, "AD03", 0},
    {XK_r, XK_R, "AD04", 0},
    {XK_t, XK_T, "AD05", 0},
    {XK_y, XK_Y, "AD06", 0},
    {XK_u, XK_U, "AD07", 0},
    {XK_i, XK_I, "AD08", 0},
    {XK_o, XK_O, "AD09", 0},
    {XK_p, XK_P, "AD10", 0},
    {XK_bracketleft, XK_braceleft, "AD11", 0},
    {XK_bracketright, XK_braceright, "AD12", 0},
    {XK_Return, NoSymbol, "RTRN", 0},
    {XK_Caps_Lock, NoSymbol, "CAPS", 0},
    {XK_a, XK_A, "AC01", 0},
    {XK_s, XK_S, "AC02", 0},
    {XK_d, XK_D, "AC03", 0},
    {XK_f, XK_F, "AC04", 0},
    {XK_g, XK_G, "AC05", 0},
    {XK_h, XK_H, "AC06", 0},
    {XK_j, XK_J, "AC07", 0},
    {XK_k, XK_K, "AC08", 0},
    {XK_l, XK_L, "AC09", 0},
    {XK_semicolon, XK_colon, "AC10", 0},
    {XK_quoteright, XK_quotedbl, "AC11", 0},
    {XK_Shift_L, NoSymbol, "LFSH", 0},
    {XK_z, XK_Z, "AB01", 0},
    {XK_x, XK_X, "AB02", 0},
    {XK_c, XK_C, "AB03", 0},
    {XK_v, XK_V, "AB04", 0},
    {XK_b, XK_B, "AB05", 0},
    {XK_n, XK_N, "AB06", 0},
    {XK_m, XK_M, "AB07", 0},
    {XK_comma, XK_less, "AB08", 0},
    {XK_period, XK_greater, "AB09", 0},
    {XK_slash, XK_question, "AB10", 0},
    {XK_backslash, XK_bar, "BKSL", 0},
    {XK_Control_L, NoSymbol, "LCTL", 0},
    {XK_space, NoSymbol, "SPCE", 0},
    {XK_Shift_R, NoSymbol, "RTSH", 0},
    {XK_Alt_L, NoSymbol, "LALT", 0},
    {XK_space, NoSymbol, "SPCE", 0},
    {XK_Control_R, NoSymbol, "RCTL", 0},
    {XK_Alt_R, NoSymbol, "RALT", 0},
    {XK_F1, NoSymbol, "FK01", 0},
    {XK_F2, NoSymbol, "FK02", 0},
    {XK_F3, NoSymbol, "FK03", 0},
    {XK_F4, NoSymbol, "FK04", 0},
    {XK_F5, NoSymbol, "FK05", 0},
    {XK_F6, NoSymbol, "FK06", 0},
    {XK_F7, NoSymbol, "FK07", 0},
    {XK_F8, NoSymbol, "FK08", 0},
    {XK_F9, NoSymbol, "FK09", 0},
    {XK_F10, NoSymbol, "FK10", 0},
    {XK_F11, NoSymbol, "FK11", 0},
    {XK_F12, NoSymbol, "FK12", 0},
    {XK_Print, NoSymbol, "PRSC", 0},
    {XK_Scroll_Lock, NoSymbol, "SCLK", 0},
    {XK_Pause, NoSymbol, "PAUS", 0},
    {XK_Insert, NoSymbol, "INS\0", 0},
    {XK_Home, NoSymbol, "HOME", 0},
    {XK_Prior, NoSymbol, "PGUP", 0},
    {XK_Delete, NoSymbol, "DELE", 0},
    {XK_End, NoSymbol, "END", 0},
    {XK_Next, NoSymbol, "PGDN", 0},
    {XK_Up, NoSymbol, "UP\0\0", 0},
    {XK_Left, NoSymbol, "LEFT", 0},
    {XK_Down, NoSymbol, "DOWN", 0},
    {XK_Right, NoSymbol, "RGHT", 0},
    {XK_Num_Lock, NoSymbol, "NMLK", 0},
    {XK_KP_Divide, NoSymbol, "KPDV", 0},
    {XK_KP_Multiply, NoSymbol, "KPMU", 0},
    {XK_KP_Subtract, NoSymbol, "KPSU", 0},
    {NoSymbol, XK_KP_7, "KP7\0", 0},
    {NoSymbol, XK_KP_8, "KP8\0", 0},
    {NoSymbol, XK_KP_9, "KP9\0", 0},
    {XK_KP_Add, NoSymbol, "KPAD", 0},
    {NoSymbol, XK_KP_4, "KP4\0", 0},
    {NoSymbol, XK_KP_5, "KP5\0", 0},
    {NoSymbol, XK_KP_6, "KP6\0", 0},
    {NoSymbol, XK_KP_1, "KP1\0", 0},
    {NoSymbol, XK_KP_2, "KP2\0", 0},
    {NoSymbol, XK_KP_3, "KP3\0", 0},
    {XK_KP_Enter, NoSymbol, "KPEN", 0},
    {NoSymbol, XK_KP_0, "KP0\0", 0},
    {XK_KP_Delete, NoSymbol, "KPDL", 0},
    {XK_less, XK_greater, "LSGT", 0},
    {XK_KP_Separator, NoSymbol, "KPCO", 0},
    {XK_Find, NoSymbol, "FIND", 0},
    {NoSymbol, NoSymbol, "\0\0\0\0", 0}
};

Status
ComputeKbdDefaults(XkbDescPtr xkb)
{
    int nUnknown;

    if ((xkb->names == NULL) || (xkb->names->keys == NULL))
    {
        Status rtrn;
        if ((rtrn = XkbAllocNames(xkb, XkbKeyNamesMask, 0, 0)) != Success)
            return rtrn;
    }
    for (KeyNameDesc *name = dfltKeys; (name->name[0] != '\0'); name++)
    {
        name->used = False;
    }
    nUnknown = 0;
    for (int i = xkb->min_key_code; i <= xkb->max_key_code; i++)
    {
        int tmp = XkbKeyNumSyms(xkb, i);
        if ((xkb->names->keys[i].name[0] == '\0') && (tmp > 0))
        {
            KeySym *syms;

            tmp = XkbKeyGroupsWidth(xkb, i);
            syms = XkbKeySymsPtr(xkb, i);
            for (KeyNameDesc *name = dfltKeys; (name->name[0] != '\0'); name++)
            {
                Bool match = True;
                if (((name->level1 != syms[0])
                     && (name->level1 != NoSymbol))
                    || ((name->level2 != NoSymbol) && (tmp < 2))
                    || ((name->level2 != syms[1])
                        && (name->level2 != NoSymbol)))
                {
                    match = False;
                }
                if (match)
                {
                    if (!name->used)
                    {
                        memcpy(xkb->names->keys[i].name, name->name,
                               XkbKeyNameLength);
                        name->used = True;
                    }
                    else
                    {
                        char tmpname[XkbKeyNameLength + 1];

                        if (warningLevel > 2)
                        {
                            WARN
                                ("Several keys match pattern for %s\n",
                                 XkbKeyNameText(name->name, XkbMessage));
                            ACTION("Using <U%03d> for key %d\n",
                                    nUnknown, i);
                        }
                        snprintf(tmpname, sizeof(tmpname), "U%03d",
                                 nUnknown++);
                        memcpy(xkb->names->keys[i].name, tmpname,
                               XkbKeyNameLength);
                    }
                    break;
                }
            }
            if (xkb->names->keys[i].name[0] == '\0')
            {
                if (warningLevel > 2)
                {
                    char tmpname[XkbKeyNameLength + 1];

                    WARN("Key %d does not match any defaults\n", i);
                    ACTION("Using name <U%03d>\n", nUnknown);
                    snprintf(tmpname, sizeof(tmpname), "U%03d", nUnknown++);
                    memcpy(xkb->names->keys[i].name, tmpname,
                           XkbKeyNameLength);
                }
            }
        }
    }
    return Success;
}

/**
 * Find the key with the given name and return its keycode in kc_rtrn.
 *
 * @param name The 4-letter name of the key as a long.
 * @param kc_rtrn Set to the keycode if the key was found, otherwise 0.
 * @param use_aliases True if the key aliases should be searched too.
 * @param create If True and the key is not found, it is added to the
 *        xkb->names at the first free keycode.
 * @param start_from Keycode to start searching from.
 *
 * @return True if found, False otherwise.
 */
Bool
FindNamedKey(XkbDescPtr xkb,
             unsigned long name,
             unsigned int *kc_rtrn,
             Bool use_aliases, Bool create, int start_from)
{
    if (start_from < xkb->min_key_code)
    {
        start_from = xkb->min_key_code;
    }
    else if (start_from > xkb->max_key_code)
    {
        return False;
    }

    *kc_rtrn = 0;               /* some callers rely on this */
    if (xkb && xkb->names && xkb->names->keys)
    {
        for (unsigned n = start_from; n <= xkb->max_key_code; n++)
        {
            unsigned long tmp;
            tmp = KeyNameToLong(xkb->names->keys[n].name);
            if (tmp == name)
            {
                *kc_rtrn = n;
                return True;
            }
        }
        if (use_aliases)
        {
            unsigned long new_name;
            if (FindKeyNameForAlias(xkb, name, &new_name))
                return FindNamedKey(xkb, new_name, kc_rtrn, False, create, 0);
        }
    }
    if (create)
    {
        if ((!xkb->names) || (!xkb->names->keys))
        {
            if (xkb->min_key_code < XkbMinLegalKeyCode)
            {
                xkb->min_key_code = XkbMinLegalKeyCode;
                xkb->max_key_code = XkbMaxLegalKeyCode;
            }
            if (XkbAllocNames(xkb, XkbKeyNamesMask, 0, 0) != Success)
            {
                if (warningLevel > 0)
                {
                    WARN("Couldn't allocate key names in FindNamedKey\n");
                    ACTION("Key \"%s\" not automatically created\n",
                            longText(name, XkbMessage));
                }
                return False;
            }
        }
        /* Find first unused keycode and store our key here */
        for (unsigned n = xkb->min_key_code; n <= xkb->max_key_code; n++)
        {
            if (xkb->names->keys[n].name[0] == '\0')
            {
                char buf[XkbKeyNameLength + 1];
                LongToKeyName(name, buf);
                memcpy(xkb->names->keys[n].name, buf, XkbKeyNameLength);
                *kc_rtrn = n;
                return True;
            }
        }
    }
    return False;
}

Bool
FindKeyNameForAlias(XkbDescPtr xkb, unsigned long lname,
                    unsigned long *real_name)
{
    char name[XkbKeyNameLength + 1];

    if (xkb && xkb->geom && xkb->geom->key_aliases)
    {
        XkbKeyAliasPtr a;
        a = xkb->geom->key_aliases;
        LongToKeyName(lname, name);
        name[XkbKeyNameLength] = '\0';
        for (int i = 0; i < xkb->geom->num_key_aliases; i++, a++)
        {
            if (strncmp(name, a->alias, XkbKeyNameLength) == 0)
            {
                *real_name = KeyNameToLong(a->real);
                return True;
            }
        }
    }
    if (xkb && xkb->names && xkb->names->key_aliases)
    {
        XkbKeyAliasPtr a;
        a = xkb->names->key_aliases;
        LongToKeyName(lname, name);
        name[XkbKeyNameLength] = '\0';
        for (int i = 0; i < xkb->names->num_key_aliases; i++, a++)
        {
            if (strncmp(name, a->alias, XkbKeyNameLength) == 0)
            {
                *real_name = KeyNameToLong(a->real);
                return True;
            }
        }
    }
    return False;
}
