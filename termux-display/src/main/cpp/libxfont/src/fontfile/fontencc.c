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

/* Binary compatibility code. */

/* This file includes code to make modules compiled for earlier
   versions of the fontenc interfaces link with this one.  It does
   *not* provide source compatibility, as many of the data structures
   now have different names. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include <X11/fonts/fontenc.h>
#include <X11/fonts/fontencc.h>

char *
font_encoding_from_xlfd(const char * name, int length)
{
    return FontEncFromXLFD(name, length);
}

FontEncPtr
font_encoding_find(const char *encoding_name, const char *filename)
{
    return FontEncFind(encoding_name, filename);
}

unsigned
font_encoding_recode(unsigned code,
                     FontEncPtr encoding, FontMapPtr mapping)
{
    if(encoding != mapping->encoding) {
        ErrorF("Inconsistent mapping/encoding\n");
        return 0;
    }
    return FontEncRecode(code, mapping);
}

char *
font_encoding_name(unsigned code,
                     FontEncPtr encoding, FontMapPtr mapping)
{
    if(encoding != mapping->encoding) {
        ErrorF("Inconsistent mapping/encoding\n");
        return 0;
    }
    return FontEncName(code, mapping);
}

char **
identifyEncodingFile(const char *filename)
{
    return FontEncIdentify(filename);
}
