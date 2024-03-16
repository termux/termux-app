/*
 *
 * Copyright 1999 SuSE, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SuSE not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  SuSE makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * SuSE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL SuSE
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Keith Packard, SuSE, Inc.
 */

#include <X11/Xdefs.h>
#include <X11/fonts/font.h>
#include <X11/fonts/fontxlfd.h>
#include <X11/fonts/fntfil.h>
#include <X11/fonts/fntfilio.h>
#include <X11/fonts/fntfilst.h>

typedef struct _BuiltinFile {
    const char  *name;
    int		len;
    const char  *bits;
} BuiltinFileRec, *BuiltinFilePtr;

typedef struct _BuiltinDir {
    const char	*file_name;
    char	*font_name;
} BuiltinDirRec, *BuiltinDirPtr;

typedef struct _BuiltinAlias {
    char	*alias_name;
    char	*font_name;
} BuiltinAliasRec, *BuiltinAliasPtr;

extern const BuiltinFileRec	builtin_files[];
extern const int		builtin_files_count;

extern const BuiltinDirRec	builtin_dir[];
extern const int		builtin_dir_count;

extern const BuiltinAliasRec	builtin_alias[];
extern const int		builtin_alias_count;

extern FontFilePtr	BuiltinFileOpen (const char *);
extern int		BuiltinFileClose (BufFilePtr, int);
extern int BuiltinReadDirectory (const char *, FontDirectoryPtr *);
extern void BuiltinRegisterFontFileFunctions (void);

extern void BuiltinRegisterFpeFunctions (void);
