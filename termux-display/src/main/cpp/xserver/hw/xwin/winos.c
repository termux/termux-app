/*
 * Copyright (c) 2010-2014 Colin Harrison All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name(s) of the above copyright
 * holders shall not be used in advertising or otherwise to promote the sale,
 * use or other dealings in this Software without prior written authorization.
 *
 * Author: Colin Harrison
 */

#ifdef HAVE_XWIN_CONFIG_H
#include <xwin-config.h>
#endif
#include "win.h"

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);

static const char*
IsWow64(void)
{
#ifdef __x86_64__
    return " (64-bit)";
#else
    WINBOOL bIsWow64;
    LPFN_ISWOW64PROCESS fnIsWow64Process =
        (LPFN_ISWOW64PROCESS) GetProcAddress(GetModuleHandle(TEXT("kernel32")),
                                             "IsWow64Process");
    if (NULL != fnIsWow64Process) {
        if (fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
            return bIsWow64 ? " (WoW64)" : " (32-bit)";
    }

    /* OS doesn't support IsWow64Process() */
    return "";
#endif
}

/*
 * Report the OS version
 */

void
winOS(void)
{
    OSVERSIONINFOEX osvi = {0};

    /* Get operating system version information */
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    GetVersionEx((LPOSVERSIONINFO)&osvi);

    ErrorF("OS: Windows NT %d.%d build %d%s\n",
           (int)osvi.dwMajorVersion, (int)osvi.dwMinorVersion,
           (int)osvi.dwBuildNumber, IsWow64());
}
