/*

Copyright 1993 by Davor Matic

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation.  Davor Matic makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

*/

#ifdef HAVE_XNEST_CONFIG_H
#include <xnest-config.h>
#endif

#include <X11/X.h>
#include <X11/Xproto.h>
#include "screenint.h"
#include "input.h"
#include "misc.h"
#include "scrnintstr.h"
#include "servermd.h"

#include "Xnest.h"

#include "Display.h"
#include "Args.h"

char *xnestDisplayName = NULL;
Bool xnestSynchronize = False;
Bool xnestFullGeneration = False;
int xnestDefaultClass;
Bool xnestUserDefaultClass = False;
int xnestDefaultDepth;
Bool xnestUserDefaultDepth = False;
Bool xnestSoftwareScreenSaver = False;
int xnestX;
int xnestY;
unsigned int xnestWidth;
unsigned int xnestHeight;
int xnestUserGeometry = 0;
int xnestBorderWidth;
Bool xnestUserBorderWidth = False;
char *xnestWindowName = NULL;
int xnestNumScreens = 0;
Bool xnestDoDirectColormaps = False;
Window xnestParentWindow = 0;

int
ddxProcessArgument(int argc, char *argv[], int i)
{
    if (!strcmp(argv[i], "-display")) {
        if (++i < argc) {
            xnestDisplayName = argv[i];
            return 2;
        }
        return 0;
    }
    if (!strcmp(argv[i], "-sync")) {
        xnestSynchronize = True;
        return 1;
    }
    if (!strcmp(argv[i], "-full")) {
        xnestFullGeneration = True;
        return 1;
    }
    if (!strcmp(argv[i], "-class")) {
        if (++i < argc) {
            if (!strcmp(argv[i], "StaticGray")) {
                xnestDefaultClass = StaticGray;
                xnestUserDefaultClass = True;
                return 2;
            }
            else if (!strcmp(argv[i], "GrayScale")) {
                xnestDefaultClass = GrayScale;
                xnestUserDefaultClass = True;
                return 2;
            }
            else if (!strcmp(argv[i], "StaticColor")) {
                xnestDefaultClass = StaticColor;
                xnestUserDefaultClass = True;
                return 2;
            }
            else if (!strcmp(argv[i], "PseudoColor")) {
                xnestDefaultClass = PseudoColor;
                xnestUserDefaultClass = True;
                return 2;
            }
            else if (!strcmp(argv[i], "TrueColor")) {
                xnestDefaultClass = TrueColor;
                xnestUserDefaultClass = True;
                return 2;
            }
            else if (!strcmp(argv[i], "DirectColor")) {
                xnestDefaultClass = DirectColor;
                xnestUserDefaultClass = True;
                return 2;
            }
        }
        return 0;
    }
    if (!strcmp(argv[i], "-cc")) {
        if (++i < argc && sscanf(argv[i], "%i", &xnestDefaultClass) == 1) {
            if (xnestDefaultClass >= 0 && xnestDefaultClass <= 5) {
                xnestUserDefaultClass = True;
                /* lex the OS layer process it as well, so return 0 */
            }
        }
        return 0;
    }
    if (!strcmp(argv[i], "-depth")) {
        if (++i < argc && sscanf(argv[i], "%i", &xnestDefaultDepth) == 1) {
            if (xnestDefaultDepth > 0) {
                xnestUserDefaultDepth = True;
                return 2;
            }
        }
        return 0;
    }
    if (!strcmp(argv[i], "-sss")) {
        xnestSoftwareScreenSaver = True;
        return 1;
    }
    if (!strcmp(argv[i], "-geometry")) {
        if (++i < argc) {
            xnestUserGeometry = XParseGeometry(argv[i],
                                               &xnestX, &xnestY,
                                               &xnestWidth, &xnestHeight);
            if (xnestUserGeometry)
                return 2;
        }
        return 0;
    }
    if (!strcmp(argv[i], "-bw")) {
        if (++i < argc && sscanf(argv[i], "%i", &xnestBorderWidth) == 1) {
            if (xnestBorderWidth >= 0) {
                xnestUserBorderWidth = True;
                return 2;
            }
        }
        return 0;
    }
    if (!strcmp(argv[i], "-name")) {
        if (++i < argc) {
            xnestWindowName = argv[i];
            return 2;
        }
        return 0;
    }
    if (!strcmp(argv[i], "-scrns")) {
        if (++i < argc && sscanf(argv[i], "%i", &xnestNumScreens) == 1) {
            if (xnestNumScreens > 0) {
                if (xnestNumScreens > MAXSCREENS) {
                    ErrorF("Maximum number of screens is %d.\n", MAXSCREENS);
                    xnestNumScreens = MAXSCREENS;
                }
                return 2;
            }
        }
        return 0;
    }
    if (!strcmp(argv[i], "-install")) {
        xnestDoDirectColormaps = True;
        return 1;
    }
    if (!strcmp(argv[i], "-parent")) {
        if (++i < argc) {
            xnestParentWindow = (XID) strtol(argv[i], (char **) NULL, 0);
            return 2;
        }
    }
    return 0;
}

void
ddxUseMsg(void)
{
    ErrorF("-display string        display name of the real server\n");
    ErrorF("-sync                  sinchronize with the real server\n");
    ErrorF("-full                  utilize full regeneration\n");
    ErrorF("-class string          default visual class\n");
    ErrorF("-depth int             default depth\n");
    ErrorF("-sss                   use software screen saver\n");
    ErrorF("-geometry WxH+X+Y      window size and position\n");
    ErrorF("-bw int                window border width\n");
    ErrorF("-name string           window name\n");
    ErrorF("-scrns int             number of screens to generate\n");
    ErrorF("-install               install colormaps directly\n");
}
