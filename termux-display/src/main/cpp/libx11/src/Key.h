
#ifndef _KEY_H_
#define _KEY_H_

#include <X11/Xlib.h>
#include <X11/Xresource.h>

#ifndef NEEDKTABLE
extern const unsigned char _XkeyTable[];
#endif

extern int
_XKeyInitialize(
    Display *dpy);

extern XrmDatabase
_XInitKeysymDB(
        void);

#endif /* _KEY_H_ */
