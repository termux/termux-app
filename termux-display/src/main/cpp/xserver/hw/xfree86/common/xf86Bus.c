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

/*
 * This file contains the interfaces to the bus-specific code
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/X.h>
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"

/* Bus-specific headers */

#include "xf86Bus.h"

#define XF86_OS_PRIVS
#include "xf86_OSproc.h"
#ifdef XSERVER_LIBPCIACCESS
#include "xf86VGAarbiter.h"
#endif
/* Entity data */
EntityPtr *xf86Entities = NULL; /* Bus slots claimed by drivers */
int xf86NumEntities = 0;
static int xf86EntityPrivateCount = 0;

BusRec primaryBus = { BUS_NONE, {0} };

/**
 * Call the driver's correct probe function.
 *
 * If the driver implements the \c DriverRec::PciProbe entry-point and an
 * appropriate PCI device (with matching Device section in the xorg.conf file)
 * is found, it is called.  If \c DriverRec::PciProbe or no devices can be
 * successfully probed with it (e.g., only non-PCI devices are available),
 * the driver's \c DriverRec::Probe function is called.
 *
 * \param drv   Driver to probe
 *
 * \return
 * If a device can be successfully probed by the driver, \c TRUE is
 * returned.  Otherwise, \c FALSE is returned.
 */
Bool
xf86CallDriverProbe(DriverPtr drv, Bool detect_only)
{
    Bool foundScreen = FALSE;

#ifdef XSERVER_PLATFORM_BUS
    /* xf86platformBus.c does not support Xorg -configure */
    if (!xf86DoConfigure && drv->platformProbe != NULL) {
        foundScreen = xf86platformProbeDev(drv);
    }
#endif

#ifdef XSERVER_LIBPCIACCESS
    if (!foundScreen && (drv->PciProbe != NULL)) {
        if (xf86DoConfigure && xf86DoConfigurePass1) {
            assert(detect_only);
            foundScreen = xf86PciAddMatchingDev(drv);
        }
        else {
            assert(!detect_only);
            foundScreen = xf86PciProbeDev(drv);
        }
    }
#endif
    if (!foundScreen && (drv->Probe != NULL)) {
        xf86Msg(X_WARNING, "Falling back to old probe method for %s\n",
                drv->driverName);
        foundScreen = (*drv->Probe) (drv, (detect_only) ? PROBE_DETECT
                                     : PROBE_DEFAULT);
    }

    return foundScreen;
}

static screenLayoutPtr
xf86BusConfigMatch(ScrnInfoPtr scrnInfo, Bool is_gpu) {
    screenLayoutPtr layout;
    int i, j;

    for (layout = xf86ConfigLayout.screens; layout->screen != NULL;
         layout++) {
        for (i = 0; i < scrnInfo->numEntities; i++) {
            GDevPtr dev =
                xf86GetDevFromEntity(scrnInfo->entityList[i],
                                     scrnInfo->entityInstanceList[i]);

            if (is_gpu) {
                for (j = 0; j < layout->screen->num_gpu_devices; j++) {
                    if (dev == layout->screen->gpu_devices[j]) {
                        /* A match has been found */
                        return layout;
                    }
                }
            } else {
                if (dev == layout->screen->device) {
                    /* A match has been found */
                    return layout;
                }
            }
        }
    }

    return NULL;
}

/**
 * @return TRUE if all buses are configured and set up correctly and FALSE
 * otherwise.
 */
Bool
xf86BusConfig(void)
{
    screenLayoutPtr layout;
    int i;

    /*
     * 3 step probe to (hopefully) ensure that we always find at least 1
     * (non GPU) screen:
     *
     * 1. Call each drivers probe function normally,
     *    Each successful probe will result in an extra entry added to the
     *    xf86Screens[] list for each instance of the hardware found.
     */
    for (i = 0; i < xf86NumDrivers; i++) {
        xf86CallDriverProbe(xf86DriverList[i], FALSE);
    }

    /*
     * 2. If no Screens were found, call each drivers probe function with
     *    ignorePrimary = TRUE, to ensure that we do actually get a
     *    Screen if there is at least one supported video card.
     */
    if (xf86NumScreens == 0) {
        xf86ProbeIgnorePrimary = TRUE;
        for (i = 0; i < xf86NumDrivers && xf86NumScreens == 0; i++) {
            xf86CallDriverProbe(xf86DriverList[i], FALSE);
        }
        xf86ProbeIgnorePrimary = FALSE;
    }

    /*
     * 3. Call xf86platformAddGPUDevices() to add any additional video cards as
     *    GPUScreens (GPUScreens are only supported by platformBus drivers).
     */
    for (i = 0; i < xf86NumDrivers; i++) {
        xf86platformAddGPUDevices(xf86DriverList[i]);
    }

    /* If nothing was detected, return now */
    if (xf86NumScreens == 0) {
        xf86Msg(X_ERROR, "No devices detected.\n");
        return FALSE;
    }

    xf86VGAarbiterInit();

    /*
     * Match up the screens found by the probes against those specified
     * in the config file.  Remove the ones that won't be used.  Sort
     * them in the order specified.
     *
     * What is the best way to do this?
     *
     * For now, go through the screens allocated by the probes, and
     * look for screen config entry which refers to the same device
     * section as picked out by the probe.
     *
     */
    for (i = 0; i < xf86NumScreens; i++) {
        layout = xf86BusConfigMatch(xf86Screens[i], FALSE);
        if (layout && layout->screen)
            xf86Screens[i]->confScreen = layout->screen;
        else {
            /* No match found */
            xf86Msg(X_ERROR,
                    "Screen %d deleted because of no matching config section.\n",
                    i);
            xf86DeleteScreen(xf86Screens[i--]);
        }
    }

    /* bind GPU conf screen to the configured protocol screen, or 0 if not configured */
    for (i = 0; i < xf86NumGPUScreens; i++) {
        layout = xf86BusConfigMatch(xf86GPUScreens[i], TRUE);
        int scrnum = (layout && layout->screen) ? layout->screen->screennum : 0;
        xf86GPUScreens[i]->confScreen = xf86Screens[scrnum]->confScreen;
    }

    /* If no screens left, return now.  */
    if (xf86NumScreens == 0) {
        xf86Msg(X_ERROR,
                "Device(s) detected, but none match those in the config file.\n");
        return FALSE;
    }

    return TRUE;
}

/*
 * Call the bus probes relevant to the architecture.
 *
 * The only one available so far is for PCI and SBUS.
 */

void
xf86BusProbe(void)
{
#ifdef XSERVER_PLATFORM_BUS
    xf86platformProbe();
    if (ServerIsNotSeat0() && xf86_num_platform_devices > 0)
        return;
#endif
#ifdef XSERVER_LIBPCIACCESS
    xf86PciProbe();
#endif
#if (defined(__sparc__) || defined(__sparc)) && !defined(__OpenBSD__)
    xf86SbusProbe();
#endif
#ifdef XSERVER_PLATFORM_BUS
    xf86platformPrimary();
#endif
}

/*
 * Determine what bus type the busID string represents.  The start of the
 * bus-dependent part of the string is returned as retID.
 */

BusType
StringToBusType(const char *busID, const char **retID)
{
    char *p, *s;
    BusType ret = BUS_NONE;

    /* If no type field, Default to PCI */
    if (isdigit(busID[0])) {
        if (retID)
            *retID = busID;
        return BUS_PCI;
    }

    s = xstrdup(busID);
    p = strtok(s, ":");
    if (p == NULL || *p == 0) {
        free(s);
        return BUS_NONE;
    }
    if (!xf86NameCmp(p, "pci") || !xf86NameCmp(p, "agp"))
        ret = BUS_PCI;
    if (!xf86NameCmp(p, "sbus"))
        ret = BUS_SBUS;
    if (!xf86NameCmp(p, "platform"))
        ret = BUS_PLATFORM;
    if (!xf86NameCmp(p, "usb"))
        ret = BUS_USB;
    if (ret != BUS_NONE)
        if (retID)
            *retID = busID + strlen(p) + 1;
    free(s);
    return ret;
}

int
xf86AllocateEntity(void)
{
    xf86NumEntities++;
    xf86Entities = xnfreallocarray(xf86Entities,
                                   xf86NumEntities, sizeof(EntityPtr));
    xf86Entities[xf86NumEntities - 1] = xnfcalloc(1, sizeof(EntityRec));
    xf86Entities[xf86NumEntities - 1]->entityPrivates =
        xnfcalloc(xf86EntityPrivateCount, sizeof(DevUnion));
    return xf86NumEntities - 1;
}

Bool
xf86IsEntityPrimary(int entityIndex)
{
    EntityPtr pEnt = xf86Entities[entityIndex];

#ifdef XSERVER_LIBPCIACCESS
    if (primaryBus.type == BUS_PLATFORM && pEnt->bus.type == BUS_PCI)
        if (primaryBus.id.plat->pdev)
            return MATCH_PCI_DEVICES(pEnt->bus.id.pci, primaryBus.id.plat->pdev);
#endif

    if (primaryBus.type != pEnt->bus.type)
        return FALSE;

    switch (pEnt->bus.type) {
    case BUS_PCI:
        return pEnt->bus.id.pci == primaryBus.id.pci;
    case BUS_SBUS:
        return pEnt->bus.id.sbus.fbNum == primaryBus.id.sbus.fbNum;
    case BUS_PLATFORM:
        return pEnt->bus.id.plat == primaryBus.id.plat;
    default:
        return FALSE;
    }
}

Bool
xf86DriverHasEntities(DriverPtr drvp)
{
    int i;

    for (i = 0; i < xf86NumEntities; i++) {
        if (xf86Entities[i]->driver == drvp)
            return TRUE;
    }
    return FALSE;
}

void
xf86AddEntityToScreen(ScrnInfoPtr pScrn, int entityIndex)
{
    if (entityIndex == -1)
        return;
    if (xf86Entities[entityIndex]->inUse &&
        !(xf86Entities[entityIndex]->entityProp & IS_SHARED_ACCEL)) {
        ErrorF("Requested Entity already in use!\n");
        return;
    }

    pScrn->numEntities++;
    pScrn->entityList = xnfreallocarray(pScrn->entityList,
                                        pScrn->numEntities, sizeof(int));
    pScrn->entityList[pScrn->numEntities - 1] = entityIndex;
    xf86Entities[entityIndex]->inUse = TRUE;
    pScrn->entityInstanceList = xnfreallocarray(pScrn->entityInstanceList,
                                                pScrn->numEntities,
                                                sizeof(int));
    pScrn->entityInstanceList[pScrn->numEntities - 1] = 0;
}

void
xf86SetEntityInstanceForScreen(ScrnInfoPtr pScrn, int entityIndex, int instance)
{
    int i;

    if (entityIndex == -1 || entityIndex >= xf86NumEntities)
        return;

    for (i = 0; i < pScrn->numEntities; i++) {
        if (pScrn->entityList[i] == entityIndex) {
            pScrn->entityInstanceList[i] = instance;
            break;
        }
    }
}

/*
 * XXX  This needs to be updated for the case where a single entity may have
 * instances associated with more than one screen.
 */
ScrnInfoPtr
xf86FindScreenForEntity(int entityIndex)
{
    int i, j;

    if (entityIndex == -1)
        return NULL;

    if (xf86Screens) {
        for (i = 0; i < xf86NumScreens; i++) {
            for (j = 0; j < xf86Screens[i]->numEntities; j++) {
                if (xf86Screens[i]->entityList[j] == entityIndex)
                    return xf86Screens[i];
            }
        }
    }
    return NULL;
}

void
xf86RemoveEntityFromScreen(ScrnInfoPtr pScrn, int entityIndex)
{
    int i;

    for (i = 0; i < pScrn->numEntities; i++) {
        if (pScrn->entityList[i] == entityIndex) {
            for (i++; i < pScrn->numEntities; i++)
                pScrn->entityList[i - 1] = pScrn->entityList[i];
            pScrn->numEntities--;
            xf86Entities[entityIndex]->inUse = FALSE;
            break;
        }
    }
}

/*
 * xf86ClearEntityListForScreen() - called when a screen is deleted
 * to mark its entities unused. Called by xf86DeleteScreen().
 */
void
xf86ClearEntityListForScreen(ScrnInfoPtr pScrn)
{
    int i, entityIndex;

    if (pScrn->entityList == NULL || pScrn->numEntities == 0)
        return;

    for (i = 0; i < pScrn->numEntities; i++) {
        entityIndex = pScrn->entityList[i];
        xf86Entities[entityIndex]->inUse = FALSE;
        /* disable resource: call the disable function */
    }
    free(pScrn->entityList);
    free(pScrn->entityInstanceList);
    pScrn->entityList = NULL;
    pScrn->entityInstanceList = NULL;
}

/*
 * Add an extra device section (GDevPtr) to an entity.
 */

void
xf86AddDevToEntity(int entityIndex, GDevPtr dev)
{
    EntityPtr pEnt;

    if (entityIndex >= xf86NumEntities)
        return;

    pEnt = xf86Entities[entityIndex];
    pEnt->numInstances++;
    pEnt->devices = xnfreallocarray(pEnt->devices,
                                    pEnt->numInstances, sizeof(GDevPtr));
    pEnt->devices[pEnt->numInstances - 1] = dev;
    dev->claimed = TRUE;
}


void
xf86RemoveDevFromEntity(int entityIndex, GDevPtr dev)
{
    EntityPtr pEnt;
    int i, j;
    if (entityIndex >= xf86NumEntities)
        return;

    pEnt = xf86Entities[entityIndex];
    for (i = 0; i < pEnt->numInstances; i++) {
        if (pEnt->devices[i] == dev) {
            for (j = i; j < pEnt->numInstances - 1; j++)
                pEnt->devices[j] = pEnt->devices[j + 1];
            break;
        }
    }
    pEnt->numInstances--;
    dev->claimed = FALSE;
}
/*
 * xf86GetEntityInfo() -- This function hands information from the
 * EntityRec struct to the drivers. The EntityRec structure itself
 * remains invisible to the driver.
 */
EntityInfoPtr
xf86GetEntityInfo(int entityIndex)
{
    EntityInfoPtr pEnt;
    int i;

    if (entityIndex == -1)
        return NULL;

    if (entityIndex >= xf86NumEntities)
        return NULL;

    pEnt = xnfcalloc(1, sizeof(EntityInfoRec));
    pEnt->index = entityIndex;
    pEnt->location = xf86Entities[entityIndex]->bus;
    pEnt->active = xf86Entities[entityIndex]->active;
    pEnt->chipset = xf86Entities[entityIndex]->chipset;
    pEnt->driver = xf86Entities[entityIndex]->driver;
    if ((xf86Entities[entityIndex]->devices) &&
        (xf86Entities[entityIndex]->devices[0])) {
        for (i = 0; i < xf86Entities[entityIndex]->numInstances; i++)
            if (xf86Entities[entityIndex]->devices[i]->screen == 0)
                break;
        pEnt->device = xf86Entities[entityIndex]->devices[i];
    }
    else
        pEnt->device = NULL;

    return pEnt;
}

int
xf86GetNumEntityInstances(int entityIndex)
{
    if (entityIndex >= xf86NumEntities)
        return -1;

    return xf86Entities[entityIndex]->numInstances;
}

GDevPtr
xf86GetDevFromEntity(int entityIndex, int instance)
{
    int i;

    /* We might not use AddDevtoEntity */
    if ((!xf86Entities[entityIndex]->devices) ||
        (!xf86Entities[entityIndex]->devices[0]))
        return NULL;

    if (entityIndex >= xf86NumEntities ||
        instance >= xf86Entities[entityIndex]->numInstances)
        return NULL;

    for (i = 0; i < xf86Entities[entityIndex]->numInstances; i++)
        if (xf86Entities[entityIndex]->devices[i]->screen == instance)
            return xf86Entities[entityIndex]->devices[i];
    return NULL;
}

/*
 * xf86PostProbe() -- Allocate all non conflicting resources
 * This function gets called by xf86Init().
 */
void
xf86PostProbe(void)
{
    if (fbSlotClaimed && (
#if (defined(__sparc__) || defined(__sparc)) && !defined(__OpenBSD__)
                             sbusSlotClaimed ||
#endif
#ifdef XSERVER_PLATFORM_BUS
                             platformSlotClaimed ||
#endif
#ifdef XSERVER_LIBPCIACCESS
                             pciSlotClaimed
#else
                             TRUE
#endif
        ))
        FatalError("Cannot run in framebuffer mode. Please specify busIDs "
                   "       for all framebuffer devices\n");
}

Bool
xf86IsEntityShared(int entityIndex)
{
    if (entityIndex < xf86NumEntities) {
        if (xf86Entities[entityIndex]->entityProp & IS_SHARED_ACCEL) {
            return TRUE;
        }
    }
    return FALSE;
}

void
xf86SetEntityShared(int entityIndex)
{
    if (entityIndex < xf86NumEntities) {
        xf86Entities[entityIndex]->entityProp |= IS_SHARED_ACCEL;
    }
}

Bool
xf86IsEntitySharable(int entityIndex)
{
    if (entityIndex < xf86NumEntities) {
        if (xf86Entities[entityIndex]->entityProp & ACCEL_IS_SHARABLE) {
            return TRUE;
        }
    }
    return FALSE;
}

void
xf86SetEntitySharable(int entityIndex)
{
    if (entityIndex < xf86NumEntities) {
        xf86Entities[entityIndex]->entityProp |= ACCEL_IS_SHARABLE;
    }
}

Bool
xf86IsPrimInitDone(int entityIndex)
{
    if (entityIndex < xf86NumEntities) {
        if (xf86Entities[entityIndex]->entityProp & SA_PRIM_INIT_DONE) {
            return TRUE;
        }
    }
    return FALSE;
}

void
xf86SetPrimInitDone(int entityIndex)
{
    if (entityIndex < xf86NumEntities) {
        xf86Entities[entityIndex]->entityProp |= SA_PRIM_INIT_DONE;
    }
}

void
xf86ClearPrimInitDone(int entityIndex)
{
    if (entityIndex < xf86NumEntities) {
        xf86Entities[entityIndex]->entityProp &= ~SA_PRIM_INIT_DONE;
    }
}

/*
 * Allocate a private in the entities.
 */

int
xf86AllocateEntityPrivateIndex(void)
{
    int idx, i;
    EntityPtr pEnt;
    DevUnion *nprivs;

    idx = xf86EntityPrivateCount++;
    for (i = 0; i < xf86NumEntities; i++) {
        pEnt = xf86Entities[i];
        nprivs = xnfreallocarray(pEnt->entityPrivates,
                                 xf86EntityPrivateCount, sizeof(DevUnion));
        /* Zero the new private */
        memset(&nprivs[idx], 0, sizeof(DevUnion));
        pEnt->entityPrivates = nprivs;
    }
    return idx;
}

DevUnion *
xf86GetEntityPrivate(int entityIndex, int privIndex)
{
    if (entityIndex >= xf86NumEntities || privIndex >= xf86EntityPrivateCount)
        return NULL;

    return &(xf86Entities[entityIndex]->entityPrivates[privIndex]);
}
