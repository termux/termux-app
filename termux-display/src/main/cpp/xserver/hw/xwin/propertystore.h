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

#ifndef PROPERTYSTORE_H
#define PROPERTYSTORE_H

#include <windows.h>

#ifdef __MINGW64_VERSION_MAJOR
/* If we are using headers from mingw-w64 project, it provides the PSDK headers this needs ... */
#include <propkey.h>
#include <propsys.h>
#else /*  !__MINGW64_VERSION_MAJOR */
/* ... otherwise, we need to define all this stuff ourselves */

typedef struct _tagpropertykey {
    GUID fmtid;
    DWORD pid;
} PROPERTYKEY;

#define REFPROPERTYKEY const PROPERTYKEY *
#define REFPROPVARIANT const PROPVARIANT *

WINOLEAPI PropVariantClear(PROPVARIANT *pvar);

#ifdef INTERFACE
#undef INTERFACE
#endif

#define INTERFACE IPropertyStore
DECLARE_INTERFACE_(IPropertyStore, IUnknown)
{
    STDMETHOD(QueryInterface) (THIS_ REFIID, PVOID *) PURE;
    STDMETHOD_(ULONG, AddRef) (THIS) PURE;
    STDMETHOD_(ULONG, Release) (THIS) PURE;
    STDMETHOD(GetCount) (THIS_ DWORD) PURE;
    STDMETHOD(GetAt) (THIS_ DWORD, PROPERTYKEY) PURE;
    STDMETHOD(GetValue) (THIS_ REFPROPERTYKEY, PROPVARIANT) PURE;
    STDMETHOD(SetValue) (THIS_ REFPROPERTYKEY, REFPROPVARIANT) PURE;
    STDMETHOD(Commit) (THIS) PURE;
};

#undef INTERFACE
typedef IPropertyStore *LPPROPERTYSTORE;

DEFINE_GUID(IID_IPropertyStore, 0x886d8eeb, 0x8cf2, 0x4446, 0x8d, 0x02, 0xcd,
            0xba, 0x1d, 0xbd, 0xcf, 0x99);

#ifdef INITGUID
#define DEFINE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) GUID_EXT const PROPERTYKEY DECLSPEC_SELECTANY name = { { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }, pid }
#else
#define DEFINE_PROPERTYKEY(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, pid) GUID_EXT const PROPERTYKEY name
#endif

DEFINE_PROPERTYKEY(PKEY_AppUserModel_ID, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0,
                   0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 5);

#endif /* !__MINGW64_VERSION_MAJOR */

typedef HRESULT(__stdcall * SHGETPROPERTYSTOREFORWINDOWPROC) (HWND, REFIID,
                                                              void **);

#endif
