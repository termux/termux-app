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

/* Private types and functions for the encoding code. */
/* Used by the files `fontenc.h' and `encparse.h' */

typedef struct _FontEncSimpleMap {
    unsigned len;               /* might be 0x10000 */
    unsigned short row_size;
    unsigned short first;
    const unsigned short *map;
} FontEncSimpleMapRec, *FontEncSimpleMapPtr;

typedef struct _FontEncSimpleName {
    unsigned len;
    unsigned short first;
    char **map;
} FontEncSimpleNameRec, *FontEncSimpleNamePtr;

unsigned FontEncSimpleRecode(unsigned, void *);
unsigned FontEncUndefinedRecode(unsigned, void *);
char *FontEncSimpleName(unsigned, void *);
char *FontEncUndefinedName(unsigned, void *);

FontEncPtr FontEncReallyLoad(const char *, const char *);
