#include <malloc.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "engine.h"
#include "math.h"
#include "renderer.h"

#define DECL_PFN(pfn) PFN_##pfn pfn = NULL
#define INIT_PFN(pfn) OXR(xrGetInstanceProcAddr(engine->Instance, #pfn, (PFN_xrVoidFunction*)(&pfn)))

DECL_PFN(xrCreatePassthroughFB);
DECL_PFN(xrDestroyPassthroughFB);
DECL_PFN(xrPassthroughStartFB);
DECL_PFN(xrPassthroughPauseFB);
DECL_PFN(xrCreatePassthroughLayerFB);
DECL_PFN(xrDestroyPassthroughLayerFB);
DECL_PFN(xrPassthroughLayerPauseFB);
DECL_PFN(xrPassthroughLayerResumeFB);

void XrRendererInit(struct XrEngine* engine, struct XrRenderer* renderer) {
    if (renderer->Initialized) {
        XrRendererDestroy(engine, renderer);
    }
    memset(renderer, 0, sizeof(renderer));

    if (engine->PlatformFlag[PLATFORM_EXTENSION_PASSTHROUGH]) {
        INIT_PFN(xrCreatePassthroughFB);
        INIT_PFN(xrDestroyPassthroughFB);
        INIT_PFN(xrPassthroughStartFB);
        INIT_PFN(xrPassthroughPauseFB);
        INIT_PFN(xrCreatePassthroughLayerFB);
        INIT_PFN(xrDestroyPassthroughLayerFB);
        INIT_PFN(xrPassthroughLayerPauseFB);
        INIT_PFN(xrPassthroughLayerResumeFB);
    }

    int eyeW, eyeH;
    XrRendererGetResolution(engine, renderer, &eyeW, &eyeH);
    renderer->ConfigInt[CONFIG_VIEWPORT_WIDTH] = eyeW;
    renderer->ConfigInt[CONFIG_VIEWPORT_HEIGHT] = eyeH;

    // Get the viewport configuration info for the chosen viewport configuration type.
    renderer->ViewportConfig.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
    OXR(xrGetViewConfigurationProperties(engine->Instance, engine->SystemId,
                                         XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,
                                         &renderer->ViewportConfig));

    uint32_t num_spaces = 0;
    OXR(xrEnumerateReferenceSpaces(engine->Session, 0, &num_spaces, NULL));
    XrReferenceSpaceType* spaces = (XrReferenceSpaceType*)malloc(num_spaces * sizeof(XrReferenceSpaceType));
    OXR(xrEnumerateReferenceSpaces(engine->Session, num_spaces, &num_spaces, spaces));

    for (uint32_t i = 0; i < num_spaces; i++) {
        if (spaces[i] == XR_REFERENCE_SPACE_TYPE_STAGE) {
            renderer->StageSupported = true;
            break;
        }
    }

    free(spaces);

    if (engine->CurrentSpace == XR_NULL_HANDLE) {
        XrRendererRecenter(engine, renderer);
    }

    renderer->Projections = (XrView*)(malloc(XrMaxNumEyes * sizeof(XrView)));
    for (int eye = 0; eye < XrMaxNumEyes; eye++) {
        memset(&renderer->Projections[eye], 0, sizeof(XrView));
        renderer->Projections[eye].type = XR_TYPE_VIEW;
    }

    // Create framebuffers.
    int width = renderer->ViewConfig[0].recommendedImageRectWidth;
    int height = renderer->ViewConfig[0].recommendedImageRectHeight;
    for (int i = 0; i < XrMaxNumEyes; i++) {
        XrFramebufferCreate(&renderer->Framebuffer[i], engine->Session, width, height);
    }

    if (engine->PlatformFlag[PLATFORM_EXTENSION_PASSTHROUGH]) {
        XrPassthroughCreateInfoFB ptci = {XR_TYPE_PASSTHROUGH_CREATE_INFO_FB};
        XrResult result;
        OXR(result = xrCreatePassthroughFB(engine->Session, &ptci, &renderer->Passthrough));

        if (XR_SUCCEEDED(result)) {
            XrPassthroughLayerCreateInfoFB plci = {XR_TYPE_PASSTHROUGH_LAYER_CREATE_INFO_FB};
            plci.passthrough = renderer->Passthrough;
            plci.purpose = XR_PASSTHROUGH_LAYER_PURPOSE_RECONSTRUCTION_FB;
            OXR(xrCreatePassthroughLayerFB(engine->Session, &plci, &renderer->PassthroughLayer));
        }

        OXR(xrPassthroughStartFB(renderer->Passthrough));
        OXR(xrPassthroughLayerResumeFB(renderer->PassthroughLayer));
    }
    renderer->Initialized = true;
}

void XrRendererDestroy(struct XrEngine* engine, struct XrRenderer* renderer) {
    if (!renderer->Initialized) {
        return;
    }

    if (engine->PlatformFlag[PLATFORM_EXTENSION_PASSTHROUGH]) {
        if (renderer->PassthroughRunning) {
            OXR(xrPassthroughLayerPauseFB(renderer->PassthroughLayer));
        }
        OXR(xrPassthroughPauseFB(renderer->Passthrough));
        OXR(xrDestroyPassthroughFB(renderer->Passthrough));
        renderer->Passthrough = XR_NULL_HANDLE;
    }

    for (int i = 0; i < XrMaxNumEyes; i++) {
        XrFramebufferDestroy(&renderer->Framebuffer[i]);
    }
    free(renderer->Projections);
    renderer->Initialized = false;
}


void XrRendererGetResolution(struct XrEngine* engine, struct XrRenderer* renderer, int* pWidth, int* pHeight) {
    static int width = 0;
    static int height = 0;

    if (engine) {
        // Enumerate the viewport configurations.
        uint32_t viewport_config_count = 0;
        OXR(xrEnumerateViewConfigurations(engine->Instance, engine->SystemId, 0,
                                          &viewport_config_count, NULL));

        XrViewConfigurationType* viewport_configs =
                (XrViewConfigurationType*)malloc(viewport_config_count * sizeof(XrViewConfigurationType));

        OXR(xrEnumerateViewConfigurations(engine->Instance, engine->SystemId,
                                          viewport_config_count, &viewport_config_count,
                                          viewport_configs));

        for (uint32_t i = 0; i < viewport_config_count; i++) {
            const XrViewConfigurationType viewport_config_type = viewport_configs[i];

            ALOGV("Viewport configuration type %d", (int)viewport_config_type);

            XrViewConfigurationProperties viewport_config;
            viewport_config.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
            OXR(xrGetViewConfigurationProperties(engine->Instance, engine->SystemId,
                                                 viewport_config_type, &viewport_config));

            uint32_t view_count;
            OXR(xrEnumerateViewConfigurationViews(engine->Instance, engine->SystemId,
                                                  viewport_config_type, 0, &view_count, NULL));

            if (view_count > 0) {
                XrViewConfigurationView* elements =
                        (XrViewConfigurationView*)malloc(view_count * sizeof(XrViewConfigurationView));

                for (uint32_t e = 0; e < view_count; e++) {
                    elements[e].type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
                    elements[e].next = NULL;
                }

                OXR(xrEnumerateViewConfigurationViews(engine->Instance, engine->SystemId,
                                                      viewport_config_type, view_count, &view_count,
                                                      elements));

                // Cache the view config properties for the selected config type.
                if (viewport_config_type == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO) {
                    assert(view_count == XrMaxNumEyes);
                    for (uint32_t e = 0; e < view_count; e++) {
                        renderer->ViewConfig[e] = elements[e];
                    }
                }

                free(elements);
            } else {
                ALOGE("Empty viewport configuration");
            }
        }

        free(viewport_configs);

        *pWidth = width = renderer->ViewConfig[0].recommendedImageRectWidth;
        *pHeight = height = renderer->ViewConfig[0].recommendedImageRectHeight;
    } else {
        // use cached values
        *pWidth = width;
        *pHeight = height;
    }
}

bool XrRendererInitFrame(struct XrEngine* engine, struct XrRenderer* renderer) {
    if (!renderer->Initialized) {
        return false;
    }
    XrRendererHandleXrEvents(engine, renderer);
    if (!renderer->SessionActive) {
        return false;
    }

    XrRendererUpdateStageBounds(engine);

    // Update passthrough
    if (renderer->PassthroughRunning != renderer->ConfigInt[CONFIG_PASSTHROUGH]) {
        if (renderer->ConfigInt[CONFIG_PASSTHROUGH]) {
            OXR(xrPassthroughLayerResumeFB(renderer->PassthroughLayer));
        } else {
            OXR(xrPassthroughLayerPauseFB(renderer->PassthroughLayer));
        }
        renderer->PassthroughRunning = renderer->ConfigInt[CONFIG_PASSTHROUGH];
    }

    XrEngineWaitForFrame(engine);

    XrViewLocateInfo projection_info = {};
    projection_info.type = XR_TYPE_VIEW_LOCATE_INFO;
    projection_info.next = NULL;
    projection_info.viewConfigurationType = renderer->ViewportConfig.viewConfigurationType;
    projection_info.displayTime = engine->PredictedDisplayTime;
    projection_info.space = engine->CurrentSpace;

    XrViewState view_state = {XR_TYPE_VIEW_STATE, NULL};

    uint32_t projection_capacity = XrMaxNumEyes;
    uint32_t projection_count = projection_capacity;

    OXR(xrLocateViews(engine->Session, &projection_info, &view_state, projection_capacity,
                      &projection_count, renderer->Projections));

    // Get the HMD pose, predicted for the middle of the time period during which
    // the new eye images will be displayed. The number of frames predicted ahead
    // depends on the pipeline depth of the engine and the synthesis rate.
    // The better the prediction, the less black will be pulled in at the edges.
    XrFrameBeginInfo begin_frame_info = {};
    begin_frame_info.type = XR_TYPE_FRAME_BEGIN_INFO;
    begin_frame_info.next = NULL;
    OXR(xrBeginFrame(engine->Session, &begin_frame_info));

    renderer->Fov.angleLeft = 0;
    renderer->Fov.angleRight = 0;
    renderer->Fov.angleUp = 0;
    renderer->Fov.angleDown = 0;
    for (int eye = 0; eye < XrMaxNumEyes; eye++) {
        renderer->Fov.angleLeft += renderer->Projections[eye].fov.angleLeft / 2.0f;
        renderer->Fov.angleRight += renderer->Projections[eye].fov.angleRight / 2.0f;
        renderer->Fov.angleUp += renderer->Projections[eye].fov.angleUp / 2.0f;
        renderer->Fov.angleDown += renderer->Projections[eye].fov.angleDown / 2.0f;
        renderer->InvertedViewPose[eye] = renderer->Projections[eye].pose;
    }

    renderer->HmdOrientation = XrQuaternionfEulerAngles(renderer->InvertedViewPose[0].orientation);
    renderer->LayerCount = 0;
    memset(renderer->Layers, 0, sizeof(XrCompositorLayer) * XrMaxLayerCount);
    return true;
}

void XrRendererBeginFrame(struct XrRenderer* renderer, int fbo_index) {
    renderer->ConfigInt[CONFIG_CURRENT_FBO] = fbo_index;
    XrFramebufferAcquire(&renderer->Framebuffer[fbo_index]);
}

void XrRendererEndFrame(struct XrRenderer* renderer) {
    int fbo_index = renderer->ConfigInt[CONFIG_CURRENT_FBO];
    XrFramebufferRelease(&renderer->Framebuffer[fbo_index]);
}

void XrRendererFinishFrame(struct XrEngine* engine, struct XrRenderer* renderer) {
    int x = 0;
    int y = 0;
    int w = renderer->Framebuffer[0].Width;
    int h = renderer->Framebuffer[0].Height;
    if (renderer->ConfigInt[CONFIG_SBS]) {
        w /= 2;
    }

    int mode = renderer->ConfigInt[CONFIG_MODE];
    XrCompositionLayerProjectionView projection_layer_elements[2] = {};
    if ((mode == RENDER_MODE_MONO_6DOF) || (mode == RENDER_MODE_STEREO_6DOF)) {
        renderer->ConfigFloat[CONFIG_MENU_YAW] = renderer->HmdOrientation.y;

        for (int eye = 0; eye < XrMaxNumEyes; eye++) {
            struct XrFramebuffer* framebuffer = &renderer->Framebuffer[0];
            XrPosef pose = renderer->InvertedViewPose[0];
            if (renderer->ConfigInt[CONFIG_SBS] && (eye == 1)) {
                x += w;
            } else if (mode != RENDER_MODE_MONO_6DOF) {
                framebuffer = &renderer->Framebuffer[eye];
                pose = renderer->InvertedViewPose[eye];
            }

            XrVector3f pitch_axis = {1, 0, 0};
            XrVector3f yaw_axis = {0, 1, 0};
            XrVector3f rotation = XrQuaternionfEulerAngles(pose.orientation);
            XrQuaternionf pitch = XrQuaternionfCreateFromVectorAngle(pitch_axis, -ToRadians(rotation.x));
            XrQuaternionf yaw = XrQuaternionfCreateFromVectorAngle(yaw_axis, ToRadians(rotation.y));
            pose.orientation = XrQuaternionfMultiply(pitch, yaw);

            memset(&projection_layer_elements[eye], 0, sizeof(XrCompositionLayerProjectionView));
            projection_layer_elements[eye].type = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
            projection_layer_elements[eye].pose = pose;
            projection_layer_elements[eye].fov = renderer->Fov;

            memset(&projection_layer_elements[eye].subImage, 0, sizeof(XrSwapchainSubImage));
            projection_layer_elements[eye].subImage.swapchain = framebuffer->Handle;
            projection_layer_elements[eye].subImage.imageRect.offset.x = x;
            projection_layer_elements[eye].subImage.imageRect.offset.y = y;
            projection_layer_elements[eye].subImage.imageRect.extent.width = w;
            projection_layer_elements[eye].subImage.imageRect.extent.height = h;
            projection_layer_elements[eye].subImage.imageArrayIndex = 0;
        }

        XrCompositionLayerProjection projection_layer = {};
        projection_layer.type = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
        projection_layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        projection_layer.layerFlags |= XR_COMPOSITION_LAYER_CORRECT_CHROMATIC_ABERRATION_BIT;
        projection_layer.space = engine->CurrentSpace;
        projection_layer.viewCount = XrMaxNumEyes;
        projection_layer.views = projection_layer_elements;

        renderer->Layers[renderer->LayerCount++].projection = projection_layer;
    } else if ((mode == RENDER_MODE_MONO_SCREEN) || (mode == RENDER_MODE_STEREO_SCREEN)) {
        // Flat screen pose
        float distance = renderer->ConfigFloat[CONFIG_CANVAS_DISTANCE];
        float menu_pitch = ToRadians(renderer->ConfigFloat[CONFIG_MENU_PITCH]);
        float menu_yaw = ToRadians(renderer->ConfigFloat[CONFIG_MENU_YAW]);
        XrVector3f pos = {renderer->InvertedViewPose[0].position.x - sinf(menu_yaw) * cosf(menu_pitch) * distance,
                          renderer->InvertedViewPose[0].position.y - sinf(menu_pitch) * distance,
                          renderer->InvertedViewPose[0].position.z - cosf(menu_yaw) * cosf(menu_pitch) * distance};
        XrVector3f pitch_axis = {1, 0, 0};
        XrVector3f yaw_axis = {0, 1, 0};
        XrQuaternionf pitch = XrQuaternionfCreateFromVectorAngle(pitch_axis, -menu_pitch);
        XrQuaternionf yaw = XrQuaternionfCreateFromVectorAngle(yaw_axis, menu_yaw);

        // Setup quad layer
        struct XrFramebuffer* framebuffer = &renderer->Framebuffer[0];
        XrCompositionLayerQuad quad_layer = {};
        quad_layer.type = XR_TYPE_COMPOSITION_LAYER_QUAD;
        quad_layer.layerFlags = XR_COMPOSITION_LAYER_BLEND_TEXTURE_SOURCE_ALPHA_BIT;
        quad_layer.space = engine->CurrentSpace;
        memset(&quad_layer.subImage, 0, sizeof(XrSwapchainSubImage));
        quad_layer.subImage.imageRect.offset.x = x;
        quad_layer.subImage.imageRect.offset.y = y;
        quad_layer.subImage.imageRect.extent.width = w;
        quad_layer.subImage.imageRect.extent.height = h;
        quad_layer.subImage.swapchain = framebuffer->Handle;
        quad_layer.subImage.imageArrayIndex = 0;
        quad_layer.pose.orientation = XrQuaternionfMultiply(pitch, yaw);
        quad_layer.pose.position = pos;
        quad_layer.size.width = 4;
        quad_layer.size.height = 4;

        // Build the cylinder layer
        if (renderer->ConfigInt[CONFIG_SBS]) {
            quad_layer.eyeVisibility = XR_EYE_VISIBILITY_LEFT;
            renderer->Layers[renderer->LayerCount++].quad = quad_layer;
            quad_layer.eyeVisibility = XR_EYE_VISIBILITY_RIGHT;
            quad_layer.subImage.imageRect.offset.x = w;
            renderer->Layers[renderer->LayerCount++].quad = quad_layer;
        } else if (mode == RENDER_MODE_MONO_SCREEN) {
            quad_layer.eyeVisibility = XR_EYE_VISIBILITY_BOTH;
            renderer->Layers[renderer->LayerCount++].quad = quad_layer;
        } else {
            quad_layer.eyeVisibility = XR_EYE_VISIBILITY_LEFT;
            renderer->Layers[renderer->LayerCount++].quad = quad_layer;
            quad_layer.eyeVisibility = XR_EYE_VISIBILITY_RIGHT;
            quad_layer.subImage.swapchain = renderer->Framebuffer[1].Handle;
            renderer->Layers[renderer->LayerCount++].quad = quad_layer;
        }
    } else {
        assert(false);
    }

    // Compose the layers for this frame.
    const XrCompositionLayerBaseHeader* layers[XrMaxLayerCount] = {};
    for (int i = 0; i < renderer->LayerCount; i++) {
        layers[i] = (const XrCompositionLayerBaseHeader*)&renderer->Layers[i];
    }

    XrFrameEndInfo end_frame_info = {};
    end_frame_info.type = XR_TYPE_FRAME_END_INFO;
    end_frame_info.displayTime = engine->PredictedDisplayTime;
    end_frame_info.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
    end_frame_info.layerCount = renderer->LayerCount;
    end_frame_info.layers = layers;
    OXR(xrEndFrame(engine->Session, &end_frame_info));
}

void XrRendererBindFramebuffer(struct XrRenderer* renderer) {
    if (!renderer->Initialized)
        return;
    int fbo_index = renderer->ConfigInt[CONFIG_CURRENT_FBO];
    XrFramebufferSetCurrent(&renderer->Framebuffer[fbo_index]);
}


void XrRendererRecenter(struct XrEngine* engine, struct XrRenderer* renderer) {
    // Calculate recenter reference
    XrReferenceSpaceCreateInfo space_info = {};
    space_info.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    space_info.poseInReferenceSpace.orientation.w = 1.0f;
    if (engine->CurrentSpace != XR_NULL_HANDLE) {
        XrSpaceLocation loc = {};
        loc.type = XR_TYPE_SPACE_LOCATION;
        OXR(xrLocateSpace(engine->HeadSpace, engine->CurrentSpace,
                          engine->PredictedDisplayTime, &loc));
        renderer->HmdOrientation = XrQuaternionfEulerAngles(loc.pose.orientation);

        renderer->ConfigFloat[CONFIG_RECENTER_YAW] += renderer->HmdOrientation.y;
        float renceter_yaw = ToRadians(renderer->ConfigFloat[CONFIG_RECENTER_YAW]);
        space_info.poseInReferenceSpace.orientation.x = 0;
        space_info.poseInReferenceSpace.orientation.y = sinf(renceter_yaw / 2);
        space_info.poseInReferenceSpace.orientation.z = 0;
        space_info.poseInReferenceSpace.orientation.w = cosf(renceter_yaw / 2);
    }

    // Delete previous space instances
    if (engine->StageSpace != XR_NULL_HANDLE) {
        OXR(xrDestroySpace(engine->StageSpace));
    }
    if (engine->FakeSpace != XR_NULL_HANDLE) {
        OXR(xrDestroySpace(engine->FakeSpace));
    }

    // Create a default stage space to use if SPACE_TYPE_STAGE is not
    // supported, or calls to xrGetReferenceSpaceBoundsRect fail.
    space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
    if (engine->PlatformFlag[PLATFORM_TRACKING_FLOOR]) {
        space_info.poseInReferenceSpace.position.y = -1.6750f;
    }
    OXR(xrCreateReferenceSpace(engine->Session, &space_info, &engine->FakeSpace));
    ALOGV("Created fake stage space from local space with offset");
    engine->CurrentSpace = engine->FakeSpace;

    if (renderer->StageSupported) {
        space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
        space_info.poseInReferenceSpace.position.y = 0.0;
        OXR(xrCreateReferenceSpace(engine->Session, &space_info, &engine->StageSpace));
        ALOGV("Created stage space");
        if (engine->PlatformFlag[PLATFORM_TRACKING_FLOOR]) {
            engine->CurrentSpace = engine->StageSpace;
        }
    }

    // Update menu orientation
    renderer->ConfigFloat[CONFIG_MENU_PITCH] = renderer->HmdOrientation.x;
    renderer->ConfigFloat[CONFIG_MENU_YAW] = 0.0f;
}

void XrRendererHandleSessionStateChanges(struct XrEngine* engine, struct XrRenderer* renderer, XrSessionState state) {
    if (state == XR_SESSION_STATE_READY) {
        assert(renderer->SessionActive == false);

        XrSessionBeginInfo session_begin_info;
        memset(&session_begin_info, 0, sizeof(session_begin_info));
        session_begin_info.type = XR_TYPE_SESSION_BEGIN_INFO;
        session_begin_info.next = NULL;
        session_begin_info.primaryViewConfigurationType = renderer->ViewportConfig.viewConfigurationType;

        XrResult result;
        OXR(result = xrBeginSession(engine->Session, &session_begin_info));
        renderer->SessionActive = (result == XR_SUCCESS);
        ALOGV("Session active = %d", renderer->SessionActive);

#ifdef ANDROID
        if (renderer->SessionActive && engine->PlatformFlag[PLATFORM_EXTENSION_PERFORMANCE]) {
            PFN_xrPerfSettingsSetPerformanceLevelEXT pfnPerfSettingsSetPerformanceLevelEXT = NULL;
            OXR(xrGetInstanceProcAddr(engine->Instance, "xrPerfSettingsSetPerformanceLevelEXT",
                                      (PFN_xrVoidFunction*)(&pfnPerfSettingsSetPerformanceLevelEXT)));

            OXR(pfnPerfSettingsSetPerformanceLevelEXT(
                    engine->Session, XR_PERF_SETTINGS_DOMAIN_CPU_EXT, XR_PERF_SETTINGS_LEVEL_BOOST_EXT));
            OXR(pfnPerfSettingsSetPerformanceLevelEXT(
                    engine->Session, XR_PERF_SETTINGS_DOMAIN_GPU_EXT, XR_PERF_SETTINGS_LEVEL_BOOST_EXT));

            PFN_xrSetAndroidApplicationThreadKHR pfnSetAndroidApplicationThreadKHR = NULL;
            OXR(xrGetInstanceProcAddr(engine->Instance, "xrSetAndroidApplicationThreadKHR",
                                      (PFN_xrVoidFunction*)(&pfnSetAndroidApplicationThreadKHR)));

            OXR(pfnSetAndroidApplicationThreadKHR(engine->Session,
                                                  XR_ANDROID_THREAD_TYPE_APPLICATION_MAIN_KHR,
                                                  engine->MainThreadId));
            OXR(pfnSetAndroidApplicationThreadKHR(engine->Session,
                                                  XR_ANDROID_THREAD_TYPE_RENDERER_MAIN_KHR,
                                                  engine->RenderThreadId));
        }
#endif
    } else if (state == XR_SESSION_STATE_STOPPING) {
        assert(renderer->SessionActive);

        OXR(xrEndSession(engine->Session));
        renderer->SessionActive = false;
    }
}

void XrRendererHandleXrEvents(struct XrEngine* engine, struct XrRenderer* renderer) {
    XrEventDataBuffer event_data_bufer = {};

    // Poll for events
    for (;;) {
        XrEventDataBaseHeader* base_event_handler = (XrEventDataBaseHeader*)(&event_data_bufer);
        base_event_handler->type = XR_TYPE_EVENT_DATA_BUFFER;
        base_event_handler->next = NULL;
        XrResult r;
        OXR(r = xrPollEvent(engine->Instance, &event_data_bufer));
        if (r != XR_SUCCESS) {
            break;
        }

        switch (base_event_handler->type) {
            case XR_TYPE_EVENT_DATA_EVENTS_LOST:
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_EVENTS_LOST");
                break;
            case XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING:
            {
                const XrEventDataInstanceLossPending* instance_loss_pending_event =
                        (XrEventDataInstanceLossPending*)(base_event_handler);
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING: time %lf",
                      FromXrTime(instance_loss_pending_event->lossTime));
            }
                break;
            case XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED:
                ALOGV("xrPollEvent: received XR_TYPE_EVENT_DATA_INTERACTION_PROFILE_CHANGED");
                break;
            case XR_TYPE_EVENT_DATA_PERF_SETTINGS_EXT:
                break;
            case XR_TYPE_EVENT_DATA_REFERENCE_SPACE_CHANGE_PENDING:
                XrRendererRecenter(engine, renderer);
                break;
            case XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED:
            {
                const XrEventDataSessionStateChanged* session_state_changed_event =
                        (XrEventDataSessionStateChanged*)(base_event_handler);
                switch (session_state_changed_event->state) {
                    case XR_SESSION_STATE_FOCUSED:
                        renderer->SessionFocused = true;
                        break;
                    case XR_SESSION_STATE_VISIBLE:
                        renderer->SessionFocused = false;
                        break;
                    case XR_SESSION_STATE_READY:
                    case XR_SESSION_STATE_STOPPING:
                        XrRendererHandleSessionStateChanges(engine, renderer, session_state_changed_event->state);
                        break;
                    default:
                        break;
                }
                break;
            }
            default:
                ALOGV("xrPollEvent: Unknown event");
                break;
        }
    }
}

void XrRendererUpdateStageBounds(struct XrEngine* engine) {
    XrExtent2Df stage_bounds = {};

    XrResult result;
    OXR(result = xrGetReferenceSpaceBoundsRect(engine->Session, XR_REFERENCE_SPACE_TYPE_STAGE,
                                               &stage_bounds));
    if (result != XR_SUCCESS) {
        stage_bounds.width = 1.0f;
        stage_bounds.height = 1.0f;

        engine->CurrentSpace = engine->FakeSpace;
    }
}
