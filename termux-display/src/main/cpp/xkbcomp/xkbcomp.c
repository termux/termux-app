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

#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "bugprone-reserved-identifier"

#include "utils.h"
#include <stdio.h>
#include <ctype.h>
#include <X11/keysym.h>

/* for symlink attack security fix -- Branden Robinson */
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
/* end BR */

#define	DEBUG_VAR debugFlags
#include "xkbcomp.h"
#include <stdlib.h>
#include "xkbpath.h"
#include "parseutils.h"
#include "misc.h"
#include "tokens.h"
#include <X11/extensions/XKBgeom.h>
#include <dlfcn.h>


#ifdef WIN32
#define S_IRGRP 0
#define S_IWGRP 0
#define S_IROTH 0
#define S_IWOTH 0
#endif

#define	lowbit(x)	((x) & (-(x)))

/***====================================================================***/

#ifdef DEBUG
unsigned int debugFlags;
#endif

static const char *rootDir;
static char *inputFile;
static const char *inputMap;
static char *outputFile;
unsigned warningLevel = 5;
unsigned verboseLevel = 0;
unsigned dirsToStrip = 0;
static const char *preErrorMsg = NULL;
static const char *postErrorMsg = NULL;
static const char *errorPrefix = NULL;
static unsigned int device_id = XkbUseCoreKbd;

/***====================================================================***/

static Bool
parseArgs(int argc, char *argv[])
{
    for (register int i = 1; i < argc; i++)
    {
        if ((argv[i][0] != '-') || (strcmp(argv[i], "-") == 0))
        {
            if (inputFile == NULL)
                inputFile = argv[i];
            else if (outputFile == NULL)
                outputFile = argv[i];
            else if (warningLevel > 0)
            {
                WARN("Too many file names on command line\n");
                ACTION
                        ("Compiling %s, writing to %s, ignoring %s\n",
                         inputFile, outputFile, argv[i]);
            }
        }
        else if (strcmp(argv[i], "-em1") == 0)
        {
            if (++i >= argc)
            {
                if (warningLevel > 0)
                {
                    WARN("No pre-error message specified\n");
                    ACTION("Trailing \"-em1\" option ignored\n");
                }
            }
            else if (preErrorMsg != NULL)
            {
                if (warningLevel > 0)
                {
                    WARN("Multiple pre-error messages specified\n");
                    ACTION("Compiling %s, ignoring %s\n",
                            preErrorMsg, argv[i]);
                }
            }
            else
                preErrorMsg = argv[i];
        }
        else if (strcmp(argv[i], "-emp") == 0)
        {
            if (++i >= argc)
            {
                if (warningLevel > 0)
                {
                    WARN("No error prefix specified\n");
                    ACTION("Trailing \"-emp\" option ignored\n");
                }
            }
            else if (errorPrefix != NULL)
            {
                if (warningLevel > 0)
                {
                    WARN("Multiple error prefixes specified\n");
                    ACTION("Compiling %s, ignoring %s\n",
                            errorPrefix, argv[i]);
                }
            }
            else
                errorPrefix = argv[i];
        }
        else if (strcmp(argv[i], "-eml") == 0)
        {
            if (++i >= argc)
            {
                if (warningLevel > 0)
                {
                    WARN("No post-error message specified\n");
                    ACTION("Trailing \"-eml\" option ignored\n");
                }
            }
            else if (postErrorMsg != NULL)
            {
                if (warningLevel > 0)
                {
                    WARN("Multiple post-error messages specified\n");
                    ACTION("Compiling %s, ignoring %s\n",
                            postErrorMsg, argv[i]);
                }
            }
            else
                postErrorMsg = argv[i];
        }
        else if (strncmp(argv[i], "-R", 2) == 0)
        {
            if (argv[i][2] == '\0')
            {
                if (warningLevel > 0)
                {
                    WARN("No root directory specified\n");
                    ACTION("Ignoring -R option\n");
                }
            }
            else if (rootDir != NULL)
            {
                if (warningLevel > 0)
                {
                    WARN("Multiple root directories specified\n");
                    ACTION("Using %s, ignoring %s\n", rootDir, argv[i]);
                }
            }
            else
            {
                rootDir = &argv[i][2];
                if (warningLevel > 8)
                {
                    WARN("Changing root directory to \"%s\"\n", rootDir);
                }
                if (chdir(rootDir) == 0)
                {
                    XkbAddDirectoryToPath(".");
                } else if (warningLevel > 0)
                {
                       WARN("Couldn't change directory to \"%s\"\n", rootDir);
                       ACTION("Root directory (-R) option ignored\n");
                       rootDir = NULL;
                }
            }
        }
        else if (strncmp(argv[i], "-w", 2) == 0)
        {
            unsigned long utmp;
            char *tmp2;
            /* If text is just after "-w" in the same word, then it must
             * be a number and it is the warning level. Otherwise, if the
             * next argument is a number, then it is the warning level,
             * else the warning level is assumed to be 0.
             */
            if (argv[i][2] == '\0')
            {
                warningLevel = 0;
                if (i < argc - 1)
                {
                    utmp = strtoul(argv[i+1], &tmp2, 10);
                    if (argv[i+1][0] != '\0' && *tmp2 == '\0')
                    {
                        warningLevel = utmp > 10 ? 10 : utmp;
                        i++;
                    }
                }
            }
            else
            {
                utmp = strtoul(&argv[i][2], &tmp2, 10);
                if (*tmp2 == '\0')
                    warningLevel = utmp > 10 ? 10 : utmp;
                else
                {
                    ERROR("Unknown flag \"%s\" on command line\n", argv[i]);
                    return False;
                }
            }
        }
        else if (strcmp(argv[i], "-xkm") == 0) {}
        else
        {
            ERROR("Unknown flag \"%s\" on command line\n", argv[i]);
            return False;
        }
    }
    if (inputFile == NULL)
    {
        ERROR("No input file specified\n");
        return False;
    }
#ifndef WIN32
    else if (strchr(inputFile, ':') == NULL)
    {
#else
    else if ((strchr(inputFile, ':') == NULL) || (strlen(inputFile) > 2 &&
                                               isalpha(inputFile[0]) &&
                                               inputFile[1] == ':'
                                               && strchr(inputFile + 2,
                                                         ':') == NULL))
    {
#endif
        int len;
        len = strlen(inputFile);
        if (inputFile[len - 1] == ')')
        {
            char *tmpstr;
            if ((tmpstr = strchr(inputFile, '(')) != NULL)
            {
                *tmpstr = '\0';
                inputFile[len - 1] = '\0';
                tmpstr++;
                if (*tmpstr == '\0')
                {
                    WARN("Empty map in filename\n");
                    ACTION("Ignored\n");
                }
                else if (inputMap == NULL)
                {
                    inputMap = uStringDup(tmpstr);
                }
                else
                {
                    WARN("Map specified in filename and with -m flag\n");
                    ACTION("map from name (\"%s\") ignored\n", tmpstr);
                }
            }
            else
            {
                ERROR("Illegal name \"%s\" for input file\n", inputFile);
                return False;
            }
        }
        if (!access(inputFile, R_OK))
        {
            return False;
        }
    }
    return True;
}

/***====================================================================***/

#ifdef DEBUG
extern int yydebug;
#endif

int xkbcomp_main(int argc, char *argv[]) {
    FILE *file;         /* input file (or stdin) */
    XkbFile *rtrn;
    XkbFile *mapToUse;
    int ok;
    XkbFileInfo result;

    scan_set_file(stdin);
#ifdef DEBUG
    uSetDebugFile(NullString);
#endif
    uSetErrorFile(NullString);

    XkbInitIncludePath();
    if (!parseArgs(argc, argv))
        exit(1);
    if (preErrorMsg)
        uSetPreErrorMessage(preErrorMsg);
    if (errorPrefix)
        uSetErrorPrefix(errorPrefix);
    if (postErrorMsg)
        uSetPostErrorMessage(postErrorMsg);
    file = NULL;
    XkbInitAtoms(NULL);
    XkbAddDefaultDirectoriesToPath();
    if (inputFile != NULL)
    {
        if (strcmp(inputFile, "-") == 0)
        {
            file = stdin;
            inputFile = "stdin";
        }
        else
        {
            file = fopen(inputFile, "r");
        }
    }
    if (file)
    {
        ok = True;
        setScanState(inputFile, 1);
        if (XKBParseFile(file, &rtrn) && (rtrn != NULL))
        {
            fclose(file);
            mapToUse = rtrn;
            if (inputMap != NULL) /* map specified on cmdline? */
            {
                while ((mapToUse)
                       && (!uStringEqual(mapToUse->name, inputMap)))
                {
                    mapToUse = (XkbFile *) mapToUse->common.next;
                }
                if (!mapToUse)
                {
                    FATAL("No map named \"%s\" in \"%s\"\n",
                           inputMap, inputFile);
                    /* NOTREACHED */
                }
            }
            else if (rtrn->common.next != NULL)
            {
                /* look for map with XkbLC_Default flag. */
                mapToUse = rtrn;
                for (; mapToUse; mapToUse = (XkbFile *) mapToUse->common.next)
                {
                    if (mapToUse->flags & XkbLC_Default)
                        break;
                }
                if (!mapToUse)
                {
                    mapToUse = rtrn;
                    if (warningLevel > 4)
                    {
                        WARN
                            ("No map specified, but \"%s\" has several\n",
                             inputFile);
                        ACTION
                            ("Using the first defined map, \"%s\"\n",
                             mapToUse->name);
                    }
                }
            }
            bzero(&result, sizeof(result));
            result.type = mapToUse->type;
            if ((result.xkb = XkbAllocKeyboard()) == NULL)
            {
                WSGO("Cannot allocate keyboard description\n");
                /* NOTREACHED */
            }
            switch (mapToUse->type)
            {
            case XkmSemanticsFile:
            case XkmLayoutFile:
            case XkmKeymapFile:
                ok = CompileKeymap(mapToUse, &result, MergeReplace);
                break;
            case XkmKeyNamesIndex:
                ok = CompileKeycodes(mapToUse, &result, MergeReplace);
                break;
            case XkmTypesIndex:
                ok = CompileKeyTypes(mapToUse, &result, MergeReplace);
                break;
            case XkmSymbolsIndex:
                /* if it's just symbols, invent key names */
                result.xkb->flags |= AutoKeyNames;
                ok = False;
                break;
            case XkmCompatMapIndex:
                ok = CompileCompatMap(mapToUse, &result, MergeReplace, NULL);
                break;
            case XkmGeometryFile:
            case XkmGeometryIndex:
                /* if it's just a geometry, invent key names */
                result.xkb->flags |= AutoKeyNames;
                ok = CompileGeometry(mapToUse, &result, MergeReplace);
                break;
            default:
                WSGO("Unknown file type %d\n", mapToUse->type);
                ok = False;
                break;
            }
            result.xkb->device_spec = device_id;
        }
        else
        {
            INFO("Errors encountered in %s; not compiled.\n", inputFile);
            ok = False;
        }
    }
    else
    {
        fprintf(stderr, "Cannot open \"%s\" to compile\n", inputFile);
        ok = 0;
    }
    if (ok)
    {
        FILE *out = stdout;
        if (outputFile != NULL)
        {
            if (strcmp(outputFile, "-") == 0)
                outputFile = "stdout";
            else
            {
                /*
                 * fix to prevent symlink attack (e.g.,
                 * ln -s /etc/passwd /var/tmp/server-0.xkm)
                 */
                /*
                 * this patch may have POSIX, Linux, or GNU libc bias
                 * -- Branden Robinson
                 */
                int outputFileFd;
                int binMode = 0;
                const char *openMode = "w";
                unlink(outputFile);
#ifdef O_BINARY
                binMode = O_BINARY;
                openMode = "wb";
#endif
                outputFileFd =
                    open(outputFile, O_WRONLY | O_CREAT | O_EXCL,
                         S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH
                         | S_IWOTH | binMode);
                if (outputFileFd < 0)
                {
                    ERROR
                        ("Cannot open \"%s\" to write keyboard description\n",
                         outputFile);
                    ACTION("Exiting\n");
                    exit(1);
                }
#ifndef WIN32
                out = fdopen(outputFileFd, openMode);
#else
                close(outputFileFd);
                out = fopen(outputFile, "wb");
#endif
                /* end BR */
                if (out == NULL)
                {
                    ERROR
                        ("Cannot open \"%s\" to write keyboard description\n",
                         outputFile);
                    ACTION("Exiting\n");
                    exit(1);
                }
            }
        }
        ok = XkbWriteXKMFile(out, &result);
        {
            if (fclose(out))
            {
                ERROR("Cannot close \"%s\" properly (not enough space?)\n",
                       outputFile);
                ok= False;
            }
            else if (!ok)
            {
                ERROR("%s in %s\n", _XkbErrMessages[_XkbErrCode],
                       _XkbErrLocation ? _XkbErrLocation : "unknown");
            }
            if (!ok)
            {
                ACTION("Output file \"%s\" removed\n", outputFile);
                unlink(outputFile);
            }
        }
    }
    uFinishUp();
    return (ok == 0);
}
