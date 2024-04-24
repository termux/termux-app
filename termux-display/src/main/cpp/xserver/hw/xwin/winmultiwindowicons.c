/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Earle F. Philhower, III
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#ifndef WINVER
#define WINVER 0x0500
#endif

#include <limits.h>
#include <stdbool.h>

#include <X11/Xwindows.h>
#include <xcb/xcb.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xcb_image.h>

#include "winresource.h"
#include "winprefs.h"
#include "winmsg.h"
#include "winmultiwindowicons.h"
#include "winglobals.h"

/*
 * global variables
 */
extern HINSTANCE g_hInstance;

/*
 * Scale an X icon ZPixmap into a Windoze icon bitmap
 */

static void
winScaleXImageToWindowsIcon(int iconSize,
                            int effBPP,
                            int stride, xcb_image_t* pixmap, unsigned char *image)
{
    int row, column, effXBPP, effXDepth;
    unsigned char *outPtr;
    unsigned char *iconData = 0;
    int xStride;
    float factX, factY;
    int posX, posY;
    unsigned char *ptr;
    unsigned int zero;
    unsigned int color;

    effXBPP = pixmap->bpp;
    if (pixmap->bpp == 15)
        effXBPP = 16;

    effXDepth = pixmap->depth;
    if (pixmap->depth == 15)
        effXDepth = 16;

    xStride = pixmap->stride;
    if (stride == 0 || xStride == 0) {
        ErrorF("winScaleXBitmapToWindows - stride or xStride is zero.  "
               "Bailing.\n");
        return;
    }

    /* Get icon data */
    iconData = (unsigned char *) pixmap->data;

    /* Keep aspect ratio */
    factX = ((float) pixmap->width) / ((float) iconSize);
    factY = ((float) pixmap->height) / ((float) iconSize);
    if (factX > factY)
        factY = factX;
    else
        factX = factY;

    /* Out-of-bounds, fill icon with zero */
    zero = 0;

    for (row = 0; row < iconSize; row++) {
        outPtr = image + stride * row;
        for (column = 0; column < iconSize; column++) {
            posX = factX * column;
            posY = factY * row;

            ptr = (unsigned char *) iconData + posY * xStride;
            if (effXBPP == 1) {
                ptr += posX / 8;

                /* Out of X icon bounds, leave space blank */
                if (posX >= pixmap->width || posY >= pixmap->height)
                    ptr = (unsigned char *) &zero;

                if ((*ptr) & (1 << (posX & 7)))
                    switch (effBPP) {
                    case 32:
                        *(outPtr++) = 0;
                    case 24:
                        *(outPtr++) = 0;
                    case 16:
                        *(outPtr++) = 0;
                    case 8:
                        *(outPtr++) = 0;
                        break;
                    case 1:
                        outPtr[column / 8] &= ~(1 << (7 - (column & 7)));
                        break;
                    }
                else
                    switch (effBPP) {
                    case 32:
                        *(outPtr++) = 255;
                        *(outPtr++) = 255;
                        *(outPtr++) = 255;
                        *(outPtr++) = 0;
                        break;
                    case 24:
                        *(outPtr++) = 255;
                    case 16:
                        *(outPtr++) = 255;
                    case 8:
                        *(outPtr++) = 255;
                        break;
                    case 1:
                        outPtr[column / 8] |= (1 << (7 - (column & 7)));
                        break;
                    }
            }
            else if (effXDepth == 24 || effXDepth == 32) {
                ptr += posX * (effXBPP / 8);

                /* Out of X icon bounds, leave space blank */
                if (posX >= pixmap->width || posY >= pixmap->height)
                    ptr = (unsigned char *) &zero;
                color = (((*ptr) << 16)
                         + ((*(ptr + 1)) << 8)
                         + ((*(ptr + 2)) << 0));
                switch (effBPP) {
                case 32:
                    *(outPtr++) = *(ptr++);     /* b */
                    *(outPtr++) = *(ptr++);     /* g */
                    *(outPtr++) = *(ptr++);     /* r */
                    *(outPtr++) = (effXDepth == 32) ? *(ptr++) : 0x0;   /* alpha */
                    break;
                case 24:
                    *(outPtr++) = *(ptr++);
                    *(outPtr++) = *(ptr++);
                    *(outPtr++) = *(ptr++);
                    break;
                case 16:
                    color = ((((*ptr) >> 2) << 10)
                             + (((*(ptr + 1)) >> 2) << 5)
                             + (((*(ptr + 2)) >> 2)));
                    *(outPtr++) = (color >> 8);
                    *(outPtr++) = (color & 255);
                    break;
                case 8:
                    color = (((*ptr))) + (((*(ptr + 1)))) + (((*(ptr + 2))));
                    color /= 3;
                    *(outPtr++) = color;
                    break;
                case 1:
                    if (color)
                        outPtr[column / 8] |= (1 << (7 - (column & 7)));
                    else
                        outPtr[column / 8] &= ~(1 << (7 - (column & 7)));
                }
            }
            else if (effXDepth == 16) {
                ptr += posX * (effXBPP / 8);

                /* Out of X icon bounds, leave space blank */
                if (posX >= pixmap->width || posY >= pixmap->height)
                    ptr = (unsigned char *) &zero;
                color = ((*ptr) << 8) + (*(ptr + 1));
                switch (effBPP) {
                case 32:
                    *(outPtr++) = (color & 31) << 2;
                    *(outPtr++) = ((color >> 5) & 31) << 2;
                    *(outPtr++) = ((color >> 10) & 31) << 2;
                    *(outPtr++) = 0;    /* resvd */
                    break;
                case 24:
                    *(outPtr++) = (color & 31) << 2;
                    *(outPtr++) = ((color >> 5) & 31) << 2;
                    *(outPtr++) = ((color >> 10) & 31) << 2;
                    break;
                case 16:
                    *(outPtr++) = *(ptr++);
                    *(outPtr++) = *(ptr++);
                    break;
                case 8:
                    *(outPtr++) = (((color & 31)
                                    + ((color >> 5) & 31)
                                    + ((color >> 10) & 31)) / 3) << 2;
                    break;
                case 1:
                    if (color)
                        outPtr[column / 8] |= (1 << (7 - (column & 7)));
                    else
                        outPtr[column / 8] &= ~(1 << (7 - (column & 7)));
                    break;
                }               /* end switch(effbpp) */
            }                   /* end if effxbpp==16) */
        }                       /* end for column */
    }                           /* end for row */
}

static HICON
NetWMToWinIconAlpha(uint32_t * icon)
{
    int width = icon[0];
    int height = icon[1];
    uint32_t *pixels = &icon[2];
    HICON result;
    HDC hdc = GetDC(NULL);
    uint32_t *DIB_pixels;
    ICONINFO ii;
    BITMAPV4HEADER bmh = { sizeof(bmh) };

    /* Define an ARGB pixel format used for Color+Alpha icons */
    bmh.bV4Width = width;
    bmh.bV4Height = -height;    /* Invert the image */
    bmh.bV4Planes = 1;
    bmh.bV4BitCount = 32;
    bmh.bV4V4Compression = BI_BITFIELDS;
    bmh.bV4AlphaMask = 0xFF000000;
    bmh.bV4RedMask = 0x00FF0000;
    bmh.bV4GreenMask = 0x0000FF00;
    bmh.bV4BlueMask = 0x000000FF;

    ii.fIcon = TRUE;
    ii.xHotspot = 0;            /* ignored */
    ii.yHotspot = 0;            /* ignored */
    ii.hbmColor = CreateDIBSection(hdc, (BITMAPINFO *) &bmh,
                                   DIB_RGB_COLORS, (void **) &DIB_pixels, NULL,
                                   0);
    ReleaseDC(NULL, hdc);

    if (!ii.hbmColor)
      return NULL;

    ii.hbmMask = CreateBitmap(width, height, 1, 1, NULL);
    memcpy(DIB_pixels, pixels, height * width * 4);

    /* CreateIconIndirect() traditionally required DDBitmaps */
    /* Systems from WinXP accept 32-bit ARGB DIBitmaps with full 8-bit alpha support */
    /* The icon is created with a DIB + empty DDB mask (an MS example does the same) */
    result = CreateIconIndirect(&ii);

    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);

    winDebug("NetWMToWinIconAlpha - %d x %d = %p\n", icon[0], icon[1], result);
    return result;
}

static HICON
NetWMToWinIconThreshold(uint32_t * icon)
{
    int width = icon[0];
    int height = icon[1];
    uint32_t *pixels = &icon[2];
    int row, col;
    HICON result;
    ICONINFO ii;

    HDC hdc = GetDC(NULL);
    HDC xorDC = CreateCompatibleDC(hdc);
    HDC andDC = CreateCompatibleDC(hdc);

    ii.fIcon = TRUE;
    ii.xHotspot = 0;            /* ignored */
    ii.yHotspot = 0;            /* ignored */
    ii.hbmColor = CreateCompatibleBitmap(hdc, width, height);
    ii.hbmMask = CreateCompatibleBitmap(hdc, width, height);
    ReleaseDC(NULL, hdc);
    SelectObject(xorDC, ii.hbmColor);
    SelectObject(andDC, ii.hbmMask);

    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            if ((*pixels & 0xFF000000) > 31 << 24) {    /* 31 alpha threshold, i.e. opaque above, transparent below */
                SetPixelV(xorDC, col, row,
                          RGB(((char *) pixels)[2], ((char *) pixels)[1],
                              ((char *) pixels)[0]));
                SetPixelV(andDC, col, row, RGB(0, 0, 0));       /* black mask */
            }
            else {
                SetPixelV(xorDC, col, row, RGB(0, 0, 0));
                SetPixelV(andDC, col, row, RGB(255, 255, 255)); /* white mask */
            }
            pixels++;
        }
    }
    DeleteDC(xorDC);
    DeleteDC(andDC);

    result = CreateIconIndirect(&ii);

    DeleteObject(ii.hbmColor);
    DeleteObject(ii.hbmMask);

    winDebug("NetWMToWinIconThreshold - %d x %d = %p\n", icon[0], icon[1],
             result);
    return result;
}

static HICON
NetWMToWinIcon(int bpp, uint32_t * icon)
{
    static bool hasIconAlphaChannel = FALSE;
    static bool versionChecked = FALSE;

    if (!versionChecked) {
        OSVERSIONINFOEX osvi = { 0 };
        ULONGLONG dwlConditionMask = 0;

        osvi.dwOSVersionInfoSize = sizeof(osvi);
        osvi.dwMajorVersion = 5;
        osvi.dwMinorVersion = 1;

        /* Windows versions later than XP have icon alpha channel support, 2000 does not */
        VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION,
                          VER_GREATER_EQUAL);
        VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION,
                          VER_GREATER_EQUAL);
        hasIconAlphaChannel =
            VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION,
                              dwlConditionMask);
        versionChecked = TRUE;

        ErrorF("OS has icon alpha channel support: %s\n",
               hasIconAlphaChannel ? "yes" : "no");
    }

    if (hasIconAlphaChannel && (bpp == 32))
        return NetWMToWinIconAlpha(icon);
    else
        return NetWMToWinIconThreshold(icon);
}

/*
 * Attempt to create a custom icon from the WM_HINTS bitmaps
 */

static
HICON
winXIconToHICON(xcb_connection_t *conn, xcb_window_t id, int iconSize)
{
    unsigned char *mask, *image = NULL, *imageMask;
    unsigned char *dst, *src;
    int planes, bpp, i;
    unsigned int biggest_size = 0;
    HDC hDC;
    ICONINFO ii;
    xcb_icccm_wm_hints_t hints;
    HICON hIcon = NULL;
    uint32_t *biggest_icon = NULL;
    static xcb_atom_t _XA_NET_WM_ICON;
    static int generation;
    uint32_t *icon, *icon_data = NULL;
    unsigned long int size;

    hDC = GetDC(GetDesktopWindow());
    planes = GetDeviceCaps(hDC, PLANES);
    bpp = GetDeviceCaps(hDC, BITSPIXEL);
    ReleaseDC(GetDesktopWindow(), hDC);

    /* Always prefer _NET_WM_ICON icons */
    if (generation != serverGeneration) {
        xcb_intern_atom_reply_t *atom_reply;
        xcb_intern_atom_cookie_t atom_cookie;
        const char *atomName = "_NET_WM_ICON";

        generation = serverGeneration;

        _XA_NET_WM_ICON = XCB_NONE;

        atom_cookie = xcb_intern_atom(conn, 0, strlen(atomName), atomName);
        atom_reply = xcb_intern_atom_reply(conn, atom_cookie, NULL);
        if (atom_reply) {
          _XA_NET_WM_ICON = atom_reply->atom;
          free(atom_reply);
        }
    }

    {
        xcb_get_property_cookie_t cookie = xcb_get_property(conn, FALSE, id, _XA_NET_WM_ICON, XCB_ATOM_CARDINAL, 0L, INT_MAX);
        xcb_get_property_reply_t *reply =  xcb_get_property_reply(conn, cookie, NULL);

        if (reply &&
            ((icon_data = xcb_get_property_value(reply)) != NULL)) {
          size = xcb_get_property_value_length(reply)/sizeof(uint32_t);
          for (icon = icon_data; icon < &icon_data[size] && *icon;
               icon = &icon[icon[0] * icon[1] + 2]) {
            winDebug("winXIconToHICON: %u x %u NetIcon\n", icon[0], icon[1]);

            /* Icon data size will overflow an int and thus is bigger than the
               property can possibly be */
            if ((INT_MAX/icon[0]) < icon[1]) {
                winDebug("winXIconToHICON: _NET_WM_ICON icon data size overflow\n");
                break;
            }

            /* Icon data size is bigger than amount of data remaining */
            if (&icon[icon[0] * icon[1] + 2] > &icon_data[size]) {
                winDebug("winXIconToHICON: _NET_WM_ICON data is malformed\n");
                break;
            }

            /* Found an exact match to the size we require...  */
            if (icon[0] == iconSize && icon[1] == iconSize) {
                winDebug("winXIconToHICON: selected %d x %d NetIcon\n",
                         iconSize, iconSize);
                hIcon = NetWMToWinIcon(bpp, icon);
                break;
            }
            /* Otherwise, find the biggest icon and let Windows scale the size */
            else if (biggest_size < icon[0]) {
                biggest_icon = icon;
                biggest_size = icon[0];
            }
        }

        if (!hIcon && biggest_icon) {
            winDebug
                ("winXIconToHICON: selected %u x %u NetIcon for scaling to %d x %d\n",
                 biggest_icon[0], biggest_icon[1], iconSize, iconSize);

            hIcon = NetWMToWinIcon(bpp, biggest_icon);
        }

        free(reply);
      }
    }

    if (!hIcon) {
        xcb_get_property_cookie_t wm_hints_cookie;

        winDebug("winXIconToHICON: no suitable NetIcon\n");

        wm_hints_cookie = xcb_icccm_get_wm_hints(conn, id);
        if (xcb_icccm_get_wm_hints_reply(conn, wm_hints_cookie, &hints, NULL)) {
            winDebug("winXIconToHICON: id 0x%x icon_pixmap hint 0x%x\n",
                     (unsigned int)id,
                     (unsigned int)hints.icon_pixmap);

            if (hints.icon_pixmap) {
                unsigned int width, height;
                xcb_image_t *xImageIcon;
                xcb_image_t *xImageMask = NULL;

                xcb_get_geometry_cookie_t geom_cookie = xcb_get_geometry(conn, hints.icon_pixmap);
                xcb_get_geometry_reply_t *geom_reply = xcb_get_geometry_reply(conn, geom_cookie, NULL);

                if (geom_reply) {
                  width = geom_reply->width;
                  height = geom_reply->height;

                  xImageIcon = xcb_image_get(conn, hints.icon_pixmap,
                                             0, 0, width, height,
                                             0xFFFFFF, XCB_IMAGE_FORMAT_Z_PIXMAP);

                  winDebug("winXIconToHICON: id 0x%x icon Ximage 0x%p\n",
                           (unsigned int)id, xImageIcon);

                  if (hints.icon_mask)
                    xImageMask = xcb_image_get(conn, hints.icon_mask,
                                               0, 0, width, height,
                                               0xFFFFFFFF, XCB_IMAGE_FORMAT_Z_PIXMAP);

                  if (xImageIcon) {
                    int effBPP, stride, maskStride;

                    /* 15 BPP is really 16BPP as far as we care */
                    if (bpp == 15)
                        effBPP = 16;
                    else
                        effBPP = bpp;

                    /* Need 16-bit aligned rows for DDBitmaps */
                    stride = ((iconSize * effBPP + 15) & (~15)) / 8;

                    /* Mask is 1-bit deep */
                    maskStride = ((iconSize * 1 + 15) & (~15)) / 8;

                    image = malloc(stride * iconSize);
                    imageMask = malloc(stride * iconSize);
                    mask = malloc(maskStride * iconSize);

                    /* Default to a completely black mask */
                    memset(imageMask, 0, stride * iconSize);
                    memset(mask, 0, maskStride * iconSize);

                    winScaleXImageToWindowsIcon(iconSize, effBPP, stride,
                                                xImageIcon, image);

                    if (xImageMask) {
                        winScaleXImageToWindowsIcon(iconSize, 1, maskStride,
                                                    xImageMask, mask);
                        winScaleXImageToWindowsIcon(iconSize, effBPP, stride,
                                                    xImageMask, imageMask);
                    }

                    /* Now we need to set all bits of the icon which are not masked */
                    /* on to 0 because Color is really an XOR, not an OR function */
                    dst = image;
                    src = imageMask;

                    for (i = 0; i < (stride * iconSize); i++)
                        if ((*(src++)))
                            *(dst++) = 0;
                        else
                            dst++;

                    ii.fIcon = TRUE;
                    ii.xHotspot = 0;    /* ignored */
                    ii.yHotspot = 0;    /* ignored */

                    /* Create Win32 mask from pixmap shape */
                    ii.hbmMask =
                        CreateBitmap(iconSize, iconSize, planes, 1, mask);

                    /* Create Win32 bitmap from pixmap */
                    ii.hbmColor =
                        CreateBitmap(iconSize, iconSize, planes, bpp, image);

                    /* Merge Win32 mask and bitmap into icon */
                    hIcon = CreateIconIndirect(&ii);

                    /* Release Win32 mask and bitmap */
                    DeleteObject(ii.hbmMask);
                    DeleteObject(ii.hbmColor);

                    /* Free X mask and bitmap */
                    free(mask);
                    free(image);
                    free(imageMask);

                    if (xImageMask)
                      xcb_image_destroy(xImageMask);

                    xcb_image_destroy(xImageIcon);
                  }
                }
            }
        }
    }
    return hIcon;
}

/*
 * Change the Windows window icon
 */

void
winUpdateIcon(HWND hWnd, xcb_connection_t *conn, xcb_window_t id, HICON hIconNew)
{
    HICON hIcon, hIconSmall = NULL, hIconOld;

    if (hIconNew)
      {
        /* Start with the icon from preferences, if any */
        hIcon = hIconNew;
        hIconSmall = hIconNew;
      }
    else
      {
        /* If we still need an icon, try and get the icon from WM_HINTS */
        hIcon = winXIconToHICON(conn, id, GetSystemMetrics(SM_CXICON));
        hIconSmall = winXIconToHICON(conn, id, GetSystemMetrics(SM_CXSMICON));
      }

    /* If we got the small, but not the large one swap them */
    if (!hIcon && hIconSmall) {
        hIcon = hIconSmall;
        hIconSmall = NULL;
    }

    /* Set the large icon */
    hIconOld = (HICON) SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
    /* Delete the old icon if its not the default */
    winDestroyIcon(hIconOld);

    /* Same for the small icon */
    hIconOld =
        (HICON) SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIconSmall);
    winDestroyIcon(hIconOld);
}

void
winInitGlobalIcons(void)
{
    int sm_cx = GetSystemMetrics(SM_CXICON);
    int sm_cxsm = GetSystemMetrics(SM_CXSMICON);

    /* Load default X icon in case it's not ready yet */
    if (!g_hIconX) {
        g_hIconX = winOverrideDefaultIcon(sm_cx);
        g_hSmallIconX = winOverrideDefaultIcon(sm_cxsm);
    }

    if (!g_hIconX) {
        g_hIconX = (HICON) LoadImage(g_hInstance,
                                     MAKEINTRESOURCE(IDI_XWIN),
                                     IMAGE_ICON,
                                     GetSystemMetrics(SM_CXICON),
                                     GetSystemMetrics(SM_CYICON), 0);
        g_hSmallIconX = (HICON) LoadImage(g_hInstance,
                                          MAKEINTRESOURCE(IDI_XWIN),
                                          IMAGE_ICON,
                                          GetSystemMetrics(SM_CXSMICON),
                                          GetSystemMetrics(SM_CYSMICON),
                                          LR_DEFAULTSIZE);
    }
}

void
winSelectIcons(HICON * pIcon, HICON * pSmallIcon)
{
    HICON hIcon, hSmallIcon;

    winInitGlobalIcons();

    /* Use default X icon */
    hIcon = g_hIconX;
    hSmallIcon = g_hSmallIconX;

    if (pIcon)
        *pIcon = hIcon;

    if (pSmallIcon)
        *pSmallIcon = hSmallIcon;
}

void
winDestroyIcon(HICON hIcon)
{
    /* Delete the icon if its not one of the application defaults or an override */
    if (hIcon &&
        hIcon != g_hIconX &&
        hIcon != g_hSmallIconX && !winIconIsOverride(hIcon))
        DestroyIcon(hIcon);
}
