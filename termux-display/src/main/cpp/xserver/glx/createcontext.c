/*
 * Copyright © 2011 Intel Corporation
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
#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <GL/glxtokens.h>
#include "glxserver.h"
#include "glxext.h"
#include "indirect_dispatch.h"
#include "opaque.h"

#define ALL_VALID_FLAGS \
    (GLX_CONTEXT_DEBUG_BIT_ARB | GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB \
     | GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB)

static Bool
validate_GL_version(int major_version, int minor_version)
{
    if (major_version <= 0 || minor_version < 0)
        return FALSE;

    switch (major_version) {
    case 1:
        if (minor_version > 5)
            return FALSE;
        break;

    case 2:
        if (minor_version > 1)
            return FALSE;
        break;

    case 3:
        if (minor_version > 3)
            return FALSE;
        break;

    default:
        break;
    }

    return TRUE;
}

static Bool
validate_render_type(uint32_t render_type)
{
    switch (render_type) {
    case GLX_RGBA_TYPE:
    case GLX_COLOR_INDEX_TYPE:
    case GLX_RGBA_FLOAT_TYPE_ARB:
    case GLX_RGBA_UNSIGNED_FLOAT_TYPE_EXT:
        return TRUE;
    default:
        return FALSE;
    }
}

int
__glXDisp_CreateContextAttribsARB(__GLXclientState * cl, GLbyte * pc)
{
    ClientPtr client = cl->client;
    xGLXCreateContextAttribsARBReq *req = (xGLXCreateContextAttribsARBReq *) pc;
    int32_t *attribs = (req->numAttribs != 0) ? (int32_t *) (req + 1) : NULL;
    unsigned i;
    int major_version = 1;
    int minor_version = 0;
    uint32_t flags = 0;
    uint32_t render_type = GLX_RGBA_TYPE;
    uint32_t flush = GLX_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB;
    __GLXcontext *ctx = NULL;
    __GLXcontext *shareCtx = NULL;
    __GLXscreen *glxScreen;
    __GLXconfig *config = NULL;
    int err;

    /* The GLX_ARB_create_context_robustness spec says:
     *
     *     "The default value for GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB
     *     is GLX_NO_RESET_NOTIFICATION_ARB."
     */
    int reset = GLX_NO_RESET_NOTIFICATION_ARB;

    /* The GLX_ARB_create_context_profile spec says:
     *
     *     "The default value for GLX_CONTEXT_PROFILE_MASK_ARB is
     *     GLX_CONTEXT_CORE_PROFILE_BIT_ARB."
     *
     * The core profile only makes sense for OpenGL versions 3.2 and later.
     * If the version ultimately specified is less than 3.2, the core profile
     * bit is cleared (see below).
     */
    int profile = GLX_CONTEXT_CORE_PROFILE_BIT_ARB;

    /* Verify that the size of the packet matches the size inferred from the
     * sizes specified for the various fields.
     */
    const unsigned expected_size = (sz_xGLXCreateContextAttribsARBReq
                                    + (req->numAttribs * 8)) / 4;

    if (req->length != expected_size)
        return BadLength;

    /* The GLX_ARB_create_context spec says:
     *
     *     "* If <config> is not a valid GLXFBConfig, GLXBadFBConfig is
     *        generated."
     *
     * On the client, the screen comes from the FBConfig, so GLXBadFBConfig
     * should be issued if the screen is nonsense.
     */
    if (!validGlxScreen(client, req->screen, &glxScreen, &err)) {
        client->errorValue = req->fbconfig;
        return __glXError(GLXBadFBConfig);
    }

    if (req->fbconfig) {
        if (!validGlxFBConfig(client, glxScreen, req->fbconfig, &config, &err)) {
            client->errorValue = req->fbconfig;
            return __glXError(GLXBadFBConfig);
        }
    }

    /* Validate the context with which the new context should share resources.
     */
    if (req->shareList != None) {
        if (!validGlxContext(client, req->shareList, DixReadAccess,
                             &shareCtx, &err))
            return err;

        /* The crazy condition is because C doesn't have a logical XOR
         * operator.  Comparing directly for equality may fail if one is 1 and
         * the other is 2 even though both are logically true.
         */
        if (!!req->isDirect != !!shareCtx->isDirect) {
            client->errorValue = req->shareList;
            return BadMatch;
        }

        /* The GLX_ARB_create_context spec says:
         *
         *     "* If the server context state for <share_context>...was
         *        created on a different screen than the one referenced by
         *        <config>...BadMatch is generated."
         */
        if (glxScreen != shareCtx->pGlxScreen) {
            client->errorValue = shareCtx->pGlxScreen->pScreen->myNum;
            return BadMatch;
        }
    }

    for (i = 0; i < req->numAttribs; i++) {
        switch (attribs[i * 2]) {
        case GLX_CONTEXT_MAJOR_VERSION_ARB:
            major_version = attribs[2 * i + 1];
            break;

        case GLX_CONTEXT_MINOR_VERSION_ARB:
            minor_version = attribs[2 * i + 1];
            break;

        case GLX_CONTEXT_FLAGS_ARB:
            flags = attribs[2 * i + 1];
            break;

        case GLX_RENDER_TYPE:
            /* Not valid for GLX_EXT_no_config_context */
            if (!req->fbconfig)
                return BadValue;
            render_type = attribs[2 * i + 1];
            break;

        case GLX_CONTEXT_PROFILE_MASK_ARB:
            profile = attribs[2 * i + 1];
            break;

        case GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB:
            reset = attribs[2 * i + 1];
            if (reset != GLX_NO_RESET_NOTIFICATION_ARB
                && reset != GLX_LOSE_CONTEXT_ON_RESET_ARB)
                return BadValue;

            break;

        case GLX_CONTEXT_RELEASE_BEHAVIOR_ARB:
            flush = attribs[2 * i + 1];
            if (flush != GLX_CONTEXT_RELEASE_BEHAVIOR_NONE_ARB
                && flush != GLX_CONTEXT_RELEASE_BEHAVIOR_FLUSH_ARB)
                return BadValue;
            break;

        case GLX_SCREEN:
            /* Only valid for GLX_EXT_no_config_context */
            if (req->fbconfig)
                return BadValue;
            /* Must match the value in the request header */
            if (attribs[2 * i + 1] != req->screen)
                return BadValue;
            break;

        case GLX_CONTEXT_OPENGL_NO_ERROR_ARB:
            /* ignore */
            break;

        default:
            if (!req->isDirect)
                return BadValue;
            break;
        }
    }

    /* The GLX_ARB_create_context spec says:
     *
     *     "If attributes GLX_CONTEXT_MAJOR_VERSION_ARB and
     *     GLX_CONTEXT_MINOR_VERSION_ARB, when considered together
     *     with attributes GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB and
     *     GLX_RENDER_TYPE, specify an OpenGL version and feature set
     *     that are not defined, BadMatch is generated.
     *
     *     ...Feature deprecation was introduced with OpenGL 3.0, so
     *     forward-compatible contexts may only be requested for
     *     OpenGL 3.0 and above. Thus, examples of invalid
     *     combinations of attributes include:
     *
     *       - Major version < 1 or > 3
     *       - Major version == 1 and minor version < 0 or > 5
     *       - Major version == 2 and minor version < 0 or > 1
     *       - Major version == 3 and minor version > 2
     *       - Forward-compatible flag set and major version < 3
     *       - Color index rendering and major version >= 3"
     */
    if (!validate_GL_version(major_version, minor_version))
        return BadMatch;

    if (major_version < 3
        && ((flags & GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB) != 0))
        return BadMatch;

    if (major_version >= 3 && render_type == GLX_COLOR_INDEX_TYPE)
        return BadMatch;

    if (!validate_render_type(render_type))
        return BadValue;

    if ((flags & ~ALL_VALID_FLAGS) != 0)
        return BadValue;

    /* The GLX_ARB_create_context_profile spec says:
     *
     *     "* If attribute GLX_CONTEXT_PROFILE_MASK_ARB has no bits set; has
     *        any bits set other than GLX_CONTEXT_CORE_PROFILE_BIT_ARB and
     *        GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB; has more than one of
     *        these bits set; or if the implementation does not support the
     *        requested profile, then GLXBadProfileARB is generated."
     *
     * The GLX_EXT_create_context_es2_profile spec doesn't exactly say what
     * is supposed to happen if an invalid version is set, but it doesn't
     * much matter as support for GLES contexts is only defined for direct
     * contexts (at the moment anyway) so we can leave it up to the driver
     * to validate.
     */
    switch (profile) {
    case GLX_CONTEXT_CORE_PROFILE_BIT_ARB:
    case GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB:
    case GLX_CONTEXT_ES2_PROFILE_BIT_EXT:
        break;
    default:
        return __glXError(GLXBadProfileARB);
    }

    /* The GLX_ARB_create_context_robustness spec says:
     *
     *     "* If the reset notification behavior of <share_context> and the
     *        newly created context are different, BadMatch is generated."
     */
    if (shareCtx != NULL && shareCtx->resetNotificationStrategy != reset)
        return BadMatch;

    /* There is no GLX protocol for desktop OpenGL versions after 1.4.  There
     * is no GLX protocol for any version of OpenGL ES.  If the application is
     * requested an indirect rendering context for a version that cannot be
     * satisfied, reject it.
     *
     * The GLX_ARB_create_context spec says:
     *
     *     "* If <config> does not support compatible OpenGL contexts
     *        providing the requested API major and minor version,
     *        forward-compatible flag, and debug context flag, GLXBadFBConfig
     *        is generated."
     */
    if (!req->isDirect && (major_version > 1 || minor_version > 4
                           || profile == GLX_CONTEXT_ES2_PROFILE_BIT_EXT)) {
        client->errorValue = req->fbconfig;
        return __glXError(GLXBadFBConfig);
    }

    /* Allocate memory for the new context
     */
    if (req->isDirect) {
        ctx = __glXdirectContextCreate(glxScreen, config, shareCtx);
        err = BadAlloc;
    }
    else {
        /* Only allow creating indirect GLX contexts if allowed by
         * server command line.  Indirect GLX is of limited use (since
         * it's only GL 1.4), it's slower than direct contexts, and
         * it's a massive attack surface for buffer overflow type
         * errors.
         */
        if (!enableIndirectGLX) {
            client->errorValue = req->isDirect;
            return BadValue;
        }

        ctx = glxScreen->createContext(glxScreen, config, shareCtx,
                                       req->numAttribs, (uint32_t *) attribs,
                                       &err);
    }

    if (ctx == NULL)
        return err;

    ctx->pGlxScreen = glxScreen;
    ctx->config = config;
    ctx->id = req->context;
    ctx->share_id = req->shareList;
    ctx->idExists = TRUE;
    ctx->isDirect = req->isDirect;
    ctx->renderMode = GL_RENDER;
    ctx->resetNotificationStrategy = reset;
    ctx->releaseBehavior = flush;
    ctx->renderType = render_type;

    /* Add the new context to the various global tables of GLX contexts.
     */
    if (!__glXAddContext(ctx)) {
        (*ctx->destroy) (ctx);
        client->errorValue = req->context;
        return BadAlloc;
    }

    return Success;
}

int
__glXDispSwap_CreateContextAttribsARB(__GLXclientState * cl, GLbyte * pc)
{
    return BadRequest;
}
