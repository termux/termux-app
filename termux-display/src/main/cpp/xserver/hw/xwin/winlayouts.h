/*
 * Copyright (c) 2005 Alexander Gottwald
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
/* Definitions for various keyboard layouts from windows and their
 * XKB settings.
 */

typedef struct {
    unsigned int winlayout;
    int winkbtype;
    const char *xkbmodel;
    const char *xkblayout;
    const char *xkbvariant;
    const char *xkboptions;
    const char *layoutname;
} WinKBLayoutRec, *WinKBLayoutPtr;

/*
   This table is sorted by low byte of winlayout, then by next byte, etc.
*/

WinKBLayoutRec winKBLayouts[] = {
    {0x00000404, -1, "pc105", "cn", NULL, NULL, "Chinese (Traditional)"},
    {0x00000804, -1, "pc105", "cn", NULL, NULL, "Chinese (Simplified)"},
    {0x00000405, -1, "pc105", "cz", NULL, NULL, "Czech"},
    {0x00010405, -1, "pc105", "cz_qwerty", NULL, NULL, "Czech (QWERTY)"},
    {0x00000406, -1, "pc105", "dk", NULL, NULL, "Danish"},
    {0x00000407, -1, "pc105", "de", NULL, NULL, "German (Germany)"},
    {0x00010407, -1, "pc105", "de", NULL, NULL, "German (Germany,IBM)"},
    {0x00000807, -1, "pc105", "ch", "de", NULL, "German (Switzerland)"},
    {0x00000409, -1, "pc105", "us", NULL, NULL, "English (USA)"},
    {0x00010409, -1, "pc105", "dvorak", NULL, NULL, "English (USA,Dvorak)"},
    {0x00020409, -1, "pc105", "us_intl", NULL, NULL,
     "English (USA,International)"},
    {0x00000809, -1, "pc105", "gb", NULL, NULL, "English (United Kingdom)"},
    {0x00001009, -1, "pc105", "ca", "fr", NULL, "French (Canada)"},
    {0x00011009, -1, "pc105", "ca", "multix", NULL,
     "Canadian Multilingual Standard"},
    {0x00001809, -1, "pc105", "ie", NULL, NULL, "Irish"},
    {0x0000040a, -1, "pc105", "es", NULL, NULL,
     "Spanish (Spain,Traditional Sort)"},
    {0x0000080a, -1, "pc105", "latam", NULL, NULL, "Latin American"},
    {0x0000040b, -1, "pc105", "fi", NULL, NULL, "Finnish"},
    {0x0000040c, -1, "pc105", "fr", NULL, NULL, "French (Standard)"},
    {0x0000080c, -1, "pc105", "be", NULL, NULL, "French (Belgian)"},
    {0x0001080c, -1, "pc105", "be", NULL, NULL, "Belgian (Comma)"},
    {0x00000c0c, -1, "pc105", "ca", "fr-legacy", NULL,
     "French (Canada, Legacy)"},
    {0x0000100c, -1, "pc105", "ch", "fr", NULL, "French (Switzerland)"},
    {0x0000040d, -1, "pc105", "il", NULL, NULL, "Hebrew"},
    {0x0000040e, -1, "pc105", "hu", NULL, NULL, "Hungarian"},
    {0x0000040f, -1, "pc105", "is", NULL, NULL, "Icelandic"},
    {0x00000410, -1, "pc105", "it", NULL, NULL, "Italian"},
    {0x00010410, -1, "pc105", "it", NULL, NULL, "Italian (142)"},
    {0x00000411, 7, "jp106", "jp", NULL, NULL, "Japanese"},
    {0x00000412, -1, "kr106", "kr", NULL, NULL, "Korean"},
    {0x00000413, -1, "pc105", "nl", NULL, NULL, "Dutch"},
    {0x00000813, -1, "pc105", "be", NULL, NULL, "Dutch (Belgian)"},
    {0x00000414, -1, "pc105", "no", NULL, NULL, "Norwegian"},
    {0x00000415, -1, "pc105", "pl", NULL, NULL, "Polish (Programmers)"},
    {0x00000416, -1, "pc105", "br", NULL, NULL, "Portuguese (Brazil,ABNT)"},
    {0x00010416, -1, "abnt2", "br", NULL, NULL, "Portuguese (Brazil,ABNT2)"},
    {0x00000816, -1, "pc105", "pt", NULL, NULL, "Portuguese (Portugal)"},
    {0x00000419, -1, "pc105", "ru", NULL, NULL, "Russian"},
    {0x0000041a, -1, "pc105", "hr", NULL, NULL, "Croatian"},
    {0x0000041d, -1, "pc105", "se", NULL, NULL, "Swedish (Sweden)"},
    {0x0000041f, -1, "pc105", "tr", NULL, NULL, "Turkish (Q)"},
    {0x0001041f, -1, "pc105", "tr", "f", NULL, "Turkish (F)"},
    {0x00000424, -1, "pc105", "si", NULL, NULL, "Slovenian"},
    {0x00000425, -1, "pc105", "ee", NULL, NULL, "Estonian"},
    {0x00000452, -1, "pc105", "gb", "intl", NULL, "United Kingdom (Extended)"},
    {-1, -1, NULL, NULL, NULL, NULL, NULL}
};

/*
  See http://technet.microsoft.com/en-us/library/cc766503%28WS.10%29.aspx
  for a listing of input locale (keyboard layout) codes
*/
