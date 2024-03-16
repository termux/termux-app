/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xmd.h>
#include "servermd.h"
#include "scrnintstr.h"
#include "dixstruct.h"
#include "cursorstr.h"
#include "dixfontstr.h"
#include "opaque.h"
#include "inputstr.h"
#include "xace.h"

typedef struct _GlyphShare {
    FontPtr font;
    unsigned short sourceChar;
    unsigned short maskChar;
    CursorBitsPtr bits;
    struct _GlyphShare *next;
} GlyphShare, *GlyphSharePtr;

static GlyphSharePtr sharedGlyphs = (GlyphSharePtr) NULL;

DevScreenPrivateKeyRec cursorScreenDevPriv;

static CARD32 cursorSerial;

static void
FreeCursorBits(CursorBitsPtr bits)
{
    if (--bits->refcnt > 0)
        return;
    free(bits->source);
    free(bits->mask);
    free(bits->argb);
    dixFiniPrivates(bits, PRIVATE_CURSOR_BITS);
    if (bits->refcnt == 0) {
        GlyphSharePtr *prev, this;

        for (prev = &sharedGlyphs;
             (this = *prev) && (this->bits != bits); prev = &this->next);
        if (this) {
            *prev = this->next;
            CloseFont(this->font, (Font) 0);
            free(this);
        }
        free(bits);
    }
}

/**
 * To be called indirectly by DeleteResource; must use exactly two args.
 *
 *  \param value must conform to DeleteType
 */
int
FreeCursor(void *value, XID cid)
{
    int nscr;
    CursorPtr pCurs = (CursorPtr) value;

    ScreenPtr pscr;
    DeviceIntPtr pDev = NULL;   /* unused anyway */


    UnrefCursor(pCurs);
    if (CursorRefCount(pCurs) != 0)
        return Success;

    BUG_WARN(CursorRefCount(pCurs) < 0);

    for (nscr = 0; nscr < screenInfo.numScreens; nscr++) {
        pscr = screenInfo.screens[nscr];
        (void) (*pscr->UnrealizeCursor) (pDev, pscr, pCurs);
    }
    FreeCursorBits(pCurs->bits);
    dixFiniPrivates(pCurs, PRIVATE_CURSOR);
    free(pCurs);
    return Success;
}

CursorPtr
RefCursor(CursorPtr cursor)
{
    if (cursor)
        cursor->refcnt++;
    return cursor;
}

CursorPtr
UnrefCursor(CursorPtr cursor)
{
    if (cursor)
        cursor->refcnt--;
    return cursor;
}

int
CursorRefCount(const CursorPtr cursor)
{
    return cursor ? cursor->refcnt : 0;
}


/*
 * We check for empty cursors so that we won't have to display them
 */
static void
CheckForEmptyMask(CursorBitsPtr bits)
{
    unsigned char *msk = bits->mask;
    int n = BitmapBytePad(bits->width) * bits->height;

    bits->emptyMask = FALSE;
    while (n--)
        if (*(msk++) != 0)
            return;
    if (bits->argb) {
        CARD32 *argb = bits->argb;

        n = bits->width * bits->height;
        while (n--)
            if (*argb++ & 0xff000000)
                return;
    }
    bits->emptyMask = TRUE;
}

/**
 * realize the cursor for every screen. Do not change the refcnt, this will be
 * changed when ChangeToCursor actually changes the sprite.
 *
 * @return Success if all cursors realize on all screens, BadAlloc if realize
 * failed for a device on a given screen.
 */
static int
RealizeCursorAllScreens(CursorPtr pCurs)
{
    DeviceIntPtr pDev;
    ScreenPtr pscr;
    int nscr;

    for (nscr = 0; nscr < screenInfo.numScreens; nscr++) {
        pscr = screenInfo.screens[nscr];
        for (pDev = inputInfo.devices; pDev; pDev = pDev->next) {
            if (DevHasCursor(pDev)) {
                if (!(*pscr->RealizeCursor) (pDev, pscr, pCurs)) {
                    /* Realize failed for device pDev on screen pscr.
                     * We have to assume that for all devices before, realize
                     * worked. We need to rollback all devices so far on the
                     * current screen and then all devices on previous
                     * screens.
                     */
                    DeviceIntPtr pDevIt = inputInfo.devices;    /*dev iterator */

                    while (pDevIt && pDevIt != pDev) {
                        if (DevHasCursor(pDevIt))
                            (*pscr->UnrealizeCursor) (pDevIt, pscr, pCurs);
                        pDevIt = pDevIt->next;
                    }
                    while (--nscr >= 0) {
                        pscr = screenInfo.screens[nscr];
                        /* now unrealize all devices on previous screens */
                        pDevIt = inputInfo.devices;
                        while (pDevIt) {
                            if (DevHasCursor(pDevIt))
                                (*pscr->UnrealizeCursor) (pDevIt, pscr, pCurs);
                            pDevIt = pDevIt->next;
                        }
                        (*pscr->UnrealizeCursor) (pDev, pscr, pCurs);
                    }
                    return BadAlloc;
                }
            }
        }
    }

    return Success;
}

/**
 * does nothing about the resource table, just creates the data structure.
 * does not copy the src and mask bits
 *
 *  \param psrcbits  server-defined padding
 *  \param pmaskbits server-defined padding
 *  \param argb      no padding
 */
int
AllocARGBCursor(unsigned char *psrcbits, unsigned char *pmaskbits,
                CARD32 *argb, CursorMetricPtr cm,
                unsigned foreRed, unsigned foreGreen, unsigned foreBlue,
                unsigned backRed, unsigned backGreen, unsigned backBlue,
                CursorPtr *ppCurs, ClientPtr client, XID cid)
{
    CursorBitsPtr bits;
    CursorPtr pCurs;
    int rc;

    *ppCurs = NULL;
    pCurs = (CursorPtr) calloc(CURSOR_REC_SIZE + CURSOR_BITS_SIZE, 1);
    if (!pCurs)
        return BadAlloc;

    bits = (CursorBitsPtr) ((char *) pCurs + CURSOR_REC_SIZE);
    dixInitPrivates(pCurs, pCurs + 1, PRIVATE_CURSOR);
    dixInitPrivates(bits, bits + 1, PRIVATE_CURSOR_BITS)
        bits->source = psrcbits;
    bits->mask = pmaskbits;
    bits->argb = argb;
    bits->width = cm->width;
    bits->height = cm->height;
    bits->xhot = cm->xhot;
    bits->yhot = cm->yhot;
    pCurs->refcnt = 1;
    bits->refcnt = -1;
    CheckForEmptyMask(bits);
    pCurs->bits = bits;
    pCurs->serialNumber = ++cursorSerial;
    pCurs->name = None;

    pCurs->foreRed = foreRed;
    pCurs->foreGreen = foreGreen;
    pCurs->foreBlue = foreBlue;

    pCurs->backRed = backRed;
    pCurs->backGreen = backGreen;
    pCurs->backBlue = backBlue;

    pCurs->id = cid;

    /* security creation/labeling check */
    rc = XaceHook(XACE_RESOURCE_ACCESS, client, cid, RT_CURSOR,
                  pCurs, RT_NONE, NULL, DixCreateAccess);
    if (rc != Success)
        goto error;

    rc = RealizeCursorAllScreens(pCurs);
    if (rc != Success)
        goto error;

    *ppCurs = pCurs;

    if (argb) {
        size_t i, size = bits->width * bits->height;

        for (i = 0; i < size; i++) {
            if ((argb[i] & 0xff000000) == 0 && (argb[i] & 0xffffff) != 0) {
                /* ARGB data doesn't seem pre-multiplied, fix it */
                for (i = 0; i < size; i++) {
                    CARD32 a, ar, ag, ab;

                    a = argb[i] >> 24;
                    ar = a * ((argb[i] >> 16) & 0xff) / 0xff;
                    ag = a * ((argb[i] >> 8) & 0xff) / 0xff;
                    ab = a * (argb[i] & 0xff) / 0xff;

                    argb[i] = a << 24 | ar << 16 | ag << 8 | ab;
                }

                break;
            }
        }
    }

    return Success;

 error:
    FreeCursorBits(bits);
    dixFiniPrivates(pCurs, PRIVATE_CURSOR);
    free(pCurs);

    return rc;
}

int
AllocGlyphCursor(Font source, unsigned sourceChar, Font mask, unsigned maskChar,
                 unsigned foreRed, unsigned foreGreen, unsigned foreBlue,
                 unsigned backRed, unsigned backGreen, unsigned backBlue,
                 CursorPtr *ppCurs, ClientPtr client, XID cid)
{
    FontPtr sourcefont, maskfont;
    unsigned char *srcbits;
    unsigned char *mskbits;
    CursorMetricRec cm;
    int rc;
    CursorBitsPtr bits;
    CursorPtr pCurs;
    GlyphSharePtr pShare;

    rc = dixLookupResourceByType((void **) &sourcefont, source, RT_FONT,
                                 client, DixUseAccess);
    if (rc != Success) {
        client->errorValue = source;
        return rc;
    }
    rc = dixLookupResourceByType((void **) &maskfont, mask, RT_FONT, client,
                                 DixUseAccess);
    if (rc != Success && mask != None) {
        client->errorValue = mask;
        return rc;
    }
    if (sourcefont != maskfont)
        pShare = (GlyphSharePtr) NULL;
    else {
        for (pShare = sharedGlyphs;
             pShare &&
             ((pShare->font != sourcefont) ||
              (pShare->sourceChar != sourceChar) ||
              (pShare->maskChar != maskChar)); pShare = pShare->next);
    }
    if (pShare) {
        pCurs = (CursorPtr) calloc(CURSOR_REC_SIZE, 1);
        if (!pCurs)
            return BadAlloc;
        dixInitPrivates(pCurs, pCurs + 1, PRIVATE_CURSOR);
        bits = pShare->bits;
        bits->refcnt++;
    }
    else {
        if (!CursorMetricsFromGlyph(sourcefont, sourceChar, &cm)) {
            client->errorValue = sourceChar;
            return BadValue;
        }
        if (!maskfont) {
            long n;
            unsigned char *mskptr;

            n = BitmapBytePad(cm.width) * (long) cm.height;
            mskptr = mskbits = malloc(n);
            if (!mskptr)
                return BadAlloc;
            while (--n >= 0)
                *mskptr++ = ~0;
        }
        else {
            if (!CursorMetricsFromGlyph(maskfont, maskChar, &cm)) {
                client->errorValue = maskChar;
                return BadValue;
            }
            if ((rc = ServerBitsFromGlyph(maskfont, maskChar, &cm, &mskbits)))
                return rc;
        }
        if ((rc = ServerBitsFromGlyph(sourcefont, sourceChar, &cm, &srcbits))) {
            free(mskbits);
            return rc;
        }
        if (sourcefont != maskfont) {
            pCurs = (CursorPtr) calloc(CURSOR_REC_SIZE + CURSOR_BITS_SIZE, 1);
            if (pCurs)
                bits = (CursorBitsPtr) ((char *) pCurs + CURSOR_REC_SIZE);
            else
                bits = (CursorBitsPtr) NULL;
        }
        else {
            pCurs = (CursorPtr) calloc(CURSOR_REC_SIZE, 1);
            if (pCurs)
                bits = (CursorBitsPtr) calloc(CURSOR_BITS_SIZE, 1);
            else
                bits = (CursorBitsPtr) NULL;
        }
        if (!bits) {
            free(pCurs);
            free(mskbits);
            free(srcbits);
            return BadAlloc;
        }
        dixInitPrivates(pCurs, pCurs + 1, PRIVATE_CURSOR);
        dixInitPrivates(bits, bits + 1, PRIVATE_CURSOR_BITS);
        bits->source = srcbits;
        bits->mask = mskbits;
        bits->argb = 0;
        bits->width = cm.width;
        bits->height = cm.height;
        bits->xhot = cm.xhot;
        bits->yhot = cm.yhot;
        if (sourcefont != maskfont)
            bits->refcnt = -1;
        else {
            bits->refcnt = 1;
            pShare = malloc(sizeof(GlyphShare));
            if (!pShare) {
                FreeCursorBits(bits);
                return BadAlloc;
            }
            pShare->font = sourcefont;
            sourcefont->refcnt++;
            pShare->sourceChar = sourceChar;
            pShare->maskChar = maskChar;
            pShare->bits = bits;
            pShare->next = sharedGlyphs;
            sharedGlyphs = pShare;
        }
    }

    CheckForEmptyMask(bits);
    pCurs->bits = bits;
    pCurs->refcnt = 1;
    pCurs->serialNumber = ++cursorSerial;
    pCurs->name = None;

    pCurs->foreRed = foreRed;
    pCurs->foreGreen = foreGreen;
    pCurs->foreBlue = foreBlue;

    pCurs->backRed = backRed;
    pCurs->backGreen = backGreen;
    pCurs->backBlue = backBlue;

    pCurs->id = cid;

    /* security creation/labeling check */
    rc = XaceHook(XACE_RESOURCE_ACCESS, client, cid, RT_CURSOR,
                  pCurs, RT_NONE, NULL, DixCreateAccess);
    if (rc != Success)
        goto error;

    rc = RealizeCursorAllScreens(pCurs);
    if (rc != Success)
        goto error;

    *ppCurs = pCurs;
    return Success;

 error:
    FreeCursorBits(bits);
    dixFiniPrivates(pCurs, PRIVATE_CURSOR);
    free(pCurs);

    return rc;
}

/** CreateRootCursor
 *
 * look up the name of a font
 * open the font
 * add the font to the resource table
 * make a cursor from the glyphs
 * add the cursor to the resource table
 *************************************************************/

CursorPtr
CreateRootCursor(char *unused1, unsigned int unused2)
{
    CursorPtr curs;
    FontPtr cursorfont;
    int err;
    XID fontID;
    const char defaultCursorFont[] = "cursor";

    fontID = FakeClientID(0);
    err = OpenFont(serverClient, fontID, FontLoadAll | FontOpenSync,
                   (unsigned) strlen(defaultCursorFont), defaultCursorFont);
    if (err != Success)
        return NullCursor;

    err = dixLookupResourceByType((void **) &cursorfont, fontID, RT_FONT,
                                  serverClient, DixReadAccess);
    if (err != Success)
        return NullCursor;
    if (AllocGlyphCursor(fontID, 0, fontID, 1, 0, 0, 0, ~0, ~0, ~0,
                         &curs, serverClient, (XID) 0) != Success)
        return NullCursor;

    if (!AddResource(FakeClientID(0), RT_CURSOR, (void *) curs))
        return NullCursor;

    return curs;
}
