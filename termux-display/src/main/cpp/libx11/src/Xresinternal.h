
#ifndef _XRESINTERNAL_H_
#define _XRESINTERNAL_H_

#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <inttypes.h>

/* type defines */
typedef uint32_t Signature;

/* prototypes */
extern XrmQuark _XrmInternalStringToQuark(
    register _Xconst char *name, register int len, register Signature sig,
    Bool permstring);

#endif /* _XRESOURCEINTERNAL_H_ */
/* DON'T ADD STUFF AFTER THIS #endif */
