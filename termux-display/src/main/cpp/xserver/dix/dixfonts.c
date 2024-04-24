/************************************************************************
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
/* The panoramix components contained the following notice */
/*
Copyright (c) 1991, 1997 Digital Equipment Corporation, Maynard, Massachusetts.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
DIGITAL EQUIPMENT CORPORATION BE LIABLE FOR ANY CLAIM, DAMAGES, INCLUDING,
BUT NOT LIMITED TO CONSEQUENTIAL OR INCIDENTAL DAMAGES, OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Digital Equipment Corporation
shall not be used in advertising or otherwise to promote the sale, use or other
dealings in this Software without prior written authorization from Digital
Equipment Corporation.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xmd.h>
#include <X11/Xproto.h>
#include "scrnintstr.h"
#include "resource.h"
#include "dixstruct.h"
#include "cursorstr.h"
#include "misc.h"
#include "opaque.h"
#include "dixfontstr.h"
#include "closestr.h"
#include "dixfont.h"
#include "xace.h"
#include <X11/fonts/libxfont2.h>

#ifdef XF86BIGFONT
#include "xf86bigfontsrv.h"
#endif

extern void *fosNaturalParams;
extern FontPtr defaultFont;

static FontPathElementPtr *font_path_elements = (FontPathElementPtr *) 0;
static int num_fpes = 0;
static xfont2_fpe_funcs_rec const **fpe_functions;
static int num_fpe_types = 0;

static unsigned char *font_path_string;

static int num_slept_fpes = 0;
static int size_slept_fpes = 0;
static FontPathElementPtr *slept_fpes = (FontPathElementPtr *) 0;
static xfont2_pattern_cache_ptr patternCache;

static int
FontToXError(int err)
{
    switch (err) {
    case Successful:
        return Success;
    case AllocError:
        return BadAlloc;
    case BadFontName:
        return BadName;
    case BadFontPath:
    case BadFontFormat:        /* is there something better? */
    case BadCharRange:
        return BadValue;
    default:
        return err;
    }
}

static int
LoadGlyphs(ClientPtr client, FontPtr pfont, unsigned nchars, int item_size,
           unsigned char *data)
{
    if (fpe_functions[pfont->fpe->type]->load_glyphs)
        return (*fpe_functions[pfont->fpe->type]->load_glyphs)
            (client, pfont, 0, nchars, item_size, data);
    else
        return Successful;
}

void
GetGlyphs(FontPtr font, unsigned long count, unsigned char *chars,
          FontEncoding fontEncoding,
          unsigned long *glyphcount,    /* RETURN */
          CharInfoPtr *glyphs)          /* RETURN */
{
    (*font->get_glyphs) (font, count, chars, fontEncoding, glyphcount, glyphs);
}

/*
 * adding RT_FONT prevents conflict with default cursor font
 */
Bool
SetDefaultFont(const char *defaultfontname)
{
    int err;
    FontPtr pf;
    XID fid;

    fid = FakeClientID(0);
    err = OpenFont(serverClient, fid, FontLoadAll | FontOpenSync,
                   (unsigned) strlen(defaultfontname), defaultfontname);
    if (err != Success)
        return FALSE;
    err = dixLookupResourceByType((void **) &pf, fid, RT_FONT, serverClient,
                                  DixReadAccess);
    if (err != Success)
        return FALSE;
    defaultFont = pf;
    return TRUE;
}

/*
 * note that the font wakeup queue is not refcounted.  this is because
 * an fpe needs to be added when it's inited, and removed when it's finally
 * freed, in order to handle any data that isn't requested, like FS events.
 *
 * since the only thing that should call these routines is the renderer's
 * init_fpe() and free_fpe(), there shouldn't be any problem in using
 * freed data.
 */
static void
QueueFontWakeup(FontPathElementPtr fpe)
{
    int i;
    FontPathElementPtr *new;

    for (i = 0; i < num_slept_fpes; i++) {
        if (slept_fpes[i] == fpe) {
            return;
        }
    }
    if (num_slept_fpes == size_slept_fpes) {
        new = reallocarray(slept_fpes, size_slept_fpes + 4,
                           sizeof(FontPathElementPtr));
        if (!new)
            return;
        slept_fpes = new;
        size_slept_fpes += 4;
    }
    slept_fpes[num_slept_fpes] = fpe;
    num_slept_fpes++;
}

static void
RemoveFontWakeup(FontPathElementPtr fpe)
{
    int i, j;

    for (i = 0; i < num_slept_fpes; i++) {
        if (slept_fpes[i] == fpe) {
            for (j = i; j < num_slept_fpes; j++) {
                slept_fpes[j] = slept_fpes[j + 1];
            }
            num_slept_fpes--;
            return;
        }
    }
}

static void
FontWakeup(void *data, int count)
{
    int i;
    FontPathElementPtr fpe;

    if (count < 0)
        return;
    /* wake up any fpe's that may be waiting for information */
    for (i = 0; i < num_slept_fpes; i++) {
        fpe = slept_fpes[i];
        (void) (*fpe_functions[fpe->type]->wakeup_fpe) (fpe);
    }
}

/* XXX -- these two funcs may want to be broken into macros */
static void
UseFPE(FontPathElementPtr fpe)
{
    fpe->refcount++;
}

static void
FreeFPE(FontPathElementPtr fpe)
{
    fpe->refcount--;
    if (fpe->refcount == 0) {
        (*fpe_functions[fpe->type]->free_fpe) (fpe);
        free((void *) fpe->name);
        free(fpe);
    }
}

static Bool
doOpenFont(ClientPtr client, OFclosurePtr c)
{
    FontPtr pfont = NullFont;
    FontPathElementPtr fpe = NULL;
    ScreenPtr pScr;
    int err = Successful;
    int i;
    char *alias, *newname;
    int newlen;
    int aliascount = 20;

    /*
     * Decide at runtime what FontFormat to use.
     */
    Mask FontFormat =
        ((screenInfo.imageByteOrder == LSBFirst) ?
         BitmapFormatByteOrderLSB : BitmapFormatByteOrderMSB) |
        ((screenInfo.bitmapBitOrder == LSBFirst) ?
         BitmapFormatBitOrderLSB : BitmapFormatBitOrderMSB) |
        BitmapFormatImageRectMin |
#if GLYPHPADBYTES == 1
        BitmapFormatScanlinePad8 |
#endif
#if GLYPHPADBYTES == 2
        BitmapFormatScanlinePad16 |
#endif
#if GLYPHPADBYTES == 4
        BitmapFormatScanlinePad32 |
#endif
#if GLYPHPADBYTES == 8
        BitmapFormatScanlinePad64 |
#endif
        BitmapFormatScanlineUnit8;

    if (client->clientGone) {
        if (c->current_fpe < c->num_fpes) {
            fpe = c->fpe_list[c->current_fpe];
            (*fpe_functions[fpe->type]->client_died) ((void *) client, fpe);
        }
        err = Successful;
        goto bail;
    }
    while (c->current_fpe < c->num_fpes) {
        fpe = c->fpe_list[c->current_fpe];
        err = (*fpe_functions[fpe->type]->open_font)
            ((void *) client, fpe, c->flags,
             c->fontname, c->fnamelen, FontFormat,
             BitmapFormatMaskByte |
             BitmapFormatMaskBit |
             BitmapFormatMaskImageRectangle |
             BitmapFormatMaskScanLinePad |
             BitmapFormatMaskScanLineUnit,
             c->fontid, &pfont, &alias,
             c->non_cachable_font && c->non_cachable_font->fpe == fpe ?
             c->non_cachable_font : (FontPtr) 0);

        if (err == FontNameAlias && alias) {
            newlen = strlen(alias);
            newname = (char *) realloc((char *) c->fontname, newlen);
            if (!newname) {
                err = AllocError;
                break;
            }
            memmove(newname, alias, newlen);
            c->fontname = newname;
            c->fnamelen = newlen;
            c->current_fpe = 0;
            if (--aliascount <= 0) {
                /* We've tried resolving this alias 20 times, we're
                 * probably stuck in an infinite loop of aliases pointing
                 * to each other - time to take emergency exit!
                 */
                err = BadImplementation;
                break;
            }
            continue;
        }
        if (err == BadFontName) {
            c->current_fpe++;
            continue;
        }
        if (err == Suspended) {
            if (!ClientIsAsleep(client))
                ClientSleep(client, (ClientSleepProcPtr) doOpenFont, c);
            return TRUE;
        }
        break;
    }

    if (err != Successful)
        goto bail;
    if (!pfont) {
        err = BadFontName;
        goto bail;
    }
    /* check values for firstCol, lastCol, firstRow, and lastRow */
    if (pfont->info.firstCol > pfont->info.lastCol ||
        pfont->info.firstRow > pfont->info.lastRow ||
        pfont->info.lastCol - pfont->info.firstCol > 255) {
        err = AllocError;
        goto bail;
    }
    if (!pfont->fpe)
        pfont->fpe = fpe;
    pfont->refcnt++;
    if (pfont->refcnt == 1) {
        UseFPE(pfont->fpe);
        for (i = 0; i < screenInfo.numScreens; i++) {
            pScr = screenInfo.screens[i];
            if (pScr->RealizeFont) {
                if (!(*pScr->RealizeFont) (pScr, pfont)) {
                    CloseFont(pfont, (Font) 0);
                    err = AllocError;
                    goto bail;
                }
            }
        }
    }
    if (!AddResource(c->fontid, RT_FONT, (void *) pfont)) {
        err = AllocError;
        goto bail;
    }
    if (patternCache && pfont != c->non_cachable_font)
        xfont2_cache_font_pattern(patternCache, c->origFontName, c->origFontNameLen,
                                  pfont);
 bail:
    if (err != Successful && c->client != serverClient) {
        SendErrorToClient(c->client, X_OpenFont, 0,
                          c->fontid, FontToXError(err));
    }
    ClientWakeup(c->client);
    for (i = 0; i < c->num_fpes; i++) {
        FreeFPE(c->fpe_list[i]);
    }
    free(c->fpe_list);
    free((void *) c->fontname);
    free(c);
    return TRUE;
}

int
OpenFont(ClientPtr client, XID fid, Mask flags, unsigned lenfname,
         const char *pfontname)
{
    OFclosurePtr c;
    int i;
    FontPtr cached = (FontPtr) 0;

    if (!lenfname || lenfname > XLFDMAXFONTNAMELEN)
        return BadName;
    if (patternCache) {

        /*
         ** Check name cache.  If we find a cached version of this font that
         ** is cachable, immediately satisfy the request with it.  If we find
         ** a cached version of this font that is non-cachable, we do not
         ** satisfy the request with it.  Instead, we pass the FontPtr to the
         ** FPE's open_font code (the fontfile FPE in turn passes the
         ** information to the rasterizer; the fserve FPE ignores it).
         **
         ** Presumably, the font is marked non-cachable because the FPE has
         ** put some licensing restrictions on it.  If the FPE, using
         ** whatever logic it relies on, determines that it is willing to
         ** share this existing font with the client, then it has the option
         ** to return the FontPtr we passed it as the newly-opened font.
         ** This allows the FPE to exercise its licensing logic without
         ** having to create another instance of a font that already exists.
         */

        cached = xfont2_find_cached_font_pattern(patternCache, pfontname, lenfname);
        if (cached && cached->info.cachable) {
            if (!AddResource(fid, RT_FONT, (void *) cached))
                return BadAlloc;
            cached->refcnt++;
            return Success;
        }
    }
    c = malloc(sizeof(OFclosureRec));
    if (!c)
        return BadAlloc;
    c->fontname = malloc(lenfname);
    c->origFontName = pfontname;
    c->origFontNameLen = lenfname;
    if (!c->fontname) {
        free(c);
        return BadAlloc;
    }
    /*
     * copy the current FPE list, so that if it gets changed by another client
     * while we're blocking, the request still appears atomic
     */
    c->fpe_list = xallocarray(num_fpes, sizeof(FontPathElementPtr));
    if (!c->fpe_list) {
        free((void *) c->fontname);
        free(c);
        return BadAlloc;
    }
    memmove(c->fontname, pfontname, lenfname);
    for (i = 0; i < num_fpes; i++) {
        c->fpe_list[i] = font_path_elements[i];
        UseFPE(c->fpe_list[i]);
    }
    c->client = client;
    c->fontid = fid;
    c->current_fpe = 0;
    c->num_fpes = num_fpes;
    c->fnamelen = lenfname;
    c->flags = flags;
    c->non_cachable_font = cached;

    (void) doOpenFont(client, c);
    return Success;
}

/**
 * Decrement font's ref count, and free storage if ref count equals zero
 *
 *  \param value must conform to DeleteType
 */
int
CloseFont(void *value, XID fid)
{
    int nscr;
    ScreenPtr pscr;
    FontPathElementPtr fpe;
    FontPtr pfont = (FontPtr) value;

    if (pfont == NullFont)
        return Success;
    if (--pfont->refcnt == 0) {
        if (patternCache)
            xfont2_remove_cached_font_pattern(patternCache, pfont);
        /*
         * since the last reference is gone, ask each screen to free any
         * storage it may have allocated locally for it.
         */
        for (nscr = 0; nscr < screenInfo.numScreens; nscr++) {
            pscr = screenInfo.screens[nscr];
            if (pscr->UnrealizeFont)
                (*pscr->UnrealizeFont) (pscr, pfont);
        }
        if (pfont == defaultFont)
            defaultFont = NULL;
#ifdef XF86BIGFONT
        XF86BigfontFreeFontShm(pfont);
#endif
        fpe = pfont->fpe;
        (*fpe_functions[fpe->type]->close_font) (fpe, pfont);
        FreeFPE(fpe);
    }
    return Success;
}

/***====================================================================***/

/**
 * Sets up pReply as the correct QueryFontReply for pFont with the first
 * nProtoCCIStructs char infos.
 *
 *  \param pReply caller must allocate this storage
  */
void
QueryFont(FontPtr pFont, xQueryFontReply * pReply, int nProtoCCIStructs)
{
    FontPropPtr pFP;
    int r, c, i;
    xFontProp *prFP;
    xCharInfo *prCI;
    xCharInfo *charInfos[256];
    unsigned char chars[512];
    int ninfos;
    unsigned long ncols;
    unsigned long count;

    /* pr->length set in dispatch */
    pReply->minCharOrByte2 = pFont->info.firstCol;
    pReply->defaultChar = pFont->info.defaultCh;
    pReply->maxCharOrByte2 = pFont->info.lastCol;
    pReply->drawDirection = pFont->info.drawDirection;
    pReply->allCharsExist = pFont->info.allExist;
    pReply->minByte1 = pFont->info.firstRow;
    pReply->maxByte1 = pFont->info.lastRow;
    pReply->fontAscent = pFont->info.fontAscent;
    pReply->fontDescent = pFont->info.fontDescent;

    pReply->minBounds = pFont->info.ink_minbounds;
    pReply->maxBounds = pFont->info.ink_maxbounds;

    pReply->nFontProps = pFont->info.nprops;
    pReply->nCharInfos = nProtoCCIStructs;

    for (i = 0, pFP = pFont->info.props, prFP = (xFontProp *) (&pReply[1]);
         i < pFont->info.nprops; i++, pFP++, prFP++) {
        prFP->name = pFP->name;
        prFP->value = pFP->value;
    }

    ninfos = 0;
    ncols = (unsigned long) (pFont->info.lastCol - pFont->info.firstCol + 1);
    prCI = (xCharInfo *) (prFP);
    for (r = pFont->info.firstRow;
         ninfos < nProtoCCIStructs && r <= (int) pFont->info.lastRow; r++) {
        i = 0;
        for (c = pFont->info.firstCol; c <= (int) pFont->info.lastCol; c++) {
            chars[i++] = r;
            chars[i++] = c;
        }
        (*pFont->get_metrics) (pFont, ncols, chars,
                               TwoD16Bit, &count, charInfos);
        i = 0;
        for (i = 0; i < (int) count && ninfos < nProtoCCIStructs; i++) {
            *prCI = *charInfos[i];
            prCI++;
            ninfos++;
        }
    }
    return;
}

static Bool
doListFontsAndAliases(ClientPtr client, LFclosurePtr c)
{
    FontPathElementPtr fpe;
    int err = Successful;
    FontNamesPtr names = NULL;
    char *name, *resolved = NULL;
    int namelen, resolvedlen;
    int nnames;
    int stringLens;
    int i;
    xListFontsReply reply;
    char *bufptr;
    char *bufferStart;
    int aliascount = 0;

    if (client->clientGone) {
        if (c->current.current_fpe < c->num_fpes) {
            fpe = c->fpe_list[c->current.current_fpe];
            (*fpe_functions[fpe->type]->client_died) ((void *) client, fpe);
        }
        err = Successful;
        goto bail;
    }

    if (!c->current.patlen)
        goto finish;

    while (c->current.current_fpe < c->num_fpes) {
        fpe = c->fpe_list[c->current.current_fpe];
        err = Successful;

        if (!fpe_functions[fpe->type]->start_list_fonts_and_aliases) {
            /* This FPE doesn't support/require list_fonts_and_aliases */

            err = (*fpe_functions[fpe->type]->list_fonts)
                ((void *) c->client, fpe, c->current.pattern,
                 c->current.patlen, c->current.max_names - c->names->nnames,
                 c->names);

            if (err == Suspended) {
                if (!ClientIsAsleep(client))
                    ClientSleep(client,
                                (ClientSleepProcPtr) doListFontsAndAliases, c);
                return TRUE;
            }

            err = BadFontName;
        }
        else {
            /* Start of list_fonts_and_aliases functionality.  Modeled
               after list_fonts_with_info in that it resolves aliases,
               except that the information collected from FPEs is just
               names, not font info.  Each list_next_font_or_alias()
               returns either a name into name/namelen or an alias into
               name/namelen and its target name into resolved/resolvedlen.
               The code at this level then resolves the alias by polling
               the FPEs.  */

            if (!c->current.list_started) {
                err = (*fpe_functions[fpe->type]->start_list_fonts_and_aliases)
                    ((void *) c->client, fpe, c->current.pattern,
                     c->current.patlen, c->current.max_names - c->names->nnames,
                     &c->current.private);
                if (err == Suspended) {
                    if (!ClientIsAsleep(client))
                        ClientSleep(client,
                                    (ClientSleepProcPtr) doListFontsAndAliases,
                                    c);
                    return TRUE;
                }
                if (err == Successful)
                    c->current.list_started = TRUE;
            }
            if (err == Successful) {
                char *tmpname;

                name = 0;
                err = (*fpe_functions[fpe->type]->list_next_font_or_alias)
                    ((void *) c->client, fpe, &name, &namelen, &tmpname,
                     &resolvedlen, c->current.private);
                if (err == Suspended) {
                    if (!ClientIsAsleep(client))
                        ClientSleep(client,
                                    (ClientSleepProcPtr) doListFontsAndAliases,
                                    c);
                    return TRUE;
                }
                if (err == FontNameAlias) {
                    free(resolved);
                    resolved = malloc(resolvedlen + 1);
                    if (resolved)
                        memmove(resolved, tmpname, resolvedlen + 1);
                }
            }

            if (err == Successful) {
                if (c->haveSaved) {
                    if (c->savedName)
                        (void) xfont2_add_font_names_name(c->names, c->savedName,
                                                c->savedNameLen);
                }
                else
                    (void) xfont2_add_font_names_name(c->names, name, namelen);
            }

            /*
             * When we get an alias back, save our state and reset back to
             * the start of the FPE looking for the specified name.  As
             * soon as a real font is found for the alias, pop back to the
             * old state
             */
            else if (err == FontNameAlias) {
                char tmp_pattern[XLFDMAXFONTNAMELEN];

                /*
                 * when an alias recurses, we need to give
                 * the last FPE a chance to clean up; so we call
                 * it again, and assume that the error returned
                 * is BadFontName, indicating the alias resolution
                 * is complete.
                 */
                memmove(tmp_pattern, resolved, resolvedlen);
                if (c->haveSaved) {
                    char *tmpname;
                    int tmpnamelen;

                    tmpname = 0;
                    (void) (*fpe_functions[fpe->type]->list_next_font_or_alias)
                        ((void *) c->client, fpe, &tmpname, &tmpnamelen,
                         &tmpname, &tmpnamelen, c->current.private);
                    if (--aliascount <= 0) {
                        err = BadFontName;
                        goto ContBadFontName;
                    }
                }
                else {
                    c->saved = c->current;
                    c->haveSaved = TRUE;
                    free(c->savedName);
                    c->savedName = malloc(namelen + 1);
                    if (c->savedName)
                        memmove(c->savedName, name, namelen + 1);
                    c->savedNameLen = namelen;
                    aliascount = 20;
                }
                memmove(c->current.pattern, tmp_pattern, resolvedlen);
                c->current.patlen = resolvedlen;
                c->current.max_names = c->names->nnames + 1;
                c->current.current_fpe = -1;
                c->current.private = 0;
                err = BadFontName;
            }
        }
        /*
         * At the end of this FPE, step to the next.  If we've finished
         * processing an alias, pop state back. If we've collected enough
         * font names, quit.
         */
        if (err == BadFontName) {
 ContBadFontName:;
            c->current.list_started = FALSE;
            c->current.current_fpe++;
            err = Successful;
            if (c->haveSaved) {
                if (c->names->nnames == c->current.max_names ||
                    c->current.current_fpe == c->num_fpes) {
                    c->haveSaved = FALSE;
                    c->current = c->saved;
                    /* Give the saved namelist a chance to clean itself up */
                    continue;
                }
            }
            if (c->names->nnames == c->current.max_names)
                break;
        }
    }

    /*
     * send the reply
     */
    if (err != Successful) {
        SendErrorToClient(client, X_ListFonts, 0, 0, FontToXError(err));
        goto bail;
    }

 finish:

    names = c->names;
    nnames = names->nnames;
    client = c->client;
    stringLens = 0;
    for (i = 0; i < nnames; i++)
        stringLens += (names->length[i] <= 255) ? names->length[i] : 0;

    reply = (xListFontsReply) {
        .type = X_Reply,
        .length = bytes_to_int32(stringLens + nnames),
        .nFonts = nnames,
        .sequenceNumber = client->sequence
    };

    bufptr = bufferStart = malloc(reply.length << 2);

    if (!bufptr && reply.length) {
        SendErrorToClient(client, X_ListFonts, 0, 0, BadAlloc);
        goto bail;
    }
    /*
     * since WriteToClient long word aligns things, copy to temp buffer and
     * write all at once
     */
    for (i = 0; i < nnames; i++) {
        if (names->length[i] > 255)
            reply.nFonts--;
        else {
            *bufptr++ = names->length[i];
            memmove(bufptr, names->names[i], names->length[i]);
            bufptr += names->length[i];
        }
    }
    nnames = reply.nFonts;
    reply.length = bytes_to_int32(stringLens + nnames);
    client->pSwapReplyFunc = ReplySwapVector[X_ListFonts];
    WriteSwappedDataToClient(client, sizeof(xListFontsReply), &reply);
    WriteToClient(client, stringLens + nnames, bufferStart);
    free(bufferStart);

 bail:
    ClientWakeup(client);
    for (i = 0; i < c->num_fpes; i++)
        FreeFPE(c->fpe_list[i]);
    free(c->fpe_list);
    free(c->savedName);
    xfont2_free_font_names(names);
    free(c);
    free(resolved);
    return TRUE;
}

int
ListFonts(ClientPtr client, unsigned char *pattern, unsigned length,
          unsigned max_names)
{
    int i;
    LFclosurePtr c;

    /*
     * The right error to return here would be BadName, however the
     * specification does not allow for a Name error on this request.
     * Perhaps a better solution would be to return a nil list, i.e.
     * a list containing zero fontnames.
     */
    if (length > XLFDMAXFONTNAMELEN)
        return BadAlloc;

    i = XaceHook(XACE_SERVER_ACCESS, client, DixGetAttrAccess);
    if (i != Success)
        return i;

    if (!(c = malloc(sizeof *c)))
        return BadAlloc;
    c->fpe_list = xallocarray(num_fpes, sizeof(FontPathElementPtr));
    if (!c->fpe_list) {
        free(c);
        return BadAlloc;
    }
    c->names = xfont2_make_font_names_record(max_names < 100 ? max_names : 100);
    if (!c->names) {
        free(c->fpe_list);
        free(c);
        return BadAlloc;
    }
    memmove(c->current.pattern, pattern, length);
    for (i = 0; i < num_fpes; i++) {
        c->fpe_list[i] = font_path_elements[i];
        UseFPE(c->fpe_list[i]);
    }
    c->client = client;
    c->num_fpes = num_fpes;
    c->current.patlen = length;
    c->current.current_fpe = 0;
    c->current.max_names = max_names;
    c->current.list_started = FALSE;
    c->current.private = 0;
    c->haveSaved = FALSE;
    c->savedName = 0;
    doListFontsAndAliases(client, c);
    return Success;
}

static int
doListFontsWithInfo(ClientPtr client, LFWIclosurePtr c)
{
    FontPathElementPtr fpe;
    int err = Successful;
    char *name;
    int namelen;
    int numFonts;
    FontInfoRec fontInfo, *pFontInfo;
    xListFontsWithInfoReply *reply;
    int length;
    xFontProp *pFP;
    int i;
    int aliascount = 0;
    xListFontsWithInfoReply finalReply;

    if (client->clientGone) {
        if (c->current.current_fpe < c->num_fpes) {
            fpe = c->fpe_list[c->current.current_fpe];
            (*fpe_functions[fpe->type]->client_died) ((void *) client, fpe);
        }
        err = Successful;
        goto bail;
    }
    client->pSwapReplyFunc = ReplySwapVector[X_ListFontsWithInfo];
    if (!c->current.patlen)
        goto finish;
    while (c->current.current_fpe < c->num_fpes) {
        fpe = c->fpe_list[c->current.current_fpe];
        err = Successful;
        if (!c->current.list_started) {
            err = (*fpe_functions[fpe->type]->start_list_fonts_with_info)
                (client, fpe, c->current.pattern, c->current.patlen,
                 c->current.max_names, &c->current.private);
            if (err == Suspended) {
                if (!ClientIsAsleep(client))
                    ClientSleep(client,
                                (ClientSleepProcPtr) doListFontsWithInfo, c);
                return TRUE;
            }
            if (err == Successful)
                c->current.list_started = TRUE;
        }
        if (err == Successful) {
            name = 0;
            pFontInfo = &fontInfo;
            err = (*fpe_functions[fpe->type]->list_next_font_with_info)
                (client, fpe, &name, &namelen, &pFontInfo,
                 &numFonts, c->current.private);
            if (err == Suspended) {
                if (!ClientIsAsleep(client))
                    ClientSleep(client,
                                (ClientSleepProcPtr) doListFontsWithInfo, c);
                return TRUE;
            }
        }
        /*
         * When we get an alias back, save our state and reset back to the
         * start of the FPE looking for the specified name.  As soon as a real
         * font is found for the alias, pop back to the old state
         */
        if (err == FontNameAlias) {
            /*
             * when an alias recurses, we need to give
             * the last FPE a chance to clean up; so we call
             * it again, and assume that the error returned
             * is BadFontName, indicating the alias resolution
             * is complete.
             */
            if (c->haveSaved) {
                char *tmpname;
                int tmpnamelen;
                FontInfoPtr tmpFontInfo;

                tmpname = 0;
                tmpFontInfo = &fontInfo;
                (void) (*fpe_functions[fpe->type]->list_next_font_with_info)
                    (client, fpe, &tmpname, &tmpnamelen, &tmpFontInfo,
                     &numFonts, c->current.private);
                if (--aliascount <= 0) {
                    err = BadFontName;
                    goto ContBadFontName;
                }
            }
            else {
                c->saved = c->current;
                c->haveSaved = TRUE;
                c->savedNumFonts = numFonts;
                free(c->savedName);
                c->savedName = malloc(namelen + 1);
                if (c->savedName)
                    memmove(c->savedName, name, namelen + 1);
                aliascount = 20;
            }
            memmove(c->current.pattern, name, namelen);
            c->current.patlen = namelen;
            c->current.max_names = 1;
            c->current.current_fpe = 0;
            c->current.private = 0;
            c->current.list_started = FALSE;
        }
        /*
         * At the end of this FPE, step to the next.  If we've finished
         * processing an alias, pop state back.  If we've sent enough font
         * names, quit.  Always wait for BadFontName to let the FPE
         * have a chance to clean up.
         */
        else if (err == BadFontName) {
 ContBadFontName:;
            c->current.list_started = FALSE;
            c->current.current_fpe++;
            err = Successful;
            if (c->haveSaved) {
                if (c->current.max_names == 0 ||
                    c->current.current_fpe == c->num_fpes) {
                    c->haveSaved = FALSE;
                    c->saved.max_names -= (1 - c->current.max_names);
                    c->current = c->saved;
                }
            }
            else if (c->current.max_names == 0)
                break;
        }
        else if (err == Successful) {
            length = sizeof(*reply) + pFontInfo->nprops * sizeof(xFontProp);
            reply = c->reply;
            if (c->length < length) {
                reply = (xListFontsWithInfoReply *) realloc(c->reply, length);
                if (!reply) {
                    err = AllocError;
                    break;
                }
                memset((char *) reply + c->length, 0, length - c->length);
                c->reply = reply;
                c->length = length;
            }
            if (c->haveSaved) {
                numFonts = c->savedNumFonts;
                name = c->savedName;
                namelen = strlen(name);
            }
            reply->type = X_Reply;
            reply->length =
                bytes_to_int32(sizeof *reply - sizeof(xGenericReply) +
                               pFontInfo->nprops * sizeof(xFontProp) + namelen);
            reply->sequenceNumber = client->sequence;
            reply->nameLength = namelen;
            reply->minBounds = pFontInfo->ink_minbounds;
            reply->maxBounds = pFontInfo->ink_maxbounds;
            reply->minCharOrByte2 = pFontInfo->firstCol;
            reply->maxCharOrByte2 = pFontInfo->lastCol;
            reply->defaultChar = pFontInfo->defaultCh;
            reply->nFontProps = pFontInfo->nprops;
            reply->drawDirection = pFontInfo->drawDirection;
            reply->minByte1 = pFontInfo->firstRow;
            reply->maxByte1 = pFontInfo->lastRow;
            reply->allCharsExist = pFontInfo->allExist;
            reply->fontAscent = pFontInfo->fontAscent;
            reply->fontDescent = pFontInfo->fontDescent;
            reply->nReplies = numFonts;
            pFP = (xFontProp *) (reply + 1);
            for (i = 0; i < pFontInfo->nprops; i++) {
                pFP->name = pFontInfo->props[i].name;
                pFP->value = pFontInfo->props[i].value;
                pFP++;
            }
            WriteSwappedDataToClient(client, length, reply);
            WriteToClient(client, namelen, name);
            if (pFontInfo == &fontInfo) {
                free(fontInfo.props);
                free(fontInfo.isStringProp);
            }
            --c->current.max_names;
        }
    }
 finish:
    length = sizeof(xListFontsWithInfoReply);
    finalReply = (xListFontsWithInfoReply) {
        .type = X_Reply,
        .sequenceNumber = client->sequence,
        .length = bytes_to_int32(sizeof(xListFontsWithInfoReply)
                                 - sizeof(xGenericReply))
    };
    WriteSwappedDataToClient(client, length, &finalReply);
 bail:
    ClientWakeup(client);
    for (i = 0; i < c->num_fpes; i++)
        FreeFPE(c->fpe_list[i]);
    free(c->reply);
    free(c->fpe_list);
    free(c->savedName);
    free(c);
    return TRUE;
}

int
StartListFontsWithInfo(ClientPtr client, int length, unsigned char *pattern,
                       int max_names)
{
    int i;
    LFWIclosurePtr c;

    /*
     * The right error to return here would be BadName, however the
     * specification does not allow for a Name error on this request.
     * Perhaps a better solution would be to return a nil list, i.e.
     * a list containing zero fontnames.
     */
    if (length > XLFDMAXFONTNAMELEN)
        return BadAlloc;

    i = XaceHook(XACE_SERVER_ACCESS, client, DixGetAttrAccess);
    if (i != Success)
        return i;

    if (!(c = malloc(sizeof *c)))
        goto badAlloc;
    c->fpe_list = xallocarray(num_fpes, sizeof(FontPathElementPtr));
    if (!c->fpe_list) {
        free(c);
        goto badAlloc;
    }
    memmove(c->current.pattern, pattern, length);
    for (i = 0; i < num_fpes; i++) {
        c->fpe_list[i] = font_path_elements[i];
        UseFPE(c->fpe_list[i]);
    }
    c->client = client;
    c->num_fpes = num_fpes;
    c->reply = 0;
    c->length = 0;
    c->current.patlen = length;
    c->current.current_fpe = 0;
    c->current.max_names = max_names;
    c->current.list_started = FALSE;
    c->current.private = 0;
    c->savedNumFonts = 0;
    c->haveSaved = FALSE;
    c->savedName = 0;
    doListFontsWithInfo(client, c);
    return Success;
 badAlloc:
    return BadAlloc;
}

#define TextEltHeader 2
#define FontShiftSize 5
static ChangeGCVal clearGC[] = { {.ptr = NullPixmap} };

#define clearGCmask (GCClipMask)

static int
doPolyText(ClientPtr client, PTclosurePtr c)
{
    FontPtr pFont = c->pGC->font, oldpFont;
    int err = Success, lgerr;   /* err is in X error, not font error, space */
    enum { NEVER_SLEPT, START_SLEEP, SLEEPING } client_state = NEVER_SLEPT;
    FontPathElementPtr fpe;
    GC *origGC = NULL;
    int itemSize = c->reqType == X_PolyText8 ? 1 : 2;

    if (client->clientGone) {
        fpe = c->pGC->font->fpe;
        (*fpe_functions[fpe->type]->client_died) ((void *) client, fpe);

        if (ClientIsAsleep(client)) {
            /* Client has died, but we cannot bail out right now.  We
               need to clean up after the work we did when going to
               sleep.  Setting the drawable pointer to 0 makes this
               happen without any attempts to render or perform other
               unnecessary activities.  */
            c->pDraw = (DrawablePtr) 0;
        }
        else {
            err = Success;
            goto bail;
        }
    }

    /* Make sure our drawable hasn't disappeared while we slept. */
    if (ClientIsAsleep(client) && c->pDraw) {
        DrawablePtr pDraw;

        dixLookupDrawable(&pDraw, c->did, client, 0, DixWriteAccess);
        if (c->pDraw != pDraw) {
            /* Our drawable has disappeared.  Treat like client died... ask
               the FPE code to clean up after client and avoid further
               rendering while we clean up after ourself.  */
            fpe = c->pGC->font->fpe;
            (*fpe_functions[fpe->type]->client_died) ((void *) client, fpe);
            c->pDraw = (DrawablePtr) 0;
        }
    }

    client_state = ClientIsAsleep(client) ? SLEEPING : NEVER_SLEPT;

    while (c->endReq - c->pElt > TextEltHeader) {
        if (*c->pElt == FontChange) {
            Font fid;

            if (c->endReq - c->pElt < FontShiftSize) {
                err = BadLength;
                goto bail;
            }

            oldpFont = pFont;

            fid = ((Font) *(c->pElt + 4))       /* big-endian */
                |((Font) *(c->pElt + 3)) << 8
                | ((Font) *(c->pElt + 2)) << 16 | ((Font) *(c->pElt + 1)) << 24;
            err = dixLookupResourceByType((void **) &pFont, fid, RT_FONT,
                                          client, DixUseAccess);
            if (err != Success) {
                /* restore pFont for step 4 (described below) */
                pFont = oldpFont;

                /* If we're in START_SLEEP mode, the following step
                   shortens the request...  in the unlikely event that
                   the fid somehow becomes valid before we come through
                   again to actually execute the polytext, which would
                   then mess up our refcounting scheme badly.  */
                c->err = err;
                c->endReq = c->pElt;

                goto bail;
            }

            /* Step 3 (described below) on our new font */
            if (client_state == START_SLEEP)
                pFont->refcnt++;
            else {
                if (pFont != c->pGC->font && c->pDraw) {
                    ChangeGCVal val;

                    val.ptr = pFont;
                    ChangeGC(NullClient, c->pGC, GCFont, &val);
                    ValidateGC(c->pDraw, c->pGC);
                }

                /* Undo the refcnt++ we performed when going to sleep */
                if (client_state == SLEEPING)
                    (void) CloseFont(c->pGC->font, (Font) 0);
            }
            c->pElt += FontShiftSize;
        }
        else {                  /* print a string */

            unsigned char *pNextElt;

            pNextElt = c->pElt + TextEltHeader + (*c->pElt) * itemSize;
            if (pNextElt > c->endReq) {
                err = BadLength;
                goto bail;
            }
            if (client_state == START_SLEEP) {
                c->pElt = pNextElt;
                continue;
            }
            if (c->pDraw) {
                lgerr = LoadGlyphs(client, c->pGC->font, *c->pElt, itemSize,
                                   c->pElt + TextEltHeader);
            }
            else
                lgerr = Successful;

            if (lgerr == Suspended) {
                if (!ClientIsAsleep(client)) {
                    int len;
                    GC *pGC;
                    PTclosurePtr new_closure;

                    /*  We're putting the client to sleep.  We need to do a few things
                       to ensure successful and atomic-appearing execution of the
                       remainder of the request.  First, copy the remainder of the
                       request into a safe malloc'd area.  Second, create a scratch GC
                       to use for the remainder of the request.  Third, mark all fonts
                       referenced in the remainder of the request to prevent their
                       deallocation.  Fourth, make the original GC look like the
                       request has completed...  set its font to the final font value
                       from this request.  These GC manipulations are for the unlikely
                       (but possible) event that some other client is using the GC.
                       Steps 3 and 4 are performed by running this procedure through
                       the remainder of the request in a special no-render mode
                       indicated by client_state = START_SLEEP.  */

                    /* Step 1 */
                    /* Allocate a malloc'd closure structure to replace
                       the local one we were passed */
                    new_closure = malloc(sizeof(PTclosureRec));
                    if (!new_closure) {
                        err = BadAlloc;
                        goto bail;
                    }
                    *new_closure = *c;

                    len = new_closure->endReq - new_closure->pElt;
                    new_closure->data = malloc(len);
                    if (!new_closure->data) {
                        free(new_closure);
                        err = BadAlloc;
                        goto bail;
                    }
                    memmove(new_closure->data, new_closure->pElt, len);
                    new_closure->pElt = new_closure->data;
                    new_closure->endReq = new_closure->pElt + len;

                    /* Step 2 */

                    pGC =
                        GetScratchGC(new_closure->pGC->depth,
                                     new_closure->pGC->pScreen);
                    if (!pGC) {
                        free(new_closure->data);
                        free(new_closure);
                        err = BadAlloc;
                        goto bail;
                    }
                    if ((err = CopyGC(new_closure->pGC, pGC, GCFunction |
                                      GCPlaneMask | GCForeground |
                                      GCBackground | GCFillStyle |
                                      GCTile | GCStipple |
                                      GCTileStipXOrigin |
                                      GCTileStipYOrigin | GCFont |
                                      GCSubwindowMode | GCClipXOrigin |
                                      GCClipYOrigin | GCClipMask)) != Success) {
                        FreeScratchGC(pGC);
                        free(new_closure->data);
                        free(new_closure);
                        err = BadAlloc;
                        goto bail;
                    }
                    c = new_closure;
                    origGC = c->pGC;
                    c->pGC = pGC;
                    ValidateGC(c->pDraw, c->pGC);

                    ClientSleep(client, (ClientSleepProcPtr) doPolyText, c);

                    /* Set up to perform steps 3 and 4 */
                    client_state = START_SLEEP;
                    continue;   /* on to steps 3 and 4 */
                }
                return TRUE;
            }
            else if (lgerr != Successful) {
                err = FontToXError(lgerr);
                goto bail;
            }
            if (c->pDraw) {
                c->xorg += *((INT8 *) (c->pElt + 1));   /* must be signed */
                if (c->reqType == X_PolyText8)
                    c->xorg =
                        (*c->pGC->ops->PolyText8) (c->pDraw, c->pGC, c->xorg,
                                                   c->yorg, *c->pElt,
                                                   (char *) (c->pElt +
                                                             TextEltHeader));
                else
                    c->xorg =
                        (*c->pGC->ops->PolyText16) (c->pDraw, c->pGC, c->xorg,
                                                    c->yorg, *c->pElt,
                                                    (unsigned short *) (c->
                                                                        pElt +
                                                                        TextEltHeader));
            }
            c->pElt = pNextElt;
        }
    }

 bail:

    if (client_state == START_SLEEP) {
        /* Step 4 */
        if (pFont != origGC->font) {
            ChangeGCVal val;

            val.ptr = pFont;
            ChangeGC(NullClient, origGC, GCFont, &val);
            ValidateGC(c->pDraw, origGC);
        }

        /* restore pElt pointer for execution of remainder of the request */
        c->pElt = c->data;
        return TRUE;
    }

    if (c->err != Success)
        err = c->err;
    if (err != Success && c->client != serverClient) {
#ifdef PANORAMIX
        if (noPanoramiXExtension || !c->pGC->pScreen->myNum)
#endif
            SendErrorToClient(c->client, c->reqType, 0, 0, err);
    }
    if (ClientIsAsleep(client)) {
        ClientWakeup(c->client);
        ChangeGC(NullClient, c->pGC, clearGCmask, clearGC);

        /* Unreference the font from the scratch GC */
        CloseFont(c->pGC->font, (Font) 0);
        c->pGC->font = NullFont;

        FreeScratchGC(c->pGC);
        free(c->data);
        free(c);
    }
    return TRUE;
}

int
PolyText(ClientPtr client, DrawablePtr pDraw, GC * pGC, unsigned char *pElt,
         unsigned char *endReq, int xorg, int yorg, int reqType, XID did)
{
    PTclosureRec local_closure;

    local_closure.pElt = pElt;
    local_closure.endReq = endReq;
    local_closure.client = client;
    local_closure.pDraw = pDraw;
    local_closure.xorg = xorg;
    local_closure.yorg = yorg;
    local_closure.reqType = reqType;
    local_closure.pGC = pGC;
    local_closure.did = did;
    local_closure.err = Success;

    (void) doPolyText(client, &local_closure);
    return Success;
}

#undef TextEltHeader
#undef FontShiftSize

static int
doImageText(ClientPtr client, ITclosurePtr c)
{
    int err = Success, lgerr;   /* err is in X error, not font error, space */
    FontPathElementPtr fpe;
    int itemSize = c->reqType == X_ImageText8 ? 1 : 2;

    if (client->clientGone) {
        fpe = c->pGC->font->fpe;
        (*fpe_functions[fpe->type]->client_died) ((void *) client, fpe);
        err = Success;
        goto bail;
    }

    /* Make sure our drawable hasn't disappeared while we slept. */
    if (ClientIsAsleep(client) && c->pDraw) {
        DrawablePtr pDraw;

        dixLookupDrawable(&pDraw, c->did, client, 0, DixWriteAccess);
        if (c->pDraw != pDraw) {
            /* Our drawable has disappeared.  Treat like client died... ask
               the FPE code to clean up after client. */
            fpe = c->pGC->font->fpe;
            (*fpe_functions[fpe->type]->client_died) ((void *) client, fpe);
            err = Success;
            goto bail;
        }
    }

    lgerr = LoadGlyphs(client, c->pGC->font, c->nChars, itemSize, c->data);
    if (lgerr == Suspended) {
        if (!ClientIsAsleep(client)) {
            GC *pGC;
            unsigned char *data;
            ITclosurePtr new_closure;
            ITclosurePtr old_closure;

            /* We're putting the client to sleep.  We need to
               save some state.  Similar problem to that handled
               in doPolyText, but much simpler because the
               request structure is much simpler. */

            new_closure = malloc(sizeof(ITclosureRec));
            if (!new_closure) {
                err = BadAlloc;
                goto bail;
            }
            old_closure = c;
            *new_closure = *c;
            c = new_closure;

            data = xallocarray(c->nChars, itemSize);
            if (!data) {
                free(c);
                c = old_closure;
                err = BadAlloc;
                goto bail;
            }
            memmove(data, c->data, c->nChars * itemSize);
            c->data = data;

            pGC = GetScratchGC(c->pGC->depth, c->pGC->pScreen);
            if (!pGC) {
                free(c->data);
                free(c);
                c = old_closure;
                err = BadAlloc;
                goto bail;
            }
            if ((err = CopyGC(c->pGC, pGC, GCFunction | GCPlaneMask |
                              GCForeground | GCBackground | GCFillStyle |
                              GCTile | GCStipple | GCTileStipXOrigin |
                              GCTileStipYOrigin | GCFont |
                              GCSubwindowMode | GCClipXOrigin |
                              GCClipYOrigin | GCClipMask)) != Success) {
                FreeScratchGC(pGC);
                free(c->data);
                free(c);
                c = old_closure;
                err = BadAlloc;
                goto bail;
            }
            c->pGC = pGC;
            ValidateGC(c->pDraw, c->pGC);

            ClientSleep(client, (ClientSleepProcPtr) doImageText, c);
        }
        return TRUE;
    }
    else if (lgerr != Successful) {
        err = FontToXError(lgerr);
        goto bail;
    }
    if (c->pDraw) {
        if (c->reqType == X_ImageText8)
            (*c->pGC->ops->ImageText8) (c->pDraw, c->pGC, c->xorg, c->yorg,
                                        c->nChars, (char *) c->data);
        else
            (*c->pGC->ops->ImageText16) (c->pDraw, c->pGC, c->xorg, c->yorg,
                                         c->nChars, (unsigned short *) c->data);
    }

 bail:

    if (err != Success && c->client != serverClient) {
        SendErrorToClient(c->client, c->reqType, 0, 0, err);
    }
    if (ClientIsAsleep(client)) {
        ClientWakeup(c->client);
        ChangeGC(NullClient, c->pGC, clearGCmask, clearGC);

        /* Unreference the font from the scratch GC */
        CloseFont(c->pGC->font, (Font) 0);
        c->pGC->font = NullFont;

        FreeScratchGC(c->pGC);
        free(c->data);
        free(c);
    }
    return TRUE;
}

int
ImageText(ClientPtr client, DrawablePtr pDraw, GC * pGC, int nChars,
          unsigned char *data, int xorg, int yorg, int reqType, XID did)
{
    ITclosureRec local_closure;

    local_closure.client = client;
    local_closure.pDraw = pDraw;
    local_closure.pGC = pGC;
    local_closure.nChars = nChars;
    local_closure.data = data;
    local_closure.xorg = xorg;
    local_closure.yorg = yorg;
    local_closure.reqType = reqType;
    local_closure.did = did;

    (void) doImageText(client, &local_closure);
    return Success;
}

/* does the necessary magic to figure out the fpe type */
static int
DetermineFPEType(const char *pathname)
{
    int i;

    for (i = 0; i < num_fpe_types; i++) {
        if ((*fpe_functions[i]->name_check) (pathname))
            return i;
    }
    return -1;
}

static void
FreeFontPath(FontPathElementPtr * list, int n, Bool force)
{
    int i;

    for (i = 0; i < n; i++) {
        if (force) {
            /* Sanity check that all refcounts will be 0 by the time
               we get to the end of the list. */
            int found = 1;      /* the first reference is us */
            int j;

            for (j = i + 1; j < n; j++) {
                if (list[j] == list[i])
                    found++;
            }
            if (list[i]->refcount != found) {
                list[i]->refcount = found;      /* ensure it will get freed */
            }
        }
        FreeFPE(list[i]);
    }
    free(list);
}

static FontPathElementPtr
find_existing_fpe(FontPathElementPtr * list, int num, unsigned char *name,
                  int len)
{
    FontPathElementPtr fpe;
    int i;

    for (i = 0; i < num; i++) {
        fpe = list[i];
        if (fpe->name_length == len && memcmp(name, fpe->name, len) == 0)
            return fpe;
    }
    return (FontPathElementPtr) 0;
}

static int
SetFontPathElements(int npaths, unsigned char *paths, int *bad, Bool persist)
{
    int i, err = 0;
    int valid_paths = 0;
    unsigned int len;
    unsigned char *cp = paths;
    FontPathElementPtr fpe = NULL, *fplist;

    fplist = xallocarray(npaths, sizeof(FontPathElementPtr));
    if (!fplist) {
        *bad = 0;
        return BadAlloc;
    }
    for (i = 0; i < num_fpe_types; i++) {
        if (fpe_functions[i]->set_path_hook)
            (*fpe_functions[i]->set_path_hook) ();
    }
    for (i = 0; i < npaths; i++) {
        len = (unsigned int) (*cp++);

        if (len == 0) {
            if (persist)
                ErrorF
                    ("[dix] Removing empty element from the valid list of fontpaths\n");
            err = BadValue;
        }
        else {
            /* if it's already in our active list, just reset it */
            /*
             * note that this can miss FPE's in limbo -- may be worth catching
             * them, though it'd muck up refcounting
             */
            fpe = find_existing_fpe(font_path_elements, num_fpes, cp, len);
            if (fpe) {
                err = (*fpe_functions[fpe->type]->reset_fpe) (fpe);
                if (err == Successful) {
                    UseFPE(fpe);        /* since it'll be decref'd later when freed
                                         * from the old list */
                }
                else
                    fpe = 0;
            }
            /* if error or can't do it, act like it's a new one */
            if (!fpe) {
                char *name;
                fpe = malloc(sizeof(FontPathElementRec));
                if (!fpe) {
                    err = BadAlloc;
                    goto bail;
                }
                name = malloc(len + 1);
                if (!name) {
                    free(fpe);
                    err = BadAlloc;
                    goto bail;
                }
                fpe->refcount = 1;

                strncpy(name, (char *) cp, (int) len);
                name[len] = '\0';
                fpe->name = name;
                fpe->name_length = len;
                fpe->type = DetermineFPEType(fpe->name);
                if (fpe->type == -1)
                    err = BadValue;
                else
                    err = (*fpe_functions[fpe->type]->init_fpe) (fpe);
                if (err != Successful) {
                    if (persist) {
                        DebugF
                            ("[dix] Could not init font path element %s, removing from list!\n",
                             fpe->name);
                    }
                    free((void *) fpe->name);
                    free(fpe);
                }
            }
        }
        if (err != Successful) {
            if (!persist)
                goto bail;
        }
        else {
            fplist[valid_paths++] = fpe;
        }
        cp += len;
    }

    FreeFontPath(font_path_elements, num_fpes, FALSE);
    font_path_elements = fplist;
    if (patternCache)
        xfont2_empty_font_pattern_cache(patternCache);
    num_fpes = valid_paths;

    return Success;
 bail:
    *bad = i;
    while (--valid_paths >= 0)
        FreeFPE(fplist[valid_paths]);
    free(fplist);
    return FontToXError(err);
}

int
SetFontPath(ClientPtr client, int npaths, unsigned char *paths)
{
    int err = XaceHook(XACE_SERVER_ACCESS, client, DixManageAccess);

    if (err != Success)
        return err;

    if (npaths == 0) {
        if (SetDefaultFontPath(defaultFontPath) != Success)
            return BadValue;
    }
    else {
        int bad;

        err = SetFontPathElements(npaths, paths, &bad, FALSE);
        client->errorValue = bad;
    }
    return err;
}

int
SetDefaultFontPath(const char *path)
{
    const char *start, *end;
    char *temp_path;
    unsigned char *cp, *pp, *nump, *newpath;
    int num = 1, len, err, size = 0, bad;

    /* ensure temp_path contains "built-ins" */
    start = path;
    while (1) {
        start = strstr(start, "built-ins");
        if (start == NULL)
            break;
        end = start + strlen("built-ins");
        if ((start == path || start[-1] == ',') && (!*end || *end == ','))
            break;
        start = end;
    }
    if (!start) {
        if (asprintf(&temp_path, "%s%sbuilt-ins", path, *path ? "," : "")
            == -1)
            temp_path = NULL;
    }
    else {
        temp_path = strdup(path);
    }
    if (!temp_path)
        return BadAlloc;

    /* get enough for string, plus values -- use up commas */
    len = strlen(temp_path) + 1;
    nump = cp = newpath = malloc(len);
    if (!newpath) {
        free(temp_path);
        return BadAlloc;
    }
    pp = (unsigned char *) temp_path;
    cp++;
    while (*pp) {
        if (*pp == ',') {
            *nump = (unsigned char) size;
            nump = cp++;
            pp++;
            num++;
            size = 0;
        }
        else {
            *cp++ = *pp++;
            size++;
        }
    }
    *nump = (unsigned char) size;

    err = SetFontPathElements(num, newpath, &bad, TRUE);

    free(newpath);
    free(temp_path);

    return err;
}

int
GetFontPath(ClientPtr client, int *count, int *length, unsigned char **result)
{
    int i;
    unsigned char *c;
    int len;
    FontPathElementPtr fpe;

    i = XaceHook(XACE_SERVER_ACCESS, client, DixGetAttrAccess);
    if (i != Success)
        return i;

    len = 0;
    for (i = 0; i < num_fpes; i++) {
        fpe = font_path_elements[i];
        len += fpe->name_length + 1;
    }
    c = realloc(font_path_string, len);
    if (c == NULL) {
        free(font_path_string);
        font_path_string = NULL;
        return BadAlloc;
    }

    font_path_string = c;
    *length = 0;
    for (i = 0; i < num_fpes; i++) {
        fpe = font_path_elements[i];
        *c = fpe->name_length;
        *length += *c++;
        memmove(c, fpe->name, fpe->name_length);
        c += fpe->name_length;
    }
    *count = num_fpes;
    *result = font_path_string;
    return Success;
}

void
DeleteClientFontStuff(ClientPtr client)
{
    int i;
    FontPathElementPtr fpe;

    for (i = 0; i < num_fpes; i++) {
        fpe = font_path_elements[i];
        if (fpe_functions[fpe->type]->client_died)
            (*fpe_functions[fpe->type]->client_died) ((void *) client, fpe);
    }
}

static int
register_fpe_funcs(const xfont2_fpe_funcs_rec *funcs)
{
    xfont2_fpe_funcs_rec const **new;

    /* grow the list */
    new = reallocarray(fpe_functions, num_fpe_types + 1, sizeof(xfont2_fpe_funcs_ptr));
    if (!new)
        return -1;
    fpe_functions = new;

    fpe_functions[num_fpe_types] = funcs;

    return num_fpe_types++;
}

static unsigned long
get_server_generation(void)
{
    return serverGeneration;
}

static void *
get_server_client(void)
{
    return serverClient;
}

static int
get_default_point_size(void)
{
    return 120;
}

static FontResolutionPtr
get_client_resolutions(int *num)
{
    static struct _FontResolution res;
    ScreenPtr pScreen;

    pScreen = screenInfo.screens[0];
    res.x_resolution = (pScreen->width * 25.4) / pScreen->mmWidth;
    /*
     * XXX - we'll want this as long as bitmap instances are prevalent
     so that we can match them from scalable fonts
     */
    if (res.x_resolution < 88)
        res.x_resolution = 75;
    else
        res.x_resolution = 100;
    res.y_resolution = (pScreen->height * 25.4) / pScreen->mmHeight;
    if (res.y_resolution < 88)
        res.y_resolution = 75;
    else
        res.y_resolution = 100;
    res.point_size = 120;
    *num = 1;
    return &res;
}

void
FreeFonts(void)
{
    if (patternCache) {
        xfont2_free_font_pattern_cache(patternCache);
        patternCache = 0;
    }
    FreeFontPath(font_path_elements, num_fpes, TRUE);
    font_path_elements = 0;
    num_fpes = 0;
    free(fpe_functions);
    num_fpe_types = 0;
    fpe_functions = NULL;
}

/* convenience functions for FS interface */

static FontPtr
find_old_font(XID id)
{
    void *pFont;

    dixLookupResourceByType(&pFont, id, RT_NONE, serverClient, DixReadAccess);
    return (FontPtr) pFont;
}

static Font
get_new_font_client_id(void)
{
    return FakeClientID(0);
}

static int
store_font_Client_font(FontPtr pfont, Font id)
{
    return AddResource(id, RT_NONE, (void *) pfont);
}

static void
delete_font_client_id(Font id)
{
    FreeResource(id, RT_NONE);
}

static int
_client_auth_generation(ClientPtr client)
{
    return 0;
}

static int fs_handlers_installed = 0;
static unsigned int last_server_gen;

static void fs_block_handler(void *blockData, void *timeout)
{
    FontBlockHandlerProcPtr block_handler = blockData;

    (*block_handler)(timeout);
}

struct fs_fd_entry {
    struct xorg_list            entry;
    int                         fd;
    void                        *data;
    FontFdHandlerProcPtr        handler;
};

static void
fs_fd_handler(int fd, int ready, void *data)
{
    struct fs_fd_entry    *entry = data;

    entry->handler(fd, entry->data);
}

static struct xorg_list fs_fd_list;

static int
add_fs_fd(int fd, FontFdHandlerProcPtr handler, void *data)
{
    struct fs_fd_entry  *entry = calloc(1, sizeof (struct fs_fd_entry));

    if (!entry)
        return FALSE;

    entry->fd = fd;
    entry->data = data;
    entry->handler = handler;
    if (!SetNotifyFd(fd, fs_fd_handler, X_NOTIFY_READ, entry)) {
        free(entry);
        return FALSE;
    }
    xorg_list_add(&entry->entry, &fs_fd_list);
    return TRUE;
}

static void
remove_fs_fd(int fd)
{
    struct fs_fd_entry  *entry, *temp;

    xorg_list_for_each_entry_safe(entry, temp, &fs_fd_list, entry) {
        if (entry->fd == fd) {
            xorg_list_del(&entry->entry);
            free(entry);
            break;
        }
    }
    RemoveNotifyFd(fd);
}

static void
adjust_fs_wait_for_delay(void *wt, unsigned long newdelay)
{
    AdjustWaitForDelay(wt, newdelay);
}

static int
_init_fs_handlers(FontPathElementPtr fpe, FontBlockHandlerProcPtr block_handler)
{
    /* if server has reset, make sure the b&w handlers are reinstalled */
    if (last_server_gen < serverGeneration) {
        last_server_gen = serverGeneration;
        fs_handlers_installed = 0;
    }
    if (fs_handlers_installed == 0) {
        if (!RegisterBlockAndWakeupHandlers(fs_block_handler,
                                            FontWakeup, (void *) block_handler))
            return AllocError;
        xorg_list_init(&fs_fd_list);
        fs_handlers_installed++;
    }
    QueueFontWakeup(fpe);
    return Successful;
}

static void
_remove_fs_handlers(FontPathElementPtr fpe, FontBlockHandlerProcPtr block_handler,
                    Bool all)
{
    if (all) {
        /* remove the handlers if no one else is using them */
        if (--fs_handlers_installed == 0) {
            RemoveBlockAndWakeupHandlers(fs_block_handler, FontWakeup,
                                         (void *) block_handler);
        }
    }
    RemoveFontWakeup(fpe);
}

static uint32_t wrap_time_in_millis(void)
{
    return GetTimeInMillis();
}

static const xfont2_client_funcs_rec xfont2_client_funcs = {
    .version = XFONT2_CLIENT_FUNCS_VERSION,
    .client_auth_generation = _client_auth_generation,
    .client_signal = ClientSignal,
    .delete_font_client_id = delete_font_client_id,
    .verrorf = VErrorF,
    .find_old_font = find_old_font,
    .get_client_resolutions = get_client_resolutions,
    .get_default_point_size = get_default_point_size,
    .get_new_font_client_id = get_new_font_client_id,
    .get_time_in_millis = wrap_time_in_millis,
    .init_fs_handlers = _init_fs_handlers,
    .register_fpe_funcs = register_fpe_funcs,
    .remove_fs_handlers = _remove_fs_handlers,
    .get_server_client = get_server_client,
    .set_font_authorizations = set_font_authorizations,
    .store_font_client_font = store_font_Client_font,
    .make_atom = MakeAtom,
    .valid_atom = ValidAtom,
    .name_for_atom = NameForAtom,
    .get_server_generation = get_server_generation,
    .add_fs_fd = add_fs_fd,
    .remove_fs_fd = remove_fs_fd,
    .adjust_fs_wait_for_delay = adjust_fs_wait_for_delay,
};

xfont2_pattern_cache_ptr fontPatternCache;

void
InitFonts(void)
{
    if (fontPatternCache)
	xfont2_free_font_pattern_cache(fontPatternCache);
    fontPatternCache = xfont2_make_font_pattern_cache();
    xfont2_init(&xfont2_client_funcs);
}
