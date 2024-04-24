/*

Copyright 1985, 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

/* Beware, here be monsters (still under construction... - JG */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <X11/Xlibint.h>
#include <X11/Xutil.h>
#define XK_MISCELLANY
#define XK_LATIN1
#define XK_LATIN2
#define XK_LATIN3
#define XK_LATIN4
#define XK_LATIN8
#define XK_LATIN9
#define XK_CYRILLIC
#define XK_GREEK
#define XK_ARMENIAN
#define XK_CAUCASUS
#define XK_VIETNAMESE
#define XK_XKB_KEYS
#define XK_SINHALA
#include <X11/keysymdef.h>
#include <stdio.h>

#include "Xresource.h"
#include "Key.h"

#ifdef XKB
#include "XKBlib.h"
#include "XKBlibint.h"
#define	XKeycodeToKeysym	_XKeycodeToKeysym
#define	XKeysymToKeycode	_XKeysymToKeycode
#define	XLookupKeysym		_XLookupKeysym
#define	XRefreshKeyboardMapping	_XRefreshKeyboardMapping
#define	XLookupString		_XLookupString
/* XKBBind.c */
#else
#define	XkbKeysymToModifiers	_XKeysymToModifiers
#endif

#define AllMods (ShiftMask|LockMask|ControlMask| \
		 Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask)

static void
ComputeMaskFromKeytrans(
    Display *dpy,
    register struct _XKeytrans *p);

struct _XKeytrans {
	struct _XKeytrans *next;/* next on list */
	char *string;		/* string to return when the time comes */
	int len;		/* length of string (since NULL is legit)*/
	KeySym key;		/* keysym rebound */
	unsigned int state;	/* modifier state */
	KeySym *modifiers;	/* modifier keysyms you want */
	int mlen;		/* length of modifier list */
};

static KeySym
KeyCodetoKeySym(register Display *dpy, KeyCode keycode, int col)
{
    register int per = dpy->keysyms_per_keycode;
    register KeySym *syms;
    KeySym lsym, usym;

    if ((col < 0) || ((col >= per) && (col > 3)) ||
	((int)keycode < dpy->min_keycode) || ((int)keycode > dpy->max_keycode))
      return NoSymbol;

    syms = &dpy->keysyms[(keycode - dpy->min_keycode) * per];
    if (col < 4) {
	if (col > 1) {
	    while ((per > 2) && (syms[per - 1] == NoSymbol))
		per--;
	    if (per < 3)
		col -= 2;
	}
	if ((per <= (col|1)) || (syms[col|1] == NoSymbol)) {
	    XConvertCase(syms[col&~1], &lsym, &usym);
	    if (!(col & 1))
		return lsym;
	    else if (usym == lsym)
		return NoSymbol;
	    else
		return usym;
	}
    }
    return syms[col];
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
    if ((! dpy->keysyms) && (! _XKeyInitialize(dpy)))
	return NoSymbol;
    return KeyCodetoKeySym(dpy, kc, col);
}

KeyCode
XKeysymToKeycode(
    Display *dpy,
    KeySym ks)
{
    register int i, j;

    if ((! dpy->keysyms) && (! _XKeyInitialize(dpy)))
	return (KeyCode) 0;
    for (j = 0; j < dpy->keysyms_per_keycode; j++) {
	for (i = dpy->min_keycode; i <= dpy->max_keycode; i++) {
	    if (KeyCodetoKeySym(dpy, (KeyCode) i, j) == ks)
		return i;
	}
    }
    return 0;
}

KeySym
XLookupKeysym(
    register XKeyEvent *event,
    int col)
{
    if ((! event->display->keysyms) && (! _XKeyInitialize(event->display)))
	return NoSymbol;
    return KeyCodetoKeySym(event->display, event->keycode, col);
}

static void
ResetModMap(
    Display *dpy)
{
    register XModifierKeymap *map;
    register int i, j, n;
    KeySym sym;
    register struct _XKeytrans *p;

    map = dpy->modifiermap;
    /* If any Lock key contains Caps_Lock, then interpret as Caps_Lock,
     * else if any contains Shift_Lock, then interpret as Shift_Lock,
     * else ignore Lock altogether.
     */
    dpy->lock_meaning = NoSymbol;
    /* Lock modifiers are in the second row of the matrix */
    n = 2 * map->max_keypermod;
    for (i = map->max_keypermod; i < n; i++) {
	for (j = 0; j < dpy->keysyms_per_keycode; j++) {
	    sym = KeyCodetoKeySym(dpy, map->modifiermap[i], j);
	    if (sym == XK_Caps_Lock) {
		dpy->lock_meaning = XK_Caps_Lock;
		break;
	    } else if (sym == XK_Shift_Lock) {
		dpy->lock_meaning = XK_Shift_Lock;
	    }
	    else if (sym == XK_ISO_Lock) {
		dpy->lock_meaning = XK_Caps_Lock;
		break;
	    }
	}
    }
    /* Now find any Mod<n> modifier acting as the Group or Numlock modifier */
    dpy->mode_switch = 0;
    dpy->num_lock = 0;
    n *= 4;
    for (i = 3*map->max_keypermod; i < n; i++) {
	for (j = 0; j < dpy->keysyms_per_keycode; j++) {
	    sym = KeyCodetoKeySym(dpy, map->modifiermap[i], j);
	    if (sym == XK_Mode_switch)
		dpy->mode_switch |= 1 << (i / map->max_keypermod);
	    if (sym == XK_Num_Lock)
		dpy->num_lock |= 1 << (i / map->max_keypermod);
	}
    }
    for (p = dpy->key_bindings; p; p = p->next)
	ComputeMaskFromKeytrans(dpy, p);
}

static int
InitModMap(
    Display *dpy)
{
    register XModifierKeymap *map;

    if (! (map = XGetModifierMapping(dpy)))
	return 0;
    LockDisplay(dpy);
    if (dpy->modifiermap)
	XFreeModifiermap(dpy->modifiermap);
    dpy->modifiermap = map;
    dpy->free_funcs->modifiermap = XFreeModifiermap;
    if (dpy->keysyms)
	ResetModMap(dpy);
    UnlockDisplay(dpy);
    return 1;
}

int
XRefreshKeyboardMapping(register XMappingEvent *event)
{

    if(event->request == MappingKeyboard) {
	/* XXX should really only refresh what is necessary
	 * for now, make initialize test fail
	 */
	LockDisplay(event->display);
	if (event->display->keysyms) {
	     Xfree (event->display->keysyms);
	     event->display->keysyms = NULL;
	}
	UnlockDisplay(event->display);
    }
    if(event->request == MappingModifier) {
	LockDisplay(event->display);
	if (event->display->modifiermap) {
	    XFreeModifiermap(event->display->modifiermap);
	    event->display->modifiermap = NULL;
	}
	UnlockDisplay(event->display);
	/* go ahead and get it now, since initialize test may not fail */
	if (event->display->keysyms)
	    (void) InitModMap(event->display);
    }
    return 1;
}

int
_XKeyInitialize(
    Display *dpy)
{
    int per, n;
    KeySym *keysyms;

    /*
     * lets go get the keysyms from the server.
     */
    if (!dpy->keysyms) {
	n = dpy->max_keycode - dpy->min_keycode + 1;
	keysyms = XGetKeyboardMapping (dpy, (KeyCode) dpy->min_keycode,
				       n, &per);
	/* keysyms may be NULL */
	if (! keysyms) return 0;

	LockDisplay(dpy);

	Xfree (dpy->keysyms);
	dpy->keysyms = keysyms;
	dpy->keysyms_per_keycode = per;
	if (dpy->modifiermap)
	    ResetModMap(dpy);

	UnlockDisplay(dpy);
    }
    if (!dpy->modifiermap)
        return InitModMap(dpy);
    return 1;
}

static void
UCSConvertCase( register unsigned code,
                KeySym *lower,
                KeySym *upper )
{
    /* Case conversion for UCS, as in Unicode Data version 4.0.0. */
    /* NB: Only converts simple one-to-one mappings. */

    /* Tables are used where they take less space than     */
    /* the code to work out the mappings. Zero values mean */
    /* undefined code points.                              */

    static unsigned short const IPAExt_upper_mapping[] = { /* part only */
                            0x0181, 0x0186, 0x0255, 0x0189, 0x018A,
    0x0258, 0x018F, 0x025A, 0x0190, 0x025C, 0x025D, 0x025E, 0x025F,
    0x0193, 0x0261, 0x0262, 0x0194, 0x0264, 0x0265, 0x0266, 0x0267,
    0x0197, 0x0196, 0x026A, 0x026B, 0x026C, 0x026D, 0x026E, 0x019C,
    0x0270, 0x0271, 0x019D, 0x0273, 0x0274, 0x019F, 0x0276, 0x0277,
    0x0278, 0x0279, 0x027A, 0x027B, 0x027C, 0x027D, 0x027E, 0x027F,
    0x01A6, 0x0281, 0x0282, 0x01A9, 0x0284, 0x0285, 0x0286, 0x0287,
    0x01AE, 0x0289, 0x01B1, 0x01B2, 0x028C, 0x028D, 0x028E, 0x028F,
    0x0290, 0x0291, 0x01B7
    };

    static unsigned short const LatinExtB_upper_mapping[] = { /* first part only */
    0x0180, 0x0181, 0x0182, 0x0182, 0x0184, 0x0184, 0x0186, 0x0187,
    0x0187, 0x0189, 0x018A, 0x018B, 0x018B, 0x018D, 0x018E, 0x018F,
    0x0190, 0x0191, 0x0191, 0x0193, 0x0194, 0x01F6, 0x0196, 0x0197,
    0x0198, 0x0198, 0x019A, 0x019B, 0x019C, 0x019D, 0x0220, 0x019F,
    0x01A0, 0x01A0, 0x01A2, 0x01A2, 0x01A4, 0x01A4, 0x01A6, 0x01A7,
    0x01A7, 0x01A9, 0x01AA, 0x01AB, 0x01AC, 0x01AC, 0x01AE, 0x01AF,
    0x01AF, 0x01B1, 0x01B2, 0x01B3, 0x01B3, 0x01B5, 0x01B5, 0x01B7,
    0x01B8, 0x01B8, 0x01BA, 0x01BB, 0x01BC, 0x01BC, 0x01BE, 0x01F7,
    0x01C0, 0x01C1, 0x01C2, 0x01C3, 0x01C4, 0x01C4, 0x01C4, 0x01C7,
    0x01C7, 0x01C7, 0x01CA, 0x01CA, 0x01CA
    };

    static unsigned short const LatinExtB_lower_mapping[] = { /* first part only */
    0x0180, 0x0253, 0x0183, 0x0183, 0x0185, 0x0185, 0x0254, 0x0188,
    0x0188, 0x0256, 0x0257, 0x018C, 0x018C, 0x018D, 0x01DD, 0x0259,
    0x025B, 0x0192, 0x0192, 0x0260, 0x0263, 0x0195, 0x0269, 0x0268,
    0x0199, 0x0199, 0x019A, 0x019B, 0x026F, 0x0272, 0x019E, 0x0275,
    0x01A1, 0x01A1, 0x01A3, 0x01A3, 0x01A5, 0x01A5, 0x0280, 0x01A8,
    0x01A8, 0x0283, 0x01AA, 0x01AB, 0x01AD, 0x01AD, 0x0288, 0x01B0,
    0x01B0, 0x028A, 0x028B, 0x01B4, 0x01B4, 0x01B6, 0x01B6, 0x0292,
    0x01B9, 0x01B9, 0x01BA, 0x01BB, 0x01BD, 0x01BD, 0x01BE, 0x01BF,
    0x01C0, 0x01C1, 0x01C2, 0x01C3, 0x01C6, 0x01C6, 0x01C6, 0x01C9,
    0x01C9, 0x01C9, 0x01CC, 0x01CC, 0x01CC
    };

    static unsigned short const Greek_upper_mapping[] = { /* updated to UD 14.0 */
    0x0370, 0x0370, 0x0372, 0x0372, 0x0374, 0x0375, 0x0376, 0x0376,
    0x0000, 0x0000, 0x037A, 0x03FD, 0x03FE, 0x03FF, 0x037E, 0x037F,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0384, 0x0385, 0x0386, 0x0387,
    0x0388, 0x0389, 0x038A, 0x0000, 0x038C, 0x0000, 0x038E, 0x038F,
    0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
    0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
    0x03A0, 0x03A1, 0x0000, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7,
    0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x0386, 0x0388, 0x0389, 0x038A,
    0x03B0, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
    0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
    0x03A0, 0x03A1, 0x03A3, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7,
    0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x038C, 0x038E, 0x038F, 0x03CF,
    0x0392, 0x0398, 0x03D2, 0x03D3, 0x03D4, 0x03A6, 0x03A0, 0x03CF,
    0x03D8, 0x03D8, 0x03DA, 0x03DA, 0x03DC, 0x03DC, 0x03DE, 0x03DE,
    0x03E0, 0x03E0, 0x03E2, 0x03E2, 0x03E4, 0x03E4, 0x03E6, 0x03E6,
    0x03E8, 0x03E8, 0x03EA, 0x03EA, 0x03EC, 0x03EC, 0x03EE, 0x03EE,
    0x039A, 0x03A1, 0x03F9, 0x037F, 0x03F4, 0x0395, 0x03F6, 0x03F7,
    0x03F7, 0x03F9, 0x03FA, 0x03FA, 0x03FC, 0x03FD, 0x03FE, 0x03FF
    };

    static unsigned short const Greek_lower_mapping[] = { /* updated to UD 14.0 */
    0x0371, 0x0371, 0x0373, 0x0373, 0x0374, 0x0375, 0x0377, 0x0377,
    0x0000, 0x0000, 0x037A, 0x037B, 0x037C, 0x037D, 0x037E, 0x03F3,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0384, 0x0385, 0x03AC, 0x0387,
    0x03AD, 0x03AE, 0x03AF, 0x0000, 0x03CC, 0x0000, 0x03CD, 0x03CE,
    0x0390, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7,
    0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
    0x03C0, 0x03C1, 0x0000, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7,
    0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03AC, 0x03AD, 0x03AE, 0x03AF,
    0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7,
    0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
    0x03C0, 0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7,
    0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03CC, 0x03CD, 0x03CE, 0x03D7,
    0x03D0, 0x03D1, 0x03D2, 0x03D3, 0x03D4, 0x03D5, 0x03D6, 0x03D7,
    0x03D9, 0x03D9, 0x03DB, 0x03DB, 0x03DD, 0x03DD, 0x03DF, 0x03DF,
    0x03E1, 0x03E1, 0x03E3, 0x03E3, 0x03E5, 0x03E5, 0x03E7, 0x03E7,
    0x03E9, 0x03E9, 0x03EB, 0x03EB, 0x03ED, 0x03ED, 0x03EF, 0x03EF,
    0x03F0, 0x03F1, 0x03F2, 0x03F3, 0x03B8, 0x03F5, 0x03F6, 0x03F8,
    0x03F8, 0x03F2, 0x03FB, 0x03FB, 0x03FC, 0x037B, 0x037C, 0x037D
    };

    static unsigned short const GreekExt_lower_mapping[] = {
    0x1F00, 0x1F01, 0x1F02, 0x1F03, 0x1F04, 0x1F05, 0x1F06, 0x1F07,
    0x1F00, 0x1F01, 0x1F02, 0x1F03, 0x1F04, 0x1F05, 0x1F06, 0x1F07,
    0x1F10, 0x1F11, 0x1F12, 0x1F13, 0x1F14, 0x1F15, 0x0000, 0x0000,
    0x1F10, 0x1F11, 0x1F12, 0x1F13, 0x1F14, 0x1F15, 0x0000, 0x0000,
    0x1F20, 0x1F21, 0x1F22, 0x1F23, 0x1F24, 0x1F25, 0x1F26, 0x1F27,
    0x1F20, 0x1F21, 0x1F22, 0x1F23, 0x1F24, 0x1F25, 0x1F26, 0x1F27,
    0x1F30, 0x1F31, 0x1F32, 0x1F33, 0x1F34, 0x1F35, 0x1F36, 0x1F37,
    0x1F30, 0x1F31, 0x1F32, 0x1F33, 0x1F34, 0x1F35, 0x1F36, 0x1F37,
    0x1F40, 0x1F41, 0x1F42, 0x1F43, 0x1F44, 0x1F45, 0x0000, 0x0000,
    0x1F40, 0x1F41, 0x1F42, 0x1F43, 0x1F44, 0x1F45, 0x0000, 0x0000,
    0x1F50, 0x1F51, 0x1F52, 0x1F53, 0x1F54, 0x1F55, 0x1F56, 0x1F57,
    0x0000, 0x1F51, 0x0000, 0x1F53, 0x0000, 0x1F55, 0x0000, 0x1F57,
    0x1F60, 0x1F61, 0x1F62, 0x1F63, 0x1F64, 0x1F65, 0x1F66, 0x1F67,
    0x1F60, 0x1F61, 0x1F62, 0x1F63, 0x1F64, 0x1F65, 0x1F66, 0x1F67,
    0x1F70, 0x1F71, 0x1F72, 0x1F73, 0x1F74, 0x1F75, 0x1F76, 0x1F77,
    0x1F78, 0x1F79, 0x1F7A, 0x1F7B, 0x1F7C, 0x1F7D, 0x0000, 0x0000,
    0x1F80, 0x1F81, 0x1F82, 0x1F83, 0x1F84, 0x1F85, 0x1F86, 0x1F87,
    0x1F80, 0x1F81, 0x1F82, 0x1F83, 0x1F84, 0x1F85, 0x1F86, 0x1F87,
    0x1F90, 0x1F91, 0x1F92, 0x1F93, 0x1F94, 0x1F95, 0x1F96, 0x1F97,
    0x1F90, 0x1F91, 0x1F92, 0x1F93, 0x1F94, 0x1F95, 0x1F96, 0x1F97,
    0x1FA0, 0x1FA1, 0x1FA2, 0x1FA3, 0x1FA4, 0x1FA5, 0x1FA6, 0x1FA7,
    0x1FA0, 0x1FA1, 0x1FA2, 0x1FA3, 0x1FA4, 0x1FA5, 0x1FA6, 0x1FA7,
    0x1FB0, 0x1FB1, 0x1FB2, 0x1FB3, 0x1FB4, 0x0000, 0x1FB6, 0x1FB7,
    0x1FB0, 0x1FB1, 0x1F70, 0x1F71, 0x1FB3, 0x1FBD, 0x1FBE, 0x1FBF,
    0x1FC0, 0x1FC1, 0x1FC2, 0x1FC3, 0x1FC4, 0x0000, 0x1FC6, 0x1FC7,
    0x1F72, 0x1F73, 0x1F74, 0x1F75, 0x1FC3, 0x1FCD, 0x1FCE, 0x1FCF,
    0x1FD0, 0x1FD1, 0x1FD2, 0x1FD3, 0x0000, 0x0000, 0x1FD6, 0x1FD7,
    0x1FD0, 0x1FD1, 0x1F76, 0x1F77, 0x0000, 0x1FDD, 0x1FDE, 0x1FDF,
    0x1FE0, 0x1FE1, 0x1FE2, 0x1FE3, 0x1FE4, 0x1FE5, 0x1FE6, 0x1FE7,
    0x1FE0, 0x1FE1, 0x1F7A, 0x1F7B, 0x1FE5, 0x1FED, 0x1FEE, 0x1FEF,
    0x0000, 0x0000, 0x1FF2, 0x1FF3, 0x1FF4, 0x0000, 0x1FF6, 0x1FF7,
    0x1F78, 0x1F79, 0x1F7C, 0x1F7D, 0x1FF3, 0x1FFD, 0x1FFE, 0x0000
    };

    static unsigned short const GreekExt_upper_mapping[] = {
    0x1F08, 0x1F09, 0x1F0A, 0x1F0B, 0x1F0C, 0x1F0D, 0x1F0E, 0x1F0F,
    0x1F08, 0x1F09, 0x1F0A, 0x1F0B, 0x1F0C, 0x1F0D, 0x1F0E, 0x1F0F,
    0x1F18, 0x1F19, 0x1F1A, 0x1F1B, 0x1F1C, 0x1F1D, 0x0000, 0x0000,
    0x1F18, 0x1F19, 0x1F1A, 0x1F1B, 0x1F1C, 0x1F1D, 0x0000, 0x0000,
    0x1F28, 0x1F29, 0x1F2A, 0x1F2B, 0x1F2C, 0x1F2D, 0x1F2E, 0x1F2F,
    0x1F28, 0x1F29, 0x1F2A, 0x1F2B, 0x1F2C, 0x1F2D, 0x1F2E, 0x1F2F,
    0x1F38, 0x1F39, 0x1F3A, 0x1F3B, 0x1F3C, 0x1F3D, 0x1F3E, 0x1F3F,
    0x1F38, 0x1F39, 0x1F3A, 0x1F3B, 0x1F3C, 0x1F3D, 0x1F3E, 0x1F3F,
    0x1F48, 0x1F49, 0x1F4A, 0x1F4B, 0x1F4C, 0x1F4D, 0x0000, 0x0000,
    0x1F48, 0x1F49, 0x1F4A, 0x1F4B, 0x1F4C, 0x1F4D, 0x0000, 0x0000,
    0x1F50, 0x1F59, 0x1F52, 0x1F5B, 0x1F54, 0x1F5D, 0x1F56, 0x1F5F,
    0x0000, 0x1F59, 0x0000, 0x1F5B, 0x0000, 0x1F5D, 0x0000, 0x1F5F,
    0x1F68, 0x1F69, 0x1F6A, 0x1F6B, 0x1F6C, 0x1F6D, 0x1F6E, 0x1F6F,
    0x1F68, 0x1F69, 0x1F6A, 0x1F6B, 0x1F6C, 0x1F6D, 0x1F6E, 0x1F6F,
    0x1FBA, 0x1FBB, 0x1FC8, 0x1FC9, 0x1FCA, 0x1FCB, 0x1FDA, 0x1FDB,
    0x1FF8, 0x1FF9, 0x1FEA, 0x1FEB, 0x1FFA, 0x1FFB, 0x0000, 0x0000,
    0x1F88, 0x1F89, 0x1F8A, 0x1F8B, 0x1F8C, 0x1F8D, 0x1F8E, 0x1F8F,
    0x1F88, 0x1F89, 0x1F8A, 0x1F8B, 0x1F8C, 0x1F8D, 0x1F8E, 0x1F8F,
    0x1F98, 0x1F99, 0x1F9A, 0x1F9B, 0x1F9C, 0x1F9D, 0x1F9E, 0x1F9F,
    0x1F98, 0x1F99, 0x1F9A, 0x1F9B, 0x1F9C, 0x1F9D, 0x1F9E, 0x1F9F,
    0x1FA8, 0x1FA9, 0x1FAA, 0x1FAB, 0x1FAC, 0x1FAD, 0x1FAE, 0x1FAF,
    0x1FA8, 0x1FA9, 0x1FAA, 0x1FAB, 0x1FAC, 0x1FAD, 0x1FAE, 0x1FAF,
    0x1FB8, 0x1FB9, 0x1FB2, 0x1FBC, 0x1FB4, 0x0000, 0x1FB6, 0x1FB7,
    0x1FB8, 0x1FB9, 0x1FBA, 0x1FBB, 0x1FBC, 0x1FBD, 0x0399, 0x1FBF,
    0x1FC0, 0x1FC1, 0x1FC2, 0x1FCC, 0x1FC4, 0x0000, 0x1FC6, 0x1FC7,
    0x1FC8, 0x1FC9, 0x1FCA, 0x1FCB, 0x1FCC, 0x1FCD, 0x1FCE, 0x1FCF,
    0x1FD8, 0x1FD9, 0x1FD2, 0x1FD3, 0x0000, 0x0000, 0x1FD6, 0x1FD7,
    0x1FD8, 0x1FD9, 0x1FDA, 0x1FDB, 0x0000, 0x1FDD, 0x1FDE, 0x1FDF,
    0x1FE8, 0x1FE9, 0x1FE2, 0x1FE3, 0x1FE4, 0x1FEC, 0x1FE6, 0x1FE7,
    0x1FE8, 0x1FE9, 0x1FEA, 0x1FEB, 0x1FEC, 0x1FED, 0x1FEE, 0x1FEF,
    0x0000, 0x0000, 0x1FF2, 0x1FFC, 0x1FF4, 0x0000, 0x1FF6, 0x1FF7,
    0x1FF8, 0x1FF9, 0x1FFA, 0x1FFB, 0x1FFC, 0x1FFD, 0x1FFE, 0x0000
    };

    *lower = code;
    *upper = code;

    /* Basic Latin and Latin-1 Supplement, U+0000 to U+00FF */
    if (code <= 0x00ff) {
        if (code >= 0x0041 && code <= 0x005a)             /* A-Z */
            *lower += 0x20;
        else if (code >= 0x0061 && code <= 0x007a)        /* a-z */
            *upper -= 0x20;
        else if ( (code >= 0x00c0 && code <= 0x00d6) ||
	          (code >= 0x00d8 && code <= 0x00de) )
            *lower += 0x20;
        else if ( (code >= 0x00e0 && code <= 0x00f6) ||
	          (code >= 0x00f8 && code <= 0x00fe) )
            *upper -= 0x20;
        else if (code == 0x00ff)      /* y with diaeresis */
            *upper = 0x0178;
        else if (code == 0x00b5)      /* micro sign */
            *upper = 0x039c;
        else if (code == 0x00df)      /* ssharp */
            *upper = 0x1e9e;
	return;
    }

    /* Latin Extended-A, U+0100 to U+017F */
    if (code <= 0x017f) {
        if ( (code >= 0x0100 && code <= 0x012f) ||
             (code >= 0x0132 && code <= 0x0137) ||
             (code >= 0x014a && code <= 0x0177) ) {
            *upper = code & ~1;
            *lower = code | 1;
        }
        else if ( (code >= 0x0139 && code <= 0x0148) ||
                  (code >= 0x0179 && code <= 0x017e) ) {
            if (code & 1)
	        *lower += 1;
            else
	        *upper -= 1;
        }
        else if (code == 0x0130)
            *lower = 0x0069;
        else if (code == 0x0131)
            *upper = 0x0049;
        else if (code == 0x0178)
            *lower = 0x00ff;
        else if (code == 0x017f)
            *upper = 0x0053;
        return;
    }

    /* Latin Extended-B, U+0180 to U+024F */
    if (code <= 0x024f) {
        if (code >= 0x0180 && code <= 0x01cc) {
            *lower = LatinExtB_lower_mapping[code - 0x0180];
            *upper = LatinExtB_upper_mapping[code - 0x0180];
        }
        else if (code >= 0x01cd && code <= 0x01dc) {
	    if (code & 1)
	       *lower += 1;
	    else
	       *upper -= 1;
        }
        else if (code == 0x01dd)
            *upper = 0x018e;
        else if ( (code >= 0x01de && code <= 0x01ef) ||
                  (code >= 0x01f4 && code <= 0x01f5) ||
                  (code >= 0x01f8 && code <= 0x021f) ||
                  (code >= 0x0222 && code <= 0x0233) ) {
            *lower |= 1;
            *upper &= ~1;
        }
        else if (code == 0x01f1 || code == 0x01f2) {
            *lower = 0x01f3;
            *upper = 0x01f1;
        }
        else if (code == 0x01f3)
            *upper = 0x01f1;
        else if (code == 0x01f6)
            *lower = 0x0195;
        else if (code == 0x01f7)
            *lower = 0x01bf;
        else if (code == 0x0220)
            *lower = 0x019e;
        return;
    }

    /* IPA Extensions, U+0250 to U+02AF */
    if (code >= 0x0253 && code <= 0x0292) {
        *upper = IPAExt_upper_mapping[code - 0x0253];
        return;
    }

    /* Combining Diacritical Marks, U+0300 to U+036F */
    if (code == 0x0345) {
        *upper = 0x0399;
        return;
    }

    /* Greek and Coptic, U+0370 to U+03FF */
    if (code >= 0x0370 && code <= 0x03ff) {
        *lower = Greek_lower_mapping[code - 0x0370];
        *upper = Greek_upper_mapping[code - 0x0370];
        if (*upper == 0)
            *upper = code;
        if (*lower == 0)
            *lower = code;
        return;
    }

    /* Cyrillic and Cyrillic Supplementary, U+0400 to U+052F */
    if ( (code >= 0x0400 && code <= 0x052f) ) {
        if (code >= 0x0400 && code <= 0x040f)
            *lower += 0x50;
        else if (code >= 0x0410 && code <= 0x042f)
            *lower += 0x20;
        else if (code >= 0x0430 && code <= 0x044f)
            *upper -= 0x20;
        else if (code >= 0x0450 && code <= 0x045f)
            *upper -= 0x50;
        else if ( (code >= 0x0460 && code <= 0x0481) ||
                  (code >= 0x048a && code <= 0x04bf) ||
	          (code >= 0x04d0 && code <= 0x04f5) ||
	          (code >= 0x04f8 && code <= 0x04f9) ||
                  (code >= 0x0500 && code <= 0x050f) ) {
            *upper &= ~1;
            *lower |= 1;
        }
        else if (code >= 0x04c1 && code <= 0x04ce) {
	    if (code & 1)
	        *lower += 1;
	    else
	        *upper -= 1;
        }
        return;
    }

    /* Armenian, U+0530 to U+058F */
    if (code >= 0x0530 && code <= 0x058f) {
        if (code >= 0x0531 && code <= 0x0556)
            *lower += 0x30;
        else if (code >=0x0561 && code <= 0x0586)
            *upper -= 0x30;
        return;
    }

    /* Latin Extended Additional, U+1E00 to U+1EFF */
    if (code >= 0x1e00 && code <= 0x1eff) {
        if ( (code >= 0x1e00 && code <= 0x1e95) ||
             (code >= 0x1ea0 && code <= 0x1ef9) ) {
            *upper &= ~1;
            *lower |= 1;
        }
        else if (code == 0x1e9b)
            *upper = 0x1e60;
        else if (code == 0x1e9e)
            *lower = 0x00df;    /* ssharp */
        return;
    }

    /* Greek Extended, U+1F00 to U+1FFF */
    if (code >= 0x1f00 && code <= 0x1fff) {
        *lower = GreekExt_lower_mapping[code - 0x1f00];
        *upper = GreekExt_upper_mapping[code - 0x1f00];
        if (*upper == 0)
            *upper = code;
        if (*lower == 0)
            *lower = code;
        return;
    }

    /* Letterlike Symbols, U+2100 to U+214F */
    if (code >= 0x2100 && code <= 0x214f) {
        switch (code) {
        case 0x2126: *lower = 0x03c9; break;
        case 0x212a: *lower = 0x006b; break;
        case 0x212b: *lower = 0x00e5; break;
        }
    }
    /* Number Forms, U+2150 to U+218F */
    else if (code >= 0x2160 && code <= 0x216f)
        *lower += 0x10;
    else if (code >= 0x2170 && code <= 0x217f)
        *upper -= 0x10;
    /* Enclosed Alphanumerics, U+2460 to U+24FF */
    else if (code >= 0x24b6 && code <= 0x24cf)
        *lower += 0x1a;
    else if (code >= 0x24d0 && code <= 0x24e9)
        *upper -= 0x1a;
    /* Halfwidth and Fullwidth Forms, U+FF00 to U+FFEF */
    else if (code >= 0xff21 && code <= 0xff3a)
        *lower += 0x20;
    else if (code >= 0xff41 && code <= 0xff5a)
        *upper -= 0x20;
    /* Deseret, U+10400 to U+104FF */
    else if (code >= 0x10400 && code <= 0x10427)
        *lower += 0x28;
    else if (code >= 0x10428 && code <= 0x1044f)
        *upper -= 0x28;
}

void
XConvertCase(
    register KeySym sym,
    KeySym *lower,
    KeySym *upper)
{
    /* Latin 1 keysym */
    if (sym < 0x100) {
        UCSConvertCase(sym, lower, upper);
	return;
    }

    /* Unicode keysym */
    if ((sym & 0xff000000) == 0x01000000) {
        UCSConvertCase((sym & 0x00ffffff), lower, upper);
        *upper |= 0x01000000;
        *lower |= 0x01000000;
        return;
    }

    /* Legacy keysym */

    *lower = sym;
    *upper = sym;

    switch(sym >> 8) {
    case 1: /* Latin 2 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym == XK_Aogonek)
	    *lower = XK_aogonek;
	else if (sym >= XK_Lstroke && sym <= XK_Sacute)
	    *lower += (XK_lstroke - XK_Lstroke);
	else if (sym >= XK_Scaron && sym <= XK_Zacute)
	    *lower += (XK_scaron - XK_Scaron);
	else if (sym >= XK_Zcaron && sym <= XK_Zabovedot)
	    *lower += (XK_zcaron - XK_Zcaron);
	else if (sym == XK_aogonek)
	    *upper = XK_Aogonek;
	else if (sym >= XK_lstroke && sym <= XK_sacute)
	    *upper -= (XK_lstroke - XK_Lstroke);
	else if (sym >= XK_scaron && sym <= XK_zacute)
	    *upper -= (XK_scaron - XK_Scaron);
	else if (sym >= XK_zcaron && sym <= XK_zabovedot)
	    *upper -= (XK_zcaron - XK_Zcaron);
	else if (sym >= XK_Racute && sym <= XK_Tcedilla)
	    *lower += (XK_racute - XK_Racute);
	else if (sym >= XK_racute && sym <= XK_tcedilla)
	    *upper -= (XK_racute - XK_Racute);
	break;
    case 2: /* Latin 3 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Hstroke && sym <= XK_Hcircumflex)
	    *lower += (XK_hstroke - XK_Hstroke);
	else if (sym >= XK_Gbreve && sym <= XK_Jcircumflex)
	    *lower += (XK_gbreve - XK_Gbreve);
	else if (sym >= XK_hstroke && sym <= XK_hcircumflex)
	    *upper -= (XK_hstroke - XK_Hstroke);
	else if (sym >= XK_gbreve && sym <= XK_jcircumflex)
	    *upper -= (XK_gbreve - XK_Gbreve);
	else if (sym >= XK_Cabovedot && sym <= XK_Scircumflex)
	    *lower += (XK_cabovedot - XK_Cabovedot);
	else if (sym >= XK_cabovedot && sym <= XK_scircumflex)
	    *upper -= (XK_cabovedot - XK_Cabovedot);
	break;
    case 3: /* Latin 4 */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Rcedilla && sym <= XK_Tslash)
	    *lower += (XK_rcedilla - XK_Rcedilla);
	else if (sym >= XK_rcedilla && sym <= XK_tslash)
	    *upper -= (XK_rcedilla - XK_Rcedilla);
	else if (sym == XK_ENG)
	    *lower = XK_eng;
	else if (sym == XK_eng)
	    *upper = XK_ENG;
	else if (sym >= XK_Amacron && sym <= XK_Umacron)
	    *lower += (XK_amacron - XK_Amacron);
	else if (sym >= XK_amacron && sym <= XK_umacron)
	    *upper -= (XK_amacron - XK_Amacron);
	break;
    case 6: /* Cyrillic */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Serbian_DJE && sym <= XK_Serbian_DZE)
	    *lower -= (XK_Serbian_DJE - XK_Serbian_dje);
	else if (sym >= XK_Serbian_dje && sym <= XK_Serbian_dze)
	    *upper += (XK_Serbian_DJE - XK_Serbian_dje);
	else if (sym >= XK_Cyrillic_YU && sym <= XK_Cyrillic_HARDSIGN)
	    *lower -= (XK_Cyrillic_YU - XK_Cyrillic_yu);
	else if (sym >= XK_Cyrillic_yu && sym <= XK_Cyrillic_hardsign)
	    *upper += (XK_Cyrillic_YU - XK_Cyrillic_yu);
        break;
    case 7: /* Greek */
	/* Assume the KeySym is a legal value (ignore discontinuities) */
	if (sym >= XK_Greek_ALPHAaccent && sym <= XK_Greek_OMEGAaccent)
	    *lower += (XK_Greek_alphaaccent - XK_Greek_ALPHAaccent);
	else if (sym >= XK_Greek_alphaaccent && sym <= XK_Greek_omegaaccent &&
		 sym != XK_Greek_iotaaccentdieresis &&
		 sym != XK_Greek_upsilonaccentdieresis)
	    *upper -= (XK_Greek_alphaaccent - XK_Greek_ALPHAaccent);
	else if (sym >= XK_Greek_ALPHA && sym <= XK_Greek_OMEGA)
	    *lower += (XK_Greek_alpha - XK_Greek_ALPHA);
	else if (sym == XK_Greek_finalsmallsigma)
	    *upper = XK_Greek_SIGMA;
	else if (sym >= XK_Greek_alpha && sym <= XK_Greek_omega)
	    *upper -= (XK_Greek_alpha - XK_Greek_ALPHA);
        break;
    case 0x13: /* Latin 9 */
        if (sym == XK_OE)
            *lower = XK_oe;
        else if (sym == XK_oe)
            *upper = XK_OE;
        else if (sym == XK_Ydiaeresis)
            *lower = XK_ydiaeresis;
        break;
    }
}

int
_XTranslateKey(	register Display *dpy,
		KeyCode keycode,
		register unsigned int modifiers,
		unsigned int *modifiers_return,
		KeySym *keysym_return)
{
    int per;
    register KeySym *syms;
    KeySym sym, lsym, usym;

    if ((! dpy->keysyms) && (! _XKeyInitialize(dpy)))
	return 0;
    *modifiers_return = ((ShiftMask|LockMask)
			 | dpy->mode_switch | dpy->num_lock);
    if (((int)keycode < dpy->min_keycode) || ((int)keycode > dpy->max_keycode))
    {
	*keysym_return = NoSymbol;
	return 1;
    }
    per = dpy->keysyms_per_keycode;
    syms = &dpy->keysyms[(keycode - dpy->min_keycode) * per];
    while ((per > 2) && (syms[per - 1] == NoSymbol))
	per--;
    if ((per > 2) && (modifiers & dpy->mode_switch)) {
	syms += 2;
	per -= 2;
    }
    if ((modifiers & dpy->num_lock) &&
	(per > 1 && (IsKeypadKey(syms[1]) || IsPrivateKeypadKey(syms[1])))) {
	if ((modifiers & ShiftMask) ||
	    ((modifiers & LockMask) && (dpy->lock_meaning == XK_Shift_Lock)))
	    *keysym_return = syms[0];
	else
	    *keysym_return = syms[1];
    } else if (!(modifiers & ShiftMask) &&
	(!(modifiers & LockMask) || (dpy->lock_meaning == NoSymbol))) {
	if ((per == 1) || (syms[1] == NoSymbol))
	    XConvertCase(syms[0], keysym_return, &usym);
	else
	    *keysym_return = syms[0];
    } else if (!(modifiers & LockMask) ||
	       (dpy->lock_meaning != XK_Caps_Lock)) {
	if ((per == 1) || ((usym = syms[1]) == NoSymbol))
	    XConvertCase(syms[0], &lsym, &usym);
	*keysym_return = usym;
    } else {
	if ((per == 1) || ((sym = syms[1]) == NoSymbol))
	    sym = syms[0];
	XConvertCase(sym, &lsym, &usym);
	if (!(modifiers & ShiftMask) && (sym != syms[0]) &&
	    ((sym != usym) || (lsym == usym)))
	    XConvertCase(syms[0], &lsym, &usym);
	*keysym_return = usym;
    }
    if (*keysym_return == XK_VoidSymbol)
	*keysym_return = NoSymbol;
    return 1;
}

int
_XTranslateKeySym(
    Display *dpy,
    register KeySym symbol,
    unsigned int modifiers,
    char *buffer,
    int nbytes)
{
    register struct _XKeytrans *p;
    int length;
    unsigned long hiBytes;
    register unsigned char c;

    if (!symbol)
	return 0;
    /* see if symbol rebound, if so, return that string. */
    for (p = dpy->key_bindings; p; p = p->next) {
	if (((modifiers & AllMods) == p->state) && (symbol == p->key)) {
	    length = p->len;
	    if (length > nbytes) length = nbytes;
	    memcpy (buffer, p->string, (size_t) length);
	    return length;
	}
    }
    /* try to convert to Latin-1, handling control */
    hiBytes = symbol >> 8;
    if (!(nbytes &&
	  ((hiBytes == 0) ||
	   ((hiBytes == 0xFF) &&
	    (((symbol >= XK_BackSpace) && (symbol <= XK_Clear)) ||
	     (symbol == XK_Return) ||
	     (symbol == XK_Escape) ||
	     (symbol == XK_KP_Space) ||
	     (symbol == XK_KP_Tab) ||
	     (symbol == XK_KP_Enter) ||
	     ((symbol >= XK_KP_Multiply) && (symbol <= XK_KP_9)) ||
	     (symbol == XK_KP_Equal) ||
	     (symbol == XK_Delete))))))
	return 0;

    /* if X keysym, convert to ascii by grabbing low 7 bits */
    if (symbol == XK_KP_Space)
	c = XK_space & 0x7F; /* patch encoding botch */
    else if (hiBytes == 0xFF)
	c = symbol & 0x7F;
    else
	c = symbol & 0xFF;
    /* only apply Control key if it makes sense, else ignore it */
    if (modifiers & ControlMask) {
	if ((c >= '@' && c < '\177') || c == ' ') c &= 0x1F;
	else if (c == '2') c = '\000';
	else if (c >= '3' && c <= '7') c -= ('3' - '\033');
	else if (c == '8') c = '\177';
	else if (c == '/') c = '_' & 0x1F;
    }
    buffer[0] = c;
    return 1;
}

/*ARGSUSED*/
int
XLookupString (
    register XKeyEvent *event,
    char *buffer,	/* buffer */
    int nbytes,	/* space in buffer for characters */
    KeySym *keysym,
    XComposeStatus *status)	/* not implemented */
{
    unsigned int modifiers;
    KeySym symbol;

    if (! _XTranslateKey(event->display, event->keycode, event->state,
		  &modifiers, &symbol))
	return 0;

    if (keysym)
	*keysym = symbol;
    /* arguable whether to use (event->state & ~modifiers) here */
    return _XTranslateKeySym(event->display, symbol, event->state,
			     buffer, nbytes);
}

static void
_XFreeKeyBindings(
    Display *dpy)
{
    register struct _XKeytrans *p, *np;

    for (p = dpy->key_bindings; p; p = np) {
	np = p->next;
	Xfree(p->string);
	Xfree(p->modifiers);
	Xfree(p);
    }
    dpy->key_bindings = NULL;
}

int
XRebindKeysym (
    Display *dpy,
    KeySym keysym,
    KeySym *mlist,
    int nm,		/* number of modifiers in mlist */
    _Xconst unsigned char *str,
    int nbytes)
{
    register struct _XKeytrans *tmp, *p;
    int nb;

    if ((! dpy->keysyms) && (! _XKeyInitialize(dpy)))
	return 0;
    LockDisplay(dpy);
    tmp = dpy->key_bindings;
    nb = sizeof(KeySym) * nm;

    if ((! (p = Xcalloc( 1, sizeof(struct _XKeytrans)))) ||
	((! (p->string = Xmalloc(nbytes))) && (nbytes > 0)) ||
	((! (p->modifiers = Xmalloc(nb))) && (nb > 0))) {
	if (p) {
	    Xfree(p->string);
	    Xfree(p->modifiers);
	    Xfree(p);
	}
	UnlockDisplay(dpy);
	return 0;
    }

    dpy->key_bindings = p;
    dpy->free_funcs->key_bindings = _XFreeKeyBindings;
    p->next = tmp;	/* chain onto list */
    memcpy (p->string, str, (size_t) nbytes);
    p->len = nbytes;
    memcpy ((char *) p->modifiers, (char *) mlist, (size_t) nb);
    p->key = keysym;
    p->mlen = nm;
    ComputeMaskFromKeytrans(dpy, p);
    UnlockDisplay(dpy);
    return 0;
}

unsigned
_XKeysymToModifiers(
    Display *dpy,
    KeySym ks)
{
    CARD8 code,mods;
    register KeySym *kmax;
    register KeySym *k;
    register XModifierKeymap *m;

    if ((! dpy->keysyms) && (! _XKeyInitialize(dpy)))
	return 0;
    kmax = dpy->keysyms +
	   (dpy->max_keycode - dpy->min_keycode + 1) * dpy->keysyms_per_keycode;
    k = dpy->keysyms;
    m = dpy->modifiermap;
    mods= 0;
    while (k<kmax) {
	if (*k == ks ) {
	    register int j = m->max_keypermod<<3;

	    code=(((k-dpy->keysyms)/dpy->keysyms_per_keycode)+dpy->min_keycode);

	    while (--j >= 0) {
		if (code == m->modifiermap[j])
		    mods|= (1<<(j/m->max_keypermod));
	    }
	}
	k++;
    }
    return mods;
}

/*
 * given a list of modifiers, computes the mask necessary for later matching.
 * This routine must lookup the key in the Keymap and then search to see
 * what modifier it is bound to, if any.  Sets the AnyModifier bit if it
 * can't map some keysym to a modifier.
 */
static void
ComputeMaskFromKeytrans(
    Display *dpy,
    register struct _XKeytrans *p)
{
    register int i;

    p->state = AnyModifier;
    for (i = 0; i < p->mlen; i++) {
	p->state|= XkbKeysymToModifiers(dpy,p->modifiers[i]);
    }
    p->state &= AllMods;
}
