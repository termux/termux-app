#include <string.h>

#include "engine.h"
#include "input.h"
#include "math.h"
#include "renderer.h"

struct XrEngine xr_engine;
struct XrInput xr_input;
struct XrRenderer xr_renderer;
bool xr_initialized = false;
int xr_params[6] = {};

#if defined(_DEBUG)
#include <GLES2/gl2.h>
void GLCheckErrors(const char* file, int line) {
	for (int i = 0; i < 10; i++) {
		const GLenum error = glGetError();
		if (error == GL_NO_ERROR) {
			break;
		}
		ALOGE("OpenGL error on line %s:%d %d", file, line, error);
	}
}

void OXRCheckErrors(XrResult result, const char* file, int line) {
	if (XR_FAILED(result)) {
		char errorBuffer[XR_MAX_RESULT_STRING_SIZE];
		xrResultToString(xr_engine->Instance, result, errorBuffer);
        ALOGE("OpenXR error on line %s:%d %s", file, line, errorBuffer);
	}
}
#endif

JNIEXPORT void JNICALL Java_com_termux_x11_XrActivity_init(JNIEnv *env, jobject obj) {

    // Do not allow second initialization
    if (xr_initialized) {
        return;
    }

    // Set platform flags
    memset(&xr_engine, 0, sizeof(xr_engine));
    xr_engine.PlatformFlag[PLATFORM_CONTROLLER_QUEST] = true;
    xr_engine.PlatformFlag[PLATFORM_EXTENSION_PASSTHROUGH] = true;
    xr_engine.PlatformFlag[PLATFORM_EXTENSION_PERFORMANCE] = true;

    // Get Java VM
    JavaVM* vm;
    (*env)->GetJavaVM(env, &vm);

    // Init XR
    xrJava java;
    java.vm = vm;
    java.activity = (*env)->NewGlobalRef(env, obj);
    XrEngineInit(&xr_engine, &java, "termux-x11", 1);
    XrEngineEnter(&xr_engine);
    XrInputInit(&xr_engine, &xr_input);
    XrRendererInit(&xr_engine, &xr_renderer);
    XrRendererGetResolution(&xr_engine, &xr_renderer, &xr_params[4], &xr_params[5]);
    xr_initialized = true;
    ALOGV("Init called");
}

JNIEXPORT void JNICALL Java_com_termux_x11_XrActivity_teardown(JNIEnv *env, jobject obj) {
    if (!xr_initialized) {
        return;
    }

    XrRendererDestroy(&xr_engine, &xr_renderer);
    XrEngineLeave(&xr_engine);
    XrEngineDestroy(&xr_engine);

    memset(&xr_engine, 0, sizeof(xr_engine));
    memset(&xr_input, 0, sizeof(xr_input));
    memset(&xr_renderer, 0, sizeof(xr_renderer));
    xr_initialized = false;
}

JNIEXPORT jboolean JNICALL Java_com_termux_x11_XrActivity_beginFrame(JNIEnv *env, jobject obj) {
    if (XrRendererInitFrame(&xr_engine, &xr_renderer)) {

        // Set renderer
        int mode = xr_params[1] ? RENDER_MODE_MONO_6DOF : RENDER_MODE_MONO_SCREEN;
        xr_renderer.ConfigFloat[CONFIG_CANVAS_DISTANCE] = xr_params[0];
        xr_renderer.ConfigInt[CONFIG_PASSTHROUGH] = xr_params[2];
        xr_renderer.ConfigInt[CONFIG_MODE] = mode;
        xr_renderer.ConfigInt[CONFIG_SBS] = xr_params[3];

        // Recenter if mode switched
        static bool last_immersive = false;
        if (last_immersive != xr_params[1]) {
            XrRendererRecenter(&xr_engine, &xr_renderer);
            last_immersive = xr_params[1];
        }

        // Update controllers state
        XrInputUpdate(&xr_engine, &xr_input);

        // Lock framebuffer
        XrRendererBeginFrame(&xr_renderer, 0);

        return true;
    }
    return false;
}

JNIEXPORT void JNICALL Java_com_termux_x11_XrActivity_finishFrame(JNIEnv *env, jobject obj) {
    XrRendererEndFrame(&xr_renderer);
    XrRendererFinishFrame(&xr_engine, &xr_renderer);
}

JNIEXPORT jfloatArray JNICALL Java_com_termux_x11_XrActivity_getAxes(JNIEnv *env, jobject obj) {
    XrPosef lPose = XrInputGetPose(&xr_input, 0);
    XrPosef rPose = XrInputGetPose(&xr_input, 1);
    XrVector2f lThumbstick = XrInputGetJoystickState(&xr_input, 0);
    XrVector2f rThumbstick = XrInputGetJoystickState(&xr_input, 1);
    XrVector3f lPosition = xr_renderer.Projections[0].pose.position;
    XrVector3f rPosition = xr_renderer.Projections[1].pose.position;
    XrVector3f angles = xr_renderer.HmdOrientation;

    int count = 0;
    float data[32];
    data[count++] = XrQuaternionfEulerAngles(lPose.orientation).x; //L_PITCH
    data[count++] = XrQuaternionfEulerAngles(lPose.orientation).y; //L_YAW
    data[count++] = XrQuaternionfEulerAngles(lPose.orientation).z; //L_ROLL
    data[count++] = lThumbstick.x; //L_THUMBSTICK_X
    data[count++] = lThumbstick.y; //L_THUMBSTICK_Y
    data[count++] = lPose.position.x; //L_X
    data[count++] = lPose.position.y; //L_Y
    data[count++] = lPose.position.z; //L_Z
    data[count++] = XrQuaternionfEulerAngles(rPose.orientation).x; //R_PITCH
    data[count++] = XrQuaternionfEulerAngles(rPose.orientation).y; //R_YAW
    data[count++] = XrQuaternionfEulerAngles(rPose.orientation).z; //R_ROLL
    data[count++] = rThumbstick.x; //R_THUMBSTICK_X
    data[count++] = rThumbstick.y; //R_THUMBSTICK_Y
    data[count++] = rPose.position.x; //R_X
    data[count++] = rPose.position.y; //R_Y
    data[count++] = rPose.position.z; //R_Z
    data[count++] = angles.x; //HMD_PITCH
    data[count++] = angles.y; //HMD_YAW
    data[count++] = angles.z; //HMD_ROLL
    data[count++] = (lPosition.x + rPosition.x) * 0.5f; //HMD_X
    data[count++] = (lPosition.y + rPosition.y) * 0.5f; //HMD_Y
    data[count++] = (lPosition.z + rPosition.z) * 0.5f; //HMD_Z
    data[count++] = XrVector3fDistance(lPosition, rPosition); //HMD_IPD

    jfloat values[count];
    memcpy(values, data, count * sizeof(float));
    jfloatArray output = (*env)->NewFloatArray(env, count);
    (*env)->SetFloatArrayRegion(env, output, (jsize)0, (jsize)count, values);
    return output;
}

JNIEXPORT jbooleanArray JNICALL Java_com_termux_x11_XrActivity_getButtons(JNIEnv *env, jobject obj) {
    uint32_t l = XrInputGetButtonState(&xr_input, 0);
    uint32_t r = XrInputGetButtonState(&xr_input, 1);

    int count = 0;
    bool data[32];
    data[count++] = l & (int)Grip; //L_GRIP
    data[count++] = l & (int)Enter; //L_MENU
    data[count++] = l & (int)LThumb; //L_THUMBSTICK_PRESS
    data[count++] = l & (int)Left; //L_THUMBSTICK_LEFT
    data[count++] = l & (int)Right; //L_THUMBSTICK_RIGHT
    data[count++] = l & (int)Up; //L_THUMBSTICK_UP
    data[count++] = l & (int)Down; //L_THUMBSTICK_DOWN
    data[count++] = l & (int)Trigger; //L_TRIGGER
    data[count++] = l & (int)X; //L_X
    data[count++] = l & (int)Y; //L_Y
    data[count++] = r & (int)A; //R_A
    data[count++] = r & (int)B; //R_B
    data[count++] = r & (int)Grip; //R_GRIP
    data[count++] = r & (int)RThumb; //R_THUMBSTICK_PRESS
    data[count++] = r & (int)Left; //R_THUMBSTICK_LEFT
    data[count++] = r & (int)Right; //R_THUMBSTICK_RIGHT
    data[count++] = r & (int)Up; //R_THUMBSTICK_UP
    data[count++] = r & (int)Down; //R_THUMBSTICK_DOWN
    data[count++] = r & (int)Trigger; //R_TRIGGER

    jboolean values[count];
    memcpy(values, data, count * sizeof(jboolean));
    jbooleanArray output = (*env)->NewBooleanArray(env, count);
    (*env)->SetBooleanArrayRegion(env, output, (jsize)0, (jsize)count, values);
    return output;
}


JNIEXPORT jint JNICALL Java_com_termux_x11_XrActivity_getRenderParam(JNIEnv *env, jobject obj, jint param) {
    return xr_params[param];
}

JNIEXPORT void JNICALL Java_com_termux_x11_XrActivity_setRenderParam(JNIEnv *env, jobject obj, jint param, jint value) {
    xr_params[param] = value;
}
