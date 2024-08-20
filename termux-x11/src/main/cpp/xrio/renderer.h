#pragma once

#include "engine.h"
#include "framebuffer.h"

enum XrConfigFloat {
    // 2D canvas positioning
    CONFIG_CANVAS_DISTANCE,
    CONFIG_MENU_PITCH,
    CONFIG_MENU_YAW,
    CONFIG_RECENTER_YAW,

    CONFIG_FLOAT_MAX
};

enum XrConfigInt {
    // switching between modes
    CONFIG_MODE,
    CONFIG_PASSTHROUGH,
    CONFIG_SBS,
    // viewport setup
    CONFIG_VIEWPORT_WIDTH,
    CONFIG_VIEWPORT_HEIGHT,
    // render status
    CONFIG_CURRENT_FBO,

    // end
    CONFIG_INT_MAX
};

enum XrRenderMode {
    RENDER_MODE_MONO_SCREEN,
    RENDER_MODE_STEREO_SCREEN,
    RENDER_MODE_MONO_6DOF,
    RENDER_MODE_STEREO_6DOF
};

struct XrRenderer {
    bool SessionActive;
    bool SessionFocused;
    bool Initialized;
    bool StageSupported;
    float ConfigFloat[CONFIG_FLOAT_MAX];
    int ConfigInt[CONFIG_INT_MAX];

    struct XrFramebuffer Framebuffer[XrMaxNumEyes];

    int LayerCount;
    XrCompositorLayer Layers[XrMaxLayerCount];
    XrPassthroughFB Passthrough;
    XrPassthroughLayerFB PassthroughLayer;
    bool PassthroughRunning;
    XrViewConfigurationProperties ViewportConfig;
    XrViewConfigurationView ViewConfig[XrMaxNumEyes];

    XrFovf Fov;
    XrView* Projections;
    XrPosef InvertedViewPose[2];
    XrVector3f HmdOrientation;
};

void XrRendererInit(struct XrEngine* engine, struct XrRenderer* renderer);
void XrRendererDestroy(struct XrEngine* engine, struct XrRenderer* renderer);
void XrRendererGetResolution(struct XrEngine* engine, struct XrRenderer* renderer, int* pWidth, int* pHeight);

bool XrRendererInitFrame(struct XrEngine* engine, struct XrRenderer* renderer);
void XrRendererBeginFrame(struct XrRenderer* renderer, int fbo_index);
void XrRendererEndFrame(struct XrRenderer* renderer);
void XrRendererFinishFrame(struct XrEngine* engine, struct XrRenderer* renderer);

void XrRendererBindFramebuffer(struct XrRenderer* renderer);
void XrRendererRecenter(struct XrEngine* engine, struct XrRenderer* renderer);

void XrRendererHandleSessionStateChanges(struct XrEngine* engine, struct XrRenderer* renderer, XrSessionState state);
void XrRendererHandleXrEvents(struct XrEngine* engine, struct XrRenderer* renderer);
void XrRendererUpdateStageBounds(struct XrEngine* engine);
