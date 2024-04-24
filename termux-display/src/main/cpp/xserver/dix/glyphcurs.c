/************************************************************************

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

************************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "misc.h"
#include <X11/fonts/fontstruct.h>
#include "dixfontstr.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "resource.h"
#include "dix.h"
#include "cursorstr.h"
#include "opaque.h"
#include "servermd.h"

/*
    get the bits out of the font in a portable way.  to avoid
dealing with padding and such-like, we draw the glyph into
a bitmap, then read the bits out with GetImage, which
uses server-natural format.
    since all screens return the same bitmap format, we'll just use
the first one we find.
    the character origin lines up with the hotspot in the
cursor metrics.
*/

int
ServerBitsFromGlyph(FontPtr pfont, unsigned ch, CursorMetricPtr cm,
                    unsigned char **ppbits)
{
    ScreenPtr pScreen;
    GCPtr pGC;
    xRectangle rect;
    PixmapPtr ppix;
    char *pbits;
    ChangeGCVal gcval[3];
    unsigned char char2b[2];

    /* turn glyph index into a protocol-format char2b */
    char2b[0] = (unsigned char) (ch >> 8);
    char2b[1] = (unsigned char) (ch & 0xff);

    pScreen = screenInfo.screens[0];
    pbits = calloc(BitmapBytePad(cm->width), cm->height);
    if (!pbits)
        return BadAlloc;

    ppix = (PixmapPtr) (*pScreen->CreatePixmap) (pScreen, cm->width,
                                                 cm->height, 1,
                                                 CREATE_PIXMAP_USAGE_SCRATCH);
    pGC = GetScratchGC(1, pScreen);
    if (!ppix || !pGC) {
        if (ppix)
            (*pScreen->DestroyPixmap) (ppix);
        if (pGC)
            FreeScratchGC(pGC);
        free(pbits);
        return BadAlloc;
    }

    rect.x = 0;
    rect.y = 0;
    rect.width = cm->width;
    rect.height = cm->height;

    /* fill the pixmap with 0 */
    gcval[0].val = GXcopy;
    gcval[1].val = 0;
    gcval[2].ptr = (void *) pfont;
    ChangeGC(NullClient, pGC, GCFunction | GCForeground | GCFont, gcval);
    ValidateGC((DrawablePtr) ppix, pGC);
    (*pGC->ops->PolyFillRect) ((DrawablePtr) ppix, pGC, 1, &rect);

    /* draw the glyph */
    gcval[0].val = 1;
    ChangeGC(NullClient, pGC, GCForeground, gcval);
    ValidateGC((DrawablePtr) ppix, pGC);
    (*pGC->ops->PolyText16) ((DrawablePtr) ppix, pGC, cm->xhot, cm->yhot,
                             1, (unsigned short *) char2b);
    (*pScreen->GetImage) ((DrawablePtr) ppix, 0, 0, cm->width, cm->height,
                          XYPixmap, 1, pbits);
    *ppbits = (unsigned char *) pbits;
    FreeScratchGC(pGC);
    (*pScreen->DestroyPixmap) (ppix);
    return Success;
}

Bool
CursorMetricsFromGlyph(FontPtr pfont, unsigned ch, CursorMetricPtr cm)
{
    CharInfoPtr pci;
    unsigned long nglyphs;
    CARD8 chs[2];
    FontEncoding encoding;

    chs[0] = ch >> 8;
    chs[1] = ch;
    encoding = (FONTLASTROW(pfont) == 0) ? Linear16Bit : TwoD16Bit;
    if (encoding == Linear16Bit) {
        if (ch < pfont->info.firstCol || pfont->info.lastCol < ch)
            return FALSE;
    }
    else {
        if (chs[0] < pfont->info.firstRow || pfont->info.lastRow < chs[0])
            return FALSE;
        if (chs[1] < pfont->info.firstCol || pfont->info.lastCol < chs[1])
            return FALSE;
    }
    (*pfont->get_glyphs) (pfont, 1, chs, encoding, &nglyphs, &pci);
    if (nglyphs == 0)
        return FALSE;
    cm->width = pci->metrics.rightSideBearing - pci->metrics.leftSideBearing;
    cm->height = pci->metrics.descent + pci->metrics.ascent;
    if (pci->metrics.leftSideBearing > 0) {
        cm->width += pci->metrics.leftSideBearing;
        cm->xhot = 0;
    }
    else {
        cm->xhot = -pci->metrics.leftSideBearing;
        if (pci->metrics.rightSideBearing < 0)
            cm->width -= pci->metrics.rightSideBearing;
    }
    if (pci->metrics.ascent < 0) {
        cm->height -= pci->metrics.ascent;
        cm->yhot = 0;
    }
    else {
        cm->yhot = pci->metrics.ascent;
        if (pci->metrics.descent < 0)
            cm->height -= pci->metrics.descent;
    }
    return TRUE;
}
