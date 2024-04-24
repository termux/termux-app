/*
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2011 Dave Airlie
 * Copyright 2019 NVIDIA CORPORATION
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * Original Author: Alan Hourihane <alanh@tungstengraphics.com>
 * Rewrite: Dave Airlie <airlied@redhat.com>
 * Additional contributors:
 *   Aaron Plattner <aplattner@nvidia.com>
 *
 */

#ifdef HAVE_DIX_CONFIG_H
#include "dix-config.h"
#endif

#include <unistd.h>
#include <fcntl.h>
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSproc.h"
#include "compiler.h"
#include "xf86Pci.h"
#include "mipointer.h"
#include "mipointrst.h"
#include "micmap.h"
#include <X11/extensions/randr.h>
#include "fb.h"
#include "edid.h"
#include "xf86i2c.h"
#include "xf86Crtc.h"
#include "miscstruct.h"
#include "dixstruct.h"
#include "xf86xv.h"
#include <X11/extensions/Xv.h>
#include <xorg-config.h>
#ifdef XSERVER_PLATFORM_BUS
#include "xf86platformBus.h"
#endif
#ifdef XSERVER_LIBPCIACCESS
#include <pciaccess.h>
#endif
#include "driver.h"

static void AdjustFrame(ScrnInfoPtr pScrn, int x, int y);
static Bool CloseScreen(ScreenPtr pScreen);
static Bool EnterVT(ScrnInfoPtr pScrn);
static void Identify(int flags);
static const OptionInfoRec *AvailableOptions(int chipid, int busid);
static ModeStatus ValidMode(ScrnInfoPtr pScrn, DisplayModePtr mode,
                            Bool verbose, int flags);
static void FreeScreen(ScrnInfoPtr pScrn);
static void LeaveVT(ScrnInfoPtr pScrn);
static Bool SwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode);
static Bool ScreenInit(ScreenPtr pScreen, int argc, char **argv);
static Bool PreInit(ScrnInfoPtr pScrn, int flags);

static Bool Probe(DriverPtr drv, int flags);
static Bool ms_pci_probe(DriverPtr driver,
                         int entity_num, struct pci_device *device,
                         intptr_t match_data);
static Bool ms_driver_func(ScrnInfoPtr scrn, xorgDriverFuncOp op, void *data);

/* window wrapper functions used to get the notification when
 * the window property changes */
static Atom vrr_atom;
static Bool property_vectors_wrapped;
static Bool restore_property_vector;
static int (*saved_change_property) (ClientPtr client);
static int (*saved_delete_property) (ClientPtr client);

#ifdef XSERVER_LIBPCIACCESS
static const struct pci_id_match ms_device_match[] = {
    {
     PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY, PCI_MATCH_ANY,
     0x00030000, 0x00ff0000, 0},

    {0, 0, 0},
};
#endif

#ifndef XSERVER_PLATFORM_BUS
struct xf86_platform_device;
#endif

#ifdef XSERVER_PLATFORM_BUS
static Bool ms_platform_probe(DriverPtr driver,
                              int entity_num, int flags,
                              struct xf86_platform_device *device,
                              intptr_t match_data);
#endif

_X_EXPORT DriverRec modesetting = {
    1,
    "modesetting",
    Identify,
    Probe,
    AvailableOptions,
    NULL,
    0,
    ms_driver_func,
    ms_device_match,
    ms_pci_probe,
#ifdef XSERVER_PLATFORM_BUS
    ms_platform_probe,
#endif
};

static SymTabRec Chipsets[] = {
    {0, "kms"},
    {-1, NULL}
};

static const OptionInfoRec Options[] = {
    {OPTION_SW_CURSOR, "SWcursor", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_DEVICE_PATH, "kmsdev", OPTV_STRING, {0}, FALSE},
    {OPTION_SHADOW_FB, "ShadowFB", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_ACCEL_METHOD, "AccelMethod", OPTV_STRING, {0}, FALSE},
    {OPTION_PAGEFLIP, "PageFlip", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_ZAPHOD_HEADS, "ZaphodHeads", OPTV_STRING, {0}, FALSE},
    {OPTION_DOUBLE_SHADOW, "DoubleShadow", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_ATOMIC, "Atomic", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_VARIABLE_REFRESH, "VariableRefresh", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_USE_GAMMA_LUT, "UseGammaLUT", OPTV_BOOLEAN, {0}, FALSE},
    {OPTION_ASYNC_FLIP_SECONDARIES, "AsyncFlipSecondaries", OPTV_BOOLEAN, {0}, FALSE},
    {-1, NULL, OPTV_NONE, {0}, FALSE}
};

int ms_entity_index = -1;

static MODULESETUPPROTO(Setup);

static XF86ModuleVersionInfo VersRec = {
    "modesetting",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    XORG_VERSION_MAJOR,
    XORG_VERSION_MINOR,
    XORG_VERSION_PATCH,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_VIDEODRV,
    {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData modesettingModuleData = { &VersRec, Setup, NULL };

static void *
Setup(void *module, void *opts, int *errmaj, int *errmin)
{
    static Bool setupDone = 0;

    /* This module should be loaded only once, but check to be sure.
     */
    if (!setupDone) {
        setupDone = 1;
        xf86AddDriver(&modesetting, module, HaveDriverFuncs);

        /*
         * The return value must be non-NULL on success even though there
         * is no TearDownProc.
         */
        return (void *) 1;
    }
    else {
        if (errmaj)
            *errmaj = LDR_ONCEONLY;
        return NULL;
    }
}

static void
Identify(int flags)
{
    xf86PrintChipsets("modesetting", "Driver for Modesetting Kernel Drivers",
                      Chipsets);
}

modesettingEntPtr ms_ent_priv(ScrnInfoPtr scrn)
{
    DevUnion     *pPriv;
    modesettingPtr ms = modesettingPTR(scrn);
    pPriv = xf86GetEntityPrivate(ms->pEnt->index,
                                 ms_entity_index);
    return pPriv->ptr;
}

static int
get_passed_fd(void)
{
    if (xf86DRMMasterFd >= 0) {
        xf86DrvMsg(-1, X_INFO, "Using passed DRM master file descriptor %d\n", xf86DRMMasterFd);
        return dup(xf86DRMMasterFd);
    }
    return -1;
}

static int
open_hw(const char *dev)
{
    int fd;

    if ((fd = get_passed_fd()) != -1)
        return fd;

    if (dev)
        fd = open(dev, O_RDWR | O_CLOEXEC, 0);
    else {
        dev = getenv("KMSDEVICE");
        if ((NULL == dev) || ((fd = open(dev, O_RDWR | O_CLOEXEC, 0)) == -1)) {
            dev = "/dev/dri/card0";
            fd = open(dev, O_RDWR | O_CLOEXEC, 0);
        }
    }
    if (fd == -1)
        xf86DrvMsg(-1, X_ERROR, "open %s: %s\n", dev, strerror(errno));

    return fd;
}

static int
check_outputs(int fd, int *count)
{
    drmModeResPtr res = drmModeGetResources(fd);
    int ret;

    if (!res)
        return FALSE;

    if (count)
        *count = res->count_connectors;

    ret = res->count_connectors > 0;
#if defined(GLAMOR_HAS_GBM_LINEAR)
    if (ret == FALSE) {
        uint64_t value = 0;
        if (drmGetCap(fd, DRM_CAP_PRIME, &value) == 0 &&
                (value & DRM_PRIME_CAP_EXPORT))
            ret = TRUE;
    }
#endif
    drmModeFreeResources(res);
    return ret;
}

static Bool
probe_hw(const char *dev, struct xf86_platform_device *platform_dev)
{
    int fd;

#ifdef XF86_PDEV_SERVER_FD
    if (platform_dev && (platform_dev->flags & XF86_PDEV_SERVER_FD)) {
        fd = xf86_platform_device_odev_attributes(platform_dev)->fd;
        if (fd == -1)
            return FALSE;
        return check_outputs(fd, NULL);
    }
#endif

    fd = open_hw(dev);
    if (fd != -1) {
        int ret = check_outputs(fd, NULL);

        close(fd);
        return ret;
    }
    return FALSE;
}

static char *
ms_DRICreatePCIBusID(const struct pci_device *dev)
{
    char *busID;

    if (asprintf(&busID, "pci:%04x:%02x:%02x.%d",
                 dev->domain, dev->bus, dev->dev, dev->func) == -1)
        return NULL;

    return busID;
}

static Bool
probe_hw_pci(const char *dev, struct pci_device *pdev)
{
    int ret = FALSE, fd = open_hw(dev);
    char *id, *devid;
    drmSetVersion sv;

    if (fd == -1)
        return FALSE;

    sv.drm_di_major = 1;
    sv.drm_di_minor = 4;
    sv.drm_dd_major = -1;
    sv.drm_dd_minor = -1;
    if (drmSetInterfaceVersion(fd, &sv)) {
        close(fd);
        return FALSE;
    }

    id = drmGetBusid(fd);
    devid = ms_DRICreatePCIBusID(pdev);

    if (id && devid && !strcmp(id, devid))
        ret = check_outputs(fd, NULL);

    close(fd);
    free(id);
    free(devid);
    return ret;
}

static const OptionInfoRec *
AvailableOptions(int chipid, int busid)
{
    return Options;
}

static Bool
ms_driver_func(ScrnInfoPtr scrn, xorgDriverFuncOp op, void *data)
{
    xorgHWFlags *flag;

    switch (op) {
    case GET_REQUIRED_HW_INTERFACES:
        flag = (CARD32 *) data;
        (*flag) = 0;
        return TRUE;
    case SUPPORTS_SERVER_FDS:
        return TRUE;
    default:
        return FALSE;
    }
}

static void
ms_setup_scrn_hooks(ScrnInfoPtr scrn)
{
    scrn->driverVersion = 1;
    scrn->driverName = "modesetting";
    scrn->name = "modeset";

    scrn->Probe = NULL;
    scrn->PreInit = PreInit;
    scrn->ScreenInit = ScreenInit;
    scrn->SwitchMode = SwitchMode;
    scrn->AdjustFrame = AdjustFrame;
    scrn->EnterVT = EnterVT;
    scrn->LeaveVT = LeaveVT;
    scrn->FreeScreen = FreeScreen;
    scrn->ValidMode = ValidMode;
}

static void
ms_setup_entity(ScrnInfoPtr scrn, int entity_num)
{
    DevUnion *pPriv;

    xf86SetEntitySharable(entity_num);

    if (ms_entity_index == -1)
        ms_entity_index = xf86AllocateEntityPrivateIndex();

    pPriv = xf86GetEntityPrivate(entity_num,
                                 ms_entity_index);

    xf86SetEntityInstanceForScreen(scrn, entity_num, xf86GetNumEntityInstances(entity_num) - 1);

    if (!pPriv->ptr)
        pPriv->ptr = xnfcalloc(sizeof(modesettingEntRec), 1);
}

#ifdef XSERVER_LIBPCIACCESS
static Bool
ms_pci_probe(DriverPtr driver,
             int entity_num, struct pci_device *dev, intptr_t match_data)
{
    ScrnInfoPtr scrn = NULL;

    scrn = xf86ConfigPciEntity(scrn, 0, entity_num, NULL,
                               NULL, NULL, NULL, NULL, NULL);
    if (scrn) {
        const char *devpath;
        GDevPtr devSection = xf86GetDevFromEntity(scrn->entityList[0],
                                                  scrn->entityInstanceList[0]);

        devpath = xf86FindOptionValue(devSection->options, "kmsdev");
        if (probe_hw_pci(devpath, dev)) {
            ms_setup_scrn_hooks(scrn);

            xf86DrvMsg(scrn->scrnIndex, X_CONFIG,
                       "claimed PCI slot %d@%d:%d:%d\n",
                       dev->bus, dev->domain, dev->dev, dev->func);
            xf86DrvMsg(scrn->scrnIndex, X_INFO,
                       "using %s\n", devpath ? devpath : "default device");

            ms_setup_entity(scrn, entity_num);
        }
        else
            scrn = NULL;
    }
    return scrn != NULL;
}
#endif

#ifdef XSERVER_PLATFORM_BUS
static Bool
ms_platform_probe(DriverPtr driver,
                  int entity_num, int flags, struct xf86_platform_device *dev,
                  intptr_t match_data)
{
    ScrnInfoPtr scrn = NULL;
    const char *path = xf86_platform_device_odev_attributes(dev)->path;
    int scr_flags = 0;

    if (flags & PLATFORM_PROBE_GPU_SCREEN)
        scr_flags = XF86_ALLOCATE_GPU_SCREEN;

    if (probe_hw(path, dev)) {
        scrn = xf86AllocateScreen(driver, scr_flags);
        if (xf86IsEntitySharable(entity_num))
            xf86SetEntityShared(entity_num);
        xf86AddEntityToScreen(scrn, entity_num);

        ms_setup_scrn_hooks(scrn);

        xf86DrvMsg(scrn->scrnIndex, X_INFO,
                   "using drv %s\n", path ? path : "default device");

        ms_setup_entity(scrn, entity_num);
    }

    return scrn != NULL;
}
#endif

static Bool
Probe(DriverPtr drv, int flags)
{
    int i, numDevSections;
    GDevPtr *devSections;
    Bool foundScreen = FALSE;
    const char *dev;
    ScrnInfoPtr scrn = NULL;

    /* For now, just bail out for PROBE_DETECT. */
    if (flags & PROBE_DETECT)
        return FALSE;

    /*
     * Find the config file Device sections that match this
     * driver, and return if there are none.
     */
    if ((numDevSections = xf86MatchDevice("modesetting", &devSections)) <= 0) {
        return FALSE;
    }

    for (i = 0; i < numDevSections; i++) {
        int entity_num;
        dev = xf86FindOptionValue(devSections[i]->options, "kmsdev");
        if (probe_hw(dev, NULL)) {

            entity_num = xf86ClaimFbSlot(drv, 0, devSections[i], TRUE);
            scrn = xf86ConfigFbEntity(scrn, 0, entity_num, NULL, NULL, NULL, NULL);
        }

        if (scrn) {
            foundScreen = TRUE;
            ms_setup_scrn_hooks(scrn);
            scrn->Probe = Probe;

            xf86DrvMsg(scrn->scrnIndex, X_INFO,
                       "using %s\n", dev ? dev : "default device");
            ms_setup_entity(scrn, entity_num);
        }
    }

    free(devSections);

    return foundScreen;
}

static Bool
GetRec(ScrnInfoPtr pScrn)
{
    if (pScrn->driverPrivate)
        return TRUE;

    pScrn->driverPrivate = xnfcalloc(sizeof(modesettingRec), 1);

    return TRUE;
}

static int
dispatch_dirty_region(ScrnInfoPtr scrn,
                      PixmapPtr pixmap, DamagePtr damage, int fb_id)
{
    modesettingPtr ms = modesettingPTR(scrn);
    RegionPtr dirty = DamageRegion(damage);
    unsigned num_cliprects = REGION_NUM_RECTS(dirty);
    int ret = 0;

    if (num_cliprects) {
        drmModeClip *clip = xallocarray(num_cliprects, sizeof(drmModeClip));
        BoxPtr rect = REGION_RECTS(dirty);
        int i;

        if (!clip)
            return -ENOMEM;

        /* XXX no need for copy? */
        for (i = 0; i < num_cliprects; i++, rect++) {
            clip[i].x1 = rect->x1;
            clip[i].y1 = rect->y1;
            clip[i].x2 = rect->x2;
            clip[i].y2 = rect->y2;
        }

        /* TODO query connector property to see if this is needed */
        ret = drmModeDirtyFB(ms->fd, fb_id, clip, num_cliprects);

        /* if we're swamping it with work, try one at a time */
        if (ret == -EINVAL) {
            for (i = 0; i < num_cliprects; i++) {
                if ((ret = drmModeDirtyFB(ms->fd, fb_id, &clip[i], 1)) < 0)
                    break;
            }
        }

        free(clip);
        DamageEmpty(damage);
    }
    return ret;
}

static void
dispatch_dirty(ScreenPtr pScreen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(scrn);
    PixmapPtr pixmap = pScreen->GetScreenPixmap(pScreen);
    int fb_id = ms->drmmode.fb_id;
    int ret;

    ret = dispatch_dirty_region(scrn, pixmap, ms->damage, fb_id);
    if (ret == -EINVAL || ret == -ENOSYS) {
        ms->dirty_enabled = FALSE;
        DamageUnregister(ms->damage);
        DamageDestroy(ms->damage);
        ms->damage = NULL;
        xf86DrvMsg(scrn->scrnIndex, X_INFO,
                   "Disabling kernel dirty updates, not required.\n");
        return;
    }
}

static void
dispatch_dirty_pixmap(ScrnInfoPtr scrn, xf86CrtcPtr crtc, PixmapPtr ppix)
{
    modesettingPtr ms = modesettingPTR(scrn);
    msPixmapPrivPtr ppriv = msGetPixmapPriv(&ms->drmmode, ppix);
    DamagePtr damage = ppriv->secondary_damage;
    int fb_id = ppriv->fb_id;

    dispatch_dirty_region(scrn, ppix, damage, fb_id);
}

static void
dispatch_secondary_dirty(ScreenPtr pScreen)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(pScreen);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);
    int c;

    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

        if (!drmmode_crtc)
            continue;

        if (drmmode_crtc->prime_pixmap)
            dispatch_dirty_pixmap(scrn, crtc, drmmode_crtc->prime_pixmap);
        if (drmmode_crtc->prime_pixmap_back)
            dispatch_dirty_pixmap(scrn, crtc, drmmode_crtc->prime_pixmap_back);
    }
}

static void
redisplay_dirty(ScreenPtr screen, PixmapDirtyUpdatePtr dirty, int *timeout)
{
    RegionRec pixregion;

    PixmapRegionInit(&pixregion, dirty->secondary_dst);
    DamageRegionAppend(&dirty->secondary_dst->drawable, &pixregion);
    PixmapSyncDirtyHelper(dirty);

    if (!screen->isGPU) {
#ifdef GLAMOR_HAS_GBM
        modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(screen));
        /*
         * When copying from the primary framebuffer to the shared pixmap,
         * we must ensure the copy is complete before the secondary starts a
         * copy to its own framebuffer (some secondarys scanout directly from
         * the shared pixmap, but not all).
         */
        if (ms->drmmode.glamor)
            ms->glamor.finish(screen);
#endif
        /* Ensure the secondary processes the damage immediately */
        if (timeout)
            *timeout = 0;
    }

    DamageRegionProcessPending(&dirty->secondary_dst->drawable);
    RegionUninit(&pixregion);
}

static void
ms_dirty_update(ScreenPtr screen, int *timeout)
{
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(screen));

    RegionPtr region;
    PixmapDirtyUpdatePtr ent;

    if (xorg_list_is_empty(&screen->pixmap_dirty_list))
        return;

    xorg_list_for_each_entry(ent, &screen->pixmap_dirty_list, ent) {
        region = DamageRegion(ent->damage);
        if (RegionNotEmpty(region)) {
            if (!screen->isGPU) {
                   msPixmapPrivPtr ppriv =
                    msGetPixmapPriv(&ms->drmmode, ent->secondary_dst->primary_pixmap);

                if (ppriv->notify_on_damage) {
                    ppriv->notify_on_damage = FALSE;

                    ent->secondary_dst->drawable.pScreen->
                        SharedPixmapNotifyDamage(ent->secondary_dst);
                }

                /* Requested manual updating */
                if (ppriv->defer_dirty_update)
                    continue;
            }

            redisplay_dirty(screen, ent, timeout);
            DamageEmpty(ent->damage);
        }
    }
}

static PixmapDirtyUpdatePtr
ms_dirty_get_ent(ScreenPtr screen, PixmapPtr secondary_dst)
{
    PixmapDirtyUpdatePtr ent;

    if (xorg_list_is_empty(&screen->pixmap_dirty_list))
        return NULL;

    xorg_list_for_each_entry(ent, &screen->pixmap_dirty_list, ent) {
        if (ent->secondary_dst == secondary_dst)
            return ent;
    }

    return NULL;
}

static void
msBlockHandler(ScreenPtr pScreen, void *timeout)
{
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(pScreen));

    pScreen->BlockHandler = ms->BlockHandler;
    pScreen->BlockHandler(pScreen, timeout);
    ms->BlockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = msBlockHandler;
    if (pScreen->isGPU && !ms->drmmode.reverse_prime_offload_mode)
        dispatch_secondary_dirty(pScreen);
    else if (ms->dirty_enabled)
        dispatch_dirty(pScreen);

    ms_dirty_update(pScreen, timeout);
}

static void
msBlockHandler_oneshot(ScreenPtr pScreen, void *pTimeout)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);

    msBlockHandler(pScreen, pTimeout);

    drmmode_set_desired_modes(pScrn, &ms->drmmode, TRUE, FALSE);
}

Bool
ms_window_has_variable_refresh(modesettingPtr ms, WindowPtr win) {
	struct ms_vrr_priv *priv = dixLookupPrivate(&win->devPrivates, &ms->drmmode.vrrPrivateKeyRec);

	return priv->variable_refresh;
}

static void
ms_vrr_property_update(WindowPtr window, Bool variable_refresh)
{
    ScrnInfoPtr scrn = xf86ScreenToScrn(window->drawable.pScreen);
    modesettingPtr ms = modesettingPTR(scrn);

    struct ms_vrr_priv *priv = dixLookupPrivate(&window->devPrivates,
                                                &ms->drmmode.vrrPrivateKeyRec);
    priv->variable_refresh = variable_refresh;

    if (ms->flip_window == window && ms->drmmode.present_flipping)
        ms_present_set_screen_vrr(scrn, variable_refresh);
}

/* Wrapper for xserver/dix/property.c:ProcChangeProperty */
static int
ms_change_property(ClientPtr client)
{
    WindowPtr window = NULL;
    int ret = 0;

    REQUEST(xChangePropertyReq);

    client->requestVector[X_ChangeProperty] = saved_change_property;
    ret = saved_change_property(client);

    if (restore_property_vector)
        return ret;

    client->requestVector[X_ChangeProperty] = ms_change_property;

    if (ret != Success)
        return ret;

    ret = dixLookupWindow(&window, stuff->window, client, DixSetPropAccess);
    if (ret != Success)
        return ret;

    // Checking for the VRR property change on the window
    if (stuff->property == vrr_atom &&
        xf86ScreenToScrn(window->drawable.pScreen)->PreInit == PreInit &&
        stuff->format == 32 && stuff->nUnits == 1) {
        uint32_t *value = (uint32_t *)(stuff + 1);
        ms_vrr_property_update(window, *value != 0);
    }

    return ret;
}

/* Wrapper for xserver/dix/property.c:ProcDeleteProperty */
static int
ms_delete_property(ClientPtr client)
{
    WindowPtr window;
    int ret;

    REQUEST(xDeletePropertyReq);

    client->requestVector[X_DeleteProperty] = saved_delete_property;
    ret = saved_delete_property(client);

    if (restore_property_vector)
        return ret;

    client->requestVector[X_DeleteProperty] = ms_delete_property;

    if (ret != Success)
        return ret;

    ret = dixLookupWindow(&window, stuff->window, client, DixSetPropAccess);
    if (ret != Success)
        return ret;

    if (stuff->property == vrr_atom &&
        xf86ScreenToScrn(window->drawable.pScreen)->PreInit == PreInit)
        ms_vrr_property_update(window, FALSE);

    return ret;
}

static void
ms_unwrap_property_requests(ScrnInfoPtr scrn)
{
    int i;

    if (!property_vectors_wrapped)
        return;

    if (ProcVector[X_ChangeProperty] == ms_change_property)
        ProcVector[X_ChangeProperty] = saved_change_property;
    else
        restore_property_vector = TRUE;

    if (ProcVector[X_DeleteProperty] == ms_delete_property)
        ProcVector[X_DeleteProperty] = saved_delete_property;
    else
        restore_property_vector = TRUE;

    for (i = 0; i < currentMaxClients; i++) {
        if (clients[i]->requestVector[X_ChangeProperty] == ms_change_property) {
            clients[i]->requestVector[X_ChangeProperty] = saved_change_property;
        } else {
            restore_property_vector = TRUE;
        }

        if (clients[i]->requestVector[X_DeleteProperty] == ms_delete_property) {
            clients[i]->requestVector[X_DeleteProperty] = saved_delete_property;
        } else {
            restore_property_vector = TRUE;
        }
    }

    if (restore_property_vector) {
        xf86DrvMsg(scrn->scrnIndex, X_WARNING,
                   "Couldn't unwrap some window property request vectors\n");
    }

    property_vectors_wrapped = FALSE;
}

static void
FreeRec(ScrnInfoPtr pScrn)
{
    modesettingPtr ms;

    if (!pScrn)
        return;

    ms = modesettingPTR(pScrn);
    if (!ms)
        return;

    if (ms->fd > 0) {
        modesettingEntPtr ms_ent;
        int ret;

        ms_ent = ms_ent_priv(pScrn);
        ms_ent->fd_ref--;
        if (!ms_ent->fd_ref) {
            ms_unwrap_property_requests(pScrn);
            if (ms->pEnt->location.type == BUS_PCI)
                ret = drmClose(ms->fd);
            else
#ifdef XF86_PDEV_SERVER_FD
                if (!(ms->pEnt->location.type == BUS_PLATFORM &&
                      (ms->pEnt->location.id.plat->flags & XF86_PDEV_SERVER_FD)))
#endif
                    ret = close(ms->fd);
            (void) ret;
            ms_ent->fd = 0;
        }
    }
    pScrn->driverPrivate = NULL;
    free(ms->drmmode.Options);
    free(ms);

}

#ifdef GLAMOR_HAS_GBM

static Bool
load_glamor(ScrnInfoPtr pScrn)
{
    void *mod = xf86LoadSubModule(pScrn, GLAMOR_EGL_MODULE_NAME);
    modesettingPtr ms = modesettingPTR(pScrn);

    if (!mod)
        return FALSE;

    ms->glamor.back_pixmap_from_fd = LoaderSymbolFromModule(mod, "glamor_back_pixmap_from_fd");
    ms->glamor.block_handler = LoaderSymbolFromModule(mod, "glamor_block_handler");
    ms->glamor.clear_pixmap = LoaderSymbolFromModule(mod, "glamor_clear_pixmap");
    ms->glamor.egl_create_textured_pixmap = LoaderSymbolFromModule(mod, "glamor_egl_create_textured_pixmap");
    ms->glamor.egl_create_textured_pixmap_from_gbm_bo = LoaderSymbolFromModule(mod, "glamor_egl_create_textured_pixmap_from_gbm_bo");
    ms->glamor.egl_exchange_buffers = LoaderSymbolFromModule(mod, "glamor_egl_exchange_buffers");
    ms->glamor.egl_get_gbm_device = LoaderSymbolFromModule(mod, "glamor_egl_get_gbm_device");
    ms->glamor.egl_init = LoaderSymbolFromModule(mod, "glamor_egl_init");
    ms->glamor.finish = LoaderSymbolFromModule(mod, "glamor_finish");
    ms->glamor.gbm_bo_from_pixmap = LoaderSymbolFromModule(mod, "glamor_gbm_bo_from_pixmap");
    ms->glamor.init = LoaderSymbolFromModule(mod, "glamor_init");
    ms->glamor.name_from_pixmap = LoaderSymbolFromModule(mod, "glamor_name_from_pixmap");
    ms->glamor.set_drawable_modifiers_func = LoaderSymbolFromModule(mod, "glamor_set_drawable_modifiers_func");
    ms->glamor.shareable_fd_from_pixmap = LoaderSymbolFromModule(mod, "glamor_shareable_fd_from_pixmap");
    ms->glamor.supports_pixmap_import_export = LoaderSymbolFromModule(mod, "glamor_supports_pixmap_import_export");
    ms->glamor.xv_init = LoaderSymbolFromModule(mod, "glamor_xv_init");
    ms->glamor.egl_get_driver_name = LoaderSymbolFromModule(mod, "glamor_egl_get_driver_name");

    return TRUE;
}

#endif

static void
try_enable_glamor(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    const char *accel_method_str = xf86GetOptValString(ms->drmmode.Options,
                                                       OPTION_ACCEL_METHOD);
    Bool do_glamor = (!accel_method_str ||
                      strcmp(accel_method_str, "glamor") == 0);

    ms->drmmode.glamor = FALSE;

#ifdef GLAMOR_HAS_GBM
    if (ms->drmmode.force_24_32) {
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "Cannot use glamor with 24bpp packed fb\n");
        return;
    }

    if (!do_glamor) {
        xf86DrvMsg(pScrn->scrnIndex, X_CONFIG, "glamor disabled\n");
        return;
    }

    if (load_glamor(pScrn)) {
        if (ms->glamor.egl_init(pScrn, ms->fd)) {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO, "glamor initialized\n");
            ms->drmmode.glamor = TRUE;
        } else {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                       "glamor initialization failed\n");
        }
    } else {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Failed to load glamor module.\n");
    }
#else
    if (do_glamor) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "No glamor support in the X Server\n");
    }
#endif
}

static Bool
msShouldDoubleShadow(ScrnInfoPtr pScrn, modesettingPtr ms)
{
    Bool ret = FALSE, asked;
    int from;
    drmVersionPtr v = drmGetVersion(ms->fd);

    if (!ms->drmmode.shadow_enable)
        return FALSE;

    if (!strcmp(v->name, "mgag200") ||
        !strcmp(v->name, "ast")) /* XXX || rn50 */
        ret = TRUE;

    drmFreeVersion(v);

    asked = xf86GetOptValBool(ms->drmmode.Options, OPTION_DOUBLE_SHADOW, &ret);

    if (asked)
        from = X_CONFIG;
    else
        from = X_INFO;

    xf86DrvMsg(pScrn->scrnIndex, from,
               "Double-buffered shadow updates: %s\n", ret ? "on" : "off");

    return ret;
}

static Bool
ms_get_drm_master_fd(ScrnInfoPtr pScrn)
{
    EntityInfoPtr pEnt;
    modesettingPtr ms;
    modesettingEntPtr ms_ent;

    ms = modesettingPTR(pScrn);
    ms_ent = ms_ent_priv(pScrn);

    pEnt = ms->pEnt;

    if (ms_ent->fd) {
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   " reusing fd for second head\n");
        ms->fd = ms_ent->fd;
        ms_ent->fd_ref++;
        return TRUE;
    }

    ms->fd_passed = FALSE;
    if ((ms->fd = get_passed_fd()) >= 0) {
        ms->fd_passed = TRUE;
        return TRUE;
    }

#ifdef XSERVER_PLATFORM_BUS
    if (pEnt->location.type == BUS_PLATFORM) {
#ifdef XF86_PDEV_SERVER_FD
        if (pEnt->location.id.plat->flags & XF86_PDEV_SERVER_FD)
            ms->fd =
                xf86_platform_device_odev_attributes(pEnt->location.id.plat)->
                fd;
        else
#endif
        {
            char *path =
                xf86_platform_device_odev_attributes(pEnt->location.id.plat)->
                path;
            ms->fd = open_hw(path);
        }
    }
    else
#endif
#ifdef XSERVER_LIBPCIACCESS
    if (pEnt->location.type == BUS_PCI) {
        char *BusID = NULL;
        struct pci_device *PciInfo;

        PciInfo = xf86GetPciInfoForEntity(ms->pEnt->index);
        if (PciInfo) {
            if ((BusID = ms_DRICreatePCIBusID(PciInfo)) != NULL) {
                ms->fd = drmOpen(NULL, BusID);
                free(BusID);
            }
        }
    }
    else
#endif
    {
        const char *devicename;
        devicename = xf86FindOptionValue(ms->pEnt->device->options, "kmsdev");
        ms->fd = open_hw(devicename);
    }
    if (ms->fd < 0)
        return FALSE;

    ms_ent->fd = ms->fd;
    ms_ent->fd_ref = 1;
    return TRUE;
}

static Bool
PreInit(ScrnInfoPtr pScrn, int flags)
{
    modesettingPtr ms;
    rgb defaultWeight = { 0, 0, 0 };
    EntityInfoPtr pEnt;
    uint64_t value = 0;
    int ret;
    int bppflags, connector_count;
    int defaultdepth, defaultbpp;

    if (pScrn->numEntities != 1)
        return FALSE;

    if (flags & PROBE_DETECT) {
        return FALSE;
    }

    /* Allocate driverPrivate */
    if (!GetRec(pScrn))
        return FALSE;

    pEnt = xf86GetEntityInfo(pScrn->entityList[0]);

    ms = modesettingPTR(pScrn);
    ms->SaveGeneration = -1;
    ms->pEnt = pEnt;
    ms->drmmode.is_secondary = FALSE;
    pScrn->displayWidth = 640;  /* default it */

    if (xf86IsEntityShared(pScrn->entityList[0])) {
        if (xf86IsPrimInitDone(pScrn->entityList[0]))
            ms->drmmode.is_secondary = TRUE;
        else
            xf86SetPrimInitDone(pScrn->entityList[0]);
    }

    pScrn->monitor = pScrn->confScreen->monitor;
    pScrn->progClock = TRUE;
    pScrn->rgbBits = 8;

    if (!ms_get_drm_master_fd(pScrn))
        return FALSE;
    ms->drmmode.fd = ms->fd;

    if (!check_outputs(ms->fd, &connector_count))
        return FALSE;

    drmmode_get_default_bpp(pScrn, &ms->drmmode, &defaultdepth, &defaultbpp);
    if (defaultdepth == 24 && defaultbpp == 24) {
        ms->drmmode.force_24_32 = TRUE;
        ms->drmmode.kbpp = 24;
        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "Using 24bpp hw front buffer with 32bpp shadow\n");
        defaultbpp = 32;
    } else {
        ms->drmmode.kbpp = 0;
    }
    bppflags = PreferConvert24to32 | SupportConvert24to32 | Support32bppFb;

    if (!xf86SetDepthBpp
        (pScrn, defaultdepth, defaultdepth, defaultbpp, bppflags))
        return FALSE;

    switch (pScrn->depth) {
    case 15:
    case 16:
    case 24:
    case 30:
        break;
    default:
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Given depth (%d) is not supported by the driver\n",
                   pScrn->depth);
        return FALSE;
    }
    xf86PrintDepthBpp(pScrn);
    if (!ms->drmmode.kbpp)
        ms->drmmode.kbpp = pScrn->bitsPerPixel;

    /* Process the options */
    xf86CollectOptions(pScrn, NULL);
    if (!(ms->drmmode.Options = malloc(sizeof(Options))))
        return FALSE;
    memcpy(ms->drmmode.Options, Options, sizeof(Options));
    xf86ProcessOptions(pScrn->scrnIndex, pScrn->options, ms->drmmode.Options);

    if (!xf86SetWeight(pScrn, defaultWeight, defaultWeight))
        return FALSE;
    if (!xf86SetDefaultVisual(pScrn, -1))
        return FALSE;

    if (xf86ReturnOptValBool(ms->drmmode.Options, OPTION_SW_CURSOR, FALSE)) {
        ms->drmmode.sw_cursor = TRUE;
    }

    ms->cursor_width = 64;
    ms->cursor_height = 64;
    ret = drmGetCap(ms->fd, DRM_CAP_CURSOR_WIDTH, &value);
    if (!ret) {
        ms->cursor_width = value;
    }
    ret = drmGetCap(ms->fd, DRM_CAP_CURSOR_HEIGHT, &value);
    if (!ret) {
        ms->cursor_height = value;
    }

    try_enable_glamor(pScrn);

    if (!ms->drmmode.glamor) {
        Bool prefer_shadow = TRUE;

        if (ms->drmmode.force_24_32) {
            prefer_shadow = TRUE;
            ms->drmmode.shadow_enable = TRUE;
        } else {
            ret = drmGetCap(ms->fd, DRM_CAP_DUMB_PREFER_SHADOW, &value);
            if (!ret) {
                prefer_shadow = !!value;
            }

            ms->drmmode.shadow_enable =
                xf86ReturnOptValBool(ms->drmmode.Options, OPTION_SHADOW_FB,
                                     prefer_shadow);
        }

        xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                   "ShadowFB: preferred %s, enabled %s\n",
                   prefer_shadow ? "YES" : "NO",
                   ms->drmmode.force_24_32 ? "FORCE" :
                   ms->drmmode.shadow_enable ? "YES" : "NO");

        ms->drmmode.shadow_enable2 = msShouldDoubleShadow(pScrn, ms);
    } else {
        if (!pScrn->is_gpu) {
            MessageType from = xf86GetOptValBool(ms->drmmode.Options, OPTION_VARIABLE_REFRESH,
                                                 &ms->vrr_support) ? X_CONFIG : X_DEFAULT;
            xf86DrvMsg(pScrn->scrnIndex, from, "VariableRefresh: %sabled\n",
                       ms->vrr_support ? "en" : "dis");

            ms->drmmode.async_flip_secondaries = FALSE;
            from = xf86GetOptValBool(ms->drmmode.Options, OPTION_ASYNC_FLIP_SECONDARIES,
                                     &ms->drmmode.async_flip_secondaries) ? X_CONFIG : X_DEFAULT;
            xf86DrvMsg(pScrn->scrnIndex, from, "AsyncFlipSecondaries: %sabled\n",
                       ms->drmmode.async_flip_secondaries ? "en" : "dis");
        }
    }

    ms->drmmode.pageflip =
        xf86ReturnOptValBool(ms->drmmode.Options, OPTION_PAGEFLIP, TRUE);

    pScrn->capabilities = 0;
    ret = drmGetCap(ms->fd, DRM_CAP_PRIME, &value);
    if (ret == 0) {
        if (connector_count && (value & DRM_PRIME_CAP_IMPORT)) {
            pScrn->capabilities |= RR_Capability_SinkOutput;
            if (ms->drmmode.glamor)
                pScrn->capabilities |= RR_Capability_SinkOffload;
        }
#ifdef GLAMOR_HAS_GBM_LINEAR
        if (value & DRM_PRIME_CAP_EXPORT && ms->drmmode.glamor)
            pScrn->capabilities |= RR_Capability_SourceOutput | RR_Capability_SourceOffload;
#endif
    }

    /*
     * Use "atomic modesetting disable" request to detect if the kms driver is
     * atomic capable, regardless if we will actually use atomic modesetting.
     * This is effectively a no-op, we only care about the return status code.
     */
    ret = drmSetClientCap(ms->fd, DRM_CLIENT_CAP_ATOMIC, 0);
    ms->atomic_modeset_capable = (ret == 0);

    if (xf86ReturnOptValBool(ms->drmmode.Options, OPTION_ATOMIC, FALSE)) {
        ret = drmSetClientCap(ms->fd, DRM_CLIENT_CAP_ATOMIC, 1);
        ms->atomic_modeset = (ret == 0);
    } else {
        ms->atomic_modeset = FALSE;
    }

    ms->kms_has_modifiers = FALSE;
    ret = drmGetCap(ms->fd, DRM_CAP_ADDFB2_MODIFIERS, &value);
    if (ret == 0 && value != 0)
        ms->kms_has_modifiers = TRUE;

    if (drmmode_pre_init(pScrn, &ms->drmmode, pScrn->bitsPerPixel / 8) == FALSE) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "KMS setup failed\n");
        goto fail;
    }

    /*
     * If the driver can do gamma correction, it should call xf86SetGamma() here.
     */
    {
        Gamma zeros = { 0.0, 0.0, 0.0 };

        if (!xf86SetGamma(pScrn, zeros)) {
            return FALSE;
        }
    }

    if (!(pScrn->is_gpu && connector_count == 0) && pScrn->modes == NULL) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "No modes.\n");
        return FALSE;
    }

    pScrn->currentMode = pScrn->modes;

    /* Set display resolution */
    xf86SetDpi(pScrn, 0, 0);

    /* Load the required sub modules */
    if (!xf86LoadSubModule(pScrn, "fb")) {
        return FALSE;
    }

    if (ms->drmmode.shadow_enable) {
        void *mod = xf86LoadSubModule(pScrn, "shadow");

        if (!mod)
            return FALSE;

        ms->shadow.Setup        = LoaderSymbolFromModule(mod, "shadowSetup");
        ms->shadow.Add          = LoaderSymbolFromModule(mod, "shadowAdd");
        ms->shadow.Remove       = LoaderSymbolFromModule(mod, "shadowRemove");
        ms->shadow.Update32to24 = LoaderSymbolFromModule(mod, "shadowUpdate32to24");
        ms->shadow.UpdatePacked = LoaderSymbolFromModule(mod, "shadowUpdatePacked");
    }

    return TRUE;
 fail:
    return FALSE;
}

static void *
msShadowWindow(ScreenPtr screen, CARD32 row, CARD32 offset, int mode,
               CARD32 *size, void *closure)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(pScrn);
    int stride;

    stride = (pScrn->displayWidth * ms->drmmode.kbpp) / 8;
    *size = stride;

    return ((uint8_t *) ms->drmmode.front_bo.dumb->ptr + row * stride + offset);
}

/* somewhat arbitrary tile size, in pixels */
#define TILE 16

static int
msUpdateIntersect(modesettingPtr ms, shadowBufPtr pBuf, BoxPtr box,
                  xRectangle *prect)
{
    int i, dirty = 0, stride = pBuf->pPixmap->devKind, cpp = ms->drmmode.cpp;
    int width = (box->x2 - box->x1) * cpp;
    unsigned char *old, *new;

    old = ms->drmmode.shadow_fb2;
    old += (box->y1 * stride) + (box->x1 * cpp);
    new = ms->drmmode.shadow_fb;
    new += (box->y1 * stride) + (box->x1 * cpp);

    for (i = box->y2 - box->y1 - 1; i >= 0; i--) {
        unsigned char *o = old + i * stride,
                      *n = new + i * stride;
        if (memcmp(o, n, width) != 0) {
            dirty = 1;
            memcpy(o, n, width);
        }
    }

    if (dirty) {
        prect->x = box->x1;
        prect->y = box->y1;
        prect->width = box->x2 - box->x1;
        prect->height = box->y2 - box->y1;
    }

    return dirty;
}

static void
msUpdatePacked(ScreenPtr pScreen, shadowBufPtr pBuf)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    Bool use_3224 = ms->drmmode.force_24_32 && pScrn->bitsPerPixel == 32;

    if (ms->drmmode.shadow_enable2 && ms->drmmode.shadow_fb2) do {
        RegionPtr damage = DamageRegion(pBuf->pDamage), tiles;
        BoxPtr extents = RegionExtents(damage);
        xRectangle *prect;
        int nrects;
        int i, j, tx1, tx2, ty1, ty2;

        tx1 = extents->x1 / TILE;
        tx2 = (extents->x2 + TILE - 1) / TILE;
        ty1 = extents->y1 / TILE;
        ty2 = (extents->y2 + TILE - 1) / TILE;

        nrects = (tx2 - tx1) * (ty2 - ty1);
        if (!(prect = calloc(nrects, sizeof(xRectangle))))
            break;

        nrects = 0;
        for (j = ty2 - 1; j >= ty1; j--) {
            for (i = tx2 - 1; i >= tx1; i--) {
                BoxRec box;

                box.x1 = max(i * TILE, extents->x1);
                box.y1 = max(j * TILE, extents->y1);
                box.x2 = min((i+1) * TILE, extents->x2);
                box.y2 = min((j+1) * TILE, extents->y2);

                if (RegionContainsRect(damage, &box) != rgnOUT) {
                    if (msUpdateIntersect(ms, pBuf, &box, prect + nrects)) {
                        nrects++;
                    }
                }
            }
        }

        tiles = RegionFromRects(nrects, prect, CT_NONE);
        RegionIntersect(damage, damage, tiles);
        RegionDestroy(tiles);
        free(prect);
    } while (0);

    if (use_3224)
        ms->shadow.Update32to24(pScreen, pBuf);
    else
        ms->shadow.UpdatePacked(pScreen, pBuf);
}

static Bool
msEnableSharedPixmapFlipping(RRCrtcPtr crtc, PixmapPtr front, PixmapPtr back)
{
    ScreenPtr screen = crtc->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    EntityInfoPtr pEnt = ms->pEnt;
    xf86CrtcPtr xf86Crtc = crtc->devPrivate;

    if (!xf86Crtc)
        return FALSE;

    /* Not supported if we can't flip */
    if (!ms->drmmode.pageflip)
        return FALSE;

    /* Not currently supported with reverse PRIME */
    if (ms->drmmode.reverse_prime_offload_mode)
        return FALSE;

#ifdef XSERVER_PLATFORM_BUS
    if (pEnt->location.type == BUS_PLATFORM) {
        char *syspath =
            xf86_platform_device_odev_attributes(pEnt->location.id.plat)->
            syspath;

        /* Not supported for devices using USB transport due to misbehaved
         * vblank events */
        if (syspath && strstr(syspath, "usb"))
            return FALSE;

        /* EVDI uses USB transport but is platform device, not usb.
         * Exclude it explicitly. */
        if (syspath && strstr(syspath, "evdi"))
            return FALSE;
    }
#endif

    return drmmode_EnableSharedPixmapFlipping(xf86Crtc, &ms->drmmode,
                                              front, back);
}

static void
msDisableSharedPixmapFlipping(RRCrtcPtr crtc)
{
    ScreenPtr screen = crtc->pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    xf86CrtcPtr xf86Crtc = crtc->devPrivate;

    if (xf86Crtc)
        drmmode_DisableSharedPixmapFlipping(xf86Crtc, &ms->drmmode);
}

static Bool
msStartFlippingPixmapTracking(RRCrtcPtr crtc, DrawablePtr src,
                              PixmapPtr secondary_dst1, PixmapPtr secondary_dst2,
                              int x, int y, int dst_x, int dst_y,
                              Rotation rotation)
{
    ScreenPtr pScreen = src->pScreen;
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(pScreen));

    msPixmapPrivPtr ppriv1 = msGetPixmapPriv(&ms->drmmode, secondary_dst1->primary_pixmap),
                    ppriv2 = msGetPixmapPriv(&ms->drmmode, secondary_dst2->primary_pixmap);

    if (!PixmapStartDirtyTracking(src, secondary_dst1, x, y,
                                  dst_x, dst_y, rotation)) {
        return FALSE;
    }

    if (!PixmapStartDirtyTracking(src, secondary_dst2, x, y,
                                  dst_x, dst_y, rotation)) {
        PixmapStopDirtyTracking(src, secondary_dst1);
        return FALSE;
    }

    ppriv1->secondary_src = src;
    ppriv2->secondary_src = src;

    ppriv1->dirty = ms_dirty_get_ent(pScreen, secondary_dst1);
    ppriv2->dirty = ms_dirty_get_ent(pScreen, secondary_dst2);

    ppriv1->defer_dirty_update = TRUE;
    ppriv2->defer_dirty_update = TRUE;

    return TRUE;
}

static Bool
msPresentSharedPixmap(PixmapPtr secondary_dst)
{
    ScreenPtr pScreen = secondary_dst->primary_pixmap->drawable.pScreen;
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(pScreen));

    msPixmapPrivPtr ppriv = msGetPixmapPriv(&ms->drmmode, secondary_dst->primary_pixmap);

    RegionPtr region = DamageRegion(ppriv->dirty->damage);

    if (RegionNotEmpty(region)) {
        redisplay_dirty(ppriv->secondary_src->pScreen, ppriv->dirty, NULL);
        DamageEmpty(ppriv->dirty->damage);

        return TRUE;
    }

    return FALSE;
}

static Bool
msStopFlippingPixmapTracking(DrawablePtr src,
                             PixmapPtr secondary_dst1, PixmapPtr secondary_dst2)
{
    ScreenPtr pScreen = src->pScreen;
    modesettingPtr ms = modesettingPTR(xf86ScreenToScrn(pScreen));

    msPixmapPrivPtr ppriv1 = msGetPixmapPriv(&ms->drmmode, secondary_dst1->primary_pixmap),
                    ppriv2 = msGetPixmapPriv(&ms->drmmode, secondary_dst2->primary_pixmap);

    Bool ret = TRUE;

    ret &= PixmapStopDirtyTracking(src, secondary_dst1);
    ret &= PixmapStopDirtyTracking(src, secondary_dst2);

    if (ret) {
        ppriv1->secondary_src = NULL;
        ppriv2->secondary_src = NULL;

        ppriv1->dirty = NULL;
        ppriv2->dirty = NULL;

        ppriv1->defer_dirty_update = FALSE;
        ppriv2->defer_dirty_update = FALSE;
    }

    return ret;
}

static Bool
CreateScreenResources(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    PixmapPtr rootPixmap;
    Bool ret;
    void *pixels = NULL;
    int err;

    pScreen->CreateScreenResources = ms->createScreenResources;
    ret = pScreen->CreateScreenResources(pScreen);
    pScreen->CreateScreenResources = CreateScreenResources;

    if (!drmmode_set_desired_modes(pScrn, &ms->drmmode, pScrn->is_gpu, FALSE))
        return FALSE;

    if (!drmmode_glamor_handle_new_screen_pixmap(&ms->drmmode))
        return FALSE;

    drmmode_uevent_init(pScrn, &ms->drmmode);

    if (!ms->drmmode.sw_cursor)
        drmmode_map_cursor_bos(pScrn, &ms->drmmode);

    if (!ms->drmmode.gbm) {
        pixels = drmmode_map_front_bo(&ms->drmmode);
        if (!pixels)
            return FALSE;
    }

    rootPixmap = pScreen->GetScreenPixmap(pScreen);

    if (ms->drmmode.shadow_enable)
        pixels = ms->drmmode.shadow_fb;

    if (ms->drmmode.shadow_enable2) {
        ms->drmmode.shadow_fb2 = calloc(1, pScrn->displayWidth * pScrn->virtualY * ((pScrn->bitsPerPixel + 7) >> 3));
        if (!ms->drmmode.shadow_fb2)
            ms->drmmode.shadow_enable2 = FALSE;
    }

    if (!pScreen->ModifyPixmapHeader(rootPixmap, -1, -1, -1, -1, -1, pixels))
        FatalError("Couldn't adjust screen pixmap\n");

    if (ms->drmmode.shadow_enable) {
        if (!ms->shadow.Add(pScreen, rootPixmap, msUpdatePacked, msShadowWindow,
                            0, 0))
            return FALSE;
    }

    err = drmModeDirtyFB(ms->fd, ms->drmmode.fb_id, NULL, 0);

    if (err != -EINVAL && err != -ENOSYS) {
        ms->damage = DamageCreate(NULL, NULL, DamageReportNone, TRUE,
                                  pScreen, rootPixmap);

        if (ms->damage) {
            DamageRegister(&rootPixmap->drawable, ms->damage);
            ms->dirty_enabled = TRUE;
            xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Damage tracking initialized\n");
        }
        else {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "Failed to create screen damage record\n");
            return FALSE;
        }
    }

    if (dixPrivateKeyRegistered(rrPrivKey)) {
        rrScrPrivPtr pScrPriv = rrGetScrPriv(pScreen);

        pScrPriv->rrEnableSharedPixmapFlipping = msEnableSharedPixmapFlipping;
        pScrPriv->rrDisableSharedPixmapFlipping = msDisableSharedPixmapFlipping;

        pScrPriv->rrStartFlippingPixmapTracking = msStartFlippingPixmapTracking;
    }

    if (ms->vrr_support &&
        !dixRegisterPrivateKey(&ms->drmmode.vrrPrivateKeyRec,
                               PRIVATE_WINDOW,
                               sizeof(struct ms_vrr_priv)))
            return FALSE;

    return ret;
}

static Bool
msSharePixmapBacking(PixmapPtr ppix, ScreenPtr secondary, void **handle)
{
#ifdef GLAMOR_HAS_GBM
    modesettingPtr ms =
        modesettingPTR(xf86ScreenToScrn(ppix->drawable.pScreen));
    int ret;
    CARD16 stride;
    CARD32 size;
    ret = ms->glamor.shareable_fd_from_pixmap(ppix->drawable.pScreen, ppix,
                                              &stride, &size);
    if (ret == -1)
        return FALSE;

    *handle = (void *)(long)(ret);
    return TRUE;
#endif
    return FALSE;
}

static Bool
msSetSharedPixmapBacking(PixmapPtr ppix, void *fd_handle)
{
#ifdef GLAMOR_HAS_GBM
    ScreenPtr screen = ppix->drawable.pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    Bool ret;
    int ihandle = (int) (long) fd_handle;

    if (ihandle == -1)
        if (!ms->drmmode.reverse_prime_offload_mode)
           return drmmode_SetSlaveBO(ppix, &ms->drmmode, ihandle, 0, 0);

    if (ms->drmmode.reverse_prime_offload_mode) {
        ret = ms->glamor.back_pixmap_from_fd(ppix, ihandle,
                                             ppix->drawable.width,
                                             ppix->drawable.height,
                                             ppix->devKind,
                                             ppix->drawable.depth,
                                             ppix->drawable.bitsPerPixel);
    } else {
        int size = ppix->devKind * ppix->drawable.height;
        ret = drmmode_SetSlaveBO(ppix, &ms->drmmode, ihandle, ppix->devKind, size);
    }
    if (ret == FALSE)
        return ret;

    return TRUE;
#else
    return FALSE;
#endif
}

static Bool
msRequestSharedPixmapNotifyDamage(PixmapPtr ppix)
{
    ScreenPtr screen = ppix->drawable.pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);

    msPixmapPrivPtr ppriv = msGetPixmapPriv(&ms->drmmode, ppix->primary_pixmap);

    ppriv->notify_on_damage = TRUE;

    return TRUE;
}

static Bool
msSharedPixmapNotifyDamage(PixmapPtr ppix)
{
    Bool ret = FALSE;
    int c;

    ScreenPtr screen = ppix->drawable.pScreen;
    ScrnInfoPtr scrn = xf86ScreenToScrn(screen);
    modesettingPtr ms = modesettingPTR(scrn);
    xf86CrtcConfigPtr xf86_config = XF86_CRTC_CONFIG_PTR(scrn);

    msPixmapPrivPtr ppriv = msGetPixmapPriv(&ms->drmmode, ppix);

    if (!ppriv->wait_for_damage)
        return ret;
    ppriv->wait_for_damage = FALSE;

    for (c = 0; c < xf86_config->num_crtc; c++) {
        xf86CrtcPtr crtc = xf86_config->crtc[c];
        drmmode_crtc_private_ptr drmmode_crtc = crtc->driver_private;

        if (!drmmode_crtc)
            continue;
        if (!(drmmode_crtc->prime_pixmap && drmmode_crtc->prime_pixmap_back))
            continue;

        // Received damage on primary screen pixmap, schedule present on vblank
        ret |= drmmode_SharedPixmapPresentOnVBlank(ppix, crtc, &ms->drmmode);
    }

    return ret;
}

static Bool
SetMaster(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);
    int ret;

#ifdef XF86_PDEV_SERVER_FD
    if (ms->pEnt->location.type == BUS_PLATFORM &&
        (ms->pEnt->location.id.plat->flags & XF86_PDEV_SERVER_FD))
        return TRUE;
#endif

    if (ms->fd_passed)
        return TRUE;

    ret = drmSetMaster(ms->fd);
    if (ret)
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "drmSetMaster failed: %s\n",
                   strerror(errno));

    return ret == 0;
}

/* When the root window is created, initialize the screen contents from
 * console if -background none was specified on the command line
 */
static Bool
CreateWindow_oneshot(WindowPtr pWin)
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    Bool ret;

    pScreen->CreateWindow = ms->CreateWindow;
    ret = pScreen->CreateWindow(pWin);

    if (ret)
        drmmode_copy_fb(pScrn, &ms->drmmode);
    return ret;
}

static Bool
ScreenInit(ScreenPtr pScreen, int argc, char **argv)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    VisualPtr visual;

    pScrn->pScreen = pScreen;

    if (!SetMaster(pScrn))
        return FALSE;

#ifdef GLAMOR_HAS_GBM
    if (ms->drmmode.glamor)
        ms->drmmode.gbm = ms->glamor.egl_get_gbm_device(pScreen);
#endif

    /* HW dependent - FIXME */
    pScrn->displayWidth = pScrn->virtualX;
    if (!drmmode_create_initial_bos(pScrn, &ms->drmmode))
        return FALSE;

    if (ms->drmmode.shadow_enable) {
        ms->drmmode.shadow_fb =
            calloc(1,
                   pScrn->displayWidth * pScrn->virtualY *
                   ((pScrn->bitsPerPixel + 7) >> 3));
        if (!ms->drmmode.shadow_fb)
            ms->drmmode.shadow_enable = FALSE;
    }

    miClearVisualTypes();

    if (!miSetVisualTypes(pScrn->depth,
                          miGetDefaultVisualMask(pScrn->depth),
                          pScrn->rgbBits, pScrn->defaultVisual))
        return FALSE;

    if (!miSetPixmapDepths())
        return FALSE;

    if (!dixRegisterScreenSpecificPrivateKey
        (pScreen, &ms->drmmode.pixmapPrivateKeyRec, PRIVATE_PIXMAP,
         sizeof(msPixmapPrivRec))) {
        return FALSE;
    }

    pScrn->memPhysBase = 0;
    pScrn->fbOffset = 0;

    if (!fbScreenInit(pScreen, NULL,
                      pScrn->virtualX, pScrn->virtualY,
                      pScrn->xDpi, pScrn->yDpi,
                      pScrn->displayWidth, pScrn->bitsPerPixel))
        return FALSE;

    if (pScrn->bitsPerPixel > 8) {
        /* Fixup RGB ordering */
        visual = pScreen->visuals + pScreen->numVisuals;
        while (--visual >= pScreen->visuals) {
            if ((visual->class | DynamicClass) == DirectColor) {
                visual->offsetRed = pScrn->offset.red;
                visual->offsetGreen = pScrn->offset.green;
                visual->offsetBlue = pScrn->offset.blue;
                visual->redMask = pScrn->mask.red;
                visual->greenMask = pScrn->mask.green;
                visual->blueMask = pScrn->mask.blue;
            }
        }
    }

    fbPictureInit(pScreen, NULL, 0);

    if (drmmode_init(pScrn, &ms->drmmode) == FALSE) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Failed to initialize glamor at ScreenInit() time.\n");
        return FALSE;
    }

    if (ms->drmmode.shadow_enable && !ms->shadow.Setup(pScreen)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "shadow fb init failed\n");
        return FALSE;
    }

    ms->createScreenResources = pScreen->CreateScreenResources;
    pScreen->CreateScreenResources = CreateScreenResources;

    xf86SetBlackWhitePixels(pScreen);

    xf86SetBackingStore(pScreen);
    xf86SetSilkenMouse(pScreen);
    miDCInitialize(pScreen, xf86GetPointerScreenFuncs());

    /* If pageflip is enabled hook the screen's cursor-sprite (swcursor) funcs.
     * So that we can disable page-flipping on fallback to a swcursor. */
    if (ms->drmmode.pageflip) {
        miPointerScreenPtr PointPriv =
            dixLookupPrivate(&pScreen->devPrivates, miPointerScreenKey);

        if (!dixRegisterScreenPrivateKey(&ms->drmmode.spritePrivateKeyRec,
                                         pScreen, PRIVATE_DEVICE,
                                         sizeof(msSpritePrivRec)))
            return FALSE;

        ms->SpriteFuncs = PointPriv->spriteFuncs;
        PointPriv->spriteFuncs = &drmmode_sprite_funcs;
    }

    /* Need to extend HWcursor support to handle mask interleave */
    if (!ms->drmmode.sw_cursor)
        xf86_cursors_init(pScreen, ms->cursor_width, ms->cursor_height,
                          HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64 |
                          HARDWARE_CURSOR_UPDATE_UNHIDDEN |
                          HARDWARE_CURSOR_ARGB);

    /* Must force it before EnterVT, so we are in control of VT and
     * later memory should be bound when allocating, e.g rotate_mem */
    pScrn->vtSema = TRUE;

    if (serverGeneration == 1 && bgNoneRoot && ms->drmmode.glamor) {
        ms->CreateWindow = pScreen->CreateWindow;
        pScreen->CreateWindow = CreateWindow_oneshot;
    }

    pScreen->SaveScreen = xf86SaveScreen;
    ms->CloseScreen = pScreen->CloseScreen;
    pScreen->CloseScreen = CloseScreen;

    ms->BlockHandler = pScreen->BlockHandler;
    pScreen->BlockHandler = msBlockHandler_oneshot;

    pScreen->SharePixmapBacking = msSharePixmapBacking;
    pScreen->SetSharedPixmapBacking = msSetSharedPixmapBacking;
    pScreen->StartPixmapTracking = PixmapStartDirtyTracking;
    pScreen->StopPixmapTracking = PixmapStopDirtyTracking;

    pScreen->SharedPixmapNotifyDamage = msSharedPixmapNotifyDamage;
    pScreen->RequestSharedPixmapNotifyDamage =
        msRequestSharedPixmapNotifyDamage;

    pScreen->PresentSharedPixmap = msPresentSharedPixmap;
    pScreen->StopFlippingPixmapTracking = msStopFlippingPixmapTracking;

    if (!xf86CrtcScreenInit(pScreen))
        return FALSE;

    if (!drmmode_setup_colormap(pScreen, pScrn))
        return FALSE;

    if (ms->atomic_modeset)
        xf86DPMSInit(pScreen, drmmode_set_dpms, 0);
    else
        xf86DPMSInit(pScreen, xf86DPMSSet, 0);

#ifdef GLAMOR_HAS_GBM
    if (ms->drmmode.glamor) {
        XF86VideoAdaptorPtr     glamor_adaptor;

        glamor_adaptor = ms->glamor.xv_init(pScreen, 16);
        if (glamor_adaptor != NULL)
            xf86XVScreenInit(pScreen, &glamor_adaptor, 1);
        else
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "Failed to initialize XV support.\n");
    }
#endif

    if (serverGeneration == 1)
        xf86ShowUnusedOptions(pScrn->scrnIndex, pScrn->options);

    if (!ms_vblank_screen_init(pScreen)) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Failed to initialize vblank support.\n");
        return FALSE;
    }

#ifdef GLAMOR_HAS_GBM
    if (ms->drmmode.glamor) {
        if (!(ms->drmmode.dri2_enable = ms_dri2_screen_init(pScreen))) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "Failed to initialize the DRI2 extension.\n");
        }

        /* enable reverse prime if we are a GPU screen, and accelerated, and not
         * i915, evdi or udl. i915 is happy scanning out from sysmem.
         * evdi and udl are virtual drivers scanning out from sysmem
         * backed dumb buffers.
         */
        if (pScreen->isGPU) {
            drmVersionPtr version;

            /* enable if we are an accelerated GPU screen */
            ms->drmmode.reverse_prime_offload_mode = TRUE;

            if ((version = drmGetVersion(ms->drmmode.fd))) {
                if (!strncmp("i915", version->name, version->name_len)) {
                    ms->drmmode.reverse_prime_offload_mode = FALSE;
                }
                if (!strncmp("evdi", version->name, version->name_len)) {
                    ms->drmmode.reverse_prime_offload_mode = FALSE;
                }
                if (!strncmp("udl", version->name, version->name_len)) {
                    ms->drmmode.reverse_prime_offload_mode = FALSE;
                }
                if (!ms->drmmode.reverse_prime_offload_mode) {
                    xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                       "Disable reverse prime offload mode for %s.\n", version->name);
                }
                drmFreeVersion(version);
            }
        }
    }
#endif
    if (!(ms->drmmode.present_enable = ms_present_screen_init(pScreen))) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Failed to initialize the Present extension.\n");
    }


    pScrn->vtSema = TRUE;

    if (ms->vrr_support) {
        if (!property_vectors_wrapped) {
            saved_change_property = ProcVector[X_ChangeProperty];
            ProcVector[X_ChangeProperty] = ms_change_property;
            saved_delete_property = ProcVector[X_DeleteProperty];
            ProcVector[X_DeleteProperty] = ms_delete_property;
            property_vectors_wrapped = TRUE;
        }
        vrr_atom = MakeAtom("_VARIABLE_REFRESH",
                             strlen("_VARIABLE_REFRESH"), TRUE);
    }

    return TRUE;
}

static void
AdjustFrame(ScrnInfoPtr pScrn, int x, int y)
{
    modesettingPtr ms = modesettingPTR(pScrn);

    drmmode_adjust_frame(pScrn, &ms->drmmode, x, y);
}

static void
FreeScreen(ScrnInfoPtr pScrn)
{
    FreeRec(pScrn);
}

static void
LeaveVT(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);

    xf86_hide_cursors(pScrn);

    pScrn->vtSema = FALSE;

#ifdef XF86_PDEV_SERVER_FD
    if (ms->pEnt->location.type == BUS_PLATFORM &&
        (ms->pEnt->location.id.plat->flags & XF86_PDEV_SERVER_FD))
        return;
#endif

    if (!ms->fd_passed)
        drmDropMaster(ms->fd);
}

/*
 * This gets called when gaining control of the VT, and from ScreenInit().
 */
static Bool
EnterVT(ScrnInfoPtr pScrn)
{
    modesettingPtr ms = modesettingPTR(pScrn);

    pScrn->vtSema = TRUE;

    SetMaster(pScrn);

    drmmode_update_kms_state(&ms->drmmode);

    /* allow not all modes to be set successfully since some events might have
     * happened while not being master that could prevent the previous
     * configuration from being re-applied.
     */
    if (!drmmode_set_desired_modes(pScrn, &ms->drmmode, TRUE, TRUE)) {
        xf86DisableUnusedFunctions(pScrn);

        /* TODO: check that at least one screen is on, to allow the user to fix
         * their setup if all modeset failed...
         */

        /* Tell the desktop environment that something changed, so that they
         * can hopefully correct the situation
         */
        RRSetChanged(xf86ScrnToScreen(pScrn));
        RRTellChanged(xf86ScrnToScreen(pScrn));
    }

    return TRUE;
}

static Bool
SwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    return xf86SetSingleMode(pScrn, mode, RR_Rotate_0);
}

static Bool
CloseScreen(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    modesettingPtr ms = modesettingPTR(pScrn);
    modesettingEntPtr ms_ent = ms_ent_priv(pScrn);

    /* Clear mask of assigned crtc's in this generation */
    ms_ent->assigned_crtcs = 0;

#ifdef GLAMOR_HAS_GBM
    if (ms->drmmode.dri2_enable) {
        ms_dri2_close_screen(pScreen);
    }
#endif

    ms_vblank_close_screen(pScreen);

    if (ms->damage) {
        DamageUnregister(ms->damage);
        DamageDestroy(ms->damage);
        ms->damage = NULL;
    }

    if (ms->drmmode.shadow_enable) {
        ms->shadow.Remove(pScreen, pScreen->GetScreenPixmap(pScreen));
        free(ms->drmmode.shadow_fb);
        ms->drmmode.shadow_fb = NULL;
        free(ms->drmmode.shadow_fb2);
        ms->drmmode.shadow_fb2 = NULL;
    }

    drmmode_uevent_fini(pScrn, &ms->drmmode);

    drmmode_free_bos(pScrn, &ms->drmmode);

    if (ms->drmmode.pageflip) {
        miPointerScreenPtr PointPriv =
            dixLookupPrivate(&pScreen->devPrivates, miPointerScreenKey);

        if (PointPriv->spriteFuncs == &drmmode_sprite_funcs)
            PointPriv->spriteFuncs = ms->SpriteFuncs;        
    }

    if (pScrn->vtSema) {
        LeaveVT(pScrn);
    }

    pScreen->CreateScreenResources = ms->createScreenResources;
    pScreen->BlockHandler = ms->BlockHandler;

    pScrn->vtSema = FALSE;
    pScreen->CloseScreen = ms->CloseScreen;
    return (*pScreen->CloseScreen) (pScreen);
}

static ModeStatus
ValidMode(ScrnInfoPtr arg, DisplayModePtr mode, Bool verbose, int flags)
{
    return MODE_OK;
}
