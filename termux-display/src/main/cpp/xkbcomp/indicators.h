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

#ifndef INDICATORS_H
#define INDICATORS_H 1

#define	_LED_Index	(1<<0)
#define	_LED_Mods	(1<<1)
#define	_LED_Groups	(1<<2)
#define	_LED_Ctrls	(1<<3)
#define	_LED_Explicit	(1<<4)
#define	_LED_Automatic	(1<<5)
#define	_LED_DrivesKbd	(1<<6)

#define	_LED_NotBound	255

typedef struct _LEDInfo
{
    CommonInfo defs;
    Atom name;
    unsigned char indicator;
    unsigned char flags;
    unsigned char which_mods;
    unsigned char real_mods;
    unsigned short vmods;
    unsigned char which_groups;
    unsigned char groups;
    unsigned int ctrls;
} LEDInfo;

extern void ClearIndicatorMapInfo(Display * /* dpy */ ,
                                  LEDInfo *     /* info */
    );


extern LEDInfo *AddIndicatorMap(LEDInfo * /* oldLEDs */ ,
                                const LEDInfo * /* newLED */
    );

extern int SetIndicatorMapField(LEDInfo * /* led */ ,
                                XkbDescPtr /* xkb */ ,
                                const char * /* field */ ,
                                const ExprDef * /* arrayNdx */ ,
                                const ExprDef * /* value */
    );

extern LEDInfo *HandleIndicatorMapDef(IndicatorMapDef * /* stmt */ ,
                                      XkbDescPtr /* xkb */ ,
                                      const LEDInfo * /* dflt */ ,
                                      LEDInfo * /* oldLEDs */ ,
                                      unsigned  /* mergeMode */
    );

extern Bool CopyIndicatorMapDefs(XkbFileInfo * /* result */ ,
                                 LEDInfo * /* leds */ ,
                                 LEDInfo **     /* unboundRtrn */
    );

extern Bool BindIndicators(XkbFileInfo * /* result */ ,
                           Bool /* force */ ,
                           LEDInfo * /* unbound */ ,
                           LEDInfo **   /* unboundRtrn */
    );

#endif /* INDICATORS_H */
