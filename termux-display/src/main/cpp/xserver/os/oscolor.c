/***********************************************************

Copyright 1987, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Digital not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <X11/keysym.h>
#include "dix.h"
#include "os.h"

typedef struct _builtinColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned short name;
} BuiltinColor;

static const char BuiltinColorNames[] = {
    "alice blue\0"
        "AliceBlue\0"
        "antique white\0"
        "AntiqueWhite\0"
        "AntiqueWhite1\0"
        "AntiqueWhite2\0"
        "AntiqueWhite3\0"
        "AntiqueWhite4\0"
        "aqua\0"
        "aquamarine\0"
        "aquamarine1\0"
        "aquamarine2\0"
        "aquamarine3\0"
        "aquamarine4\0"
        "azure\0"
        "azure1\0"
        "azure2\0"
        "azure3\0"
        "azure4\0"
        "beige\0"
        "bisque\0"
        "bisque1\0"
        "bisque2\0"
        "bisque3\0"
        "bisque4\0"
        "black\0"
        "blanched almond\0"
        "BlanchedAlmond\0"
        "blue\0"
        "blue violet\0"
        "blue1\0"
        "blue2\0"
        "blue3\0"
        "blue4\0"
        "BlueViolet\0"
        "brown\0"
        "brown1\0"
        "brown2\0"
        "brown3\0"
        "brown4\0"
        "burlywood\0"
        "burlywood1\0"
        "burlywood2\0"
        "burlywood3\0"
        "burlywood4\0"
        "cadet blue\0"
        "CadetBlue\0"
        "CadetBlue1\0"
        "CadetBlue2\0"
        "CadetBlue3\0"
        "CadetBlue4\0"
        "chartreuse\0"
        "chartreuse1\0"
        "chartreuse2\0"
        "chartreuse3\0"
        "chartreuse4\0"
        "chocolate\0"
        "chocolate1\0"
        "chocolate2\0"
        "chocolate3\0"
        "chocolate4\0"
        "coral\0"
        "coral1\0"
        "coral2\0"
        "coral3\0"
        "coral4\0"
        "cornflower blue\0"
        "CornflowerBlue\0"
        "cornsilk\0"
        "cornsilk1\0"
        "cornsilk2\0"
        "cornsilk3\0"
        "cornsilk4\0"
        "crimson\0"
        "cyan\0"
        "cyan1\0"
        "cyan2\0"
        "cyan3\0"
        "cyan4\0"
        "dark blue\0"
        "dark cyan\0"
        "dark goldenrod\0"
        "dark gray\0"
        "dark green\0"
        "dark grey\0"
        "dark khaki\0"
        "dark magenta\0"
        "dark olive green\0"
        "dark orange\0"
        "dark orchid\0"
        "dark red\0"
        "dark salmon\0"
        "dark sea green\0"
        "dark slate blue\0"
        "dark slate gray\0"
        "dark slate grey\0"
        "dark turquoise\0"
        "dark violet\0"
        "DarkBlue\0"
        "DarkCyan\0"
        "DarkGoldenrod\0"
        "DarkGoldenrod1\0"
        "DarkGoldenrod2\0"
        "DarkGoldenrod3\0"
        "DarkGoldenrod4\0"
        "DarkGray\0"
        "DarkGreen\0"
        "DarkGrey\0"
        "DarkKhaki\0"
        "DarkMagenta\0"
        "DarkOliveGreen\0"
        "DarkOliveGreen1\0"
        "DarkOliveGreen2\0"
        "DarkOliveGreen3\0"
        "DarkOliveGreen4\0"
        "DarkOrange\0"
        "DarkOrange1\0"
        "DarkOrange2\0"
        "DarkOrange3\0"
        "DarkOrange4\0"
        "DarkOrchid\0"
        "DarkOrchid1\0"
        "DarkOrchid2\0"
        "DarkOrchid3\0"
        "DarkOrchid4\0"
        "DarkRed\0"
        "DarkSalmon\0"
        "DarkSeaGreen\0"
        "DarkSeaGreen1\0"
        "DarkSeaGreen2\0"
        "DarkSeaGreen3\0"
        "DarkSeaGreen4\0"
        "DarkSlateBlue\0"
        "DarkSlateGray\0"
        "DarkSlateGray1\0"
        "DarkSlateGray2\0"
        "DarkSlateGray3\0"
        "DarkSlateGray4\0"
        "DarkSlateGrey\0"
        "DarkTurquoise\0"
        "DarkViolet\0"
        "deep pink\0"
        "deep sky blue\0"
        "DeepPink\0"
        "DeepPink1\0"
        "DeepPink2\0"
        "DeepPink3\0"
        "DeepPink4\0"
        "DeepSkyBlue\0"
        "DeepSkyBlue1\0"
        "DeepSkyBlue2\0"
        "DeepSkyBlue3\0"
        "DeepSkyBlue4\0"
        "dim gray\0"
        "dim grey\0"
        "DimGray\0"
        "DimGrey\0"
        "dodger blue\0"
        "DodgerBlue\0"
        "DodgerBlue1\0"
        "DodgerBlue2\0"
        "DodgerBlue3\0"
        "DodgerBlue4\0"
        "firebrick\0"
        "firebrick1\0"
        "firebrick2\0"
        "firebrick3\0"
        "firebrick4\0"
        "floral white\0"
        "FloralWhite\0"
        "forest green\0"
        "ForestGreen\0"
        "fuchsia\0"
        "gainsboro\0"
        "ghost white\0"
        "GhostWhite\0"
        "gold\0"
        "gold1\0"
        "gold2\0"
        "gold3\0"
        "gold4\0"
        "goldenrod\0"
        "goldenrod1\0"
        "goldenrod2\0"
        "goldenrod3\0"
        "goldenrod4\0"
        "gray\0"
        "gray0\0"
        "gray1\0"
        "gray10\0"
        "gray100\0"
        "gray11\0"
        "gray12\0"
        "gray13\0"
        "gray14\0"
        "gray15\0"
        "gray16\0"
        "gray17\0"
        "gray18\0"
        "gray19\0"
        "gray2\0"
        "gray20\0"
        "gray21\0"
        "gray22\0"
        "gray23\0"
        "gray24\0"
        "gray25\0"
        "gray26\0"
        "gray27\0"
        "gray28\0"
        "gray29\0"
        "gray3\0"
        "gray30\0"
        "gray31\0"
        "gray32\0"
        "gray33\0"
        "gray34\0"
        "gray35\0"
        "gray36\0"
        "gray37\0"
        "gray38\0"
        "gray39\0"
        "gray4\0"
        "gray40\0"
        "gray41\0"
        "gray42\0"
        "gray43\0"
        "gray44\0"
        "gray45\0"
        "gray46\0"
        "gray47\0"
        "gray48\0"
        "gray49\0"
        "gray5\0"
        "gray50\0"
        "gray51\0"
        "gray52\0"
        "gray53\0"
        "gray54\0"
        "gray55\0"
        "gray56\0"
        "gray57\0"
        "gray58\0"
        "gray59\0"
        "gray6\0"
        "gray60\0"
        "gray61\0"
        "gray62\0"
        "gray63\0"
        "gray64\0"
        "gray65\0"
        "gray66\0"
        "gray67\0"
        "gray68\0"
        "gray69\0"
        "gray7\0"
        "gray70\0"
        "gray71\0"
        "gray72\0"
        "gray73\0"
        "gray74\0"
        "gray75\0"
        "gray76\0"
        "gray77\0"
        "gray78\0"
        "gray79\0"
        "gray8\0"
        "gray80\0"
        "gray81\0"
        "gray82\0"
        "gray83\0"
        "gray84\0"
        "gray85\0"
        "gray86\0"
        "gray87\0"
        "gray88\0"
        "gray89\0"
        "gray9\0"
        "gray90\0"
        "gray91\0"
        "gray92\0"
        "gray93\0"
        "gray94\0"
        "gray95\0"
        "gray96\0"
        "gray97\0"
        "gray98\0"
        "gray99\0"
        "green\0"
        "green yellow\0"
        "green1\0"
        "green2\0"
        "green3\0"
        "green4\0"
        "GreenYellow\0"
        "grey\0"
        "grey0\0"
        "grey1\0"
        "grey10\0"
        "grey100\0"
        "grey11\0"
        "grey12\0"
        "grey13\0"
        "grey14\0"
        "grey15\0"
        "grey16\0"
        "grey17\0"
        "grey18\0"
        "grey19\0"
        "grey2\0"
        "grey20\0"
        "grey21\0"
        "grey22\0"
        "grey23\0"
        "grey24\0"
        "grey25\0"
        "grey26\0"
        "grey27\0"
        "grey28\0"
        "grey29\0"
        "grey3\0"
        "grey30\0"
        "grey31\0"
        "grey32\0"
        "grey33\0"
        "grey34\0"
        "grey35\0"
        "grey36\0"
        "grey37\0"
        "grey38\0"
        "grey39\0"
        "grey4\0"
        "grey40\0"
        "grey41\0"
        "grey42\0"
        "grey43\0"
        "grey44\0"
        "grey45\0"
        "grey46\0"
        "grey47\0"
        "grey48\0"
        "grey49\0"
        "grey5\0"
        "grey50\0"
        "grey51\0"
        "grey52\0"
        "grey53\0"
        "grey54\0"
        "grey55\0"
        "grey56\0"
        "grey57\0"
        "grey58\0"
        "grey59\0"
        "grey6\0"
        "grey60\0"
        "grey61\0"
        "grey62\0"
        "grey63\0"
        "grey64\0"
        "grey65\0"
        "grey66\0"
        "grey67\0"
        "grey68\0"
        "grey69\0"
        "grey7\0"
        "grey70\0"
        "grey71\0"
        "grey72\0"
        "grey73\0"
        "grey74\0"
        "grey75\0"
        "grey76\0"
        "grey77\0"
        "grey78\0"
        "grey79\0"
        "grey8\0"
        "grey80\0"
        "grey81\0"
        "grey82\0"
        "grey83\0"
        "grey84\0"
        "grey85\0"
        "grey86\0"
        "grey87\0"
        "grey88\0"
        "grey89\0"
        "grey9\0"
        "grey90\0"
        "grey91\0"
        "grey92\0"
        "grey93\0"
        "grey94\0"
        "grey95\0"
        "grey96\0"
        "grey97\0"
        "grey98\0"
        "grey99\0"
        "honeydew\0"
        "honeydew1\0"
        "honeydew2\0"
        "honeydew3\0"
        "honeydew4\0"
        "hot pink\0"
        "HotPink\0"
        "HotPink1\0"
        "HotPink2\0"
        "HotPink3\0"
        "HotPink4\0"
        "indian red\0"
        "IndianRed\0"
        "IndianRed1\0"
        "IndianRed2\0"
        "IndianRed3\0"
        "IndianRed4\0"
        "indigo\0"
        "ivory\0"
        "ivory1\0"
        "ivory2\0"
        "ivory3\0"
        "ivory4\0"
        "khaki\0"
        "khaki1\0"
        "khaki2\0"
        "khaki3\0"
        "khaki4\0"
        "lavender\0"
        "lavender blush\0"
        "LavenderBlush\0"
        "LavenderBlush1\0"
        "LavenderBlush2\0"
        "LavenderBlush3\0"
        "LavenderBlush4\0"
        "lawn green\0"
        "LawnGreen\0"
        "lemon chiffon\0"
        "LemonChiffon\0"
        "LemonChiffon1\0"
        "LemonChiffon2\0"
        "LemonChiffon3\0"
        "LemonChiffon4\0"
        "light blue\0"
        "light coral\0"
        "light cyan\0"
        "light goldenrod\0"
        "light goldenrod yellow\0"
        "light gray\0"
        "light green\0"
        "light grey\0"
        "light pink\0"
        "light salmon\0"
        "light sea green\0"
        "light sky blue\0"
        "light slate blue\0"
        "light slate gray\0"
        "light slate grey\0"
        "light steel blue\0"
        "light yellow\0"
        "LightBlue\0"
        "LightBlue1\0"
        "LightBlue2\0"
        "LightBlue3\0"
        "LightBlue4\0"
        "LightCoral\0"
        "LightCyan\0"
        "LightCyan1\0"
        "LightCyan2\0"
        "LightCyan3\0"
        "LightCyan4\0"
        "LightGoldenrod\0"
        "LightGoldenrod1\0"
        "LightGoldenrod2\0"
        "LightGoldenrod3\0"
        "LightGoldenrod4\0"
        "LightGoldenrodYellow\0"
        "LightGray\0"
        "LightGreen\0"
        "LightGrey\0"
        "LightPink\0"
        "LightPink1\0"
        "LightPink2\0"
        "LightPink3\0"
        "LightPink4\0"
        "LightSalmon\0"
        "LightSalmon1\0"
        "LightSalmon2\0"
        "LightSalmon3\0"
        "LightSalmon4\0"
        "LightSeaGreen\0"
        "LightSkyBlue\0"
        "LightSkyBlue1\0"
        "LightSkyBlue2\0"
        "LightSkyBlue3\0"
        "LightSkyBlue4\0"
        "LightSlateBlue\0"
        "LightSlateGray\0"
        "LightSlateGrey\0"
        "LightSteelBlue\0"
        "LightSteelBlue1\0"
        "LightSteelBlue2\0"
        "LightSteelBlue3\0"
        "LightSteelBlue4\0"
        "LightYellow\0"
        "LightYellow1\0"
        "LightYellow2\0"
        "LightYellow3\0"
        "LightYellow4\0"
        "lime\0"
        "lime green\0"
        "LimeGreen\0"
        "linen\0"
        "magenta\0"
        "magenta1\0"
        "magenta2\0"
        "magenta3\0"
        "magenta4\0"
        "maroon\0"
        "maroon1\0"
        "maroon2\0"
        "maroon3\0"
        "maroon4\0"
        "medium aquamarine\0"
        "medium blue\0"
        "medium orchid\0"
        "medium purple\0"
        "medium sea green\0"
        "medium slate blue\0"
        "medium spring green\0"
        "medium turquoise\0"
        "medium violet red\0"
        "MediumAquamarine\0"
        "MediumBlue\0"
        "MediumOrchid\0"
        "MediumOrchid1\0"
        "MediumOrchid2\0"
        "MediumOrchid3\0"
        "MediumOrchid4\0"
        "MediumPurple\0"
        "MediumPurple1\0"
        "MediumPurple2\0"
        "MediumPurple3\0"
        "MediumPurple4\0"
        "MediumSeaGreen\0"
        "MediumSlateBlue\0"
        "MediumSpringGreen\0"
        "MediumTurquoise\0"
        "MediumVioletRed\0"
        "midnight blue\0"
        "MidnightBlue\0"
        "mint cream\0"
        "MintCream\0"
        "misty rose\0"
        "MistyRose\0"
        "MistyRose1\0"
        "MistyRose2\0"
        "MistyRose3\0"
        "MistyRose4\0"
        "moccasin\0"
        "navajo white\0"
        "NavajoWhite\0"
        "NavajoWhite1\0"
        "NavajoWhite2\0"
        "NavajoWhite3\0"
        "NavajoWhite4\0"
        "navy\0"
        "navy blue\0"
        "NavyBlue\0"
        "old lace\0"
        "OldLace\0"
        "olive\0"
        "olive drab\0"
        "OliveDrab\0"
        "OliveDrab1\0"
        "OliveDrab2\0"
        "OliveDrab3\0"
        "OliveDrab4\0"
        "orange\0"
        "orange red\0"
        "orange1\0"
        "orange2\0"
        "orange3\0"
        "orange4\0"
        "OrangeRed\0"
        "OrangeRed1\0"
        "OrangeRed2\0"
        "OrangeRed3\0"
        "OrangeRed4\0"
        "orchid\0"
        "orchid1\0"
        "orchid2\0"
        "orchid3\0"
        "orchid4\0"
        "pale goldenrod\0"
        "pale green\0"
        "pale turquoise\0"
        "pale violet red\0"
        "PaleGoldenrod\0"
        "PaleGreen\0"
        "PaleGreen1\0"
        "PaleGreen2\0"
        "PaleGreen3\0"
        "PaleGreen4\0"
        "PaleTurquoise\0"
        "PaleTurquoise1\0"
        "PaleTurquoise2\0"
        "PaleTurquoise3\0"
        "PaleTurquoise4\0"
        "PaleVioletRed\0"
        "PaleVioletRed1\0"
        "PaleVioletRed2\0"
        "PaleVioletRed3\0"
        "PaleVioletRed4\0"
        "papaya whip\0"
        "PapayaWhip\0"
        "peach puff\0"
        "PeachPuff\0"
        "PeachPuff1\0"
        "PeachPuff2\0"
        "PeachPuff3\0"
        "PeachPuff4\0"
        "peru\0"
        "pink\0"
        "pink1\0"
        "pink2\0"
        "pink3\0"
        "pink4\0"
        "plum\0"
        "plum1\0"
        "plum2\0"
        "plum3\0"
        "plum4\0"
        "powder blue\0"
        "PowderBlue\0"
        "purple\0"
        "purple1\0"
        "purple2\0"
        "purple3\0"
        "purple4\0"
        "rebecca purple\0"
        "RebeccaPurple\0"
        "red\0"
        "red1\0"
        "red2\0"
        "red3\0"
        "red4\0"
        "rosy brown\0"
        "RosyBrown\0"
        "RosyBrown1\0"
        "RosyBrown2\0"
        "RosyBrown3\0"
        "RosyBrown4\0"
        "royal blue\0"
        "RoyalBlue\0"
        "RoyalBlue1\0"
        "RoyalBlue2\0"
        "RoyalBlue3\0"
        "RoyalBlue4\0"
        "saddle brown\0"
        "SaddleBrown\0"
        "salmon\0"
        "salmon1\0"
        "salmon2\0"
        "salmon3\0"
        "salmon4\0"
        "sandy brown\0"
        "SandyBrown\0"
        "sea green\0"
        "SeaGreen\0"
        "SeaGreen1\0"
        "SeaGreen2\0"
        "SeaGreen3\0"
        "SeaGreen4\0"
        "seashell\0"
        "seashell1\0"
        "seashell2\0"
        "seashell3\0"
        "seashell4\0"
        "sienna\0"
        "sienna1\0"
        "sienna2\0"
        "sienna3\0"
        "sienna4\0"
        "silver\0"
        "sky blue\0"
        "SkyBlue\0"
        "SkyBlue1\0"
        "SkyBlue2\0"
        "SkyBlue3\0"
        "SkyBlue4\0"
        "slate blue\0"
        "slate gray\0"
        "slate grey\0"
        "SlateBlue\0"
        "SlateBlue1\0"
        "SlateBlue2\0"
        "SlateBlue3\0"
        "SlateBlue4\0"
        "SlateGray\0"
        "SlateGray1\0"
        "SlateGray2\0"
        "SlateGray3\0"
        "SlateGray4\0"
        "SlateGrey\0"
        "snow\0"
        "snow1\0"
        "snow2\0"
        "snow3\0"
        "snow4\0"
        "spring green\0"
        "SpringGreen\0"
        "SpringGreen1\0"
        "SpringGreen2\0"
        "SpringGreen3\0"
        "SpringGreen4\0"
        "steel blue\0"
        "SteelBlue\0"
        "SteelBlue1\0"
        "SteelBlue2\0"
        "SteelBlue3\0"
        "SteelBlue4\0"
        "tan\0"
        "tan1\0"
        "tan2\0"
        "tan3\0"
        "tan4\0"
        "teal\0"
        "thistle\0"
        "thistle1\0"
        "thistle2\0"
        "thistle3\0"
        "thistle4\0"
        "tomato\0"
        "tomato1\0"
        "tomato2\0"
        "tomato3\0"
        "tomato4\0"
        "turquoise\0"
        "turquoise1\0"
        "turquoise2\0"
        "turquoise3\0"
        "turquoise4\0"
        "violet\0"
        "violet red\0"
        "VioletRed\0"
        "VioletRed1\0"
        "VioletRed2\0"
        "VioletRed3\0"
        "VioletRed4\0"
        "web gray\0"
        "web green\0"
        "web grey\0"
        "web maroon\0"
        "web purple\0"
        "WebGray\0"
        "WebGreen\0"
        "WebGrey\0"
        "WebMaroon\0"
        "WebPurple\0"
        "wheat\0"
        "wheat1\0"
        "wheat2\0"
        "wheat3\0"
        "wheat4\0"
        "white\0"
        "white smoke\0"
        "WhiteSmoke\0"
        "x11 gray\0"
        "x11 green\0"
        "x11 grey\0"
        "x11 maroon\0"
        "x11 purple\0"
        "X11Gray\0"
        "X11Green\0"
        "X11Grey\0"
        "X11Maroon\0"
        "X11Purple\0"
        "yellow\0"
        "yellow green\0"
        "yellow1\0"
        "yellow2\0"
        "yellow3\0"
        "yellow4\0"
        "YellowGreen\0"
};

static const BuiltinColor BuiltinColors[] = {
    {240, 248, 255, 0},         /* alice blue */
    {240, 248, 255, 11},        /* AliceBlue */
    {250, 235, 215, 21},        /* antique white */
    {250, 235, 215, 35},        /* AntiqueWhite */
    {255, 239, 219, 48},        /* AntiqueWhite1 */
    {238, 223, 204, 62},        /* AntiqueWhite2 */
    {205, 192, 176, 76},        /* AntiqueWhite3 */
    {139, 131, 120, 90},        /* AntiqueWhite4 */
    {0, 255, 255, 104},         /* aqua */
    {127, 255, 212, 109},       /* aquamarine */
    {127, 255, 212, 120},       /* aquamarine1 */
    {118, 238, 198, 132},       /* aquamarine2 */
    {102, 205, 170, 144},       /* aquamarine3 */
    {69, 139, 116, 156},        /* aquamarine4 */
    {240, 255, 255, 168},       /* azure */
    {240, 255, 255, 174},       /* azure1 */
    {224, 238, 238, 181},       /* azure2 */
    {193, 205, 205, 188},       /* azure3 */
    {131, 139, 139, 195},       /* azure4 */
    {245, 245, 220, 202},       /* beige */
    {255, 228, 196, 208},       /* bisque */
    {255, 228, 196, 215},       /* bisque1 */
    {238, 213, 183, 223},       /* bisque2 */
    {205, 183, 158, 231},       /* bisque3 */
    {139, 125, 107, 239},       /* bisque4 */
    {0, 0, 0, 247},             /* black */
    {255, 235, 205, 253},       /* blanched almond */
    {255, 235, 205, 269},       /* BlanchedAlmond */
    {0, 0, 255, 284},           /* blue */
    {138, 43, 226, 289},        /* blue violet */
    {0, 0, 255, 301},           /* blue1 */
    {0, 0, 238, 307},           /* blue2 */
    {0, 0, 205, 313},           /* blue3 */
    {0, 0, 139, 319},           /* blue4 */
    {138, 43, 226, 325},        /* BlueViolet */
    {165, 42, 42, 336},         /* brown */
    {255, 64, 64, 342},         /* brown1 */
    {238, 59, 59, 349},         /* brown2 */
    {205, 51, 51, 356},         /* brown3 */
    {139, 35, 35, 363},         /* brown4 */
    {222, 184, 135, 370},       /* burlywood */
    {255, 211, 155, 380},       /* burlywood1 */
    {238, 197, 145, 391},       /* burlywood2 */
    {205, 170, 125, 402},       /* burlywood3 */
    {139, 115, 85, 413},        /* burlywood4 */
    {95, 158, 160, 424},        /* cadet blue */
    {95, 158, 160, 435},        /* CadetBlue */
    {152, 245, 255, 445},       /* CadetBlue1 */
    {142, 229, 238, 456},       /* CadetBlue2 */
    {122, 197, 205, 467},       /* CadetBlue3 */
    {83, 134, 139, 478},        /* CadetBlue4 */
    {127, 255, 0, 489},         /* chartreuse */
    {127, 255, 0, 500},         /* chartreuse1 */
    {118, 238, 0, 512},         /* chartreuse2 */
    {102, 205, 0, 524},         /* chartreuse3 */
    {69, 139, 0, 536},          /* chartreuse4 */
    {210, 105, 30, 548},        /* chocolate */
    {255, 127, 36, 558},        /* chocolate1 */
    {238, 118, 33, 569},        /* chocolate2 */
    {205, 102, 29, 580},        /* chocolate3 */
    {139, 69, 19, 591},         /* chocolate4 */
    {255, 127, 80, 602},        /* coral */
    {255, 114, 86, 608},        /* coral1 */
    {238, 106, 80, 615},        /* coral2 */
    {205, 91, 69, 622},         /* coral3 */
    {139, 62, 47, 629},         /* coral4 */
    {100, 149, 237, 636},       /* cornflower blue */
    {100, 149, 237, 652},       /* CornflowerBlue */
    {255, 248, 220, 667},       /* cornsilk */
    {255, 248, 220, 676},       /* cornsilk1 */
    {238, 232, 205, 686},       /* cornsilk2 */
    {205, 200, 177, 696},       /* cornsilk3 */
    {139, 136, 120, 706},       /* cornsilk4 */
    {220, 20, 60, 716},         /* crimson */
    {0, 255, 255, 724},         /* cyan */
    {0, 255, 255, 729},         /* cyan1 */
    {0, 238, 238, 735},         /* cyan2 */
    {0, 205, 205, 741},         /* cyan3 */
    {0, 139, 139, 747},         /* cyan4 */
    {0, 0, 139, 753},           /* dark blue */
    {0, 139, 139, 763},         /* dark cyan */
    {184, 134, 11, 773},        /* dark goldenrod */
    {169, 169, 169, 788},       /* dark gray */
    {0, 100, 0, 798},           /* dark green */
    {169, 169, 169, 809},       /* dark grey */
    {189, 183, 107, 819},       /* dark khaki */
    {139, 0, 139, 830},         /* dark magenta */
    {85, 107, 47, 843},         /* dark olive green */
    {255, 140, 0, 860},         /* dark orange */
    {153, 50, 204, 872},        /* dark orchid */
    {139, 0, 0, 884},           /* dark red */
    {233, 150, 122, 893},       /* dark salmon */
    {143, 188, 143, 905},       /* dark sea green */
    {72, 61, 139, 920},         /* dark slate blue */
    {47, 79, 79, 936},          /* dark slate gray */
    {47, 79, 79, 952},          /* dark slate grey */
    {0, 206, 209, 968},         /* dark turquoise */
    {148, 0, 211, 983},         /* dark violet */
    {0, 0, 139, 995},           /* DarkBlue */
    {0, 139, 139, 1004},        /* DarkCyan */
    {184, 134, 11, 1013},       /* DarkGoldenrod */
    {255, 185, 15, 1027},       /* DarkGoldenrod1 */
    {238, 173, 14, 1042},       /* DarkGoldenrod2 */
    {205, 149, 12, 1057},       /* DarkGoldenrod3 */
    {139, 101, 8, 1072},        /* DarkGoldenrod4 */
    {169, 169, 169, 1087},      /* DarkGray */
    {0, 100, 0, 1096},          /* DarkGreen */
    {169, 169, 169, 1106},      /* DarkGrey */
    {189, 183, 107, 1115},      /* DarkKhaki */
    {139, 0, 139, 1125},        /* DarkMagenta */
    {85, 107, 47, 1137},        /* DarkOliveGreen */
    {202, 255, 112, 1152},      /* DarkOliveGreen1 */
    {188, 238, 104, 1168},      /* DarkOliveGreen2 */
    {162, 205, 90, 1184},       /* DarkOliveGreen3 */
    {110, 139, 61, 1200},       /* DarkOliveGreen4 */
    {255, 140, 0, 1216},        /* DarkOrange */
    {255, 127, 0, 1227},        /* DarkOrange1 */
    {238, 118, 0, 1239},        /* DarkOrange2 */
    {205, 102, 0, 1251},        /* DarkOrange3 */
    {139, 69, 0, 1263},         /* DarkOrange4 */
    {153, 50, 204, 1275},       /* DarkOrchid */
    {191, 62, 255, 1286},       /* DarkOrchid1 */
    {178, 58, 238, 1298},       /* DarkOrchid2 */
    {154, 50, 205, 1310},       /* DarkOrchid3 */
    {104, 34, 139, 1322},       /* DarkOrchid4 */
    {139, 0, 0, 1334},          /* DarkRed */
    {233, 150, 122, 1342},      /* DarkSalmon */
    {143, 188, 143, 1353},      /* DarkSeaGreen */
    {193, 255, 193, 1366},      /* DarkSeaGreen1 */
    {180, 238, 180, 1380},      /* DarkSeaGreen2 */
    {155, 205, 155, 1394},      /* DarkSeaGreen3 */
    {105, 139, 105, 1408},      /* DarkSeaGreen4 */
    {72, 61, 139, 1422},        /* DarkSlateBlue */
    {47, 79, 79, 1436},         /* DarkSlateGray */
    {151, 255, 255, 1450},      /* DarkSlateGray1 */
    {141, 238, 238, 1465},      /* DarkSlateGray2 */
    {121, 205, 205, 1480},      /* DarkSlateGray3 */
    {82, 139, 139, 1495},       /* DarkSlateGray4 */
    {47, 79, 79, 1510},         /* DarkSlateGrey */
    {0, 206, 209, 1524},        /* DarkTurquoise */
    {148, 0, 211, 1538},        /* DarkViolet */
    {255, 20, 147, 1549},       /* deep pink */
    {0, 191, 255, 1559},        /* deep sky blue */
    {255, 20, 147, 1573},       /* DeepPink */
    {255, 20, 147, 1582},       /* DeepPink1 */
    {238, 18, 137, 1592},       /* DeepPink2 */
    {205, 16, 118, 1602},       /* DeepPink3 */
    {139, 10, 80, 1612},        /* DeepPink4 */
    {0, 191, 255, 1622},        /* DeepSkyBlue */
    {0, 191, 255, 1634},        /* DeepSkyBlue1 */
    {0, 178, 238, 1647},        /* DeepSkyBlue2 */
    {0, 154, 205, 1660},        /* DeepSkyBlue3 */
    {0, 104, 139, 1673},        /* DeepSkyBlue4 */
    {105, 105, 105, 1686},      /* dim gray */
    {105, 105, 105, 1695},      /* dim grey */
    {105, 105, 105, 1704},      /* DimGray */
    {105, 105, 105, 1712},      /* DimGrey */
    {30, 144, 255, 1720},       /* dodger blue */
    {30, 144, 255, 1732},       /* DodgerBlue */
    {30, 144, 255, 1743},       /* DodgerBlue1 */
    {28, 134, 238, 1755},       /* DodgerBlue2 */
    {24, 116, 205, 1767},       /* DodgerBlue3 */
    {16, 78, 139, 1779},        /* DodgerBlue4 */
    {178, 34, 34, 1791},        /* firebrick */
    {255, 48, 48, 1801},        /* firebrick1 */
    {238, 44, 44, 1812},        /* firebrick2 */
    {205, 38, 38, 1823},        /* firebrick3 */
    {139, 26, 26, 1834},        /* firebrick4 */
    {255, 250, 240, 1845},      /* floral white */
    {255, 250, 240, 1858},      /* FloralWhite */
    {34, 139, 34, 1870},        /* forest green */
    {34, 139, 34, 1883},        /* ForestGreen */
    {255, 0, 255, 1895},        /* fuchsia */
    {220, 220, 220, 1903},      /* gainsboro */
    {248, 248, 255, 1913},      /* ghost white */
    {248, 248, 255, 1925},      /* GhostWhite */
    {255, 215, 0, 1936},        /* gold */
    {255, 215, 0, 1941},        /* gold1 */
    {238, 201, 0, 1947},        /* gold2 */
    {205, 173, 0, 1953},        /* gold3 */
    {139, 117, 0, 1959},        /* gold4 */
    {218, 165, 32, 1965},       /* goldenrod */
    {255, 193, 37, 1975},       /* goldenrod1 */
    {238, 180, 34, 1986},       /* goldenrod2 */
    {205, 155, 29, 1997},       /* goldenrod3 */
    {139, 105, 20, 2008},       /* goldenrod4 */
    {190, 190, 190, 2019},      /* gray */
    {0, 0, 0, 2024},            /* gray0 */
    {3, 3, 3, 2030},            /* gray1 */
    {26, 26, 26, 2036},         /* gray10 */
    {255, 255, 255, 2043},      /* gray100 */
    {28, 28, 28, 2051},         /* gray11 */
    {31, 31, 31, 2058},         /* gray12 */
    {33, 33, 33, 2065},         /* gray13 */
    {36, 36, 36, 2072},         /* gray14 */
    {38, 38, 38, 2079},         /* gray15 */
    {41, 41, 41, 2086},         /* gray16 */
    {43, 43, 43, 2093},         /* gray17 */
    {46, 46, 46, 2100},         /* gray18 */
    {48, 48, 48, 2107},         /* gray19 */
    {5, 5, 5, 2114},            /* gray2 */
    {51, 51, 51, 2120},         /* gray20 */
    {54, 54, 54, 2127},         /* gray21 */
    {56, 56, 56, 2134},         /* gray22 */
    {59, 59, 59, 2141},         /* gray23 */
    {61, 61, 61, 2148},         /* gray24 */
    {64, 64, 64, 2155},         /* gray25 */
    {66, 66, 66, 2162},         /* gray26 */
    {69, 69, 69, 2169},         /* gray27 */
    {71, 71, 71, 2176},         /* gray28 */
    {74, 74, 74, 2183},         /* gray29 */
    {8, 8, 8, 2190},            /* gray3 */
    {77, 77, 77, 2196},         /* gray30 */
    {79, 79, 79, 2203},         /* gray31 */
    {82, 82, 82, 2210},         /* gray32 */
    {84, 84, 84, 2217},         /* gray33 */
    {87, 87, 87, 2224},         /* gray34 */
    {89, 89, 89, 2231},         /* gray35 */
    {92, 92, 92, 2238},         /* gray36 */
    {94, 94, 94, 2245},         /* gray37 */
    {97, 97, 97, 2252},         /* gray38 */
    {99, 99, 99, 2259},         /* gray39 */
    {10, 10, 10, 2266},         /* gray4 */
    {102, 102, 102, 2272},      /* gray40 */
    {105, 105, 105, 2279},      /* gray41 */
    {107, 107, 107, 2286},      /* gray42 */
    {110, 110, 110, 2293},      /* gray43 */
    {112, 112, 112, 2300},      /* gray44 */
    {115, 115, 115, 2307},      /* gray45 */
    {117, 117, 117, 2314},      /* gray46 */
    {120, 120, 120, 2321},      /* gray47 */
    {122, 122, 122, 2328},      /* gray48 */
    {125, 125, 125, 2335},      /* gray49 */
    {13, 13, 13, 2342},         /* gray5 */
    {127, 127, 127, 2348},      /* gray50 */
    {130, 130, 130, 2355},      /* gray51 */
    {133, 133, 133, 2362},      /* gray52 */
    {135, 135, 135, 2369},      /* gray53 */
    {138, 138, 138, 2376},      /* gray54 */
    {140, 140, 140, 2383},      /* gray55 */
    {143, 143, 143, 2390},      /* gray56 */
    {145, 145, 145, 2397},      /* gray57 */
    {148, 148, 148, 2404},      /* gray58 */
    {150, 150, 150, 2411},      /* gray59 */
    {15, 15, 15, 2418},         /* gray6 */
    {153, 153, 153, 2424},      /* gray60 */
    {156, 156, 156, 2431},      /* gray61 */
    {158, 158, 158, 2438},      /* gray62 */
    {161, 161, 161, 2445},      /* gray63 */
    {163, 163, 163, 2452},      /* gray64 */
    {166, 166, 166, 2459},      /* gray65 */
    {168, 168, 168, 2466},      /* gray66 */
    {171, 171, 171, 2473},      /* gray67 */
    {173, 173, 173, 2480},      /* gray68 */
    {176, 176, 176, 2487},      /* gray69 */
    {18, 18, 18, 2494},         /* gray7 */
    {179, 179, 179, 2500},      /* gray70 */
    {181, 181, 181, 2507},      /* gray71 */
    {184, 184, 184, 2514},      /* gray72 */
    {186, 186, 186, 2521},      /* gray73 */
    {189, 189, 189, 2528},      /* gray74 */
    {191, 191, 191, 2535},      /* gray75 */
    {194, 194, 194, 2542},      /* gray76 */
    {196, 196, 196, 2549},      /* gray77 */
    {199, 199, 199, 2556},      /* gray78 */
    {201, 201, 201, 2563},      /* gray79 */
    {20, 20, 20, 2570},         /* gray8 */
    {204, 204, 204, 2576},      /* gray80 */
    {207, 207, 207, 2583},      /* gray81 */
    {209, 209, 209, 2590},      /* gray82 */
    {212, 212, 212, 2597},      /* gray83 */
    {214, 214, 214, 2604},      /* gray84 */
    {217, 217, 217, 2611},      /* gray85 */
    {219, 219, 219, 2618},      /* gray86 */
    {222, 222, 222, 2625},      /* gray87 */
    {224, 224, 224, 2632},      /* gray88 */
    {227, 227, 227, 2639},      /* gray89 */
    {23, 23, 23, 2646},         /* gray9 */
    {229, 229, 229, 2652},      /* gray90 */
    {232, 232, 232, 2659},      /* gray91 */
    {235, 235, 235, 2666},      /* gray92 */
    {237, 237, 237, 2673},      /* gray93 */
    {240, 240, 240, 2680},      /* gray94 */
    {242, 242, 242, 2687},      /* gray95 */
    {245, 245, 245, 2694},      /* gray96 */
    {247, 247, 247, 2701},      /* gray97 */
    {250, 250, 250, 2708},      /* gray98 */
    {252, 252, 252, 2715},      /* gray99 */
    {0, 255, 0, 2722},          /* green */
    {173, 255, 47, 2728},       /* green yellow */
    {0, 255, 0, 2741},          /* green1 */
    {0, 238, 0, 2748},          /* green2 */
    {0, 205, 0, 2755},          /* green3 */
    {0, 139, 0, 2762},          /* green4 */
    {173, 255, 47, 2769},       /* GreenYellow */
    {190, 190, 190, 2781},      /* grey */
    {0, 0, 0, 2786},            /* grey0 */
    {3, 3, 3, 2792},            /* grey1 */
    {26, 26, 26, 2798},         /* grey10 */
    {255, 255, 255, 2805},      /* grey100 */
    {28, 28, 28, 2813},         /* grey11 */
    {31, 31, 31, 2820},         /* grey12 */
    {33, 33, 33, 2827},         /* grey13 */
    {36, 36, 36, 2834},         /* grey14 */
    {38, 38, 38, 2841},         /* grey15 */
    {41, 41, 41, 2848},         /* grey16 */
    {43, 43, 43, 2855},         /* grey17 */
    {46, 46, 46, 2862},         /* grey18 */
    {48, 48, 48, 2869},         /* grey19 */
    {5, 5, 5, 2876},            /* grey2 */
    {51, 51, 51, 2882},         /* grey20 */
    {54, 54, 54, 2889},         /* grey21 */
    {56, 56, 56, 2896},         /* grey22 */
    {59, 59, 59, 2903},         /* grey23 */
    {61, 61, 61, 2910},         /* grey24 */
    {64, 64, 64, 2917},         /* grey25 */
    {66, 66, 66, 2924},         /* grey26 */
    {69, 69, 69, 2931},         /* grey27 */
    {71, 71, 71, 2938},         /* grey28 */
    {74, 74, 74, 2945},         /* grey29 */
    {8, 8, 8, 2952},            /* grey3 */
    {77, 77, 77, 2958},         /* grey30 */
    {79, 79, 79, 2965},         /* grey31 */
    {82, 82, 82, 2972},         /* grey32 */
    {84, 84, 84, 2979},         /* grey33 */
    {87, 87, 87, 2986},         /* grey34 */
    {89, 89, 89, 2993},         /* grey35 */
    {92, 92, 92, 3000},         /* grey36 */
    {94, 94, 94, 3007},         /* grey37 */
    {97, 97, 97, 3014},         /* grey38 */
    {99, 99, 99, 3021},         /* grey39 */
    {10, 10, 10, 3028},         /* grey4 */
    {102, 102, 102, 3034},      /* grey40 */
    {105, 105, 105, 3041},      /* grey41 */
    {107, 107, 107, 3048},      /* grey42 */
    {110, 110, 110, 3055},      /* grey43 */
    {112, 112, 112, 3062},      /* grey44 */
    {115, 115, 115, 3069},      /* grey45 */
    {117, 117, 117, 3076},      /* grey46 */
    {120, 120, 120, 3083},      /* grey47 */
    {122, 122, 122, 3090},      /* grey48 */
    {125, 125, 125, 3097},      /* grey49 */
    {13, 13, 13, 3104},         /* grey5 */
    {127, 127, 127, 3110},      /* grey50 */
    {130, 130, 130, 3117},      /* grey51 */
    {133, 133, 133, 3124},      /* grey52 */
    {135, 135, 135, 3131},      /* grey53 */
    {138, 138, 138, 3138},      /* grey54 */
    {140, 140, 140, 3145},      /* grey55 */
    {143, 143, 143, 3152},      /* grey56 */
    {145, 145, 145, 3159},      /* grey57 */
    {148, 148, 148, 3166},      /* grey58 */
    {150, 150, 150, 3173},      /* grey59 */
    {15, 15, 15, 3180},         /* grey6 */
    {153, 153, 153, 3186},      /* grey60 */
    {156, 156, 156, 3193},      /* grey61 */
    {158, 158, 158, 3200},      /* grey62 */
    {161, 161, 161, 3207},      /* grey63 */
    {163, 163, 163, 3214},      /* grey64 */
    {166, 166, 166, 3221},      /* grey65 */
    {168, 168, 168, 3228},      /* grey66 */
    {171, 171, 171, 3235},      /* grey67 */
    {173, 173, 173, 3242},      /* grey68 */
    {176, 176, 176, 3249},      /* grey69 */
    {18, 18, 18, 3256},         /* grey7 */
    {179, 179, 179, 3262},      /* grey70 */
    {181, 181, 181, 3269},      /* grey71 */
    {184, 184, 184, 3276},      /* grey72 */
    {186, 186, 186, 3283},      /* grey73 */
    {189, 189, 189, 3290},      /* grey74 */
    {191, 191, 191, 3297},      /* grey75 */
    {194, 194, 194, 3304},      /* grey76 */
    {196, 196, 196, 3311},      /* grey77 */
    {199, 199, 199, 3318},      /* grey78 */
    {201, 201, 201, 3325},      /* grey79 */
    {20, 20, 20, 3332},         /* grey8 */
    {204, 204, 204, 3338},      /* grey80 */
    {207, 207, 207, 3345},      /* grey81 */
    {209, 209, 209, 3352},      /* grey82 */
    {212, 212, 212, 3359},      /* grey83 */
    {214, 214, 214, 3366},      /* grey84 */
    {217, 217, 217, 3373},      /* grey85 */
    {219, 219, 219, 3380},      /* grey86 */
    {222, 222, 222, 3387},      /* grey87 */
    {224, 224, 224, 3394},      /* grey88 */
    {227, 227, 227, 3401},      /* grey89 */
    {23, 23, 23, 3408},         /* grey9 */
    {229, 229, 229, 3414},      /* grey90 */
    {232, 232, 232, 3421},      /* grey91 */
    {235, 235, 235, 3428},      /* grey92 */
    {237, 237, 237, 3435},      /* grey93 */
    {240, 240, 240, 3442},      /* grey94 */
    {242, 242, 242, 3449},      /* grey95 */
    {245, 245, 245, 3456},      /* grey96 */
    {247, 247, 247, 3463},      /* grey97 */
    {250, 250, 250, 3470},      /* grey98 */
    {252, 252, 252, 3477},      /* grey99 */
    {240, 255, 240, 3484},      /* honeydew */
    {240, 255, 240, 3493},      /* honeydew1 */
    {224, 238, 224, 3503},      /* honeydew2 */
    {193, 205, 193, 3513},      /* honeydew3 */
    {131, 139, 131, 3523},      /* honeydew4 */
    {255, 105, 180, 3533},      /* hot pink */
    {255, 105, 180, 3542},      /* HotPink */
    {255, 110, 180, 3550},      /* HotPink1 */
    {238, 106, 167, 3559},      /* HotPink2 */
    {205, 96, 144, 3568},       /* HotPink3 */
    {139, 58, 98, 3577},        /* HotPink4 */
    {205, 92, 92, 3586},        /* indian red */
    {205, 92, 92, 3597},        /* IndianRed */
    {255, 106, 106, 3607},      /* IndianRed1 */
    {238, 99, 99, 3618},        /* IndianRed2 */
    {205, 85, 85, 3629},        /* IndianRed3 */
    {139, 58, 58, 3640},        /* IndianRed4 */
    {75, 0, 130, 3651},         /* indigo */
    {255, 255, 240, 3658},      /* ivory */
    {255, 255, 240, 3664},      /* ivory1 */
    {238, 238, 224, 3671},      /* ivory2 */
    {205, 205, 193, 3678},      /* ivory3 */
    {139, 139, 131, 3685},      /* ivory4 */
    {240, 230, 140, 3692},      /* khaki */
    {255, 246, 143, 3698},      /* khaki1 */
    {238, 230, 133, 3705},      /* khaki2 */
    {205, 198, 115, 3712},      /* khaki3 */
    {139, 134, 78, 3719},       /* khaki4 */
    {230, 230, 250, 3726},      /* lavender */
    {255, 240, 245, 3735},      /* lavender blush */
    {255, 240, 245, 3750},      /* LavenderBlush */
    {255, 240, 245, 3764},      /* LavenderBlush1 */
    {238, 224, 229, 3779},      /* LavenderBlush2 */
    {205, 193, 197, 3794},      /* LavenderBlush3 */
    {139, 131, 134, 3809},      /* LavenderBlush4 */
    {124, 252, 0, 3824},        /* lawn green */
    {124, 252, 0, 3835},        /* LawnGreen */
    {255, 250, 205, 3845},      /* lemon chiffon */
    {255, 250, 205, 3859},      /* LemonChiffon */
    {255, 250, 205, 3872},      /* LemonChiffon1 */
    {238, 233, 191, 3886},      /* LemonChiffon2 */
    {205, 201, 165, 3900},      /* LemonChiffon3 */
    {139, 137, 112, 3914},      /* LemonChiffon4 */
    {173, 216, 230, 3928},      /* light blue */
    {240, 128, 128, 3939},      /* light coral */
    {224, 255, 255, 3951},      /* light cyan */
    {238, 221, 130, 3962},      /* light goldenrod */
    {250, 250, 210, 3978},      /* light goldenrod yellow */
    {211, 211, 211, 4001},      /* light gray */
    {144, 238, 144, 4012},      /* light green */
    {211, 211, 211, 4024},      /* light grey */
    {255, 182, 193, 4035},      /* light pink */
    {255, 160, 122, 4046},      /* light salmon */
    {32, 178, 170, 4059},       /* light sea green */
    {135, 206, 250, 4075},      /* light sky blue */
    {132, 112, 255, 4090},      /* light slate blue */
    {119, 136, 153, 4107},      /* light slate gray */
    {119, 136, 153, 4124},      /* light slate grey */
    {176, 196, 222, 4141},      /* light steel blue */
    {255, 255, 224, 4158},      /* light yellow */
    {173, 216, 230, 4171},      /* LightBlue */
    {191, 239, 255, 4181},      /* LightBlue1 */
    {178, 223, 238, 4192},      /* LightBlue2 */
    {154, 192, 205, 4203},      /* LightBlue3 */
    {104, 131, 139, 4214},      /* LightBlue4 */
    {240, 128, 128, 4225},      /* LightCoral */
    {224, 255, 255, 4236},      /* LightCyan */
    {224, 255, 255, 4246},      /* LightCyan1 */
    {209, 238, 238, 4257},      /* LightCyan2 */
    {180, 205, 205, 4268},      /* LightCyan3 */
    {122, 139, 139, 4279},      /* LightCyan4 */
    {238, 221, 130, 4290},      /* LightGoldenrod */
    {255, 236, 139, 4305},      /* LightGoldenrod1 */
    {238, 220, 130, 4321},      /* LightGoldenrod2 */
    {205, 190, 112, 4337},      /* LightGoldenrod3 */
    {139, 129, 76, 4353},       /* LightGoldenrod4 */
    {250, 250, 210, 4369},      /* LightGoldenrodYellow */
    {211, 211, 211, 4390},      /* LightGray */
    {144, 238, 144, 4400},      /* LightGreen */
    {211, 211, 211, 4411},      /* LightGrey */
    {255, 182, 193, 4421},      /* LightPink */
    {255, 174, 185, 4431},      /* LightPink1 */
    {238, 162, 173, 4442},      /* LightPink2 */
    {205, 140, 149, 4453},      /* LightPink3 */
    {139, 95, 101, 4464},       /* LightPink4 */
    {255, 160, 122, 4475},      /* LightSalmon */
    {255, 160, 122, 4487},      /* LightSalmon1 */
    {238, 149, 114, 4500},      /* LightSalmon2 */
    {205, 129, 98, 4513},       /* LightSalmon3 */
    {139, 87, 66, 4526},        /* LightSalmon4 */
    {32, 178, 170, 4539},       /* LightSeaGreen */
    {135, 206, 250, 4553},      /* LightSkyBlue */
    {176, 226, 255, 4566},      /* LightSkyBlue1 */
    {164, 211, 238, 4580},      /* LightSkyBlue2 */
    {141, 182, 205, 4594},      /* LightSkyBlue3 */
    {96, 123, 139, 4608},       /* LightSkyBlue4 */
    {132, 112, 255, 4622},      /* LightSlateBlue */
    {119, 136, 153, 4637},      /* LightSlateGray */
    {119, 136, 153, 4652},      /* LightSlateGrey */
    {176, 196, 222, 4667},      /* LightSteelBlue */
    {202, 225, 255, 4682},      /* LightSteelBlue1 */
    {188, 210, 238, 4698},      /* LightSteelBlue2 */
    {162, 181, 205, 4714},      /* LightSteelBlue3 */
    {110, 123, 139, 4730},      /* LightSteelBlue4 */
    {255, 255, 224, 4746},      /* LightYellow */
    {255, 255, 224, 4758},      /* LightYellow1 */
    {238, 238, 209, 4771},      /* LightYellow2 */
    {205, 205, 180, 4784},      /* LightYellow3 */
    {139, 139, 122, 4797},      /* LightYellow4 */
    {0, 255, 0, 4810},          /* lime */
    {50, 205, 50, 4815},        /* lime green */
    {50, 205, 50, 4826},        /* LimeGreen */
    {250, 240, 230, 4836},      /* linen */
    {255, 0, 255, 4842},        /* magenta */
    {255, 0, 255, 4850},        /* magenta1 */
    {238, 0, 238, 4859},        /* magenta2 */
    {205, 0, 205, 4868},        /* magenta3 */
    {139, 0, 139, 4877},        /* magenta4 */
    {176, 48, 96, 4886},        /* maroon */
    {255, 52, 179, 4893},       /* maroon1 */
    {238, 48, 167, 4901},       /* maroon2 */
    {205, 41, 144, 4909},       /* maroon3 */
    {139, 28, 98, 4917},        /* maroon4 */
    {102, 205, 170, 4925},      /* medium aquamarine */
    {0, 0, 205, 4943},          /* medium blue */
    {186, 85, 211, 4955},       /* medium orchid */
    {147, 112, 219, 4969},      /* medium purple */
    {60, 179, 113, 4983},       /* medium sea green */
    {123, 104, 238, 5000},      /* medium slate blue */
    {0, 250, 154, 5018},        /* medium spring green */
    {72, 209, 204, 5038},       /* medium turquoise */
    {199, 21, 133, 5055},       /* medium violet red */
    {102, 205, 170, 5073},      /* MediumAquamarine */
    {0, 0, 205, 5090},          /* MediumBlue */
    {186, 85, 211, 5101},       /* MediumOrchid */
    {224, 102, 255, 5114},      /* MediumOrchid1 */
    {209, 95, 238, 5128},       /* MediumOrchid2 */
    {180, 82, 205, 5142},       /* MediumOrchid3 */
    {122, 55, 139, 5156},       /* MediumOrchid4 */
    {147, 112, 219, 5170},      /* MediumPurple */
    {171, 130, 255, 5183},      /* MediumPurple1 */
    {159, 121, 238, 5197},      /* MediumPurple2 */
    {137, 104, 205, 5211},      /* MediumPurple3 */
    {93, 71, 139, 5225},        /* MediumPurple4 */
    {60, 179, 113, 5239},       /* MediumSeaGreen */
    {123, 104, 238, 5254},      /* MediumSlateBlue */
    {0, 250, 154, 5270},        /* MediumSpringGreen */
    {72, 209, 204, 5288},       /* MediumTurquoise */
    {199, 21, 133, 5304},       /* MediumVioletRed */
    {25, 25, 112, 5320},        /* midnight blue */
    {25, 25, 112, 5334},        /* MidnightBlue */
    {245, 255, 250, 5347},      /* mint cream */
    {245, 255, 250, 5358},      /* MintCream */
    {255, 228, 225, 5368},      /* misty rose */
    {255, 228, 225, 5379},      /* MistyRose */
    {255, 228, 225, 5389},      /* MistyRose1 */
    {238, 213, 210, 5400},      /* MistyRose2 */
    {205, 183, 181, 5411},      /* MistyRose3 */
    {139, 125, 123, 5422},      /* MistyRose4 */
    {255, 228, 181, 5433},      /* moccasin */
    {255, 222, 173, 5442},      /* navajo white */
    {255, 222, 173, 5455},      /* NavajoWhite */
    {255, 222, 173, 5467},      /* NavajoWhite1 */
    {238, 207, 161, 5480},      /* NavajoWhite2 */
    {205, 179, 139, 5493},      /* NavajoWhite3 */
    {139, 121, 94, 5506},       /* NavajoWhite4 */
    {0, 0, 128, 5519},          /* navy */
    {0, 0, 128, 5524},          /* navy blue */
    {0, 0, 128, 5534},          /* NavyBlue */
    {253, 245, 230, 5543},      /* old lace */
    {253, 245, 230, 5552},      /* OldLace */
    {128, 128, 0, 5560},        /* olive */
    {107, 142, 35, 5566},       /* olive drab */
    {107, 142, 35, 5577},       /* OliveDrab */
    {192, 255, 62, 5587},       /* OliveDrab1 */
    {179, 238, 58, 5598},       /* OliveDrab2 */
    {154, 205, 50, 5609},       /* OliveDrab3 */
    {105, 139, 34, 5620},       /* OliveDrab4 */
    {255, 165, 0, 5631},        /* orange */
    {255, 69, 0, 5638},         /* orange red */
    {255, 165, 0, 5649},        /* orange1 */
    {238, 154, 0, 5657},        /* orange2 */
    {205, 133, 0, 5665},        /* orange3 */
    {139, 90, 0, 5673},         /* orange4 */
    {255, 69, 0, 5681},         /* OrangeRed */
    {255, 69, 0, 5691},         /* OrangeRed1 */
    {238, 64, 0, 5702},         /* OrangeRed2 */
    {205, 55, 0, 5713},         /* OrangeRed3 */
    {139, 37, 0, 5724},         /* OrangeRed4 */
    {218, 112, 214, 5735},      /* orchid */
    {255, 131, 250, 5742},      /* orchid1 */
    {238, 122, 233, 5750},      /* orchid2 */
    {205, 105, 201, 5758},      /* orchid3 */
    {139, 71, 137, 5766},       /* orchid4 */
    {238, 232, 170, 5774},      /* pale goldenrod */
    {152, 251, 152, 5789},      /* pale green */
    {175, 238, 238, 5800},      /* pale turquoise */
    {219, 112, 147, 5815},      /* pale violet red */
    {238, 232, 170, 5831},      /* PaleGoldenrod */
    {152, 251, 152, 5845},      /* PaleGreen */
    {154, 255, 154, 5855},      /* PaleGreen1 */
    {144, 238, 144, 5866},      /* PaleGreen2 */
    {124, 205, 124, 5877},      /* PaleGreen3 */
    {84, 139, 84, 5888},        /* PaleGreen4 */
    {175, 238, 238, 5899},      /* PaleTurquoise */
    {187, 255, 255, 5913},      /* PaleTurquoise1 */
    {174, 238, 238, 5928},      /* PaleTurquoise2 */
    {150, 205, 205, 5943},      /* PaleTurquoise3 */
    {102, 139, 139, 5958},      /* PaleTurquoise4 */
    {219, 112, 147, 5973},      /* PaleVioletRed */
    {255, 130, 171, 5987},      /* PaleVioletRed1 */
    {238, 121, 159, 6002},      /* PaleVioletRed2 */
    {205, 104, 137, 6017},      /* PaleVioletRed3 */
    {139, 71, 93, 6032},        /* PaleVioletRed4 */
    {255, 239, 213, 6047},      /* papaya whip */
    {255, 239, 213, 6059},      /* PapayaWhip */
    {255, 218, 185, 6070},      /* peach puff */
    {255, 218, 185, 6081},      /* PeachPuff */
    {255, 218, 185, 6091},      /* PeachPuff1 */
    {238, 203, 173, 6102},      /* PeachPuff2 */
    {205, 175, 149, 6113},      /* PeachPuff3 */
    {139, 119, 101, 6124},      /* PeachPuff4 */
    {205, 133, 63, 6135},       /* peru */
    {255, 192, 203, 6140},      /* pink */
    {255, 181, 197, 6145},      /* pink1 */
    {238, 169, 184, 6151},      /* pink2 */
    {205, 145, 158, 6157},      /* pink3 */
    {139, 99, 108, 6163},       /* pink4 */
    {221, 160, 221, 6169},      /* plum */
    {255, 187, 255, 6174},      /* plum1 */
    {238, 174, 238, 6180},      /* plum2 */
    {205, 150, 205, 6186},      /* plum3 */
    {139, 102, 139, 6192},      /* plum4 */
    {176, 224, 230, 6198},      /* powder blue */
    {176, 224, 230, 6210},      /* PowderBlue */
    {160, 32, 240, 6221},       /* purple */
    {155, 48, 255, 6228},       /* purple1 */
    {145, 44, 238, 6236},       /* purple2 */
    {125, 38, 205, 6244},       /* purple3 */
    {85, 26, 139, 6252},        /* purple4 */
    {102, 51, 153, 6260},       /* rebecca purple */
    {102, 51, 153, 6275},       /* RebeccaPurple */
    {255, 0, 0, 6289},          /* red */
    {255, 0, 0, 6293},          /* red1 */
    {238, 0, 0, 6298},          /* red2 */
    {205, 0, 0, 6303},          /* red3 */
    {139, 0, 0, 6308},          /* red4 */
    {188, 143, 143, 6313},      /* rosy brown */
    {188, 143, 143, 6324},      /* RosyBrown */
    {255, 193, 193, 6334},      /* RosyBrown1 */
    {238, 180, 180, 6345},      /* RosyBrown2 */
    {205, 155, 155, 6356},      /* RosyBrown3 */
    {139, 105, 105, 6367},      /* RosyBrown4 */
    {65, 105, 225, 6378},       /* royal blue */
    {65, 105, 225, 6389},       /* RoyalBlue */
    {72, 118, 255, 6399},       /* RoyalBlue1 */
    {67, 110, 238, 6410},       /* RoyalBlue2 */
    {58, 95, 205, 6421},        /* RoyalBlue3 */
    {39, 64, 139, 6432},        /* RoyalBlue4 */
    {139, 69, 19, 6443},        /* saddle brown */
    {139, 69, 19, 6456},        /* SaddleBrown */
    {250, 128, 114, 6468},      /* salmon */
    {255, 140, 105, 6475},      /* salmon1 */
    {238, 130, 98, 6483},       /* salmon2 */
    {205, 112, 84, 6491},       /* salmon3 */
    {139, 76, 57, 6499},        /* salmon4 */
    {244, 164, 96, 6507},       /* sandy brown */
    {244, 164, 96, 6519},       /* SandyBrown */
    {46, 139, 87, 6530},        /* sea green */
    {46, 139, 87, 6540},        /* SeaGreen */
    {84, 255, 159, 6549},       /* SeaGreen1 */
    {78, 238, 148, 6559},       /* SeaGreen2 */
    {67, 205, 128, 6569},       /* SeaGreen3 */
    {46, 139, 87, 6579},        /* SeaGreen4 */
    {255, 245, 238, 6589},      /* seashell */
    {255, 245, 238, 6598},      /* seashell1 */
    {238, 229, 222, 6608},      /* seashell2 */
    {205, 197, 191, 6618},      /* seashell3 */
    {139, 134, 130, 6628},      /* seashell4 */
    {160, 82, 45, 6638},        /* sienna */
    {255, 130, 71, 6645},       /* sienna1 */
    {238, 121, 66, 6653},       /* sienna2 */
    {205, 104, 57, 6661},       /* sienna3 */
    {139, 71, 38, 6669},        /* sienna4 */
    {192, 192, 192, 6677},      /* silver */
    {135, 206, 235, 6684},      /* sky blue */
    {135, 206, 235, 6693},      /* SkyBlue */
    {135, 206, 255, 6701},      /* SkyBlue1 */
    {126, 192, 238, 6710},      /* SkyBlue2 */
    {108, 166, 205, 6719},      /* SkyBlue3 */
    {74, 112, 139, 6728},       /* SkyBlue4 */
    {106, 90, 205, 6737},       /* slate blue */
    {112, 128, 144, 6748},      /* slate gray */
    {112, 128, 144, 6759},      /* slate grey */
    {106, 90, 205, 6770},       /* SlateBlue */
    {131, 111, 255, 6780},      /* SlateBlue1 */
    {122, 103, 238, 6791},      /* SlateBlue2 */
    {105, 89, 205, 6802},       /* SlateBlue3 */
    {71, 60, 139, 6813},        /* SlateBlue4 */
    {112, 128, 144, 6824},      /* SlateGray */
    {198, 226, 255, 6834},      /* SlateGray1 */
    {185, 211, 238, 6845},      /* SlateGray2 */
    {159, 182, 205, 6856},      /* SlateGray3 */
    {108, 123, 139, 6867},      /* SlateGray4 */
    {112, 128, 144, 6878},      /* SlateGrey */
    {255, 250, 250, 6888},      /* snow */
    {255, 250, 250, 6893},      /* snow1 */
    {238, 233, 233, 6899},      /* snow2 */
    {205, 201, 201, 6905},      /* snow3 */
    {139, 137, 137, 6911},      /* snow4 */
    {0, 255, 127, 6917},        /* spring green */
    {0, 255, 127, 6930},        /* SpringGreen */
    {0, 255, 127, 6942},        /* SpringGreen1 */
    {0, 238, 118, 6955},        /* SpringGreen2 */
    {0, 205, 102, 6968},        /* SpringGreen3 */
    {0, 139, 69, 6981},         /* SpringGreen4 */
    {70, 130, 180, 6994},       /* steel blue */
    {70, 130, 180, 7005},       /* SteelBlue */
    {99, 184, 255, 7015},       /* SteelBlue1 */
    {92, 172, 238, 7026},       /* SteelBlue2 */
    {79, 148, 205, 7037},       /* SteelBlue3 */
    {54, 100, 139, 7048},       /* SteelBlue4 */
    {210, 180, 140, 7059},      /* tan */
    {255, 165, 79, 7063},       /* tan1 */
    {238, 154, 73, 7068},       /* tan2 */
    {205, 133, 63, 7073},       /* tan3 */
    {139, 90, 43, 7078},        /* tan4 */
    {0, 128, 128, 7083},        /* teal */
    {216, 191, 216, 7088},      /* thistle */
    {255, 225, 255, 7096},      /* thistle1 */
    {238, 210, 238, 7105},      /* thistle2 */
    {205, 181, 205, 7114},      /* thistle3 */
    {139, 123, 139, 7123},      /* thistle4 */
    {255, 99, 71, 7132},        /* tomato */
    {255, 99, 71, 7139},        /* tomato1 */
    {238, 92, 66, 7147},        /* tomato2 */
    {205, 79, 57, 7155},        /* tomato3 */
    {139, 54, 38, 7163},        /* tomato4 */
    {64, 224, 208, 7171},       /* turquoise */
    {0, 245, 255, 7181},        /* turquoise1 */
    {0, 229, 238, 7192},        /* turquoise2 */
    {0, 197, 205, 7203},        /* turquoise3 */
    {0, 134, 139, 7214},        /* turquoise4 */
    {238, 130, 238, 7225},      /* violet */
    {208, 32, 144, 7232},       /* violet red */
    {208, 32, 144, 7243},       /* VioletRed */
    {255, 62, 150, 7253},       /* VioletRed1 */
    {238, 58, 140, 7264},       /* VioletRed2 */
    {205, 50, 120, 7275},       /* VioletRed3 */
    {139, 34, 82, 7286},        /* VioletRed4 */
    {128, 128, 128, 7297},      /* web gray */
    {0, 128, 0, 7306},          /* web green */
    {128, 128, 128, 7316},      /* web grey */
    {128, 0, 0, 7325},          /* web maroon */
    {128, 0, 128, 7336},        /* web purple */
    {128, 128, 128, 7347},      /* WebGray */
    {0, 128, 0, 7355},          /* WebGreen */
    {128, 128, 128, 7364},      /* WebGrey */
    {128, 0, 0, 7372},          /* WebMaroon */
    {128, 0, 128, 7382},        /* WebPurple */
    {245, 222, 179, 7392},      /* wheat */
    {255, 231, 186, 7398},      /* wheat1 */
    {238, 216, 174, 7405},      /* wheat2 */
    {205, 186, 150, 7412},      /* wheat3 */
    {139, 126, 102, 7419},      /* wheat4 */
    {255, 255, 255, 7426},      /* white */
    {245, 245, 245, 7432},      /* white smoke */
    {245, 245, 245, 7444},      /* WhiteSmoke */
    {190, 190, 190, 7455},      /* x11 gray */
    {0, 255, 0, 7464},          /* x11 green */
    {190, 190, 190, 7474},      /* x11 grey */
    {176, 48, 96, 7483},        /* x11 maroon */
    {160, 32, 240, 7494},       /* x11 purple */
    {190, 190, 190, 7505},      /* X11Gray */
    {0, 255, 0, 7513},          /* X11Green */
    {190, 190, 190, 7522},      /* X11Grey */
    {176, 48, 96, 7530},        /* X11Maroon */
    {160, 32, 240, 7540},       /* X11Purple */
    {255, 255, 0, 7550},        /* yellow */
    {154, 205, 50, 7557},       /* yellow green */
    {255, 255, 0, 7570},        /* yellow1 */
    {238, 238, 0, 7578},        /* yellow2 */
    {205, 205, 0, 7586},        /* yellow3 */
    {139, 139, 0, 7594},        /* yellow4 */
    {154, 205, 50, 7602},       /* YellowGreen */
};

Bool
OsLookupColor(int screen,
              char *name,
              unsigned int len,
              unsigned short *pred,
              unsigned short *pgreen, unsigned short *pblue)
{
    const BuiltinColor *c;
    int low, mid, high;
    int r;

    low = 0;
    high = ARRAY_SIZE(BuiltinColors) - 1;
    while (high >= low) {
        mid = (low + high) / 2;
        c = &BuiltinColors[mid];
        r = strncasecmp(&BuiltinColorNames[c->name], name, len);
        if (r == 0 && len == strlen(&BuiltinColorNames[c->name])) {
            *pred = c->red * 0x101;
            *pgreen = c->green * 0x101;
            *pblue = c->blue * 0x101;
            return TRUE;
        }
        if (r < 0)
            low = mid + 1;
        else
            high = mid - 1;
    }
    return FALSE;
}
