/**************************************************************
 *
 * Xplugin cursor support
 *
 * Copyright (c) 2001 Torrey T. Lyons and Greg Parker.
 * Copyright (c) 2002 Apple Computer, Inc.
 *                 All Rights Reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 */

#include "sanitizedCarbon.h"

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "quartz.h"
#include "xpr.h"
#include "darwinEvents.h"
#include <Xplugin.h>

#include "mi.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "mipointrst.h"
#include "windowstr.h"
#include "globals.h"
#include "servermd.h"
#include "dixevents.h"
#include "x-hash.h"

typedef struct {
    int cursorVisible;
    QueryBestSizeProcPtr QueryBestSize;
    miPointerSpriteFuncPtr spriteFuncs;
} QuartzCursorScreenRec, *QuartzCursorScreenPtr;

static DevPrivateKeyRec darwinCursorScreenKeyRec;
#define darwinCursorScreenKey (&darwinCursorScreenKeyRec)

#define CURSOR_PRIV(pScreen) ((QuartzCursorScreenPtr) \
                              dixLookupPrivate(&pScreen->devPrivates, \
                                               darwinCursorScreenKey))

static Bool
load_cursor(CursorPtr src, int screen)
{
    uint32_t *data;
    Bool free_data = FALSE;
    uint32_t rowbytes;
    int width, height;
    int hot_x, hot_y;

    uint32_t fg_color, bg_color;
    uint8_t *srow, *sptr;
    uint8_t *mrow, *mptr;
    uint32_t *drow, *dptr;
    unsigned xcount, ycount;

    xp_error err;

    width = src->bits->width;
    height = src->bits->height;
    hot_x = src->bits->xhot;
    hot_y = src->bits->yhot;

    if (src->bits->argb != NULL) {
#if BITMAP_BIT_ORDER == MSBFirst
        rowbytes = src->bits->width * sizeof(CARD32);
        data = (uint32_t *)src->bits->argb;
#else
        const uint32_t *be_data = (uint32_t *)src->bits->argb;
        unsigned i;
        rowbytes = src->bits->width * sizeof(CARD32);
        data = malloc(rowbytes * src->bits->height);
        free_data = TRUE;
        if (!data) {
            FatalError("Failed to allocate memory in %s\n", __func__);
        }
        for (i = 0; i < (src->bits->width * src->bits->height); i++)
            data[i] = ntohl(be_data[i]);
#endif
    }
    else
    {
        fg_color = 0xFF00 | (src->foreRed >> 8);
        fg_color <<= 16;
        fg_color |= src->foreGreen & 0xFF00;
        fg_color |= src->foreBlue >> 8;

        bg_color = 0xFF00 | (src->backRed >> 8);
        bg_color <<= 16;
        bg_color |= src->backGreen & 0xFF00;
        bg_color |= src->backBlue >> 8;

        fg_color = htonl(fg_color);
        bg_color = htonl(bg_color);

        /* round up to 8 pixel boundary so we can convert whole bytes */
        rowbytes = ((src->bits->width * 4) + 31) & ~31;
        data = malloc(rowbytes * src->bits->height);
        free_data = TRUE;
        if (!data) {
            FatalError("Failed to allocate memory in %s\n", __func__);
        }

        if (!src->bits->emptyMask) {
            ycount = src->bits->height;
            srow = src->bits->source;
            mrow = src->bits->mask;
            drow = data;

            while (ycount-- > 0)
            {
                xcount = bits_to_bytes(src->bits->width);
                sptr = srow;
                mptr = mrow;
                dptr = drow;

                while (xcount-- > 0)
                {
                    uint8_t s, m;
                    int i;

                    s = *sptr++;
                    m = *mptr++;
                    for (i = 0; i < 8; i++) {
#if BITMAP_BIT_ORDER == MSBFirst
                        if (m & 128)
                            *dptr++ = (s & 128) ? fg_color : bg_color;
                        else
                            *dptr++ = 0;
                        s <<= 1;
                        m <<= 1;
#else
                        if (m & 1)
                            *dptr++ = (s & 1) ? fg_color : bg_color;
                        else
                            *dptr++ = 0;
                        s >>= 1;
                        m >>= 1;
#endif
                    }
                }

                srow += BitmapBytePad(src->bits->width);
                mrow += BitmapBytePad(src->bits->width);
                drow = (uint32_t *)((char *)drow + rowbytes);
            }
        }
        else {
            memset(data, 0, src->bits->height * rowbytes);
        }
    }

    err = xp_set_cursor(width, height, hot_x, hot_y, data, rowbytes);
    if (free_data)
        free(data);
    return err == Success;
}

/*
   ===========================================================================

   Pointer sprite functions

   ===========================================================================
 */

/*
 * QuartzRealizeCursor
 *  Convert the X cursor representation to native format if possible.
 */
static Bool
QuartzRealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    if (pCursor == NULL || pCursor->bits == NULL)
        return FALSE;

    /* FIXME: cache ARGB8888 representation? */

    return TRUE;
}

/*
 * QuartzUnrealizeCursor
 *  Free the storage space associated with a realized cursor.
 */
static Bool
QuartzUnrealizeCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor)
{
    return TRUE;
}

/*
 * QuartzSetCursor
 *  Set the cursor sprite and position.
 */
static void
QuartzSetCursor(DeviceIntPtr pDev, ScreenPtr pScreen, CursorPtr pCursor,
                int x,
                int y)
{
    QuartzCursorScreenPtr ScreenPriv = CURSOR_PRIV(pScreen);

    if (!XQuartzServerVisible)
        return;

    if (pCursor == NULL) {
        if (ScreenPriv->cursorVisible) {
            xp_hide_cursor();
            ScreenPriv->cursorVisible = FALSE;
        }
    }
    else {
        load_cursor(pCursor, pScreen->myNum);

        if (!ScreenPriv->cursorVisible) {
            xp_show_cursor();
            ScreenPriv->cursorVisible = TRUE;
        }
    }
}

/*
 * QuartzMoveCursor
 *  Move the cursor. This is a noop for us.
 */
static void
QuartzMoveCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{}

/*
   ===========================================================================

   Pointer screen functions

   ===========================================================================
 */

/*
 * QuartzCursorOffScreen
 */
static Bool
QuartzCursorOffScreen(ScreenPtr *pScreen, int *x, int *y)
{
    return FALSE;
}

/*
 * QuartzCrossScreen
 */
static void
QuartzCrossScreen(ScreenPtr pScreen, Bool entering)
{
    return;
}

/*
 * QuartzWarpCursor
 *  Change the cursor position without generating an event or motion history.
 *  The input coordinates (x,y) are in pScreen-local X11 coordinates.
 *
 */
static void
QuartzWarpCursor(DeviceIntPtr pDev, ScreenPtr pScreen, int x, int y)
{
    if (XQuartzServerVisible) {
        int sx, sy;

        sx = pScreen->x + darwinMainScreenX;
        sy = pScreen->y + darwinMainScreenY;

        CGWarpMouseCursorPosition(CGPointMake(sx + x, sy + y));
    }

    miPointerWarpCursor(pDev, pScreen, x, y);
    miPointerUpdateSprite(pDev);
}

static miPointerScreenFuncRec quartzScreenFuncsRec = {
    QuartzCursorOffScreen,
    QuartzCrossScreen,
    QuartzWarpCursor,
};

/*
   ===========================================================================

   Other screen functions

   ===========================================================================
 */

/*
 * QuartzCursorQueryBestSize
 *  Handle queries for best cursor size
 */
static void
QuartzCursorQueryBestSize(int class, unsigned short *width,
                          unsigned short *height, ScreenPtr pScreen)
{
    QuartzCursorScreenPtr ScreenPriv = CURSOR_PRIV(pScreen);

    if (class == CursorShape) {
        /* FIXME: query window server? */
        *width = 32;
        *height = 32;
    }
    else {
        (*ScreenPriv->QueryBestSize)(class, width, height, pScreen);
    }
}

/*
 * QuartzInitCursor
 *  Initialize cursor support
 */
Bool
QuartzInitCursor(ScreenPtr pScreen)
{
    QuartzCursorScreenPtr ScreenPriv;
    miPointerScreenPtr PointPriv;

    /* initialize software cursor handling (always needed as backup) */
    if (!miDCInitialize(pScreen, &quartzScreenFuncsRec))
        return FALSE;

    if (!dixRegisterPrivateKey(&darwinCursorScreenKeyRec, PRIVATE_SCREEN, 0))
        return FALSE;

    ScreenPriv = calloc(1, sizeof(QuartzCursorScreenRec));
    if (ScreenPriv == NULL)
        return FALSE;

    /* CURSOR_PRIV(pScreen) = ScreenPriv; */
    dixSetPrivate(&pScreen->devPrivates, darwinCursorScreenKey, ScreenPriv);

    /* override some screen procedures */
    ScreenPriv->QueryBestSize = pScreen->QueryBestSize;
    pScreen->QueryBestSize = QuartzCursorQueryBestSize;

    PointPriv = dixLookupPrivate(&pScreen->devPrivates, miPointerScreenKey);

    ScreenPriv->spriteFuncs = PointPriv->spriteFuncs;

    PointPriv->spriteFuncs->RealizeCursor = QuartzRealizeCursor;
    PointPriv->spriteFuncs->UnrealizeCursor = QuartzUnrealizeCursor;
    PointPriv->spriteFuncs->SetCursor = QuartzSetCursor;
    PointPriv->spriteFuncs->MoveCursor = QuartzMoveCursor;

    ScreenPriv->cursorVisible = TRUE;
    return TRUE;
}

/*
 * QuartzSuspendXCursor
 *  X server is hiding. Restore the Aqua cursor.
 */
void
QuartzSuspendXCursor(ScreenPtr pScreen)
{
    xp_show_cursor();
}

/*
 * QuartzResumeXCursor
 *  X server is showing. Restore the X cursor.
 */
void
QuartzResumeXCursor(ScreenPtr pScreen)
{
    WindowPtr pWin;
    CursorPtr pCursor;

    /* TODO: Tablet? */

    pWin = GetSpriteWindow(darwinPointer);
    if (pWin->drawable.pScreen != pScreen)
        return;

    pCursor = GetSpriteCursor(darwinPointer);
    if (pCursor == NULL)
        return;

    QuartzSetCursor(darwinPointer, pScreen, pCursor, /* x */ 0, /* y */ 0);
}
