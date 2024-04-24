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
#include "xf86_OSproc.h"
#define _INT10_PRIVATE
#include "xf86int10.h"

xf86Int10InfoPtr
xf86InitInt10(int entityIndex)
{
    return xf86ExtendedInitInt10(entityIndex, 0);
}

xf86Int10InfoPtr
xf86ExtendedInitInt10(int entityIndex, int Flags)
{
    return NULL;
}

Bool
MapCurrentInt10(xf86Int10InfoPtr pInt)
{
    return FALSE;
}

void
xf86FreeInt10(xf86Int10InfoPtr pInt)
{
    return;
}

void *
xf86Int10AllocPages(xf86Int10InfoPtr pInt, int num, int *off)
{
    *off = 0;
    return NULL;
}

void
xf86Int10FreePages(xf86Int10InfoPtr pInt, void *pbase, int num)
{
    return;
}

Bool
xf86Int10ExecSetup(xf86Int10InfoPtr pInt)
{
    return FALSE;
}

void
xf86ExecX86int10(xf86Int10InfoPtr pInt)
{
    return;
}

void *
xf86int10Addr(xf86Int10InfoPtr pInt, uint32_t addr)
{
    return 0;
}
