/*
Copyright (c) 1998-2001 by Juliusz Chroboczek

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/* Backend-independent encoding code */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <strings.h>
#include <stdlib.h>

#define FALSE 0
#define TRUE 1
#define MAXFONTNAMELEN 1024
#define MAXFONTFILENAMELEN 1024

#include <X11/fonts/fontenc.h>
#include "fontencI.h"
#include "reallocarray.h"

/* Functions local to this file */

static FontEncPtr FontEncLoad(const char *, const char *);

/* Early versions of this code only knew about hardwired encodings,
   hence the following data.  Now that the code knows how to load an
   encoding from a file, most of these tables could go away. */

/* At any rate, no new hardcoded encodings will be added. */

static FontMapRec iso10646[] = {
    {FONT_ENCODING_UNICODE, 0, 0, NULL, NULL, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

/* Notice that the Apple encodings do not have all the characters in
   the corresponding ISO 8859, and therefore the table has some holes.
   There's not much more we can do with fonts without a Unicode cmap
   unless we are willing to combine cmaps (which we are not). */

static const unsigned short
 iso8859_1_apple_roman[] = {
    0xCA, 0xC1, 0xA2, 0xA3, 0xDB, 0xB4, 0x00, 0xA4,
    0xAC, 0xA9, 0xBB, 0xC7, 0xC2, 0x00, 0xA8, 0xF8,
    0xA1, 0xB1, 0x00, 0x00, 0xAB, 0xB5, 0xA6, 0xE1,
    0xFC, 0x00, 0xBC, 0xC8, 0x00, 0x00, 0x00, 0xC0,
    0xCB, 0xE7, 0xE5, 0xCC, 0x80, 0x81, 0xAE, 0x82,
    0xE9, 0x83, 0xE6, 0xE8, 0xED, 0xEA, 0xEB, 0xEC,
    0x00, 0x84, 0xF1, 0xEE, 0xEF, 0xCD, 0x85, 0x00,
    0xAF, 0xF4, 0xF2, 0xF3, 0x86, 0x00, 0x00, 0xA7,
    0x88, 0x87, 0x89, 0x8B, 0x8A, 0x8C, 0xBE, 0x8D,
    0x8F, 0x8E, 0x90, 0x91, 0x93, 0x92, 0x94, 0x95,
    0x00, 0x96, 0x98, 0x97, 0x99, 0x9B, 0x9A, 0xD6,
    0xBF, 0x9D, 0x9C, 0x9E, 0x9F, 0x00, 0x00, 0xD8
};

/* Cannot use simple_recode because need to eliminate 0x80<=code<0xA0 */
static unsigned
iso8859_1_to_apple_roman(unsigned isocode, void *client_data)
{
    if (isocode <= 0x80)
        return isocode;
    else if (isocode >= 0xA0)
        return iso8859_1_apple_roman[isocode - 0xA0];
    else
        return 0;
}

static FontMapRec iso8859_1[] = {
    {FONT_ENCODING_TRUETYPE, 2, 2, NULL, NULL, NULL, NULL, NULL},       /* ISO 8859-1 */
    {FONT_ENCODING_UNICODE, 0, 0, NULL, NULL, NULL, NULL, NULL},        /* ISO 8859-1 coincides with Unicode */
    {FONT_ENCODING_TRUETYPE, 1, 0, iso8859_1_to_apple_roman, NULL, NULL, NULL,
     NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static const unsigned short iso8859_2_tophalf[] = {
    0x00A0, 0x0104, 0x02D8, 0x0141, 0x00A4, 0x013D, 0x015A, 0x00A7,
    0x00A8, 0x0160, 0x015E, 0x0164, 0x0179, 0x00AD, 0x017D, 0x017B,
    0x00B0, 0x0105, 0x02DB, 0x0142, 0x00B4, 0x013E, 0x015B, 0x02C7,
    0x00B8, 0x0161, 0x015F, 0x0165, 0x017A, 0x02DD, 0x017E, 0x017C,
    0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
    0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
    0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
    0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
    0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
    0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
    0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
    0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9
};

static FontEncSimpleMapRec iso8859_2_to_unicode_map =
    { 0x60, 0, 0xA0, iso8859_2_tophalf };

static const unsigned short iso8859_2_apple_centeuro[] = {
    0xCA, 0x84, 0x00, 0xFC, 0x00, 0xBB, 0xE5, 0xA4,
    0xAC, 0xE1, 0x00, 0xE8, 0x8F, 0x00, 0xEB, 0xFB,
    0xA1, 0x88, 0x00, 0xB8, 0x00, 0xBC, 0xE6, 0xFF,
    0x00, 0xE4, 0x00, 0xE9, 0x90, 0x00, 0xEC, 0xFD,
    0xD9, 0xE7, 0x00, 0x00, 0x80, 0xBD, 0x8C, 0x00,
    0x89, 0x83, 0xA2, 0x00, 0x9D, 0xEA, 0x00, 0x91,
    0x00, 0xC1, 0xC5, 0xEE, 0xEF, 0xCC, 0x85, 0x00,
    0xDB, 0xF1, 0xF2, 0xF4, 0x86, 0xF8, 0x00, 0xA7,
    0xDA, 0x87, 0x00, 0x00, 0x8A, 0xBE, 0x8D, 0x00,
    0x8B, 0x8E, 0xAB, 0x00, 0x9E, 0x92, 0x00, 0x93,
    0x00, 0xC4, 0xCB, 0x97, 0x99, 0xCE, 0x9A, 0xD6,
    0xDE, 0xF3, 0x9C, 0xF5, 0x9F, 0xF9, 0x00, 0x00
};

static unsigned
iso8859_2_to_apple_centeuro(unsigned isocode, void *client_data)
{
    if (isocode <= 0x80)
        return isocode;
    else if (isocode >= 0xA0)
        return iso8859_2_apple_centeuro[isocode - 0xA0];
    else
        return 0;
}

static FontMapRec iso8859_2[] = {
    {FONT_ENCODING_UNICODE, 0, 0,
     FontEncSimpleRecode, NULL, &iso8859_2_to_unicode_map, NULL, NULL},
    {FONT_ENCODING_TRUETYPE, 1, 29, iso8859_2_to_apple_centeuro, NULL, NULL,
     NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static const unsigned short iso8859_3_tophalf[] = {
    0x00A0, 0x0126, 0x02D8, 0x00A3, 0x00A4, 0x0000, 0x0124, 0x00A7,
    0x00A8, 0x0130, 0x015E, 0x011E, 0x0134, 0x00AD, 0x0000, 0x017B,
    0x00B0, 0x0127, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x0125, 0x00B7,
    0x00B8, 0x0131, 0x015F, 0x011F, 0x0135, 0x00BD, 0x0000, 0x017C,
    0x00C0, 0x00C1, 0x00C2, 0x0000, 0x00C4, 0x010A, 0x0108, 0x00C7,
    0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
    0x0000, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x0120, 0x00D6, 0x00D7,
    0x011C, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x016C, 0x015C, 0x00DF,
    0x00E0, 0x00E1, 0x00E2, 0x0000, 0x00E4, 0x010B, 0x0109, 0x00E7,
    0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
    0x0000, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x0121, 0x00F6, 0x00F7,
    0x011D, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x016D, 0x015D, 0x02D9
};

static FontEncSimpleMapRec iso8859_3_to_unicode_map =
    { 0x60, 0, 0xA0, iso8859_3_tophalf };

static FontMapRec iso8859_3[] = {
    {FONT_ENCODING_UNICODE, 0, 0,
     FontEncSimpleRecode, NULL, &iso8859_3_to_unicode_map, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static const unsigned short iso8859_4_tophalf[] = {
    0x00A0, 0x0104, 0x0138, 0x0156, 0x00A4, 0x0128, 0x013B, 0x00A7,
    0x00A8, 0x0160, 0x0112, 0x0122, 0x0166, 0x00AD, 0x017D, 0x00AF,
    0x00B0, 0x0105, 0x02DB, 0x0157, 0x00B4, 0x0129, 0x013C, 0x02C7,
    0x00B8, 0x0161, 0x0113, 0x0123, 0x0167, 0x014A, 0x017E, 0x014B,
    0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E,
    0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x012A,
    0x0110, 0x0145, 0x014C, 0x0136, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
    0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x0168, 0x016A, 0x00DF,
    0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F,
    0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x012B,
    0x0111, 0x0146, 0x014D, 0x0137, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
    0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x0169, 0x016B, 0x02D9,
};

static FontEncSimpleMapRec iso8859_4_to_unicode_map =
    { 0x60, 0, 0xA0, iso8859_4_tophalf };

static FontMapRec iso8859_4[] = {
    {FONT_ENCODING_UNICODE, 0, 0, FontEncSimpleRecode, NULL,
     &iso8859_4_to_unicode_map, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static const unsigned short iso8859_5_tophalf[] = {
    0x00A0, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407,
    0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x00AD, 0x040E, 0x040F,
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
    0x2116, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457,
    0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x00A7, 0x045E, 0x045F
};

static FontEncSimpleMapRec iso8859_5_to_unicode_map =
    { 0x60, 0, 0xA0, iso8859_5_tophalf };

static const unsigned short iso8859_5_apple_cyrillic[] = {
    0xCA, 0xDD, 0xAB, 0xAE, 0xB8, 0xC1, 0xA7, 0xBA,
    0xB7, 0xBC, 0xBE, 0xCB, 0xCD, 0x00, 0xD8, 0xDA,
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
    0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
    0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
    0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
    0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
    0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
    0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xDF,
    0xDC, 0xDE, 0xAC, 0xAF, 0xB9, 0xCF, 0xB4, 0xBB,
    0xC0, 0xBD, 0xBF, 0xCC, 0xCE, 0xA4, 0xD9, 0xDB
};

static unsigned
iso8859_5_to_apple_cyrillic(unsigned isocode, void *client_data)
{
    if (isocode <= 0x80)
        return isocode;
    else if (isocode >= 0xA0)
        return iso8859_5_apple_cyrillic[isocode - 0x80];
    else
        return 0;
}

static FontMapRec iso8859_5[] = {
    {FONT_ENCODING_UNICODE, 0, 0, FontEncSimpleRecode, NULL,
     &iso8859_5_to_unicode_map, NULL, NULL},
    {FONT_ENCODING_TRUETYPE, 1, 7, iso8859_5_to_apple_cyrillic, NULL, NULL,
     NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

/* ISO 8859-6 seems useless for serving fonts (not enough presentation
 * forms).  What do Arabic-speakers use? */

static unsigned
iso8859_6_to_unicode(unsigned isocode, void *client_data)
{
    if (isocode <= 0xA0 || isocode == 0xA4 || isocode == 0xAD)
        return isocode;
    else if (isocode == 0xAC || isocode == 0xBB ||
             isocode == 0xBF ||
             (isocode >= 0xC1 && isocode <= 0xDA) ||
             (isocode >= 0xE0 && isocode <= 0xEF) ||
             (isocode >= 0xF0 && isocode <= 0xF2))
        return isocode - 0xA0 + 0x0600;
    else
        return 0;
}

static FontMapRec iso8859_6[] = {
    {FONT_ENCODING_UNICODE, 0, 0, iso8859_6_to_unicode, NULL, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static unsigned
iso8859_7_to_unicode(unsigned isocode, void *client_data)
{
    if (isocode <= 0xA0 ||
        (isocode >= 0xA3 && isocode <= 0xAD) ||
        (isocode >= 0xB0 && isocode <= 0xB3) ||
        isocode == 0xB7 || isocode == 0xBB || isocode == 0xBD)
        return isocode;
    else if (isocode == 0xA1)
        return 0x2018;
    else if (isocode == 0xA2)
        return 0x2019;
    else if (isocode == 0xAF)
        return 0x2015;
    else if (isocode == 0xD2)   /* unassigned */
        return 0;
    else if (isocode >= 0xB4 && isocode <= 0xFE)
        return isocode - 0xA0 + 0x0370;
    else
        return 0;
}

static FontMapRec iso8859_7[] = {
    {FONT_ENCODING_UNICODE, 0, 0, iso8859_7_to_unicode, NULL, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static unsigned
iso8859_8_to_unicode(unsigned isocode, void *client_data)
{
    if (isocode == 0xA1)
        return 0;
    else if (isocode < 0xBF)
        return isocode;
    else if (isocode == 0xDF)
        return 0x2017;
    else if (isocode >= 0xE0 && isocode <= 0xFA)
        return isocode + 0x04F0;
    else
        return 0;
}

static FontMapRec iso8859_8[] = {
    {FONT_ENCODING_UNICODE, 0, 0, iso8859_8_to_unicode, NULL, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static unsigned
iso8859_9_to_unicode(unsigned isocode, void *client_data)
{
    switch (isocode) {
    case 0xD0:
        return 0x011E;
    case 0xDD:
        return 0x0130;
    case 0xDE:
        return 0x015E;
    case 0xF0:
        return 0x011F;
    case 0xFD:
        return 0x0131;
    case 0xFE:
        return 0x015F;
    default:
        return isocode;
    }
}

static FontMapRec iso8859_9[] = {
    {FONT_ENCODING_UNICODE, 0, 0, iso8859_9_to_unicode, NULL, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static const unsigned short iso8859_10_tophalf[] = {
    0x00A0, 0x0104, 0x0112, 0x0122, 0x012A, 0x0128, 0x0136, 0x00A7,
    0x013B, 0x0110, 0x0160, 0x0166, 0x017D, 0x00AD, 0x016A, 0x014A,
    0x00B0, 0x0105, 0x0113, 0x0123, 0x012B, 0x0129, 0x0137, 0x00B7,
    0x013C, 0x0111, 0x0161, 0x0167, 0x017E, 0x2014, 0x016B, 0x014B,
    0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E,
    0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x00CF,
    0x00D0, 0x0145, 0x014C, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0168,
    0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
    0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F,
    0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x00EF,
    0x00F0, 0x0146, 0x014D, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0169,
    0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x0138
};

static FontEncSimpleMapRec iso8859_10_to_unicode_map =
    { 0x60, 0, 0xA0, iso8859_10_tophalf };

static FontMapRec iso8859_10[] = {
    {FONT_ENCODING_UNICODE, 0, 0, FontEncSimpleRecode, NULL,
     &iso8859_10_to_unicode_map, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static unsigned
iso8859_15_to_unicode(unsigned isocode, void *client_data)
{
    switch (isocode) {
    case 0xA4:
        return 0x20AC;
    case 0xA6:
        return 0x0160;
    case 0xA8:
        return 0x0161;
    case 0xB4:
        return 0x017D;
    case 0xB8:
        return 0x017E;
    case 0xBC:
        return 0x0152;
    case 0xBD:
        return 0x0153;
    case 0xBE:
        return 0x0178;
    default:
        return isocode;
    }
}

static FontMapRec iso8859_15[] = {
    {FONT_ENCODING_UNICODE, 0, 0, iso8859_15_to_unicode, NULL, NULL, NULL,
     NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static const unsigned short koi8_r_tophalf[] = {
    0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524,
    0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590,
    0x2591, 0x2592, 0x2593, 0x2320, 0x25A0, 0x2022, 0x221A, 0x2248,
    0x2264, 0x2265, 0x00A0, 0x2321, 0x00B0, 0x00B2, 0x00B7, 0x00F7,
    0x2550, 0x2551, 0x2552, 0x0451, 0x2553, 0x2554, 0x2555, 0x2556,
    0x2557, 0x2558, 0x2559, 0x255A, 0x255B, 0x255C, 0x255D, 0x255E,
    0x255F, 0x2560, 0x2561, 0x0401, 0x2562, 0x2563, 0x2564, 0x2565,
    0x2566, 0x2567, 0x2568, 0x2569, 0x256A, 0x256B, 0x256C, 0x00A9,
    0x044E, 0x0430, 0x0431, 0x0446, 0x0434, 0x0435, 0x0444, 0x0433,
    0x0445, 0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E,
    0x043F, 0x044F, 0x0440, 0x0441, 0x0442, 0x0443, 0x0436, 0x0432,
    0x044C, 0x044B, 0x0437, 0x0448, 0x044D, 0x0449, 0x0447, 0x044A,
    0x042E, 0x0410, 0x0411, 0x0426, 0x0414, 0x0415, 0x0424, 0x0413,
    0x0425, 0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E,
    0x041F, 0x042F, 0x0420, 0x0421, 0x0422, 0x0423, 0x0416, 0x0412,
    0x042C, 0x042B, 0x0417, 0x0428, 0x042D, 0x0429, 0x0427, 0x042A
};

static FontEncSimpleMapRec koi8_r_to_unicode_map =
    { 0x80, 0, 0x80, koi8_r_tophalf };

static FontMapRec koi8_r[] = {
    {FONT_ENCODING_UNICODE, 0, 0, FontEncSimpleRecode, NULL,
     &koi8_r_to_unicode_map, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static unsigned
koi8_ru_to_unicode(unsigned koicode, void *client_data)
{
    switch (koicode) {
    case 0x93:
        return 0x201C;
    case 0x96:
        return 0x201D;
    case 0x97:
        return 0x2014;
    case 0x98:
        return 0x2116;
    case 0x99:
        return 0x2122;
    case 0x9B:
        return 0x00BB;
    case 0x9C:
        return 0x00AE;
    case 0x9D:
        return 0x00AB;
    case 0x9F:
        return 0x00A4;
    case 0xA4:
        return 0x0454;
    case 0xA6:
        return 0x0456;
    case 0xA7:
        return 0x0457;
    case 0xAD:
        return 0x0491;
    case 0xAE:
        return 0x045E;
    case 0xB4:
        return 0x0404;
    case 0xB6:
        return 0x0406;
    case 0xB7:
        return 0x0407;
    case 0xBD:
        return 0x0490;
    case 0xBE:
        return 0x040E;
    default:
        return FontEncSimpleRecode(koicode, &koi8_r_to_unicode_map);
    }
}

static FontMapRec koi8_ru[] = {
    {FONT_ENCODING_UNICODE, 0, 0, koi8_ru_to_unicode, NULL, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

/* koi8-e, ISO-IR-111 or ECMA-Cyrillic */

static const unsigned short koi8_e_A0_BF[] = {
    0x00A0, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457,
    0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x00AD, 0x045E, 0x045F,
    0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407,
    0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x00A4, 0x040E, 0x040F
};

static unsigned
koi8_e_to_unicode(unsigned koicode, void *client_data)
{
    if (koicode < 0xA0)
        return koicode;
    else if (koicode < 0xC0)
        return koi8_e_A0_BF[koicode - 0xA0];
    else
        return FontEncSimpleRecode(koicode, &koi8_r_to_unicode_map);
}

static FontMapRec koi8_e[] = {
    {FONT_ENCODING_UNICODE, 0, 0, koi8_e_to_unicode, NULL, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

/* Koi8 unified */

static const unsigned short koi8_uni_80_BF[] = {
    0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518, 0x251C, 0x2524,
    0x252C, 0x2534, 0x253C, 0x2580, 0x2584, 0x2588, 0x258C, 0x2590,
    0x2591, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
    0x00A9, 0x2122, 0x00A0, 0x00BB, 0x00AE, 0x00AB, 0x00B7, 0x00A4,
    0x00A0, 0x0452, 0x0453, 0x0451, 0x0454, 0x0455, 0x0456, 0x0457,
    0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x0491, 0x045E, 0x045F,
    0x2116, 0x0402, 0x0403, 0x0401, 0x0404, 0x0405, 0x0406, 0x0407,
    0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x0490, 0x040E, 0x040F
};

static unsigned
koi8_uni_to_unicode(unsigned koicode, void *client_data)
{
    if (koicode < 0x80)
        return koicode;
    else if (koicode < 0xC0)
        return koi8_uni_80_BF[koicode - 0x80];
    else
        return FontEncSimpleRecode(koicode, &koi8_r_to_unicode_map);
}

static FontMapRec koi8_uni[] = {
    {FONT_ENCODING_UNICODE, 0, 0, koi8_uni_to_unicode, NULL, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

/* Ukrainian variant of Koi8-R; see RFC 2319 */

static unsigned
koi8_u_to_unicode(unsigned koicode, void *client_data)
{
    switch (koicode) {
    case 0xA4:
        return 0x0454;
    case 0xA6:
        return 0x0456;
    case 0xA7:
        return 0x0457;
    case 0xAD:
        return 0x0491;
    case 0xB4:
        return 0x0404;
    case 0xB6:
        return 0x0406;
    case 0xB7:
        return 0x0407;
    case 0xBD:
        return 0x0490;
    default:
        return FontEncSimpleRecode(koicode, &koi8_r_to_unicode_map);
    }
}

static FontMapRec koi8_u[] = {
    {FONT_ENCODING_UNICODE, 0, 0, koi8_u_to_unicode, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

/* Microsoft Symbol, which is only meaningful for TrueType fonts, is
   treated specially in ftenc.c, where we add usFirstCharIndex-0x20 to
   the glyph index before applying the cmap.  Lovely design. */

static FontMapRec microsoft_symbol[] = {
    {FONT_ENCODING_TRUETYPE, 3, 0, NULL, NULL, NULL, NULL, NULL},
    /* You never know */
    {FONT_ENCODING_TRUETYPE, 3, 1, NULL, NULL, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

static FontMapRec apple_roman[] = {
    {FONT_ENCODING_TRUETYPE, 1, 0, NULL, NULL, NULL, NULL, NULL},
    {0, 0, 0, NULL, NULL, NULL, NULL, NULL}
};

/* The data for recodings */

/* For compatibility with X11R6.4.  Losers. */
static char *iso8859_15_aliases[2] = { "fcd8859-15", NULL };

static FontEncRec initial_encodings[] = {
    {"iso10646-1", NULL, 256 * 256, 0, iso10646, NULL, 0, 0},   /* Unicode */
    {"iso8859-1", NULL, 256, 0, iso8859_1, NULL, 0, 0}, /* Latin 1 (West European) */
    {"iso8859-2", NULL, 256, 0, iso8859_2, NULL, 0, 0}, /* Latin 2 (East European) */
    {"iso8859-3", NULL, 256, 0, iso8859_3, NULL, 0, 0}, /* Latin 3 (South European) */
    {"iso8859-4", NULL, 256, 0, iso8859_4, NULL, 0, 0}, /* Latin 4 (North European) */
    {"iso8859-5", NULL, 256, 0, iso8859_5, NULL, 0, 0}, /* Cyrillic */
    {"iso8859-6", NULL, 256, 0, iso8859_6, NULL, 0, 0}, /* Arabic */
    {"iso8859-7", NULL, 256, 0, iso8859_7, NULL, 0, 0}, /* Greek */
    {"iso8859-8", NULL, 256, 0, iso8859_8, NULL, 0, 0}, /* Hebrew */
    {"iso8859-9", NULL, 256, 0, iso8859_9, NULL, 0, 0}, /* Latin 5 (Turkish) */
    {"iso8859-10", NULL, 256, 0, iso8859_10, NULL, 0, 0},       /* Latin 6 (Nordic) */
    {"iso8859-15", iso8859_15_aliases, 256, 0, iso8859_15, NULL, 0, 0}, /* Latin 9 */
    {"koi8-r", NULL, 256, 0, koi8_r, NULL, 0, 0},       /* Russian */
    {"koi8-ru", NULL, 256, 0, koi8_ru, NULL, 0, 0},     /* Ukrainian */
    {"koi8-uni", NULL, 256, 0, koi8_uni, NULL, 0, 0},   /* Russian/Ukrainian/Bielorussian */
    {"koi8-e", NULL, 256, 0, koi8_e, NULL, 0, 0},       /* ``European'' */
    {"koi8-u", NULL, 256, 0, koi8_u, NULL, 0, 0},       /* Ukrainian too */
    {"microsoft-symbol", NULL, 256, 0, microsoft_symbol, NULL, 0, 0},
    {"apple-roman", NULL, 256, 0, apple_roman, NULL, 0, 0},
    {NULL, NULL, 0, 0, NULL, NULL, 0, 0}
};

static FontEncPtr font_encodings = NULL;

static void
define_initial_encoding_info(void)
{
    FontEncPtr encoding;
    FontMapPtr mapping;

    font_encodings = initial_encodings;
    for (encoding = font_encodings; ; encoding++) {
        encoding->next = encoding + 1;
        for (mapping = encoding->mappings; ; mapping++) {
            mapping->next = mapping + 1;
            mapping->encoding = encoding;
            if (mapping->next->type == 0) {
                mapping->next = NULL;
                break;
            }
        }
        if (!encoding->next->name) {
            encoding->next = NULL;
            break;
        }
    }
}

char *
FontEncFromXLFD(const char *name, int length)
{
    const char *p;
    char *q;
    static char charset[MAXFONTNAMELEN];
    int len;

    if (length > MAXFONTNAMELEN - 1)
        return NULL;

    if (name == NULL)
        p = NULL;
    else {
        p = name + length - 1;
        while (p > name && *p != '-')
            p--;
        p--;
        while (p >= name && *p != '-')
            p--;
        if (p <= name)
            p = NULL;
    }

    /* now p either is null or points at the '-' before the charset registry */

    if (p == NULL)
        return NULL;

    len = length - (p - name) - 1;
    memcpy(charset, p + 1, len);
    charset[len] = 0;

    /* check for a subset specification */
    if ((q = strchr(charset, (int) '[')))
        *q = 0;

    return charset;
}

unsigned
FontEncRecode(unsigned code, FontMapPtr mapping)
{
    FontEncPtr encoding = mapping->encoding;

    if (encoding && mapping->recode) {
        if (encoding->row_size == 0) {
            /* linear encoding */
            if (code < encoding->first || code >= encoding->size)
                return 0;
        }
        else {
            /* matrix encoding */
            int row = code / 0x100, col = code & 0xFF;

            if (row < encoding->first || row >= encoding->size ||
                col < encoding->first_col || col >= encoding->row_size)
                return 0;
        }
        return (*mapping->recode) (code, mapping->client_data);
    }
    else
        return code;
}

char *
FontEncName(unsigned code, FontMapPtr mapping)
{
    FontEncPtr encoding = mapping->encoding;

    if (encoding && mapping->name) {
        if ((encoding->row_size == 0 && code >= encoding->size) ||
            (encoding->row_size != 0 &&
             (code / 0x100 >= encoding->size ||
              (code & 0xFF) >= encoding->row_size)))
            return NULL;
        return (*mapping->name) (code, mapping->client_data);
    }
    else
        return NULL;
}

FontEncPtr
FontEncFind(const char *encoding_name, const char *filename)
{
    FontEncPtr encoding;
    char **alias;

    if (font_encodings == NULL)
        define_initial_encoding_info();

    for (encoding = font_encodings; encoding; encoding = encoding->next) {
        if (!strcasecmp(encoding->name, encoding_name))
            return encoding;
        if (encoding->aliases)
            for (alias = encoding->aliases; *alias; alias++)
                if (!strcasecmp(*alias, encoding_name))
                    return encoding;
    }

    /* Unknown charset, try to load a definition file */
    return FontEncLoad(encoding_name, filename);
}

FontMapPtr
FontMapFind(FontEncPtr encoding, int type, int pid, int eid)
{
    FontMapPtr mapping;

    if (encoding == NULL)
        return NULL;

    for (mapping = encoding->mappings; mapping; mapping = mapping->next) {
        if (mapping->type != type)
            continue;
        if (pid > 0 && mapping->pid != pid)
            continue;
        if (eid > 0 && mapping->eid != eid)
            continue;
        return mapping;
    }
    return NULL;
}

FontMapPtr
FontEncMapFind(const char *encoding_name, int type, int pid, int eid,
               const char *filename)
{
    FontEncPtr encoding;
    FontMapPtr mapping;

    encoding = FontEncFind(encoding_name, filename);
    if (encoding == NULL)
        return NULL;
    mapping = FontMapFind(encoding, type, pid, eid);
    return mapping;
}

static FontEncPtr
FontEncLoad(const char *encoding_name, const char *filename)
{
    FontEncPtr encoding;

    encoding = FontEncReallyLoad(encoding_name, filename);
    if (encoding == NULL) {
        return NULL;
    }
    else {
        char **alias;
        int found = 0;

        /* Check whether the name is already known for this encoding */
        if (strcasecmp(encoding->name, encoding_name) == 0) {
            found = 1;
        }
        else {
            if (encoding->aliases) {
                for (alias = encoding->aliases; *alias; alias++)
                    if (!strcasecmp(*alias, encoding_name)) {
                        found = 1;
                        break;
                    }
            }
        }

        if (!found) {
            /* Add a new alias.  This works because we know that this
               particular encoding has been allocated dynamically */
            char **new_aliases;
            char *new_name;
            int numaliases = 0;

            new_name = strdup(encoding_name);
            if (new_name == NULL)
                return NULL;
            if (encoding->aliases) {
                for (alias = encoding->aliases; *alias; alias++)
                    numaliases++;
            }
            new_aliases = Xmallocarray(numaliases + 2, sizeof(char *));
            if (new_aliases == NULL) {
                free(new_name);
                return NULL;
            }
            if (encoding->aliases) {
                memcpy(new_aliases, encoding->aliases,
                       numaliases * sizeof(char *));
                free(encoding->aliases);
            }
            new_aliases[numaliases] = new_name;
            new_aliases[numaliases + 1] = NULL;
            encoding->aliases = new_aliases;
        }

        /* register the new encoding */
        encoding->next = font_encodings;
        font_encodings = encoding;

        return encoding;
    }
}

unsigned
FontEncSimpleRecode(unsigned code, void *client_data)
{
    FontEncSimpleMapPtr map;
    unsigned index;

    map = client_data;

    if (code > 0xFFFF || (map->row_size && (code & 0xFF) >= map->row_size))
        return 0;

    if (map->row_size)
        index = (code & 0xFF) + (code >> 8) * map->row_size;
    else
        index = code;

    if (map->map && index >= map->first && index < map->first + map->len)
        return map->map[index - map->first];
    else
        return code;
}

char *
FontEncSimpleName(unsigned code, void *client_data)
{
    FontEncSimpleNamePtr map;

    map = client_data;
    if (map && code >= map->first && code < map->first + map->len)
        return map->map[code - map->first];
    else
        return NULL;
}

unsigned
FontEncUndefinedRecode(unsigned code, void *client_data)
{
    return code;
}

char *
FontEncUndefinedName(unsigned code, void *client_data)
{
    return NULL;
}

#define FONTENC_SEGMENT_SIZE 256
#define FONTENC_SEGMENTS 256
#define FONTENC_INVERSE_CODES (FONTENC_SEGMENT_SIZE * FONTENC_SEGMENTS)

static unsigned int
reverse_reverse(unsigned i, void *data)
{
    int s, j;
    unsigned **map = (unsigned **) data;

    if (i >= FONTENC_INVERSE_CODES)
        return 0;

    if (map == NULL)
        return 0;

    s = i / FONTENC_SEGMENT_SIZE;
    j = i % FONTENC_SEGMENT_SIZE;

    if (map[s] == NULL)
        return 0;
    else
        return map[s][j];
}

static int
tree_set(unsigned int **map, unsigned int i, unsigned int j)
{
    int s, c;

    if (i >= FONTENC_INVERSE_CODES)
        return FALSE;

    s = i / FONTENC_SEGMENT_SIZE;
    c = i % FONTENC_SEGMENT_SIZE;

    if (map[s] == NULL) {
        map[s] = calloc(FONTENC_SEGMENT_SIZE, sizeof(int));
        if (map[s] == NULL)
            return FALSE;
    }

    map[s][c] = j;
    return TRUE;
}

FontMapReversePtr
FontMapReverse(FontMapPtr mapping)
{
    FontEncPtr encoding = mapping->encoding;
    FontMapReversePtr reverse = NULL;
    unsigned int **map = NULL;
    int i, j, k;

    if (encoding == NULL)
        goto bail;

    map = calloc(FONTENC_SEGMENTS, sizeof(int *));
    if (map == NULL)
        goto bail;

    if (encoding->row_size == 0) {
        for (i = encoding->first; i < encoding->size; i++) {
            k = FontEncRecode(i, mapping);
            if (k != 0)
                if (!tree_set(map, k, i))
                    goto bail;
        }
    }
    else {
        for (i = encoding->first; i < encoding->size; i++) {
            for (j = encoding->first_col; j < encoding->row_size; j++) {
                k = FontEncRecode(i * 256 + j, mapping);
                if (k != 0)
                    if (!tree_set(map, k, i * 256 + j))
                        goto bail;
            }
        }
    }

    reverse = malloc(sizeof(FontMapReverseRec));
    if (!reverse)
        goto bail;

    reverse->reverse = reverse_reverse;
    reverse->data = map;
    return reverse;

 bail:
    free(map);
    free(reverse);
    return NULL;
}

void
FontMapReverseFree(FontMapReversePtr delendum)
{
    unsigned int **map = (unsigned int **) delendum;
    int i;

    if (map == NULL)
        return;

    for (i = 0; i < FONTENC_SEGMENTS; i++)
        free(map[i]);

    free(map);
    return;
}
