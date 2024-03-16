/*

Copyright 1993, 1998  The Open Group
Copyright (C) Colin Harrison 2005-2008

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall
not be used in advertising or otherwise to promote the sale, use or
other dealings in this Software without prior written authorization
from The Open Group.

*/

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include "win.h"
#include "winmonitors.h"

/*
 * getMonitorInfo - callback function used to return information from the enumeration of monitors attached
 */

static
WINBOOL CALLBACK
getMonitorInfo(HMONITOR hMonitor, HDC hdc, LPRECT rect, LPARAM _data)
{
    struct GetMonitorInfoData *data = (struct GetMonitorInfoData *) _data;

    // only get data for monitor number specified in <data>
    data->monitorNum++;
    if (data->monitorNum == data->requestedMonitor) {
        data->bMonitorSpecifiedExists = TRUE;
        data->monitorOffsetX = rect->left;
        data->monitorOffsetY = rect->top;
        data->monitorHeight = rect->bottom - rect->top;
        data->monitorWidth = rect->right - rect->left;
        data->monitorHandle = hMonitor;
        return FALSE;
    }
    return TRUE;
}

Bool
QueryMonitor(int i, struct GetMonitorInfoData *data)
{
    /* prepare data */
    if (data == NULL)
        return FALSE;
    memset(data, 0, sizeof(*data));
    data->requestedMonitor = i;

    /* query information */
    EnumDisplayMonitors(NULL, NULL, getMonitorInfo, (LPARAM) data);

    return data->bMonitorSpecifiedExists;
}
