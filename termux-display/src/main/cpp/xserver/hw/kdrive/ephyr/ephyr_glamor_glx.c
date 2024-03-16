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

/** @file ephyr_glamor_glx.c
 *
 * Separate file for hiding Xlib and GLX-using parts of xephyr from
 * the rest of the server-struct-aware build.
 */

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#undef Xcalloc
#undef Xrealloc
#undef Xfree
#include <X11/Xlib-xcb.h>
#include <xcb/xcb_aux.h>
#include <pixman.h>
#include <epoxy/glx.h>
#include "ephyr_glamor_glx.h"
#include "os.h"
#include <X11/Xproto.h>

/* until we need geometry shaders GL3.1 should suffice. */
/* Xephyr has its own copy of this for build reasons */
#define GLAMOR_GL_CORE_VER_MAJOR 3
#define GLAMOR_GL_CORE_VER_MINOR 1
/** @{
 *
 * global state for Xephyr with glamor.
 *
 * Xephyr can render with multiple windows, but all the windows have
 * to be on the same X connection and all have to have the same
 * visual.
 */
static Display *dpy;
static XVisualInfo *visual_info;
static GLXFBConfig fb_config;
Bool ephyr_glamor_gles2;
Bool ephyr_glamor_skip_present;
/** @} */

/**
 * Per-screen state for Xephyr with glamor.
 */
struct ephyr_glamor {
    GLXContext ctx;
    Window win;
    GLXWindow glx_win;

    GLuint tex;

    GLuint texture_shader;
    GLuint texture_shader_position_loc;
    GLuint texture_shader_texcoord_loc;

    /* Size of the window that we're rendering to. */
    unsigned width, height;

    GLuint vao, vbo;
};

static GLint
ephyr_glamor_compile_glsl_prog(GLenum type, const char *source)
{
    GLint ok;
    GLint prog;

    prog = glCreateShader(type);
    glShaderSource(prog, 1, (const GLchar **) &source, NULL);
    glCompileShader(prog);
    glGetShaderiv(prog, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLchar *info;
        GLint size;

        glGetShaderiv(prog, GL_INFO_LOG_LENGTH, &size);
        info = malloc(size);
        if (info) {
            glGetShaderInfoLog(prog, size, NULL, info);
            ErrorF("Failed to compile %s: %s\n",
                   type == GL_FRAGMENT_SHADER ? "FS" : "VS", info);
            ErrorF("Program source:\n%s", source);
            free(info);
        }
        else
            ErrorF("Failed to get shader compilation info.\n");
        FatalError("GLSL compile failure\n");
    }

    return prog;
}

static GLuint
ephyr_glamor_build_glsl_prog(GLuint vs, GLuint fs)
{
    GLint ok;
    GLuint prog;

    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);

    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLchar *info;
        GLint size;

        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &size);
        info = malloc(size);

        glGetProgramInfoLog(prog, size, NULL, info);
        ErrorF("Failed to link: %s\n", info);
        FatalError("GLSL link failure\n");
    }

    return prog;
}

static void
ephyr_glamor_setup_texturing_shader(struct ephyr_glamor *glamor)
{
    const char *vs_source =
        "attribute vec2 texcoord;\n"
        "attribute vec2 position;\n"
        "varying vec2 t;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    t = texcoord;\n"
        "    gl_Position = vec4(position, 0, 1);\n"
        "}\n";

    const char *fs_source =
        "#ifdef GL_ES\n"
        "precision mediump float;\n"
        "#endif\n"
        "\n"
        "varying vec2 t;\n"
        "uniform sampler2D s; /* initially 0 */\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_FragColor = texture2D(s, t);\n"
        "}\n";

    GLuint fs, vs, prog;

    vs = ephyr_glamor_compile_glsl_prog(GL_VERTEX_SHADER, vs_source);
    fs = ephyr_glamor_compile_glsl_prog(GL_FRAGMENT_SHADER, fs_source);
    prog = ephyr_glamor_build_glsl_prog(vs, fs);

    glamor->texture_shader = prog;
    glamor->texture_shader_position_loc = glGetAttribLocation(prog, "position");
    assert(glamor->texture_shader_position_loc != -1);
    glamor->texture_shader_texcoord_loc = glGetAttribLocation(prog, "texcoord");
    assert(glamor->texture_shader_texcoord_loc != -1);
}

xcb_connection_t *
ephyr_glamor_connect(void)
{
    dpy = XOpenDisplay(NULL);
    if (!dpy)
        return NULL;

    XSetEventQueueOwner(dpy, XCBOwnsEventQueue);

    return XGetXCBConnection(dpy);
}

void
ephyr_glamor_set_texture(struct ephyr_glamor *glamor, uint32_t tex)
{
    glamor->tex = tex;
}

static void
ephyr_glamor_set_vertices(struct ephyr_glamor *glamor)
{
    glVertexAttribPointer(glamor->texture_shader_position_loc,
                          2, GL_FLOAT, FALSE, 0, (void *) 0);
    glVertexAttribPointer(glamor->texture_shader_texcoord_loc,
                          2, GL_FLOAT, FALSE, 0, (void *) (sizeof (float) * 8));

    glEnableVertexAttribArray(glamor->texture_shader_position_loc);
    glEnableVertexAttribArray(glamor->texture_shader_texcoord_loc);
}

void
ephyr_glamor_damage_redisplay(struct ephyr_glamor *glamor,
                              struct pixman_region16 *damage)
{
    GLint old_vao;

    /* Skip presenting the output in this mode.  Presentation is
     * expensive, and if we're just running the X Test suite headless,
     * nobody's watching.
     */
    if (ephyr_glamor_skip_present)
        return;

    glXMakeCurrent(dpy, glamor->glx_win, glamor->ctx);

    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);
    glBindVertexArray(glamor->vao);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(glamor->texture_shader);
    glViewport(0, 0, glamor->width, glamor->height);
    if (!ephyr_glamor_gles2)
        glDisable(GL_COLOR_LOGIC_OP);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, glamor->tex);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glBindVertexArray(old_vao);

    glXSwapBuffers(dpy, glamor->glx_win);
}

/**
 * Xlib-based handling of xcb events for glamor.
 *
 * We need to let the Xlib event filtering run on the event so that
 * Mesa's dri2_glx.c userspace event mangling gets run, and we
 * correctly get our invalidate events propagated into the driver.
 */
void
ephyr_glamor_process_event(xcb_generic_event_t *xev)
{

    uint32_t response_type = xev->response_type & 0x7f;
    /* Note the types on wire_to_event: there's an Xlib XEvent (with
     * the broken types) that it returns, and a protocol xEvent that
     * it inspects.
     */
    Bool (*wire_to_event)(Display *dpy, XEvent *ret, xEvent *event);

    XLockDisplay(dpy);
    /* Set the event handler to NULL to get access to the current one. */
    wire_to_event = XESetWireToEvent(dpy, response_type, NULL);
    if (wire_to_event) {
        XEvent processed_event;

        /* OK they had an event handler.  Plug it back in, and call
         * through to it.
         */
        XESetWireToEvent(dpy, response_type, wire_to_event);
        xev->sequence = LastKnownRequestProcessed(dpy);
        wire_to_event(dpy, &processed_event, (xEvent *)xev);
    }
    XUnlockDisplay(dpy);
}

static int
ephyr_glx_error_handler(Display * _dpy, XErrorEvent * ev)
{
    return 0;
}

struct ephyr_glamor *
ephyr_glamor_glx_screen_init(xcb_window_t win)
{
    int (*oldErrorHandler) (Display *, XErrorEvent *);
    static const float position[] = {
        -1, -1,
         1, -1,
         1,  1,
        -1,  1,
        0, 1,
        1, 1,
        1, 0,
        0, 0,
    };
    GLint old_vao;

    GLXContext ctx;
    struct ephyr_glamor *glamor;
    GLXWindow glx_win;

    glamor = calloc(1, sizeof(struct ephyr_glamor));
    if (!glamor) {
        FatalError("malloc");
        return NULL;
    }

    glx_win = glXCreateWindow(dpy, fb_config, win, NULL);

    if (ephyr_glamor_gles2) {
        static const int context_attribs[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 2,
            GLX_CONTEXT_MINOR_VERSION_ARB, 0,
            GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_ES_PROFILE_BIT_EXT,
            0,
        };
        if (epoxy_has_glx_extension(dpy, DefaultScreen(dpy),
                                    "GLX_EXT_create_context_es2_profile")) {
            ctx = glXCreateContextAttribsARB(dpy, fb_config, NULL, True,
                                             context_attribs);
        } else {
            FatalError("Xephyr -glamor_gles2 requires "
                       "GLX_EXT_create_context_es2_profile\n");
        }
    } else {
        if (epoxy_has_glx_extension(dpy, DefaultScreen(dpy),
                                    "GLX_ARB_create_context")) {
            static const int context_attribs[] = {
                GLX_CONTEXT_PROFILE_MASK_ARB,
                GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                GLX_CONTEXT_MAJOR_VERSION_ARB,
                GLAMOR_GL_CORE_VER_MAJOR,
                GLX_CONTEXT_MINOR_VERSION_ARB,
                GLAMOR_GL_CORE_VER_MINOR,
                0,
            };
            oldErrorHandler = XSetErrorHandler(ephyr_glx_error_handler);
            ctx = glXCreateContextAttribsARB(dpy, fb_config, NULL, True,
                                             context_attribs);
            XSync(dpy, False);
            XSetErrorHandler(oldErrorHandler);
        } else {
            ctx = NULL;
        }

        if (!ctx)
            ctx = glXCreateContext(dpy, visual_info, NULL, True);
    }
    if (ctx == NULL)
        FatalError("glXCreateContext failed\n");

    if (!glXMakeCurrent(dpy, glx_win, ctx))
        FatalError("glXMakeCurrent failed\n");

    glamor->ctx = ctx;
    glamor->win = win;
    glamor->glx_win = glx_win;
    ephyr_glamor_setup_texturing_shader(glamor);

    glGenVertexArrays(1, &glamor->vao);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &old_vao);
    glBindVertexArray(glamor->vao);

    glGenBuffers(1, &glamor->vbo);

    glBindBuffer(GL_ARRAY_BUFFER, glamor->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof (position), position, GL_STATIC_DRAW);

    ephyr_glamor_set_vertices(glamor);
    glBindVertexArray(old_vao);

    return glamor;
}

void
ephyr_glamor_glx_screen_fini(struct ephyr_glamor *glamor)
{
    glXMakeCurrent(dpy, None, NULL);
    glXDestroyContext(dpy, glamor->ctx);
    glXDestroyWindow(dpy, glamor->glx_win);

    free(glamor);
}

xcb_visualtype_t *
ephyr_glamor_get_visual(void)
{
    xcb_screen_t *xscreen =
        xcb_aux_get_screen(XGetXCBConnection(dpy), DefaultScreen(dpy));
    int attribs[] = {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_DOUBLEBUFFER, 1,
        None
    };
    int event_base = 0, error_base = 0, nelements;
    GLXFBConfig *fbconfigs;

    if (!glXQueryExtension (dpy, &error_base, &event_base))
        FatalError("Couldn't find GLX extension\n");

    fbconfigs = glXChooseFBConfig(dpy, DefaultScreen(dpy), attribs, &nelements);
    if (!nelements)
        FatalError("Couldn't choose an FBConfig\n");
    fb_config = fbconfigs[0];
    free(fbconfigs);

    visual_info = glXGetVisualFromFBConfig(dpy, fb_config);
    if (visual_info == NULL)
        FatalError("Couldn't get RGB visual\n");

    return xcb_aux_find_visual_by_id(xscreen, visual_info->visualid);
}

void
ephyr_glamor_set_window_size(struct ephyr_glamor *glamor,
                             unsigned width, unsigned height)
{
    if (!glamor)
        return;

    glamor->width = width;
    glamor->height = height;
}
