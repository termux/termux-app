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

#include <X11/Xfuncs.h>

#include <X11/Xfuncs.h>


#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include "XKBfileInt.h"


#include <X11/extensions/XKBconfig.h>

/***====================================================================***/

#define	XKBCF_MAX_STR_LEN	100
static char _XkbCF_rtrn[XKBCF_MAX_STR_LEN + 1];

static int
ScanIdent(FILE *file, int ch, XkbCFScanResultPtr val_rtrn)
{
    register int i;
    char *str;

    val_rtrn->str = str = _XkbCF_rtrn;
    for (i = 0; (isalpha(ch) || isdigit(ch) || (ch == '_')); ch = getc(file)) {
        if (i < XKBCF_MAX_STR_LEN)
            str[i++] = ch;
    }
    if ((ch != EOF) && (ch != ' ') && (ch != '\t'))
        ungetc(ch, file);
    str[i] = '\0';
    return XkbCF_Ident;
}

static int
ScanString(FILE *file, int quote, XkbCFScanResultPtr val_rtrn)
{
    int ch, nInBuf;

    nInBuf = 0;
    while (((ch = getc(file)) != EOF) && (ch != '\n') && (ch != quote)) {
        if (ch == '\\') {
            if ((ch = getc(file)) != EOF) {
                if (ch == 'n')
                    ch = '\n';
                else if (ch == 't')
                    ch = '\t';
                else if (ch == 'v')
                    ch = '\v';
                else if (ch == 'b')
                    ch = '\b';
                else if (ch == 'r')
                    ch = '\r';
                else if (ch == 'f')
                    ch = '\f';
                else if (ch == 'e')
                    ch = '\033';
                else if (ch == '0') {
                    int tmp, stop;

                    ch = stop = 0;
                    if (((tmp = getc(file)) != EOF) && (isdigit(tmp)) &&
                        (tmp != '8') && (tmp != '9')) {
                        ch = (ch * 8) + (tmp - '0');
                    }
                    else {
                        stop = 1;
                        ungetc(tmp, file);
                    }
                    if ((!stop) && ((tmp = getc(file)) != EOF) && (isdigit(tmp))
                        && (tmp != '8') && (tmp != '9')) {
                        ch = (ch * 8) + (tmp - '0');
                    }
                    else {
                        stop = 1;
                        ungetc(tmp, file);
                    }
                    if ((!stop) && ((tmp = getc(file)) != EOF) && (isdigit(tmp))
                        && (tmp != '8') && (tmp != '9')) {
                        ch = (ch * 8) + (tmp - '0');
                    }
                    else {
                        stop = 1;
                        ungetc(tmp, file);
                    }
                }
            }
            else
                return XkbCF_EOF;
        }

        if (nInBuf < XKBCF_MAX_STR_LEN - 1)
            _XkbCF_rtrn[nInBuf++] = ch;
    }
    if (ch == quote) {
        _XkbCF_rtrn[nInBuf++] = '\0';
        val_rtrn->str = _XkbCF_rtrn;
        return XkbCF_String;
    }
    return XkbCF_UnterminatedString;
}

static int
ScanInteger(FILE *file, int ch, XkbCFScanResultPtr val_rtrn)
{
    int i;

    if (isdigit(ch))
        ungetc(ch, file);
    if (fscanf(file, "%i", &i) == 1) {
        val_rtrn->ival = i;
        return XkbCF_Integer;
    }
    return XkbCF_Unknown;
}

int
XkbCFScan(FILE *file, XkbCFScanResultPtr val_rtrn, XkbConfigRtrnPtr rtrn)
{
    int ch;

    do {
        ch = getc(file);
    } while ((ch == '\t') || (ch == ' '));
    if (isalpha(ch))
        return ScanIdent(file, ch, val_rtrn);
    else if (isdigit(ch))
        return ScanInteger(file, ch, val_rtrn);
    else if (ch == '"')
        return ScanString(file, ch, val_rtrn);
    else if (ch == '\n') {
        rtrn->line++;
        return XkbCF_EOL;
    }
    else if (ch == ';')
        return XkbCF_Semi;
    else if (ch == '=')
        return XkbCF_Equals;
    else if (ch == '+') {
        ch = getc(file);
        if (ch == '=')
            return XkbCF_PlusEquals;
        if ((ch != EOF) && (ch != ' ') && (ch != '\t'))
            ungetc(ch, file);
        return XkbCF_Plus;
    }
    else if (ch == '-') {
        ch = getc(file);
        if (ch == '=')
            return XkbCF_MinusEquals;
        if ((ch != EOF) && (ch != ' ') && (ch != '\t'))
            ungetc(ch, file);
        return XkbCF_Minus;
    }
    else if (ch == EOF)
        return XkbCF_EOF;
    else if ((ch == '#') || ((ch == '/') && (getc(file) == '/'))) {
        while ((ch != '\n') && (ch != EOF))
            ch = getc(file);
        rtrn->line++;
        return XkbCF_EOL;
    }
    return XkbCF_Unknown;
}

/***====================================================================***/

#define	_XkbCF_Illegal			0
#define	_XkbCF_Keymap		 	1
#define	_XkbCF_Keycodes		 	2
#define	_XkbCF_Geometry		 	3
#define	_XkbCF_PhysSymbols	 	4
#define _XkbCF_Symbols		 	5
#define	_XkbCF_Types		 	6
#define	_XkbCF_CompatMap	 	7

#define	_XkbCF_RulesFile		8
#define	_XkbCF_Model			9
#define	_XkbCF_Layout			10
#define	_XkbCF_Variant			11
#define	_XkbCF_Options			12

#define	_XkbCF_InitialMods	 	13
#define	_XkbCF_InitialCtrls	 	14

#define	_XkbCF_ClickVolume	 	15
#define	_XkbCF_BellVolume	 	16
#define	_XkbCF_BellPitch	 	17
#define	_XkbCF_BellDuration	 	18
#define	_XkbCF_RepeatDelay	 	19
#define	_XkbCF_RepeatInterval	 	20
#define	_XkbCF_SlowKeysDelay	 	21
#define	_XkbCF_DebounceDelay		22
#define	_XkbCF_MouseKeysDelay		23
#define	_XkbCF_MouseKeysInterval	24
#define	_XkbCF_MouseKeysTimeToMax	25
#define	_XkbCF_MouseKeysMaxSpeed	26
#define	_XkbCF_MouseKeysCurve		27
#define	_XkbCF_AccessXTimeout		28
#define	_XkbCF_AccessXTimeoutCtrlsOn	29
#define	_XkbCF_AccessXTimeoutCtrlsOff	30
#define	_XkbCF_AccessXTimeoutOptsOn	31
#define	_XkbCF_AccessXTimeoutOptsOff	32

#define	_XkbCF_IgnoreLockMods		33
#define	_XkbCF_IgnoreGroupLock		34
#define	_XkbCF_InternalMods		35

#define	_XkbCF_GroupsWrap		36
#define	_XkbCF_InitialFeedback		37

static Bool
AddCtrlByName(XkbConfigRtrnPtr rtrn, char *name, unsigned long *ctrls_rtrn)
{
    if ((_XkbStrCaseCmp(name, "repeat") == 0) ||
        (_XkbStrCaseCmp(name, "repeatkeys") == 0))
        *ctrls_rtrn = XkbRepeatKeysMask;
    else if (_XkbStrCaseCmp(name, "slowkeys") == 0)
        *ctrls_rtrn = XkbSlowKeysMask;
    else if (_XkbStrCaseCmp(name, "bouncekeys") == 0)
        *ctrls_rtrn = XkbBounceKeysMask;
    else if (_XkbStrCaseCmp(name, "stickykeys") == 0)
        *ctrls_rtrn = XkbStickyKeysMask;
    else if (_XkbStrCaseCmp(name, "mousekeys") == 0)
        *ctrls_rtrn = XkbMouseKeysMask;
    else if (_XkbStrCaseCmp(name, "mousekeysaccel") == 0)
        *ctrls_rtrn = XkbMouseKeysAccelMask;
    else if (_XkbStrCaseCmp(name, "accessxkeys") == 0)
        *ctrls_rtrn = XkbAccessXKeysMask;
    else if (_XkbStrCaseCmp(name, "accessxtimeout") == 0)
        *ctrls_rtrn = XkbAccessXTimeoutMask;
    else if (_XkbStrCaseCmp(name, "accessxfeedback") == 0)
        *ctrls_rtrn = XkbAccessXFeedbackMask;
    else if (_XkbStrCaseCmp(name, "audiblebell") == 0)
        *ctrls_rtrn = XkbAudibleBellMask;
    else if (_XkbStrCaseCmp(name, "overlay1") == 0)
        *ctrls_rtrn = XkbOverlay1Mask;
    else if (_XkbStrCaseCmp(name, "overlay2") == 0)
        *ctrls_rtrn = XkbOverlay2Mask;
    else if (_XkbStrCaseCmp(name, "ignoregrouplock") == 0)
        *ctrls_rtrn = XkbIgnoreGroupLockMask;
    else {
        rtrn->error = XkbCF_ExpectedControl;
        return False;
    }
    return True;
}

static Bool
AddAXTimeoutOptByName(XkbConfigRtrnPtr rtrn,
                      char *name,
                      unsigned short *opts_rtrn)
{
    if (_XkbStrCaseCmp(name, "slowkeyspress") == 0)
        *opts_rtrn = XkbAX_SKPressFBMask;
    else if (_XkbStrCaseCmp(name, "slowkeysaccept") == 0)
        *opts_rtrn = XkbAX_SKAcceptFBMask;
    else if (_XkbStrCaseCmp(name, "feature") == 0)
        *opts_rtrn = XkbAX_FeatureFBMask;
    else if (_XkbStrCaseCmp(name, "slowwarn") == 0)
        *opts_rtrn = XkbAX_SlowWarnFBMask;
    else if (_XkbStrCaseCmp(name, "indicator") == 0)
        *opts_rtrn = XkbAX_IndicatorFBMask;
    else if (_XkbStrCaseCmp(name, "stickykeys") == 0)
        *opts_rtrn = XkbAX_StickyKeysFBMask;
    else if (_XkbStrCaseCmp(name, "twokeys") == 0)
        *opts_rtrn = XkbAX_TwoKeysMask;
    else if (_XkbStrCaseCmp(name, "latchtolock") == 0)
        *opts_rtrn = XkbAX_LatchToLockMask;
    else if (_XkbStrCaseCmp(name, "slowkeysrelease") == 0)
        *opts_rtrn = XkbAX_SKReleaseFBMask;
    else if (_XkbStrCaseCmp(name, "slowkeysreject") == 0)
        *opts_rtrn = XkbAX_SKRejectFBMask;
    else if (_XkbStrCaseCmp(name, "bouncekeysreject") == 0)
        *opts_rtrn = XkbAX_BKRejectFBMask;
    else if (_XkbStrCaseCmp(name, "dumbbell") == 0)
        *opts_rtrn = XkbAX_DumbBellFBMask;
    else {
        rtrn->error = XkbCF_ExpectedControl;
        return False;
    }
    return True;
}

XkbConfigUnboundModPtr
XkbCFAddModByName(XkbConfigRtrnPtr rtrn, int what, char *name, Bool merge,
                  XkbConfigUnboundModPtr last)
{
    if (rtrn->num_unbound_mods >= rtrn->sz_unbound_mods) {
        rtrn->sz_unbound_mods += 5;
        rtrn->unbound_mods = _XkbTypedRealloc(rtrn->unbound_mods,
                                              rtrn->sz_unbound_mods,
                                              XkbConfigUnboundModRec);
        if (rtrn->unbound_mods == NULL) {
            rtrn->error = XkbCF_BadAlloc;
            return NULL;
        }
    }
    if (last == NULL) {
        last = &rtrn->unbound_mods[rtrn->num_unbound_mods++];
        last->what = what;
        last->mods = 0;
        last->vmods = 0;
        last->merge = merge;
        last->name = NULL;
    }
    if (_XkbStrCaseCmp(name, "shift") == 0)
        last->mods |= ShiftMask;
    else if (_XkbStrCaseCmp(name, "lock") == 0)
        last->mods |= LockMask;
    else if ((_XkbStrCaseCmp(name, "control") == 0) ||
             (_XkbStrCaseCmp(name, "ctrl") == 0))
        last->mods |= ControlMask;
    else if (_XkbStrCaseCmp(name, "mod1") == 0)
        last->mods |= Mod1Mask;
    else if (_XkbStrCaseCmp(name, "mod2") == 0)
        last->mods |= Mod2Mask;
    else if (_XkbStrCaseCmp(name, "mod3") == 0)
        last->mods |= Mod3Mask;
    else if (_XkbStrCaseCmp(name, "mod4") == 0)
        last->mods |= Mod4Mask;
    else if (_XkbStrCaseCmp(name, "mod5") == 0)
        last->mods |= Mod5Mask;
    else {
        if (last->name != NULL) {
            last = &rtrn->unbound_mods[rtrn->num_unbound_mods++];
            last->what = what;
            last->mods = 0;
            last->vmods = 0;
            last->merge = merge;
            last->name = NULL;
        }
        last->name = _XkbDupString(name);
    }
    return last;
}

int
XkbCFBindMods(XkbConfigRtrnPtr rtrn, XkbDescPtr xkb)
{
    register int n, v;
    Atom name;
    XkbConfigUnboundModPtr mod;
    int missing;

    if (rtrn->num_unbound_mods < 1)
        return 0;
    if ((xkb == NULL) || (xkb->names == NULL))
        return -1;

    missing = 0;
    for (n = 0, mod = rtrn->unbound_mods; n < rtrn->num_unbound_mods;
         n++, mod++) {
        if (mod->name != NULL) {
            name = XkbInternAtom(xkb->dpy, mod->name, True);
            if (name == None)
                continue;
            for (v = 0; v < XkbNumVirtualMods; v++) {
                if (xkb->names->vmods[v] == name) {
                    mod->vmods = (1 << v);
                    _XkbFree(mod->name);
                    mod->name = NULL;
                    break;
                }
            }
            if (mod->name != NULL)
                missing++;
        }
    }
    return missing;
}

Bool
XkbCFApplyMods(XkbConfigRtrnPtr rtrn, int what, XkbConfigModInfoPtr info)
{
    register int n;
    XkbConfigUnboundModPtr mod;

    if (rtrn->num_unbound_mods < 1)
        return True;

    for (n = 0, mod = rtrn->unbound_mods; n < rtrn->num_unbound_mods;
         n++, mod++) {
        if (mod->what != what)
            continue;
        if (mod->merge == XkbCF_MergeRemove) {
            info->mods_clear |= mod->mods;
            info->vmods_clear |= mod->vmods;
        }
        else {
            if (mod->merge == XkbCF_MergeSet)
                info->replace = True;
            info->mods |= mod->mods;
            info->vmods |= mod->vmods;
        }
        if (mod->name == NULL) {
            mod->what = _XkbCF_Illegal;
        }
        else {
            mod->mods = 0;
            mod->vmods = 0;
        }
    }
    return True;
}

/*ARGSUSED*/
static Bool
DefaultParser(FILE *             file,
              XkbConfigFieldsPtr fields,
              XkbConfigFieldPtr  field,
              XkbDescPtr         xkb,
              XkbConfigRtrnPtr   rtrn)
{
    int tok;
    XkbCFScanResultRec val;
    char **str;
    int merge;
    unsigned long *ctrls, ctrls_mask;
    unsigned short *opts, opts_mask;
    int *pival, sign;
    int onoff;
    XkbConfigUnboundModPtr last;
    unsigned what;

    tok = XkbCFScan(file, &val, rtrn);
    str = NULL;
    onoff = 0;
    pival = NULL;
    switch (field->field_id) {
    case _XkbCF_RulesFile:      str = &rtrn->rules_file; goto process_str;
    case _XkbCF_Model:          str = &rtrn->model; goto process_str;
    case _XkbCF_Layout:         str = &rtrn->layout; goto process_str;
    case _XkbCF_Variant:        str = &rtrn->variant; goto process_str;
    case _XkbCF_Options:        str = &rtrn->options; goto process_str;
    case _XkbCF_Keymap:         str = &rtrn->keymap; goto process_str;
    case _XkbCF_Keycodes:       str = &rtrn->keycodes; goto process_str;
    case _XkbCF_Geometry:       str = &rtrn->geometry; goto process_str;
    case _XkbCF_PhysSymbols:    str = &rtrn->phys_symbols; goto process_str;
    case _XkbCF_Symbols:        str = &rtrn->symbols; goto process_str;
    case _XkbCF_Types:          str = &rtrn->types; goto process_str;
    case _XkbCF_CompatMap:      str = &rtrn->compat; goto process_str;

    process_str:
        if (tok != XkbCF_Equals) {
            rtrn->error = XkbCF_MissingEquals;
            goto BAILOUT;
        }
        tok = XkbCFScan(file, &val, rtrn);
        if ((tok != XkbCF_String) && (tok != XkbCF_Ident)) {
            rtrn->error = XkbCF_ExpectedString;
            return False;
        }
        tok = XkbCFScan(file, &val, rtrn);
        if ((tok != XkbCF_EOL) && (tok != XkbCF_Semi) && (tok != XkbCF_EOF)) {
            rtrn->error = XkbCF_ExpectedEOS;
            return False;
        }
        if (*str != NULL)
            _XkbFree(*str);
        *str = _XkbDupString(val.str);
        break;
    case _XkbCF_InitialMods:
    case _XkbCF_IgnoreLockMods:
    case _XkbCF_InternalMods:
        what = XkbCF_InitialMods;
        if (field->field_id == _XkbCF_InitialMods)
            rtrn->defined |= (what = XkbCF_InitialMods);
        else if (field->field_id == _XkbCF_InternalMods)
            rtrn->defined |= (what = XkbCF_InternalMods);
        else if (field->field_id == _XkbCF_IgnoreLockMods)
            rtrn->defined |= (what = XkbCF_IgnoreLockMods);
        if (tok == XkbCF_Equals)
            merge = XkbCF_MergeSet;
        else if (tok == XkbCF_MinusEquals)
            merge = XkbCF_MergeRemove;
        else if (tok == XkbCF_PlusEquals)
            merge = XkbCF_MergeAdd;
        else {
            rtrn->error = XkbCF_MissingEquals;
            goto BAILOUT;
        }
        tok = XkbCFScan(file, &val, rtrn);
        if ((tok == XkbCF_EOL) || (tok == XkbCF_Semi) || (tok == XkbCF_EOF)) {
            rtrn->error = XkbCF_ExpectedModifier;
            return False;
        }
        last = NULL;
        while ((tok != XkbCF_EOL) && (tok != XkbCF_Semi) && (tok != XkbCF_EOF)) {
            if ((tok != XkbCF_Ident) && (tok != XkbCF_String)) {
                rtrn->error = XkbCF_ExpectedModifier;
                return False;
            }
            last = XkbCFAddModByName(rtrn, what, val.str, merge, last);
            if (last == NULL)
                return False;
            if (merge == XkbCF_MergeSet)
                merge = XkbCF_MergeAdd;
            tok = XkbCFScan(file, &val, rtrn);
            if ((tok != XkbCF_EOL) && (tok != XkbCF_EOF) && (tok != XkbCF_Semi)) {
                if (tok != XkbCF_Plus) {
                    rtrn->error = XkbCF_ExpectedOperator;
                    return False;
                }
                tok = XkbCFScan(file, &val, rtrn);
            }
        }
        break;
    case _XkbCF_InitialCtrls:
        rtrn->defined |= XkbCF_InitialCtrls;
        ctrls = NULL;
        if (tok == XkbCF_PlusEquals)
            ctrls = &rtrn->initial_ctrls;
        else if (tok == XkbCF_MinusEquals)
            ctrls = &rtrn->initial_ctrls_clear;
        else if (tok == XkbCF_Equals) {
            ctrls = &rtrn->initial_ctrls;
            rtrn->replace_initial_ctrls = True;
            *ctrls = 0;
        }
        else {
            rtrn->error = XkbCF_MissingEquals;
            goto BAILOUT;
        }
        tok = XkbCFScan(file, &val, rtrn);
        if ((tok == XkbCF_EOL) || (tok == XkbCF_Semi) || (tok == XkbCF_EOF)) {
            rtrn->error = XkbCF_ExpectedControl;
            return False;
        }
        while ((tok != XkbCF_EOL) && (tok != XkbCF_Semi) && (tok != XkbCF_EOF)) {
            if ((tok != XkbCF_Ident) && (tok != XkbCF_String)) {
                rtrn->error = XkbCF_ExpectedControl;
                return False;
            }
            if (!AddCtrlByName(rtrn, val.str, &ctrls_mask)) {
                return False;
            }
            *ctrls |= ctrls_mask;
            tok = XkbCFScan(file, &val, rtrn);
            if ((tok != XkbCF_EOL) && (tok != XkbCF_EOF) && (tok != XkbCF_Semi)) {
                if (tok != XkbCF_Plus) {
                    rtrn->error = XkbCF_ExpectedOperator;
                    return False;
                }
                tok = XkbCFScan(file, &val, rtrn);
            }
        }
        break;
    case _XkbCF_AccessXTimeoutCtrlsOn:
    case _XkbCF_AccessXTimeoutCtrlsOff:
        opts = NULL;
        if (tok == XkbCF_MinusEquals) {
            ctrls = &rtrn->axt_ctrls_ignore;
            opts = &rtrn->axt_opts_ignore;
        }
        else if ((tok == XkbCF_PlusEquals) || (tok == XkbCF_Equals)) {
            if (field->field_id == _XkbCF_AccessXTimeoutCtrlsOff) {
                ctrls = &rtrn->axt_ctrls_off;
                opts = &rtrn->axt_opts_off;
                if (tok == XkbCF_Equals)
                    rtrn->replace_axt_ctrls_off = True;
            }
            else {
                ctrls = &rtrn->axt_ctrls_on;
                opts = &rtrn->axt_opts_on;
                if (tok == XkbCF_Equals)
                    rtrn->replace_axt_ctrls_on = True;
            }
            *ctrls = 0;
        }
        else {
            rtrn->error = XkbCF_MissingEquals;
            goto BAILOUT;
        }
        tok = XkbCFScan(file, &val, rtrn);
        if ((tok == XkbCF_EOL) || (tok == XkbCF_Semi) || (tok == XkbCF_EOF)) {
            rtrn->error = XkbCF_ExpectedControl;
            return False;
        }
        while ((tok != XkbCF_EOL) && (tok != XkbCF_Semi) && (tok != XkbCF_EOF)) {
            if ((tok != XkbCF_Ident) && (tok != XkbCF_String)) {
                rtrn->error = XkbCF_ExpectedControl;
                return False;
            }
            if (!AddCtrlByName(rtrn, val.str, &ctrls_mask)) {
                if (!AddAXTimeoutOptByName(rtrn, val.str, &opts_mask))
                    return False;
                *opts |= opts_mask;
                if (field->field_id == _XkbCF_AccessXTimeoutCtrlsOff) {
                    rtrn->defined |= XkbCF_AccessXTimeoutOptsOff;
                    if (rtrn->replace_axt_ctrls_off)
                        rtrn->replace_axt_opts_off = True;
                }
                else {
                    rtrn->defined |= XkbCF_AccessXTimeoutOptsOn;
                    if (rtrn->replace_axt_ctrls_on)
                        rtrn->replace_axt_opts_on = True;
                }
            }
            else
                *ctrls |= ctrls_mask;
            tok = XkbCFScan(file, &val, rtrn);
            if ((tok != XkbCF_EOL) && (tok != XkbCF_EOF) && (tok != XkbCF_Semi)) {
                if (tok != XkbCF_Plus) {
                    rtrn->error = XkbCF_ExpectedOperator;
                    return False;
                }
                tok = XkbCFScan(file, &val, rtrn);
            }
        }
        break;
    case _XkbCF_InitialFeedback:
        rtrn->defined |= XkbCF_InitialOpts;
        opts = NULL;
        if (tok == XkbCF_PlusEquals)
            opts = &rtrn->initial_opts;
        else if (tok == XkbCF_MinusEquals)
            opts = &rtrn->initial_opts_clear;
        else if (tok == XkbCF_Equals) {
            opts = &rtrn->initial_opts;
            rtrn->replace_initial_opts = True;
            *opts = 0;
        }
        else {
            rtrn->error = XkbCF_MissingEquals;
            goto BAILOUT;
        }
        tok = XkbCFScan(file, &val, rtrn);
        if ((tok == XkbCF_EOL) || (tok == XkbCF_Semi) || (tok == XkbCF_EOF)) {
            rtrn->error = XkbCF_ExpectedAXOption;
            return False;
        }
        while ((tok != XkbCF_EOL) && (tok != XkbCF_Semi) && (tok != XkbCF_EOF)) {
            if ((tok != XkbCF_Ident) && (tok != XkbCF_String)) {
                rtrn->error = XkbCF_ExpectedAXOption;
                return False;
            }
            if (!AddAXTimeoutOptByName(rtrn, val.str, &opts_mask)) {
                return False;
            }
            *opts |= opts_mask;
            tok = XkbCFScan(file, &val, rtrn);
            if ((tok != XkbCF_EOL) && (tok != XkbCF_EOF) && (tok != XkbCF_Semi)) {
                if (tok != XkbCF_Plus) {
                    rtrn->error = XkbCF_ExpectedOperator;
                    return False;
                }
                tok = XkbCFScan(file, &val, rtrn);
            }
        }
        break;
    case _XkbCF_AccessXTimeoutOptsOff:
    case _XkbCF_AccessXTimeoutOptsOn:
        opts = NULL;
        if (tok == XkbCF_MinusEquals)
            opts = &rtrn->axt_opts_ignore;
        else if ((tok == XkbCF_PlusEquals) || (tok == XkbCF_Equals)) {
            if (field->field_id == _XkbCF_AccessXTimeoutOptsOff) {
                opts = &rtrn->axt_opts_off;
                if (tok == XkbCF_Equals)
                    rtrn->replace_axt_opts_off = True;
            }
            else {
                opts = &rtrn->axt_opts_on;
                if (tok == XkbCF_Equals)
                    rtrn->replace_axt_opts_on = True;
            }
            *opts = 0;
        }
        else {
            rtrn->error = XkbCF_MissingEquals;
            goto BAILOUT;
        }
        tok = XkbCFScan(file, &val, rtrn);
        if ((tok == XkbCF_EOL) || (tok == XkbCF_Semi) || (tok == XkbCF_EOF)) {
            rtrn->error = XkbCF_ExpectedControl;
            return False;
        }
        while ((tok != XkbCF_EOL) && (tok != XkbCF_Semi) && (tok != XkbCF_EOF)) {
            if ((tok != XkbCF_Ident) && (tok != XkbCF_String)) {
                rtrn->error = XkbCF_ExpectedControl;
                return False;
            }
            if (!AddAXTimeoutOptByName(rtrn, val.str, &opts_mask))
                return False;
            *opts |= opts_mask;

            tok = XkbCFScan(file, &val, rtrn);
            if ((tok != XkbCF_EOL) && (tok != XkbCF_EOF) && (tok != XkbCF_Semi)) {
                if (tok != XkbCF_Plus) {
                    rtrn->error = XkbCF_ExpectedOperator;
                    return False;
                }
                tok = XkbCFScan(file, &val, rtrn);
            }
        }
        break;
    case _XkbCF_ClickVolume:
        pival = &rtrn->click_volume;
        onoff = 100;
        goto pival;
    case _XkbCF_BellVolume:
        pival = &rtrn->bell_volume;
        onoff = 100;
        goto pival;
    case _XkbCF_BellPitch:          pival = &rtrn->bell_pitch; goto pival;
    case _XkbCF_BellDuration:       pival = &rtrn->bell_duration; goto pival;
    case _XkbCF_RepeatDelay:        pival = &rtrn->repeat_delay; goto pival;
    case _XkbCF_RepeatInterval:     pival = &rtrn->repeat_interval; goto pival;
    case _XkbCF_SlowKeysDelay:      pival = &rtrn->slow_keys_delay; goto pival;
    case _XkbCF_DebounceDelay:      pival = &rtrn->debounce_delay; goto pival;
    case _XkbCF_MouseKeysDelay:     pival = &rtrn->mk_delay; goto pival;
    case _XkbCF_MouseKeysInterval:  pival = &rtrn->mk_interval; goto pival;
    case _XkbCF_MouseKeysTimeToMax: pival = &rtrn->mk_time_to_max; goto pival;
    case _XkbCF_MouseKeysMaxSpeed:  pival = &rtrn->mk_max_speed; goto pival;
    case _XkbCF_MouseKeysCurve:     pival = &rtrn->mk_curve; goto pival;
    case _XkbCF_AccessXTimeout:     pival = &rtrn->ax_timeout; goto pival;

    pival:
        if (tok != XkbCF_Equals) {
            rtrn->error = XkbCF_MissingEquals;
            goto BAILOUT;
        }
        tok = XkbCFScan(file, &val, rtrn);
        if (tok == XkbCF_Minus && field->field_id == _XkbCF_MouseKeysCurve) {
            /* This can be a negative value */
            tok = XkbCFScan(file, &val, rtrn);
            sign = -1;
        }
        else
            sign = 1;
        if (tok != XkbCF_Integer) {
            Bool ok = False;

            if ((onoff) && (tok == XkbCF_Ident) && (val.str != NULL)) {
                if (_XkbStrCaseCmp(val.str, "on")) {
                    val.ival = onoff;
                    ok = True;
                }
                else if (_XkbStrCaseCmp(val.str, "off")) {
                    val.ival = 0;
                    ok = True;
                }
            }
            if (!ok) {
                rtrn->error = XkbCF_ExpectedInteger;
                goto BAILOUT;
            }
        }
        *pival = val.ival * sign;
        if (field->field_id == _XkbCF_AccessXTimeout)
            rtrn->defined |= XkbCF_AccessXTimeout;
        tok = XkbCFScan(file, &val, rtrn);
        if ((tok != XkbCF_EOL) && (tok != XkbCF_Semi) && (tok != XkbCF_EOF)) {
            rtrn->error = XkbCF_ExpectedEOS;
            return False;
        }
        break;
    case _XkbCF_GroupsWrap:
        if (tok != XkbCF_Equals) {
            rtrn->error = XkbCF_MissingEquals;
            goto BAILOUT;
        }
        tok = XkbCFScan(file, &val, rtrn);
        if (tok == XkbCF_Ident) {
            if (_XkbStrCaseCmp(val.str, "wrap") == 0) {
                rtrn->groups_wrap = XkbSetGroupInfo(0, XkbWrapIntoRange, 0);
            }
            else if (_XkbStrCaseCmp(val.str, "clamp") == 0) {
                rtrn->groups_wrap = XkbSetGroupInfo(0, XkbClampIntoRange, 0);
            }
            else {
                rtrn->error = XkbCF_ExpectedOORGroupBehavior;
                return False;
            }
        }
        else if ((tok == XkbCF_Integer) && (XkbIsLegalGroup(val.ival - 1))) {
            rtrn->groups_wrap = XkbSetGroupInfo(0, XkbRedirectIntoRange,
                                                val.ival - 1);
        }
        else {
            rtrn->error = XkbCF_ExpectedOORGroupBehavior;
            return False;
        }
        rtrn->defined |= XkbCF_GroupsWrap;
        tok = XkbCFScan(file, &val, rtrn);
        if ((tok != XkbCF_EOL) && (tok != XkbCF_Semi) && (tok != XkbCF_EOF)) {
            rtrn->error = XkbCF_ExpectedEOS;
            return False;
        }
        break;
    default:
        rtrn->error = XkbCF_ExpectedInteger;
        goto BAILOUT;

    }
    return True;
 BAILOUT:
    return False;
}

static Bool
DefaultCleanUp(XkbConfigRtrnPtr rtrn)
{
    if (rtrn->keymap)
        _XkbFree(rtrn->keymap);
    if (rtrn->keycodes)
        _XkbFree(rtrn->keycodes);
    if (rtrn->geometry)
        _XkbFree(rtrn->geometry);
    if (rtrn->phys_symbols)
        _XkbFree(rtrn->phys_symbols);
    if (rtrn->symbols)
        _XkbFree(rtrn->symbols);
    if (rtrn->types)
        _XkbFree(rtrn->types);
    if (rtrn->compat)
        _XkbFree(rtrn->compat);
    rtrn->keycodes = rtrn->geometry = NULL;
    rtrn->symbols = rtrn->phys_symbols = NULL;
    rtrn->types = rtrn->compat = NULL;
    if ((rtrn->unbound_mods != NULL) && (rtrn->num_unbound_mods > 0)) {
        register int i;

        for (i = 0; i < rtrn->num_unbound_mods; i++) {
            if (rtrn->unbound_mods[i].name != NULL) {
                _XkbFree(rtrn->unbound_mods[i].name);
                rtrn->unbound_mods[i].name = NULL;
            }
        }
        _XkbFree(rtrn->unbound_mods);
        rtrn->sz_unbound_mods = 0;
        rtrn->num_unbound_mods = 0;
        rtrn->unbound_mods = NULL;
    }
    return True;
}

static Bool
DefaultApplyNames(XkbConfigRtrnPtr rtrn, XkbDescPtr xkb)
{
    char *str;

    if (XkbAllocNames(xkb, XkbComponentNamesMask, 0, 0) != Success)
        return False;
    if ((str = rtrn->keycodes) != NULL) {
        xkb->names->keycodes = XkbInternAtom(xkb->dpy, str, False);
        _XkbFree(str);
        rtrn->keycodes = NULL;
    }
    if ((str = rtrn->geometry) != NULL) {
        xkb->names->geometry = XkbInternAtom(xkb->dpy, str, False);
        _XkbFree(str);
        rtrn->geometry = NULL;
    }
    if ((str = rtrn->symbols) != NULL) {
        xkb->names->symbols = XkbInternAtom(xkb->dpy, str, False);
        _XkbFree(str);
        rtrn->symbols = NULL;
    }
    if ((str = rtrn->phys_symbols) != NULL) {
        xkb->names->phys_symbols = XkbInternAtom(xkb->dpy, str, False);
        _XkbFree(str);
        rtrn->phys_symbols = NULL;
    }
    if ((str = rtrn->types) != NULL) {
        xkb->names->types = XkbInternAtom(xkb->dpy, str, False);
        _XkbFree(str);
        rtrn->types = NULL;
    }
    if ((str = rtrn->compat) != NULL) {
        xkb->names->compat = XkbInternAtom(xkb->dpy, str, False);
        _XkbFree(str);
        rtrn->compat = NULL;
    }
    return True;
}

static Bool
DefaultApplyControls(XkbConfigRtrnPtr rtrn, XkbDescPtr xkb)
{
    unsigned on, off;
    XkbControlsPtr ctrls;
    unsigned int mask;

    if (XkbAllocControls(xkb, XkbAllControlsMask) != Success)
        return False;
    ctrls = xkb->ctrls;
    if (rtrn->replace_initial_ctrls)
        ctrls->enabled_ctrls = rtrn->initial_ctrls;
    else
        ctrls->enabled_ctrls |= rtrn->initial_ctrls;
    ctrls->enabled_ctrls &= ~rtrn->initial_ctrls_clear;
    if (rtrn->internal_mods.replace) {
        ctrls->internal.real_mods = rtrn->internal_mods.mods;
        ctrls->internal.vmods = rtrn->internal_mods.vmods;
    }
    else {
        ctrls->internal.real_mods &= ~rtrn->internal_mods.mods_clear;
        ctrls->internal.vmods &= ~rtrn->internal_mods.vmods_clear;
        ctrls->internal.real_mods |= rtrn->internal_mods.mods;
        ctrls->internal.vmods |= rtrn->internal_mods.vmods;
    }
    mask = 0;
    (void) XkbVirtualModsToReal(xkb, ctrls->internal.vmods, &mask);
    ctrls->internal.mask = (ctrls->internal.real_mods | mask);

    if (rtrn->ignore_lock_mods.replace) {
        ctrls->ignore_lock.real_mods = rtrn->ignore_lock_mods.mods;
        ctrls->ignore_lock.vmods = rtrn->ignore_lock_mods.vmods;
    }
    else {
        ctrls->ignore_lock.real_mods &= ~rtrn->ignore_lock_mods.mods_clear;
        ctrls->ignore_lock.vmods &= ~rtrn->ignore_lock_mods.vmods_clear;
        ctrls->ignore_lock.real_mods |= rtrn->ignore_lock_mods.mods;
        ctrls->ignore_lock.vmods |= rtrn->ignore_lock_mods.vmods;
    }
    mask = 0;
    (void) XkbVirtualModsToReal(xkb, ctrls->ignore_lock.vmods, &mask);
    ctrls->ignore_lock.mask = (ctrls->ignore_lock.real_mods | mask);

    if (rtrn->repeat_delay > 0)
        ctrls->repeat_delay = rtrn->repeat_delay;
    if (rtrn->repeat_interval > 0)
        ctrls->repeat_interval = rtrn->repeat_interval;
    if (rtrn->slow_keys_delay > 0)
        ctrls->slow_keys_delay = rtrn->slow_keys_delay;
    if (rtrn->debounce_delay > 0)
        ctrls->debounce_delay = rtrn->debounce_delay;
    if (rtrn->mk_delay > 0)
        ctrls->mk_delay = rtrn->mk_delay;
    if (rtrn->mk_interval > 0)
        ctrls->mk_interval = rtrn->mk_interval;
    if (rtrn->mk_time_to_max > 0)
        ctrls->mk_time_to_max = rtrn->mk_time_to_max;
    if (rtrn->mk_max_speed > 0)
        ctrls->mk_max_speed = rtrn->mk_max_speed;
    if (rtrn->mk_curve > 0)
        ctrls->mk_curve = rtrn->mk_curve;
    if (rtrn->defined & XkbCF_AccessXTimeout && rtrn->ax_timeout > 0)
        ctrls->ax_timeout = rtrn->ax_timeout;

    /* any value set to both off and on is reset to ignore */
    if ((off = (rtrn->axt_ctrls_on & rtrn->axt_ctrls_off)) != 0)
        rtrn->axt_ctrls_ignore |= off;

    /* ignore takes priority over on and off */
    rtrn->axt_ctrls_on &= ~rtrn->axt_ctrls_ignore;
    rtrn->axt_ctrls_off &= ~rtrn->axt_ctrls_ignore;

    if (!rtrn->replace_axt_ctrls_off) {
        off = (ctrls->axt_ctrls_mask & (~ctrls->axt_ctrls_values));
        off &= ~rtrn->axt_ctrls_on;
        off |= rtrn->axt_ctrls_off;
    }
    else
        off = rtrn->axt_ctrls_off;
    if (!rtrn->replace_axt_ctrls_on) {
        on = (ctrls->axt_ctrls_mask & ctrls->axt_ctrls_values);
        on &= ~rtrn->axt_ctrls_off;
        on |= rtrn->axt_ctrls_on;
    }
    else
        on = rtrn->axt_ctrls_on;
    ctrls->axt_ctrls_mask = (on | off) & ~rtrn->axt_ctrls_ignore;
    ctrls->axt_ctrls_values = on & ~rtrn->axt_ctrls_ignore;

    /* any value set to both off and on is reset to ignore */
    if ((off = (rtrn->axt_opts_on & rtrn->axt_opts_off)) != 0)
        rtrn->axt_opts_ignore |= off;

    /* ignore takes priority over on and off */
    rtrn->axt_opts_on &= ~rtrn->axt_opts_ignore;
    rtrn->axt_opts_off &= ~rtrn->axt_opts_ignore;

    if (rtrn->replace_axt_opts_off) {
        off = (ctrls->axt_opts_mask & (~ctrls->axt_opts_values));
        off &= ~rtrn->axt_opts_on;
        off |= rtrn->axt_opts_off;
    }
    else
        off = rtrn->axt_opts_off;
    if (!rtrn->replace_axt_opts_on) {
        on = (ctrls->axt_opts_mask & ctrls->axt_opts_values);
        on &= ~rtrn->axt_opts_off;
        on |= rtrn->axt_opts_on;
    }
    else
        on = rtrn->axt_opts_on;
    ctrls->axt_opts_mask =
        (unsigned short) ((on | off) & ~rtrn->axt_ctrls_ignore);
    ctrls->axt_opts_values = (unsigned short) (on & ~rtrn->axt_ctrls_ignore);

    if (rtrn->defined & XkbCF_GroupsWrap) {
        int n;

        n = XkbNumGroups(ctrls->groups_wrap);
        rtrn->groups_wrap = XkbSetNumGroups(rtrn->groups_wrap, n);
        ctrls->groups_wrap = rtrn->groups_wrap;
    }
    return True;
}

/*ARGSUSED*/
static Bool
DefaultFinish(XkbConfigFieldsPtr fields, XkbDescPtr xkb,
              XkbConfigRtrnPtr rtrn, int what)
{
    if ((what == XkbCF_Destroy) || (what == XkbCF_CleanUp))
        return DefaultCleanUp(rtrn);
    if (what == XkbCF_Check) {
        if ((rtrn->symbols == NULL) && (rtrn->phys_symbols != NULL))
            rtrn->symbols = _XkbDupString(rtrn->phys_symbols);
    }
    if ((what == XkbCF_Apply) || (what == XkbCF_Check)) {
        if (xkb && xkb->names && (rtrn->num_unbound_mods > 0))
            XkbCFBindMods(rtrn, xkb);
        XkbCFApplyMods(rtrn, XkbCF_InitialMods, &rtrn->initial_mods);
        XkbCFApplyMods(rtrn, XkbCF_InternalMods, &rtrn->internal_mods);
        XkbCFApplyMods(rtrn, XkbCF_IgnoreLockMods, &rtrn->ignore_lock_mods);
    }
    if (what == XkbCF_Apply) {
        if (xkb != NULL) {
            DefaultApplyNames(rtrn, xkb);
            DefaultApplyControls(rtrn, xkb);
            XkbCFBindMods(rtrn, xkb);
        }
    }
    return True;
}

static XkbConfigFieldRec _XkbCFDfltFields[] = {
    {"rules",                   _XkbCF_RulesFile},
    {"model",                   _XkbCF_Model},
    {"layout",                  _XkbCF_Layout},
    {"variant",                 _XkbCF_Variant},
    {"options",                 _XkbCF_Options},
    {"keymap",                  _XkbCF_Keymap},
    {"keycodes",                _XkbCF_Keycodes},
    {"geometry",                _XkbCF_Geometry},
    {"realsymbols",             _XkbCF_PhysSymbols},
    {"actualsymbols",           _XkbCF_PhysSymbols},
    {"symbols",                 _XkbCF_Symbols},
    {"symbolstouse",            _XkbCF_Symbols},
    {"types",                   _XkbCF_Types},
    {"compat",                  _XkbCF_CompatMap},
    {"modifiers",               _XkbCF_InitialMods},
    {"controls",                _XkbCF_InitialCtrls},
    {"click",                   _XkbCF_ClickVolume},
    {"clickvolume",             _XkbCF_ClickVolume},
    {"bell",                    _XkbCF_BellVolume},
    {"bellvolume",              _XkbCF_BellVolume},
    {"bellpitch",               _XkbCF_BellPitch},
    {"bellduration",            _XkbCF_BellDuration},
    {"repeatdelay",             _XkbCF_RepeatDelay},
    {"repeatinterval",          _XkbCF_RepeatInterval},
    {"slowkeysdelay",           _XkbCF_SlowKeysDelay},
    {"debouncedelay",           _XkbCF_DebounceDelay},
    {"mousekeysdelay",          _XkbCF_MouseKeysDelay},
    {"mousekeysinterval",       _XkbCF_MouseKeysInterval},
    {"mousekeystimetomax",      _XkbCF_MouseKeysTimeToMax},
    {"mousekeysmaxspeed",       _XkbCF_MouseKeysMaxSpeed},
    {"mousekeyscurve",          _XkbCF_MouseKeysCurve},
    {"accessxtimeout",          _XkbCF_AccessXTimeout},
    {"axtimeout",               _XkbCF_AccessXTimeout},
    {"accessxtimeoutctrlson",   _XkbCF_AccessXTimeoutCtrlsOn},
    {"axtctrlson",              _XkbCF_AccessXTimeoutCtrlsOn},
    {"accessxtimeoutctrlsoff",  _XkbCF_AccessXTimeoutCtrlsOff},
    {"axtctrlsoff",             _XkbCF_AccessXTimeoutCtrlsOff},
    {"accessxtimeoutfeedbackon",_XkbCF_AccessXTimeoutOptsOn},
    {"axtfeedbackon",           _XkbCF_AccessXTimeoutOptsOn},
    {"accessxtimeoutfeedbackoff",_XkbCF_AccessXTimeoutOptsOff},
    {"axtfeedbackoff",          _XkbCF_AccessXTimeoutOptsOff},
    {"ignorelockmods",          _XkbCF_IgnoreLockMods},
    {"ignorelockmodifiers",     _XkbCF_IgnoreLockMods},
    {"ignoregrouplock",         _XkbCF_IgnoreGroupLock},
    {"internalmods",            _XkbCF_InternalMods},
    {"internalmodifiers",       _XkbCF_InternalMods},
    {"outofrangegroups",        _XkbCF_GroupsWrap},
    {"groups",                  _XkbCF_GroupsWrap},
    {"feedback",                _XkbCF_InitialFeedback},
};
#define	_XkbCFNumDfltFields (sizeof(_XkbCFDfltFields)/sizeof(XkbConfigFieldRec))

static XkbConfigFieldsRec _XkbCFDflts = {
    0,                          /* cfg_id */
    _XkbCFNumDfltFields,        /* num_fields */
    _XkbCFDfltFields,           /* fields */
    DefaultParser,              /* parser */
    DefaultFinish,              /* finish */
    NULL,                       /* priv */
    NULL                        /* next */
};

XkbConfigFieldsPtr XkbCFDflts = &_XkbCFDflts;

/***====================================================================***/

XkbConfigFieldsPtr
XkbCFDup(XkbConfigFieldsPtr fields)
{
    XkbConfigFieldsPtr pNew;

    pNew = _XkbTypedAlloc(XkbConfigFieldsRec);
    if (pNew != NULL) {
        memcpy(pNew, fields, sizeof(XkbConfigFieldsRec));
        if ((pNew->fields != NULL) && (pNew->num_fields > 0)) {
            pNew->fields = _XkbTypedCalloc(pNew->num_fields, XkbConfigFieldRec);
            if (pNew->fields) {
                memcpy(fields->fields, pNew->fields,
                       (pNew->num_fields * sizeof(XkbConfigFieldRec)));
            }
            else {
                _XkbFree(pNew);
                return NULL;
            }
        }
        else {
            pNew->num_fields = 0;
            pNew->fields = NULL;
        }
        pNew->next = NULL;
    }
    return pNew;
}

XkbConfigFieldsPtr
XkbCFFree(XkbConfigFieldsPtr fields, Bool all)
{
    XkbConfigFieldsPtr next;

    next = NULL;
    while (fields != NULL) {
        next = fields->next;
        if (fields != XkbCFDflts) {
            if (fields->fields) {
                _XkbFree(fields->fields);
                fields->fields = NULL;
                fields->num_fields = 0;
            }
            _XkbFree(fields);
        }
        fields = (all ? next : NULL);
    }
    return next;
}

Bool
XkbCFApplyRtrnValues(XkbConfigRtrnPtr rtrn,
                     XkbConfigFieldsPtr fields,
                     XkbDescPtr xkb)
{
    Bool ok;

    if ((fields == NULL) || (rtrn == NULL) || (xkb == NULL))
        return False;
    for (ok = True; fields != NULL; fields = fields->next) {
        if (fields->finish != NULL)
            ok = (*fields->finish) (fields, xkb, rtrn, XkbCF_Apply) && ok;
    }
    return ok;
}

XkbConfigRtrnPrivPtr
XkbCFAddPrivate(XkbConfigRtrnPtr rtrn, XkbConfigFieldsPtr fields, XPointer ptr)
{
    XkbConfigRtrnPrivPtr priv;

    if ((rtrn == NULL) || (fields == NULL))
        return NULL;
    priv = _XkbTypedAlloc(XkbConfigRtrnPrivRec);
    if (priv != NULL) {
        priv->cfg_id = fields->cfg_id;
        priv->priv = ptr;
        priv->next = rtrn->priv;
        rtrn->priv = priv;
    }
    return priv;
}

void
XkbCFFreeRtrn(XkbConfigRtrnPtr rtrn, XkbConfigFieldsPtr fields, XkbDescPtr xkb)
{
    XkbConfigRtrnPrivPtr tmp, next;

    if ((fields == NULL) || (rtrn == NULL))
        return;
    while (fields != NULL) {
        if (fields->finish != NULL)
            (*fields->finish) (fields, xkb, rtrn, XkbCF_Destroy);
        fields = fields->next;
    }
    for (tmp = rtrn->priv; tmp != NULL; tmp = next) {
        next = tmp->next;
        bzero((char *) tmp, sizeof(XkbConfigRtrnPrivRec));
        _XkbFree(tmp);
    }
    bzero((char *) rtrn, sizeof(XkbConfigRtrnRec));
    return;
}

Bool
XkbCFParse(FILE *file, XkbConfigFieldsPtr fields,
           XkbDescPtr xkb, XkbConfigRtrnPtr rtrn)
{
    int tok;
    XkbCFScanResultRec val;
    XkbConfigFieldsPtr tmp;

    if ((file == NULL) || (fields == NULL) || (rtrn == NULL))
        return False;
    for (tok = 0, tmp = fields; tmp != NULL; tmp = tmp->next, tok++) {
        fields->cfg_id = tok;
    }
    bzero((char *) rtrn, sizeof(XkbConfigRtrnRec));
    rtrn->line = 1;
    rtrn->click_volume = -1;
    rtrn->bell_volume = -1;
    while ((tok = XkbCFScan(file, &val, rtrn)) != XkbCF_EOF) {
        if (tok == XkbCF_Ident) {
            Bool done;

            for (tmp = fields, done = False; (tmp != NULL) && (!done);
                 tmp = tmp->next) {
                register int i;

                XkbConfigFieldPtr f;

                for (i = 0, f = tmp->fields; (i < tmp->num_fields) && (!done);
                     i++, f++) {
                    if (_XkbStrCaseCmp(val.str, f->field) != 0)
                        continue;
                    if ((*tmp->parser) (file, tmp, f, xkb, rtrn))
                        done = True;
                    else
                        goto BAILOUT;
                }
            }
        }
        else if ((tok != XkbCF_EOL) && (tok != XkbCF_Semi)) {
            rtrn->error = XkbCF_MissingIdent;
            goto BAILOUT;
        }
    }
    for (tmp = fields; tmp != NULL; tmp = tmp->next) {
        if ((tmp->finish) && (!(*tmp->finish) (tmp, xkb, rtrn, XkbCF_Check)))
            goto BAILOUT;
    }
    return True;
 BAILOUT:
    for (tmp = fields; tmp != NULL; tmp = tmp->next) {
        if (tmp->finish)
            (*tmp->finish) (tmp, xkb, rtrn, XkbCF_CleanUp);
    }
    return False;
}

/*ARGSUSED*/
void
XkbCFReportError(FILE *file, char *name, int error, int line)
{
    const char *msg;

    switch (error) {
    case XkbCF_BadAlloc:
        msg = "allocation failed";
        break;
    case XkbCF_UnterminatedString:
        msg = "unterminated string";
        break;
    case XkbCF_MissingIdent:
        msg = "expected identifier";
        break;
    case XkbCF_MissingEquals:
        msg = "expected '='";
        break;
    case XkbCF_ExpectedEOS:
        msg = "expected ';' or newline";
        break;
    case XkbCF_ExpectedBoolean:
        msg = "expected a boolean value";
        break;
    case XkbCF_ExpectedInteger:
        msg = "expected a numeric value";
        break;
    case XkbCF_ExpectedString:
        msg = "expected a string";
        break;
    case XkbCF_ExpectedModifier:
        msg = "expected a modifier name";
        break;
    case XkbCF_ExpectedControl:
        msg = "expected a control name";
        break;
    case XkbCF_ExpectedAXOption:
        msg = "expected an AccessX option";
        break;
    case XkbCF_ExpectedOperator:
        msg = "expected '+' or '-'";
        break;
    case XkbCF_ExpectedOORGroupBehavior:
        msg = "expected wrap, clamp or group number";
        break;
    default:
        msg = "unknown error";
        break;
    }
    fprintf(file, "%s on line %d", msg, line);
    if (name)
        fprintf(file, " of %s\n", name);
    else
        fprintf(file, "\n");
    return;
}
