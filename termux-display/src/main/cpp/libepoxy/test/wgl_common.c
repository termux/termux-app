/*
 * Copyright © 2013 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <epoxy/wgl.h>
#include "wgl_common.h"

static int (*test_callback)(HDC hdc);

static void
setup_pixel_format(HDC hdc)
{
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,
        PFD_SUPPORT_OPENGL |
        PFD_DRAW_TO_WINDOW |
        PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA,
        32,
        0, 0, 0, 0, 0, 0,
        0,
        0,
        0,
        0, 0, 0, 0,
        16,
        0,
        0,
        PFD_MAIN_PLANE,
        0,
        0, 0, 0,
    };
    int pixel_format;

    pixel_format = ChoosePixelFormat(hdc, &pfd);
    if (!pixel_format) {
        fputs("ChoosePixelFormat failed.\n", stderr);
        exit(1);
    }

    if (SetPixelFormat(hdc, pixel_format, &pfd) != TRUE) {
        fputs("SetPixelFormat() failed.\n", stderr);
        exit(1);
    }
}

static LRESULT CALLBACK
window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HDC hdc = GetDC(hwnd);
    int ret;

    switch (message) {
    case WM_CREATE:
        setup_pixel_format(hdc);
        ret = test_callback(hdc);
        ReleaseDC(hwnd, hdc);
        exit(ret);
        return 0;
    default:
        return DefWindowProc(hwnd, message, wparam, lparam);
    }
}

void
make_window_and_test(int (*callback)(HDC hdc))
{
    const char *class_name = "epoxy";
    const char *window_name = "epoxy";
    int width = 150;
    int height = 150;
    HWND hwnd;
    HINSTANCE hcurrentinst = NULL;
    WNDCLASS window_class;
    MSG msg;

    test_callback = callback;

    memset(&window_class, 0, sizeof(window_class));
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = window_proc;
    window_class.cbClsExtra = 0;
    window_class.cbWndExtra = 0;
    window_class.hInstance = hcurrentinst;
    window_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    window_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    window_class.lpszMenuName = NULL;
    window_class.lpszClassName = class_name;
    if (!RegisterClass(&window_class)) {
        fputs("Failed to register window class\n", stderr);
        exit(1);
    }

    /* create window */
    hwnd = CreateWindow(class_name, window_name,
                        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                        0, 0, width, height,
                        NULL, NULL, hcurrentinst, NULL);

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}
