/* DRI.c -- DRI Section in XF86Config file
 * Created: Fri Mar 19 08:40:22 1999 by faith@precisioninsight.com
 * Revised: Thu Jun 17 16:08:05 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 *
 */

#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#include "os.h"
#include "xf86Parser.h"
#include "xf86tokens.h"
#include "Configint.h"


static const xf86ConfigSymTabRec DRITab[] = {
    {ENDSECTION, "endsection"},
    {GROUP, "group"},
    {MODE, "mode"},
    {-1, ""},
};

#define CLEANUP xf86freeDRI

XF86ConfDRIPtr
xf86parseDRISection(void)
{
    int token;

    parsePrologue(XF86ConfDRIPtr, XF86ConfDRIRec);

    /* Zero is a valid value for this. */
    ptr->dri_group = -1;
    while ((token = xf86getToken(DRITab)) != ENDSECTION) {
        switch (token) {
        case GROUP:
            if ((token = xf86getSubToken(&(ptr->dri_comment))) == STRING)
                ptr->dri_group_name = xf86_lex_val.str;
            else if (token == NUMBER)
                ptr->dri_group = xf86_lex_val.num;
            else
                Error(GROUP_MSG);
            break;
        case MODE:
            if (xf86getSubToken(&(ptr->dri_comment)) != NUMBER)
                Error(NUMBER_MSG, "Mode");
            if (xf86_lex_val.numType != PARSE_OCTAL)
                Error(MUST_BE_OCTAL_MSG, xf86_lex_val.num);
            ptr->dri_mode = xf86_lex_val.num;
            break;
        case EOF_TOKEN:
            Error(UNEXPECTED_EOF_MSG);
            break;
        case COMMENT:
            ptr->dri_comment = xf86addComment(ptr->dri_comment, xf86_lex_val.str);
            break;
        default:
            Error(INVALID_KEYWORD_MSG, xf86tokenString());
            break;
        }
    }

#ifdef DEBUG
    ErrorF("DRI section parsed\n");
#endif

    return ptr;
}

#undef CLEANUP

void
xf86printDRISection(FILE * cf, XF86ConfDRIPtr ptr)
{
    if (ptr == NULL)
        return;

    fprintf(cf, "Section \"DRI\"\n");
    if (ptr->dri_comment)
        fprintf(cf, "%s", ptr->dri_comment);
    if (ptr->dri_group_name)
        fprintf(cf, "\tGroup        \"%s\"\n", ptr->dri_group_name);
    else if (ptr->dri_group >= 0)
        fprintf(cf, "\tGroup        %d\n", ptr->dri_group);
    if (ptr->dri_mode)
        fprintf(cf, "\tMode         0%o\n", ptr->dri_mode);
    fprintf(cf, "EndSection\n\n");
}

void
xf86freeDRI(XF86ConfDRIPtr ptr)
{
    if (ptr == NULL)
        return;

    TestFree(ptr->dri_comment);
    free(ptr);
}
