/* all drivers need this */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include <string.h>

#include "xf86.h"
#include "xf86Modes.h"
#include "xf86_OSproc.h"

/* pci stuff */
#include "xf86Pci.h"

#include "xf86cmap.h"

#include "fbdevhw.h"
#include "fbpriv.h"
#include "globals.h"
#include <X11/extensions/dpmsconst.h>

#define PAGE_MASK               (~(getpagesize() - 1))

static XF86ModuleVersionInfo fbdevHWVersRec = {
    "fbdevhw",
    MODULEVENDORSTRING,
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_CURRENT,
    0, 0, 2,
    ABI_CLASS_VIDEODRV,
    ABI_VIDEODRV_VERSION,
    MOD_CLASS_NONE,
    {0, 0, 0, 0}
};

_X_EXPORT XF86ModuleData fbdevhwModuleData = {
    &fbdevHWVersRec,
    NULL,
    NULL
};

#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* -------------------------------------------------------------------- */
/* our private data, and two functions to allocate/free this            */

#define FBDEVHWPTRLVAL(p) (p)->privates[fbdevHWPrivateIndex].ptr
#define FBDEVHWPTR(p) ((fbdevHWPtr)(FBDEVHWPTRLVAL(p)))

static int fbdevHWPrivateIndex = -1;

typedef struct {
    /* framebuffer device: filename (/dev/fb*), handle, more */
    char *device;
    int fd;
    void *fbmem;
    unsigned int fbmem_len;
    unsigned int fboff;
    char *mmio;
    unsigned int mmio_len;

    /* current hardware state */
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;

    /* saved video mode */
    struct fb_var_screeninfo saved_var;

    /* buildin video mode */
    DisplayModeRec buildin;

    /* disable non-fatal unsupported ioctls */
    CARD32 unsupported_ioctls;
} fbdevHWRec, *fbdevHWPtr;

enum {
    FBIOBLANK_UNSUPPORTED = 0,
};

Bool
fbdevHWGetRec(ScrnInfoPtr pScrn)
{
    if (fbdevHWPrivateIndex < 0)
        fbdevHWPrivateIndex = xf86AllocateScrnInfoPrivateIndex();

    if (FBDEVHWPTR(pScrn) != NULL)
        return TRUE;

    FBDEVHWPTRLVAL(pScrn) = xnfcalloc(sizeof(fbdevHWRec), 1);
    return TRUE;
}

void
fbdevHWFreeRec(ScrnInfoPtr pScrn)
{
    if (fbdevHWPrivateIndex < 0)
        return;
    free(FBDEVHWPTR(pScrn));
    FBDEVHWPTRLVAL(pScrn) = NULL;
}

int
fbdevHWGetFD(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr;

    fbdevHWGetRec(pScrn);
    fPtr = FBDEVHWPTR(pScrn);

    return fPtr->fd;
}

/* -------------------------------------------------------------------- */
/* some helpers for printing debug information                          */

#ifdef DEBUG
static void
print_fbdev_mode(const char *txt, struct fb_var_screeninfo *var)
{
    ErrorF("fbdev %s mode:\t%d   %d %d %d %d   %d %d %d %d   %d %d:%d:%d\n",
           txt, var->pixclock,
           var->xres, var->right_margin, var->hsync_len, var->left_margin,
           var->yres, var->lower_margin, var->vsync_len, var->upper_margin,
           var->bits_per_pixel,
           var->red.length, var->green.length, var->blue.length);
}

static void
print_xfree_mode(const char *txt, DisplayModePtr mode)
{
    ErrorF("xfree %s mode:\t%d   %d %d %d %d   %d %d %d %d\n",
           txt, mode->Clock,
           mode->HDisplay, mode->HSyncStart, mode->HSyncEnd, mode->HTotal,
           mode->VDisplay, mode->VSyncStart, mode->VSyncEnd, mode->VTotal);
}
#endif

/* -------------------------------------------------------------------- */
/* Convert timings between the XFree and the Frame Buffer Device        */

static void
xfree2fbdev_fblayout(ScrnInfoPtr pScrn, struct fb_var_screeninfo *var)
{
    var->xres_virtual = pScrn->displayWidth ? pScrn->displayWidth :
        pScrn->virtualX;
    var->yres_virtual = pScrn->virtualY;
    var->bits_per_pixel = pScrn->bitsPerPixel;
    if (pScrn->defaultVisual == TrueColor ||
        pScrn->defaultVisual == DirectColor) {
        var->red.length = pScrn->weight.red;
        var->green.length = pScrn->weight.green;
        var->blue.length = pScrn->weight.blue;
    }
    else {
        var->red.length = 8;
        var->green.length = 8;
        var->blue.length = 8;
    }
}

static void
xfree2fbdev_timing(DisplayModePtr mode, struct fb_var_screeninfo *var)
{
    var->xres = mode->HDisplay;
    var->yres = mode->VDisplay;
    if (var->xres_virtual < var->xres)
        var->xres_virtual = var->xres;
    if (var->yres_virtual < var->yres)
        var->yres_virtual = var->yres;
    var->xoffset = var->yoffset = 0;
    var->pixclock = mode->Clock ? 1000000000 / mode->Clock : 0;
    var->right_margin = mode->HSyncStart - mode->HDisplay;
    var->hsync_len = mode->HSyncEnd - mode->HSyncStart;
    var->left_margin = mode->HTotal - mode->HSyncEnd;
    var->lower_margin = mode->VSyncStart - mode->VDisplay;
    var->vsync_len = mode->VSyncEnd - mode->VSyncStart;
    var->upper_margin = mode->VTotal - mode->VSyncEnd;
    var->sync = 0;
    if (mode->Flags & V_PHSYNC)
        var->sync |= FB_SYNC_HOR_HIGH_ACT;
    if (mode->Flags & V_PVSYNC)
        var->sync |= FB_SYNC_VERT_HIGH_ACT;
    if (mode->Flags & V_PCSYNC)
        var->sync |= FB_SYNC_COMP_HIGH_ACT;
    if (mode->Flags & V_BCAST)
        var->sync |= FB_SYNC_BROADCAST;
    if (mode->Flags & V_INTERLACE)
        var->vmode = FB_VMODE_INTERLACED;
    else if (mode->Flags & V_DBLSCAN)
        var->vmode = FB_VMODE_DOUBLE;
    else
        var->vmode = FB_VMODE_NONINTERLACED;
}

static Bool
fbdev_modes_equal(struct fb_var_screeninfo *set, struct fb_var_screeninfo *req)
{
    return (set->xres_virtual >= req->xres_virtual &&
            set->yres_virtual >= req->yres_virtual &&
            set->bits_per_pixel == req->bits_per_pixel &&
            set->red.length == req->red.length &&
            set->green.length == req->green.length &&
            set->blue.length == req->blue.length &&
            set->xres == req->xres && set->yres == req->yres &&
            set->right_margin == req->right_margin &&
            set->hsync_len == req->hsync_len &&
            set->left_margin == req->left_margin &&
            set->lower_margin == req->lower_margin &&
            set->vsync_len == req->vsync_len &&
            set->upper_margin == req->upper_margin &&
            set->sync == req->sync && set->vmode == req->vmode);
}

static void
fbdev2xfree_timing(struct fb_var_screeninfo *var, DisplayModePtr mode)
{
    mode->Clock = var->pixclock ? 1000000000 / var->pixclock : 0;
    mode->HDisplay = var->xres;
    mode->HSyncStart = mode->HDisplay + var->right_margin;
    mode->HSyncEnd = mode->HSyncStart + var->hsync_len;
    mode->HTotal = mode->HSyncEnd + var->left_margin;
    mode->VDisplay = var->yres;
    mode->VSyncStart = mode->VDisplay + var->lower_margin;
    mode->VSyncEnd = mode->VSyncStart + var->vsync_len;
    mode->VTotal = mode->VSyncEnd + var->upper_margin;
    mode->Flags = 0;
    mode->Flags |= var->sync & FB_SYNC_HOR_HIGH_ACT ? V_PHSYNC : V_NHSYNC;
    mode->Flags |= var->sync & FB_SYNC_VERT_HIGH_ACT ? V_PVSYNC : V_NVSYNC;
    mode->Flags |= var->sync & FB_SYNC_COMP_HIGH_ACT ? V_PCSYNC : V_NCSYNC;
    if (var->sync & FB_SYNC_BROADCAST)
        mode->Flags |= V_BCAST;
    if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_INTERLACED)
        mode->Flags |= V_INTERLACE;
    else if ((var->vmode & FB_VMODE_MASK) == FB_VMODE_DOUBLE)
        mode->Flags |= V_DBLSCAN;
    mode->SynthClock = mode->Clock;
    mode->CrtcHDisplay = mode->HDisplay;
    mode->CrtcHSyncStart = mode->HSyncStart;
    mode->CrtcHSyncEnd = mode->HSyncEnd;
    mode->CrtcHTotal = mode->HTotal;
    mode->CrtcVDisplay = mode->VDisplay;
    mode->CrtcVSyncStart = mode->VSyncStart;
    mode->CrtcVSyncEnd = mode->VSyncEnd;
    mode->CrtcVTotal = mode->VTotal;
    mode->CrtcHAdjusted = FALSE;
    mode->CrtcVAdjusted = FALSE;
}

/* -------------------------------------------------------------------- */
/* open correct framebuffer device                                      */

/**
 * Try to find the framebuffer device for a given PCI device
 */
static int
fbdev_open_pci(struct pci_device *pPci, char **namep)
{
    struct fb_fix_screeninfo fix;
    char filename[256];
    int fd, i;

    for (i = 0; i < 8; i++) {
        snprintf(filename, sizeof(filename),
                 "/sys/bus/pci/devices/%04x:%02x:%02x.%d/graphics/fb%d",
                 pPci->domain, pPci->bus, pPci->dev, pPci->func, i);

        fd = open(filename, O_RDONLY, 0);
        if (fd < 0) {
            snprintf(filename, sizeof(filename),
                     "/sys/bus/pci/devices/%04x:%02x:%02x.%d/graphics:fb%d",
                     pPci->domain, pPci->bus, pPci->dev, pPci->func, i);
            fd = open(filename, O_RDONLY, 0);
        }
        if (fd >= 0) {
            close(fd);
            snprintf(filename, sizeof(filename), "/dev/fb%d", i);

            fd = open(filename, O_RDWR, 0);
            if (fd != -1) {
                if (ioctl(fd, FBIOGET_FSCREENINFO, (void *) &fix) != -1) {
                    if (namep) {
                        *namep = xnfalloc(16);
                        strncpy(*namep, fix.id, 16);
                    }

                    return fd;
                }
                close(fd);
            }
        }
    }

    if (namep)
        *namep = NULL;

    xf86DrvMsg(-1, X_ERROR, "Unable to find a valid framebuffer device\n");
    return -1;
}

static int
fbdev_open(int scrnIndex, const char *dev, char **namep)
{
    struct fb_fix_screeninfo fix;
    int fd;

    /* try argument (from XF86Config) first */
    if (dev) {
        fd = open(dev, O_RDWR, 0);
    }
    else {
        /* second: environment variable */
        dev = getenv("FRAMEBUFFER");
        if ((NULL == dev) || ((fd = open(dev, O_RDWR, 0)) == -1)) {
            /* last try: default device */
            dev = "/dev/fb0";
            fd = open(dev, O_RDWR, 0);
        }
    }

    if (fd == -1) {
        xf86DrvMsg(scrnIndex, X_ERROR, "open %s: %s\n", dev, strerror(errno));
        return -1;
    }

    /* only touch non-PCI devices on this path */
    {
        char buf[PATH_MAX] = {0};
        char *sysfs_path = NULL;
        char *node = strrchr(dev, '/') + 1;

        if (asprintf(&sysfs_path, "/sys/class/graphics/%s", node) < 0 ||
            readlink(sysfs_path, buf, sizeof(buf) - 1) < 0 ||
            strstr(buf, "devices/pci")) {
            free(sysfs_path);
            close(fd);
            return -1;
        }
        free(sysfs_path);
    }

    if (namep) {
        if (-1 == ioctl(fd, FBIOGET_FSCREENINFO, (void *) (&fix))) {
            *namep = NULL;
            xf86DrvMsg(scrnIndex, X_ERROR,
                       "FBIOGET_FSCREENINFO: %s\n", strerror(errno));
            return -1;
        }
        else {
            *namep = xnfalloc(16);
            strncpy(*namep, fix.id, 16);
        }
    }
    return fd;
}

/* -------------------------------------------------------------------- */

Bool
fbdevHWProbe(struct pci_device *pPci, char *device, char **namep)
{
    int fd;

    if (pPci)
        fd = fbdev_open_pci(pPci, namep);
    else
        fd = fbdev_open(-1, device, namep);

    if (-1 == fd)
        return FALSE;
    close(fd);
    return TRUE;
}

Bool
fbdevHWInit(ScrnInfoPtr pScrn, struct pci_device *pPci, char *device)
{
    fbdevHWPtr fPtr;

    fbdevHWGetRec(pScrn);
    fPtr = FBDEVHWPTR(pScrn);

    /* open device */
    if (pPci)
        fPtr->fd = fbdev_open_pci(pPci, NULL);
    else
        fPtr->fd = fbdev_open(pScrn->scrnIndex, device, NULL);
    if (-1 == fPtr->fd) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "Failed to open framebuffer device, consult warnings"
                   " and/or errors above for possible reasons\n"
                   "\t(you may have to look at the server log to see"
                   " warnings)\n");
        return FALSE;
    }

    /* get current fb device settings */
    if (-1 == ioctl(fPtr->fd, FBIOGET_FSCREENINFO, (void *) (&fPtr->fix))) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "ioctl FBIOGET_FSCREENINFO: %s\n", strerror(errno));
        return FALSE;
    }
    if (-1 == ioctl(fPtr->fd, FBIOGET_VSCREENINFO, (void *) (&fPtr->var))) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "ioctl FBIOGET_VSCREENINFO: %s\n", strerror(errno));
        return FALSE;
    }

    /* we can use the current settings as "buildin mode" */
    fbdev2xfree_timing(&fPtr->var, &fPtr->buildin);
    fPtr->buildin.name = "current";
    fPtr->buildin.next = &fPtr->buildin;
    fPtr->buildin.prev = &fPtr->buildin;
    fPtr->buildin.type |= M_T_BUILTIN;

    return TRUE;
}

char *
fbdevHWGetName(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    return fPtr->fix.id;
}

int
fbdevHWGetDepth(ScrnInfoPtr pScrn, int *fbbpp)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    if (fbbpp)
        *fbbpp = fPtr->var.bits_per_pixel;

    if (fPtr->fix.visual == FB_VISUAL_TRUECOLOR ||
        fPtr->fix.visual == FB_VISUAL_DIRECTCOLOR)
        return fPtr->var.red.length + fPtr->var.green.length +
            fPtr->var.blue.length;
    else
        return fPtr->var.bits_per_pixel;
}

int
fbdevHWGetLineLength(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    if (fPtr->fix.line_length)
        return fPtr->fix.line_length;
    else
        return fPtr->var.xres_virtual * fPtr->var.bits_per_pixel / 8;
}

int
fbdevHWGetType(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    return fPtr->fix.type;
}

int
fbdevHWGetVidmem(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    return fPtr->fix.smem_len;
}

static Bool
fbdevHWSetMode(ScrnInfoPtr pScrn, DisplayModePtr mode, Bool check)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
    struct fb_var_screeninfo req_var = fPtr->var, set_var;

    xfree2fbdev_fblayout(pScrn, &req_var);
    xfree2fbdev_timing(mode, &req_var);

#ifdef DEBUG
    print_xfree_mode("init", mode);
    print_fbdev_mode("init", &req_var);
#endif

    set_var = req_var;

    if (check)
        set_var.activate = FB_ACTIVATE_TEST;

    if (0 != ioctl(fPtr->fd, FBIOPUT_VSCREENINFO, (void *) (&set_var))) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
        return FALSE;
    }

    if (!fbdev_modes_equal(&set_var, &req_var)) {
        if (!check)
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "FBIOPUT_VSCREENINFO succeeded but modified " "mode\n");
#ifdef DEBUG
        print_fbdev_mode("returned", &set_var);
#endif
        return FALSE;
    }

    if (!check)
        fPtr->var = set_var;

    return TRUE;
}

void
fbdevHWSetVideoModes(ScrnInfoPtr pScrn)
{
    const char **modename;
    DisplayModePtr mode, this, last = pScrn->modes;

    if (NULL == pScrn->display->modes)
        return;

    pScrn->virtualX = pScrn->display->virtualX;
    pScrn->virtualY = pScrn->display->virtualY;

    for (modename = pScrn->display->modes; *modename != NULL; modename++) {
        for (mode = pScrn->monitor->Modes; mode != NULL; mode = mode->next) {
            if (0 == strcmp(mode->name, *modename)) {
                if (fbdevHWSetMode(pScrn, mode, TRUE))
                    break;

                xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                           "\tmode \"%s\" test failed\n", *modename);
            }
        }

        if (NULL == mode) {
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                       "\tmode \"%s\" not found\n", *modename);
            continue;
        }

        xf86DrvMsg(pScrn->scrnIndex, X_INFO, "\tmode \"%s\" ok\n", *modename);

        if (pScrn->virtualX < mode->HDisplay)
            pScrn->virtualX = mode->HDisplay;
        if (pScrn->virtualY < mode->VDisplay)
            pScrn->virtualY = mode->VDisplay;

        if (NULL == pScrn->modes) {
            this = pScrn->modes = xf86DuplicateMode(mode);
            this->next = this;
            this->prev = this;
        }
        else {
            this = xf86DuplicateMode(mode);
            this->next = pScrn->modes;
            this->prev = last;
            last->next = this;
            pScrn->modes->prev = this;
        }
        last = this;
    }
}

DisplayModePtr
fbdevHWGetBuildinMode(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    return &fPtr->buildin;
}

void
fbdevHWUseBuildinMode(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    pScrn->modes = &fPtr->buildin;
    pScrn->virtualX = pScrn->display->virtualX;
    pScrn->virtualY = pScrn->display->virtualY;
    if (pScrn->virtualX < fPtr->buildin.HDisplay)
        pScrn->virtualX = fPtr->buildin.HDisplay;
    if (pScrn->virtualY < fPtr->buildin.VDisplay)
        pScrn->virtualY = fPtr->buildin.VDisplay;
}

/* -------------------------------------------------------------------- */

static void
calculateFbmem_len(fbdevHWPtr fPtr)
{
    fPtr->fboff = (unsigned long) fPtr->fix.smem_start & ~PAGE_MASK;
    fPtr->fbmem_len = (fPtr->fboff + fPtr->fix.smem_len + ~PAGE_MASK) &
        PAGE_MASK;
}

void *
fbdevHWMapVidmem(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    if (NULL == fPtr->fbmem) {
        calculateFbmem_len(fPtr);
        fPtr->fbmem = mmap(NULL, fPtr->fbmem_len, PROT_READ | PROT_WRITE,
                           MAP_SHARED, fPtr->fd, 0);
        if (-1 == (long) fPtr->fbmem) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "mmap fbmem: %s\n", strerror(errno));
            fPtr->fbmem = NULL;
        }
        else {
            /* Perhaps we'd better add fboff to fbmem and return 0 in
               fbdevHWLinearOffset()? Of course we then need to mask
               fPtr->fbmem with PAGE_MASK in fbdevHWUnmapVidmem() as
               well. [geert] */
        }
    }
    pScrn->memPhysBase =
        (unsigned long) fPtr->fix.smem_start & (unsigned long) (PAGE_MASK);
    pScrn->fbOffset =
        (unsigned long) fPtr->fix.smem_start & (unsigned long) (~PAGE_MASK);
    return fPtr->fbmem;
}

int
fbdevHWLinearOffset(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    return fPtr->fboff;
}

Bool
fbdevHWUnmapVidmem(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    if (NULL != fPtr->fbmem) {
        if (-1 == munmap(fPtr->fbmem, fPtr->fbmem_len))
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "munmap fbmem: %s\n", strerror(errno));
        fPtr->fbmem = NULL;
    }
    return TRUE;
}

void *
fbdevHWMapMMIO(ScrnInfoPtr pScrn)
{
    unsigned int mmio_off;

    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    if (NULL == fPtr->mmio) {
        /* tell the kernel not to use accels to speed up console scrolling */
        fPtr->var.accel_flags = 0;
        if (0 != ioctl(fPtr->fd, FBIOPUT_VSCREENINFO, (void *) (&fPtr->var))) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
            return FALSE;
        }
        mmio_off = (unsigned long) fPtr->fix.mmio_start & ~PAGE_MASK;
        fPtr->mmio_len = (mmio_off + fPtr->fix.mmio_len + ~PAGE_MASK) &
            PAGE_MASK;
        if (NULL == fPtr->fbmem)
            calculateFbmem_len(fPtr);
        fPtr->mmio = mmap(NULL, fPtr->mmio_len, PROT_READ | PROT_WRITE,
                          MAP_SHARED, fPtr->fd, fPtr->fbmem_len);
        if (-1 == (long) fPtr->mmio) {
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "mmap mmio: %s\n", strerror(errno));
            fPtr->mmio = NULL;
        }
        else
            fPtr->mmio += mmio_off;
    }
    return fPtr->mmio;
}

Bool
fbdevHWUnmapMMIO(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    if (NULL != fPtr->mmio) {
        if (-1 ==
            munmap((void *) ((unsigned long) fPtr->mmio & PAGE_MASK),
                   fPtr->mmio_len))
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR, "munmap mmio: %s\n",
                       strerror(errno));
        fPtr->mmio = NULL;
        /* FIXME: restore var.accel_flags [geert] */
    }
    return TRUE;
}

/* -------------------------------------------------------------------- */

Bool
fbdevHWModeInit(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    pScrn->vtSema = TRUE;

    /* set */
    if (!fbdevHWSetMode(pScrn, mode, FALSE))
        return FALSE;

    /* read back */
    if (0 != ioctl(fPtr->fd, FBIOGET_FSCREENINFO, (void *) (&fPtr->fix))) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "FBIOGET_FSCREENINFO: %s\n", strerror(errno));
        return FALSE;
    }
    if (0 != ioctl(fPtr->fd, FBIOGET_VSCREENINFO, (void *) (&fPtr->var))) {
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "FBIOGET_VSCREENINFO: %s\n", strerror(errno));
        return FALSE;
    }

    if (pScrn->defaultVisual == TrueColor ||
        pScrn->defaultVisual == DirectColor) {
        /* XXX: This is a hack, but it should be a NOP for all the setups that
         * worked before and actually seems to fix some others...
         */
        pScrn->offset.red = fPtr->var.red.offset;
        pScrn->offset.green = fPtr->var.green.offset;
        pScrn->offset.blue = fPtr->var.blue.offset;
        pScrn->mask.red =
            ((1 << fPtr->var.red.length) - 1) << fPtr->var.red.offset;
        pScrn->mask.green =
            ((1 << fPtr->var.green.length) - 1) << fPtr->var.green.offset;
        pScrn->mask.blue =
            ((1 << fPtr->var.blue.length) - 1) << fPtr->var.blue.offset;
    }

    return TRUE;
}

/* -------------------------------------------------------------------- */
/* video mode save/restore                                              */
void
fbdevHWSave(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    if (0 != ioctl(fPtr->fd, FBIOGET_VSCREENINFO, (void *) (&fPtr->saved_var)))
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "FBIOGET_VSCREENINFO: %s\n", strerror(errno));
}

void
fbdevHWRestore(ScrnInfoPtr pScrn)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    if (0 != ioctl(fPtr->fd, FBIOPUT_VSCREENINFO, (void *) (&fPtr->saved_var)))
        xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                   "FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
}

/* -------------------------------------------------------------------- */
/* callback for xf86HandleColormaps                                     */

void
fbdevHWLoadPalette(ScrnInfoPtr pScrn, int numColors, int *indices,
                   LOCO * colors, VisualPtr pVisual)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
    struct fb_cmap cmap;
    unsigned short red, green, blue;
    int i;

    cmap.len = 1;
    cmap.red = &red;
    cmap.green = &green;
    cmap.blue = &blue;
    cmap.transp = NULL;
    for (i = 0; i < numColors; i++) {
        cmap.start = indices[i];
        red = (colors[indices[i]].red << 8) | colors[indices[i]].red;
        green = (colors[indices[i]].green << 8) | colors[indices[i]].green;
        blue = (colors[indices[i]].blue << 8) | colors[indices[i]].blue;
        if (-1 == ioctl(fPtr->fd, FBIOPUTCMAP, (void *) &cmap))
            xf86DrvMsg(pScrn->scrnIndex, X_ERROR,
                       "FBIOPUTCMAP: %s\n", strerror(errno));
    }
}

/* -------------------------------------------------------------------- */
/* these can be hooked directly into ScrnInfoRec                        */

ModeStatus
fbdevHWValidMode(ScrnInfoPtr pScrn, DisplayModePtr mode, Bool verbose, int flags)
{
    if (!fbdevHWSetMode(pScrn, mode, TRUE))
        return MODE_BAD;

    return MODE_OK;
}

Bool
fbdevHWSwitchMode(ScrnInfoPtr pScrn, DisplayModePtr mode)
{
    if (!fbdevHWSetMode(pScrn, mode, FALSE))
        return FALSE;

    return TRUE;
}

void
fbdevHWAdjustFrame(ScrnInfoPtr pScrn, int x, int y)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);

    if (x < 0 || x + fPtr->var.xres > fPtr->var.xres_virtual ||
        y < 0 || y + fPtr->var.yres > fPtr->var.yres_virtual)
        return;

    fPtr->var.xoffset = x;
    fPtr->var.yoffset = y;
    if (-1 == ioctl(fPtr->fd, FBIOPAN_DISPLAY, (void *) &fPtr->var))
        xf86DrvMsgVerb(pScrn->scrnIndex, X_WARNING, 5,
                       "FBIOPAN_DISPLAY: %s\n", strerror(errno));
}

Bool
fbdevHWEnterVT(ScrnInfoPtr pScrn)
{
    if (!fbdevHWModeInit(pScrn, pScrn->currentMode))
        return FALSE;
    fbdevHWAdjustFrame(pScrn, pScrn->frameX0, pScrn->frameY0);
    return TRUE;
}

void
fbdevHWLeaveVT(ScrnInfoPtr pScrn)
{
    fbdevHWRestore(pScrn);
}

void
fbdevHWDPMSSet(ScrnInfoPtr pScrn, int mode, int flags)
{
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
    unsigned long fbmode;

    if (!pScrn->vtSema)
        return;

    if (fPtr->unsupported_ioctls & (1 << FBIOBLANK_UNSUPPORTED))
        return;

    switch (mode) {
    case DPMSModeOn:
        fbmode = 0;
        break;
    case DPMSModeStandby:
        fbmode = 2;
        break;
    case DPMSModeSuspend:
        fbmode = 3;
        break;
    case DPMSModeOff:
        fbmode = 4;
        break;
    default:
        return;
    }

RETRY:
    if (-1 == ioctl(fPtr->fd, FBIOBLANK, (void *) fbmode)) {
        switch (errno) {
        case EAGAIN:
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                       "FBIOBLANK: %s\n", strerror(errno));
	    break;
        case EINTR:
        case ERESTART:
            goto RETRY;
        default:
            fPtr->unsupported_ioctls |= (1 << FBIOBLANK_UNSUPPORTED);
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                       "FBIOBLANK: %s (Screen blanking not supported "
                       "by kernel - disabling)\n", strerror(errno));
        }
    }
}

Bool
fbdevHWSaveScreen(ScreenPtr pScreen, int mode)
{
    ScrnInfoPtr pScrn = xf86ScreenToScrn(pScreen);
    fbdevHWPtr fPtr = FBDEVHWPTR(pScrn);
    unsigned long unblank;

    if (!pScrn->vtSema)
        return TRUE;

    if (fPtr->unsupported_ioctls & (1 << FBIOBLANK_UNSUPPORTED))
        return FALSE;

    unblank = xf86IsUnblank(mode);

RETRY:
    if (-1 == ioctl(fPtr->fd, FBIOBLANK, (void *) (1 - unblank))) {
        switch (errno) {
        case EAGAIN:
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                       "FBIOBLANK: %s\n", strerror(errno));
            break;
        case EINTR:
        case ERESTART:
            goto RETRY;
        default:
            fPtr->unsupported_ioctls |= (1 << FBIOBLANK_UNSUPPORTED);
            xf86DrvMsg(pScrn->scrnIndex, X_INFO,
                       "FBIOBLANK: %s (Screen blanking not supported "
                       "by kernel - disabling)\n", strerror(errno));
        }
        return FALSE;
    }

    return TRUE;
}

xf86SwitchModeProc *
fbdevHWSwitchModeWeak(void)
{
    return fbdevHWSwitchMode;
}

xf86AdjustFrameProc *
fbdevHWAdjustFrameWeak(void)
{
    return fbdevHWAdjustFrame;
}

xf86EnterVTProc *
fbdevHWEnterVTWeak(void)
{
    return fbdevHWEnterVT;
}

xf86LeaveVTProc *
fbdevHWLeaveVTWeak(void)
{
    return fbdevHWLeaveVT;
}

xf86ValidModeProc *
fbdevHWValidModeWeak(void)
{
    return fbdevHWValidMode;
}

xf86DPMSSetProc *
fbdevHWDPMSSetWeak(void)
{
    return fbdevHWDPMSSet;
}

xf86LoadPaletteProc *
fbdevHWLoadPaletteWeak(void)
{
    return fbdevHWLoadPalette;
}

SaveScreenProcPtr
fbdevHWSaveScreenWeak(void)
{
    return fbdevHWSaveScreen;
}
