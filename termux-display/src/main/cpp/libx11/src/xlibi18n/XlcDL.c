/*
Copyright 1985, 1986, 1987, 1991, 1998  The Open Group

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions: The above copyright notice and this
permission notice shall be included in all copies or substantial
portions of the Software.


THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE
EVEN IF ADVISED IN ADVANCE OF THE POSSIBILITY OF SUCH DAMAGES.


Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.


X Window System is a trademark of The Open Group

OSF/1, OSF/Motif and Motif are registered trademarks, and OSF, the OSF
logo, LBX, X Window System, and Xinerama are trademarks of the Open
Group. All other trademarks and registered trademarks mentioned herein
are the property of their respective owners. No right, title or
interest in or to any trademark, service mark, logo or trade name of
Sun Microsystems, Inc. or its licensors is granted.

*/
/*
 * Copyright (c) 2000, Oracle and/or its affiliates.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifdef HAVE_CONFIG_H
# include <config.h>
#else
# if defined(hpux)
#  define HAVE_DL_H
# else
#  define HAVE_DLFCN_H
# endif
#endif

#include <stdio.h>

#ifdef HAVE_DL_H
#include <dl.h>
#endif

#ifdef HAVE_DLFCN_H
#include <dlfcn.h>
#endif

#include <ctype.h>

#include "Xlibint.h"
#include "XlcPublic.h"
#include "XlcPubI.h"
#include "reallocarray.h"

#define XI18N_DLREL		2

#define	iscomment(ch)	((ch) == '\0' || (ch) == '#')

typedef enum {
  XLC_OBJECT,
  XIM_OBJECT,
  XOM_OBJECT
} XI18NDLType;

typedef struct {
  XI18NDLType type;
  int	locale_name_len;
  char *locale_name;
  char *dl_name;
  char *open;
  char *im_register;
  char *im_unregister;
  int dl_release;
  unsigned int refcount;
#if defined(hpux)
  shl_t dl_module;
#else
  void *dl_module;
#endif
} XI18NObjectsListRec, *XI18NObjectsList;

#define OBJECT_INIT_LEN 8
#define OBJECT_INC_LEN 4
static int lc_len = 0;
static XI18NObjectsListRec *xi18n_objects_list = NULL;
static int lc_count = 0;

static int
parse_line(char *line, char **argv, int argsize)
{
    int argc = 0;
    char *p = line;

    while (argc < argsize) {
	while (isspace(*p)) {
	    ++p;
	}
	if (iscomment(*p)){
	    break;
	}
	argv[argc++] = p;
	while (!isspace(*p)) {
	    ++p;
	}
	if (iscomment(*p)) {
	    break;
	}
	*p++ = '\0';
    }
    return argc;
}

static char *
strdup_with_underscore(const char *symbol)
{
	char *result;

	if ((result = malloc(strlen(symbol) + 2)) == NULL)
		return NULL;
	result[0] = '_';
	strcpy(result + 1, symbol);
	return result;
}

#ifndef hpux
static void *
try_both_dlsym (void *handle, char *name)
{
    void    *ret;

    ret = dlsym (handle, name);
    if (!ret)
    {
	 name = strdup_with_underscore (name);
	 if (name)
	 {
	     ret = dlsym (handle, name);
	     free (name);
	 }
    }
    return ret;
}
#endif

static void
resolve_object(char *path, const char *lc_name)
{
    char filename[BUFSIZ];
    FILE *fp;
    char buf[BUFSIZ];

    if (lc_len == 0) { /* True only for the 1st time */
      lc_len = OBJECT_INIT_LEN;
      xi18n_objects_list = Xmallocarray(lc_len, sizeof(XI18NObjectsListRec));
      if (!xi18n_objects_list) return;
    }
    snprintf(filename, sizeof(filename), "%s/%s", path, "XI18N_OBJS");
    fp = fopen(filename, "r");
    if (fp == (FILE *)NULL){
	return;
    }

    while (fgets(buf, BUFSIZ, fp) != NULL){
	char *p = buf;
	int n;
	char *args[6];
	while (isspace(*p)){
	    ++p;
	}
	if (iscomment(*p)){
	    continue;
	}

	if (lc_count == lc_len) {
	  int new_len = lc_len + OBJECT_INC_LEN;
	  XI18NObjectsListRec *tmp =
              Xreallocarray(xi18n_objects_list, new_len,
                            sizeof(XI18NObjectsListRec));
	  if (tmp == NULL)
	      goto done;
	  xi18n_objects_list = tmp;
	  lc_len = new_len;
	}
	n = parse_line(p, args, 6);

	if (n == 3 || n == 5) {
	  if (!strcmp(args[0], "XLC")){
	    xi18n_objects_list[lc_count].type = XLC_OBJECT;
	  } else if (!strcmp(args[0], "XOM")){
	    xi18n_objects_list[lc_count].type = XOM_OBJECT;
	  } else if (!strcmp(args[0], "XIM")){
	    xi18n_objects_list[lc_count].type = XIM_OBJECT;
	  }
	  xi18n_objects_list[lc_count].dl_name = strdup(args[1]);
	  xi18n_objects_list[lc_count].open = strdup(args[2]);
	  xi18n_objects_list[lc_count].dl_release = XI18N_DLREL;
	  xi18n_objects_list[lc_count].locale_name = strdup(lc_name);
	  xi18n_objects_list[lc_count].refcount = 0;
	  xi18n_objects_list[lc_count].dl_module = (void*)NULL;
	  if (n == 5) {
	    xi18n_objects_list[lc_count].im_register = strdup(args[3]);
	    xi18n_objects_list[lc_count].im_unregister = strdup(args[4]);
	  } else {
	    xi18n_objects_list[lc_count].im_register = NULL;
	    xi18n_objects_list[lc_count].im_unregister = NULL;
	  }
	  lc_count++;
	}
    }
  done:
    fclose(fp);
}

static char*
__lc_path(const char *dl_name, const char *lc_dir)
{
    char *path;
    size_t len;
    char *slash_p;

    /*
     * reject this for possible security issue
     */
    if (strstr (dl_name, "../"))
	return NULL;

    len = (lc_dir ? strlen(lc_dir) : 0 ) +
	(dl_name ? strlen(dl_name) : 0) + 10;
#if defined POSTLOCALELIBDIR
    len += (strlen(POSTLOCALELIBDIR) + 1);
#endif
    path = Xmalloc(len + 1);

    if (strchr(dl_name, '/') != NULL) {
	slash_p = strrchr(lc_dir, '/');
	*slash_p = '\0';
    } else
	slash_p = NULL;

#if defined POSTLOCALELIBDIR
    snprintf(path, len + 1, "%s/%s/%s.so.2",
             lc_dir, POSTLOCALELIBDIR, dl_name);
#else
    snprintf(path, len + 1, "%s/%s.so.2", lc_dir, dl_name);
#endif

    if (slash_p != NULL)
	*slash_p = '/';

    return path;
}

/* We reference count dlopen() and dlclose() of modules; unfortunately,
 * since XCloseIM, XCloseOM, XlcClose aren't wrapped, but directly
 * call the close method of the object, we leak a reference count every
 * time we open then close a module. Fixing this would require
 * either creating proxy objects or hooks for close_im/close_om
 * in XLCd
 */
static Bool
open_object(
     XI18NObjectsList object,
     char *lc_dir)
{
  char *path;

  if (object->refcount == 0) {
      path = __lc_path(object->dl_name, lc_dir);
      if (!path)
	  return False;
#if defined(hpux)
      object->dl_module = shl_load(path, BIND_DEFERRED, 0L);
#else
      object->dl_module = dlopen(path, RTLD_LAZY);
#endif
      Xfree(path);

      if (!object->dl_module)
	  return False;
    }

  object->refcount++;
  return True;
}

static void *
fetch_symbol(
     XI18NObjectsList object,
     char *symbol)
{
    void *result = NULL;
#if defined(hpux)
    int getsyms_cnt, i;
    struct shl_symbol *symbols;
#endif

    if (symbol == NULL)
    	return NULL;

#if defined(hpux)
    getsyms_cnt = shl_getsymbols(object->dl_module, TYPE_PROCEDURE,
				 EXPORT_SYMBOLS, malloc, &symbols);

    for(i=0; i<getsyms_cnt; i++) {
        if(!strcmp(symbols[i].name, symbol)) {
	    result = symbols[i].value;
	    break;
         }
    }

    if(getsyms_cnt > 0) {
        free(symbols);
    }
#else
    result = try_both_dlsym(object->dl_module, symbol);
#endif

    return result;
}

static void
close_object(XI18NObjectsList object)
{
  object->refcount--;
  if (object->refcount == 0)
    {
#if defined(hpux)
        shl_unload(object->dl_module);
#else
        dlclose(object->dl_module);
#endif
        object->dl_module = NULL;
    }
}


typedef XLCd (*dynamicLoadProc)(const char *);

XLCd
_XlcDynamicLoad(const char *lc_name)
{
    XLCd lcd = (XLCd)NULL;
    dynamicLoadProc lc_loader = (dynamicLoadProc)NULL;
    int count;
    XI18NObjectsList objects_list;
    char lc_dir[BUFSIZE], lc_lib_dir[BUFSIZE];

    if (lc_name == NULL) return (XLCd)NULL;

    if (_XlcLocaleDirName(lc_dir, BUFSIZE, lc_name) == NULL)
        return (XLCd)NULL;
    if (_XlcLocaleLibDirName(lc_lib_dir, BUFSIZE, lc_name) == NULL)
	return (XLCd)NULL;

    resolve_object(lc_dir, lc_name);
    resolve_object(lc_lib_dir, lc_name);

    objects_list = xi18n_objects_list;
    count = lc_count;
    for (; count-- > 0; objects_list++) {
        if (objects_list->type != XLC_OBJECT ||
	    strcmp(objects_list->locale_name, lc_name)) continue;
	if (!open_object (objects_list, lc_dir) && \
            !open_object (objects_list, lc_lib_dir))
	    continue;

	lc_loader = (dynamicLoadProc)fetch_symbol (objects_list, objects_list->open);
	if (!lc_loader) continue;
	lcd = (*lc_loader)(lc_name);
	if (lcd != (XLCd)NULL) {
	    break;
	}

	close_object (objects_list);
    }
    return (XLCd)lcd;
}


typedef XIM (*dynamicOpenProcp)(XLCd, Display *, XrmDatabase, char *, char *);

static XIM
_XDynamicOpenIM(XLCd lcd, Display *display, XrmDatabase rdb,
		char *res_name, char *res_class)
{
  XIM im = (XIM)NULL;
  char lc_dir[BUFSIZE];
  char *lc_name;
  dynamicOpenProcp im_openIM = (dynamicOpenProcp)NULL;
  int count;
  XI18NObjectsList objects_list = xi18n_objects_list;

  lc_name = lcd->core->name;

  if (_XlcLocaleLibDirName(lc_dir, BUFSIZE, lc_name) == NULL) return (XIM)0;

  count = lc_count;
  for (; count-- > 0; objects_list++) {
    if (objects_list->type != XIM_OBJECT ||
	strcmp(objects_list->locale_name, lc_name)) continue;

    if (!open_object (objects_list, lc_dir))
        continue;

    im_openIM = (dynamicOpenProcp)fetch_symbol(objects_list, objects_list->open);
    if (!im_openIM) continue;
    im = (*im_openIM)(lcd, display, rdb, res_name, res_class);
    if (im != (XIM)NULL) {
        break;
    }

    close_object (objects_list);
  }
  return (XIM)im;
}

typedef Bool (*dynamicRegisterCBProcp)(
    XLCd, Display *, XrmDatabase, char *, char *, XIDProc, XPointer);

static Bool
_XDynamicRegisterIMInstantiateCallback(
    XLCd	 lcd,
    Display	*display,
    XrmDatabase	 rdb,
    char	*res_name,
    char        *res_class,
    XIDProc	 callback,
    XPointer	 client_data)
{
  char lc_dir[BUFSIZE];
  char *lc_name;
  dynamicRegisterCBProcp im_registerIM = (dynamicRegisterCBProcp)NULL;
  Bool ret_flag = False;
  int count;
  XI18NObjectsList objects_list = xi18n_objects_list;
#if defined(hpux)
  int getsyms_cnt, i;
  struct shl_symbol *symbols;
#endif

  lc_name = lcd->core->name;

  if (_XlcLocaleLibDirName(lc_dir, BUFSIZE, lc_name) == NULL) return False;

  count = lc_count;
  for (; count-- > 0; objects_list++) {
    if (objects_list->type != XIM_OBJECT ||
	strcmp(objects_list->locale_name, lc_name)) continue;

    if (!open_object (objects_list, lc_dir))
        continue;
    im_registerIM = (dynamicRegisterCBProcp)fetch_symbol(objects_list,
					    objects_list->im_register);
    if (!im_registerIM) continue;
    ret_flag = (*im_registerIM)(lcd, display, rdb,
				res_name, res_class,
				callback, client_data);
    if (ret_flag) break;

    close_object (objects_list);
  }
  return (Bool)ret_flag;
}

typedef Bool (*dynamicUnregisterProcp)(
    XLCd, Display *, XrmDatabase, char *, char *, XIDProc, XPointer);

static Bool
_XDynamicUnRegisterIMInstantiateCallback(
    XLCd	 lcd,
    Display	*display,
    XrmDatabase	 rdb,
    char	*res_name,
    char        *res_class,
    XIDProc	 callback,
    XPointer	 client_data)
{
  char lc_dir[BUFSIZE];
  const char *lc_name;
  dynamicUnregisterProcp im_unregisterIM = (dynamicUnregisterProcp)NULL;
  Bool ret_flag = False;
  int count;
  XI18NObjectsList objects_list = xi18n_objects_list;
#if defined(hpux)
  int getsyms_cnt, i;
  struct shl_symbol *symbols;
#endif

  lc_name = lcd->core->name;
  if (_XlcLocaleDirName(lc_dir, BUFSIZE, lc_name) == NULL) return False;

  count = lc_count;
  for (; count-- > 0; objects_list++) {
    if (objects_list->type != XIM_OBJECT ||
	strcmp(objects_list->locale_name, lc_name)) continue;

    if (!objects_list->refcount) /* Must already be opened */
        continue;

    im_unregisterIM = (dynamicUnregisterProcp)fetch_symbol(objects_list,
					      objects_list->im_unregister);

    if (!im_unregisterIM) continue;
    ret_flag = (*im_unregisterIM)(lcd, display, rdb,
				  res_name, res_class,
				  callback, client_data);
    if (ret_flag) {
        close_object (objects_list); /* opened in RegisterIMInstantiateCallback */
	break;
    }
  }
  return (Bool)ret_flag;
}

Bool
_XInitDynamicIM(XLCd lcd)
{
    if(lcd == (XLCd)NULL)
	return False;
    lcd->methods->open_im = _XDynamicOpenIM;
    lcd->methods->register_callback = _XDynamicRegisterIMInstantiateCallback;
    lcd->methods->unregister_callback = _XDynamicUnRegisterIMInstantiateCallback;
    return True;
}


typedef XOM (*dynamicIOpenProcp)(
        XLCd, Display *, XrmDatabase, _Xconst char *, _Xconst char *);

static XOM
_XDynamicOpenOM(XLCd lcd, Display *display, XrmDatabase rdb,
		_Xconst char *res_name, _Xconst char *res_class)
{
  XOM om = (XOM)NULL;
  int count;
  char lc_dir[BUFSIZE];
  char *lc_name;
  dynamicIOpenProcp om_openOM = (dynamicIOpenProcp)NULL;
  XI18NObjectsList objects_list = xi18n_objects_list;
#if defined(hpux)
  int getsyms_cnt, i;
  struct shl_symbol *symbols;
#endif

  lc_name = lcd->core->name;

  if (_XlcLocaleLibDirName(lc_dir, BUFSIZE, lc_name) == NULL) return (XOM)0;

  count = lc_count;
  for (; count-- > 0; objects_list++) {
    if (objects_list->type != XOM_OBJECT ||
	strcmp(objects_list->locale_name, lc_name)) continue;
    if (!open_object (objects_list, lc_dir))
        continue;

    om_openOM = (dynamicIOpenProcp)fetch_symbol(objects_list, objects_list->open);
    if (!om_openOM) continue;
    om = (*om_openOM)(lcd, display, rdb, res_name, res_class);
    if (om != (XOM)NULL) {
        break;
    }
    close_object(objects_list);
  }
  return (XOM)om;
}

Bool
_XInitDynamicOM(XLCd lcd)
{
    if(lcd == (XLCd)NULL)
	return False;

    lcd->methods->open_om = _XDynamicOpenOM;

    return True;
}
