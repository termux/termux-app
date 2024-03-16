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
#include "Xprintf.h"


static const xf86ConfigSymTabRec PointerTab[] = {
    {PROTOCOL, "protocol"},
    {EMULATE3, "emulate3buttons"},
    {EM3TIMEOUT, "emulate3timeout"},
    {ENDSUBSECTION, "endsubsection"},
    {ENDSECTION, "endsection"},
    {PDEVICE, "device"},
    {PDEVICE, "port"},
    {BAUDRATE, "baudrate"},
    {SAMPLERATE, "samplerate"},
    {CLEARDTR, "cleardtr"},
    {CLEARRTS, "clearrts"},
    {CHORDMIDDLE, "chordmiddle"},
    {PRESOLUTION, "resolution"},
    {DEVICE_NAME, "devicename"},
    {ALWAYSCORE, "alwayscore"},
    {PBUTTONS, "buttons"},
    {ZAXISMAPPING, "zaxismapping"},
    {-1, ""},
};

static const xf86ConfigSymTabRec ZMapTab[] = {
    {XAXIS, "x"},
    {YAXIS, "y"},
    {-1, ""},
};

#define CLEANUP xf86freeInputList

XF86ConfInputPtr
xf86parsePointerSection(void)
{
    char *s;
    unsigned long val1;
    int token;

    parsePrologue(XF86ConfInputPtr, XF86ConfInputRec)

        while ((token = xf86getToken(PointerTab)) != ENDSECTION) {
        switch (token) {
        case COMMENT:
            ptr->inp_comment = xf86addComment(ptr->inp_comment, xf86_lex_val.str);
            break;
        case PROTOCOL:
            if (xf86getSubToken(&(ptr->inp_comment)) != STRING)
                Error(QUOTE_MSG, "Protocol");
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("Protocol"), xf86_lex_val.str);
            break;
        case PDEVICE:
            if (xf86getSubToken(&(ptr->inp_comment)) != STRING)
                Error(QUOTE_MSG, "Device");
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("Device"), xf86_lex_val.str);
            break;
        case EMULATE3:
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("Emulate3Buttons"),
                                                   NULL);
            break;
        case EM3TIMEOUT:
            if (xf86getSubToken(&(ptr->inp_comment)) != NUMBER || xf86_lex_val.num < 0)
                Error(POSITIVE_INT_MSG, "Emulate3Timeout");
            s = xf86uLongToString(xf86_lex_val.num);
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("Emulate3Timeout"),
                                                   s);
            break;
        case CHORDMIDDLE:
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("ChordMiddle"), NULL);
            break;
        case PBUTTONS:
            if (xf86getSubToken(&(ptr->inp_comment)) != NUMBER || xf86_lex_val.num < 0)
                Error(POSITIVE_INT_MSG, "Buttons");
            s = xf86uLongToString(xf86_lex_val.num);
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("Buttons"), s);
            break;
        case BAUDRATE:
            if (xf86getSubToken(&(ptr->inp_comment)) != NUMBER || xf86_lex_val.num < 0)
                Error(POSITIVE_INT_MSG, "BaudRate");
            s = xf86uLongToString(xf86_lex_val.num);
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("BaudRate"), s);
            break;
        case SAMPLERATE:
            if (xf86getSubToken(&(ptr->inp_comment)) != NUMBER || xf86_lex_val.num < 0)
                Error(POSITIVE_INT_MSG, "SampleRate");
            s = xf86uLongToString(xf86_lex_val.num);
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("SampleRate"), s);
            break;
        case PRESOLUTION:
            if (xf86getSubToken(&(ptr->inp_comment)) != NUMBER || xf86_lex_val.num < 0)
                Error(POSITIVE_INT_MSG, "Resolution");
            s = xf86uLongToString(xf86_lex_val.num);
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("Resolution"), s);
            break;
        case CLEARDTR:
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("ClearDTR"), NULL);
            break;
        case CLEARRTS:
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("ClearRTS"), NULL);
            break;
        case ZAXISMAPPING:
            switch (xf86getToken(ZMapTab)) {
            case NUMBER:
                if (xf86_lex_val.num < 0)
                    Error(ZAXISMAPPING_MSG);
                val1 = xf86_lex_val.num;
                if (xf86getSubToken(&(ptr->inp_comment)) != NUMBER ||
                    xf86_lex_val.num < 0) {
                    Error(ZAXISMAPPING_MSG);
                }
                if (asprintf(&s, "%lu %u", val1, xf86_lex_val.num) == -1)
                    s = NULL;
                break;
            case XAXIS:
                s = strdup("x");
                break;
            case YAXIS:
                s = strdup("y");
                break;
            default:
                Error(ZAXISMAPPING_MSG);
                break;
            }
            ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                                   strdup("ZAxisMapping"), s);
            break;
        case ALWAYSCORE:
            break;
        case EOF_TOKEN:
            Error(UNEXPECTED_EOF_MSG);
            break;
        default:
            Error(INVALID_KEYWORD_MSG, xf86tokenString());
            break;
        }
    }

    ptr->inp_identifier = strdup(CONF_IMPLICIT_POINTER);
    ptr->inp_driver = strdup("mouse");
    ptr->inp_option_lst = xf86addNewOption(ptr->inp_option_lst,
                                           strdup("CorePointer"), NULL);

#ifdef DEBUG
    printf("Pointer section parsed\n");
#endif

    return ptr;
}

#undef CLEANUP
