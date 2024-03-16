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

#ifndef XNESTSCREEN_H
#define XNESTSCREEN_H

extern Window xnestDefaultWindows[MAXSCREENS];
extern Window xnestScreenSaverWindows[MAXSCREENS];

ScreenPtr xnestScreen(Window window);
Bool xnestOpenScreen(ScreenPtr pScreen, int argc, char *argv[]);
Bool xnestCloseScreen(ScreenPtr pScreen);

#endif                          /* XNESTSCREEN_H */
