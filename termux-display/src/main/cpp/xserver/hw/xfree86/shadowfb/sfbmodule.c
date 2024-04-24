#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86Module.h"

static XF86ModuleVersionInfo VersRec = {
    "shadowfb",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    1, 0, 0,
    ABI_CLASS_ANSIC,            /* Only need the ansic layer */
    ABI_ANSIC_VERSION,
    MOD_CLASS_NONE,
    {0, 0, 0, 0}                /* signature, to be patched into the file by a tool */
};

_X_EXPORT XF86ModuleData shadowfbModuleData = { &VersRec, NULL, NULL };
