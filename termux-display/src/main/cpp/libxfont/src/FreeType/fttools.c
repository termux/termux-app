/*
  Copyright (c) 1997 by Mark Leisher
  Copyright (c) 1998-2002 by Juliusz Chroboczek

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include <X11/fonts/fontmisc.h>
#include <ctype.h>
#include <string.h>

#include <X11/fonts/font.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H
#include FT_TRUETYPE_IDS_H
#include "ft.h"

#ifndef LSBFirst
#define LSBFirst 0
#define MSBFirst 1
#endif

#define LOBYTE(s,byte) ((byte)==LSBFirst?*(char*)(s):*((char*)(s)+1))
#define HIBYTE(s,byte) ((byte)==LSBFirst?*((char*)(s)+1):*(char*)(s))

int FTtoXReturnCode(int rc)
{
    if(rc == 0x40)
        return AllocError;
    /* Anything else stops the font matching mechanism */
    else return BadFontName;

}

/* Convert slen bytes from UCS-2 to ISO 8859-1.  Byte specifies the
   endianness of the string, max the maximum number of bytes written into
   to. */
static int
FTu2a(int slen, FT_Byte *from, char *to, int byte, int max)
{
    int i, n;

    n = 0;
    for (i = 0; i < slen; i += 2) {
        if(n >= max - 1)
            break;
        if(HIBYTE(from+i, byte)!=0)
            *to++='?';
        else
            *to++ = LOBYTE(from+i,byte);
        n++;
    }
    *to = 0;
    return n;
}

static int
FTGetName(FT_Face face, int nid, int pid, int eid, FT_SfntName *name_return)
{
    FT_SfntName name;
    int n, i;

    n = FT_Get_Sfnt_Name_Count(face);
    if(n <= 0)
        return 0;

    for(i = 0; i < n; i++) {
        if(FT_Get_Sfnt_Name(face, i, &name))
            continue;
        if(name.name_id == nid &&
           name.platform_id == pid &&
           (eid < 0 || name.encoding_id == eid)) {
            switch(name.platform_id) {
            case TT_PLATFORM_APPLE_UNICODE:
            case TT_PLATFORM_MACINTOSH:
                if(name.language_id != TT_MAC_LANGID_ENGLISH)
                    continue;
                break;
            case TT_PLATFORM_MICROSOFT:
                if(name.language_id != TT_MS_LANGID_ENGLISH_UNITED_STATES &&
                   name.language_id != TT_MS_LANGID_ENGLISH_UNITED_KINGDOM)
                    continue;
                break;
            default:
                break;
            }
            *name_return = name;
            return 1;
        }
    }
    return 0;
}

int
FTGetEnglishName(FT_Face face, int nid, char *name_return, int name_len)
{
    FT_SfntName name;
    int len;

    if(FTGetName(face, nid,
                 TT_PLATFORM_MICROSOFT, TT_MS_ID_UNICODE_CS, &name) ||
       FTGetName(face, nid,
                 TT_PLATFORM_APPLE_UNICODE, -1, &name))
        return FTu2a(name.string_len, name.string, name_return,
                     MSBFirst, name_len);

    /* Pretend that Apple Roman is ISO 8859-1. */
    if(FTGetName(face, nid, TT_PLATFORM_MACINTOSH, TT_MAC_ID_ROMAN, &name)) {
        len = name.string_len;
        if(len > name_len  - 1)
            len = name_len - 1;
        memcpy(name_return, name.string, len);
        name_return[len] = '\0'; /* ensure nul terminaison */
        return len;
    }

    /* Must be some font that can only be named in Polish or something. */
    return -1;
}
