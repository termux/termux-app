/************************************************************
 Copyright 1996 by Silicon Graphics Computer Systems, Inc.

 Permission to use, copy, modify, and distribute this
 software and its documentation for any purpose and without
 fee is hereby granted, provided that the above copyright
 notice appear in all copies and that both that copyright
 notice and this permission notice appear in supporting
 documentation, and that the name of Silicon Graphics not be
 used in advertising or publicity pertaining to distribution
 of the software without specific prior written permission.
 Silicon Graphics makes no representation about the suitability
 of this software for any purpose. It is provided "as is"
 without any express or implied warranty.

 SILICON GRAPHICS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
 SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SILICON
 GRAPHICS BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
 THE USE OR PERFORMANCE OF THIS SOFTWARE.

 ********************************************************/
/***********************************************************

Copyright 1988, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


Copyright 1988 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "utils.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/keysym.h>

#define	DEBUG_VAR listingDebug
#include "xkbcomp.h"
#include <stdlib.h>

#ifdef _POSIX_SOURCE
# include <limits.h>
#else
# define _POSIX_SOURCE
# include <limits.h>
# undef _POSIX_SOURCE
#endif

#ifndef PATH_MAX
#ifdef WIN32
#define PATH_MAX 512
#else
#include <sys/param.h>
#endif
#ifndef PATH_MAX
#ifdef MAXPATHLEN
#define PATH_MAX MAXPATHLEN
#else
#define PATH_MAX 1024
#endif
#endif
#endif

#ifdef WIN32
# define WIN32_LEAN_AND_MEAN
# include <X11/Xwindows.h>
# define FileName(file) file.cFileName
# undef TEXT
# undef ALTERNATE
#else
# include <dirent.h>
# define FileName(file) file->d_name
#endif

#include "xkbpath.h"
#include "parseutils.h"
#include "misc.h"
#include "tokens.h"
#include <X11/extensions/XKBgeom.h>

#define	lowbit(x)	((x) & (-(x)))

#ifdef DEBUG
unsigned int listingDebug;
#endif

static int szListing = 0;
static int nListed = 0;
static int nFilesListed = 0;

typedef struct _Listing
{
    char *file;
    char *map;
} Listing;

static int szMapOnly;
static int nMapOnly;
static char **mapOnly;

static Listing *list = NULL;

/***====================================================================***/

int
AddMapOnly(char *map)
{
    if (nMapOnly >= szMapOnly)
    {
        if (szMapOnly < 1)
            szMapOnly = 5;
        else
            szMapOnly *= 2;
        mapOnly = reallocarray(list, szMapOnly, sizeof(char *));
        if (!mapOnly)
        {
            WSGO("Couldn't allocate list of maps\n");
            return 0;
        }
    }
    mapOnly[nMapOnly++] = map;
    return 1;
}

static int
AddListing(char *file, char *map)
{
    if (nListed >= szListing)
    {
        if (szListing < 1)
            szListing = 10;
        else
            szListing *= 2;
        list = reallocarray(list, szListing, sizeof(Listing));
        if (!list)
        {
            WSGO("Couldn't allocate list of files and maps\n");
            ACTION("Exiting\n");
            exit(1);
        }
    }

    list[nListed].file = file;
    list[nListed].map = map;
    nListed++;
    if (file != NULL)
        nFilesListed++;
    return 1;
}

/***====================================================================***/

static void
ListFile(FILE *outFile, const char *fileName, XkbFile *map)
{
    unsigned flags;
    const char *mapName;

    flags = map->flags;
    if ((flags & XkbLC_Hidden) && (!(verboseLevel & WantHiddenMaps)))
        return;
    if ((flags & XkbLC_Partial) && (!(verboseLevel & WantPartialMaps)))
        return;
    if (verboseLevel & WantLongListing)
    {
        fprintf(outFile, (flags & XkbLC_Hidden) ? "h" : "-");
        fprintf(outFile, (flags & XkbLC_Default) ? "d" : "-");
        fprintf(outFile, (flags & XkbLC_Partial) ? "p" : "-");
        fprintf(outFile, "----- ");
        if (map->type == XkmSymbolsIndex)
        {
            fprintf(outFile, (flags & XkbLC_AlphanumericKeys) ? "a" : "-");
            fprintf(outFile, (flags & XkbLC_ModifierKeys) ? "m" : "-");
            fprintf(outFile, (flags & XkbLC_KeypadKeys) ? "k" : "-");
            fprintf(outFile, (flags & XkbLC_FunctionKeys) ? "f" : "-");
            fprintf(outFile, (flags & XkbLC_AlternateGroup) ? "g" : "-");
            fprintf(outFile, "--- ");
        }
        else
            fprintf(outFile, "-------- ");
    }
    mapName = map->name;
    if ((!(verboseLevel & WantFullNames)) && ((flags & XkbLC_Default) != 0))
        mapName = NULL;
    if (dirsToStrip > 0)
    {
        const char *tmp, *last;
        int i;
        for (i = 0, tmp = last = fileName; (i < dirsToStrip) && tmp; i++)
        {
            last = tmp;
            tmp = strchr(tmp, '/');
            if (tmp != NULL)
                tmp++;
        }
        fileName = (tmp ? tmp : last);
    }
    if (mapName)
        fprintf(outFile, "%s(%s)\n", fileName, mapName);
    else
        fprintf(outFile, "%s\n", fileName);
    return;
}

/***====================================================================***/

static int
AddDirectory(char *head, char *ptrn, char *rest, char *map)
{
#ifdef WIN32
    HANDLE dirh;
    WIN32_FIND_DATA file;
#else
    DIR *dirp;
    struct dirent *file;
#endif
    int nMatch;

    if (map == NULL)
    {
        char *tmp = ptrn;
        if ((rest == NULL) && (ptrn != NULL) && (strchr(ptrn, '/') == NULL))
        {
            tmp = ptrn;
            map = strchr(ptrn, '(');
        }
        else if ((rest == NULL) && (ptrn == NULL) &&
                 (head != NULL) && (strchr(head, '/') == NULL))
        {
            tmp = head;
            map = strchr(head, '(');
        }
        if (map != NULL)
        {
            tmp = strchr(tmp, ')');
            if ((tmp == NULL) || (tmp[1] != '\0'))
            {
                ERROR("File and map must have the format file(map)\n");
                return 0;
            }
            *map = '\0';
            map++;
            *tmp = '\0';
        }
    }
#ifdef WIN32
    if ((dirh = FindFirstFile("*.*", &file)) == INVALID_HANDLE_VALUE)
        return 0;
#else
    if ((dirp = opendir((head ? head : "."))) == NULL)
        return 0;
#endif
    nMatch = 0;
#ifdef WIN32
    do
#else
    while ((file = readdir(dirp)) != NULL)
#endif
    {
        char *tmp, *filename;
        struct stat sbuf;

        filename = FileName(file);
        if (!filename || filename[0] == '.')
            continue;
        if (ptrn && (!XkbNameMatchesPattern(filename, ptrn)))
            continue;
#ifdef HAVE_ASPRINTF
        if (asprintf(&tmp, "%s%s%s",
                     (head ? head : ""), (head ? "/" : ""), filename) < 0)
            continue;
#else
        size_t tmpsize = (head ? strlen(head) : 0) + strlen(filename) + 2;
        tmp = malloc(tmpsize);
        if (!tmp)
            continue;
        snprintf(tmp, tmpsize, "%s%s%s",
                 (head ? head : ""), (head ? "/" : ""), filename);
#endif
        if (stat(tmp, &sbuf) < 0)
        {
            free(tmp);
            continue;
        }
        if (((rest != NULL) && (!S_ISDIR(sbuf.st_mode))) ||
            ((map != NULL) && (S_ISDIR(sbuf.st_mode))))
        {
            free(tmp);
            continue;
        }
        if (S_ISDIR(sbuf.st_mode))
        {
            if ((rest != NULL) || (verboseLevel & ListRecursive))
                nMatch += AddDirectory(tmp, rest, NULL, map);
        }
        else
            nMatch += AddListing(tmp, map);
    }
#ifdef WIN32
    while (FindNextFile(dirh, &file));
#endif
    return nMatch;
}

/***====================================================================***/

Bool
AddMatchingFiles(char *head_in)
{
    char *str, *head, *ptrn, *rest = NULL;

    if (head_in == NULL)
        return 0;
    ptrn = NULL;
    for (str = head_in; (*str != '\0') && (*str != '?') && (*str != '*');
         str++)
    {
        if ((str != head_in) && (*str == '/'))
            ptrn = str;
    }
    if (*str == '\0')
    {                           /* no wildcards */
        head = head_in;
        ptrn = NULL;
        rest = NULL;
    }
    else if (ptrn == NULL)
    {                           /* no slash before the first wildcard */
        head = NULL;
        ptrn = head_in;
    }
    else
    {                           /* slash followed by wildcard */
        head = head_in;
        *ptrn = '\0';
        ptrn++;
    }
    if (ptrn)
    {
        rest = strchr(ptrn, '/');
        if (rest != NULL)
        {
            *rest = '\0';
            rest++;
        }
    }
    if (((rest && ptrn)
         && ((strchr(ptrn, '(') != NULL) || (strchr(ptrn, ')') != NULL)))
        || (head
            && ((strchr(head, '(') != NULL) || (strchr(head, ')') != NULL))))
    {
        ERROR("Files/maps to list must have the form file(map)\n");
        ACTION("Illegal specifier ignored\n");
        return 0;
    }
    return AddDirectory(head, ptrn, rest, NULL);
}

/***====================================================================***/

static Bool
MapMatches(char *mapToConsider, char *ptrn)
{
    if (ptrn != NULL)
        return XkbNameMatchesPattern(mapToConsider, ptrn);
    if (nMapOnly < 1)
        return True;
    for (int i = 0; i < nMapOnly; i++)
    {
        if (XkbNameMatchesPattern(mapToConsider, mapOnly[i]))
            return True;
    }
    return False;
}

int
GenerateListing(const char *out_name)
{
    FILE *outFile;
    XkbFile *rtrn;

    if (nFilesListed < 1)
    {
        ERROR("Must specify at least one file or pattern to list\n");
        return 0;
    }
    if ((!out_name) || ((out_name[0] == '-') && (out_name[1] == '\0')))
        outFile = stdout;
    else if ((outFile = fopen(out_name, "w")) == NULL)
    {
        ERROR("Cannot open \"%s\" to write keyboard description\n",
               out_name);
        ACTION("Exiting\n");
        return 0;
    }
#ifdef DEBUG
    if (warningLevel > 9)
        fprintf(stderr, "should list:\n");
#endif
    for (int i = 0; i < nListed; i++)
    {
        unsigned oldWarningLevel;
#ifdef DEBUG
        if (warningLevel > 9)
        {
            fprintf(stderr, "%s(%s)\n",
                    (list[i].file ? list[i].file : "*"),
                    (list[i].map ? list[i].map : "*"));
        }
#endif
        oldWarningLevel = warningLevel;
        warningLevel = 0;
        if (list[i].file)
        {
            FILE *inputFile;
            struct stat sbuf;

            if (stat(list[i].file, &sbuf) < 0)
            {
                if (oldWarningLevel > 5)
                    WARN("Couldn't open \"%s\"\n", list[i].file);
                continue;
            }
            if (S_ISDIR(sbuf.st_mode))
            {
                if (verboseLevel & ListRecursive)
                    AddDirectory(list[i].file, NULL, NULL, NULL);
                continue;
            }

            inputFile = fopen(list[i].file, "r");
            if (!inputFile)
            {
                if (oldWarningLevel > 5)
                    WARN("Couldn't open \"%s\"\n", list[i].file);
                continue;
            }
            setScanState(list[i].file, 1);
            if (XKBParseFile(inputFile, &rtrn) && (rtrn != NULL))
            {
                char *mapName = list[i].map;
                XkbFile *mapToUse = rtrn;
                for (; mapToUse; mapToUse = (XkbFile *) mapToUse->common.next)
                {
                    if (!MapMatches(mapToUse->name, mapName))
                        continue;
                    ListFile(outFile, list[i].file, mapToUse);
                }
            }
            fclose(inputFile);
        }
        warningLevel = oldWarningLevel;
    }
    if (outFile != stdout)
    {
        fclose(outFile);
    }
    return 1;
}
