#include "engine.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void XrEngineInit(struct XrEngine* engine, void* system, const char* name, int version) {
    if (engine->Initialized)
        return;
    memset(engine, 0, sizeof(engine));

#ifdef ANDROID
    PFN_xrInitializeLoaderKHR xrInitializeLoaderKHR;
    xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrInitializeLoaderKHR",
                          (PFN_xrVoidFunction*)&xrInitializeLoaderKHR);
    if (xrInitializeLoaderKHR != NULL) {
        xrJava* java = (xrJava*)system;
        XrLoaderInitInfoAndroidKHR loader_info;
        memset(&loader_info, 0, sizeof(loader_info));
        loader_info.type = XR_TYPE_LOADER_INIT_INFO_ANDROID_KHR;
        loader_info.next = NULL;
        loader_info.applicationVM = java->vm;
        loader_info.applicationContext = java->activity;
        xrInitializeLoaderKHR((XrLoaderInitInfoBaseHeaderKHR*)&loader_info);
    }
#endif

    int count = 0;
    const char* extensions[32];
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
    extensions[count++] = XR_KHR_OPENGL_ES_ENABLE_EXTENSION_NAME;
#endif
#ifdef ANDROID
    if (engine->PlatformFlag[PLATFORM_EXTENSION_INSTANCE]) {
        extensions[count++] = XR_KHR_ANDROID_CREATE_INSTANCE_EXTENSION_NAME;
    }
    if (engine->PlatformFlag[PLATFORM_EXTENSION_PASSTHROUGH]) {
        extensions[count++] = XR_FB_PASSTHROUGH_EXTENSION_NAME;
    }
    if (engine->PlatformFlag[PLATFORM_EXTENSION_PERFORMANCE]) {
        extensions[count++] = XR_EXT_PERFORMANCE_SETTINGS_EXTENSION_NAME;
        extensions[count++] = XR_KHR_ANDROID_THREAD_SETTINGS_EXTENSION_NAME;
    }
#endif

    // Create the OpenXR instance.
    XrApplicationInfo app_info;
    memset(&app_info, 0, sizeof(app_info));
    strcpy(app_info.applicationName, name);
    strcpy(app_info.engineName, name);
    app_info.applicationVersion = version;
    app_info.engineVersion = version;
    app_info.apiVersion = XR_API_VERSION_1_0;

    XrInstanceCreateInfo instance_info;
    memset(&instance_info, 0, sizeof(instance_info));
    instance_info.type = XR_TYPE_INSTANCE_CREATE_INFO;
    instance_info.next = NULL;
    instance_info.createFlags = 0;
    instance_info.applicationInfo = app_info;
    instance_info.enabledApiLayerCount = 0;
    instance_info.enabledApiLayerNames = NULL;
    instance_info.enabledExtensionCount = count;
    instance_info.enabledExtensionNames = extensions;

#ifdef ANDROID
    XrInstanceCreateInfoAndroidKHR instance_info_android = {XR_TYPE_INSTANCE_CREATE_INFO_ANDROID_KHR};
    if (engine->PlatformFlag[PLATFORM_EXTENSION_INSTANCE]) {
        xrJava* java = (xrJava*)system;
        instance_info_android.applicationVM = java->vm;
        instance_info_android.applicationActivity = java->activity;
        instance_info.next = (XrBaseInStructure*)&instance_info_android;
    }
#endif

    XrResult result;
    OXR(result = xrCreateInstance(&instance_info, &engine->Instance));
    if (result != XR_SUCCESS) {
        ALOGE("Failed to create XR instance: %d", (int)result);
        exit(1);
    }

    XrInstanceProperties instance_properties;
    instance_properties.type = XR_TYPE_INSTANCE_PROPERTIES;
    instance_properties.next = NULL;
    OXR(xrGetInstanceProperties(engine->Instance, &instance_properties));
    ALOGV("Runtime %s: Version : %d.%d.%d", instance_properties.runtimeName,
          XR_VERSION_MAJOR(instance_properties.runtimeVersion),
          XR_VERSION_MINOR(instance_properties.runtimeVersion),
          XR_VERSION_PATCH(instance_properties.runtimeVersion));

    XrSystemGetInfo system_info;
    memset(&system_info, 0, sizeof(system_info));
    system_info.type = XR_TYPE_SYSTEM_GET_INFO;
    system_info.next = NULL;
    system_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;

    OXR(result = xrGetSystem(engine->Instance, &system_info, &engine->SystemId));
    if (result != XR_SUCCESS) {
        ALOGE("Failed to get system");
        exit(1);
    }

    // Get the graphics requirements.
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
    PFN_xrGetOpenGLESGraphicsRequirementsKHR pfnGetOpenGLESGraphicsRequirementsKHR = NULL;
    OXR(xrGetInstanceProcAddr(engine->Instance, "xrGetOpenGLESGraphicsRequirementsKHR",
                              (PFN_xrVoidFunction*)(&pfnGetOpenGLESGraphicsRequirementsKHR)));

    XrGraphicsRequirementsOpenGLESKHR graphics_requirements = {};
    graphics_requirements.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_ES_KHR;
    OXR(pfnGetOpenGLESGraphicsRequirementsKHR(engine->Instance, engine->SystemId, &graphics_requirements));
#endif

#ifdef ANDROID
    engine->MainThreadId = gettid();
#endif
    engine->Initialized = true;
}

void XrEngineDestroy(struct XrEngine* engine) {
    if (engine->Initialized) {
        xrDestroyInstance(engine->Instance);
        engine->Initialized = false;
    }
}

void XrEngineEnter(struct XrEngine* engine) {
    if (engine->Session) {
        ALOGE("EnterXR called with existing session");
        return;
    }

    // Create the OpenXR Session.
    XrSessionCreateInfo session_info;
    memset(&session_info, 0, sizeof(session_info));
#ifdef XR_USE_GRAPHICS_API_OPENGL_ES
    XrGraphicsBindingOpenGLESAndroidKHR graphics_binding_gl = {};
    graphics_binding_gl.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_ES_ANDROID_KHR;
    graphics_binding_gl.next = NULL;
    graphics_binding_gl.display = eglGetCurrentDisplay();
    graphics_binding_gl.config = NULL;
    graphics_binding_gl.context = eglGetCurrentContext();
    session_info.next = &graphics_binding_gl;
#endif
    session_info.type = XR_TYPE_SESSION_CREATE_INFO;
    session_info.createFlags = 0;
    session_info.systemId = engine->SystemId;

    XrResult result;
    OXR(result = xrCreateSession(engine->Instance, &session_info, &engine->Session));
    if (result != XR_SUCCESS) {
        ALOGE("Failed to create XR session: %d", (int)result);
        exit(1);
    }

    // Create a space to the first path
    XrReferenceSpaceCreateInfo space_info = {};
    space_info.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
    space_info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
    space_info.poseInReferenceSpace.orientation.w = 1.0f;
    OXR(xrCreateReferenceSpace(engine->Session, &space_info, &engine->HeadSpace));
}

void XrEngineLeave(struct XrEngine* engine) {
    if (engine->Session) {
        OXR(xrDestroySpace(engine->HeadSpace));
        // StageSpace is optional.
        if (engine->StageSpace != XR_NULL_HANDLE) {
            OXR(xrDestroySpace(engine->StageSpace));
        }
        OXR(xrDestroySpace(engine->FakeSpace));
        engine->CurrentSpace = XR_NULL_HANDLE;
        OXR(xrDestroySession(engine->Session));
        engine->Session = XR_NULL_HANDLE;
    }
}

void XrEngineWaitForFrame(struct XrEngine* engine) {
    XrFrameWaitInfo wait_frame_info = {};
    wait_frame_info.type = XR_TYPE_FRAME_WAIT_INFO;
    wait_frame_info.next = NULL;

    XrFrameState frame_state = {};
    frame_state.type = XR_TYPE_FRAME_STATE;
    frame_state.next = NULL;

    OXR(xrWaitFrame(engine->Session, &wait_frame_info, &frame_state));
    engine->PredictedDisplayTime = frame_state.predictedDisplayTime;
}
