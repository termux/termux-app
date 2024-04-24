
#ifndef __DGAPROC_H
#define __DGAPROC_H

#include <X11/Xproto.h>
#include "pixmap.h"

#define DGA_CONCURRENT_ACCESS	0x00000001
#define DGA_FILL_RECT		0x00000002
#define DGA_BLIT_RECT		0x00000004
#define DGA_BLIT_RECT_TRANS	0x00000008
#define DGA_PIXMAP_AVAILABLE	0x00000010

#define DGA_INTERLACED		0x00010000
#define DGA_DOUBLESCAN		0x00020000

#define DGA_FLIP_IMMEDIATE	0x00000001
#define DGA_FLIP_RETRACE	0x00000002

#define DGA_COMPLETED		0x00000000
#define DGA_PENDING		0x00000001

#define DGA_NEED_ROOT		0x00000001

typedef struct {
    int num;                    /* A unique identifier for the mode (num > 0) */
    const char *name;           /* name of mode given in the XF86Config */
    int VSync_num;
    int VSync_den;
    int flags;                  /* DGA_CONCURRENT_ACCESS, etc... */
    int imageWidth;             /* linear accessible portion (pixels) */
    int imageHeight;
    int pixmapWidth;            /* Xlib accessible portion (pixels) */
    int pixmapHeight;           /* both fields ignored if no concurrent access */
    int bytesPerScanline;
    int byteOrder;              /* MSBFirst, LSBFirst */
    int depth;
    int bitsPerPixel;
    unsigned long red_mask;
    unsigned long green_mask;
    unsigned long blue_mask;
    short visualClass;
    int viewportWidth;
    int viewportHeight;
    int xViewportStep;          /* viewport position granularity */
    int yViewportStep;
    int maxViewportX;           /* max viewport origin */
    int maxViewportY;
    int viewportFlags;          /* types of page flipping possible */
    int offset;
    int reserved1;
    int reserved2;
} XDGAModeRec, *XDGAModePtr;

/* DDX interface */

extern Bool DGAScreenAvailable(ScreenPtr pScreen);
extern Bool DGAActive(int Index);

extern Bool DGAVTSwitch(void);
extern Bool DGAStealButtonEvent(DeviceIntPtr dev, int Index,
                                int button, int is_down);
extern Bool DGAStealMotionEvent(DeviceIntPtr dev, int Index, int dx, int dy);
extern Bool DGAStealKeyEvent(DeviceIntPtr dev, int Index,
                             int key_code, int is_down);

#endif                          /* __DGAPROC_H */
