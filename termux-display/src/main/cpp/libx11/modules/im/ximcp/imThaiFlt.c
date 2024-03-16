/***********************************************************

Copyright 1993, 1998  The Open Group

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


Copyright 1993 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
**++
**  FACILITY:
**
**      Xlib
**
**  ABSTRACT:
**
**	Thai specific functions.
**	Handles character classifications, composibility checking,
**	Input sequence check and other Thai specific requirements
**	according to WTT specification and DEC extensions.
**
**  MODIFICATION HISTORY:
**
**/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"
#include "XimThai.h"
#include "XlcPubI.h"


#define SPACE   32

/* character classification table */
#define TACTIS_CHARS 256
static 
char const tactis_chtype[TACTIS_CHARS] = {
    CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL,  /*  0 -  7 */
    CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL,  /*  8 - 15 */
    CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL,  /* 16 - 23 */
    CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL,  /* 24 - 31 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 32 - 39 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 40 - 47 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 48 - 55 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 56 - 63 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 64 - 71 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 72 - 79 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 80 - 87 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 88 - 95 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 96 - 103 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 104 - 111 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 112 - 119 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  CTRL,  /* 120 - 127 */
    CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL,  /* 128 - 135 */
    CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL,  /* 136 - 143 */
    CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL,  /* 144 - 151 */
    CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL, CTRL,  /* 152 - 159 */
    NON,  CONS, CONS, CONS, CONS, CONS, CONS, CONS,  /* 160 - 167 */
    CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,  /* 168 - 175 */
    CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,  /* 176 - 183 */
    CONS, CONS, CONS, CONS, CONS, CONS, CONS, CONS,  /* 184 - 191 */
    CONS, CONS, CONS, CONS,  FV3, CONS,  FV3, CONS,  /* 192 - 199 */
    CONS, CONS, CONS, CONS, CONS, CONS, CONS, NON,   /* 200 - 207 */
    FV1,  AV2,  FV1,  FV1,  AV1,  AV3,  AV2,  AV3,   /* 208 - 215 */
    BV1,  BV2,  BD,   NON,  NON,  NON,  NON,  NON,   /* 216 - 223 */
    LV,   LV,   LV,   LV,   LV,   FV2,  NON,  AD2,   /* 224 - 231 */
    TONE, TONE, TONE, TONE, AD1,  AD1,  AD3,  NON,   /* 232 - 239 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  NON,   /* 240 - 247 */
    NON,  NON,  NON,  NON,  NON,  NON,  NON,  CTRL   /* 248 - 255 */
};

/* Composibility checking tables */
#define NC  0   /* NOT COMPOSIBLE - following char displays in next cell */
#define CP  1   /* COMPOSIBLE - following char is displayed in the same cell
                                as leading char, also implies ACCEPT */
#define XC  3   /* Non-display */
#define AC  4   /* ACCEPT - display the following char in the next cell */
#define RJ  5   /* REJECT - discard that following char, ignore it */

#define CH_CLASSES      17  /* 17 classes of chars */

static 
char const write_rules_lookup[CH_CLASSES][CH_CLASSES] = {
        /* Table 0: writing/outputting rules */
        /* row: leading char,  column: following char */
/* CTRL NON CONS LV FV1 FV2 FV3 BV1 BV2 BD TONE AD1 AD2 AD3 AV1 AV2 AV3 */
   {XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*CTRL*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*NON*/
  ,{XC, NC, NC, NC, NC, NC, NC, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP}/*CONS*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*LV*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*FV1*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*FV2*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*FV3*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, CP, CP, NC, NC, NC, NC, NC}/*BV1*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, CP, NC, NC, NC, NC, NC, NC}/*BV2*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*BD*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*TONE*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*AD1*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*AD2*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC, NC}/*AD3*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, CP, CP, NC, NC, NC, NC, NC}/*AV1*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, CP, NC, NC, NC, NC, NC, NC}/*AV2*/
  ,{XC, NC, NC, NC, NC, NC, NC, NC, NC, NC, CP, NC, CP, NC, NC, NC, NC}/*AV3*/
};

static 
char const wtt_isc1_lookup[CH_CLASSES][CH_CLASSES] = {
      /* Table 1: WTT default input sequence check rules */
      /* row: leading char,  column: following char */
/* CTRL NON CONS LV FV1 FV2 FV3 BV1 BV2 BD TONE AD1 AD2 AD3 AV1 AV2 AV3 */
   {XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*CTRL*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*NON*/
  ,{XC, AC, AC, AC, AC, AC, AC, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP}/*CONS*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*LV*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*FV1*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*FV2*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*FV3*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, CP, CP, RJ, RJ, RJ, RJ, RJ}/*BV1*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, CP, RJ, RJ, RJ, RJ, RJ, RJ}/*BV2*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*BD*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*TONE*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*AD1*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*AD2*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*AD3*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, CP, CP, RJ, RJ, RJ, RJ, RJ}/*AV1*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, CP, RJ, RJ, RJ, RJ, RJ, RJ}/*AV2*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, CP, RJ, CP, RJ, RJ, RJ, RJ}/*AV3*/
};

static 
char const wtt_isc2_lookup[CH_CLASSES][CH_CLASSES] = {
      /* Table 2: WTT strict input sequence check rules */
      /* row: leading char,  column: following char */
/* CTRL NON CONS LV FV1 FV2 FV3 BV1 BV2 BD TONE AD1 AD2 AD3 AV1 AV2 AV3 */
   {XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*CTRL*/
  ,{XC, AC, AC, AC, RJ, RJ, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*NON*/
  ,{XC, AC, AC, AC, AC, RJ, AC, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP}/*CONS*/
  ,{XC, RJ, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*LV*/
  ,{XC, AC, AC, AC, AC, RJ, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*FV1*/
  ,{XC, AC, AC, AC, AC, RJ, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*FV2*/
  ,{XC, AC, AC, AC, RJ, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*FV3*/
  ,{XC, AC, AC, AC, AC, RJ, AC, RJ, RJ, RJ, CP, CP, RJ, RJ, RJ, RJ, RJ}/*BV1*/
  ,{XC, AC, AC, AC, RJ, RJ, AC, RJ, RJ, RJ, CP, RJ, RJ, RJ, RJ, RJ, RJ}/*BV2*/
  ,{XC, AC, AC, AC, RJ, RJ, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*BD*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*TONE*/
  ,{XC, AC, AC, AC, RJ, RJ, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*AD1*/
  ,{XC, AC, AC, AC, RJ, RJ, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*AD2*/
  ,{XC, AC, AC, AC, RJ, RJ, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*AD3*/
  ,{XC, AC, AC, AC, RJ, RJ, AC, RJ, RJ, RJ, CP, CP, RJ, RJ, RJ, RJ, RJ}/*AV1*/
  ,{XC, AC, AC, AC, RJ, RJ, AC, RJ, RJ, RJ, CP, RJ, RJ, RJ, RJ, RJ, RJ}/*AV2*/
  ,{XC, AC, AC, AC, RJ, RJ, AC, RJ, RJ, RJ, CP, RJ, CP, RJ, RJ, RJ, RJ}/*AV3*/
};

static 
char const thaicat_isc_lookup[CH_CLASSES][CH_CLASSES] = {
      /* Table 3: Thaicat input sequence check rules */
      /* row: leading char,  column: following char */
/* CTRL NON CONS LV FV1 FV2 FV3 BV1 BV2 BD TONE AD1 AD2 AD3 AV1 AV2 AV3 */
   {XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*CTRL*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*NON*/
  ,{XC, AC, AC, AC, AC, AC, AC, CP, CP, CP, CP, CP, CP, CP, CP, CP, CP}/*CONS*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*LV*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*FV1*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*FV2*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ} /*FV3*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, CP, CP, RJ, RJ, RJ, RJ, RJ}/*BV1*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, CP, RJ, RJ, RJ, RJ, RJ, RJ}/*BV2*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*BD*/
  ,{XC, AC, AC, AC, AC, AC, AC, CP, CP, RJ, RJ, RJ, RJ, RJ, CP, CP, CP}/*TONE*/
  ,{XC, AC, AC, AC, AC, AC, AC, CP, RJ, RJ, RJ, RJ, RJ, RJ, CP, RJ, RJ}/*AD1*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, CP}/*AD2*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ, RJ}/*AD3*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, CP, CP, RJ, RJ, RJ, RJ, RJ}/*AV1*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, CP, RJ, RJ, RJ, RJ, RJ, RJ}/*AV2*/
  ,{XC, AC, AC, AC, AC, AC, AC, RJ, RJ, RJ, CP, RJ, CP, RJ, RJ, RJ, RJ}/*AV3*/
};


/* returns classification of a char */
static int
THAI_chtype (unsigned char	ch)
{
    return tactis_chtype[ch];
}

#ifdef UNUSED
/* returns the display level */
static int
THAI_chlevel (unsigned char	ch)
{
    int     chlevel;

    switch (tactis_chtype[ch])
    {
        case CTRL:
            chlevel = NON;
            break;
        case BV1:
        case BV2:
        case BD:
            chlevel = BELOW;
            break;
        case TONE:
        case AD1:
        case AD2:
            chlevel = TOP;
            break;
        case AV1:
        case AV2:
        case AV3:
        case AD3:
            chlevel = ABOVE;
            break;
        case NON:
        case CONS:
        case LV:
        case FV1:
        case FV2:
        case FV3:
        default: /* if tactis_chtype is invalid */
            chlevel = BASE;
            break;
    }
    return chlevel;
}


/* return True if char is non-spacing */
static Bool
THAI_isdead (unsigned char	ch)
{
    return ((tactis_chtype[ch] == CTRL) || (tactis_chtype[ch] == BV1) ||
            (tactis_chtype[ch] == BV2)  || (tactis_chtype[ch] == BD)  ||
            (tactis_chtype[ch] == TONE) || (tactis_chtype[ch] == AD1) ||
            (tactis_chtype[ch] == AD2)  || (tactis_chtype[ch] == AD3) ||
            (tactis_chtype[ch] == AV1)  || (tactis_chtype[ch] == AV2) ||
            (tactis_chtype[ch] == AV3));
}


/* return True if char is consonant */
static Bool
THAI_iscons (unsigned char	ch)
{
    return (tactis_chtype[ch] == CONS);
}


/* return True if char is vowel */
static Bool
THAI_isvowel (unsigned char	ch)
{
    return ((tactis_chtype[ch] == LV)  || (tactis_chtype[ch] == FV1) ||
            (tactis_chtype[ch] == FV2) || (tactis_chtype[ch] == FV3) ||
            (tactis_chtype[ch] == BV1) || (tactis_chtype[ch] == BV2) ||
            (tactis_chtype[ch] == AV1) || (tactis_chtype[ch] == AV2) ||
            (tactis_chtype[ch] == AV3));
}


/* return True if char is tonemark */
static Bool
THAI_istone (unsigned char	ch)
{
    return (tactis_chtype[ch] == TONE);
}
#endif

static Bool
THAI_iscomposible (
    unsigned char	follow_ch,
    unsigned char	lead_ch)
{/* "Can follow_ch be put in the same display cell as lead_ch?" */

    return (write_rules_lookup[THAI_chtype(lead_ch)][THAI_chtype(follow_ch)]
            == CP);
}

static Bool
THAI_isaccepted (
    unsigned char	follow_ch,
    unsigned char	lead_ch,
    unsigned char	mode)
{
    Bool iskeyvalid; /*  means "Can follow_ch be keyed in after lead_ch?" */

    switch (mode)
    {
        case WTT_ISC1:
            iskeyvalid =
          (wtt_isc1_lookup[THAI_chtype(lead_ch)][THAI_chtype(follow_ch)] != RJ);
            break;
        case WTT_ISC2:
            iskeyvalid =
          (wtt_isc2_lookup[THAI_chtype(lead_ch)][THAI_chtype(follow_ch)] != RJ);
            break;
        case THAICAT_ISC:
            iskeyvalid =
       (thaicat_isc_lookup[THAI_chtype(lead_ch)][THAI_chtype(follow_ch)] != RJ);
            break;
        default:
            iskeyvalid = True;
            break;
    }

    return iskeyvalid;
}

#ifdef UNUSED
static void
THAI_apply_write_rules(
    unsigned char	*instr,
    unsigned char	*outstr,
    unsigned char	insert_ch,
    int 		*num_insert_ch)
{
/*
Input parameters:
    instr - input string
    insert_ch specify what char to be added when invalid composition is found
Output parameters:
    outstr - output string after input string has been applied the rules
    num_insert_ch - number of insert_ch added to outstr.
*/
    unsigned char   *lead_ch = NULL, *follow_ch = NULL, *out_ch = NULL;

    *num_insert_ch = 0;
    lead_ch = follow_ch = instr;
    out_ch = outstr;
    if ((*lead_ch == '\0') || !(THAI_find_chtype(instr,DEAD)))
    {   /* Empty string or can't find any non-spacing char*/
        strcpy((char *)outstr, (char *)instr);
    } else { /* String of length >= 1, keep looking */
        follow_ch++;
        if (THAI_isdead(*lead_ch)) { /* is first char non-spacing? */
            *out_ch++ = SPACE;
            (*num_insert_ch)++;
        }
        *out_ch++ = *lead_ch;
        while (*follow_ch != '\0')  /* more char in string to check */
        {
            if (THAI_isdead(*follow_ch) &&
                 !THAI_iscomposible(*follow_ch,*lead_ch))
            {
                *out_ch++ = SPACE;
                (*num_insert_ch)++;
            }
            *out_ch++ = *follow_ch;
            lead_ch = follow_ch;
            follow_ch++;
        }
        *out_ch = '\0';
    }
}

static int
THAI_find_chtype (
    unsigned char	*instr,
    int		chtype)
{
/*
Input parameters:
    instr - input string
    chtype - type of character to look for
Output parameters:
    function returns first position of character with matched chtype
    function returns -1 if it does not find.
*/
    int i = 0, position = -1;

    switch (chtype)
    {
        case DEAD:
            for (i = 0; *instr != '\0' && THAI_isdead(*instr); i++, instr++)
		;
            if (*instr != '\0') position = i;
            break;
        default:
            break;
    }
    return position;
}


static int
THAI_apply_scm(
    unsigned char	*instr,
    unsigned char	*outstr,
    unsigned char	spec_ch,
    int		num_sp,
    unsigned char	insert_ch)
{
    unsigned char   *scan, *outch;
    int             i, dead_count, found_count;
    Bool            isconsecutive;

    scan = instr;
    outch = outstr;
    dead_count = found_count = 0;
    isconsecutive = False;
    while (*scan != '\0') {
        if (THAI_isdead(*scan))
            dead_count++;       /* count number of non-spacing char */
        if (*scan == spec_ch)
            if (!isconsecutive)
                found_count++;      /* count number consecutive spec char found */
        *outch++ = *scan++;
        if (found_count == num_sp) {
            for (i = 0; i < dead_count; i++)
                *outch++ = insert_ch;
            dead_count = found_count = 0;
        }
    }
    /* what to return? */
    return 0; /* probably not right but better than returning garbage */
}


/* The following functions are copied from XKeyBind.c */

static void ComputeMaskFromKeytrans();
static int IsCancelComposeKey(KeySym *symbol, XKeyEvent *event);
static void SetLed(Display *dpy, int num, int state);
static CARD8 FindKeyCode();


/* The following functions are specific to this module */

static int XThaiTranslateKey();
static int XThaiTranslateKeySym();


static KeySym HexIMNormalKey(
    XicThaiPart *thai_part,
    KeySym symbol,
    XKeyEvent *event);
static KeySym HexIMFirstComposeKey(
    XicThaiPart *thai_part,
    KeySym symbol,
    XKeyEvent *event);
static KeySym HexIMSecondComposeKey(
    XicThaiPart *thai_part,
    KeySym symbol
    XKeyEvent *event);
static KeySym HexIMComposeSequence(KeySym ks1, KeySym ks2);
static void InitIscMode(Xic ic);
static Bool ThaiComposeConvert(
    Display *dpy,
    KeySym insym,
    KeySym *outsym, KeySym *lower, KeySym *upper);
#endif

/*
 * Definitions
 */

#define BellVolume 		0

#define ucs2tis(wc)  \
 (unsigned char) ( \
   (0<=(wc)&&(wc)<=0x7F) ? \
     (wc) : \
     ((0x0E01<=(wc)&&(wc)<=0x0E5F) ? ((wc)-0x0E00+0xA0) : 0))
/* "c" is an unsigned char */
#define tis2ucs(c)  \
  ( \
   ((c)<=0x7F) ? \
     (wchar_t)(c) : \
     ((0x0A1<=(c)) ? ((wchar_t)(c)-0xA0+0x0E00) : 0))

/*
 * Macros to save and recall last input character in XIC
 */
#define IC_SavePreviousChar(ic,ch) \
                ((ic)->private.local.base.mb[(ic)->private.local.base.tree[(ic)->private.local.context].mb] = (char) (ch))
#define IC_ClearPreviousChar(ic) \
                ((ic)->private.local.base.mb[(ic)->private.local.base.tree[(ic)->private.local.context].mb] = 0)
#define IC_GetPreviousChar(ic) \
		(IC_RealGetPreviousChar(ic,1))
#define IC_GetContextChar(ic) \
		(IC_RealGetPreviousChar(ic,2))
#define IC_DeletePreviousChar(ic) \
		(IC_RealDeletePreviousChar(ic))

static unsigned char
IC_RealGetPreviousChar(Xic ic, unsigned short pos)
{
    XICCallback* cb = &ic->core.string_conversion_callback;
    DefTreeBase *b = &ic->private.local.base;

    if (cb && cb->callback) {
        XIMStringConversionCallbackStruct screc;
        unsigned char c;

        /* Use a safe value of position = 0 and stretch the range to desired
         * place, as XIM protocol is unclear here whether it could be negative
         */
        screc.position = 0;
        screc.direction = XIMBackwardChar;
        screc.operation = XIMStringConversionRetrieval;
        screc.factor = pos;
        screc.text = 0;

        (cb->callback)((XIC)ic, cb->client_data, (XPointer)&screc);
        if (!screc.text)
            return (unsigned char) b->mb[b->tree[(ic)->private.local.context].mb];
        if ((screc.text->feedback &&
             *screc.text->feedback == XIMStringConversionLeftEdge) ||
            screc.text->length < 1)
        {
            c = 0;
        } else {
            Xim     im;
            XlcConv conv;
            int     from_left;
            int     to_left;
            char   *from_buf;
            char   *to_buf;

            im = (Xim) XIMOfIC((XIC)ic);
            if (screc.text->encoding_is_wchar) {
                conv = _XlcOpenConverter(im->core.lcd, XlcNWideChar,
                                         im->core.lcd, XlcNCharSet);
                from_buf = (char *) screc.text->string.wcs;
                from_left = screc.text->length * sizeof(wchar_t);
            } else {
                conv = _XlcOpenConverter(im->core.lcd, XlcNMultiByte,
                                         im->core.lcd, XlcNCharSet);
                from_buf = screc.text->string.mbs;
                from_left = screc.text->length;
            }
            to_buf = (char *)&c;
            to_left = 1;

            _XlcResetConverter(conv);
            if (_XlcConvert(conv, (XPointer *)&from_buf, &from_left,
                            (XPointer *)&to_buf, &to_left, NULL, 0) < 0)
            {
                c = (unsigned char) b->mb[b->tree[(ic)->private.local.context].mb];
            }
            _XlcCloseConverter(conv);

            XFree(screc.text->string.mbs);
        }
        XFree(screc.text);
        return c;
    } else {
        return (unsigned char) b->mb[b->tree[(ic)->private.local.context].mb];
    }
}

static unsigned char
IC_RealDeletePreviousChar(Xic ic)
{
    XICCallback* cb = &ic->core.string_conversion_callback;

    if (cb && cb->callback) {
        XIMStringConversionCallbackStruct screc;
        unsigned char c;

        screc.position = 0;
        screc.direction = XIMBackwardChar;
        screc.operation = XIMStringConversionSubstitution;
        screc.factor = 1;
        screc.text = 0;

        (cb->callback)((XIC)ic, cb->client_data, (XPointer)&screc);
        if (!screc.text) { return 0; }
        if ((screc.text->feedback &&
             *screc.text->feedback == XIMStringConversionLeftEdge) ||
            screc.text->length < 1)
        {
            c = 0;
        } else {
            if (screc.text->encoding_is_wchar) {
                c = ucs2tis(screc.text->string.wcs[0]);
                XFree(screc.text->string.wcs);
            } else {
                c = screc.text->string.mbs[0];
                XFree(screc.text->string.mbs);
            }
        }
        XFree(screc.text);
        return c;
    } else {
        return 0;
    }
}
/*
 * Input sequence check mode in XIC
 */
#define IC_IscMode(ic)		((ic)->private.local.thai.input_mode)

/*
 * Max. size of string handled by the two String Lookup functions.
 */
#define STR_LKUP_BUF_SIZE	256

/*
 * Size of buffer to contain previous locale name.
 */
#define SAV_LOCALE_NAME_SIZE	256

/*
 * Size of buffer to contain the IM modifier.
 */
#define MAXTHAIIMMODLEN 20

#define AllMods (ShiftMask|LockMask|ControlMask| \
		 Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask)


#define IsISOControlKey(ks) ((ks) >= XK_2 && (ks) <= XK_8)

#define IsValidControlKey(ks)   (((((ks)>=XK_A && (ks)<=XK_asciitilde) || \
                (ks)==XK_space || (ks)==XK_Delete) && \
                ((ks)!=0)))

#define COMPOSE_LED 2

#ifdef UNUSED
typedef KeySym (*StateProc)(
    XicThaiPart *thai_part,
    KeySym symbol,
    XKeyEvent *event);


/*
 * macros to classify XKeyEvent state field
 */

#define IsShift(state) (((state) & ShiftMask) != 0)
#define IsLock(state) (((state) & LockMask) != 0)
#define IsControl(state) (((state) & ControlMask) != 0)
#define IsMod1(state) (((state) & Mod1Mask) != 0)
#define IsMod2(state) (((state) & Mod2Mask) != 0)
#define IsMod3(state) (((state) & Mod3Mask) != 0)
#define IsMod4(state) (((state) & Mod4Mask) != 0)
#define IsMod5(state) (((state) & Mod5Mask) != 0)

/*
 * key starts Thai compose sequence (Hex input method) if :
 */

#define IsComposeKey(ks, event)  \
	(( ks==XK_Alt_L && 	\
	   IsControl((event)->state) &&	\
	   !IsShift((event)->state))	\
	 ? True : False)


/*
 *  State handler to implement the Thai hex input method.
 */

static int const nstate_handlers = 3;
static StateProc state_handler[] = {
	HexIMNormalKey,
	HexIMFirstComposeKey,
	HexIMSecondComposeKey
};


/*
 *  Table for 'Thai Compose' character input.
 *  The current implementation uses latin-1 keysyms.
 */
struct _XMapThaiKey {
	KeySym from;
	KeySym to;
};

static struct _XMapThaiKey const ThaiComposeTable[] = {
	{ /* 0xa4 */ XK_currency,	/* 0xa5 */ XK_yen },
	{ /* 0xa2 */ XK_cent,		/* 0xa3 */ XK_sterling },
	{ /* 0xe6 */ XK_ae,		/* 0xef */ XK_idiaeresis },
	{ /* 0xd3 */ XK_Oacute,		/* 0xee */ XK_icircumflex },
	{ /* 0xb9 */ XK_onesuperior,	/* 0xfa */ XK_uacute },
	{ /* 0xd2 */ XK_Ograve,		/* 0xe5 */ XK_aring },
	{ /* 0xbc */ XK_onequarter,	/* 0xfb */ XK_ucircumflex },
	{	     XK_VoidSymbol,		   XK_VoidSymbol }
};

struct _XKeytrans {
	struct _XKeytrans *next;/* next on list */
	char *string;		/* string to return when the time comes */
	int len;		/* length of string (since NULL is legit)*/
	KeySym key;		/* keysym rebound */
	unsigned int state;	/* modifier state */
	KeySym *modifiers;	/* modifier keysyms you want */
	int mlen;		/* length of modifier list */
};


/* Convert keysym to 'Thai Compose' keysym */
/* The current implementation use latin-1 keysyms */
static Bool
ThaiComposeConvert(
    Display *dpy,
    KeySym insym,
    KeySym *outsym, KeySym *lower, KeySym *upper)
{
    struct _XMapThaiKey const *table_entry = ThaiComposeTable;

    while (table_entry->from != XK_VoidSymbol) {
	if (table_entry->from == insym) {
	    *outsym = table_entry->to;
	    *lower = *outsym;
	    *upper = *outsym;
	    return True;
	}
	table_entry++;
    }
    return False;
}

static int
XThaiTranslateKey(
    register Display *dpy,
    KeyCode keycode,
    register unsigned int modifiers,
    unsigned int *modifiers_return,
    KeySym *keysym_return,
    KeySym *lsym_return,
    KeySym *usym_return)
{
    int per;
    register KeySym *syms;
    KeySym sym = 0, lsym = 0, usym = 0;

    if ((! dpy->keysyms) && (! _XKeyInitialize(dpy)))
	return 0;
    *modifiers_return = (ShiftMask|LockMask) | dpy->mode_switch;
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
    if (!(modifiers & ShiftMask) &&
	(!(modifiers & LockMask) || (dpy->lock_meaning == NoSymbol))) {
	if ((per == 1) || (syms[1] == NoSymbol))
	    XConvertCase(syms[0], keysym_return, &usym);
	else {
	    XConvertCase(syms[0], &lsym, &usym);
	    *keysym_return = syms[0];
	}
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
    /*
     * ThaiCat keyboard support :
     * When the Shift and Thai keys are hold for some keys a 'Thai Compose'
     * character code is generated which is different from column 3 and
     * 4 of the keymap.
     * Since we don't know whether ThaiCat keyboard or WTT keyboard is
     * in use, the same mapping is done for all Thai input.
     * We just arbitrarily choose to use column 3 keysyms as the indices of
     * this mapping.
     * When the control key is also hold, this mapping has no effect.
     */
    if ((modifiers & Mod1Mask) &&
	(modifiers & ShiftMask) &&
	!(modifiers & ControlMask)) {
	if (ThaiComposeConvert(dpy, syms[0], &sym, &lsym, &usym))
	    *keysym_return = sym;
    }

    if (*keysym_return == XK_VoidSymbol)
	*keysym_return = NoSymbol;
    *lsym_return = lsym;
    *usym_return = usym;
    return 1;
}

/*
 * XThaiTranslateKeySym
 *
 * Translate KeySym to TACTIS code output.
 * The current implementation uses ISO latin-1 keysym.
 * Should be changed to TACTIS keysyms when they are defined by the
 * standard.
 */
static int
XThaiTranslateKeySym(
    Display *dpy,
    register KeySym symbol,
    register KeySym lsym,
    register KeySym usym,
    unsigned int modifiers,
    unsigned char *buffer,
    int nbytes)
{
    KeySym ckey = 0;
    register struct _XKeytrans *p;
    int length;
    unsigned long hiBytes;
    register unsigned char c;

    /*
     * initialize length = 1 ;
     */
    length = 1;

    if (!symbol)
	return 0;
    /* see if symbol rebound, if so, return that string. */
    for (p = dpy->key_bindings; p; p = p->next) {
	if (((modifiers & AllMods) == p->state) && (symbol == p->key)) {
	    length = p->len;
	    if (length > nbytes) length = nbytes;
	    memcpy (buffer, p->string, length);
	    return length;
	}
    }
    /* try to convert to TACTIS, handling control */
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
             (symbol == XK_Scroll_Lock) ||
#ifdef DXK_PRIVATE /* DEC private keysyms */
             (symbol == DXK_Remove) ||
#endif
             (symbol == NoSymbol) ||
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
    if (!(IsKeypadKey(lsym) || lsym==XK_Return || lsym==XK_Tab)) {
        if (IsISOControlKey(lsym)) ckey = lsym;
        else if (IsISOControlKey(usym)) ckey = usym;
        else if (lsym == XK_question) ckey = lsym;
        else if (usym == XK_question) ckey = usym;
        else if (IsValidControlKey(lsym)) ckey = lsym;
        else if (IsValidControlKey(usym)) ckey = usym;
        else length = 0;

        if (length != 0) {
        if (ckey == XK_2) c = '\000';
        else if (ckey >= XK_3 && ckey <= XK_7)
            c = (char)(ckey-('3'-'\033'));
        else if (ckey == XK_8) c = '\177';
        else if (ckey == XK_Delete) c = '\030';
        else if (ckey == XK_question) c = '\037';
        else if (ckey == XK_quoteleft) c = '\036';  /* KLee 1/24/91 */
        else c = (char)(ckey & 0x1f);
        }
    }
    }
    /*
     *  ThaiCat has a key that generates two TACTIS codes D1 & E9.
     *  It is represented by the latin-1 keysym XK_thorn (0xfe).
     *  If c is XK_thorn, this key is pressed and it is converted to
     *  0xd1 0xe9.
     */
    if (c == XK_thorn) {
	buffer[0] = 0xd1;
	buffer[1] = 0xe9;
	buffer[2] = '\0';
	return 2;
    }
    else {
	/* Normal case */
        buffer[0] = c;
	buffer[1] = '\0';
        return 1;
    }
}

/*
 * given a KeySym, returns the first keycode containing it, if any.
 */
static CARD8
FindKeyCode(
    register Display *dpy,
    register KeySym code)
{

    register KeySym *kmax = dpy->keysyms +
	(dpy->max_keycode - dpy->min_keycode + 1) * dpy->keysyms_per_keycode;
    register KeySym *k = dpy->keysyms;
    while (k < kmax) {
	if (*k == code)
	    return (((k - dpy->keysyms) / dpy->keysyms_per_keycode) +
		    dpy->min_keycode);
	k += 1;
	}
    return 0;
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
    register CARD8 code;
    register XModifierKeymap *m = dpy->modifiermap;

    p->state = AnyModifier;
    for (i = 0; i < p->mlen; i++) {
	/* if not found, then not on current keyboard */
	if ((code = FindKeyCode(dpy, p->modifiers[i])) == 0)
		return;
	/* code is now the keycode for the modifier you want */
	{
	    register int j = m->max_keypermod<<3;

	    while ((--j >= 0) && (code != m->modifiermap[j]))
		;
	    if (j < 0)
		return;
	    p->state |= (1<<(j/m->max_keypermod));
	}
    }
    p->state &= AllMods;
}

/************************************************************************
 *
 *
 * Compose handling routines - compose handlers 0,1,2
 *
 *
 ************************************************************************/

#define NORMAL_KEY_STATE 0
#define FIRST_COMPOSE_KEY_STATE 1
#define SECOND_COMPOSE_KEY_STATE 2

static 
KeySym HexIMNormalKey(
    XicThaiPart *thai_part,
    KeySym symbol,
    XKeyEvent *event)
{
    if (IsComposeKey (symbol, event))	/* start compose sequence	*/
	{
	SetLed (event->display,COMPOSE_LED, LedModeOn);
	thai_part->comp_state = FIRST_COMPOSE_KEY_STATE;
	return NoSymbol;
	}
    return symbol;
}


static 
KeySym HexIMFirstComposeKey(
    XicThaiPart *thai_part,
    KeySym symbol,
    XKeyEvent *event)
{
    if (IsModifierKey (symbol)) return symbol; /* ignore shift etc. */
    if (IsCancelComposeKey (&symbol, event))	/* cancel sequence */
	{
	SetLed (event->display,COMPOSE_LED, LedModeOff);
	thai_part->comp_state = NORMAL_KEY_STATE;
	return symbol;
	}
    if (IsComposeKey (symbol, event))		/* restart sequence ?? */
	{
	return NoSymbol;			/* no state change necessary */
	}

    thai_part->keysym = symbol;		/* save key pressed */
    thai_part->comp_state = SECOND_COMPOSE_KEY_STATE;
    return NoSymbol;
}

static 
KeySym HexIMSecondComposeKey(
    XicThaiPart *thai_part,
    KeySym symbol,
    XKeyEvent *event)
{
    if (IsModifierKey (symbol)) return symbol;	/* ignore shift etc. */
    if (IsComposeKey (symbol, event))		/* restart sequence ? */
	{
	thai_part->comp_state =FIRST_COMPOSE_KEY_STATE;
	return NoSymbol;
	}
    SetLed (event->display,COMPOSE_LED, LedModeOff);
    if (IsCancelComposeKey (&symbol, event))	/* cancel sequence ? */
	{
	thai_part->comp_state = NORMAL_KEY_STATE;
	return symbol;
	}

    if ((symbol = HexIMComposeSequence (thai_part->keysym, symbol))
								==NoSymbol)
	{ /* invalid compose sequence */
	XBell(event->display, BellVolume);
	}
    thai_part->comp_state = NORMAL_KEY_STATE; /* reset to normal state */
    return symbol;
}


/*
 * Interprets two keysyms entered as hex digits and return the Thai keysym
 * correspond to the TACTIS code formed.
 * The current implementation of this routine returns ISO Latin Keysyms.
 */

static 
KeySym HexIMComposeSequence(KeySym ks1, KeySym ks2)
{
int	hi_digit;
int	lo_digit;
int	tactis_code;

    if ((ks1 >= XK_0) && (ks1 <= XK_9))
	hi_digit = ks1 - XK_0;
    else if ((ks1 >= XK_A) && (ks1 <= XK_F))
	hi_digit = ks1 - XK_A + 10;
    else if ((ks1 >= XK_a) && (ks1 <= XK_f))
	hi_digit = ks1 - XK_a + 10;
    else	/* out of range */
	return NoSymbol;

    if ((ks2 >= XK_0) && (ks2 <= XK_9))
	lo_digit = ks2 - XK_0;
    else if ((ks2 >= XK_A) && (ks2 <= XK_F))
	lo_digit = ks2 - XK_A + 10;
    else if ((ks2 >= XK_a) && (ks2 <= XK_f))
	lo_digit = ks2 - XK_a + 10;
    else	/* out of range */
	return NoSymbol;

    tactis_code = hi_digit * 0x10 + lo_digit ;

    return (KeySym)tactis_code;

}

/*
 * routine determines
 *	1) whether key event should cancel a compose sequence
 *	2) whether cancelling key event should be processed or ignored
 */

static 
int IsCancelComposeKey(
    KeySym *symbol,
    XKeyEvent *event)
{
    if (*symbol==XK_Delete && !IsControl(event->state) &&
						!IsMod1(event->state)) {
	*symbol=NoSymbol;  /* cancel compose sequence, and ignore key */
	return True;
    }
    if (IsComposeKey(*symbol, event)) return False;
    return (
	IsControl (event->state) ||
	IsMod1(event->state) ||
	IsKeypadKey (*symbol) ||
	IsFunctionKey (*symbol) ||
	IsMiscFunctionKey (*symbol) ||
#ifdef DXK_PRIVATE /* DEC private keysyms */
	*symbol == DXK_Remove ||
#endif
	IsPFKey (*symbol) ||
	IsCursorKey (*symbol) ||
	(*symbol >= XK_Tab && *symbol < XK_Multi_key)
		? True : False);	/* cancel compose sequence and pass */
					/* cancelling key through	    */
}


/*
 *	set specified keyboard LED on or off
 */

static 
void SetLed(
    Display *dpy,
    int num,
    int state)
{
    XKeyboardControl led_control;

    led_control.led_mode = state;
    led_control.led = num;
    XChangeKeyboardControl (dpy, KBLed | KBLedMode,	&led_control);
}
#endif

/*
 * Initialize ISC mode from im modifier
 */
static void InitIscMode(Xic ic)
{
    Xim im;
    char *im_modifier_name;

    /* If already defined, just return */

    if (IC_IscMode(ic)) return;

    /* Get IM modifier */

    im = (Xim) XIMOfIC((XIC)ic);
    im_modifier_name = im->core.im_name;

    /* Match with predefined value, default is Basic Check */

    if (!strncmp(im_modifier_name,"BasicCheck",MAXTHAIIMMODLEN+1))
	IC_IscMode(ic) = WTT_ISC1;
    else if (!strncmp(im_modifier_name,"Strict",MAXTHAIIMMODLEN+1))
	IC_IscMode(ic) = WTT_ISC2;
    else if (!strncmp(im_modifier_name,"Thaicat",MAXTHAIIMMODLEN+1))
	IC_IscMode(ic) = THAICAT_ISC;
    else if (!strncmp(im_modifier_name,"Passthrough",MAXTHAIIMMODLEN+1))
	IC_IscMode(ic) = NOISC;
    else
	IC_IscMode(ic) = WTT_ISC1;

    return;
}

/*
 * Helper functions for _XimThaiFilter()
 */
static Bool
ThaiFltAcceptInput(Xic ic, unsigned char new_char, KeySym symbol)
{
    DefTreeBase *b = &ic->private.local.base;
    b->wc[b->tree[ic->private.local.composed].wc+0] = tis2ucs(new_char);
    b->wc[b->tree[ic->private.local.composed].wc+1] = '\0';

    if ((new_char <= 0x1f) || (new_char == 0x7f))
        b->tree[ic->private.local.composed].keysym = symbol;
    else
        b->tree[ic->private.local.composed].keysym = NoSymbol;

    return True;
}

static Bool
ThaiFltReorderInput(Xic ic, unsigned char previous_char, unsigned char new_char)
{
    DefTreeBase *b = &ic->private.local.base;
    if (!IC_DeletePreviousChar(ic)) return False;
    b->wc[b->tree[ic->private.local.composed].wc+0] = tis2ucs(new_char);
    b->wc[b->tree[ic->private.local.composed].wc+1] = tis2ucs(previous_char);
    b->wc[b->tree[ic->private.local.composed].wc+2] = '\0';

    b->tree[ic->private.local.composed].keysym = NoSymbol;

    return True;
}

static Bool
ThaiFltReplaceInput(Xic ic, unsigned char new_char, KeySym symbol)
{
    DefTreeBase *b = &ic->private.local.base;
    if (!IC_DeletePreviousChar(ic)) return False;
    b->wc[b->tree[ic->private.local.composed].wc+0] = tis2ucs(new_char);
    b->wc[b->tree[ic->private.local.composed].wc+1] = '\0';

    if ((new_char <= 0x1f) || (new_char == 0x7f))
        b->tree[ic->private.local.composed].keysym = symbol;
    else
        b->tree[ic->private.local.composed].keysym = NoSymbol;

    return True;
}

static unsigned
NumLockMask(Display *d)
{
    int i;
    XModifierKeymap *map;
    KeyCode numlock_keycode = XKeysymToKeycode (d, XK_Num_Lock);
    if (numlock_keycode == NoSymbol)
        return 0;

    map = XGetModifierMapping (d);
    if (!map)
        return 0;

    for (i = 0; i < 8; i++) {
        if (map->modifiermap[map->max_keypermod * i] == numlock_keycode) {
            XFreeModifiermap(map);
            return 1 << i;
        }
    }
    XFreeModifiermap(map);
    return 0;
}

/*
 * Filter function for TACTIS
 */
Bool
_XimThaiFilter(Display *d, Window w, XEvent *ev, XPointer client_data)
{
    Xic		    ic = (Xic)client_data;
    KeySym 	    symbol;
    int 	    isc_mode; /* Thai Input Sequence Check mode */
    unsigned char   previous_char; /* Last inputted Thai char */
    unsigned char   new_char;
#ifdef UNUSED
    unsigned int    modifiers;
    KeySym	    lsym,usym;
    int		    state;
    XicThaiPart     *thai_part;
    char	    buf[10];
#endif
    wchar_t	    wbuf[10];
    Bool            isReject;
    DefTreeBase    *b = &ic->private.local.base;

    if ((ev->type != KeyPress)
        || (ev->xkey.keycode == 0))
        return False;

    if (!IC_IscMode(ic)) InitIscMode(ic);

    XwcLookupString((XIC)ic, &ev->xkey, wbuf, sizeof(wbuf) / sizeof(wbuf[0]),
		    &symbol, NULL);

    if ((ev->xkey.state & (AllMods & ~(ShiftMask|LockMask|NumLockMask(d)))) ||
         ((symbol >> 8 == 0xFF) &&
         ((XK_BackSpace <= symbol && symbol <= XK_Clear) ||
           (symbol == XK_Return) ||
           (symbol == XK_Pause) ||
           (symbol == XK_Scroll_Lock) ||
           (symbol == XK_Sys_Req) ||
           (symbol == XK_Escape) ||
           (symbol == XK_Delete) ||
           IsCursorKey(symbol) ||
           IsKeypadKey(symbol) ||
           IsMiscFunctionKey(symbol) ||
           IsFunctionKey(symbol))))
        {
            IC_ClearPreviousChar(ic);
            return False;
        }
    if (((symbol >> 8 == 0xFF) &&
         IsModifierKey(symbol)) ||
#ifdef XK_XKB_KEYS
        ((symbol >> 8 == 0xFE) &&
         (XK_ISO_Lock <= symbol && symbol <= XK_ISO_Last_Group_Lock)) ||
#endif
        (symbol == NoSymbol))
    {
        return False;
    }
#ifdef UNUSED
    if (! XThaiTranslateKey(ev->xkey.display, ev->xkey.keycode, ev->xkey.state,
	 		&modifiers, &symbol, &lsym, &usym))
	return False;

    /*
     *  Hex input method processing
     */

    thai_part = &ic->private.local.thai;
    state = thai_part->comp_state;
    if (state >= 0 && state < nstate_handlers) /* call handler for state */
    {
        symbol = (* state_handler[state])(thai_part, symbol, (XKeyEvent *)ev);
    }

    /*
     *  Translate KeySym into mb.
     */
    count = XThaiTranslateKeySym(ev->xkey.display, symbol, lsym,
				usym, ev->xkey.state, buf, 10);

    if (!symbol && !count)
	return True;

    /* Return symbol if cannot convert to character */
    if (!count)
	return False;
#endif

    /*
     *  Thai Input sequence check
     */
    isc_mode = IC_IscMode(ic);
    if (!(previous_char = IC_GetPreviousChar(ic))) previous_char = ' ';
    new_char = ucs2tis(wbuf[0]);
    isReject = True;
    if (THAI_isaccepted(new_char, previous_char, isc_mode)) {
        ThaiFltAcceptInput(ic, new_char, symbol);
        isReject = False;
    } else {
        unsigned char context_char;

        context_char = IC_GetContextChar(ic);
        if (context_char) {
            if (THAI_iscomposible(new_char, context_char)) {
                if (THAI_iscomposible(previous_char, new_char)) {
                    isReject = !ThaiFltReorderInput(ic, previous_char, new_char);
                } else if (THAI_iscomposible(previous_char, context_char)) {
                    isReject = !ThaiFltReplaceInput(ic, new_char, symbol);
                } else if (THAI_chtype(previous_char) == FV1
                           && THAI_chtype(new_char) == TONE) {
                    isReject = !ThaiFltReorderInput(ic, previous_char, new_char);
                }
            } else if (THAI_isaccepted(new_char, context_char, isc_mode)) {
                isReject = !ThaiFltReplaceInput(ic, new_char, symbol);
            }
        }
    }
    if (isReject) {
        /* reject character */
        XBell(ev->xkey.display, BellVolume);
        return True;
    }

    _Xlcwcstombs(ic->core.im->core.lcd, &b->mb[b->tree[ic->private.local.composed].mb],
		 &b->wc[b->tree[ic->private.local.composed].wc], 10);

    _Xlcmbstoutf8(ic->core.im->core.lcd, &b->utf8[b->tree[ic->private.local.composed].utf8],
		  &b->mb[b->tree[ic->private.local.composed].mb], 10);

    /* Remember the last character inputted
     * (as fallback in case StringConversionCallback is not provided)
     */
    IC_SavePreviousChar(ic, new_char);

    ev->xkey.keycode = 0;
    XPutBackEvent(d, ev);
    return True;
}
