/*
 * Copyright 1998 by The XFree86 Project, Inc
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86Module.h"

static XF86ModuleVersionInfo VersRec = {
    "vgahw",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    0, 1, 0,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_NONE,
    {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData vgahwModuleData = { &VersRec, NULL, NULL };
