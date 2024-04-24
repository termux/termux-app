/*
 *
 * Copyright (c) 1997  Metro Link Incorporated
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 *
 */
/*
 * Copyright (c) 1997-2003 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the copyright holder(s)
 * and author(s) shall not be used in advertising or otherwise to promote
 * the sale, use or other dealings in this Software without prior written
 * authorization from the copyright holder(s) and author(s).
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#ifndef _xf86_tokens_h
#define _xf86_tokens_h

/* Undefine symbols that some OSs might define */
#undef IOBASE

/*
 * Each token should have a unique value regardless of the section
 * it is used in.
 */

typedef enum {
    /* errno-style tokens */
    OBSOLETE_TOKEN = -5,
    EOF_TOKEN = -4,
    LOCK_TOKEN = -3,
    ERROR_TOKEN = -2,

    /* value type tokens */
    NUMBER = 1,
    STRING,

    /* Tokens that can appear in many sections */
    SECTION,
    SUBSECTION,
    ENDSECTION,
    ENDSUBSECTION,
    IDENTIFIER,
    VENDOR,
    DASH,
    COMMA,
    MATCHSEAT,
    OPTION,
    COMMENT,

    /* Frequency units */
    HRZ,
    KHZ,
    MHZ,

    /* File tokens */
    FONTPATH,
    MODULEPATH,
    LOGFILEPATH,
    XKBDIR,

    /* Server Flag tokens.  These are deprecated in favour of generic Options */
    DONTZAP,
    DONTZOOM,
    DISABLEVIDMODE,
    ALLOWNONLOCAL,
    DISABLEMODINDEV,
    MODINDEVALLOWNONLOCAL,
    ALLOWMOUSEOPENFAIL,
    BLANKTIME,
    STANDBYTIME,
    SUSPENDTIME,
    OFFTIME,
    DEFAULTLAYOUT,

    /* Monitor tokens */
    MODEL,
    MODELINE,
    DISPLAYSIZE,
    HORIZSYNC,
    VERTREFRESH,
    MODE,
    GAMMA,
    USEMODES,

    /* Modes tokens */
    /* no new ones */

    /* Mode tokens */
    DOTCLOCK,
    HTIMINGS,
    VTIMINGS,
    FLAGS,
    HSKEW,
    BCAST,
    VSCAN,
    ENDMODE,

    /* Screen tokens */
    OBSDRIVER,
    MDEVICE,
    GDEVICE,
    MONITOR,
    SCREENNO,
    DEFAULTDEPTH,
    DEFAULTBPP,
    DEFAULTFBBPP,

    /* VideoAdaptor tokens */
    VIDEOADAPTOR,

    /* Mode timing tokens */
    TT_INTERLACE,
    TT_PHSYNC,
    TT_NHSYNC,
    TT_PVSYNC,
    TT_NVSYNC,
    TT_CSYNC,
    TT_PCSYNC,
    TT_NCSYNC,
    TT_DBLSCAN,
    TT_HSKEW,
    TT_BCAST,
    TT_VSCAN,

    /* Module tokens */
    LOAD,
    LOAD_DRIVER,
    DISABLE,

    /* Device tokens */
    DRIVER,
    CHIPSET,
    CLOCKS,
    VIDEORAM,
    BOARD,
    IOBASE,
    RAMDAC,
    DACSPEED,
    BIOSBASE,
    MEMBASE,
    CLOCKCHIP,
    CHIPID,
    CHIPREV,
    CARD,
    BUSID,
    IRQ,

    /* Keyboard tokens */
    AUTOREPEAT,
    XLEDS,
    KPROTOCOL,
    XKBKEYMAP,
    XKBCOMPAT,
    XKBTYPES,
    XKBKEYCODES,
    XKBGEOMETRY,
    XKBSYMBOLS,
    XKBDISABLE,
    PANIX106,
    XKBRULES,
    XKBMODEL,
    XKBLAYOUT,
    XKBVARIANT,
    XKBOPTIONS,
    /* Obsolete keyboard tokens */
    SERVERNUM,
    LEFTALT,
    RIGHTALT,
    SCROLLLOCK_TOK,
    RIGHTCTL,
    /* arguments for the above obsolete tokens */
    CONF_KM_META,
    CONF_KM_COMPOSE,
    CONF_KM_MODESHIFT,
    CONF_KM_MODELOCK,
    CONF_KM_SCROLLLOCK,
    CONF_KM_CONTROL,

    /* Pointer tokens */
    EMULATE3,
    BAUDRATE,
    SAMPLERATE,
    PRESOLUTION,
    CLEARDTR,
    CLEARRTS,
    CHORDMIDDLE,
    PROTOCOL,
    PDEVICE,
    EM3TIMEOUT,
    DEVICE_NAME,
    ALWAYSCORE,
    PBUTTONS,
    ZAXISMAPPING,

    /* Pointer Z axis mapping tokens */
    XAXIS,
    YAXIS,

    /* Display tokens */
    MODES,
    VIEWPORT,
    VIRTUAL,
    VISUAL,
    BLACK_TOK,
    WHITE_TOK,
    DEPTH,
    BPP,
    WEIGHT,

    /* Layout Tokens */
    SCREEN,
    INACTIVE,
    INPUTDEVICE,

    /* Adjaceny Tokens */
    RIGHTOF,
    LEFTOF,
    ABOVE,
    BELOW,
    RELATIVE,
    ABSOLUTE,

    /* Vendor Tokens */
    VENDORNAME,

    /* DRI Tokens */
    GROUP,

    /* InputClass Tokens */
    MATCH_PRODUCT,
    MATCH_VENDOR,
    MATCH_DEVICE_PATH,
    MATCH_OS,
    MATCH_PNPID,
    MATCH_USBID,
    MATCH_DRIVER,
    MATCH_TAG,
    MATCH_LAYOUT,
    MATCH_IS_KEYBOARD,
    MATCH_IS_POINTER,
    MATCH_IS_JOYSTICK,
    MATCH_IS_TABLET,
    MATCH_IS_TABLET_PAD,
    MATCH_IS_TOUCHPAD,
    MATCH_IS_TOUCHSCREEN,

    NOMATCH_PRODUCT,
    NOMATCH_VENDOR,
    NOMATCH_DEVICE_PATH,
    NOMATCH_OS,
    NOMATCH_PNPID,
    NOMATCH_USBID,
    NOMATCH_DRIVER,
    NOMATCH_TAG,
    NOMATCH_LAYOUT,
} ParserTokens;

#endif                          /* _xf86_tokens_h */
