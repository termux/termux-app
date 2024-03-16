/*
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "libxfontint.h"
#include "builtin.h"

static BuiltinDirPtr
BuiltinDirsDup (const BuiltinDirPtr a_dirs,
                int a_dirs_len)
{
    BuiltinDirPtr dirs=NULL ;
    int i=0 ;

    if (!a_dirs)
        return NULL ;

    dirs = calloc (a_dirs_len, sizeof (BuiltinDirRec)) ;
    if (!dirs)
        return NULL ;

    for (i=0; i < a_dirs_len; i++) {
	dirs[i].file_name = strdup(a_dirs[i].file_name);
	dirs[i].font_name = strdup(a_dirs[i].font_name);
    }
    return dirs ;
}

/**
 * Copy a_save back into a_cur
 * @param a_cur the instance of BuiltinDir to restore
 * @param a_saved the saved instance of BuiltinDir to copy into a_cur
 * @return 0 if went okay, 1 otherwise.
 */
static int
BuiltinDirRestore (BuiltinDirPtr a_cur,
                   const BuiltinDirPtr a_saved)
{
    if (!a_cur)
        return 1 ;
    if (!a_saved)
        return 0 ;

    if (a_saved->font_name)
        memmove (a_cur->font_name, a_saved->font_name, strlen (a_saved->font_name)) ;
    return 0 ;
}


static int
BuiltinDirsRestore (BuiltinDirPtr a_cur_tab,
                    const BuiltinDirPtr a_saved_tab,
                    int a_tab_len)
{
    int i=0 ;

    if (!a_cur_tab)
        return 1 ;
    if (!a_saved_tab)
        return 0 ;

    for (i=0 ; i < a_tab_len; i++) {
        if (BuiltinDirRestore (&a_cur_tab[i], &a_saved_tab[i]))
            return 1 ;
    }
    return 0 ;
}

static BuiltinAliasPtr
BuiltinAliasesDup (const BuiltinAliasPtr a_aliases,
                   int a_aliases_len)
{
    BuiltinAliasPtr aliases=NULL ;
    int i=0 ;

    if (!a_aliases)
        return NULL ;

    aliases = calloc (a_aliases_len, sizeof (BuiltinAliasRec)) ;
    if (!aliases)
        return NULL ;

    for (i=0; i < a_aliases_len; i++) {
	aliases[i].font_name = strdup(a_aliases[i].font_name);
    }
    return aliases ;
}

/**
 * Copy a_save back into a_cur
 * @param a_cur the instance of BuiltinAlias to restore
 * @param a_saved the saved instance of BuiltinAlias to copy into a_cur
 * @return 0 if went okay, 1 otherwise.
 */
static int
BuiltinAliasRestore (BuiltinAliasPtr a_cur,
                     const BuiltinAliasPtr a_save)
{
    if (!a_cur)
        return 1 ;
    if (!a_save)
        return 0 ;
    if (a_save->alias_name)
        memmove (a_cur->alias_name, a_save->alias_name, strlen (a_save->alias_name)) ;
    if (a_save->font_name)
        memmove (a_cur->font_name, a_save->font_name, strlen (a_save->font_name)) ;
    return 0 ;
}

static int
BuiltinAliasesRestore (BuiltinAliasPtr a_cur_tab,
                       const BuiltinAliasPtr a_saved_tab,
                       int a_tab_len)
{
    int i=0 ;

    if (!a_cur_tab)
        return 1 ;
    if (!a_saved_tab)
        return 0 ;

    for (i=0 ; i < a_tab_len; i++) {
        if (BuiltinAliasRestore (&a_cur_tab[i], &a_saved_tab[i]))
            return 1 ;
    }
    return 0 ;
}

int
BuiltinReadDirectory (const char *directory, FontDirectoryPtr *pdir)
{
    FontDirectoryPtr	dir;
    int			i;

    static BuiltinDirPtr saved_builtin_dir;
    static BuiltinAliasPtr saved_builtin_alias;

    dir = FontFileMakeDir ("", builtin_dir_count);

    if (saved_builtin_dir)
    {
        BuiltinDirsRestore ((BuiltinDirPtr) builtin_dir,
                            saved_builtin_dir,
                            builtin_dir_count) ;
    }
    else
    {
        saved_builtin_dir = BuiltinDirsDup ((const BuiltinDirPtr) builtin_dir,
                                            builtin_dir_count) ;
    }

    if (saved_builtin_alias)
    {
        BuiltinAliasesRestore ((BuiltinAliasPtr) builtin_alias,
                               saved_builtin_alias,
                               builtin_alias_count) ;
    }
    else
    {
        saved_builtin_alias = BuiltinAliasesDup ((const BuiltinAliasPtr) builtin_alias,
                                                 builtin_alias_count) ;
    }

    for (i = 0; i < builtin_dir_count; i++)
    {
	if (!FontFileAddFontFile (dir,
				  (char *) builtin_dir[i].font_name,
				  (char *) builtin_dir[i].file_name))
	{
	    FontFileFreeDir (dir);
	    return BadFontPath;
	}
    }
    for (i = 0; i < builtin_alias_count; i++)
    {
	if (!FontFileAddFontAlias (dir,
				   (char *) builtin_alias[i].alias_name,
				   (char *) builtin_alias[i].font_name))
	{
	    FontFileFreeDir (dir);
	    return BadFontPath;
	}
    }
    FontFileSortDir (dir);
    *pdir = dir;
    return Successful;
}
