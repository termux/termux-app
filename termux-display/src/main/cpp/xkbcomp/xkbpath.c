/************************************************************
 Copyright (c) 1994 by Silicon Graphics Computer Systems, Inc.

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

#include <X11/Xlib.h>
#include <X11/XKBlib.h>

#define	DEBUG_VAR debugFlags
#include "utils.h"
#include <stdlib.h>
#include <X11/extensions/XKM.h>
#include "xkbpath.h"

#ifndef PATH_MAX
#define	PATH_MAX 1024
#endif

#define	PATH_CHUNK	8       /* initial szPath */

static Bool noDefaultPath = False;
static int szPath;         /* number of entries allocated for includePath */
static int nPathEntries;   /* number of actual entries in includePath */
static char **includePath; /* Holds all directories we might be including data from */

/**
 * Extract the first token from an include statement.
 * @param str_inout Input statement, modified in-place. Can be passed in
 * repeatedly. If str_inout is NULL, the parsing has completed.
 * @param file_rtrn Set to the include file to be used.
 * @param map_rtrn Set to whatever comes after ), if any.
 * @param nextop_rtrn Set to the next operation in the complete statement.
 * @param extra_data Set to the string between ( and ), if any.
 *
 * @return True if parsing was successful, False for an illegal string.
 *
 * Example: "evdev+aliases(qwerty)"
 *      str_inout = aliases(qwerty)
 *      nextop_retrn = +
 *      extra_data = NULL
 *      file_rtrn = evdev
 *      map_rtrn = NULL
 *
 * 2nd run with "aliases(qwerty)"
 *      str_inout = NULL
 *      file_rtrn = aliases
 *      map_rtrn = qwerty
 *      extra_data = NULL
 *      nextop_retrn = ""
 *
 */
Bool
XkbParseIncludeMap(char **str_inout, char **file_rtrn, char **map_rtrn,
                   char *nextop_rtrn, char **extra_data)
{
    char *tmp, *str, *next;

    str = *str_inout;
    if ((*str == '+') || (*str == '|'))
    {
        *file_rtrn = *map_rtrn = NULL;
        *nextop_rtrn = *str;
        next = str + 1;
    }
    else if (*str == '%')
    {
        *file_rtrn = *map_rtrn = NULL;
        *nextop_rtrn = str[1];
        next = str + 2;
    }
    else
    {
        /* search for tokens inside the string */
        next = strpbrk(str, "|+");
        if (next)
        {
            /* set nextop_rtrn to \0, next to next character */
            *nextop_rtrn = *next;
            *next++ = '\0';
        }
        else
        {
            *nextop_rtrn = '\0';
            next = NULL;
        }
        /* search for :, store result in extra_data */
        tmp = strchr(str, ':');
        if (tmp != NULL)
        {
            *tmp++ = '\0';
            *extra_data = uStringDup(tmp);
        }
        else
        {
            *extra_data = NULL;
        }
        tmp = strchr(str, '(');
        if (tmp == NULL)
        {
            *file_rtrn = uStringDup(str);
            *map_rtrn = NULL;
        }
        else if (str[0] == '(')
        {
            free(*extra_data);
            return False;
        }
        else
        {
            *tmp++ = '\0';
            *file_rtrn = uStringDup(str);
            str = tmp;
            tmp = strchr(str, ')');
            if ((tmp == NULL) || (tmp[1] != '\0'))
            {
                free(*file_rtrn);
                free(*extra_data);
                return False;
            }
            *tmp++ = '\0';
            *map_rtrn = uStringDup(str);
        }
    }
    if (*nextop_rtrn == '\0')
        *str_inout = NULL;
    else if ((*nextop_rtrn == '|') || (*nextop_rtrn == '+'))
        *str_inout = next;
    else
        return False;
    return True;
}

/**
 * Init memory for include paths.
 */
Bool
XkbInitIncludePath(void)
{
    szPath = PATH_CHUNK;
    includePath = (char **) calloc(szPath, sizeof(char *));
    if (includePath == NULL)
        return False;
    return True;
}

void
XkbAddDefaultDirectoriesToPath(void)
{
    if (noDefaultPath)
        return;
    XkbAddDirectoryToPath(DFLT_XKB_CONFIG_ROOT);
}

/**
 * Remove all entries from the global includePath.
 */
static void
XkbClearIncludePath(void)
{
    if (szPath > 0)
    {
        for (int i = 0; i < nPathEntries; i++)
        {
            free(includePath[i]);
            includePath[i] = NULL;
        }
        nPathEntries = 0;
    }
    noDefaultPath = True;
    return;
}

/**
 * Add the given path to the global includePath variable.
 * If dir is NULL, the includePath is emptied.
 */
Bool
XkbAddDirectoryToPath(const char *dir)
{
    int len;
    if ((dir == NULL) || (dir[0] == '\0'))
    {
        XkbClearIncludePath();
        return True;
    }
    len = strlen(dir);
    if (len + 2 >= PATH_MAX)
    {                           /* allow for '/' and at least one character */
        ERROR("Path entry (%s) too long (maximum length is %d)\n",
               dir, PATH_MAX - 3);
        return False;
    }
    if (nPathEntries >= szPath)
    {
        char **new;
        szPath += PATH_CHUNK;
        new = (char **) realloc(includePath, szPath * sizeof(char *));
        if (new == NULL)
        {
            WSGO("Allocation failed (includePath)\n");
            return False;
        }
        else
            includePath = new;
    }
    includePath[nPathEntries] = strdup(dir);
    if (includePath[nPathEntries] == NULL)
    {
        WSGO("Allocation failed (includePath[%d])\n", nPathEntries);
        return False;
    }
    nPathEntries++;
    return True;
}

/***====================================================================***/

/**
 * Return the xkb directory based on the type.
 * Do not free the memory returned by this function.
 */
char *
XkbDirectoryForInclude(unsigned type)
{
    static char buf[32];

    switch (type)
    {
    case XkmSemanticsFile:
        strcpy(buf, "semantics");
        break;
    case XkmLayoutFile:
        strcpy(buf, "layout");
        break;
    case XkmKeymapFile:
        strcpy(buf, "keymap");
        break;
    case XkmKeyNamesIndex:
        strcpy(buf, "keycodes");
        break;
    case XkmTypesIndex:
        strcpy(buf, "types");
        break;
    case XkmSymbolsIndex:
        strcpy(buf, "symbols");
        break;
    case XkmCompatMapIndex:
        strcpy(buf, "compat");
        break;
    case XkmGeometryFile:
    case XkmGeometryIndex:
        strcpy(buf, "geometry");
        break;
    default:
        strcpy(buf, "");
        break;
    }
    return buf;
}

/***====================================================================***/

typedef struct _FileCacheEntry
{
    const char *name;
    unsigned type;
    char *path;
    void *data;
    struct _FileCacheEntry *next;
} FileCacheEntry;
static FileCacheEntry *fileCache;

/**
 * Add the file with the given name to the internal cache to avoid opening and
 * parsing the file multiple times. If a cache entry for the same name + type
 * is already present, the entry is overwritten and the data belonging to the
 * previous entry is returned.
 *
 * @parameter name The name of the file (e.g. evdev).
 * @parameter type Type of the file (XkbTypesIdx, ... or XkbSemanticsFile, ...)
 * @parameter path The full path to the file.
 * @parameter data Already parsed data.
 *
 * @return The data from the overwritten file or NULL.
 */
void *
XkbAddFileToCache(const char *name, unsigned type, char *path, void *data)
{
    FileCacheEntry *entry;

    for (entry = fileCache; entry != NULL; entry = entry->next)
    {
        if ((type == entry->type) && (uStringEqual(name, entry->name)))
        {
            void *old = entry->data;
            WSGO("Replacing file cache entry (%s/%d)\n", name, type);
            entry->path = path;
            entry->data = data;
            return old;
        }
    }
    entry = malloc(sizeof(FileCacheEntry));
    if (entry != NULL)
    {
        *entry = (FileCacheEntry) {
            .name = name,
            .type = type,
            .path = path,
            .data = data,
            .next = fileCache
        };
        fileCache = entry;
    }
    return NULL;
}

/**
 * Search for the given name + type in the cache.
 *
 * @parameter name The name of the file (e.g. evdev).
 * @parameter type Type of the file (XkbTypesIdx, ... or XkbSemanticsFile, ...)
 * @parameter pathRtrn Set to the full path of the given entry.
 *
 * @return the data from the cache entry or NULL if no matching entry was found.
 */
void *
XkbFindFileInCache(const char *name, unsigned type, char **pathRtrn)
{
    FileCacheEntry *entry;

    for (entry = fileCache; entry != NULL; entry = entry->next)
    {
        if ((type == entry->type) && (uStringEqual(name, entry->name)))
        {
            *pathRtrn = entry->path;
            return entry->data;
        }
    }
    return NULL;
}

/***====================================================================***/

/**
 * Search for the given file name in the include directories.
 *
 * @param type one of XkbTypesIndex, XkbCompatMapIndex, ..., or
 * XkbSemanticsFile, XkmKeymapFile, ...
 * @param pathReturn is set to the full path of the file if found.
 *
 * @return an FD to the file or NULL. If NULL is returned, the value of
 * pathRtrn is undefined.
 */
FILE *
XkbFindFileInPath(const char *name, unsigned type, char **pathRtrn)
{
    FILE *file = NULL;
    int nameLen, typeLen;
    char buf[PATH_MAX];
    const char *typeDir;

    typeDir = XkbDirectoryForInclude(type);
    nameLen = strlen(name);
    typeLen = strlen(typeDir);
    for (int i = 0; i < nPathEntries; i++)
    {
        int pathLen = strlen(includePath[i]);
        if (typeLen < 1)
            continue;

        if ((nameLen + typeLen + pathLen + 2) >= PATH_MAX)
        {
            ERROR("File name (%s/%s/%s) too long\n", includePath[i],
                   typeDir, name);
            ACTION("Ignored\n");
            continue;
        }
        snprintf(buf, sizeof(buf), "%s/%s/%s", includePath[i], typeDir, name);
        file = fopen(buf, "r");
        if (file != NULL)
            break;
    }

    if ((file != NULL) && (pathRtrn != NULL))
    {
        *pathRtrn = strdup(buf);
    }
    return file;
}
