/* THIS IS A GENERATED FILE
 *
 * Do not change!  Changing this file implies a protocol change!
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/X.h>
#include <X11/Xatom.h>
#include "misc.h"
#include "dix.h"
void
MakePredeclaredAtoms(void)
{
    if (MakeAtom("PRIMARY", 7, 1) != XA_PRIMARY)
        AtomError();
    if (MakeAtom("SECONDARY", 9, 1) != XA_SECONDARY)
        AtomError();
    if (MakeAtom("ARC", 3, 1) != XA_ARC)
        AtomError();
    if (MakeAtom("ATOM", 4, 1) != XA_ATOM)
        AtomError();
    if (MakeAtom("BITMAP", 6, 1) != XA_BITMAP)
        AtomError();
    if (MakeAtom("CARDINAL", 8, 1) != XA_CARDINAL)
        AtomError();
    if (MakeAtom("COLORMAP", 8, 1) != XA_COLORMAP)
        AtomError();
    if (MakeAtom("CURSOR", 6, 1) != XA_CURSOR)
        AtomError();
    if (MakeAtom("CUT_BUFFER0", 11, 1) != XA_CUT_BUFFER0)
        AtomError();
    if (MakeAtom("CUT_BUFFER1", 11, 1) != XA_CUT_BUFFER1)
        AtomError();
    if (MakeAtom("CUT_BUFFER2", 11, 1) != XA_CUT_BUFFER2)
        AtomError();
    if (MakeAtom("CUT_BUFFER3", 11, 1) != XA_CUT_BUFFER3)
        AtomError();
    if (MakeAtom("CUT_BUFFER4", 11, 1) != XA_CUT_BUFFER4)
        AtomError();
    if (MakeAtom("CUT_BUFFER5", 11, 1) != XA_CUT_BUFFER5)
        AtomError();
    if (MakeAtom("CUT_BUFFER6", 11, 1) != XA_CUT_BUFFER6)
        AtomError();
    if (MakeAtom("CUT_BUFFER7", 11, 1) != XA_CUT_BUFFER7)
        AtomError();
    if (MakeAtom("DRAWABLE", 8, 1) != XA_DRAWABLE)
        AtomError();
    if (MakeAtom("FONT", 4, 1) != XA_FONT)
        AtomError();
    if (MakeAtom("INTEGER", 7, 1) != XA_INTEGER)
        AtomError();
    if (MakeAtom("PIXMAP", 6, 1) != XA_PIXMAP)
        AtomError();
    if (MakeAtom("POINT", 5, 1) != XA_POINT)
        AtomError();
    if (MakeAtom("RECTANGLE", 9, 1) != XA_RECTANGLE)
        AtomError();
    if (MakeAtom("RESOURCE_MANAGER", 16, 1) != XA_RESOURCE_MANAGER)
        AtomError();
    if (MakeAtom("RGB_COLOR_MAP", 13, 1) != XA_RGB_COLOR_MAP)
        AtomError();
    if (MakeAtom("RGB_BEST_MAP", 12, 1) != XA_RGB_BEST_MAP)
        AtomError();
    if (MakeAtom("RGB_BLUE_MAP", 12, 1) != XA_RGB_BLUE_MAP)
        AtomError();
    if (MakeAtom("RGB_DEFAULT_MAP", 15, 1) != XA_RGB_DEFAULT_MAP)
        AtomError();
    if (MakeAtom("RGB_GRAY_MAP", 12, 1) != XA_RGB_GRAY_MAP)
        AtomError();
    if (MakeAtom("RGB_GREEN_MAP", 13, 1) != XA_RGB_GREEN_MAP)
        AtomError();
    if (MakeAtom("RGB_RED_MAP", 11, 1) != XA_RGB_RED_MAP)
        AtomError();
    if (MakeAtom("STRING", 6, 1) != XA_STRING)
        AtomError();
    if (MakeAtom("VISUALID", 8, 1) != XA_VISUALID)
        AtomError();
    if (MakeAtom("WINDOW", 6, 1) != XA_WINDOW)
        AtomError();
    if (MakeAtom("WM_COMMAND", 10, 1) != XA_WM_COMMAND)
        AtomError();
    if (MakeAtom("WM_HINTS", 8, 1) != XA_WM_HINTS)
        AtomError();
    if (MakeAtom("WM_CLIENT_MACHINE", 17, 1) != XA_WM_CLIENT_MACHINE)
        AtomError();
    if (MakeAtom("WM_ICON_NAME", 12, 1) != XA_WM_ICON_NAME)
        AtomError();
    if (MakeAtom("WM_ICON_SIZE", 12, 1) != XA_WM_ICON_SIZE)
        AtomError();
    if (MakeAtom("WM_NAME", 7, 1) != XA_WM_NAME)
        AtomError();
    if (MakeAtom("WM_NORMAL_HINTS", 15, 1) != XA_WM_NORMAL_HINTS)
        AtomError();
    if (MakeAtom("WM_SIZE_HINTS", 13, 1) != XA_WM_SIZE_HINTS)
        AtomError();
    if (MakeAtom("WM_ZOOM_HINTS", 13, 1) != XA_WM_ZOOM_HINTS)
        AtomError();
    if (MakeAtom("MIN_SPACE", 9, 1) != XA_MIN_SPACE)
        AtomError();
    if (MakeAtom("NORM_SPACE", 10, 1) != XA_NORM_SPACE)
        AtomError();
    if (MakeAtom("MAX_SPACE", 9, 1) != XA_MAX_SPACE)
        AtomError();
    if (MakeAtom("END_SPACE", 9, 1) != XA_END_SPACE)
        AtomError();
    if (MakeAtom("SUPERSCRIPT_X", 13, 1) != XA_SUPERSCRIPT_X)
        AtomError();
    if (MakeAtom("SUPERSCRIPT_Y", 13, 1) != XA_SUPERSCRIPT_Y)
        AtomError();
    if (MakeAtom("SUBSCRIPT_X", 11, 1) != XA_SUBSCRIPT_X)
        AtomError();
    if (MakeAtom("SUBSCRIPT_Y", 11, 1) != XA_SUBSCRIPT_Y)
        AtomError();
    if (MakeAtom("UNDERLINE_POSITION", 18, 1) != XA_UNDERLINE_POSITION)
        AtomError();
    if (MakeAtom("UNDERLINE_THICKNESS", 19, 1) != XA_UNDERLINE_THICKNESS)
        AtomError();
    if (MakeAtom("STRIKEOUT_ASCENT", 16, 1) != XA_STRIKEOUT_ASCENT)
        AtomError();
    if (MakeAtom("STRIKEOUT_DESCENT", 17, 1) != XA_STRIKEOUT_DESCENT)
        AtomError();
    if (MakeAtom("ITALIC_ANGLE", 12, 1) != XA_ITALIC_ANGLE)
        AtomError();
    if (MakeAtom("X_HEIGHT", 8, 1) != XA_X_HEIGHT)
        AtomError();
    if (MakeAtom("QUAD_WIDTH", 10, 1) != XA_QUAD_WIDTH)
        AtomError();
    if (MakeAtom("WEIGHT", 6, 1) != XA_WEIGHT)
        AtomError();
    if (MakeAtom("POINT_SIZE", 10, 1) != XA_POINT_SIZE)
        AtomError();
    if (MakeAtom("RESOLUTION", 10, 1) != XA_RESOLUTION)
        AtomError();
    if (MakeAtom("COPYRIGHT", 9, 1) != XA_COPYRIGHT)
        AtomError();
    if (MakeAtom("NOTICE", 6, 1) != XA_NOTICE)
        AtomError();
    if (MakeAtom("FONT_NAME", 9, 1) != XA_FONT_NAME)
        AtomError();
    if (MakeAtom("FAMILY_NAME", 11, 1) != XA_FAMILY_NAME)
        AtomError();
    if (MakeAtom("FULL_NAME", 9, 1) != XA_FULL_NAME)
        AtomError();
    if (MakeAtom("CAP_HEIGHT", 10, 1) != XA_CAP_HEIGHT)
        AtomError();
    if (MakeAtom("WM_CLASS", 8, 1) != XA_WM_CLASS)
        AtomError();
    if (MakeAtom("WM_TRANSIENT_FOR", 16, 1) != XA_WM_TRANSIENT_FOR)
        AtomError();
}
