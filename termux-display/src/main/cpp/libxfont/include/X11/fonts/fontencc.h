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

/* Binary compatibility entry points. */

/* This file includes code to make modules compiled for earlier
   versions of the fontenc interfaces link with this one.  It does
   *not* provide source compatibility, as many of the data structures
   now have different names. */

extern char *font_encoding_from_xlfd(const char*, int);
extern unsigned font_encoding_recode(unsigned, FontEncPtr, FontMapPtr);
extern FontEncPtr font_encoding_find(const char*, const char*);
extern char *font_encoding_name(unsigned, FontEncPtr, FontMapPtr);
extern char **identifyEncodingFile(const char *fileName);

