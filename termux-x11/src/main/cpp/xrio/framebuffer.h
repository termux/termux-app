#pragma once

#include "engine.h"

struct XrFramebuffer {
    int Width;
    int Height;
    bool Acquired;
    XrSwapchain Handle;

    uint32_t SwapchainIndex;
    uint32_t SwapchainLength;
    void* SwapchainImage;

    unsigned int* GLDepthBuffers;
    unsigned int* GLFrameBuffers;
};

bool XrFramebufferCreate(struct XrFramebuffer *framebuffer, XrSession session, int width, int height);
void XrFramebufferDestroy(struct XrFramebuffer *framebuffer);

void XrFramebufferAcquire(struct XrFramebuffer *framebuffer);
void XrFramebufferRelease(struct XrFramebuffer *framebuffer);
void XrFramebufferSetCurrent(struct XrFramebuffer *framebuffer);

#if XR_USE_GRAPHICS_API_OPENGL_ES
bool XrFramebufferCreateGL(struct XrFramebuffer *framebuffer, XrSession session, int width, int height);
#endif
