#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include	"libxfontint.h"

static xfont2_client_funcs_rec const *_f;

int
client_auth_generation(ClientPtr client)
{
    if (_f)
	return _f->client_auth_generation(client);
    return 0;
}

Bool
ClientSignal(ClientPtr client)
{
    if (_f)
	return _f->client_signal(client);
    return TRUE;
}

void
DeleteFontClientID(Font id)
{
    if (_f)
	_f->delete_font_client_id(id);
}

void
ErrorF(const char *f, ...)
{
    if (_f) {
	va_list	ap;
	va_start(ap, f);
	_f->verrorf(f, ap);
	va_end(ap);
    }
}

FontPtr
find_old_font(FSID id)
{
    if (_f)
	return _f->find_old_font(id);
    return (FontPtr)NULL;
}

FontResolutionPtr
GetClientResolutions(int *num)
{
    if (_f)
	return _f->get_client_resolutions(num);
    return (FontResolutionPtr) 0;
}

int
GetDefaultPointSize(void)
{
    if (_f)
	return _f->get_default_point_size();
    return 12;
}

Font
GetNewFontClientID(void)
{
    if (_f)
	return _f->get_new_font_client_id();
    return (Font)0;
}

unsigned long
GetTimeInMillis (void)
{
    if (_f)
	return _f->get_time_in_millis();
    return 0;
}

int
init_fs_handlers2(FontPathElementPtr fpe,
                 FontBlockHandlerProcPtr block_handler)
{
    if (_f)
	return _f->init_fs_handlers(fpe, block_handler);
    return Successful;
}

int
register_fpe_funcs(const xfont2_fpe_funcs_rec *funcs)
{
    if (_f)
	return _f->register_fpe_funcs(funcs);
    return 0;
}

void
remove_fs_handlers2(FontPathElementPtr fpe,
		    FontBlockHandlerProcPtr blockHandler,
		    Bool all)
{
    if (_f)
	_f->remove_fs_handlers(fpe, blockHandler, all);
}

void *
__GetServerClient(void)
{
    if (_f)
	return _f->get_server_client();
    return NULL;
}

int
set_font_authorizations(char **authorizations, int *authlen, ClientPtr client)
{
    if (_f)
	return _f->set_font_authorizations(authorizations, authlen, client);
    return 0;
}

int
StoreFontClientFont(FontPtr pfont, Font id)
{
    if (_f)
	return _f->store_font_client_font(pfont, id);
    return 0;
}

Atom
MakeAtom(const char *string, unsigned len, int makeit)
{
    if (_f && _f->make_atom)
	return _f->make_atom(string, len, makeit);
    return __libxfont_internal__MakeAtom(string, len, makeit);
}

int
ValidAtom(Atom atom)
{
    if (_f && _f->valid_atom)
	return _f->valid_atom(atom);
    return __libxfont_internal__ValidAtom(atom);
}

const char *
NameForAtom(Atom atom)
{
    if (_f && _f->name_for_atom)
	return _f->name_for_atom(atom);
    return __libxfont_internal__NameForAtom(atom);
}

unsigned long
__GetServerGeneration (void)
{
    if (_f)
	return _f->get_server_generation();
    return 1;
}


int
add_fs_fd(int fd, FontFdHandlerProcPtr handler, void *data)
{
    if (_f)
	return _f->add_fs_fd(fd, handler, data);
    return 0;
}

void
remove_fs_fd(int fd)
{
    if (_f)
	_f->remove_fs_fd(fd);
}

void
adjust_fs_wait_for_delay(void *wt, unsigned long newdelay)
{
    if (_f)
	_f->adjust_fs_wait_for_delay(wt, newdelay);
}

int
xfont2_init(xfont2_client_funcs_rec const *client_funcs)
{
    _f = client_funcs;

    ResetFontPrivateIndex();

    register_fpe_functions();

    return Successful;
}
