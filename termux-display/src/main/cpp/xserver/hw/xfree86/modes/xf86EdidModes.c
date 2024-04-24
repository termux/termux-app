/*
 * Copyright 2006 Luc Verhaegen.
 * Copyright 2008 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * @file This file covers code to convert a xf86MonPtr containing EDID-probed
 * information into a list of modes, including applying monitor-specific
 * quirks to fix broken EDID data.
 */
#ifdef HAVE_XORG_CONFIG_H
#include <xorg-config.h>
#endif

#define _PARSE_EDID_
#include "xf86.h"
#include "xf86DDC.h"
#include <X11/Xatom.h>
#include "property.h"
#include "propertyst.h"
#include "xf86Crtc.h"
#include <string.h>
#include <math.h>

static void
handle_detailed_rblank(struct detailed_monitor_section *det_mon, void *data)
{
    if (det_mon->type == DS_RANGES)
        if (det_mon->section.ranges.supported_blanking & CVT_REDUCED)
            *(Bool *) data = TRUE;
}

static Bool
xf86MonitorSupportsReducedBlanking(xf86MonPtr DDC)
{
    /* EDID 1.4 explicitly defines RB support */
    if (DDC->ver.revision >= 4) {
        Bool ret = FALSE;

        xf86ForEachDetailedBlock(DDC, handle_detailed_rblank, &ret);
        return ret;
    }

    /* For anything older, assume digital means RB support. Boo. */
    if (DDC->features.input_type)
        return TRUE;

    return FALSE;
}

static Bool
quirk_prefer_large_60(int scrnIndex, xf86MonPtr DDC)
{
    /* Belinea 10 15 55 */
    if (memcmp(DDC->vendor.name, "MAX", 4) == 0 &&
        ((DDC->vendor.prod_id == 1516) || (DDC->vendor.prod_id == 0x77e)))
        return TRUE;

    /* Acer AL1706 */
    if (memcmp(DDC->vendor.name, "ACR", 4) == 0 && DDC->vendor.prod_id == 44358)
        return TRUE;

    /* Bug #10814: Samsung SyncMaster 225BW */
    if (memcmp(DDC->vendor.name, "SAM", 4) == 0 && DDC->vendor.prod_id == 596)
        return TRUE;

    /* Bug #10545: Samsung SyncMaster 226BW */
    if (memcmp(DDC->vendor.name, "SAM", 4) == 0 && DDC->vendor.prod_id == 638)
        return TRUE;

    /* Acer F51 */
    if (memcmp(DDC->vendor.name, "API", 4) == 0 &&
        DDC->vendor.prod_id == 0x7602)
        return TRUE;

    return FALSE;
}

static Bool
quirk_prefer_large_75(int scrnIndex, xf86MonPtr DDC)
{
    /* Bug #11603: Funai Electronics PM36B */
    if (memcmp(DDC->vendor.name, "FCM", 4) == 0 && DDC->vendor.prod_id == 13600)
        return TRUE;

    return FALSE;
}

static Bool
quirk_detailed_h_in_cm(int scrnIndex, xf86MonPtr DDC)
{
    /* Bug #11603: Funai Electronics PM36B */
    if (memcmp(DDC->vendor.name, "FCM", 4) == 0 && DDC->vendor.prod_id == 13600)
        return TRUE;

    return FALSE;
}

static Bool
quirk_detailed_v_in_cm(int scrnIndex, xf86MonPtr DDC)
{
    /* Bug #11603: Funai Electronics PM36B */
    if (memcmp(DDC->vendor.name, "FCM", 4) == 0 && DDC->vendor.prod_id == 13600)
        return TRUE;

    /* Bug #21000: LGPhilipsLCD LP154W01-TLAJ */
    if (memcmp(DDC->vendor.name, "LPL", 4) == 0 && DDC->vendor.prod_id == 47360)
        return TRUE;

    /* Bug #10304: LGPhilipsLCD LP154W01-A5 */
    if (memcmp(DDC->vendor.name, "LPL", 4) == 0 && DDC->vendor.prod_id == 0)
        return TRUE;

    /* Bug #24482: LGPhilipsLCD LP154W01-TLA1 */
    if (memcmp(DDC->vendor.name, "LPL", 4) == 0 &&
        DDC->vendor.prod_id == 0x2a00)
        return TRUE;

    /* Bug #28414: HP Compaq NC8430 LP154W01-TLA8 */
    if (memcmp(DDC->vendor.name, "LPL", 4) == 0 && DDC->vendor.prod_id == 5750)
        return TRUE;

    /* Bug #21750: Samsung Syncmaster 2333HD */
    if (memcmp(DDC->vendor.name, "SAM", 4) == 0 && DDC->vendor.prod_id == 1157)
        return TRUE;

    return FALSE;
}

static Bool
quirk_detailed_use_maximum_size(int scrnIndex, xf86MonPtr DDC)
{
    /* ADA 1024x600 7" display */
    if (memcmp(DDC->vendor.name, "ADA", 4) == 0 &&
        DDC->vendor.prod_id == 4)
        return TRUE;

    /* Bug #21324: Iiyama Vision Master 450 */
    if (memcmp(DDC->vendor.name, "IVM", 4) == 0 && DDC->vendor.prod_id == 6400)
        return TRUE;

    /* Bug #41141: Acer Aspire One */
    if (memcmp(DDC->vendor.name, "LGD", 4) == 0 &&
        DDC->vendor.prod_id == 0x7f01)
        return TRUE;

    /* Sony Vaio Pro 13 */
    if (memcmp(DDC->vendor.name, "MEI", 4) == 0 &&
        DDC->vendor.prod_id == 0x96a2)
        return TRUE;

    return FALSE;
}

static Bool
quirk_135_clock_too_high(int scrnIndex, xf86MonPtr DDC)
{
    /* Envision Peripherals, Inc. EN-7100e.  See bug #9550. */
    if (memcmp(DDC->vendor.name, "EPI", 4) == 0 && DDC->vendor.prod_id == 59264)
        return TRUE;

    return FALSE;
}

static Bool
quirk_first_detailed_preferred(int scrnIndex, xf86MonPtr DDC)
{
    /* Philips 107p5 CRT. Reported on xorg@ with pastebin. */
    if (memcmp(DDC->vendor.name, "PHL", 4) == 0 && DDC->vendor.prod_id == 57364)
        return TRUE;

    /* Proview AY765C 17" LCD. See bug #15160 */
    if (memcmp(DDC->vendor.name, "PTS", 4) == 0 && DDC->vendor.prod_id == 765)
        return TRUE;

    /* ACR of some sort RH #284231 */
    if (memcmp(DDC->vendor.name, "ACR", 4) == 0 && DDC->vendor.prod_id == 2423)
        return TRUE;

    /* Peacock Ergovision 19.  See rh#492359 */
    if (memcmp(DDC->vendor.name, "PEA", 4) == 0 && DDC->vendor.prod_id == 9003)
        return TRUE;

    return FALSE;
}

static Bool
quirk_detailed_sync_pp(int scrnIndex, xf86MonPtr DDC)
{
    /* Bug #12439: Samsung SyncMaster 205BW */
    if (memcmp(DDC->vendor.name, "SAM", 4) == 0 && DDC->vendor.prod_id == 541)
        return TRUE;
    return FALSE;
}

/* This should probably be made more generic */
static Bool
quirk_dvi_single_link(int scrnIndex, xf86MonPtr DDC)
{
    /* Red Hat bug #453106: Apple 23" Cinema Display */
    if (memcmp(DDC->vendor.name, "APL", 4) == 0 &&
        DDC->vendor.prod_id == 0x921c)
        return TRUE;
    return FALSE;
}

typedef struct {
    Bool (*detect) (int scrnIndex, xf86MonPtr DDC);
    ddc_quirk_t quirk;
    const char *description;
} ddc_quirk_map_t;

static const ddc_quirk_map_t ddc_quirks[] = {
    {
     quirk_prefer_large_60, DDC_QUIRK_PREFER_LARGE_60,
     "Detailed timing is not preferred, use largest mode at 60Hz"},
    {
     quirk_135_clock_too_high, DDC_QUIRK_135_CLOCK_TOO_HIGH,
     "Recommended 135MHz pixel clock is too high"},
    {
     quirk_prefer_large_75, DDC_QUIRK_PREFER_LARGE_75,
     "Detailed timing is not preferred, use largest mode at 75Hz"},
    {
     quirk_detailed_h_in_cm, DDC_QUIRK_DETAILED_H_IN_CM,
     "Detailed timings give horizontal size in cm."},
    {
     quirk_detailed_v_in_cm, DDC_QUIRK_DETAILED_V_IN_CM,
     "Detailed timings give vertical size in cm."},
    {
     quirk_detailed_use_maximum_size, DDC_QUIRK_DETAILED_USE_MAXIMUM_SIZE,
     "Use maximum size instead of detailed timing sizes."},
    {
     quirk_first_detailed_preferred, DDC_QUIRK_FIRST_DETAILED_PREFERRED,
     "First detailed timing was not marked as preferred."},
    {
     quirk_detailed_sync_pp, DDC_QUIRK_DETAILED_SYNC_PP,
     "Use +hsync +vsync for detailed timing."},
    {
     quirk_dvi_single_link, DDC_QUIRK_DVI_SINGLE_LINK,
     "Forcing maximum pixel clock to single DVI link."},
    {
     NULL, DDC_QUIRK_NONE,
     "No known quirks"},
};

/*
 * These more or less come from the DMT spec.  The 720x400 modes are
 * inferred from historical 80x25 practice.  The 640x480@67 and 832x624@75
 * modes are old-school Mac modes.  The EDID spec says the 1152x864@75 mode
 * should be 1152x870, again for the Mac, but instead we use the x864 DMT
 * mode.
 *
 * The DMT modes have been fact-checked; the rest are mild guesses.
 */
#define MODEPREFIX NULL, NULL, NULL, 0, M_T_DRIVER
#define MODESUFFIX 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,FALSE,FALSE,0,NULL,0,0.0,0.0

static const DisplayModeRec DDCEstablishedModes[17] = {
    {MODEPREFIX, 40000, 800, 840, 968, 1056, 0, 600, 601, 605, 628, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 800x600@60Hz */
    {MODEPREFIX, 36000, 800, 824, 896, 1024, 0, 600, 601, 603, 625, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 800x600@56Hz */
    {MODEPREFIX, 31500, 640, 656, 720, 840, 0, 480, 481, 484, 500, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* 640x480@75Hz */
    {MODEPREFIX, 31500, 640, 664, 704, 832, 0, 480, 489, 492, 520, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* 640x480@72Hz */
    {MODEPREFIX, 30240, 640, 704, 768, 864, 0, 480, 483, 486, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* 640x480@67Hz */
    {MODEPREFIX, 25175, 640, 656, 752, 800, 0, 480, 490, 492, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* 640x480@60Hz */
    {MODEPREFIX, 35500, 720, 738, 846, 900, 0, 400, 421, 423, 449, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* 720x400@88Hz */
    {MODEPREFIX, 28320, 720, 738, 846, 900, 0, 400, 412, 414, 449, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX}, /* 720x400@70Hz */
    {MODEPREFIX, 135000, 1280, 1296, 1440, 1688, 0, 1024, 1025, 1028, 1066, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 1280x1024@75Hz */
    {MODEPREFIX, 78750, 1024, 1040, 1136, 1312, 0, 768, 769, 772, 800, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},     /* 1024x768@75Hz */
    {MODEPREFIX, 75000, 1024, 1048, 1184, 1328, 0, 768, 771, 777, 806, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* 1024x768@70Hz */
    {MODEPREFIX, 65000, 1024, 1048, 1184, 1344, 0, 768, 771, 777, 806, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* 1024x768@60Hz */
    {MODEPREFIX, 44900, 1024, 1032, 1208, 1264, 0, 768, 768, 772, 817, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE, MODESUFFIX},       /* 1024x768@43Hz */
    {MODEPREFIX, 57284, 832, 864, 928, 1152, 0, 624, 625, 628, 667, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},        /* 832x624@75Hz */
    {MODEPREFIX, 49500, 800, 816, 896, 1056, 0, 600, 601, 604, 625, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 800x600@75Hz */
    {MODEPREFIX, 50000, 800, 856, 976, 1040, 0, 600, 637, 643, 666, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 800x600@72Hz */
    {MODEPREFIX, 108000, 1152, 1216, 1344, 1600, 0, 864, 865, 868, 900, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},    /* 1152x864@75Hz */
};

static DisplayModePtr
DDCModesFromEstablished(int scrnIndex, struct established_timings *timing,
                        ddc_quirk_t quirks)
{
    DisplayModePtr Modes = NULL, Mode = NULL;
    CARD32 bits = (timing->t1) | (timing->t2 << 8) |
        ((timing->t_manu & 0x80) << 9);
    int i;

    for (i = 0; i < 17; i++) {
        if (bits & (0x01 << i)) {
            Mode = xf86DuplicateMode(&DDCEstablishedModes[i]);
            Modes = xf86ModesAdd(Modes, Mode);
        }
    }

    return Modes;
}

/* Autogenerated from the DMT spec */
const DisplayModeRec DMTModes[] = {
    {MODEPREFIX, 31500, 640, 672, 736, 832, 0, 350, 382, 385, 445, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX}, /* 640x350@85Hz */
    {MODEPREFIX, 31500, 640, 672, 736, 832, 0, 400, 401, 404, 445, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX}, /* 640x400@85Hz */
    {MODEPREFIX, 35500, 720, 756, 828, 936, 0, 400, 401, 404, 446, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX}, /* 720x400@85Hz */
    {MODEPREFIX, 25175, 640, 656, 752, 800, 0, 480, 490, 492, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* 640x480@60Hz */
    {MODEPREFIX, 31500, 640, 664, 704, 832, 0, 480, 489, 492, 520, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* 640x480@72Hz */
    {MODEPREFIX, 31500, 640, 656, 720, 840, 0, 480, 481, 484, 500, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* 640x480@75Hz */
    {MODEPREFIX, 36000, 640, 696, 752, 832, 0, 480, 481, 484, 509, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* 640x480@85Hz */
    {MODEPREFIX, 36000, 800, 824, 896, 1024, 0, 600, 601, 603, 625, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 800x600@56Hz */
    {MODEPREFIX, 40000, 800, 840, 968, 1056, 0, 600, 601, 605, 628, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 800x600@60Hz */
    {MODEPREFIX, 50000, 800, 856, 976, 1040, 0, 600, 637, 643, 666, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 800x600@72Hz */
    {MODEPREFIX, 49500, 800, 816, 896, 1056, 0, 600, 601, 604, 625, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 800x600@75Hz */
    {MODEPREFIX, 56250, 800, 832, 896, 1048, 0, 600, 601, 604, 631, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 800x600@85Hz */
    {MODEPREFIX, 73250, 800, 848, 880, 960, 0, 600, 603, 607, 636, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX}, /* 800x600@120Hz RB */
    {MODEPREFIX, 33750, 848, 864, 976, 1088, 0, 480, 486, 494, 517, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 848x480@60Hz */
    {MODEPREFIX, 44900, 1024, 1032, 1208, 1264, 0, 768, 768, 772, 817, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE, MODESUFFIX},       /* 1024x768@43Hz (interlaced) */
    {MODEPREFIX, 65000, 1024, 1048, 1184, 1344, 0, 768, 771, 777, 806, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* 1024x768@60Hz */
    {MODEPREFIX, 75000, 1024, 1048, 1184, 1328, 0, 768, 771, 777, 806, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* 1024x768@70Hz */
    {MODEPREFIX, 78750, 1024, 1040, 1136, 1312, 0, 768, 769, 772, 800, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},     /* 1024x768@75Hz */
    {MODEPREFIX, 94500, 1024, 1072, 1168, 1376, 0, 768, 769, 772, 808, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},     /* 1024x768@85Hz */
    {MODEPREFIX, 115500, 1024, 1072, 1104, 1184, 0, 768, 771, 775, 813, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},    /* 1024x768@120Hz RB */
    {MODEPREFIX, 108000, 1152, 1216, 1344, 1600, 0, 864, 865, 868, 900, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},    /* 1152x864@75Hz */
    {MODEPREFIX, 68250, 1280, 1328, 1360, 1440, 0, 768, 771, 778, 790, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},     /* 1280x768@60Hz RB */
    {MODEPREFIX, 79500, 1280, 1344, 1472, 1664, 0, 768, 771, 778, 798, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},     /* 1280x768@60Hz */
    {MODEPREFIX, 102250, 1280, 1360, 1488, 1696, 0, 768, 771, 778, 805, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},    /* 1280x768@75Hz */
    {MODEPREFIX, 117500, 1280, 1360, 1496, 1712, 0, 768, 771, 778, 809, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},    /* 1280x768@85Hz */
    {MODEPREFIX, 140250, 1280, 1328, 1360, 1440, 0, 768, 771, 778, 813, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},    /* 1280x768@120Hz RB */
    {MODEPREFIX, 71000, 1280, 1328, 1360, 1440, 0, 800, 803, 809, 823, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},     /* 1280x800@60Hz RB */
    {MODEPREFIX, 83500, 1280, 1352, 1480, 1680, 0, 800, 803, 809, 831, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},     /* 1280x800@60Hz */
    {MODEPREFIX, 106500, 1280, 1360, 1488, 1696, 0, 800, 803, 809, 838, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},    /* 1280x800@75Hz */
    {MODEPREFIX, 122500, 1280, 1360, 1496, 1712, 0, 800, 803, 809, 843, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},    /* 1280x800@85Hz */
    {MODEPREFIX, 146250, 1280, 1328, 1360, 1440, 0, 800, 803, 809, 847, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},    /* 1280x800@120Hz RB */
    {MODEPREFIX, 108000, 1280, 1376, 1488, 1800, 0, 960, 961, 964, 1000, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},   /* 1280x960@60Hz */
    {MODEPREFIX, 148500, 1280, 1344, 1504, 1728, 0, 960, 961, 964, 1011, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},   /* 1280x960@85Hz */
    {MODEPREFIX, 175500, 1280, 1328, 1360, 1440, 0, 960, 963, 967, 1017, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},   /* 1280x960@120Hz RB */
    {MODEPREFIX, 108000, 1280, 1328, 1440, 1688, 0, 1024, 1025, 1028, 1066, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 1280x1024@60Hz */
    {MODEPREFIX, 135000, 1280, 1296, 1440, 1688, 0, 1024, 1025, 1028, 1066, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 1280x1024@75Hz */
    {MODEPREFIX, 157500, 1280, 1344, 1504, 1728, 0, 1024, 1025, 1028, 1072, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 1280x1024@85Hz */
    {MODEPREFIX, 187250, 1280, 1328, 1360, 1440, 0, 1024, 1027, 1034, 1084, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1280x1024@120Hz RB */
    {MODEPREFIX, 85500, 1360, 1424, 1536, 1792, 0, 768, 771, 777, 795, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},     /* 1360x768@60Hz */
    {MODEPREFIX, 148250, 1360, 1408, 1440, 1520, 0, 768, 771, 776, 813, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},    /* 1360x768@120Hz RB */
    {MODEPREFIX, 101000, 1400, 1448, 1480, 1560, 0, 1050, 1053, 1057, 1080, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1400x1050@60Hz RB */
    {MODEPREFIX, 121750, 1400, 1488, 1632, 1864, 0, 1050, 1053, 1057, 1089, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1400x1050@60Hz */
    {MODEPREFIX, 156000, 1400, 1504, 1648, 1896, 0, 1050, 1053, 1057, 1099, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1400x1050@75Hz */
    {MODEPREFIX, 179500, 1400, 1504, 1656, 1912, 0, 1050, 1053, 1057, 1105, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1400x1050@85Hz */
    {MODEPREFIX, 208000, 1400, 1448, 1480, 1560, 0, 1050, 1053, 1057, 1112, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1400x1050@120Hz RB */
    {MODEPREFIX, 88750, 1440, 1488, 1520, 1600, 0, 900, 903, 909, 926, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},     /* 1440x900@60Hz RB */
    {MODEPREFIX, 106500, 1440, 1520, 1672, 1904, 0, 900, 903, 909, 934, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},    /* 1440x900@60Hz */
    {MODEPREFIX, 136750, 1440, 1536, 1688, 1936, 0, 900, 903, 909, 942, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},    /* 1440x900@75Hz */
    {MODEPREFIX, 157000, 1440, 1544, 1696, 1952, 0, 900, 903, 909, 948, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},    /* 1440x900@85Hz */
    {MODEPREFIX, 182750, 1440, 1488, 1520, 1600, 0, 900, 903, 909, 953, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},    /* 1440x900@120Hz RB */
    {MODEPREFIX, 162000, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 1600x1200@60Hz */
    {MODEPREFIX, 175500, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 1600x1200@65Hz */
    {MODEPREFIX, 189000, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 1600x1200@70Hz */
    {MODEPREFIX, 202500, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 1600x1200@75Hz */
    {MODEPREFIX, 229500, 1600, 1664, 1856, 2160, 0, 1200, 1201, 1204, 1250, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* 1600x1200@85Hz */
    {MODEPREFIX, 268250, 1600, 1648, 1680, 1760, 0, 1200, 1203, 1207, 1271, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1600x1200@120Hz RB */
    {MODEPREFIX, 119000, 1680, 1728, 1760, 1840, 0, 1050, 1053, 1059, 1080, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1680x1050@60Hz RB */
    {MODEPREFIX, 146250, 1680, 1784, 1960, 2240, 0, 1050, 1053, 1059, 1089, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1680x1050@60Hz */
    {MODEPREFIX, 187000, 1680, 1800, 1976, 2272, 0, 1050, 1053, 1059, 1099, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1680x1050@75Hz */
    {MODEPREFIX, 214750, 1680, 1808, 1984, 2288, 0, 1050, 1053, 1059, 1105, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1680x1050@85Hz */
    {MODEPREFIX, 245500, 1680, 1728, 1760, 1840, 0, 1050, 1053, 1059, 1112, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1680x1050@120Hz RB */
    {MODEPREFIX, 204750, 1792, 1920, 2120, 2448, 0, 1344, 1345, 1348, 1394, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1792x1344@60Hz */
    {MODEPREFIX, 261000, 1792, 1888, 2104, 2456, 0, 1344, 1345, 1348, 1417, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1792x1344@75Hz */
    {MODEPREFIX, 333250, 1792, 1840, 1872, 1952, 0, 1344, 1347, 1351, 1423, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1792x1344@120Hz RB */
    {MODEPREFIX, 218250, 1856, 1952, 2176, 2528, 0, 1392, 1393, 1396, 1439, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1856x1392@60Hz */
    {MODEPREFIX, 288000, 1856, 1984, 2208, 2560, 0, 1392, 1393, 1396, 1500, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1856x1392@75Hz */
    {MODEPREFIX, 356500, 1856, 1904, 1936, 2016, 0, 1392, 1395, 1399, 1474, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1856x1392@120Hz RB */
    {MODEPREFIX, 154000, 1920, 1968, 2000, 2080, 0, 1200, 1203, 1209, 1235, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1920x1200@60Hz RB */
    {MODEPREFIX, 193250, 1920, 2056, 2256, 2592, 0, 1200, 1203, 1209, 1245, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1920x1200@60Hz */
    {MODEPREFIX, 245250, 1920, 2056, 2264, 2608, 0, 1200, 1203, 1209, 1255, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1920x1200@75Hz */
    {MODEPREFIX, 281250, 1920, 2064, 2272, 2624, 0, 1200, 1203, 1209, 1262, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1920x1200@85Hz */
    {MODEPREFIX, 317000, 1920, 1968, 2000, 2080, 0, 1200, 1203, 1209, 1271, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1920x1200@120Hz RB */
    {MODEPREFIX, 234000, 1920, 2048, 2256, 2600, 0, 1440, 1441, 1444, 1500, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1920x1440@60Hz */
    {MODEPREFIX, 297000, 1920, 2064, 2288, 2640, 0, 1440, 1441, 1444, 1500, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 1920x1440@75Hz */
    {MODEPREFIX, 380500, 1920, 1968, 2000, 2080, 0, 1440, 1443, 1447, 1525, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 1920x1440@120Hz RB */
    {MODEPREFIX, 268500, 2560, 2608, 2640, 2720, 0, 1600, 1603, 1609, 1646, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 2560x1600@60Hz RB */
    {MODEPREFIX, 348500, 2560, 2752, 3032, 3504, 0, 1600, 1603, 1609, 1658, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 2560x1600@60Hz */
    {MODEPREFIX, 443250, 2560, 2768, 3048, 3536, 0, 1600, 1603, 1609, 1672, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 2560x1600@75Hz */
    {MODEPREFIX, 505250, 2560, 2768, 3048, 3536, 0, 1600, 1603, 1609, 1682, 0, V_NHSYNC | V_PVSYNC, MODESUFFIX},        /* 2560x1600@85Hz */
    {MODEPREFIX, 552750, 2560, 2608, 2640, 2720, 0, 1600, 1603, 1609, 1694, 0, V_PHSYNC | V_NVSYNC, MODESUFFIX},        /* 2560x1600@120Hz RB */
};

#define LEVEL_DMT 0
#define LEVEL_GTF 1
#define LEVEL_CVT 2

static int
MonitorStandardTimingLevel(xf86MonPtr DDC)
{
    if (DDC->ver.revision >= 2) {
        if (DDC->ver.revision >= 4 && CVT_SUPPORTED(DDC->features.msc)) {
            return LEVEL_CVT;
        }
        return LEVEL_GTF;
    }
    return LEVEL_DMT;
}

static int
ModeRefresh(const DisplayModeRec * mode)
{
    return (int) (xf86ModeVRefresh(mode) + 0.5);
}

/*
 * If rb is not set, then we'll not consider reduced-blanking modes as
 * part of the DMT pool.  For the 'standard' EDID mode descriptor there's
 * no way to specify whether the mode should be RB or not.
 */
DisplayModePtr
FindDMTMode(int hsize, int vsize, int refresh, Bool rb)
{
    int i;
    const DisplayModeRec *ret;

    for (i = 0; i < ARRAY_SIZE(DMTModes); i++) {
        ret = &DMTModes[i];

        if (!rb && xf86ModeIsReduced(ret))
            continue;

        if (ret->HDisplay == hsize &&
            ret->VDisplay == vsize && refresh == ModeRefresh(ret))
            return xf86DuplicateMode(ret);
    }

    return NULL;
}

/*
 * Appendix B of the EDID 1.4 spec defines the right thing to do here.
 * If the timing given here matches a mode defined in the VESA DMT standard,
 * we _must_ use that.  If the device supports CVT modes, then we should
 * generate a CVT timing.  If both of the above fail, use GTF.
 *
 * There are some wrinkles here.  EDID 1.1 and 1.0 sinks can't really
 * "support" GTF, since it wasn't a standard yet; so if they ask for a
 * timing in this section that isn't defined in DMT, returning a GTF mode
 * may not actually be valid.  EDID 1.3 sinks often report support for
 * some CVT modes, but they are not required to support CVT timings for
 * modes in the standard timing descriptor, so we should _not_ treat them
 * as CVT-compliant (unless specified in an extension block I suppose).
 *
 * EDID 1.4 requires that all sink devices support both GTF and CVT timings
 * for modes in this section, but does say that CVT is preferred.
 */
static DisplayModePtr
DDCModesFromStandardTiming(DisplayModePtr pool, struct std_timings *timing,
                           ddc_quirk_t quirks,
                           int timing_level, Bool rb)
{
    DisplayModePtr Modes = NULL, Mode = NULL;
    int i, hsize, vsize, refresh;

    for (i = 0; i < STD_TIMINGS; i++) {
        DisplayModePtr p = NULL;
        hsize = timing[i].hsize;
        vsize = timing[i].vsize;
        refresh = timing[i].refresh;

        /* HDTV hack, part one */
        if (refresh == 60 &&
            ((hsize == 1360 && vsize == 765) ||
             (hsize == 1368 && vsize == 769))) {
            hsize = 1366;
            vsize = 768;
        }

        /* If we already have a detailed timing for this size, don't add more */
        for (p = pool; p; p = p->next) {
            if (p->HDisplay == hsize && p->VDisplay == vsize &&
                refresh == round(xf86ModeVRefresh(p)))
                break;
        }
        if (p)
            continue;

        /* HDTV hack, because you can't say 1366 */
        if (refresh == 60 && hsize == 1366 && vsize == 768) {
            Mode = xf86CVTMode(1366, 768, 60, FALSE, FALSE);
            Mode->HDisplay = 1366;
            Mode->HSyncStart--;
            Mode->HSyncEnd--;
        }
        else if (hsize && vsize && refresh) {
            Mode = FindDMTMode(hsize, vsize, refresh, rb);

            if (!Mode) {
                if (timing_level == LEVEL_CVT)
                    /* pass rb here too? */
                    Mode = xf86CVTMode(hsize, vsize, refresh, FALSE, FALSE);
                else if (timing_level == LEVEL_GTF)
                    Mode = xf86GTFMode(hsize, vsize, refresh, FALSE, FALSE);
            }

        }

        if (Mode) {
            Mode->type = M_T_DRIVER;
            Modes = xf86ModesAdd(Modes, Mode);
        }
        Mode = NULL;
    }

    return Modes;
}

static void
DDCModeDoInterlaceQuirks(DisplayModePtr mode)
{
    /*
     * EDID is delightfully ambiguous about how interlaced modes are to be
     * encoded.  X's internal representation is of frame height, but some
     * HDTV detailed timings are encoded as field height.
     *
     * The format list here is from CEA, in frame size.  Technically we
     * should be checking refresh rate too.  Whatever.
     */
    static const struct {
        int w, h;
    } cea_interlaced[] = {
        {1920, 1080},
        {720, 480},
        {1440, 480},
        {2880, 480},
        {720, 576},
        {1440, 576},
        {2880, 576},
    };
    int i;

    for (i = 0; i < ARRAY_SIZE(cea_interlaced); i++) {
        if ((mode->HDisplay == cea_interlaced[i].w) &&
            (mode->VDisplay == cea_interlaced[i].h / 2)) {
            mode->VDisplay *= 2;
            mode->VSyncStart *= 2;
            mode->VSyncEnd *= 2;
            mode->VTotal *= 2;
            mode->VTotal |= 1;
        }
    }

    mode->Flags |= V_INTERLACE;
}

/*
 *
 */
static DisplayModePtr
DDCModeFromDetailedTiming(int scrnIndex, struct detailed_timings *timing,
                          Bool preferred, ddc_quirk_t quirks)
{
    DisplayModePtr Mode;

    /*
     * Refuse to create modes that are insufficiently large.  64 is a random
     * number, maybe the spec says something about what the minimum is.  In
     * particular I see this frequently with _old_ EDID, 1.0 or so, so maybe
     * our parser is just being too aggressive there.
     */
    if (timing->h_active < 64 || timing->v_active < 64) {
        xf86DrvMsg(scrnIndex, X_INFO,
                   "%s: Ignoring tiny %dx%d mode\n", __func__,
                   timing->h_active, timing->v_active);
        return NULL;
    }

    /* We don't do stereo */
    if (timing->stereo) {
        xf86DrvMsg(scrnIndex, X_INFO,
                   "%s: Ignoring: We don't handle stereo.\n", __func__);
        return NULL;
    }

    /* We only do separate sync currently */
    if (timing->sync != 0x03) {
        xf86DrvMsg(scrnIndex, X_INFO,
                   "%s: %dx%d Warning: We only handle separate"
                   " sync.\n", __func__, timing->h_active, timing->v_active);
    }

    Mode = xnfcalloc(1, sizeof(DisplayModeRec));

    Mode->type = M_T_DRIVER;
    if (preferred)
        Mode->type |= M_T_PREFERRED;

    if ((quirks & DDC_QUIRK_135_CLOCK_TOO_HIGH) && timing->clock == 135000000)
        Mode->Clock = 108880;
    else
        Mode->Clock = timing->clock / 1000.0;

    Mode->HDisplay = timing->h_active;
    Mode->HSyncStart = timing->h_active + timing->h_sync_off;
    Mode->HSyncEnd = Mode->HSyncStart + timing->h_sync_width;
    Mode->HTotal = timing->h_active + timing->h_blanking;

    Mode->VDisplay = timing->v_active;
    Mode->VSyncStart = timing->v_active + timing->v_sync_off;
    Mode->VSyncEnd = Mode->VSyncStart + timing->v_sync_width;
    Mode->VTotal = timing->v_active + timing->v_blanking;

    /* perform basic check on the detail timing */
    if (Mode->HSyncEnd > Mode->HTotal || Mode->VSyncEnd > Mode->VTotal) {
        free(Mode);
        return NULL;
    }

    /* We ignore h/v_size and h/v_border for now. */

    if (timing->interlaced)
        DDCModeDoInterlaceQuirks(Mode);

    if (quirks & DDC_QUIRK_DETAILED_SYNC_PP)
        Mode->Flags |= V_PVSYNC | V_PHSYNC;
    else {
        if (timing->misc & 0x02)
            Mode->Flags |= V_PVSYNC;
        else
            Mode->Flags |= V_NVSYNC;

        if (timing->misc & 0x01)
            Mode->Flags |= V_PHSYNC;
        else
            Mode->Flags |= V_NHSYNC;
    }

    xf86SetModeDefaultName(Mode);

    return Mode;
}

static DisplayModePtr
DDCModesFromCVT(int scrnIndex, struct cvt_timings *t)
{
    DisplayModePtr modes = NULL;
    int i;

    for (i = 0; i < 4; i++) {
        if (t[i].height) {
            if (t[i].rates & 0x10)
                modes = xf86ModesAdd(modes,
                                     xf86CVTMode(t[i].width, t[i].height, 50, 0,
                                                 0));
            if (t[i].rates & 0x08)
                modes = xf86ModesAdd(modes,
                                     xf86CVTMode(t[i].width, t[i].height, 60, 0,
                                                 0));
            if (t[i].rates & 0x04)
                modes = xf86ModesAdd(modes,
                                     xf86CVTMode(t[i].width, t[i].height, 75, 0,
                                                 0));
            if (t[i].rates & 0x02)
                modes = xf86ModesAdd(modes,
                                     xf86CVTMode(t[i].width, t[i].height, 85, 0,
                                                 0));
            if (t[i].rates & 0x01)
                modes = xf86ModesAdd(modes,
                                     xf86CVTMode(t[i].width, t[i].height, 60, 1,
                                                 0));
        }
        else
            break;
    }

    return modes;
}

static const struct {
    short w;
    short h;
    short r;
    short rb;
} EstIIIModes[] = {
        /* byte 6 */
    {640, 350, 85, 0},
    {640, 400, 85, 0},
    {720, 400, 85, 0},
    {640, 480, 85, 0},
    {848, 480, 60, 0},
    {800, 600, 85, 0},
    {1024, 768, 85, 0},
    {1152, 864, 75, 0},
        /* byte 7 */
    {1280, 768, 60, 1},
    {1280, 768, 60, 0},
    {1280, 768, 75, 0},
    {1280, 768, 85, 0},
    {1280, 960, 60, 0},
    {1280, 960, 85, 0},
    {1280, 1024, 60, 0},
    {1280, 1024, 85, 0},
        /* byte 8 */
    {1360, 768, 60, 0},
    {1440, 900, 60, 1},
    {1440, 900, 60, 0},
    {1440, 900, 75, 0},
    {1440, 900, 85, 0},
    {1400, 1050, 60, 1},
    {1400, 1050, 60, 0},
    {1400, 1050, 75, 0},
        /* byte 9 */
    {1400, 1050, 85, 0},
    {1680, 1050, 60, 1},
    {1680, 1050, 60, 0},
    {1680, 1050, 75, 0},
    {1680, 1050, 85, 0},
    {1600, 1200, 60, 0},
    {1600, 1200, 65, 0},
    {1600, 1200, 70, 0},
        /* byte 10 */
    {1600, 1200, 75, 0},
    {1600, 1200, 85, 0},
    {1792, 1344, 60, 0},
    {1792, 1344, 75, 0},
    {1856, 1392, 60, 0},
    {1856, 1392, 75, 0},
    {1920, 1200, 60, 1},
    {1920, 1200, 60, 0},
        /* byte 11 */
    {1920, 1200, 75, 0},
    {1920, 1200, 85, 0},
    {1920, 1440, 60, 0},
    {1920, 1440, 75, 0},
        /* fill up last byte */
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0},
    {0,0,0,0},
};

static DisplayModePtr
DDCModesFromEstIII(unsigned char *est)
{
    DisplayModePtr modes = NULL;
    int i, j, m;

    for (i = 0; i < 6; i++) {
        for (j = 7; j >= 0; j--) {
            if (est[i] & (1 << j)) {
                m = (i * 8) + (7 - j);
                if (EstIIIModes[m].w)
                    modes = xf86ModesAdd(modes,
                                     FindDMTMode(EstIIIModes[m].w,
                                                 EstIIIModes[m].h,
                                                 EstIIIModes[m].r,
                                                 EstIIIModes[m].rb));
            }
        }
    }

    return modes;
}

/*
 * This is only valid when the sink claims to be continuous-frequency
 * but does not supply a detailed range descriptor.  Such sinks are
 * arguably broken.  Currently the mode validation code isn't aware of
 * this; the non-RANDR code even punts the decision of optional sync
 * range checking to the driver.  Loss.
 */
static void
DDCGuessRangesFromModes(int scrnIndex, MonPtr Monitor, DisplayModePtr Modes)
{
    DisplayModePtr Mode = Modes;

    if (!Monitor || !Modes)
        return;

    /* set up the ranges for scanning through the modes */
    Monitor->nHsync = 1;
    Monitor->hsync[0].lo = 1024.0;
    Monitor->hsync[0].hi = 0.0;

    Monitor->nVrefresh = 1;
    Monitor->vrefresh[0].lo = 1024.0;
    Monitor->vrefresh[0].hi = 0.0;

    while (Mode) {
        if (!Mode->HSync)
            Mode->HSync = ((float) Mode->Clock) / ((float) Mode->HTotal);

        if (!Mode->VRefresh)
            Mode->VRefresh = (1000.0 * ((float) Mode->Clock)) /
                ((float) (Mode->HTotal * Mode->VTotal));

        if (Mode->HSync < Monitor->hsync[0].lo)
            Monitor->hsync[0].lo = Mode->HSync;

        if (Mode->HSync > Monitor->hsync[0].hi)
            Monitor->hsync[0].hi = Mode->HSync;

        if (Mode->VRefresh < Monitor->vrefresh[0].lo)
            Monitor->vrefresh[0].lo = Mode->VRefresh;

        if (Mode->VRefresh > Monitor->vrefresh[0].hi)
            Monitor->vrefresh[0].hi = Mode->VRefresh;

        Mode = Mode->next;
    }
}

ddc_quirk_t
xf86DDCDetectQuirks(int scrnIndex, xf86MonPtr DDC, Bool verbose)
{
    ddc_quirk_t quirks;
    int i;

    quirks = DDC_QUIRK_NONE;
    for (i = 0; ddc_quirks[i].detect; i++) {
        if (ddc_quirks[i].detect(scrnIndex, DDC)) {
            if (verbose) {
                xf86DrvMsg(scrnIndex, X_INFO, "    EDID quirk: %s\n",
                           ddc_quirks[i].description);
            }
            quirks |= ddc_quirks[i].quirk;
        }
    }

    return quirks;
}

void
xf86DetTimingApplyQuirks(struct detailed_monitor_section *det_mon,
                         ddc_quirk_t quirks, int hsize, int vsize)
{
    if (det_mon->type != DT)
        return;

    if (quirks & DDC_QUIRK_DETAILED_H_IN_CM)
        det_mon->section.d_timings.h_size *= 10;

    if (quirks & DDC_QUIRK_DETAILED_V_IN_CM)
        det_mon->section.d_timings.v_size *= 10;

    if (quirks & DDC_QUIRK_DETAILED_USE_MAXIMUM_SIZE) {
        det_mon->section.d_timings.h_size = 10 * hsize;
        det_mon->section.d_timings.v_size = 10 * vsize;
    }
}

/**
 * Applies monitor-specific quirks to the decoded EDID information.
 *
 * Note that some quirks applying to the mode list are still implemented in
 * xf86DDCGetModes.
 */
void
xf86DDCApplyQuirks(int scrnIndex, xf86MonPtr DDC)
{
    ddc_quirk_t quirks = xf86DDCDetectQuirks(scrnIndex, DDC, FALSE);
    int i;

    for (i = 0; i < DET_TIMINGS; i++) {
        xf86DetTimingApplyQuirks(DDC->det_mon + i, quirks,
                                 DDC->features.hsize, DDC->features.vsize);
    }
}

/**
 * Walks the modes list, finding the mode with the largest area which is
 * closest to the target refresh rate, and marks it as the only preferred mode.
*/
static void
xf86DDCSetPreferredRefresh(int scrnIndex, DisplayModePtr modes,
                           float target_refresh)
{
    DisplayModePtr mode, best = modes;

    for (mode = modes; mode; mode = mode->next) {
        mode->type &= ~M_T_PREFERRED;

        if (mode == best)
            continue;

        if (mode->HDisplay * mode->VDisplay > best->HDisplay * best->VDisplay) {
            best = mode;
            continue;
        }
        if (mode->HDisplay * mode->VDisplay == best->HDisplay * best->VDisplay) {
            double mode_refresh = xf86ModeVRefresh(mode);
            double best_refresh = xf86ModeVRefresh(best);
            double mode_dist = fabs(mode_refresh - target_refresh);
            double best_dist = fabs(best_refresh - target_refresh);

            if (mode_dist < best_dist) {
                best = mode;
                continue;
            }
        }
    }
    if (best)
        best->type |= M_T_PREFERRED;
}

#define CEA_VIDEO_MODES_NUM  64
static const DisplayModeRec CEAVideoModes[CEA_VIDEO_MODES_NUM] = {
    {MODEPREFIX, 25175, 640, 656, 752, 800, 0, 480, 490, 492, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* VIC 1:640x480@60Hz */
    {MODEPREFIX, 27000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* VIC 2:720x480@60Hz */
    {MODEPREFIX, 27000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* VIC 3:720x480@60Hz */
    {MODEPREFIX, 74250, 1280, 1390, 1430, 1650, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},     /* VIC 4: 1280x720@60Hz */
    {MODEPREFIX, 74250, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1094, 1125, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE, MODESUFFIX},   /* VIC 5:1920x1080i@60Hz */
    {MODEPREFIX, 27000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},       /* VIC 6:1440x480i@60Hz */
    {MODEPREFIX, 27000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},       /* VIC 7:1440x480i@60Hz */
    {MODEPREFIX, 27000, 1440, 1478, 1602, 1716, 0, 240, 244, 247, 262, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 8:1440x240@60Hz */
    {MODEPREFIX, 27000, 1440, 1478, 1602, 1716, 0, 240, 244, 247, 262, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 9:1440x240@60Hz */
    {MODEPREFIX, 54000, 2880, 2956, 3204, 3432, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},       /* VIC 10:2880x480i@60Hz */
    {MODEPREFIX, 54000, 2880, 2956, 3204, 3432, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},       /* VIC 11:2880x480i@60Hz */
    {MODEPREFIX, 54000, 2880, 2956, 3204, 3432, 0, 240, 244, 247, 262, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 12:2880x240@60Hz */
    {MODEPREFIX, 54000, 2880, 2956, 3204, 3432, 0, 240, 244, 247, 262, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 13:2880x240@60Hz */
    {MODEPREFIX, 54000, 1440, 1472, 1596, 1716, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 14:1440x480@60Hz */
    {MODEPREFIX, 54000, 1440, 1472, 1596, 1716, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 15:1440x480@60Hz */
    {MODEPREFIX, 148500, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* VIC 16:1920x1080@60Hz */
    {MODEPREFIX, 27000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* VIC 17:720x576@50Hz */
    {MODEPREFIX, 27000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* VIC 18:720x576@50Hz */
    {MODEPREFIX, 74250, 1280, 1720, 1760, 1980, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},     /* VIC 19: 1280x720@50Hz */
    {MODEPREFIX, 74250, 1920, 2448, 2492, 2640, 0, 1080, 1084, 1094, 1125, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE, MODESUFFIX},   /* VIC 20:1920x1080i@50Hz */
    {MODEPREFIX, 27000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},       /* VIC 21:1440x576i@50Hz */
    {MODEPREFIX, 27000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},       /* VIC 22:1440x576i@50Hz */
    {MODEPREFIX, 27000, 1440, 1464, 1590, 1728, 0, 288, 290, 293, 312, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 23:1440x288@50Hz */
    {MODEPREFIX, 27000, 1440, 1464, 1590, 1728, 0, 288, 290, 293, 312, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 24:1440x288@50Hz */
    {MODEPREFIX, 54000, 2880, 2928, 3180, 3456, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},       /* VIC 25:2880x576i@50Hz */
    {MODEPREFIX, 54000, 2880, 2928, 3180, 3456, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},       /* VIC 26:2880x576i@50Hz */
    {MODEPREFIX, 54000, 2880, 2928, 3180, 3456, 0, 288, 290, 293, 312, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 27:2880x288@50Hz */
    {MODEPREFIX, 54000, 2880, 2928, 3180, 3456, 0, 288, 290, 293, 312, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 28:2880x288@50Hz */
    {MODEPREFIX, 54000, 1440, 1464, 1592, 1728, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 29:1440x576@50Hz */
    {MODEPREFIX, 54000, 1440, 1464, 1592, 1728, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 30:1440x576@50Hz */
    {MODEPREFIX, 148500, 1920, 2448, 2492, 2640, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* VIC 31:1920x1080@50Hz */
    {MODEPREFIX, 74250, 1920, 2558, 2602, 2750, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX}, /* VIC 32:1920x1080@24Hz */
    {MODEPREFIX, 74250, 1920, 2448, 2492, 2640, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX}, /* VIC 33:1920x1080@25Hz */
    {MODEPREFIX, 74250, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX}, /* VIC 34:1920x1080@30Hz */
    {MODEPREFIX, 108000, 2880, 2944, 3192, 3432, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},    /* VIC 35:2880x480@60Hz */
    {MODEPREFIX, 108000, 2880, 2944, 3192, 3432, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},    /* VIC 36:2880x480@60Hz */
    {MODEPREFIX, 108000, 2880, 2928, 3184, 3456, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},    /* VIC 37:2880x576@50Hz */
    {MODEPREFIX, 108000, 2880, 2928, 3184, 3456, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},    /* VIC 38:2880x576@50Hz */
    {MODEPREFIX, 72000, 1920, 1952, 2120, 2304, 0, 1080, 1126, 1136, 1250, 0, V_PHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},   /* VIC 39:1920x1080i@50Hz */
    {MODEPREFIX, 148500, 1920, 2448, 2492, 2640, 0, 1080, 1084, 1094, 1125, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE, MODESUFFIX},  /* VIC 40:1920x1080i@100Hz */
    {MODEPREFIX, 148500, 1280, 1720, 1760, 1980, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},    /* VIC 41:1280x720@100Hz */
    {MODEPREFIX, 54000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* VIC 42:720x576@100Hz */
    {MODEPREFIX, 54000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* VIC 43:720x576@100Hz */
    {MODEPREFIX, 54000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 44:1440x576i@100Hz */
    {MODEPREFIX, 54000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},     /* VIC 45:1440x576i@100Hz */
    {MODEPREFIX, 148500, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1094, 1125, 0, V_PHSYNC | V_PVSYNC | V_INTERLACE, MODESUFFIX},  /* VIC 46:1920x1080i@120Hz */
    {MODEPREFIX, 148500, 1280, 1390, 1430, 1650, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},    /* VIC 47:1280x720@120Hz */
    {MODEPREFIX, 54000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* VIC 48:720x480@120Hz */
    {MODEPREFIX, 54000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX}, /* VIC 49:720x480@120Hz */
    {MODEPREFIX, 54000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},       /* VIC 50:1440x480i@120Hz */
    {MODEPREFIX, 54000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},       /* VIC 51:1440x480i@120Hz */
    {MODEPREFIX, 108000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},        /* VIC 52:720x576@200Hz */
    {MODEPREFIX, 108000, 720, 732, 796, 864, 0, 576, 581, 586, 625, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},        /* VIC 53:720x576@200Hz */
    {MODEPREFIX, 108000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},      /* VIC 54:1440x576i@200Hz */
    {MODEPREFIX, 108000, 1440, 1464, 1590, 1728, 0, 576, 580, 586, 625, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},      /* VIC 55:1440x576i@200Hz */
    {MODEPREFIX, 108000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},        /* VIC 56:720x480@240Hz */
    {MODEPREFIX, 108000, 720, 736, 798, 858, 0, 480, 489, 495, 525, 0, V_NHSYNC | V_NVSYNC, MODESUFFIX},        /* VIC 57:720x480@240Hz */
    {MODEPREFIX, 108000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},      /* VIC 58:1440x480i@240 */
    {MODEPREFIX, 108000, 1440, 1478, 1602, 1716, 0, 480, 488, 494, 525, 0, V_NHSYNC | V_NVSYNC | V_INTERLACE, MODESUFFIX},      /* VIC 59:1440x480i@240 */
    {MODEPREFIX, 59400, 1280, 3040, 3080, 3300, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},     /* VIC 60: 1280x720@24Hz */
    {MODEPREFIX, 74250, 1280, 3700, 3740, 3960, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},     /* VIC 61: 1280x720@25Hz */
    {MODEPREFIX, 74250, 1280, 3040, 3080, 3300, 0, 720, 725, 730, 750, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},     /* VIC 62: 1280x720@30Hz */
    {MODEPREFIX, 297000, 1920, 2008, 2052, 2200, 0, 1080, 1084, 1089, 1125, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* VIC 63: 1920x1080@120Hz */
    {MODEPREFIX, 297000, 1920, 2448, 2492, 2640, 0, 1080, 1084, 1094, 1125, 0, V_PHSYNC | V_PVSYNC, MODESUFFIX},        /* VIC 64:1920x1080@100Hz */
};

/* chose mode line by cea short video descriptor*/
static void
handle_cea_svd(struct cea_video_block *video, void *data)
{
    DisplayModePtr Mode;
    DisplayModePtr *Modes = (DisplayModePtr *) data;
    int vid;

    vid = video->video_code & 0x7f;
    if (vid >= 1 && vid <= CEA_VIDEO_MODES_NUM) {
        Mode = xf86DuplicateMode(CEAVideoModes + (vid - 1));
        *Modes = xf86ModesAdd(*Modes, Mode);
    }
}

static DisplayModePtr
DDCModesFromCEAExtension(int scrnIndex, xf86MonPtr mon_ptr)
{
    DisplayModePtr Modes = NULL;

    xf86ForEachVideoBlock(mon_ptr, handle_cea_svd, &Modes);

    return Modes;
}

struct det_modes_parameter {
    xf86MonPtr DDC;
    ddc_quirk_t quirks;
    DisplayModePtr Modes;
    Bool rb;
    Bool preferred;
    int timing_level;
};

static void
handle_detailed_modes(struct detailed_monitor_section *det_mon, void *data)
{
    DisplayModePtr Mode;
    struct det_modes_parameter *p = (struct det_modes_parameter *) data;

    xf86DetTimingApplyQuirks(det_mon, p->quirks,
                             p->DDC->features.hsize, p->DDC->features.vsize);

    switch (det_mon->type) {
    case DT:
        Mode = DDCModeFromDetailedTiming(p->DDC->scrnIndex,
                                         &det_mon->section.d_timings,
                                         p->preferred, p->quirks);
        p->preferred = FALSE;
        p->Modes = xf86ModesAdd(p->Modes, Mode);
        break;
    case DS_STD_TIMINGS:
        Mode = DDCModesFromStandardTiming(p->Modes,
                                          det_mon->section.std_t,
                                          p->quirks, p->timing_level, p->rb);
        p->Modes = xf86ModesAdd(p->Modes, Mode);
        break;
    case DS_CVT:
        Mode = DDCModesFromCVT(p->DDC->scrnIndex, det_mon->section.cvt);
        p->Modes = xf86ModesAdd(p->Modes, Mode);
        break;
    case DS_EST_III:
        Mode = DDCModesFromEstIII(det_mon->section.est_iii);
        p->Modes = xf86ModesAdd(p->Modes, Mode);
        break;
    default:
        break;
    }
}

DisplayModePtr
xf86DDCGetModes(int scrnIndex, xf86MonPtr DDC)
{
    DisplayModePtr Modes = NULL, Mode;
    ddc_quirk_t quirks;
    Bool preferred, rb;
    int timing_level;
    struct det_modes_parameter p;

    xf86DrvMsg(scrnIndex, X_INFO, "EDID vendor \"%s\", prod id %d\n",
               DDC->vendor.name, DDC->vendor.prod_id);

    quirks = xf86DDCDetectQuirks(scrnIndex, DDC, TRUE);

    preferred = PREFERRED_TIMING_MODE(DDC->features.msc);
    if (DDC->ver.revision >= 4)
        preferred = TRUE;
    if (quirks & DDC_QUIRK_FIRST_DETAILED_PREFERRED)
        preferred = TRUE;
    if (quirks & (DDC_QUIRK_PREFER_LARGE_60 | DDC_QUIRK_PREFER_LARGE_75))
        preferred = FALSE;

    rb = xf86MonitorSupportsReducedBlanking(DDC);

    timing_level = MonitorStandardTimingLevel(DDC);

    p.quirks = quirks;
    p.DDC = DDC;
    p.Modes = Modes;
    p.rb = rb;
    p.preferred = preferred;
    p.timing_level = timing_level;
    xf86ForEachDetailedBlock(DDC, handle_detailed_modes, &p);
    Modes = p.Modes;

    /* Add cea-extension mode timings */
    Mode = DDCModesFromCEAExtension(scrnIndex, DDC);
    Modes = xf86ModesAdd(Modes, Mode);

    /* Add established timings */
    Mode = DDCModesFromEstablished(scrnIndex, &DDC->timings1, quirks);
    Modes = xf86ModesAdd(Modes, Mode);

    /* Add standard timings */
    Mode = DDCModesFromStandardTiming(Modes, DDC->timings2, quirks,
                                      timing_level, rb);
    Modes = xf86ModesAdd(Modes, Mode);

    if (quirks & DDC_QUIRK_PREFER_LARGE_60)
        xf86DDCSetPreferredRefresh(scrnIndex, Modes, 60);

    if (quirks & DDC_QUIRK_PREFER_LARGE_75)
        xf86DDCSetPreferredRefresh(scrnIndex, Modes, 75);

    Modes = xf86PruneDuplicateModes(Modes);

    return Modes;
}

struct det_mon_parameter {
    MonPtr Monitor;
    ddc_quirk_t quirks;
    Bool have_hsync;
    Bool have_vrefresh;
    Bool have_maxpixclock;
};

static void
handle_detailed_monset(struct detailed_monitor_section *det_mon, void *data)
{
    int clock;
    struct det_mon_parameter *p = (struct det_mon_parameter *) data;
    int scrnIndex = ((xf86MonPtr) (p->Monitor->DDC))->scrnIndex;

    switch (det_mon->type) {
    case DS_RANGES:
        if (!p->have_hsync) {
            if (!p->Monitor->nHsync)
                xf86DrvMsg(scrnIndex, X_INFO,
                           "Using EDID range info for horizontal sync\n");
            p->Monitor->hsync[p->Monitor->nHsync].lo =
                det_mon->section.ranges.min_h;
            p->Monitor->hsync[p->Monitor->nHsync].hi =
                det_mon->section.ranges.max_h;
            p->Monitor->nHsync++;
        }
        else {
            xf86DrvMsg(scrnIndex, X_INFO,
                       "Using hsync ranges from config file\n");
        }

        if (!p->have_vrefresh) {
            if (!p->Monitor->nVrefresh)
                xf86DrvMsg(scrnIndex, X_INFO,
                           "Using EDID range info for vertical refresh\n");
            p->Monitor->vrefresh[p->Monitor->nVrefresh].lo =
                det_mon->section.ranges.min_v;
            p->Monitor->vrefresh[p->Monitor->nVrefresh].hi =
                det_mon->section.ranges.max_v;
            p->Monitor->nVrefresh++;
        }
        else {
            xf86DrvMsg(scrnIndex, X_INFO,
                       "Using vrefresh ranges from config file\n");
        }

        clock = det_mon->section.ranges.max_clock * 1000;
        if (p->quirks & DDC_QUIRK_DVI_SINGLE_LINK)
            clock = min(clock, 165000);
        if (!p->have_maxpixclock && clock > p->Monitor->maxPixClock)
            p->Monitor->maxPixClock = clock;

        break;
    default:
        break;
    }
}

/*
 * Fill out MonPtr with xf86MonPtr information.
 */
void
xf86EdidMonitorSet(int scrnIndex, MonPtr Monitor, xf86MonPtr DDC)
{
    DisplayModePtr Modes = NULL, Mode;
    struct det_mon_parameter p;

    if (!Monitor || !DDC)
        return;

    Monitor->DDC = DDC;

    if (Monitor->widthmm <= 0 || Monitor->heightmm <= 0) {
        Monitor->widthmm = 10 * DDC->features.hsize;
        Monitor->heightmm = 10 * DDC->features.vsize;
    }

    Monitor->reducedblanking = xf86MonitorSupportsReducedBlanking(DDC);

    Modes = xf86DDCGetModes(scrnIndex, DDC);

    /* Go through the detailed monitor sections */
    p.Monitor = Monitor;
    p.quirks = xf86DDCDetectQuirks(scrnIndex, Monitor->DDC, FALSE);
    p.have_hsync = (Monitor->nHsync != 0);
    p.have_vrefresh = (Monitor->nVrefresh != 0);
    p.have_maxpixclock = (Monitor->maxPixClock != 0);
    xf86ForEachDetailedBlock(DDC, handle_detailed_monset, &p);

    if (Modes) {
        /* Print Modes */
        xf86DrvMsg(scrnIndex, X_INFO, "Printing DDC gathered Modelines:\n");

        Mode = Modes;
        while (Mode) {
            xf86PrintModeline(scrnIndex, Mode);
            Mode = Mode->next;
        }

        /* Do we still need ranges to be filled in? */
        if (!Monitor->nHsync || !Monitor->nVrefresh)
            DDCGuessRangesFromModes(scrnIndex, Monitor, Modes);

        /* add to MonPtr */
        if (Monitor->Modes) {
            Monitor->Last->next = Modes;
            Modes->prev = Monitor->Last;
        }
        else {
            Monitor->Modes = Modes;
        }

        Monitor->Modes = xf86PruneDuplicateModes(Monitor->Modes);

        /* Update pointer to last mode */
        for (Mode = Monitor->Modes; Mode && Mode->next; Mode = Mode->next) {}
        Monitor->Last = Mode;
    }
}
