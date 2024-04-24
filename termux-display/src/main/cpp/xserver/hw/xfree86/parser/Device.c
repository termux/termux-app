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


static const xf86ConfigSymTabRec DeviceTab[] = {
    {ENDSECTION, "endsection"},
    {IDENTIFIER, "identifier"},
    {VENDOR, "vendorname"},
    {BOARD, "boardname"},
    {CHIPSET, "chipset"},
    {RAMDAC, "ramdac"},
    {DACSPEED, "dacspeed"},
    {CLOCKS, "clocks"},
    {MATCHSEAT, "matchseat"},
    {OPTION, "option"},
    {VIDEORAM, "videoram"},
    {BIOSBASE, "biosbase"},
    {MEMBASE, "membase"},
    {IOBASE, "iobase"},
    {CLOCKCHIP, "clockchip"},
    {CHIPID, "chipid"},
    {CHIPREV, "chiprev"},
    {CARD, "card"},
    {DRIVER, "driver"},
    {BUSID, "busid"},
    {IRQ, "irq"},
    {SCREEN, "screen"},
    {-1, ""},
};

#define CLEANUP xf86freeDeviceList

XF86ConfDevicePtr
xf86parseDeviceSection(void)
{
    int i;
    int has_ident = FALSE;
    int token;

    parsePrologue(XF86ConfDevicePtr, XF86ConfDeviceRec)

        /* Zero is a valid value for these */
        ptr->dev_chipid = -1;
    ptr->dev_chiprev = -1;
    ptr->dev_irq = -1;
    while ((token = xf86getToken(DeviceTab)) != ENDSECTION) {
        switch (token) {
        case COMMENT:
            ptr->dev_comment = xf86addComment(ptr->dev_comment, xf86_lex_val.str);
            break;
        case IDENTIFIER:
            if (xf86getSubToken(&(ptr->dev_comment)) != STRING)
                Error(QUOTE_MSG, "Identifier");
            if (has_ident == TRUE)
                Error(MULTIPLE_MSG, "Identifier");
            ptr->dev_identifier = xf86_lex_val.str;
            has_ident = TRUE;
            break;
        case VENDOR:
            if (xf86getSubToken(&(ptr->dev_comment)) != STRING)
                Error(QUOTE_MSG, "Vendor");
            ptr->dev_vendor = xf86_lex_val.str;
            break;
        case BOARD:
            if (xf86getSubToken(&(ptr->dev_comment)) != STRING)
                Error(QUOTE_MSG, "Board");
            ptr->dev_board = xf86_lex_val.str;
            break;
        case CHIPSET:
            if (xf86getSubToken(&(ptr->dev_comment)) != STRING)
                Error(QUOTE_MSG, "Chipset");
            ptr->dev_chipset = xf86_lex_val.str;
            break;
        case CARD:
            if (xf86getSubToken(&(ptr->dev_comment)) != STRING)
                Error(QUOTE_MSG, "Card");
            ptr->dev_card = xf86_lex_val.str;
            break;
        case DRIVER:
            if (xf86getSubToken(&(ptr->dev_comment)) != STRING)
                Error(QUOTE_MSG, "Driver");
            ptr->dev_driver = xf86_lex_val.str;
            break;
        case RAMDAC:
            if (xf86getSubToken(&(ptr->dev_comment)) != STRING)
                Error(QUOTE_MSG, "Ramdac");
            ptr->dev_ramdac = xf86_lex_val.str;
            break;
        case DACSPEED:
            for (i = 0; i < CONF_MAXDACSPEEDS; i++)
                ptr->dev_dacSpeeds[i] = 0;
            if (xf86getSubToken(&(ptr->dev_comment)) != NUMBER) {
                Error(DACSPEED_MSG, CONF_MAXDACSPEEDS);
            }
            else {
                ptr->dev_dacSpeeds[0] = (int) (xf86_lex_val.realnum * 1000.0 + 0.5);
                for (i = 1; i < CONF_MAXDACSPEEDS; i++) {
                    if (xf86getSubToken(&(ptr->dev_comment)) == NUMBER)
                        ptr->dev_dacSpeeds[i] = (int)
                            (xf86_lex_val.realnum * 1000.0 + 0.5);
                    else {
                        xf86unGetToken(token);
                        break;
                    }
                }
            }
            break;
        case VIDEORAM:
            if (xf86getSubToken(&(ptr->dev_comment)) != NUMBER)
                Error(NUMBER_MSG, "VideoRam");
            ptr->dev_videoram = xf86_lex_val.num;
            break;
        case BIOSBASE:
            if (xf86getSubToken(&(ptr->dev_comment)) != NUMBER)
                Error(NUMBER_MSG, "BIOSBase");
            /* ignored */
            break;
        case MEMBASE:
            if (xf86getSubToken(&(ptr->dev_comment)) != NUMBER)
                Error(NUMBER_MSG, "MemBase");
            ptr->dev_mem_base = xf86_lex_val.num;
            break;
        case IOBASE:
            if (xf86getSubToken(&(ptr->dev_comment)) != NUMBER)
                Error(NUMBER_MSG, "IOBase");
            ptr->dev_io_base = xf86_lex_val.num;
            break;
        case CLOCKCHIP:
            if (xf86getSubToken(&(ptr->dev_comment)) != STRING)
                Error(QUOTE_MSG, "ClockChip");
            ptr->dev_clockchip = xf86_lex_val.str;
            break;
        case CHIPID:
            if (xf86getSubToken(&(ptr->dev_comment)) != NUMBER)
                Error(NUMBER_MSG, "ChipID");
            ptr->dev_chipid = xf86_lex_val.num;
            break;
        case CHIPREV:
            if (xf86getSubToken(&(ptr->dev_comment)) != NUMBER)
                Error(NUMBER_MSG, "ChipRev");
            ptr->dev_chiprev = xf86_lex_val.num;
            break;

        case CLOCKS:
            token = xf86getSubToken(&(ptr->dev_comment));
            for (i = ptr->dev_clocks;
                 token == NUMBER && i < CONF_MAXCLOCKS; i++) {
                ptr->dev_clock[i] = (int) (xf86_lex_val.realnum * 1000.0 + 0.5);
                token = xf86getSubToken(&(ptr->dev_comment));
            }
            ptr->dev_clocks = i;
            xf86unGetToken(token);
            break;
        case MATCHSEAT:
            if (xf86getSubToken(&(ptr->dev_comment)) != STRING)
                Error(QUOTE_MSG, "MatchSeat");
            ptr->match_seat = xf86_lex_val.str;
            break;
        case OPTION:
            ptr->dev_option_lst = xf86parseOption(ptr->dev_option_lst);
            break;
        case BUSID:
            if (xf86getSubToken(&(ptr->dev_comment)) != STRING)
                Error(QUOTE_MSG, "BusID");
            ptr->dev_busid = xf86_lex_val.str;
            break;
        case IRQ:
            if (xf86getSubToken(&(ptr->dev_comment)) != NUMBER)
                Error(QUOTE_MSG, "IRQ");
            ptr->dev_irq = xf86_lex_val.num;
            break;
        case SCREEN:
            if (xf86getSubToken(&(ptr->dev_comment)) != NUMBER)
                Error(NUMBER_MSG, "Screen");
            ptr->dev_screen = xf86_lex_val.num;
            break;
        case EOF_TOKEN:
            Error(UNEXPECTED_EOF_MSG);
            break;
        default:
            Error(INVALID_KEYWORD_MSG, xf86tokenString());
            break;
        }
    }

    if (!has_ident)
        Error(NO_IDENT_MSG);

#ifdef DEBUG
    printf("Device section parsed\n");
#endif

    return ptr;
}

#undef CLEANUP

void
xf86printDeviceSection(FILE * cf, XF86ConfDevicePtr ptr)
{
    int i;

    while (ptr) {
        fprintf(cf, "Section \"Device\"\n");
        if (ptr->dev_comment)
            fprintf(cf, "%s", ptr->dev_comment);
        if (ptr->dev_identifier)
            fprintf(cf, "\tIdentifier  \"%s\"\n", ptr->dev_identifier);
        if (ptr->dev_driver)
            fprintf(cf, "\tDriver      \"%s\"\n", ptr->dev_driver);
        if (ptr->dev_vendor)
            fprintf(cf, "\tVendorName  \"%s\"\n", ptr->dev_vendor);
        if (ptr->dev_board)
            fprintf(cf, "\tBoardName   \"%s\"\n", ptr->dev_board);
        if (ptr->dev_chipset)
            fprintf(cf, "\tChipSet     \"%s\"\n", ptr->dev_chipset);
        if (ptr->dev_card)
            fprintf(cf, "\tCard        \"%s\"\n", ptr->dev_card);
        if (ptr->dev_ramdac)
            fprintf(cf, "\tRamDac      \"%s\"\n", ptr->dev_ramdac);
        if (ptr->dev_dacSpeeds[0] > 0) {
            fprintf(cf, "\tDacSpeed    ");
            for (i = 0; i < CONF_MAXDACSPEEDS && ptr->dev_dacSpeeds[i] > 0; i++)
                fprintf(cf, "%g ", (double) (ptr->dev_dacSpeeds[i]) / 1000.0);
            fprintf(cf, "\n");
        }
        if (ptr->dev_videoram)
            fprintf(cf, "\tVideoRam    %d\n", ptr->dev_videoram);
        if (ptr->dev_mem_base)
            fprintf(cf, "\tMemBase     0x%lx\n", ptr->dev_mem_base);
        if (ptr->dev_io_base)
            fprintf(cf, "\tIOBase      0x%lx\n", ptr->dev_io_base);
        if (ptr->dev_clockchip)
            fprintf(cf, "\tClockChip   \"%s\"\n", ptr->dev_clockchip);
        if (ptr->dev_chipid != -1)
            fprintf(cf, "\tChipId      0x%x\n", ptr->dev_chipid);
        if (ptr->dev_chiprev != -1)
            fprintf(cf, "\tChipRev     0x%x\n", ptr->dev_chiprev);

        xf86printOptionList(cf, ptr->dev_option_lst, 1);
        if (ptr->dev_clocks > 0) {
            fprintf(cf, "\tClocks      ");
            for (i = 0; i < ptr->dev_clocks; i++)
                fprintf(cf, "%.1f ", (double) ptr->dev_clock[i] / 1000.0);
            fprintf(cf, "\n");
        }
        if (ptr->dev_busid)
            fprintf(cf, "\tBusID       \"%s\"\n", ptr->dev_busid);
        if (ptr->dev_screen > 0)
            fprintf(cf, "\tScreen      %d\n", ptr->dev_screen);
        if (ptr->dev_irq >= 0)
            fprintf(cf, "\tIRQ         %d\n", ptr->dev_irq);
        fprintf(cf, "EndSection\n\n");
        ptr = ptr->list.next;
    }
}

void
xf86freeDeviceList(XF86ConfDevicePtr ptr)
{
    XF86ConfDevicePtr prev;

    while (ptr) {
        TestFree(ptr->dev_identifier);
        TestFree(ptr->dev_vendor);
        TestFree(ptr->dev_board);
        TestFree(ptr->dev_chipset);
        TestFree(ptr->dev_card);
        TestFree(ptr->dev_driver);
        TestFree(ptr->dev_ramdac);
        TestFree(ptr->dev_clockchip);
        TestFree(ptr->dev_comment);
        xf86optionListFree(ptr->dev_option_lst);

        prev = ptr;
        ptr = ptr->list.next;
        free(prev);
    }
}

XF86ConfDevicePtr
xf86findDevice(const char *ident, XF86ConfDevicePtr p)
{
    while (p) {
        if (xf86nameCompare(ident, p->dev_identifier) == 0)
            return p;

        p = p->list.next;
    }
    return NULL;
}
