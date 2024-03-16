
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include "windowstr.h"
#include <xkbsrv.h>

int
XkbDDXPrivate(DeviceIntPtr dev, KeyCode key, XkbAction *act)
{
    return 0;
}
