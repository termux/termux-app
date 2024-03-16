/*
 * Copyright (C) 2011 Tobias Häußler
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

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif

#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xwindows.h>
#include <pthread.h>
#include "winwindow.h"
#include "os.h"
#include "winmsg.h"

#include <shlwapi.h>

#define INITGUID
#include "initguid.h"
#include "propertystore.h"
#undef INITGUID

static HMODULE g_hmodShell32Dll = NULL;
static SHGETPROPERTYSTOREFORWINDOWPROC g_pSHGetPropertyStoreForWindow = NULL;

void
winPropertyStoreInit(void)
{
    /*
       Load library and get function pointer to SHGetPropertyStoreForWindow()

       SHGetPropertyStoreForWindow is only supported since Windows 7. On previous
       versions the pointer will be NULL and taskbar grouping is not supported.
       winSetAppUserModelID() will do nothing in this case.
     */
    g_hmodShell32Dll = LoadLibrary("shell32.dll");
    if (g_hmodShell32Dll == NULL) {
        ErrorF("winPropertyStoreInit - Could not load shell32.dll\n");
        return;
    }

    g_pSHGetPropertyStoreForWindow =
        (SHGETPROPERTYSTOREFORWINDOWPROC) GetProcAddress(g_hmodShell32Dll,
                                                         "SHGetPropertyStoreForWindow");
    if (g_pSHGetPropertyStoreForWindow == NULL) {
        ErrorF
            ("winPropertyStoreInit - Could not get SHGetPropertyStoreForWindow address\n");
        return;
    }
}

void
winPropertyStoreDestroy(void)
{
    if (g_hmodShell32Dll != NULL) {
        FreeLibrary(g_hmodShell32Dll);
        g_hmodShell32Dll = NULL;
        g_pSHGetPropertyStoreForWindow = NULL;
    }
}

void
winSetAppUserModelID(HWND hWnd, const char *AppID)
{
    PROPVARIANT pv;
    IPropertyStore *pps = NULL;
    HRESULT hr;

    if (g_pSHGetPropertyStoreForWindow == NULL) {
        return;
    }

    winDebug("winSetAppUserMOdelID - hwnd 0x%p appid '%s'\n", hWnd, AppID);

    hr = g_pSHGetPropertyStoreForWindow(hWnd, &IID_IPropertyStore,
                                        (void **) &pps);
    if (SUCCEEDED(hr) && pps) {
        memset(&pv, 0, sizeof(PROPVARIANT));
        if (AppID) {
            pv.vt = VT_LPWSTR;
            hr = SHStrDupA(AppID, &pv.pwszVal);
        }

        if (SUCCEEDED(hr)) {
            pps->lpVtbl->SetValue(pps, &PKEY_AppUserModel_ID, &pv);
            PropVariantClear(&pv);
        }
        pps->lpVtbl->Release(pps);
    }
}
