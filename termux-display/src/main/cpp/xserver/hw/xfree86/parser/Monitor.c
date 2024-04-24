/*
 *
 * Copyright (c) 1997  Metro Link Incorporated
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
 * THE X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the Metro Link shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Metro Link.
 *
 */
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

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"


static const xf86ConfigSymTabRec MonitorTab[] = {
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {VENDOR, "vendorname"},
    {MODEL, "modelname"},
    {USEMODES, "usemodes"},
    {MODELINE, "modeline"},
    {DISPLAYSIZE, "displaysize"},
    {HORIZSYNC, "horizsync"},
    {VERTREFRESH, "vertrefresh"},
    {MODE, "mode"},
    {GAMMA, "gamma"},
    {OPTION, "option"},
    {-1, ""},
};

static const xf86ConfigSymTabRec ModesTab[] = {
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {MODELINE, "modeline"},
    {MODE, "mode"},
    {-1, ""},
};

static const xf86ConfigSymTabRec TimingTab[] = {
    {TT_INTERLACE, "interlace"},
    {TT_PHSYNC, "+hsync"},
    {TT_NHSYNC, "-hsync"},
    {TT_PVSYNC, "+vsync"},
    {TT_NVSYNC, "-vsync"},
    {TT_CSYNC, "composite"},
    {TT_PCSYNC, "+csync"},
    {TT_NCSYNC, "-csync"},
    {TT_DBLSCAN, "doublescan"},
    {TT_HSKEW, "hskew"},
    {TT_BCAST, "bcast"},
    {TT_VSCAN, "vscan"},
    {-1, ""},
};

static const xf86ConfigSymTabRec ModeTab[] = {
    {DOTCLOCK, "dotclock"},
    {HTIMINGS, "htimings"},
    {VTIMINGS, "vtimings"},
    {FLAGS, "flags"},
    {HSKEW, "hskew"},
    {BCAST, "bcast"},
    {VSCAN, "vscan"},
    {ENDMODE, "endmode"},
    {-1, ""},
};

#define CLEANUP xf86freeModeLineList

static void
xf86freeModeLineList(XF86ConfModeLinePtr ptr)
{
    XF86ConfModeLinePtr prev;

    while (ptr) {
        TestFree(ptr->ml_identifier);
        TestFree(ptr->ml_comment);
        prev = ptr;
        ptr = ptr->list.next;
        free(prev);
    }
}

static XF86ConfModeLinePtr
xf86parseModeLine(void)
{
    int token;

    parsePrologue(XF86ConfModeLinePtr, XF86ConfModeLineRec)

        /* Identifier */
        if (xf86getSubToken(&(ptr->ml_comment)) != STRING)
        Error("ModeLine identifier expected");
    ptr->ml_identifier = xf86_lex_val.str;

    /* DotClock */
    if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
        Error("ModeLine dotclock expected");
    ptr->ml_clock = (int) (xf86_lex_val.realnum * 1000.0 + 0.5);

    /* HDisplay */
    if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
        Error("ModeLine Hdisplay expected");
    ptr->ml_hdisplay = xf86_lex_val.num;

    /* HSyncStart */
    if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
        Error("ModeLine HSyncStart expected");
    ptr->ml_hsyncstart = xf86_lex_val.num;

    /* HSyncEnd */
    if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
        Error("ModeLine HSyncEnd expected");
    ptr->ml_hsyncend = xf86_lex_val.num;

    /* HTotal */
    if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
        Error("ModeLine HTotal expected");
    ptr->ml_htotal = xf86_lex_val.num;

    /* VDisplay */
    if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
        Error("ModeLine Vdisplay expected");
    ptr->ml_vdisplay = xf86_lex_val.num;

    /* VSyncStart */
    if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
        Error("ModeLine VSyncStart expected");
    ptr->ml_vsyncstart = xf86_lex_val.num;

    /* VSyncEnd */
    if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
        Error("ModeLine VSyncEnd expected");
    ptr->ml_vsyncend = xf86_lex_val.num;

    /* VTotal */
    if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
        Error("ModeLine VTotal expected");
    ptr->ml_vtotal = xf86_lex_val.num;

    token = xf86getSubTokenWithTab(&(ptr->ml_comment), TimingTab);
    while ((token == TT_INTERLACE) || (token == TT_PHSYNC) ||
           (token == TT_NHSYNC) || (token == TT_PVSYNC) ||
           (token == TT_NVSYNC) || (token == TT_CSYNC) ||
           (token == TT_PCSYNC) || (token == TT_NCSYNC) ||
           (token == TT_DBLSCAN) || (token == TT_HSKEW) ||
           (token == TT_VSCAN) || (token == TT_BCAST)) {
        switch (token) {

        case TT_INTERLACE:
            ptr->ml_flags |= XF86CONF_INTERLACE;
            break;
        case TT_PHSYNC:
            ptr->ml_flags |= XF86CONF_PHSYNC;
            break;
        case TT_NHSYNC:
            ptr->ml_flags |= XF86CONF_NHSYNC;
            break;
        case TT_PVSYNC:
            ptr->ml_flags |= XF86CONF_PVSYNC;
            break;
        case TT_NVSYNC:
            ptr->ml_flags |= XF86CONF_NVSYNC;
            break;
        case TT_CSYNC:
            ptr->ml_flags |= XF86CONF_CSYNC;
            break;
        case TT_PCSYNC:
            ptr->ml_flags |= XF86CONF_PCSYNC;
            break;
        case TT_NCSYNC:
            ptr->ml_flags |= XF86CONF_NCSYNC;
            break;
        case TT_DBLSCAN:
            ptr->ml_flags |= XF86CONF_DBLSCAN;
            break;
        case TT_HSKEW:
            if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
                Error(NUMBER_MSG, "Hskew");
            ptr->ml_hskew = xf86_lex_val.num;
            ptr->ml_flags |= XF86CONF_HSKEW;
            break;
        case TT_BCAST:
            ptr->ml_flags |= XF86CONF_BCAST;
            break;
        case TT_VSCAN:
            if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
                Error(NUMBER_MSG, "Vscan");
            ptr->ml_vscan = xf86_lex_val.num;
            ptr->ml_flags |= XF86CONF_VSCAN;
            break;
        case EOF_TOKEN:
            Error(UNEXPECTED_EOF_MSG);
            break;
        default:
            Error(INVALID_KEYWORD_MSG, xf86tokenString());
            break;
        }
        token = xf86getSubTokenWithTab(&(ptr->ml_comment), TimingTab);
    }
    xf86unGetToken(token);

#ifdef DEBUG
    printf("ModeLine parsed\n");
#endif
    return ptr;
}

static XF86ConfModeLinePtr
xf86parseVerboseMode(void)
{
    int token, token2;
    int had_dotclock = 0, had_htimings = 0, had_vtimings = 0;

    parsePrologue(XF86ConfModeLinePtr, XF86ConfModeLineRec)

        if (xf86getSubToken(&(ptr->ml_comment)) != STRING)
        Error("Mode name expected");
    ptr->ml_identifier = xf86_lex_val.str;
    while ((token = xf86getToken(ModeTab)) != ENDMODE) {
        switch (token) {
        case COMMENT:
            ptr->ml_comment = xf86addComment(ptr->ml_comment, xf86_lex_val.str);
            break;
        case DOTCLOCK:
            if ((token = xf86getSubToken(&(ptr->ml_comment))) != NUMBER)
                Error(NUMBER_MSG, "DotClock");
            ptr->ml_clock = (int) (xf86_lex_val.realnum * 1000.0 + 0.5);
            had_dotclock = 1;
            break;
        case HTIMINGS:
            if (xf86getSubToken(&(ptr->ml_comment)) == NUMBER)
                ptr->ml_hdisplay = xf86_lex_val.num;
            else
                Error("Horizontal display expected");

            if (xf86getSubToken(&(ptr->ml_comment)) == NUMBER)
                ptr->ml_hsyncstart = xf86_lex_val.num;
            else
                Error("Horizontal sync start expected");

            if (xf86getSubToken(&(ptr->ml_comment)) == NUMBER)
                ptr->ml_hsyncend = xf86_lex_val.num;
            else
                Error("Horizontal sync end expected");

            if (xf86getSubToken(&(ptr->ml_comment)) == NUMBER)
                ptr->ml_htotal = xf86_lex_val.num;
            else
                Error("Horizontal total expected");
            had_htimings = 1;
            break;
        case VTIMINGS:
            if (xf86getSubToken(&(ptr->ml_comment)) == NUMBER)
                ptr->ml_vdisplay = xf86_lex_val.num;
            else
                Error("Vertical display expected");

            if (xf86getSubToken(&(ptr->ml_comment)) == NUMBER)
                ptr->ml_vsyncstart = xf86_lex_val.num;
            else
                Error("Vertical sync start expected");

            if (xf86getSubToken(&(ptr->ml_comment)) == NUMBER)
                ptr->ml_vsyncend = xf86_lex_val.num;
            else
                Error("Vertical sync end expected");

            if (xf86getSubToken(&(ptr->ml_comment)) == NUMBER)
                ptr->ml_vtotal = xf86_lex_val.num;
            else
                Error("Vertical total expected");
            had_vtimings = 1;
            break;
        case FLAGS:
            token = xf86getSubToken(&(ptr->ml_comment));
            if (token != STRING)
                Error(QUOTE_MSG, "Flags");
            while (token == STRING) {
                token2 = xf86getStringToken(TimingTab);
                switch (token2) {
                case TT_INTERLACE:
                    ptr->ml_flags |= XF86CONF_INTERLACE;
                    break;
                case TT_PHSYNC:
                    ptr->ml_flags |= XF86CONF_PHSYNC;
                    break;
                case TT_NHSYNC:
                    ptr->ml_flags |= XF86CONF_NHSYNC;
                    break;
                case TT_PVSYNC:
                    ptr->ml_flags |= XF86CONF_PVSYNC;
                    break;
                case TT_NVSYNC:
                    ptr->ml_flags |= XF86CONF_NVSYNC;
                    break;
                case TT_CSYNC:
                    ptr->ml_flags |= XF86CONF_CSYNC;
                    break;
                case TT_PCSYNC:
                    ptr->ml_flags |= XF86CONF_PCSYNC;
                    break;
                case TT_NCSYNC:
                    ptr->ml_flags |= XF86CONF_NCSYNC;
                    break;
                case TT_DBLSCAN:
                    ptr->ml_flags |= XF86CONF_DBLSCAN;
                    break;
                case EOF_TOKEN:
                    Error(UNEXPECTED_EOF_MSG);
                    break;
                default:
                    Error("Unknown flag string");
                    break;
                }
                token = xf86getSubToken(&(ptr->ml_comment));
            }
            xf86unGetToken(token);
            break;
        case HSKEW:
            if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
                Error("Horizontal skew expected");
            ptr->ml_flags |= XF86CONF_HSKEW;
            ptr->ml_hskew = xf86_lex_val.num;
            break;
        case VSCAN:
            if (xf86getSubToken(&(ptr->ml_comment)) != NUMBER)
                Error("Vertical scan count expected");
            ptr->ml_flags |= XF86CONF_VSCAN;
            ptr->ml_vscan = xf86_lex_val.num;
            break;
        case EOF_TOKEN:
            Error(UNEXPECTED_EOF_MSG);
            break;
        default:
            Error("Unexpected token in verbose \"Mode\" entry\n");
        }
    }
    if (!had_dotclock)
        Error("the dotclock is missing");
    if (!had_htimings)
        Error("the horizontal timings are missing");
    if (!had_vtimings)
        Error("the vertical timings are missing");

#ifdef DEBUG
    printf("Verbose Mode parsed\n");
#endif
    return ptr;
}

#undef CLEANUP

#define CLEANUP xf86freeMonitorList

XF86ConfMonitorPtr
xf86parseMonitorSection(void)
{
    int has_ident = FALSE;
    int token;

    parsePrologue(XF86ConfMonitorPtr, XF86ConfMonitorRec)

        while ((token = xf86getToken(MonitorTab)) != ENDSECTION) {
        switch (token) {
        case COMMENT:
            ptr->mon_comment = xf86addComment(ptr->mon_comment, xf86_lex_val.str);
            break;
        case IDENTIFIER:
            if (xf86getSubToken(&(ptr->mon_comment)) != STRING)
                Error(QUOTE_MSG, "Identifier");
            if (has_ident == TRUE)
                Error(MULTIPLE_MSG, "Identifier");
            ptr->mon_identifier = xf86_lex_val.str;
            has_ident = TRUE;
            break;
        case VENDOR:
            if (xf86getSubToken(&(ptr->mon_comment)) != STRING)
                Error(QUOTE_MSG, "Vendor");
            ptr->mon_vendor = xf86_lex_val.str;
            break;
        case MODEL:
            if (xf86getSubToken(&(ptr->mon_comment)) != STRING)
                Error(QUOTE_MSG, "ModelName");
            ptr->mon_modelname = xf86_lex_val.str;
            break;
        case MODE:
            HANDLE_LIST(mon_modeline_lst, xf86parseVerboseMode,
                        XF86ConfModeLinePtr);
            break;
        case MODELINE:
            HANDLE_LIST(mon_modeline_lst, xf86parseModeLine,
                        XF86ConfModeLinePtr);
            break;
        case DISPLAYSIZE:
            if (xf86getSubToken(&(ptr->mon_comment)) != NUMBER)
                Error(DISPLAYSIZE_MSG);
            ptr->mon_width = xf86_lex_val.realnum;
            if (xf86getSubToken(&(ptr->mon_comment)) != NUMBER)
                Error(DISPLAYSIZE_MSG);
            ptr->mon_height = xf86_lex_val.realnum;
            break;

        case HORIZSYNC:
            if (xf86getSubToken(&(ptr->mon_comment)) != NUMBER)
                Error(HORIZSYNC_MSG);
            do {
                if (ptr->mon_n_hsync >= CONF_MAX_HSYNC)
                    Error("Sorry. Too many horizontal sync intervals.");
                ptr->mon_hsync[ptr->mon_n_hsync].lo = xf86_lex_val.realnum;
                switch (token = xf86getSubToken(&(ptr->mon_comment))) {
                case COMMA:
                    ptr->mon_hsync[ptr->mon_n_hsync].hi =
                        ptr->mon_hsync[ptr->mon_n_hsync].lo;
                    break;
                case DASH:
                    if (xf86getSubToken(&(ptr->mon_comment)) != NUMBER ||
                        (float) xf86_lex_val.realnum <
                        ptr->mon_hsync[ptr->mon_n_hsync].lo)
                        Error(HORIZSYNC_MSG);
                    ptr->mon_hsync[ptr->mon_n_hsync].hi = xf86_lex_val.realnum;
                    if ((token = xf86getSubToken(&(ptr->mon_comment))) == COMMA)
                        break;
                    ptr->mon_n_hsync++;
                    goto HorizDone;
                default:
                    /* We cannot currently know if a '\n' was found,
                     * or this is a real error
                     */
                    ptr->mon_hsync[ptr->mon_n_hsync].hi =
                        ptr->mon_hsync[ptr->mon_n_hsync].lo;
                    ptr->mon_n_hsync++;
                    goto HorizDone;
                }
                ptr->mon_n_hsync++;
            } while ((token = xf86getSubToken(&(ptr->mon_comment))) == NUMBER);
 HorizDone:
            xf86unGetToken(token);
            break;

        case VERTREFRESH:
            if (xf86getSubToken(&(ptr->mon_comment)) != NUMBER)
                Error(VERTREFRESH_MSG);
            do {
                ptr->mon_vrefresh[ptr->mon_n_vrefresh].lo = xf86_lex_val.realnum;
                switch (token = xf86getSubToken(&(ptr->mon_comment))) {
                case COMMA:
                    ptr->mon_vrefresh[ptr->mon_n_vrefresh].hi =
                        ptr->mon_vrefresh[ptr->mon_n_vrefresh].lo;
                    break;
                case DASH:
                    if (xf86getSubToken(&(ptr->mon_comment)) != NUMBER ||
                        (float) xf86_lex_val.realnum <
                        ptr->mon_vrefresh[ptr->mon_n_vrefresh].lo)
                        Error(VERTREFRESH_MSG);
                    ptr->mon_vrefresh[ptr->mon_n_vrefresh].hi = xf86_lex_val.realnum;
                    if ((token = xf86getSubToken(&(ptr->mon_comment))) == COMMA)
                        break;
                    ptr->mon_n_vrefresh++;
                    goto VertDone;
                default:
                    /* We cannot currently know if a '\n' was found,
                     * or this is a real error
                     */
                    ptr->mon_vrefresh[ptr->mon_n_vrefresh].hi =
                        ptr->mon_vrefresh[ptr->mon_n_vrefresh].lo;
                    ptr->mon_n_vrefresh++;
                    goto VertDone;
                }
                if (ptr->mon_n_vrefresh >= CONF_MAX_VREFRESH)
                    Error("Sorry. Too many vertical refresh intervals.");
                ptr->mon_n_vrefresh++;
            } while ((token = xf86getSubToken(&(ptr->mon_comment))) == NUMBER);
 VertDone:
            xf86unGetToken(token);
            break;

        case GAMMA:
            if (xf86getSubToken(&(ptr->mon_comment)) != NUMBER) {
                Error(INVALID_GAMMA_MSG);
            }
            else {
                ptr->mon_gamma_red = ptr->mon_gamma_green =
                    ptr->mon_gamma_blue = xf86_lex_val.realnum;
                if (xf86getSubToken(&(ptr->mon_comment)) == NUMBER) {
                    ptr->mon_gamma_green = xf86_lex_val.realnum;
                    if (xf86getSubToken(&(ptr->mon_comment)) == NUMBER) {
                        ptr->mon_gamma_blue = xf86_lex_val.realnum;
                    }
                    else {
                        Error(INVALID_GAMMA_MSG);
                    }
                }
                else
                    xf86unGetToken(token);
            }
            break;
        case OPTION:
            ptr->mon_option_lst = xf86parseOption(ptr->mon_option_lst);
            break;
        case USEMODES:
        {
            XF86ConfModesLinkPtr mptr;

            if ((token = xf86getSubToken(&(ptr->mon_comment))) != STRING)
                Error(QUOTE_MSG, "UseModes");

            /* add to the end of the list of modes sections
               referenced here */
            mptr = calloc(1, sizeof(XF86ConfModesLinkRec));
            mptr->list.next = NULL;
            mptr->ml_modes_str = xf86_lex_val.str;
            mptr->ml_modes = NULL;
            ptr->mon_modes_sect_lst = (XF86ConfModesLinkPtr)
                xf86addListItem((GenericListPtr) ptr->mon_modes_sect_lst,
                                (GenericListPtr) mptr);
        }
            break;
        case EOF_TOKEN:
            Error(UNEXPECTED_EOF_MSG);
            break;
        default:
            xf86parseError(INVALID_KEYWORD_MSG, xf86tokenString());
            CLEANUP(ptr);
            return NULL;
            break;
        }
    }

    if (!has_ident)
        Error(NO_IDENT_MSG);

#ifdef DEBUG
    printf("Monitor section parsed\n");
#endif
    return ptr;
}

#undef CLEANUP
#define CLEANUP xf86freeModesList

XF86ConfModesPtr
xf86parseModesSection(void)
{
    int has_ident = FALSE;
    int token;

    parsePrologue(XF86ConfModesPtr, XF86ConfModesRec)

        while ((token = xf86getToken(ModesTab)) != ENDSECTION) {
        switch (token) {
        case COMMENT:
            ptr->modes_comment = xf86addComment(ptr->modes_comment, xf86_lex_val.str);
            break;
        case IDENTIFIER:
            if (xf86getSubToken(&(ptr->modes_comment)) != STRING)
                Error(QUOTE_MSG, "Identifier");
            if (has_ident == TRUE)
                Error(MULTIPLE_MSG, "Identifier");
            ptr->modes_identifier = xf86_lex_val.str;
            has_ident = TRUE;
            break;
        case MODE:
            HANDLE_LIST(mon_modeline_lst, xf86parseVerboseMode,
                        XF86ConfModeLinePtr);
            break;
        case MODELINE:
            HANDLE_LIST(mon_modeline_lst, xf86parseModeLine,
                        XF86ConfModeLinePtr);
            break;
        default:
            xf86parseError(INVALID_KEYWORD_MSG, xf86tokenString());
            CLEANUP(ptr);
            return NULL;
            break;
        }
    }

    if (!has_ident)
        Error(NO_IDENT_MSG);

#ifdef DEBUG
    printf("Modes section parsed\n");
#endif
    return ptr;
}

#undef CLEANUP

void
xf86printMonitorSection(FILE * cf, XF86ConfMonitorPtr ptr)
{
    int i;
    XF86ConfModeLinePtr mlptr;
    XF86ConfModesLinkPtr mptr;

    while (ptr) {
        mptr = ptr->mon_modes_sect_lst;
        fprintf(cf, "Section \"Monitor\"\n");
        if (ptr->mon_comment)
            fprintf(cf, "%s", ptr->mon_comment);
        if (ptr->mon_identifier)
            fprintf(cf, "\tIdentifier   \"%s\"\n", ptr->mon_identifier);
        if (ptr->mon_vendor)
            fprintf(cf, "\tVendorName   \"%s\"\n", ptr->mon_vendor);
        if (ptr->mon_modelname)
            fprintf(cf, "\tModelName    \"%s\"\n", ptr->mon_modelname);
        while (mptr) {
            fprintf(cf, "\tUseModes     \"%s\"\n", mptr->ml_modes_str);
            mptr = mptr->list.next;
        }
        if (ptr->mon_width)
            fprintf(cf, "\tDisplaySize  %d\t%d\n",
                    ptr->mon_width, ptr->mon_height);
        for (i = 0; i < ptr->mon_n_hsync; i++) {
            fprintf(cf, "\tHorizSync    %2.1f - %2.1f\n",
                    ptr->mon_hsync[i].lo, ptr->mon_hsync[i].hi);
        }
        for (i = 0; i < ptr->mon_n_vrefresh; i++) {
            fprintf(cf, "\tVertRefresh  %2.1f - %2.1f\n",
                    ptr->mon_vrefresh[i].lo, ptr->mon_vrefresh[i].hi);
        }
        if (ptr->mon_gamma_red) {
            if (ptr->mon_gamma_red == ptr->mon_gamma_green
                && ptr->mon_gamma_red == ptr->mon_gamma_blue) {
                fprintf(cf, "\tGamma        %.4g\n", ptr->mon_gamma_red);
            }
            else {
                fprintf(cf, "\tGamma        %.4g %.4g %.4g\n",
                        ptr->mon_gamma_red,
                        ptr->mon_gamma_green, ptr->mon_gamma_blue);
            }
        }
        for (mlptr = ptr->mon_modeline_lst; mlptr; mlptr = mlptr->list.next) {
            fprintf(cf, "\tModeLine     \"%s\" %2.1f ",
                    mlptr->ml_identifier, mlptr->ml_clock / 1000.0);
            fprintf(cf, "%d %d %d %d %d %d %d %d",
                    mlptr->ml_hdisplay, mlptr->ml_hsyncstart,
                    mlptr->ml_hsyncend, mlptr->ml_htotal,
                    mlptr->ml_vdisplay, mlptr->ml_vsyncstart,
                    mlptr->ml_vsyncend, mlptr->ml_vtotal);
            if (mlptr->ml_flags & XF86CONF_PHSYNC)
                fprintf(cf, " +hsync");
            if (mlptr->ml_flags & XF86CONF_NHSYNC)
                fprintf(cf, " -hsync");
            if (mlptr->ml_flags & XF86CONF_PVSYNC)
                fprintf(cf, " +vsync");
            if (mlptr->ml_flags & XF86CONF_NVSYNC)
                fprintf(cf, " -vsync");
            if (mlptr->ml_flags & XF86CONF_INTERLACE)
                fprintf(cf, " interlace");
            if (mlptr->ml_flags & XF86CONF_CSYNC)
                fprintf(cf, " composite");
            if (mlptr->ml_flags & XF86CONF_PCSYNC)
                fprintf(cf, " +csync");
            if (mlptr->ml_flags & XF86CONF_NCSYNC)
                fprintf(cf, " -csync");
            if (mlptr->ml_flags & XF86CONF_DBLSCAN)
                fprintf(cf, " doublescan");
            if (mlptr->ml_flags & XF86CONF_HSKEW)
                fprintf(cf, " hskew %d", mlptr->ml_hskew);
            if (mlptr->ml_flags & XF86CONF_BCAST)
                fprintf(cf, " bcast");
            fprintf(cf, "\n");
        }
        xf86printOptionList(cf, ptr->mon_option_lst, 1);
        fprintf(cf, "EndSection\n\n");
        ptr = ptr->list.next;
    }
}

void
xf86printModesSection(FILE * cf, XF86ConfModesPtr ptr)
{
    XF86ConfModeLinePtr mlptr;

    while (ptr) {
        fprintf(cf, "Section \"Modes\"\n");
        if (ptr->modes_comment)
            fprintf(cf, "%s", ptr->modes_comment);
        if (ptr->modes_identifier)
            fprintf(cf, "\tIdentifier     \"%s\"\n", ptr->modes_identifier);
        for (mlptr = ptr->mon_modeline_lst; mlptr; mlptr = mlptr->list.next) {
            fprintf(cf, "\tModeLine     \"%s\" %2.1f ",
                    mlptr->ml_identifier, mlptr->ml_clock / 1000.0);
            fprintf(cf, "%d %d %d %d %d %d %d %d",
                    mlptr->ml_hdisplay, mlptr->ml_hsyncstart,
                    mlptr->ml_hsyncend, mlptr->ml_htotal,
                    mlptr->ml_vdisplay, mlptr->ml_vsyncstart,
                    mlptr->ml_vsyncend, mlptr->ml_vtotal);
            if (mlptr->ml_flags & XF86CONF_PHSYNC)
                fprintf(cf, " +hsync");
            if (mlptr->ml_flags & XF86CONF_NHSYNC)
                fprintf(cf, " -hsync");
            if (mlptr->ml_flags & XF86CONF_PVSYNC)
                fprintf(cf, " +vsync");
            if (mlptr->ml_flags & XF86CONF_NVSYNC)
                fprintf(cf, " -vsync");
            if (mlptr->ml_flags & XF86CONF_INTERLACE)
                fprintf(cf, " interlace");
            if (mlptr->ml_flags & XF86CONF_CSYNC)
                fprintf(cf, " composite");
            if (mlptr->ml_flags & XF86CONF_PCSYNC)
                fprintf(cf, " +csync");
            if (mlptr->ml_flags & XF86CONF_NCSYNC)
                fprintf(cf, " -csync");
            if (mlptr->ml_flags & XF86CONF_DBLSCAN)
                fprintf(cf, " doublescan");
            if (mlptr->ml_flags & XF86CONF_HSKEW)
                fprintf(cf, " hskew %d", mlptr->ml_hskew);
            if (mlptr->ml_flags & XF86CONF_VSCAN)
                fprintf(cf, " vscan %d", mlptr->ml_vscan);
            if (mlptr->ml_flags & XF86CONF_BCAST)
                fprintf(cf, " bcast");
            if (mlptr->ml_comment)
                fprintf(cf, "%s", mlptr->ml_comment);
            else
                fprintf(cf, "\n");
        }
        fprintf(cf, "EndSection\n\n");
        ptr = ptr->list.next;
    }
}

void
xf86freeMonitorList(XF86ConfMonitorPtr ptr)
{
    XF86ConfMonitorPtr prev;

    while (ptr) {
        TestFree(ptr->mon_identifier);
        TestFree(ptr->mon_vendor);
        TestFree(ptr->mon_modelname);
        TestFree(ptr->mon_comment);
        xf86optionListFree(ptr->mon_option_lst);
        xf86freeModeLineList(ptr->mon_modeline_lst);
        prev = ptr;
        ptr = ptr->list.next;
        free(prev);
    }
}

void
xf86freeModesList(XF86ConfModesPtr ptr)
{
    XF86ConfModesPtr prev;

    while (ptr) {
        TestFree(ptr->modes_identifier);
        TestFree(ptr->modes_comment);
        xf86freeModeLineList(ptr->mon_modeline_lst);
        prev = ptr;
        ptr = ptr->list.next;
        free(prev);
    }
}

XF86ConfMonitorPtr
xf86findMonitor(const char *ident, XF86ConfMonitorPtr p)
{
    while (p) {
        if (xf86nameCompare(ident, p->mon_identifier) == 0)
            return p;

        p = p->list.next;
    }
    return NULL;
}

XF86ConfModesPtr
xf86findModes(const char *ident, XF86ConfModesPtr p)
{
    while (p) {
        if (xf86nameCompare(ident, p->modes_identifier) == 0)
            return p;

        p = p->list.next;
    }
    return NULL;
}

XF86ConfModeLinePtr
xf86findModeLine(const char *ident, XF86ConfModeLinePtr p)
{
    while (p) {
        if (xf86nameCompare(ident, p->ml_identifier) == 0)
            return p;

        p = p->list.next;
    }
    return NULL;
}

int
xf86validateMonitor(XF86ConfigPtr p, XF86ConfScreenPtr screen)
{
    XF86ConfMonitorPtr monitor = screen->scrn_monitor;
    XF86ConfModesLinkPtr modeslnk = monitor->mon_modes_sect_lst;
    XF86ConfModesPtr modes;

    while (modeslnk) {
        modes = xf86findModes(modeslnk->ml_modes_str, p->conf_modes_lst);
        if (!modes) {
            xf86validationError(UNDEFINED_MODES_MSG,
                                modeslnk->ml_modes_str,
                                screen->scrn_identifier);
            return FALSE;
        }
        modeslnk->ml_modes = modes;
        modeslnk = modeslnk->list.next;
    }
    return TRUE;
}
