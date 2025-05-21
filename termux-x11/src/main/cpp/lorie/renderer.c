#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "UnusedParameter"
#pragma ide diagnostic ignored "DanglingPointer"
#pragma ide diagnostic ignored "ConstantConditionsOC"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "UnreachableCode"
#pragma ide diagnostic ignored "OCUnusedMacroInspection"
#pragma ide diagnostic ignored "misc-no-recursion"
#pragma clang diagnostic ignored "-Wincompatible-pointer-types-discards-qualifiers"
#define EGL_EGLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include "list.h"
#include "lorie.h"

#define log(...) __android_log_print(ANDROID_LOG_DEBUG, "gles-renderer", __VA_ARGS__)
#define loge(...) __android_log_print(ANDROID_LOG_ERROR, "gles-renderer", __VA_ARGS__)

static GLuint createProgram(const char* p_vertex_source, const char* p_fragment_source);

static int printEglError(char* msg, int line) {
    char descBuf[32] = {0};
    char* desc;
    int err = eglGetError();
    switch(err) {
#define E(code, text) case code: desc = (char*) text; break
        case EGL_SUCCESS: desc = NULL; // "No error"
        E(EGL_NOT_INITIALIZED, "EGL not initialized or failed to initialize");
        E(EGL_BAD_ACCESS, "Resource inaccessible");
        E(EGL_BAD_ALLOC, "Cannot allocate resources");
        E(EGL_BAD_ATTRIBUTE, "Unrecognized attribute or attribute value");
        E(EGL_BAD_CONTEXT, "Invalid EGL context");
        E(EGL_BAD_CONFIG, "Invalid EGL frame buffer configuration");
        E(EGL_BAD_CURRENT_SURFACE, "Current surface is no longer valid");
        E(EGL_BAD_DISPLAY, "Invalid EGL display");
        E(EGL_BAD_SURFACE, "Invalid surface");
        E(EGL_BAD_MATCH, "Inconsistent arguments");
        E(EGL_BAD_PARAMETER, "Invalid argument");
        E(EGL_BAD_NATIVE_PIXMAP, "Invalid native pixmap");
        E(EGL_BAD_NATIVE_WINDOW, "Invalid native window");
        E(EGL_CONTEXT_LOST, "Context lost");
#undef E
        default:
            snprintf(descBuf, sizeof(descBuf) - 1, "Unknown error (%d)", err);
            desc = descBuf;
    }

    if (desc)
        log("renderer: %s: %s (%s:%d)\n", msg, desc, __FILE__, line);

    return 0;
}

static inline __always_inline void vprintEglError(char* msg, int line) {
    printEglError(msg, line);
}

static void checkGlError(int line) {
    GLenum error;
    char *desc = NULL;
    for (error = glGetError(); error; error = glGetError()) {
        switch (error) {
#define E(code) case code: desc = (char*)#code; break
            E(GL_INVALID_ENUM);
            E(GL_INVALID_VALUE);
            E(GL_INVALID_OPERATION);
            E(GL_STACK_OVERFLOW_KHR);
            E(GL_STACK_UNDERFLOW_KHR);
            E(GL_OUT_OF_MEMORY);
            E(GL_INVALID_FRAMEBUFFER_OPERATION);
            E(GL_CONTEXT_LOST_KHR);
            default:
                continue;
#undef E
        }
        log("Xlorie: GLES %d ERROR: %s.\n", line, desc);
        return;
    }
}

#define checkGlError() checkGlError(__LINE__)

static const char vertexShaderSrc[] =
    "attribute vec4 position;\n"
    "attribute vec2 texCoords;"
    "varying vec2 outTexCoords;\n"
    "void main(void) {\n"
    "   outTexCoords = texCoords;\n"
    "   gl_Position = position;\n"
    "}\n";

#define FRAGMENT_SHADER(texture) \
    "precision mediump float;\n" \
    "varying vec2 outTexCoords;\n" \
    "uniform sampler2D texture;\n" \
    "void main(void) {\n" \
    "   gl_FragColor = texture2D(texture, outTexCoords)" texture ";\n" \
    "}\n"

static const char fragmentShaderSrc[] = FRAGMENT_SHADER();
static const char fragmentShaderBgraSrc[] = FRAGMENT_SHADER(".bgra");

static EGLDisplay egl_display = EGL_NO_DISPLAY;
static EGLContext ctx = EGL_NO_CONTEXT;
static EGLSurface defaultSfc = EGL_NO_SURFACE, sfc = EGL_NO_SURFACE;
static EGLConfig cfg = 0;
static ANativeWindow *defaultWin = NULL, *win = NULL;
static volatile struct xorg_list addedBuffers, buffers, removedBuffers;

static JNIEnv* renderEnv = NULL;
static volatile bool stateChanged = false, windowChanged = false;
static volatile struct lorie_shared_server_state* pendingState = NULL;
static volatile ANativeWindow* pendingWin = NULL;

static pthread_mutex_t stateLock;
static pthread_cond_t stateCond;
static pthread_cond_t stateChangeFinishCond;
static pthread_spinlock_t bufferLock;
static volatile struct lorie_shared_server_state* state = NULL;
static struct {
    GLuint id;
    bool cursorChanged;
} cursor;

GLuint g_texture_program = 0, gv_pos = 0, gv_coords = 0;
GLuint g_texture_program_bgra = 0, gv_pos_bgra = 0, gv_coords_bgra = 0;

static void* rendererThread(void);

static void pthreadCondVarProxyInit(void);
static void* pthreadCondVarProxyThread(void* cookie);
static void pthreadCondVarProxyListenOtherCondVar(pthread_cond_t* var);

static inline __always_inline void bindLinearTexture(GLuint id) {
    glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

static EGLint configAttribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_NONE
};

const EGLint ctxattribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE
};

int rendererInitThread(JavaVM *vm) {
    JNIEnv* env;
    EGLint major, minor;
    EGLint numConfigs;
    EGLint *const alphaAttrib = &configAttribs[11];

    pthread_setname_np(pthread_self(), "LorieRendererThread");

    xorg_list_init(&addedBuffers);
    xorg_list_init(&buffers);
    xorg_list_init(&removedBuffers);

    (*vm)->AttachCurrentThread(vm, &env, NULL);

    egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (egl_display == EGL_NO_DISPLAY)
        return printEglError("Got no EGL display", __LINE__);

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE)
        return printEglError("Unable to initialize EGL", __LINE__);

    log("Xlorie: Initialized EGL version %d.%d\n", major, minor);
    eglBindAPI(EGL_OPENGL_ES_API);

    if (eglChooseConfig(egl_display, configAttribs, &cfg, 1, &numConfigs) != EGL_TRUE &&
        (*alphaAttrib = 8) &&
        eglChooseConfig(egl_display, configAttribs, &cfg, 1, &numConfigs) != EGL_TRUE)
        return printEglError("eglChooseConfig failed", __LINE__);

    ctx = eglCreateContext(egl_display, cfg, NULL, ctxattribs);
    if (ctx == EGL_NO_CONTEXT)
        return printEglError("eglCreateContext failed", __LINE__);

    // Weird devices without proper EGL_KHR_surfaceless_context support
    // We can not use pbuffer-based surfaces because it will require searching for configs supporting it
    // and I am not sure all devices have configs supporting both pbuffers and regular surfaces simultaneously
    jclass surfaceTextureClass = (*env)->FindClass(env, "android/graphics/SurfaceTexture");
    jclass surfaceClass = (*env)->FindClass(env, "android/view/Surface");

    jmethodID surfaceTextureConstructor = (*env)->GetMethodID(env, surfaceTextureClass, "<init>", "(Z)V");
    jmethodID surfaceConstructor = (*env)->GetMethodID(env, surfaceClass, "<init>", "(Landroid/graphics/SurfaceTexture;)V");

    jobject surfaceTextureObject = (*env)->NewObject(env, surfaceTextureClass, surfaceTextureConstructor, true);
    jobject surfaceObject = (*env)->NewObject(env, surfaceClass, surfaceConstructor, surfaceTextureObject);

    // We will use this surfacetexture each time surface is destroyed, zero reasons to clean it up
    (*env)->NewGlobalRef(env, surfaceTextureObject);
    (*env)->NewGlobalRef(env, surfaceObject);

    win = defaultWin = ANativeWindow_fromSurface(env, surfaceObject);
    ANativeWindow_acquire(defaultWin);

    sfc = defaultSfc = eglCreateWindowSurface(egl_display, cfg, win, NULL);

    eglMakeCurrent(egl_display, sfc, sfc, ctx);
    eglSwapInterval(egl_display, 0);

    g_texture_program = createProgram(vertexShaderSrc, fragmentShaderSrc);
    if (!g_texture_program)
        log("Xlorie: GLESv2: Unable to create shader program.\n");

    g_texture_program_bgra = createProgram(vertexShaderSrc, fragmentShaderBgraSrc);
    if (!g_texture_program_bgra)
        log("Xlorie: GLESv2: Unable to create bgra shader program.\n");

    gv_pos = (GLuint) glGetAttribLocation(g_texture_program, "position");
    gv_coords = (GLuint) glGetAttribLocation(g_texture_program, "texCoords");

    gv_pos_bgra = (GLuint) glGetAttribLocation(g_texture_program_bgra, "position");
    gv_coords_bgra = (GLuint) glGetAttribLocation(g_texture_program_bgra, "texCoords");

    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &cursor.id);

    rendererThread();
    return 1;
}

void rendererInit(JNIEnv* env) {
    pthread_t t;
    JavaVM *vm;

    if (ctx)
        return;

    (*env)->GetJavaVM(env, &vm);

    pthreadCondVarProxyInit();

    pthread_mutex_init(&stateLock, NULL);
    pthread_cond_init(&stateCond, NULL);
    pthread_cond_init(&stateChangeFinishCond, NULL);
    pthread_spin_init(&bufferLock, false);

    pthread_create(&t, NULL, (void*(*)(void*)) rendererInitThread, vm);
}

void rendererTestCapabilities(int* legacy_drawing, uint8_t* flip) {
    // Some devices do not support sampling from HAL_PIXEL_FORMAT_BGRA_8888, here we are checking it.
    const EGLint imageAttributes[] = {EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE};
    EGLint numConfigs;
    EGLClientBuffer clientBuffer;
    EGLImageKHR img;
    AHardwareBuffer *new = NULL;
    int status;
    AHardwareBuffer_Desc d0 = {
            .width = 64,
            .height = 64,
            .layers = 1,
            .usage = AHARDWAREBUFFER_USAGE_GPU_SAMPLED_IMAGE | AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
            .format = AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM
    };

    if (egl_display == EGL_NO_DISPLAY) {
        egl_display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (egl_display == EGL_NO_DISPLAY)
            return vprintEglError("Got no EGL display", __LINE__);
    }

    status = AHardwareBuffer_allocate(&d0, &new);
    if (status != 0 || new == NULL) {
        loge("Failed to allocate native buffer (%p, error %d)", new, status);
        loge("Forcing legacy drawing");
        *legacy_drawing = 1;
        return;
    }

    uint32_t *pixels;
    if (AHardwareBuffer_lock(new, AHARDWAREBUFFER_USAGE_CPU_WRITE_OFTEN | AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, (void **) &pixels) == 0) {
        pixels[0] = 0xAABBCCDD;
        AHardwareBuffer_unlock(new, NULL);
    } else {
        loge("Failed to lock native buffer (%p, error %d)", new, status);
        loge("Forcing legacy drawing");
        *legacy_drawing = 1;
        AHardwareBuffer_release(new);
        return;
    }

    clientBuffer = eglGetNativeClientBufferANDROID(new);
    if (!clientBuffer) {
        *legacy_drawing = 1;
        AHardwareBuffer_release(new);
        return vprintEglError("Failed to obtain EGLClientBuffer from AHardwareBuffer, forcing legacy drawing", __LINE__);
    }

    if (!(img = eglCreateImageKHR(egl_display, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_ANDROID, clientBuffer, imageAttributes))) {
        if (eglGetError() == EGL_BAD_PARAMETER) {
            loge("Sampling from HAL_PIXEL_FORMAT_BGRA_8888 is not supported, forcing AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM");
            *flip = 1;
        } else {
            loge("Failed to obtain EGLImageKHR from EGLClientBuffer");
            loge("Forcing legacy drawing");
            *legacy_drawing = 1;
        }
        AHardwareBuffer_release(new);
    } else {
        // For some reason all devices I checked had no GL_EXT_texture_format_BGRA8888 support, but some of them still provided BGRA extension.
        // EGL does not provide functions to query texture format in runtime.
        // Workarounds are less performant but at least they let us use Termux:X11 on devices with missing BGRA support.
        // We handle two cases.
        // If resulting texture has BGRA format but still drawing RGBA we should flip format to RGBA and flip pixels manually in shader.
        // In the case if for some reason we can not use HAL_PIXEL_FORMAT_BGRA_8888 we should fallback to legacy drawing method (uploading pixels via glTexImage2D).
        configAttribs[1] = EGL_PBUFFER_BIT;
        EGLConfig checkcfg = 0;
        GLuint fbo = 0, texture = 0;
        if (eglChooseConfig(egl_display, configAttribs, &checkcfg, 1, &numConfigs) != EGL_TRUE)
            return vprintEglError("check eglChooseConfig failed", __LINE__);

        EGLContext testctx = eglCreateContext(egl_display, checkcfg, NULL, ctxattribs);
        if (testctx == EGL_NO_CONTEXT)
            return vprintEglError("check eglCreateContext failed", __LINE__);

        const EGLint pbufferAttributes[] = {
                EGL_WIDTH, 64,
                EGL_HEIGHT, 64,
                EGL_NONE,
        };
        EGLSurface checksfc = eglCreatePbufferSurface(egl_display, checkcfg, pbufferAttributes);

        if (eglMakeCurrent(egl_display, checksfc, checksfc, testctx) != EGL_TRUE)
            return vprintEglError("check eglMakeCurrent failed", __LINE__);

        glActiveTexture(GL_TEXTURE0); checkGlError();
        glGenTextures(1, &texture); checkGlError();
        bindLinearTexture(texture);
        glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, img); checkGlError();
        glGenFramebuffers(1, &fbo); checkGlError();
        glBindFramebuffer(GL_FRAMEBUFFER, fbo); checkGlError();
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0); checkGlError();
        uint32_t pixel[64*64];
        glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, &pixel); checkGlError();
        if (pixel[0] == 0xAABBCCDD) {
            log("Xlorie: GLES draws pixels unchanged, probably system does not support AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM. Forcing bgra.\n");
            *flip = 1;
        } else if (pixel[0] != 0xAADDCCBB) {
            log("Xlorie: GLES receives broken pixels. Forcing legacy drawing. 0x%X\n", pixel[0]);
            *legacy_drawing = 1;
        }
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(egl_display, testctx);
        eglDestroyImageKHR(egl_display, img);
        eglDestroySurface(egl_display, checksfc);
        AHardwareBuffer_release(new);
    }
}

__unused void rendererSetSharedState(struct lorie_shared_server_state* newState) {
    pthread_mutex_lock(&stateLock);
    pendingState = newState;
    stateChanged = true;
    pthread_cond_signal(&stateCond);

    while(stateChanged)
        pthread_cond_wait(&stateChangeFinishCond, &stateLock);

    pthread_mutex_unlock(&stateLock);
}

void rendererAddBuffer(LorieBuffer* buf) {
    pthread_spin_lock(&bufferLock);
    LorieBuffer_addToList(buf, &addedBuffers);
    pthread_cond_signal(&stateCond);
    pthread_spin_unlock(&bufferLock);
}

void rendererRemoveBuffer(uint64_t id) {
    pthread_spin_lock(&bufferLock);
    LorieBuffer* buf = LorieBufferList_findById(&addedBuffers, id);
    if (buf)
        // Buffer was not attached to GL yet, it is safe to release it now.
        LorieBuffer_release(buf);
    else {
        buf = LorieBufferList_findById(&buffers, id);
        if (buf) {
            // The buffer is attached to GL so we should release it from renderer thread.
            LorieBuffer_removeFromList(buf);
            LorieBuffer_addToList(buf, &removedBuffers);
        }
    }
    pthread_spin_unlock(&bufferLock);
}

void rendererRemoveAllBuffers(void) {
    LorieBuffer *buf = NULL;

    pthread_spin_lock(&bufferLock);
    while ((buf = LorieBufferList_first(&addedBuffers))) {
        // These buffers are not yet attached to GL, it is safe to release them
        LorieBuffer_release(buf);
    }
    while ((buf = LorieBufferList_first(&buffers))) {
        // These buffers are attached to GL, we must release them from renderer thread.
        LorieBuffer_removeFromList(buf);
        LorieBuffer_addToList(buf, &removedBuffers);
    }
    pthread_spin_unlock(&bufferLock);
}

void rendererSetWindow(ANativeWindow* newWin) {
    pthread_mutex_lock(&stateLock);
    if (newWin && pendingWin == newWin) {
        ANativeWindow_release(newWin);
        pthread_mutex_unlock(&stateLock);
        return;
    }

    if (pendingWin)
        ANativeWindow_release(pendingWin);

    pendingWin = newWin;
    windowChanged = TRUE;

    pthread_cond_signal(&stateCond);

    // We should wait until renderer destroys EGLSurface before SurfaceCallback::surfaceDestroyed finishes
    // Otherwise we will have weird errors like
    // `freeAllBuffers: 1 buffers were freed while being dequeued!`
    // or
    // `query: BufferQueue has been abandoned`
    while(windowChanged)
        pthread_cond_wait(&stateChangeFinishCond, &stateLock);

    pthread_mutex_unlock(&stateLock);
}

static inline __always_inline void releaseWinAndSurface(ANativeWindow** anw, EGLSurface *esfc) {
    if (esfc && *esfc && *esfc != defaultSfc) {
        // Requeue the dequeued buffer, causes flickering during window reconfiguring
        eglSwapBuffers(egl_display, *esfc);
        if (eglMakeCurrent(egl_display, defaultSfc, defaultSfc, ctx) != EGL_TRUE)
            return vprintEglError("eglMakeCurrent failed (EGL_NO_SURFACE)", __LINE__);
        if (eglDestroySurface(egl_display, *esfc) != EGL_TRUE)
            return vprintEglError("eglDestoySurface failed", __LINE__);
        *esfc = defaultSfc;
    }

    if (anw && *anw && *anw != defaultWin) {
        ANativeWindow_release(*anw);
        *anw = defaultWin;
    }
}

void rendererRefreshContext(void) {
    int width = pendingWin ? ANativeWindow_getWidth(pendingWin) : 0;
    int height = pendingWin ? ANativeWindow_getHeight(pendingWin) : 0;
    log("rendererSetWindow %p %d %d", pendingWin, width, height);

    releaseWinAndSurface(&win, &sfc);

    if (pendingWin && (width <= 0 || height <= 0)) {
        log("Xlorie: We've got invalid surface. Probably it became invalid before we started working with it.\n");
        releaseWinAndSurface(&pendingWin, NULL);
    }

    win = pendingWin;
    pendingWin = NULL;
    windowChanged = FALSE;

    if (!win) {
        win = defaultWin;
        eglMakeCurrent(egl_display, defaultSfc, defaultSfc, ctx);
        if (state)
            state->surfaceAvailable = false;
        return;
    }

    sfc = eglCreateWindowSurface(egl_display, cfg, win, NULL);
    if (sfc == EGL_NO_SURFACE)
        return vprintEglError("eglCreateWindowSurface failed", __LINE__);

    if (eglMakeCurrent(egl_display, sfc, sfc, ctx) != EGL_TRUE) {
        if (state)
            state->surfaceAvailable = false;
        return vprintEglError("eglMakeCurrent failed", __LINE__);
    }

    eglSwapInterval(egl_display, 0);

    // We should redraw image at least once right after surface change
    if (state)
        state->surfaceAvailable = state->drawRequested = state->cursor.updated = win != defaultWin;

    glViewport(0, 0, ANativeWindow_getWidth(win), ANativeWindow_getHeight(win));
    log("Xlorie: new surface applied: %p\n", sfc);
}

static void draw(GLuint id, float x0, float y0, float x1, float y1, float xfactor, uint8_t flip);
static void drawCursor(float displayWidth, float displayHeight);

void rendererRedrawLocked(bool* waitingForBuffers) {
    float xfactor = 1.f;
    LorieBuffer_Desc *desc = NULL;
    EGLSync fence;
    // The buffer will not be released until this function ends, but main thread can modify buffer list
    pthread_spin_lock(&bufferLock);
    LorieBuffer *buffer = LorieBufferList_findById(&buffers, state->rootWindowTextureID);
    // Probably X server requested us to draw removed buffer and immediately requested to remove it. Let's display it one last time.
    if (!buffer)
        buffer = LorieBufferList_findById(&removedBuffers, state->rootWindowTextureID);
    if (!buffer)
        *waitingForBuffers = true;
    pthread_spin_unlock(&bufferLock);
    if (!buffer) {
        log("Buffer %llu not found", state->rootWindowTextureID);
        return;
    }

    desc = LorieBuffer_description(buffer);

    // We should signal X server to not use root window while we actively copy it
    lorie_mutex_lock(&state->lock, &state->lockingPid);
    state->drawRequested = FALSE;

    LorieBuffer_bindTexture(buffer);
    if (desc->type == LORIEBUFFER_FD)
        xfactor = (float) desc->width/(float) desc->stride;
    draw(0, -1.f, -1.f, 1.f, 1.f, xfactor, LorieBuffer_isRgba(buffer));
    fence = eglCreateSyncKHR(egl_display, EGL_SYNC_FENCE_KHR, NULL);
    glFlush();

    if (state->cursor.updated) {
        log("Xlorie: updating cursor\n");
        lorie_mutex_lock(&state->cursor.lock, &state->cursor.lockingPid);
        state->cursor.updated = false;
        bindLinearTexture(cursor.id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei) state->cursor.width, (GLsizei) state->cursor.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, state->cursor.bits);
        lorie_mutex_unlock(&state->cursor.lock, &state->cursor.lockingPid);
    }

    state->cursor.moved = FALSE;
    drawCursor((float) (LorieBuffer_getWidth(buffer)), (float) (LorieBuffer_getHeight(buffer)));
    glFlush();

    // Wait until root window drawing is finished before giving control back to X server
    eglClientWaitSyncKHR(egl_display, fence, 0, EGL_FOREVER);
    eglDestroySyncKHR(egl_display, fence);
    state->waitForNextFrame = true;
    lorie_mutex_unlock(&state->lock, &state->lockingPid);

    if (eglSwapBuffers(egl_display, sfc) != EGL_TRUE)
        printEglError("Failed to swap buffers", __LINE__);

    // Perform a little drawing operation to make sure the next buffer is ready on the next invocation of drawing
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, 1, 1);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    fence = eglCreateSyncKHR(egl_display, EGL_SYNC_FENCE_KHR, NULL);
    eglClientWaitSyncKHR(egl_display, fence, 0, EGL_FOREVER);
    eglDestroySyncKHR(egl_display, fence);

    state->renderedFrames++;
}

static inline __always_inline bool rendererShouldWait(bool *waitingForBuffers) {
    static uint64_t lastRequestedBufferId = 0;
    bool buffersChanged;
    pthread_spin_lock(&bufferLock);
    buffersChanged = !xorg_list_is_empty(&addedBuffers) || !xorg_list_is_empty(&removedBuffers);
    pthread_spin_unlock(&bufferLock);
    if (stateChanged || windowChanged || buffersChanged)
        // If there are pending changes we should process them immediately.
        return false;

    if (state) {
        if (lastRequestedBufferId != state->rootWindowTextureID)
            *waitingForBuffers = false;
        lastRequestedBufferId = state->rootWindowTextureID;
    }

    if (!state || !state->surfaceAvailable || state->waitForNextFrame || *waitingForBuffers)
        // Even in the case if there are pending changes, we can not draw it without rendering surface
        return true;

    if (state->drawRequested || state->cursor.moved || state->cursor.updated)
        // X server reported drawing or cursor changes, no need to wait.
        return false;

    // Probably spurious wake, no changes we can work with.
    return true;
}

__noreturn static void* rendererThread(void) {
    LorieBuffer* buf;
    bool waitingForBuffers = false;
    while (true) {
        while (rendererShouldWait(&waitingForBuffers))
            pthread_cond_wait(&stateCond, &stateLock);

        if (stateChanged) {
            struct lorie_shared_server_state* oldState = NULL;
            if (state && pendingState != state)
                oldState = state;

            state = pendingState;
            pendingState = NULL;
            stateChanged = false;
            waitingForBuffers = false;

            if (state)
                state->surfaceAvailable = win != defaultWin;
            else if (win != defaultWin) {
                glClearColor(0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT);
                eglSwapBuffers(egl_display, sfc);
            }

            pthreadCondVarProxyListenOtherCondVar(state ? &state->cond : NULL);

            if (oldState)
                munmap(oldState, sizeof(*oldState));
        }

        if (windowChanged)
            rendererRefreshContext();

        // Attach all pending buffers to GL.
        pthread_spin_lock(&bufferLock);
        while((buf = LorieBufferList_first(&addedBuffers))) {
            LorieBuffer_attachToGL(buf);
            LorieBuffer_addToList(buf, &buffers);
            waitingForBuffers = false;
        }
        pthread_spin_unlock(&bufferLock);

        pthread_cond_signal(&stateChangeFinishCond);
        pthread_mutex_unlock(&stateLock);

        if (state && state->surfaceAvailable && !state->waitForNextFrame && (state->drawRequested || state->cursor.moved || state->cursor.updated))
            rendererRedrawLocked(&waitingForBuffers);

        pthread_spin_lock(&bufferLock);
        // Remove all buffers which were attached to GL.
        while((buf = LorieBufferList_first(&removedBuffers)))
            LorieBuffer_release(buf);
        pthread_spin_unlock(&bufferLock);
        pthread_mutex_lock(&stateLock);
    }
}

static GLuint loadShader(GLenum shaderType, const char* pSource) {
    GLint compiled = 0, infoLen = 0;
    GLuint shader = glCreateShader(shaderType);
    if (!shader)
        return 0;

    glShaderSource(shader, 1, &pSource, NULL);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled)
        return shader;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if (infoLen) {
        char buf[infoLen];
        glGetShaderInfoLog(shader, infoLen, NULL, buf);
        log("renderer: Could not compile shader %d:\n%s\n", shaderType, buf);
    }
    glDeleteShader(shader);

    return 0;
}

static GLuint createProgram(const char* p_vertex_source, const char* p_fragment_source) {
    GLuint program, vertexShader, pixelShader;
    GLint linkStatus = GL_FALSE, bufLength = 0;
    vertexShader = loadShader(GL_VERTEX_SHADER, p_vertex_source);
    pixelShader = loadShader(GL_FRAGMENT_SHADER, p_fragment_source);
    if (!pixelShader || !vertexShader) {
        return 0;
    }

    program = glCreateProgram();
    if (!program)
        return 0;

    glAttachShader(program, vertexShader);
    glAttachShader(program, pixelShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_TRUE)
        return program;

    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
    if (bufLength) {
        char buf[bufLength];
        glGetProgramInfoLog(program, bufLength, NULL, buf);
        log("renderer: Could not link program:\n%s\n", buf);
    }
    glDeleteProgram(program);

    return 0;
}

static void draw(GLuint id, float x0, float y0, float x1, float y1, float xfactor, uint8_t flip) {
    float coords[16] = {
        x0, -y0, 0.f, 0.f,
        x1, -y0, xfactor, 0.f,
        x0, -y1, 0.f, 1.f,
        x1, -y1, xfactor, 1.f,
    };

    GLuint p = flip ? gv_pos_bgra : gv_pos, c = flip ? gv_coords_bgra : gv_coords;

    glActiveTexture(GL_TEXTURE0);
    glUseProgram(flip ? g_texture_program_bgra : g_texture_program);
    if (id)
        glBindTexture(GL_TEXTURE_2D, id);

    glVertexAttribPointer(p, 2, GL_FLOAT, GL_FALSE, 16, coords);
    glVertexAttribPointer(c, 2, GL_FLOAT, GL_FALSE, 16, &coords[2]);
    glEnableVertexAttribArray(p);
    glEnableVertexAttribArray(c);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); checkGlError();
}

__unused static void drawCursor(float displayWidth, float displayHeight) {
    float x, y, w, h;

    if (!state->cursor.width || !state->cursor.height)
        return;

    x = 2.f * ((float) state->cursor.x - (float) state->cursor.xhot) / displayWidth - 1.f;
    y = 2.f * ((float) state->cursor.y - (float) state->cursor.yhot) / displayHeight - 1.f;
    w = 2.f * (float) state->cursor.width / displayWidth;
    h = 2.f * (float) state->cursor.height / displayHeight;
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw(cursor.id, x, y, x + w, y + h, 1.f, false);
    glDisable(GL_BLEND);
}

// auxillary pthread condition var proxy shenanigans
static volatile struct {
    pthread_mutex_t lock;
    pthread_cond_t def, *current, *pending;
    bool relocked;
} proxy = { .current = &proxy.def };

static void pthreadCondVarProxyInit(void) {
    pthread_t t;
    pthread_mutex_init(&proxy.lock, NULL);
    pthread_cond_init(&proxy.def, NULL);
    pthread_create(&t, NULL, pthreadCondVarProxyThread, NULL);
}

__noreturn static void* pthreadCondVarProxyThread(void* cookie) {
    // We can not wait for two conditional variables simultaneously.
    // But we are required to listen for both remote and local events.
    // This thread waits for remote signals and proxies them to the local cond var.
    pthread_setname_np(pthread_self(), "PthreadCondVarProxy");
    pthread_mutex_lock(&proxy.lock);
    log("pthreadCondVarProxyThread %ld started", pthread_self());
    while (true) {
        pthread_cond_wait(proxy.current, &proxy.lock);
        if (proxy.pending) {
            proxy.current = proxy.pending;
            proxy.pending = NULL;
        }
        proxy.relocked = true;
        pthread_cond_signal(&stateCond);
    }
}

static void pthreadCondVarProxyListenOtherCondVar(pthread_cond_t* var) {
    pthread_mutex_lock(&proxy.lock);
    pthread_cond_broadcast(proxy.current);
    proxy.pending = var ?: &proxy.def;
    proxy.relocked = false;
    while(!proxy.relocked)
        pthread_cond_wait(&stateCond, &proxy.lock);
    pthread_mutex_unlock(&proxy.lock);
}
