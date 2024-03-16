/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86.h"
#include "xf86str.h"
#include "xf86Pci.h"
#include "xf86int10.h"

#ifndef MOD_NAME
#define MOD_NAME int10
#endif

#define stringify(x) #x
#define STRING(x) stringify(x)
#define concat(x,y) x ## y
#define combine(a,b) concat(a,b)
#define NAME(x) combine(MOD_NAME,x)

static XF86ModuleVersionInfo NAME(VersRec) = {
    STRING(NAME()), MODULEVENDORSTRING, MODINFOSTRING1, MODINFOSTRING2, XORG_VERSION_CURRENT, 1, 0, 0, ABI_CLASS_VIDEODRV,      /* needs the video driver ABI */
        ABI_VIDEODRV_VERSION, MOD_CLASS_NONE, {
    0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData NAME(ModuleData) = {
&NAME(VersRec), NULL, NULL};
