/*

Copyright 1991, 1998  The Open Group

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

*/

/*
 * Author:  Keith Packard, MIT X Consortium
 */

/*
 * dirfile.c
 *
 * Read fonts.dir and fonts.alias files
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include <X11/fonts/fntfilst.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include "src/util/replace.h"

static Bool AddFileNameAliases ( FontDirectoryPtr dir );
static int ReadFontAlias ( char *directory, Bool isFile,
			   FontDirectoryPtr *pdir );
static int lexAlias ( FILE *file, char **lexToken );
static int lexc ( FILE *file );

#pragma GCC diagnostic ignored "-Wformat-nonliteral"

int
FontFileReadDirectory (const char *directory, FontDirectoryPtr *pdir)
{
    char        file_name[MAXFONTFILENAMELEN];
    char        font_name[MAXFONTNAMELEN];
    char        dir_file[MAXFONTFILENAMELEN];
    char	dir_path[MAXFONTFILENAMELEN];
    char	*ptr;
    FILE       *file = 0;
    int         file_fd,
                count,
                num_fonts,
                status;
    struct stat	statb;
    static char format[24] = "";
#if defined(WIN32)
    int i;
#endif

    FontDirectoryPtr	dir = NullFontDirectory;

    if (strlen(directory) + 1 + sizeof(FontDirFile) > sizeof(dir_file))
	return BadFontPath;

    /* Check for font directory attributes */
#if !defined(WIN32)
    if ((ptr = strchr(directory, ':'))) {
#else
    /* WIN32 path might start with a drive letter, don't clip this */
    if ((ptr = strchr(directory+2, ':'))) {
#endif
	strncpy(dir_path, directory, ptr - directory);
	dir_path[ptr - directory] = '\0';
    } else {
	strlcpy(dir_path, directory, sizeof(dir_path));
    }
    strlcpy(dir_file, dir_path, sizeof(dir_file));
    if (dir_file[strlen(dir_file) - 1] != '/')
	strlcat(dir_file, "/", sizeof(dir_file));
    strlcat(dir_file, FontDirFile, sizeof(dir_file));
#ifndef WIN32
    file_fd = open(dir_file, O_RDONLY | O_NOFOLLOW);
    if (file_fd >= 0) {
	file = fdopen(file_fd, "rt");
    }
#else
    file = fopen(dir_file, "rt");
#endif
    if (file) {
#ifndef WIN32
	if (fstat (fileno(file), &statb) == -1)
#else
	if (stat (dir_file, &statb) == -1)
#endif
        {
            fclose(file);
	    return BadFontPath;
        }
	count = fscanf(file, "%d\n", &num_fonts);
	if ((count == EOF) || (count != 1)) {
	    fclose(file);
	    return BadFontPath;
	}
	dir = FontFileMakeDir(directory, num_fonts);
	if (dir == NULL) {
	    fclose(file);
	    return BadFontPath;
	}
	dir->dir_mtime = statb.st_mtime;
	if (format[0] == '\0')
	    snprintf(format, sizeof(format), "%%%ds %%%d[^\n]\n",
		     MAXFONTFILENAMELEN-1, MAXFONTNAMELEN-1);

	while ((count = fscanf(file, format, file_name, font_name)) != EOF) {
#if defined(WIN32)
	    /* strip any existing trailing CR */
	    for (i=0; i<strlen(font_name); i++) {
		if (font_name[i]=='\r') font_name[i] = '\0';
	    }
#endif
	    if (count != 2) {
		FontFileFreeDir (dir);
		fclose(file);
		return BadFontPath;
	    }

	    /*
	     * We blindly try to load all the font files specified.
	     * In theory, we might want to warn that some of the fonts
	     * couldn't be loaded.
	     */
	    FontFileAddFontFile (dir, font_name, file_name);
	}
	fclose(file);

    } else if (errno != ENOENT) {
	return BadFontPath;
    }
    status = ReadFontAlias(dir_path, FALSE, &dir);
    if (status != Successful) {
	if (dir)
	    FontFileFreeDir (dir);
	return status;
    }
    if (!dir)
	return BadFontPath;

    FontFileSortDir(dir);

    *pdir = dir;
    return Successful;
}

Bool
FontFileDirectoryChanged(FontDirectoryPtr dir)
{
    char	dir_file[MAXFONTFILENAMELEN];
    struct stat	statb;

    if (strlen(dir->directory) + sizeof(FontDirFile) > sizeof(dir_file))
	return FALSE;

    strlcpy (dir_file, dir->directory, sizeof(dir_file));
    strlcat (dir_file, FontDirFile, sizeof(dir_file));
    if (stat (dir_file, &statb) == -1)
    {
	if (errno != ENOENT || dir->dir_mtime != 0)
	    return TRUE;
	return FALSE;		/* doesn't exist and never did: no change */
    }
    if (dir->dir_mtime != statb.st_mtime)
	return TRUE;

    if ((strlen(dir->directory) + sizeof(FontAliasFile)) > sizeof(dir_file))
	return FALSE;
    strlcpy (dir_file, dir->directory, sizeof(dir_file));
    strlcat (dir_file, FontAliasFile, sizeof(dir_file));
    if (stat (dir_file, &statb) == -1)
    {
	if (errno != ENOENT || dir->alias_mtime != 0)
	    return TRUE;
	return FALSE;		/* doesn't exist and never did: no change */
    }
    if (dir->alias_mtime != statb.st_mtime)
	return TRUE;
    return FALSE;
}

/*
 * Make each of the file names an automatic alias for each of the files.
 */

static Bool
AddFileNameAliases(FontDirectoryPtr dir)
{
    int		    i;
    char	    copy[MAXFONTFILENAMELEN];
    char	    *fileName;
    FontTablePtr    table;
    FontRendererPtr renderer;
    int		    len;
    FontNameRec	    name;

    table = &dir->nonScalable;
    for (i = 0; i < table->used; i++) {
	if (table->entries[i].type != FONT_ENTRY_BITMAP)
	    continue;
	fileName = table->entries[i].u.bitmap.fileName;
	renderer = FontFileMatchRenderer (fileName);
	if (!renderer)
	    continue;

	len = strlen (fileName) - renderer->fileSuffixLen;
	if (len >= sizeof(copy))
	    continue;
	CopyISOLatin1Lowered (copy, fileName, len);
	copy[len] = '\0';
	name.name = copy;
	name.length = len;
	name.ndashes = FontFileCountDashes (copy, len);

	if (!FontFileFindNameInDir(table, &name)) {
	    if (!FontFileAddFontAlias (dir, copy, table->entries[i].name.name))
		return FALSE;
	}
    }
    return TRUE;
}

/*
 * parse the font.alias file.  Format is:
 *
 * alias font-name
 *
 * To embed white-space in an alias name, enclose it like "font name"
 * in double quotes.  \ escapes and character, so
 * "font name \"With Double Quotes\" \\ and \\ back-slashes"
 * works just fine.
 *
 * A line beginning with a ! denotes a newline-terminated comment.
 */

/*
 * token types
 */

#define NAME		0
#define NEWLINE		1
#define DONE		2
#define EALLOC		3

static int
ReadFontAlias(char *directory, Bool isFile, FontDirectoryPtr *pdir)
{
    char		alias[MAXFONTNAMELEN];
    char		font_name[MAXFONTNAMELEN];
    char		alias_file[MAXFONTFILENAMELEN];
    int			file_fd;
    FILE		*file = 0;
    FontDirectoryPtr	dir;
    int			token;
    char		*lexToken;
    int			status = Successful;
    struct stat		statb;

    if (strlen(directory) >= sizeof(alias_file))
	return BadFontPath;
    dir = *pdir;
    strlcpy(alias_file, directory, sizeof(alias_file));
    if (!isFile) {
	if (strlen(directory) + 1 + sizeof(FontAliasFile) > sizeof(alias_file))
	    return BadFontPath;
	if (directory[strlen(directory) - 1] != '/')
	    strlcat(alias_file, "/", sizeof(alias_file));
	strlcat(alias_file, FontAliasFile, sizeof(alias_file));
    }

#ifndef WIN32
    file_fd = open(alias_file, O_RDONLY | O_NOFOLLOW);
    if (file_fd >= 0) {
	file = fdopen(file_fd, "rt");
    }
#else
    file = fopen(alias_file, "rt");
#endif

    if (!file)
	return ((errno == ENOENT) ? Successful : BadFontPath);
    if (!dir)
	*pdir = dir = FontFileMakeDir(directory, 10);
    if (!dir)
    {
	fclose (file);
	return AllocError;
    }
#ifndef WIN32
    if (fstat (fileno (file), &statb) == -1)
#else
    if (stat (alias_file, &statb) == -1)
#endif
    {
	fclose (file);
	return BadFontPath;
    }
    dir->alias_mtime = statb.st_mtime;
    while (status == Successful) {
	token = lexAlias(file, &lexToken);
	switch (token) {
	case NEWLINE:
	    break;
	case DONE:
	    fclose(file);
	    return Successful;
	case EALLOC:
	    status = AllocError;
	    break;
	case NAME:
	    if (strlen(lexToken) >= sizeof(alias)) {
		status = BadFontPath;
		break;
	    }
	    strlcpy(alias, lexToken, sizeof(alias));
	    token = lexAlias(file, &lexToken);
	    switch (token) {
	    case NEWLINE:
		if (strcmp(alias, "FILE_NAMES_ALIASES"))
		    status = BadFontPath;
		else if (!AddFileNameAliases(dir))
		    status = AllocError;
		break;
	    case DONE:
		status = BadFontPath;
		break;
	    case EALLOC:
		status = AllocError;
		break;
	    case NAME:
		if (strlen(lexToken) >= sizeof(font_name)) {
		    status = BadFontPath;
		    break;
		}
		CopyISOLatin1Lowered(alias, alias, strlen(alias));
		CopyISOLatin1Lowered(font_name, lexToken, strlen(lexToken));
		if (!FontFileAddFontAlias (dir, alias, font_name))
		    status = AllocError;
		break;
	    }
	}
    }
    fclose(file);
    return status;
}

#define QUOTE		0
#define WHITE		1
#define NORMAL		2
#define END		3
#define NL		4
#define BANG		5

static int  charClass;

static int
lexAlias(FILE *file, char **lexToken)
{
    int         c;
    char       *t;
    enum state {
	Begin, Normal, Quoted, Comment
    }           state;
    int         count;

    static char *tokenBuf = (char *) NULL;
    static int  tokenSize = 0;

    t = tokenBuf;
    count = 0;
    state = Begin;
    for (;;) {
	if (count == tokenSize) {
	    int         nsize;
	    char       *nbuf;

	    if (tokenSize >= (INT_MAX >> 2))
		/* Stop before we overflow */
		return EALLOC;
	    nsize = tokenSize ? (tokenSize << 1) : 64;
	    nbuf = realloc(tokenBuf, nsize);
	    if (!nbuf)
		return EALLOC;
	    tokenBuf = nbuf;
	    tokenSize = nsize;
	    t = tokenBuf + count;
	}
	c = lexc(file);
	switch (charClass) {
	case QUOTE:
	    switch (state) {
	    case Begin:
	    case Normal:
		state = Quoted;
		break;
	    case Quoted:
		state = Normal;
		break;
	    case Comment:
		break;
	    }
	    break;
	case WHITE:
	    switch (state) {
	    case Begin:
	    case Comment:
		continue;
	    case Normal:
		*t = '\0';
		*lexToken = tokenBuf;
		return NAME;
	    case Quoted:
		break;
	    }
	    /* fall through */
	case NORMAL:
	    switch (state) {
	    case Begin:
		state = Normal;
		break;
	    case Comment:
		continue;
	    default:
		break;
	    }
	    *t++ = c;
	    ++count;
	    break;
	case END:
	case NL:
	    switch (state) {
	    case Begin:
	    case Comment:
		*lexToken = (char *) NULL;
		return charClass == END ? DONE : NEWLINE;
	    default:
		*t = '\0';
		*lexToken = tokenBuf;
		ungetc(c, file);
		return NAME;
	    }
	    break;
	case BANG:
	    switch (state) {
	    case Begin:
		state = Comment;
		break;
            case Comment:
		break;
            default:
		*t++ = c;
		++count;
	    }
	    break;
	}
    }
}

static int
lexc(FILE *file)
{
    int         c;

    c = getc(file);
    switch (c) {
    case EOF:
	charClass = END;
	break;
    case '\\':
	c = getc(file);
	if (c == EOF)
	    charClass = END;
	else
	    charClass = NORMAL;
	break;
    case '"':
	charClass = QUOTE;
	break;
    case ' ':
    case '\t':
	charClass = WHITE;
	break;
    case '\r':
    case '\n':
	charClass = NL;
	break;
    case '!':
	charClass = BANG;
	break;
    default:
	charClass = NORMAL;
	break;
    }
    return c;
}
