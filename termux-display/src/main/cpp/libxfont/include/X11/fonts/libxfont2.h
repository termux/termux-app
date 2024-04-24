/*
 * Copyright © 2015 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#ifndef _LIBXFONT2_H_
#define _LIBXFONT2_H_

#include	<stdarg.h>
#include	<stdint.h>
#include	<X11/Xfuncproto.h>
#include	<X11/fonts/font.h>

#define XFONT2_FPE_FUNCS_VERSION	1

typedef int (*WakeupFpe) (FontPathElementPtr fpe);

typedef struct _xfont2_fpe_funcs {
	int		version;
	NameCheckFunc	name_check;
	InitFpeFunc	init_fpe;
	FreeFpeFunc	free_fpe;
	ResetFpeFunc	reset_fpe;
	OpenFontFunc	open_font;
	CloseFontFunc	close_font;
	ListFontsFunc	list_fonts;
	StartLfwiFunc	start_list_fonts_with_info;
	NextLfwiFunc	list_next_font_with_info;
	WakeupFpe	wakeup_fpe;
	ClientDiedFunc	client_died;
	LoadGlyphsFunc	load_glyphs;
	StartLaFunc	start_list_fonts_and_aliases;
	NextLaFunc	list_next_font_or_alias;
	SetPathFunc	set_path_hook;
} xfont2_fpe_funcs_rec, *xfont2_fpe_funcs_ptr;

typedef void (*FontBlockHandlerProcPtr) (void *timeout);

typedef void (*FontFdHandlerProcPtr) (int fd, void *data);

#define XFONT2_CLIENT_FUNCS_VERSION	1

typedef struct _xfont2_client_funcs {
	int			version;
	int			(*client_auth_generation)(ClientPtr client);
	Bool			(*client_signal)(ClientPtr client);
	void			(*delete_font_client_id)(Font id);
	void			(*verrorf)(const char *f, va_list ap) _X_ATTRIBUTE_PRINTF(1,0);
	FontPtr			(*find_old_font)(FSID id);
	FontResolutionPtr	(*get_client_resolutions)(int *num);
	int			(*get_default_point_size)(void);
	Font			(*get_new_font_client_id)(void);
	uint32_t		(*get_time_in_millis)(void);
	int			(*init_fs_handlers)(FontPathElementPtr fpe,
						    FontBlockHandlerProcPtr block_handler);
	int			(*register_fpe_funcs)(const xfont2_fpe_funcs_rec *funcs);
	void			(*remove_fs_handlers)(FontPathElementPtr fpe,
						      FontBlockHandlerProcPtr block_handler,
						      Bool all );
	void			*(*get_server_client)(void);
	int			(*set_font_authorizations)(char **authorizations,
							   int *authlen, void *client);
	int			(*store_font_client_font)(FontPtr pfont, Font id);
	Atom			(*make_atom)(const char *string, unsigned len, int makeit);
	int			(*valid_atom)(Atom atom);
	const char		*(*name_for_atom)(Atom atom);
	unsigned long		(*get_server_generation)(void);
	int			(*add_fs_fd)(int fd, FontFdHandlerProcPtr handler, void *data);
	void			(*remove_fs_fd)(int fd);
	void			(*adjust_fs_wait_for_delay)(void *wt, unsigned long newdelay);
} xfont2_client_funcs_rec, *xfont2_client_funcs_ptr;

_X_EXPORT int
xfont2_init(xfont2_client_funcs_rec const *client_funcs);

_X_EXPORT void
xfont2_query_glyph_extents(FontPtr pFont, CharInfoPtr *charinfo,
			   unsigned long count, ExtentInfoRec *info);

_X_EXPORT Bool
xfont2_query_text_extents(FontPtr pFont, unsigned long count,
			  unsigned char *chars, ExtentInfoRec *info);

_X_EXPORT Bool
xfont2_parse_glyph_caching_mode(char *str);

_X_EXPORT void
xfont2_init_glyph_caching(void);

_X_EXPORT void
xfont2_set_glyph_caching_mode(int newmode);

_X_EXPORT FontNamesPtr
xfont2_make_font_names_record(unsigned size);

_X_EXPORT void
xfont2_free_font_names(FontNamesPtr pFN);

_X_EXPORT int
xfont2_add_font_names_name(FontNamesPtr names,
			   char *name,
			   int length);

typedef struct _xfont2_pattern_cache    *xfont2_pattern_cache_ptr;

_X_EXPORT xfont2_pattern_cache_ptr
xfont2_make_font_pattern_cache(void);

_X_EXPORT void
xfont2_free_font_pattern_cache(xfont2_pattern_cache_ptr cache);

_X_EXPORT void
xfont2_empty_font_pattern_cache(xfont2_pattern_cache_ptr cache);

_X_EXPORT void
xfont2_cache_font_pattern(xfont2_pattern_cache_ptr cache,
			  const char * pattern,
			  int patlen,
			  FontPtr pFont);

_X_EXPORT FontPtr
xfont2_find_cached_font_pattern(xfont2_pattern_cache_ptr cache,
				const char * pattern,
				int patlen);

_X_EXPORT void
xfont2_remove_cached_font_pattern(xfont2_pattern_cache_ptr cache,
				  FontPtr pFont);

/* private.c */

_X_EXPORT int
xfont2_allocate_font_private_index (void);

static inline void *
xfont2_font_get_private(FontPtr pFont, int n)
{
	if (n > pFont->maxPrivate)
		return NULL;
	return pFont->devPrivates[n];
}

_X_EXPORT Bool
xfont2_font_set_private(FontPtr pFont, int n, void *ptr);

#endif /* _LIBXFONT2_H_ */
