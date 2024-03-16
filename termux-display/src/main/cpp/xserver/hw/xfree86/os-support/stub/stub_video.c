#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86_OSlib.h"
#include "xf86OSpriv.h"

void
xf86OSInitVidMem(VidMemInfoPtr pVidMem)
{
    pVidMem->initialised = TRUE;
    return;
}
