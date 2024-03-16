/* DO NOT EDIT - This file generated automatically by glX_proto_recv.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <inttypes.h>
#include "glxserver.h"
#include "indirect_size.h"
#include "indirect_size_get.h"
#include "indirect_dispatch.h"
#include "glxbyteorder.h"
#include "indirect_util.h"
#include "singlesize.h"

#define __GLX_PAD(x)  (((x) + 3) & ~3)

typedef struct {
    __GLX_PIXEL_3D_HDR;
} __GLXpixel3DHeader;

extern GLboolean __glXErrorOccured(void);
extern void __glXClearErrorOccured(void);

static const unsigned dummy_answer[2] = { 0, 0 };

static GLsizei
bswap_CARD32(const void *src)
{
    union {
        uint32_t dst;
        GLsizei ret;
    } x;

    x.dst = bswap_32(*(uint32_t *) src);
    return x.ret;
}

static GLshort
bswap_CARD16(const void *src)
{
    union {
        uint16_t dst;
        GLshort ret;
    } x;

    x.dst = bswap_16(*(uint16_t *) src);
    return x.ret;
}

static GLenum
bswap_ENUM(const void *src)
{
    union {
        uint32_t dst;
        GLenum ret;
    } x;

    x.dst = bswap_32(*(uint32_t *) src);
    return x.ret;
}

static GLdouble
bswap_FLOAT64(const void *src)
{
    union {
        uint64_t dst;
        GLdouble ret;
    } x;

    x.dst = bswap_64(*(uint64_t *) src);
    return x.ret;
}

static GLfloat
bswap_FLOAT32(const void *src)
{
    union {
        uint32_t dst;
        GLfloat ret;
    } x;

    x.dst = bswap_32(*(uint32_t *) src);
    return x.ret;
}

static void *
bswap_16_array(uint16_t * src, unsigned count)
{
    unsigned i;

    for (i = 0; i < count; i++) {
        uint16_t temp = bswap_16(src[i]);

        src[i] = temp;
    }

    return src;
}

static void *
bswap_32_array(uint32_t * src, unsigned count)
{
    unsigned i;

    for (i = 0; i < count; i++) {
        uint32_t temp = bswap_32(src[i]);

        src[i] = temp;
    }

    return src;
}

static void *
bswap_64_array(uint64_t * src, unsigned count)
{
    unsigned i;

    for (i = 0; i < count; i++) {
        uint64_t temp = bswap_64(src[i]);

        src[i] = temp;
    }

    return src;
}

int
__glXDispSwap_NewList(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        glNewList((GLuint) bswap_CARD32(pc + 0), (GLenum) bswap_ENUM(pc + 4));
        error = Success;
    }

    return error;
}

int
__glXDispSwap_EndList(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        glEndList();
        error = Success;
    }

    return error;
}

void
__glXDispSwap_CallList(GLbyte * pc)
{
    glCallList((GLuint) bswap_CARD32(pc + 0));
}

void
__glXDispSwap_CallLists(GLbyte * pc)
{
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);
    const GLenum type = (GLenum) bswap_ENUM(pc + 4);
    const GLvoid *lists;

    switch (type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_2_BYTES:
    case GL_3_BYTES:
    case GL_4_BYTES:
        lists = (const GLvoid *) (pc + 8);
        break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        lists = (const GLvoid *) bswap_16_array((uint16_t *) (pc + 8), n);
        break;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        lists = (const GLvoid *) bswap_32_array((uint32_t *) (pc + 8), n);
        break;
    default:
        return;
    }

    glCallLists(n, type, lists);
}

int
__glXDispSwap_DeleteLists(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        glDeleteLists((GLuint) bswap_CARD32(pc + 0),
                      (GLsizei) bswap_CARD32(pc + 4));
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GenLists(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        GLuint retval;

        retval = glGenLists((GLsizei) bswap_CARD32(pc + 0));
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_ListBase(GLbyte * pc)
{
    glListBase((GLuint) bswap_CARD32(pc + 0));
}

void
__glXDispSwap_Begin(GLbyte * pc)
{
    glBegin((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_Bitmap(GLbyte * pc)
{
    const GLubyte *const bitmap = (const GLubyte *) ((pc + 44));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glBitmap((GLsizei) bswap_CARD32(pc + 20),
             (GLsizei) bswap_CARD32(pc + 24),
             (GLfloat) bswap_FLOAT32(pc + 28),
             (GLfloat) bswap_FLOAT32(pc + 32),
             (GLfloat) bswap_FLOAT32(pc + 36),
             (GLfloat) bswap_FLOAT32(pc + 40), bitmap);
}

void
__glXDispSwap_Color3bv(GLbyte * pc)
{
    glColor3bv((const GLbyte *) (pc + 0));
}

void
__glXDispSwap_Color3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 24);
        pc -= 4;
    }
#endif

    glColor3dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 3));
}

void
__glXDispSwap_Color3fv(GLbyte * pc)
{
    glColor3fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_Color3iv(GLbyte * pc)
{
    glColor3iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_Color3sv(GLbyte * pc)
{
    glColor3sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 3));
}

void
__glXDispSwap_Color3ubv(GLbyte * pc)
{
    glColor3ubv((const GLubyte *) (pc + 0));
}

void
__glXDispSwap_Color3uiv(GLbyte * pc)
{
    glColor3uiv((const GLuint *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_Color3usv(GLbyte * pc)
{
    glColor3usv((const GLushort *) bswap_16_array((uint16_t *) (pc + 0), 3));
}

void
__glXDispSwap_Color4bv(GLbyte * pc)
{
    glColor4bv((const GLbyte *) (pc + 0));
}

void
__glXDispSwap_Color4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 32);
        pc -= 4;
    }
#endif

    glColor4dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 4));
}

void
__glXDispSwap_Color4fv(GLbyte * pc)
{
    glColor4fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 4));
}

void
__glXDispSwap_Color4iv(GLbyte * pc)
{
    glColor4iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 4));
}

void
__glXDispSwap_Color4sv(GLbyte * pc)
{
    glColor4sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 4));
}

void
__glXDispSwap_Color4ubv(GLbyte * pc)
{
    glColor4ubv((const GLubyte *) (pc + 0));
}

void
__glXDispSwap_Color4uiv(GLbyte * pc)
{
    glColor4uiv((const GLuint *) bswap_32_array((uint32_t *) (pc + 0), 4));
}

void
__glXDispSwap_Color4usv(GLbyte * pc)
{
    glColor4usv((const GLushort *) bswap_16_array((uint16_t *) (pc + 0), 4));
}

void
__glXDispSwap_EdgeFlagv(GLbyte * pc)
{
    glEdgeFlagv((const GLboolean *) (pc + 0));
}

void
__glXDispSwap_End(GLbyte * pc)
{
    glEnd();
}

void
__glXDispSwap_Indexdv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 8);
        pc -= 4;
    }
#endif

    glIndexdv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 1));
}

void
__glXDispSwap_Indexfv(GLbyte * pc)
{
    glIndexfv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 1));
}

void
__glXDispSwap_Indexiv(GLbyte * pc)
{
    glIndexiv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 1));
}

void
__glXDispSwap_Indexsv(GLbyte * pc)
{
    glIndexsv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 1));
}

void
__glXDispSwap_Normal3bv(GLbyte * pc)
{
    glNormal3bv((const GLbyte *) (pc + 0));
}

void
__glXDispSwap_Normal3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 24);
        pc -= 4;
    }
#endif

    glNormal3dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 3));
}

void
__glXDispSwap_Normal3fv(GLbyte * pc)
{
    glNormal3fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_Normal3iv(GLbyte * pc)
{
    glNormal3iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_Normal3sv(GLbyte * pc)
{
    glNormal3sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 3));
}

void
__glXDispSwap_RasterPos2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 16);
        pc -= 4;
    }
#endif

    glRasterPos2dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 2));
}

void
__glXDispSwap_RasterPos2fv(GLbyte * pc)
{
    glRasterPos2fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 2));
}

void
__glXDispSwap_RasterPos2iv(GLbyte * pc)
{
    glRasterPos2iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 2));
}

void
__glXDispSwap_RasterPos2sv(GLbyte * pc)
{
    glRasterPos2sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 2));
}

void
__glXDispSwap_RasterPos3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 24);
        pc -= 4;
    }
#endif

    glRasterPos3dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 3));
}

void
__glXDispSwap_RasterPos3fv(GLbyte * pc)
{
    glRasterPos3fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_RasterPos3iv(GLbyte * pc)
{
    glRasterPos3iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_RasterPos3sv(GLbyte * pc)
{
    glRasterPos3sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 3));
}

void
__glXDispSwap_RasterPos4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 32);
        pc -= 4;
    }
#endif

    glRasterPos4dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 4));
}

void
__glXDispSwap_RasterPos4fv(GLbyte * pc)
{
    glRasterPos4fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 4));
}

void
__glXDispSwap_RasterPos4iv(GLbyte * pc)
{
    glRasterPos4iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 4));
}

void
__glXDispSwap_RasterPos4sv(GLbyte * pc)
{
    glRasterPos4sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 4));
}

void
__glXDispSwap_Rectdv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 32);
        pc -= 4;
    }
#endif

    glRectdv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 2),
             (const GLdouble *) bswap_64_array((uint64_t *) (pc + 16), 2));
}

void
__glXDispSwap_Rectfv(GLbyte * pc)
{
    glRectfv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 2),
             (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8), 2));
}

void
__glXDispSwap_Rectiv(GLbyte * pc)
{
    glRectiv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 2),
             (const GLint *) bswap_32_array((uint32_t *) (pc + 8), 2));
}

void
__glXDispSwap_Rectsv(GLbyte * pc)
{
    glRectsv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 2),
             (const GLshort *) bswap_16_array((uint16_t *) (pc + 4), 2));
}

void
__glXDispSwap_TexCoord1dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 8);
        pc -= 4;
    }
#endif

    glTexCoord1dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 1));
}

void
__glXDispSwap_TexCoord1fv(GLbyte * pc)
{
    glTexCoord1fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 1));
}

void
__glXDispSwap_TexCoord1iv(GLbyte * pc)
{
    glTexCoord1iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 1));
}

void
__glXDispSwap_TexCoord1sv(GLbyte * pc)
{
    glTexCoord1sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 1));
}

void
__glXDispSwap_TexCoord2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 16);
        pc -= 4;
    }
#endif

    glTexCoord2dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 2));
}

void
__glXDispSwap_TexCoord2fv(GLbyte * pc)
{
    glTexCoord2fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 2));
}

void
__glXDispSwap_TexCoord2iv(GLbyte * pc)
{
    glTexCoord2iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 2));
}

void
__glXDispSwap_TexCoord2sv(GLbyte * pc)
{
    glTexCoord2sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 2));
}

void
__glXDispSwap_TexCoord3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 24);
        pc -= 4;
    }
#endif

    glTexCoord3dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 3));
}

void
__glXDispSwap_TexCoord3fv(GLbyte * pc)
{
    glTexCoord3fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_TexCoord3iv(GLbyte * pc)
{
    glTexCoord3iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_TexCoord3sv(GLbyte * pc)
{
    glTexCoord3sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 3));
}

void
__glXDispSwap_TexCoord4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 32);
        pc -= 4;
    }
#endif

    glTexCoord4dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 4));
}

void
__glXDispSwap_TexCoord4fv(GLbyte * pc)
{
    glTexCoord4fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 4));
}

void
__glXDispSwap_TexCoord4iv(GLbyte * pc)
{
    glTexCoord4iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 4));
}

void
__glXDispSwap_TexCoord4sv(GLbyte * pc)
{
    glTexCoord4sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 4));
}

void
__glXDispSwap_Vertex2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 16);
        pc -= 4;
    }
#endif

    glVertex2dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 2));
}

void
__glXDispSwap_Vertex2fv(GLbyte * pc)
{
    glVertex2fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 2));
}

void
__glXDispSwap_Vertex2iv(GLbyte * pc)
{
    glVertex2iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 2));
}

void
__glXDispSwap_Vertex2sv(GLbyte * pc)
{
    glVertex2sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 2));
}

void
__glXDispSwap_Vertex3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 24);
        pc -= 4;
    }
#endif

    glVertex3dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 3));
}

void
__glXDispSwap_Vertex3fv(GLbyte * pc)
{
    glVertex3fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_Vertex3iv(GLbyte * pc)
{
    glVertex3iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_Vertex3sv(GLbyte * pc)
{
    glVertex3sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 3));
}

void
__glXDispSwap_Vertex4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 32);
        pc -= 4;
    }
#endif

    glVertex4dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 4));
}

void
__glXDispSwap_Vertex4fv(GLbyte * pc)
{
    glVertex4fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 4));
}

void
__glXDispSwap_Vertex4iv(GLbyte * pc)
{
    glVertex4iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 4));
}

void
__glXDispSwap_Vertex4sv(GLbyte * pc)
{
    glVertex4sv((const GLshort *) bswap_16_array((uint16_t *) (pc + 0), 4));
}

void
__glXDispSwap_ClipPlane(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 36);
        pc -= 4;
    }
#endif

    glClipPlane((GLenum) bswap_ENUM(pc + 32),
                (const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 4));
}

void
__glXDispSwap_ColorMaterial(GLbyte * pc)
{
    glColorMaterial((GLenum) bswap_ENUM(pc + 0), (GLenum) bswap_ENUM(pc + 4));
}

void
__glXDispSwap_CullFace(GLbyte * pc)
{
    glCullFace((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_Fogf(GLbyte * pc)
{
    glFogf((GLenum) bswap_ENUM(pc + 0), (GLfloat) bswap_FLOAT32(pc + 4));
}

void
__glXDispSwap_Fogfv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 0);
    const GLfloat *params;

    params =
        (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                         __glFogfv_size(pname));

    glFogfv(pname, params);
}

void
__glXDispSwap_Fogi(GLbyte * pc)
{
    glFogi((GLenum) bswap_ENUM(pc + 0), (GLint) bswap_CARD32(pc + 4));
}

void
__glXDispSwap_Fogiv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 0);
    const GLint *params;

    params =
        (const GLint *) bswap_32_array((uint32_t *) (pc + 4),
                                       __glFogiv_size(pname));

    glFogiv(pname, params);
}

void
__glXDispSwap_FrontFace(GLbyte * pc)
{
    glFrontFace((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_Hint(GLbyte * pc)
{
    glHint((GLenum) bswap_ENUM(pc + 0), (GLenum) bswap_ENUM(pc + 4));
}

void
__glXDispSwap_Lightf(GLbyte * pc)
{
    glLightf((GLenum) bswap_ENUM(pc + 0),
             (GLenum) bswap_ENUM(pc + 4), (GLfloat) bswap_FLOAT32(pc + 8));
}

void
__glXDispSwap_Lightfv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLfloat *params;

    params =
        (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                         __glLightfv_size(pname));

    glLightfv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_Lighti(GLbyte * pc)
{
    glLighti((GLenum) bswap_ENUM(pc + 0),
             (GLenum) bswap_ENUM(pc + 4), (GLint) bswap_CARD32(pc + 8));
}

void
__glXDispSwap_Lightiv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLint *params;

    params =
        (const GLint *) bswap_32_array((uint32_t *) (pc + 8),
                                       __glLightiv_size(pname));

    glLightiv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_LightModelf(GLbyte * pc)
{
    glLightModelf((GLenum) bswap_ENUM(pc + 0), (GLfloat) bswap_FLOAT32(pc + 4));
}

void
__glXDispSwap_LightModelfv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 0);
    const GLfloat *params;

    params =
        (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                         __glLightModelfv_size(pname));

    glLightModelfv(pname, params);
}

void
__glXDispSwap_LightModeli(GLbyte * pc)
{
    glLightModeli((GLenum) bswap_ENUM(pc + 0), (GLint) bswap_CARD32(pc + 4));
}

void
__glXDispSwap_LightModeliv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 0);
    const GLint *params;

    params =
        (const GLint *) bswap_32_array((uint32_t *) (pc + 4),
                                       __glLightModeliv_size(pname));

    glLightModeliv(pname, params);
}

void
__glXDispSwap_LineStipple(GLbyte * pc)
{
    glLineStipple((GLint) bswap_CARD32(pc + 0),
                  (GLushort) bswap_CARD16(pc + 4));
}

void
__glXDispSwap_LineWidth(GLbyte * pc)
{
    glLineWidth((GLfloat) bswap_FLOAT32(pc + 0));
}

void
__glXDispSwap_Materialf(GLbyte * pc)
{
    glMaterialf((GLenum) bswap_ENUM(pc + 0),
                (GLenum) bswap_ENUM(pc + 4), (GLfloat) bswap_FLOAT32(pc + 8));
}

void
__glXDispSwap_Materialfv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLfloat *params;

    params =
        (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                         __glMaterialfv_size(pname));

    glMaterialfv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_Materiali(GLbyte * pc)
{
    glMateriali((GLenum) bswap_ENUM(pc + 0),
                (GLenum) bswap_ENUM(pc + 4), (GLint) bswap_CARD32(pc + 8));
}

void
__glXDispSwap_Materialiv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLint *params;

    params =
        (const GLint *) bswap_32_array((uint32_t *) (pc + 8),
                                       __glMaterialiv_size(pname));

    glMaterialiv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_PointSize(GLbyte * pc)
{
    glPointSize((GLfloat) bswap_FLOAT32(pc + 0));
}

void
__glXDispSwap_PolygonMode(GLbyte * pc)
{
    glPolygonMode((GLenum) bswap_ENUM(pc + 0), (GLenum) bswap_ENUM(pc + 4));
}

void
__glXDispSwap_PolygonStipple(GLbyte * pc)
{
    const GLubyte *const mask = (const GLubyte *) ((pc + 20));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glPolygonStipple(mask);
}

void
__glXDispSwap_Scissor(GLbyte * pc)
{
    glScissor((GLint) bswap_CARD32(pc + 0),
              (GLint) bswap_CARD32(pc + 4),
              (GLsizei) bswap_CARD32(pc + 8), (GLsizei) bswap_CARD32(pc + 12));
}

void
__glXDispSwap_ShadeModel(GLbyte * pc)
{
    glShadeModel((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_TexParameterf(GLbyte * pc)
{
    glTexParameterf((GLenum) bswap_ENUM(pc + 0),
                    (GLenum) bswap_ENUM(pc + 4),
                    (GLfloat) bswap_FLOAT32(pc + 8));
}

void
__glXDispSwap_TexParameterfv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLfloat *params;

    params =
        (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                         __glTexParameterfv_size(pname));

    glTexParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_TexParameteri(GLbyte * pc)
{
    glTexParameteri((GLenum) bswap_ENUM(pc + 0),
                    (GLenum) bswap_ENUM(pc + 4), (GLint) bswap_CARD32(pc + 8));
}

void
__glXDispSwap_TexParameteriv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLint *params;

    params =
        (const GLint *) bswap_32_array((uint32_t *) (pc + 8),
                                       __glTexParameteriv_size(pname));

    glTexParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_TexImage1D(GLbyte * pc)
{
    const GLvoid *const pixels = (const GLvoid *) ((pc + 52));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glTexImage1D((GLenum) bswap_ENUM(pc + 20),
                 (GLint) bswap_CARD32(pc + 24),
                 (GLint) bswap_CARD32(pc + 28),
                 (GLsizei) bswap_CARD32(pc + 32),
                 (GLint) bswap_CARD32(pc + 40),
                 (GLenum) bswap_ENUM(pc + 44),
                 (GLenum) bswap_ENUM(pc + 48), pixels);
}

void
__glXDispSwap_TexImage2D(GLbyte * pc)
{
    const GLvoid *const pixels = (const GLvoid *) ((pc + 52));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glTexImage2D((GLenum) bswap_ENUM(pc + 20),
                 (GLint) bswap_CARD32(pc + 24),
                 (GLint) bswap_CARD32(pc + 28),
                 (GLsizei) bswap_CARD32(pc + 32),
                 (GLsizei) bswap_CARD32(pc + 36),
                 (GLint) bswap_CARD32(pc + 40),
                 (GLenum) bswap_ENUM(pc + 44),
                 (GLenum) bswap_ENUM(pc + 48), pixels);
}

void
__glXDispSwap_TexEnvf(GLbyte * pc)
{
    glTexEnvf((GLenum) bswap_ENUM(pc + 0),
              (GLenum) bswap_ENUM(pc + 4), (GLfloat) bswap_FLOAT32(pc + 8));
}

void
__glXDispSwap_TexEnvfv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLfloat *params;

    params =
        (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                         __glTexEnvfv_size(pname));

    glTexEnvfv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_TexEnvi(GLbyte * pc)
{
    glTexEnvi((GLenum) bswap_ENUM(pc + 0),
              (GLenum) bswap_ENUM(pc + 4), (GLint) bswap_CARD32(pc + 8));
}

void
__glXDispSwap_TexEnviv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLint *params;

    params =
        (const GLint *) bswap_32_array((uint32_t *) (pc + 8),
                                       __glTexEnviv_size(pname));

    glTexEnviv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_TexGend(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 16);
        pc -= 4;
    }
#endif

    glTexGend((GLenum) bswap_ENUM(pc + 8),
              (GLenum) bswap_ENUM(pc + 12), (GLdouble) bswap_FLOAT64(pc + 0));
}

void
__glXDispSwap_TexGendv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLdouble *params;

#ifdef __GLX_ALIGN64
    const GLuint compsize = __glTexGendv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 8)) - 4;

    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, cmdlen);
        pc -= 4;
    }
#endif

    params =
        (const GLdouble *) bswap_64_array((uint64_t *) (pc + 8),
                                          __glTexGendv_size(pname));

    glTexGendv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_TexGenf(GLbyte * pc)
{
    glTexGenf((GLenum) bswap_ENUM(pc + 0),
              (GLenum) bswap_ENUM(pc + 4), (GLfloat) bswap_FLOAT32(pc + 8));
}

void
__glXDispSwap_TexGenfv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLfloat *params;

    params =
        (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                         __glTexGenfv_size(pname));

    glTexGenfv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_TexGeni(GLbyte * pc)
{
    glTexGeni((GLenum) bswap_ENUM(pc + 0),
              (GLenum) bswap_ENUM(pc + 4), (GLint) bswap_CARD32(pc + 8));
}

void
__glXDispSwap_TexGeniv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLint *params;

    params =
        (const GLint *) bswap_32_array((uint32_t *) (pc + 8),
                                       __glTexGeniv_size(pname));

    glTexGeniv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_InitNames(GLbyte * pc)
{
    glInitNames();
}

void
__glXDispSwap_LoadName(GLbyte * pc)
{
    glLoadName((GLuint) bswap_CARD32(pc + 0));
}

void
__glXDispSwap_PassThrough(GLbyte * pc)
{
    glPassThrough((GLfloat) bswap_FLOAT32(pc + 0));
}

void
__glXDispSwap_PopName(GLbyte * pc)
{
    glPopName();
}

void
__glXDispSwap_PushName(GLbyte * pc)
{
    glPushName((GLuint) bswap_CARD32(pc + 0));
}

void
__glXDispSwap_DrawBuffer(GLbyte * pc)
{
    glDrawBuffer((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_Clear(GLbyte * pc)
{
    glClear((GLbitfield) bswap_CARD32(pc + 0));
}

void
__glXDispSwap_ClearAccum(GLbyte * pc)
{
    glClearAccum((GLfloat) bswap_FLOAT32(pc + 0),
                 (GLfloat) bswap_FLOAT32(pc + 4),
                 (GLfloat) bswap_FLOAT32(pc + 8),
                 (GLfloat) bswap_FLOAT32(pc + 12));
}

void
__glXDispSwap_ClearIndex(GLbyte * pc)
{
    glClearIndex((GLfloat) bswap_FLOAT32(pc + 0));
}

void
__glXDispSwap_ClearColor(GLbyte * pc)
{
    glClearColor((GLclampf) bswap_FLOAT32(pc + 0),
                 (GLclampf) bswap_FLOAT32(pc + 4),
                 (GLclampf) bswap_FLOAT32(pc + 8),
                 (GLclampf) bswap_FLOAT32(pc + 12));
}

void
__glXDispSwap_ClearStencil(GLbyte * pc)
{
    glClearStencil((GLint) bswap_CARD32(pc + 0));
}

void
__glXDispSwap_ClearDepth(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 8);
        pc -= 4;
    }
#endif

    glClearDepth((GLclampd) bswap_FLOAT64(pc + 0));
}

void
__glXDispSwap_StencilMask(GLbyte * pc)
{
    glStencilMask((GLuint) bswap_CARD32(pc + 0));
}

void
__glXDispSwap_ColorMask(GLbyte * pc)
{
    glColorMask(*(GLboolean *) (pc + 0),
                *(GLboolean *) (pc + 1),
                *(GLboolean *) (pc + 2), *(GLboolean *) (pc + 3));
}

void
__glXDispSwap_DepthMask(GLbyte * pc)
{
    glDepthMask(*(GLboolean *) (pc + 0));
}

void
__glXDispSwap_IndexMask(GLbyte * pc)
{
    glIndexMask((GLuint) bswap_CARD32(pc + 0));
}

void
__glXDispSwap_Accum(GLbyte * pc)
{
    glAccum((GLenum) bswap_ENUM(pc + 0), (GLfloat) bswap_FLOAT32(pc + 4));
}

void
__glXDispSwap_Disable(GLbyte * pc)
{
    glDisable((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_Enable(GLbyte * pc)
{
    glEnable((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_PopAttrib(GLbyte * pc)
{
    glPopAttrib();
}

void
__glXDispSwap_PushAttrib(GLbyte * pc)
{
    glPushAttrib((GLbitfield) bswap_CARD32(pc + 0));
}

void
__glXDispSwap_MapGrid1d(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 20);
        pc -= 4;
    }
#endif

    glMapGrid1d((GLint) bswap_CARD32(pc + 16),
                (GLdouble) bswap_FLOAT64(pc + 0),
                (GLdouble) bswap_FLOAT64(pc + 8));
}

void
__glXDispSwap_MapGrid1f(GLbyte * pc)
{
    glMapGrid1f((GLint) bswap_CARD32(pc + 0),
                (GLfloat) bswap_FLOAT32(pc + 4),
                (GLfloat) bswap_FLOAT32(pc + 8));
}

void
__glXDispSwap_MapGrid2d(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 40);
        pc -= 4;
    }
#endif

    glMapGrid2d((GLint) bswap_CARD32(pc + 32),
                (GLdouble) bswap_FLOAT64(pc + 0),
                (GLdouble) bswap_FLOAT64(pc + 8),
                (GLint) bswap_CARD32(pc + 36),
                (GLdouble) bswap_FLOAT64(pc + 16),
                (GLdouble) bswap_FLOAT64(pc + 24));
}

void
__glXDispSwap_MapGrid2f(GLbyte * pc)
{
    glMapGrid2f((GLint) bswap_CARD32(pc + 0),
                (GLfloat) bswap_FLOAT32(pc + 4),
                (GLfloat) bswap_FLOAT32(pc + 8),
                (GLint) bswap_CARD32(pc + 12),
                (GLfloat) bswap_FLOAT32(pc + 16),
                (GLfloat) bswap_FLOAT32(pc + 20));
}

void
__glXDispSwap_EvalCoord1dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 8);
        pc -= 4;
    }
#endif

    glEvalCoord1dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 1));
}

void
__glXDispSwap_EvalCoord1fv(GLbyte * pc)
{
    glEvalCoord1fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 1));
}

void
__glXDispSwap_EvalCoord2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 16);
        pc -= 4;
    }
#endif

    glEvalCoord2dv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 2));
}

void
__glXDispSwap_EvalCoord2fv(GLbyte * pc)
{
    glEvalCoord2fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 2));
}

void
__glXDispSwap_EvalMesh1(GLbyte * pc)
{
    glEvalMesh1((GLenum) bswap_ENUM(pc + 0),
                (GLint) bswap_CARD32(pc + 4), (GLint) bswap_CARD32(pc + 8));
}

void
__glXDispSwap_EvalPoint1(GLbyte * pc)
{
    glEvalPoint1((GLint) bswap_CARD32(pc + 0));
}

void
__glXDispSwap_EvalMesh2(GLbyte * pc)
{
    glEvalMesh2((GLenum) bswap_ENUM(pc + 0),
                (GLint) bswap_CARD32(pc + 4),
                (GLint) bswap_CARD32(pc + 8),
                (GLint) bswap_CARD32(pc + 12), (GLint) bswap_CARD32(pc + 16));
}

void
__glXDispSwap_EvalPoint2(GLbyte * pc)
{
    glEvalPoint2((GLint) bswap_CARD32(pc + 0), (GLint) bswap_CARD32(pc + 4));
}

void
__glXDispSwap_AlphaFunc(GLbyte * pc)
{
    glAlphaFunc((GLenum) bswap_ENUM(pc + 0), (GLclampf) bswap_FLOAT32(pc + 4));
}

void
__glXDispSwap_BlendFunc(GLbyte * pc)
{
    glBlendFunc((GLenum) bswap_ENUM(pc + 0), (GLenum) bswap_ENUM(pc + 4));
}

void
__glXDispSwap_LogicOp(GLbyte * pc)
{
    glLogicOp((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_StencilFunc(GLbyte * pc)
{
    glStencilFunc((GLenum) bswap_ENUM(pc + 0),
                  (GLint) bswap_CARD32(pc + 4), (GLuint) bswap_CARD32(pc + 8));
}

void
__glXDispSwap_StencilOp(GLbyte * pc)
{
    glStencilOp((GLenum) bswap_ENUM(pc + 0),
                (GLenum) bswap_ENUM(pc + 4), (GLenum) bswap_ENUM(pc + 8));
}

void
__glXDispSwap_DepthFunc(GLbyte * pc)
{
    glDepthFunc((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_PixelZoom(GLbyte * pc)
{
    glPixelZoom((GLfloat) bswap_FLOAT32(pc + 0),
                (GLfloat) bswap_FLOAT32(pc + 4));
}

void
__glXDispSwap_PixelTransferf(GLbyte * pc)
{
    glPixelTransferf((GLenum) bswap_ENUM(pc + 0),
                     (GLfloat) bswap_FLOAT32(pc + 4));
}

void
__glXDispSwap_PixelTransferi(GLbyte * pc)
{
    glPixelTransferi((GLenum) bswap_ENUM(pc + 0), (GLint) bswap_CARD32(pc + 4));
}

int
__glXDispSwap_PixelStoref(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        glPixelStoref((GLenum) bswap_ENUM(pc + 0),
                      (GLfloat) bswap_FLOAT32(pc + 4));
        error = Success;
    }

    return error;
}

int
__glXDispSwap_PixelStorei(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        glPixelStorei((GLenum) bswap_ENUM(pc + 0),
                      (GLint) bswap_CARD32(pc + 4));
        error = Success;
    }

    return error;
}

void
__glXDispSwap_PixelMapfv(GLbyte * pc)
{
    const GLsizei mapsize = (GLsizei) bswap_CARD32(pc + 4);

    glPixelMapfv((GLenum) bswap_ENUM(pc + 0),
                 mapsize,
                 (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8), 0));
}

void
__glXDispSwap_PixelMapuiv(GLbyte * pc)
{
    const GLsizei mapsize = (GLsizei) bswap_CARD32(pc + 4);

    glPixelMapuiv((GLenum) bswap_ENUM(pc + 0),
                  mapsize,
                  (const GLuint *) bswap_32_array((uint32_t *) (pc + 8), 0));
}

void
__glXDispSwap_PixelMapusv(GLbyte * pc)
{
    const GLsizei mapsize = (GLsizei) bswap_CARD32(pc + 4);

    glPixelMapusv((GLenum) bswap_ENUM(pc + 0),
                  mapsize,
                  (const GLushort *) bswap_16_array((uint16_t *) (pc + 8), 0));
}

void
__glXDispSwap_ReadBuffer(GLbyte * pc)
{
    glReadBuffer((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_CopyPixels(GLbyte * pc)
{
    glCopyPixels((GLint) bswap_CARD32(pc + 0),
                 (GLint) bswap_CARD32(pc + 4),
                 (GLsizei) bswap_CARD32(pc + 8),
                 (GLsizei) bswap_CARD32(pc + 12), (GLenum) bswap_ENUM(pc + 16));
}

void
__glXDispSwap_DrawPixels(GLbyte * pc)
{
    const GLvoid *const pixels = (const GLvoid *) ((pc + 36));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glDrawPixels((GLsizei) bswap_CARD32(pc + 20),
                 (GLsizei) bswap_CARD32(pc + 24),
                 (GLenum) bswap_ENUM(pc + 28),
                 (GLenum) bswap_ENUM(pc + 32), pixels);
}

int
__glXDispSwap_GetBooleanv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 0);

        const GLuint compsize = __glGetBooleanv_size(pname);
        GLboolean answerBuffer[200];
        GLboolean *params =
            __glXGetAnswerBuffer(cl, compsize, answerBuffer,
                                 sizeof(answerBuffer), 1);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetBooleanv(pname, params);
        __glXSendReplySwap(cl->client, params, compsize, 1, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetClipPlane(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        GLdouble equation[4];

        glGetClipPlane((GLenum) bswap_ENUM(pc + 0), equation);
        (void) bswap_64_array((uint64_t *) equation, 4);
        __glXSendReplySwap(cl->client, equation, 4, 8, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetDoublev(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 0);

        const GLuint compsize = __glGetDoublev_size(pname);
        GLdouble answerBuffer[200];
        GLdouble *params =
            __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer,
                                 sizeof(answerBuffer), 8);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetDoublev(pname, params);
        (void) bswap_64_array((uint64_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetError(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        GLenum retval;

        retval = glGetError();
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetFloatv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 0);

        const GLuint compsize = __glGetFloatv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetFloatv(pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetIntegerv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 0);

        const GLuint compsize = __glGetIntegerv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetIntegerv(pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetLightfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetLightfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetLightfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetLightiv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetLightiv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetLightiv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetMapdv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum target = (GLenum) bswap_ENUM(pc + 0);
        const GLenum query = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetMapdv_size(target, query);
        GLdouble answerBuffer[200];
        GLdouble *v =
            __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer,
                                 sizeof(answerBuffer), 8);

        if (v == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetMapdv(target, query, v);
        (void) bswap_64_array((uint64_t *) v, compsize);
        __glXSendReplySwap(cl->client, v, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetMapfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum target = (GLenum) bswap_ENUM(pc + 0);
        const GLenum query = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetMapfv_size(target, query);
        GLfloat answerBuffer[200];
        GLfloat *v =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (v == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetMapfv(target, query, v);
        (void) bswap_32_array((uint32_t *) v, compsize);
        __glXSendReplySwap(cl->client, v, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetMapiv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum target = (GLenum) bswap_ENUM(pc + 0);
        const GLenum query = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetMapiv_size(target, query);
        GLint answerBuffer[200];
        GLint *v =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (v == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetMapiv(target, query, v);
        (void) bswap_32_array((uint32_t *) v, compsize);
        __glXSendReplySwap(cl->client, v, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetMaterialfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetMaterialfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetMaterialfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetMaterialiv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetMaterialiv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetMaterialiv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetPixelMapfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum map = (GLenum) bswap_ENUM(pc + 0);

        const GLuint compsize = __glGetPixelMapfv_size(map);
        GLfloat answerBuffer[200];
        GLfloat *values =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (values == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetPixelMapfv(map, values);
        (void) bswap_32_array((uint32_t *) values, compsize);
        __glXSendReplySwap(cl->client, values, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetPixelMapuiv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum map = (GLenum) bswap_ENUM(pc + 0);

        const GLuint compsize = __glGetPixelMapuiv_size(map);
        GLuint answerBuffer[200];
        GLuint *values =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (values == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetPixelMapuiv(map, values);
        (void) bswap_32_array((uint32_t *) values, compsize);
        __glXSendReplySwap(cl->client, values, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetPixelMapusv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum map = (GLenum) bswap_ENUM(pc + 0);

        const GLuint compsize = __glGetPixelMapusv_size(map);
        GLushort answerBuffer[200];
        GLushort *values =
            __glXGetAnswerBuffer(cl, compsize * 2, answerBuffer,
                                 sizeof(answerBuffer), 2);

        if (values == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetPixelMapusv(map, values);
        (void) bswap_16_array((uint16_t *) values, compsize);
        __glXSendReplySwap(cl->client, values, compsize, 2, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetTexEnvfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetTexEnvfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetTexEnvfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetTexEnviv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetTexEnviv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetTexEnviv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetTexGendv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetTexGendv_size(pname);
        GLdouble answerBuffer[200];
        GLdouble *params =
            __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer,
                                 sizeof(answerBuffer), 8);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetTexGendv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_64_array((uint64_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetTexGenfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetTexGenfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetTexGenfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetTexGeniv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetTexGeniv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetTexGeniv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetTexParameterfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetTexParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetTexParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetTexParameteriv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetTexParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetTexParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetTexLevelParameterfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 8);

        const GLuint compsize = __glGetTexLevelParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetTexLevelParameterfv((GLenum) bswap_ENUM(pc + 0),
                                 (GLint) bswap_CARD32(pc + 4), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetTexLevelParameteriv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 8);

        const GLuint compsize = __glGetTexLevelParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetTexLevelParameteriv((GLenum) bswap_ENUM(pc + 0),
                                 (GLint) bswap_CARD32(pc + 4), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_IsEnabled(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        GLboolean retval;

        retval = glIsEnabled((GLenum) bswap_ENUM(pc + 0));
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_IsList(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        GLboolean retval;

        retval = glIsList((GLuint) bswap_CARD32(pc + 0));
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_DepthRange(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 16);
        pc -= 4;
    }
#endif

    glDepthRange((GLclampd) bswap_FLOAT64(pc + 0),
                 (GLclampd) bswap_FLOAT64(pc + 8));
}

void
__glXDispSwap_Frustum(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 48);
        pc -= 4;
    }
#endif

    glFrustum((GLdouble) bswap_FLOAT64(pc + 0),
              (GLdouble) bswap_FLOAT64(pc + 8),
              (GLdouble) bswap_FLOAT64(pc + 16),
              (GLdouble) bswap_FLOAT64(pc + 24),
              (GLdouble) bswap_FLOAT64(pc + 32),
              (GLdouble) bswap_FLOAT64(pc + 40));
}

void
__glXDispSwap_LoadIdentity(GLbyte * pc)
{
    glLoadIdentity();
}

void
__glXDispSwap_LoadMatrixf(GLbyte * pc)
{
    glLoadMatrixf((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 16));
}

void
__glXDispSwap_LoadMatrixd(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 128);
        pc -= 4;
    }
#endif

    glLoadMatrixd((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 16));
}

void
__glXDispSwap_MatrixMode(GLbyte * pc)
{
    glMatrixMode((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_MultMatrixf(GLbyte * pc)
{
    glMultMatrixf((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 16));
}

void
__glXDispSwap_MultMatrixd(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 128);
        pc -= 4;
    }
#endif

    glMultMatrixd((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 16));
}

void
__glXDispSwap_Ortho(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 48);
        pc -= 4;
    }
#endif

    glOrtho((GLdouble) bswap_FLOAT64(pc + 0),
            (GLdouble) bswap_FLOAT64(pc + 8),
            (GLdouble) bswap_FLOAT64(pc + 16),
            (GLdouble) bswap_FLOAT64(pc + 24),
            (GLdouble) bswap_FLOAT64(pc + 32),
            (GLdouble) bswap_FLOAT64(pc + 40));
}

void
__glXDispSwap_PopMatrix(GLbyte * pc)
{
    glPopMatrix();
}

void
__glXDispSwap_PushMatrix(GLbyte * pc)
{
    glPushMatrix();
}

void
__glXDispSwap_Rotated(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 32);
        pc -= 4;
    }
#endif

    glRotated((GLdouble) bswap_FLOAT64(pc + 0),
              (GLdouble) bswap_FLOAT64(pc + 8),
              (GLdouble) bswap_FLOAT64(pc + 16),
              (GLdouble) bswap_FLOAT64(pc + 24));
}

void
__glXDispSwap_Rotatef(GLbyte * pc)
{
    glRotatef((GLfloat) bswap_FLOAT32(pc + 0),
              (GLfloat) bswap_FLOAT32(pc + 4),
              (GLfloat) bswap_FLOAT32(pc + 8),
              (GLfloat) bswap_FLOAT32(pc + 12));
}

void
__glXDispSwap_Scaled(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 24);
        pc -= 4;
    }
#endif

    glScaled((GLdouble) bswap_FLOAT64(pc + 0),
             (GLdouble) bswap_FLOAT64(pc + 8),
             (GLdouble) bswap_FLOAT64(pc + 16));
}

void
__glXDispSwap_Scalef(GLbyte * pc)
{
    glScalef((GLfloat) bswap_FLOAT32(pc + 0),
             (GLfloat) bswap_FLOAT32(pc + 4), (GLfloat) bswap_FLOAT32(pc + 8));
}

void
__glXDispSwap_Translated(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 24);
        pc -= 4;
    }
#endif

    glTranslated((GLdouble) bswap_FLOAT64(pc + 0),
                 (GLdouble) bswap_FLOAT64(pc + 8),
                 (GLdouble) bswap_FLOAT64(pc + 16));
}

void
__glXDispSwap_Translatef(GLbyte * pc)
{
    glTranslatef((GLfloat) bswap_FLOAT32(pc + 0),
                 (GLfloat) bswap_FLOAT32(pc + 4),
                 (GLfloat) bswap_FLOAT32(pc + 8));
}

void
__glXDispSwap_Viewport(GLbyte * pc)
{
    glViewport((GLint) bswap_CARD32(pc + 0),
               (GLint) bswap_CARD32(pc + 4),
               (GLsizei) bswap_CARD32(pc + 8), (GLsizei) bswap_CARD32(pc + 12));
}

void
__glXDispSwap_BindTexture(GLbyte * pc)
{
    glBindTexture((GLenum) bswap_ENUM(pc + 0), (GLuint) bswap_CARD32(pc + 4));
}

void
__glXDispSwap_Indexubv(GLbyte * pc)
{
    glIndexubv((const GLubyte *) (pc + 0));
}

void
__glXDispSwap_PolygonOffset(GLbyte * pc)
{
    glPolygonOffset((GLfloat) bswap_FLOAT32(pc + 0),
                    (GLfloat) bswap_FLOAT32(pc + 4));
}

int
__glXDispSwap_AreTexturesResident(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        GLboolean retval;
        GLboolean answerBuffer[200];
        GLboolean *residences =
            __glXGetAnswerBuffer(cl, n, answerBuffer, sizeof(answerBuffer), 1);

        if (residences == NULL)
            return BadAlloc;
        retval =
            glAreTexturesResident(n,
                                  (const GLuint *)
                                  bswap_32_array((uint32_t *) (pc + 4), 0),
                                  residences);
        __glXSendReplySwap(cl->client, residences, n, 1, GL_TRUE, retval);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_AreTexturesResidentEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        GLboolean retval;
        GLboolean answerBuffer[200];
        GLboolean *residences =
            __glXGetAnswerBuffer(cl, n, answerBuffer, sizeof(answerBuffer), 1);

        if (residences == NULL)
            return BadAlloc;
        retval =
            glAreTexturesResident(n,
                                  (const GLuint *)
                                  bswap_32_array((uint32_t *) (pc + 4), 0),
                                  residences);
        __glXSendReplySwap(cl->client, residences, n, 1, GL_TRUE, retval);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_CopyTexImage1D(GLbyte * pc)
{
    glCopyTexImage1D((GLenum) bswap_ENUM(pc + 0),
                     (GLint) bswap_CARD32(pc + 4),
                     (GLenum) bswap_ENUM(pc + 8),
                     (GLint) bswap_CARD32(pc + 12),
                     (GLint) bswap_CARD32(pc + 16),
                     (GLsizei) bswap_CARD32(pc + 20),
                     (GLint) bswap_CARD32(pc + 24));
}

void
__glXDispSwap_CopyTexImage2D(GLbyte * pc)
{
    glCopyTexImage2D((GLenum) bswap_ENUM(pc + 0),
                     (GLint) bswap_CARD32(pc + 4),
                     (GLenum) bswap_ENUM(pc + 8),
                     (GLint) bswap_CARD32(pc + 12),
                     (GLint) bswap_CARD32(pc + 16),
                     (GLsizei) bswap_CARD32(pc + 20),
                     (GLsizei) bswap_CARD32(pc + 24),
                     (GLint) bswap_CARD32(pc + 28));
}

void
__glXDispSwap_CopyTexSubImage1D(GLbyte * pc)
{
    glCopyTexSubImage1D((GLenum) bswap_ENUM(pc + 0),
                        (GLint) bswap_CARD32(pc + 4),
                        (GLint) bswap_CARD32(pc + 8),
                        (GLint) bswap_CARD32(pc + 12),
                        (GLint) bswap_CARD32(pc + 16),
                        (GLsizei) bswap_CARD32(pc + 20));
}

void
__glXDispSwap_CopyTexSubImage2D(GLbyte * pc)
{
    glCopyTexSubImage2D((GLenum) bswap_ENUM(pc + 0),
                        (GLint) bswap_CARD32(pc + 4),
                        (GLint) bswap_CARD32(pc + 8),
                        (GLint) bswap_CARD32(pc + 12),
                        (GLint) bswap_CARD32(pc + 16),
                        (GLint) bswap_CARD32(pc + 20),
                        (GLsizei) bswap_CARD32(pc + 24),
                        (GLsizei) bswap_CARD32(pc + 28));
}

int
__glXDispSwap_DeleteTextures(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        glDeleteTextures(n,
                         (const GLuint *) bswap_32_array((uint32_t *) (pc + 4),
                                                         0));
        error = Success;
    }

    return error;
}

int
__glXDispSwap_DeleteTexturesEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        glDeleteTextures(n,
                         (const GLuint *) bswap_32_array((uint32_t *) (pc + 4),
                                                         0));
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GenTextures(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        GLuint answerBuffer[200];
        GLuint *textures =
            __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer),
                                 4);

        if (textures == NULL)
            return BadAlloc;
        glGenTextures(n, textures);
        (void) bswap_32_array((uint32_t *) textures, n);
        __glXSendReplySwap(cl->client, textures, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GenTexturesEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        GLuint answerBuffer[200];
        GLuint *textures =
            __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer),
                                 4);

        if (textures == NULL)
            return BadAlloc;
        glGenTextures(n, textures);
        (void) bswap_32_array((uint32_t *) textures, n);
        __glXSendReplySwap(cl->client, textures, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_IsTexture(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        GLboolean retval;

        retval = glIsTexture((GLuint) bswap_CARD32(pc + 0));
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_IsTextureEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLboolean retval;

        retval = glIsTexture((GLuint) bswap_CARD32(pc + 0));
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_PrioritizeTextures(GLbyte * pc)
{
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

    glPrioritizeTextures(n,
                         (const GLuint *) bswap_32_array((uint32_t *) (pc + 4),
                                                         0),
                         (const GLclampf *)
                         bswap_32_array((uint32_t *) (pc + 4), 0));
}

void
__glXDispSwap_TexSubImage1D(GLbyte * pc)
{
    const GLvoid *const pixels = (const GLvoid *) ((pc + 56));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glTexSubImage1D((GLenum) bswap_ENUM(pc + 20),
                    (GLint) bswap_CARD32(pc + 24),
                    (GLint) bswap_CARD32(pc + 28),
                    (GLsizei) bswap_CARD32(pc + 36),
                    (GLenum) bswap_ENUM(pc + 44),
                    (GLenum) bswap_ENUM(pc + 48), pixels);
}

void
__glXDispSwap_TexSubImage2D(GLbyte * pc)
{
    const GLvoid *const pixels = (const GLvoid *) ((pc + 56));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glTexSubImage2D((GLenum) bswap_ENUM(pc + 20),
                    (GLint) bswap_CARD32(pc + 24),
                    (GLint) bswap_CARD32(pc + 28),
                    (GLint) bswap_CARD32(pc + 32),
                    (GLsizei) bswap_CARD32(pc + 36),
                    (GLsizei) bswap_CARD32(pc + 40),
                    (GLenum) bswap_ENUM(pc + 44),
                    (GLenum) bswap_ENUM(pc + 48), pixels);
}

void
__glXDispSwap_BlendColor(GLbyte * pc)
{
    glBlendColor((GLclampf) bswap_FLOAT32(pc + 0),
                 (GLclampf) bswap_FLOAT32(pc + 4),
                 (GLclampf) bswap_FLOAT32(pc + 8),
                 (GLclampf) bswap_FLOAT32(pc + 12));
}

void
__glXDispSwap_BlendEquation(GLbyte * pc)
{
    glBlendEquation((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_ColorTable(GLbyte * pc)
{
    const GLvoid *const table = (const GLvoid *) ((pc + 40));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glColorTable((GLenum) bswap_ENUM(pc + 20),
                 (GLenum) bswap_ENUM(pc + 24),
                 (GLsizei) bswap_CARD32(pc + 28),
                 (GLenum) bswap_ENUM(pc + 32),
                 (GLenum) bswap_ENUM(pc + 36), table);
}

void
__glXDispSwap_ColorTableParameterfv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLfloat *params;

    params =
        (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                         __glColorTableParameterfv_size(pname));

    glColorTableParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_ColorTableParameteriv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLint *params;

    params =
        (const GLint *) bswap_32_array((uint32_t *) (pc + 8),
                                       __glColorTableParameteriv_size(pname));

    glColorTableParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_CopyColorTable(GLbyte * pc)
{
    glCopyColorTable((GLenum) bswap_ENUM(pc + 0),
                     (GLenum) bswap_ENUM(pc + 4),
                     (GLint) bswap_CARD32(pc + 8),
                     (GLint) bswap_CARD32(pc + 12),
                     (GLsizei) bswap_CARD32(pc + 16));
}

int
__glXDispSwap_GetColorTableParameterfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetColorTableParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetColorTableParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetColorTableParameterfvSGI(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetColorTableParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetColorTableParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetColorTableParameteriv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetColorTableParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetColorTableParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetColorTableParameterivSGI(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetColorTableParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetColorTableParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_ColorSubTable(GLbyte * pc)
{
    const GLvoid *const data = (const GLvoid *) ((pc + 40));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glColorSubTable((GLenum) bswap_ENUM(pc + 20),
                    (GLsizei) bswap_CARD32(pc + 24),
                    (GLsizei) bswap_CARD32(pc + 28),
                    (GLenum) bswap_ENUM(pc + 32),
                    (GLenum) bswap_ENUM(pc + 36), data);
}

void
__glXDispSwap_CopyColorSubTable(GLbyte * pc)
{
    glCopyColorSubTable((GLenum) bswap_ENUM(pc + 0),
                        (GLsizei) bswap_CARD32(pc + 4),
                        (GLint) bswap_CARD32(pc + 8),
                        (GLint) bswap_CARD32(pc + 12),
                        (GLsizei) bswap_CARD32(pc + 16));
}

void
__glXDispSwap_ConvolutionFilter1D(GLbyte * pc)
{
    const GLvoid *const image = (const GLvoid *) ((pc + 44));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glConvolutionFilter1D((GLenum) bswap_ENUM(pc + 20),
                          (GLenum) bswap_ENUM(pc + 24),
                          (GLsizei) bswap_CARD32(pc + 28),
                          (GLenum) bswap_ENUM(pc + 36),
                          (GLenum) bswap_ENUM(pc + 40), image);
}

void
__glXDispSwap_ConvolutionFilter2D(GLbyte * pc)
{
    const GLvoid *const image = (const GLvoid *) ((pc + 44));
    __GLXpixelHeader *const hdr = (__GLXpixelHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glConvolutionFilter2D((GLenum) bswap_ENUM(pc + 20),
                          (GLenum) bswap_ENUM(pc + 24),
                          (GLsizei) bswap_CARD32(pc + 28),
                          (GLsizei) bswap_CARD32(pc + 32),
                          (GLenum) bswap_ENUM(pc + 36),
                          (GLenum) bswap_ENUM(pc + 40), image);
}

void
__glXDispSwap_ConvolutionParameterf(GLbyte * pc)
{
    glConvolutionParameterf((GLenum) bswap_ENUM(pc + 0),
                            (GLenum) bswap_ENUM(pc + 4),
                            (GLfloat) bswap_FLOAT32(pc + 8));
}

void
__glXDispSwap_ConvolutionParameterfv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLfloat *params;

    params =
        (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                         __glConvolutionParameterfv_size
                                         (pname));

    glConvolutionParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_ConvolutionParameteri(GLbyte * pc)
{
    glConvolutionParameteri((GLenum) bswap_ENUM(pc + 0),
                            (GLenum) bswap_ENUM(pc + 4),
                            (GLint) bswap_CARD32(pc + 8));
}

void
__glXDispSwap_ConvolutionParameteriv(GLbyte * pc)
{
    const GLenum pname = (GLenum) bswap_ENUM(pc + 4);
    const GLint *params;

    params =
        (const GLint *) bswap_32_array((uint32_t *) (pc + 8),
                                       __glConvolutionParameteriv_size(pname));

    glConvolutionParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
}

void
__glXDispSwap_CopyConvolutionFilter1D(GLbyte * pc)
{
    glCopyConvolutionFilter1D((GLenum) bswap_ENUM(pc + 0),
                              (GLenum) bswap_ENUM(pc + 4),
                              (GLint) bswap_CARD32(pc + 8),
                              (GLint) bswap_CARD32(pc + 12),
                              (GLsizei) bswap_CARD32(pc + 16));
}

void
__glXDispSwap_CopyConvolutionFilter2D(GLbyte * pc)
{
    glCopyConvolutionFilter2D((GLenum) bswap_ENUM(pc + 0),
                              (GLenum) bswap_ENUM(pc + 4),
                              (GLint) bswap_CARD32(pc + 8),
                              (GLint) bswap_CARD32(pc + 12),
                              (GLsizei) bswap_CARD32(pc + 16),
                              (GLsizei) bswap_CARD32(pc + 20));
}

int
__glXDispSwap_GetConvolutionParameterfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetConvolutionParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetConvolutionParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetConvolutionParameterfvEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetConvolutionParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetConvolutionParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetConvolutionParameteriv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetConvolutionParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetConvolutionParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetConvolutionParameterivEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetConvolutionParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetConvolutionParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetHistogramParameterfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetHistogramParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetHistogramParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetHistogramParameterfvEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetHistogramParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetHistogramParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetHistogramParameteriv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetHistogramParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetHistogramParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetHistogramParameterivEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetHistogramParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetHistogramParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetMinmaxParameterfv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetMinmaxParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetMinmaxParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetMinmaxParameterfvEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetMinmaxParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetMinmaxParameterfv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetMinmaxParameteriv(__GLXclientState * cl, GLbyte * pc)
{
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetMinmaxParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetMinmaxParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetMinmaxParameterivEXT(__GLXclientState * cl, GLbyte * pc)
{
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetMinmaxParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        glGetMinmaxParameteriv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_Histogram(GLbyte * pc)
{
    glHistogram((GLenum) bswap_ENUM(pc + 0),
                (GLsizei) bswap_CARD32(pc + 4),
                (GLenum) bswap_ENUM(pc + 8), *(GLboolean *) (pc + 12));
}

void
__glXDispSwap_Minmax(GLbyte * pc)
{
    glMinmax((GLenum) bswap_ENUM(pc + 0),
             (GLenum) bswap_ENUM(pc + 4), *(GLboolean *) (pc + 8));
}

void
__glXDispSwap_ResetHistogram(GLbyte * pc)
{
    glResetHistogram((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_ResetMinmax(GLbyte * pc)
{
    glResetMinmax((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_TexImage3D(GLbyte * pc)
{
    const CARD32 ptr_is_null = *(CARD32 *) (pc + 76);
    const GLvoid *const pixels =
        (const GLvoid *) ((ptr_is_null != 0) ? NULL : (pc + 80));
    __GLXpixel3DHeader *const hdr = (__GLXpixel3DHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT,
                  (GLint) bswap_CARD32(&hdr->imageHeight));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_IMAGES,
                  (GLint) bswap_CARD32(&hdr->skipImages));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glTexImage3D((GLenum) bswap_ENUM(pc + 36),
                 (GLint) bswap_CARD32(pc + 40),
                 (GLint) bswap_CARD32(pc + 44),
                 (GLsizei) bswap_CARD32(pc + 48),
                 (GLsizei) bswap_CARD32(pc + 52),
                 (GLsizei) bswap_CARD32(pc + 56),
                 (GLint) bswap_CARD32(pc + 64),
                 (GLenum) bswap_ENUM(pc + 68),
                 (GLenum) bswap_ENUM(pc + 72), pixels);
}

void
__glXDispSwap_TexSubImage3D(GLbyte * pc)
{
    const GLvoid *const pixels = (const GLvoid *) ((pc + 88));
    __GLXpixel3DHeader *const hdr = (__GLXpixel3DHeader *) (pc);

    glPixelStorei(GL_UNPACK_SWAP_BYTES, hdr->swapBytes);
    glPixelStorei(GL_UNPACK_LSB_FIRST, hdr->lsbFirst);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, (GLint) bswap_CARD32(&hdr->rowLength));
    glPixelStorei(GL_UNPACK_IMAGE_HEIGHT,
                  (GLint) bswap_CARD32(&hdr->imageHeight));
    glPixelStorei(GL_UNPACK_SKIP_ROWS, (GLint) bswap_CARD32(&hdr->skipRows));
    glPixelStorei(GL_UNPACK_SKIP_IMAGES,
                  (GLint) bswap_CARD32(&hdr->skipImages));
    glPixelStorei(GL_UNPACK_SKIP_PIXELS,
                  (GLint) bswap_CARD32(&hdr->skipPixels));
    glPixelStorei(GL_UNPACK_ALIGNMENT, (GLint) bswap_CARD32(&hdr->alignment));

    glTexSubImage3D((GLenum) bswap_ENUM(pc + 36),
                    (GLint) bswap_CARD32(pc + 40),
                    (GLint) bswap_CARD32(pc + 44),
                    (GLint) bswap_CARD32(pc + 48),
                    (GLint) bswap_CARD32(pc + 52),
                    (GLsizei) bswap_CARD32(pc + 60),
                    (GLsizei) bswap_CARD32(pc + 64),
                    (GLsizei) bswap_CARD32(pc + 68),
                    (GLenum) bswap_ENUM(pc + 76),
                    (GLenum) bswap_ENUM(pc + 80), pixels);
}

void
__glXDispSwap_CopyTexSubImage3D(GLbyte * pc)
{
    glCopyTexSubImage3D((GLenum) bswap_ENUM(pc + 0),
                        (GLint) bswap_CARD32(pc + 4),
                        (GLint) bswap_CARD32(pc + 8),
                        (GLint) bswap_CARD32(pc + 12),
                        (GLint) bswap_CARD32(pc + 16),
                        (GLint) bswap_CARD32(pc + 20),
                        (GLint) bswap_CARD32(pc + 24),
                        (GLsizei) bswap_CARD32(pc + 28),
                        (GLsizei) bswap_CARD32(pc + 32));
}

void
__glXDispSwap_ActiveTexture(GLbyte * pc)
{
    glActiveTextureARB((GLenum) bswap_ENUM(pc + 0));
}

void
__glXDispSwap_MultiTexCoord1dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 12);
        pc -= 4;
    }
#endif

    glMultiTexCoord1dvARB((GLenum) bswap_ENUM(pc + 8),
                          (const GLdouble *) bswap_64_array((uint64_t *) (pc + 0),
                                                         1));
}

void
__glXDispSwap_MultiTexCoord1fvARB(GLbyte * pc)
{
    glMultiTexCoord1fvARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLfloat *)
                          bswap_32_array((uint32_t *) (pc + 4), 1));
}

void
__glXDispSwap_MultiTexCoord1iv(GLbyte * pc)
{
    glMultiTexCoord1ivARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLint *) bswap_32_array((uint32_t *) (pc + 4),
                                                         1));
}

void
__glXDispSwap_MultiTexCoord1sv(GLbyte * pc)
{
    glMultiTexCoord1svARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLshort *) bswap_16_array((uint16_t *) (pc + 4),
                                                           1));
}

void
__glXDispSwap_MultiTexCoord2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 20);
        pc -= 4;
    }
#endif

    glMultiTexCoord2dvARB((GLenum) bswap_ENUM(pc + 16),
                          (const GLdouble *) bswap_64_array((uint64_t *) (pc + 0),
                                                            2));
}

void
__glXDispSwap_MultiTexCoord2fvARB(GLbyte * pc)
{
    glMultiTexCoord2fvARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLfloat *)
                          bswap_32_array((uint32_t *) (pc + 4), 2));
}

void
__glXDispSwap_MultiTexCoord2iv(GLbyte * pc)
{
    glMultiTexCoord2ivARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLint *) bswap_32_array((uint32_t *) (pc + 4),
                                                         2));
}

void
__glXDispSwap_MultiTexCoord2sv(GLbyte * pc)
{
    glMultiTexCoord2svARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLshort *) bswap_16_array((uint16_t *) (pc + 4),
                                                           2));
}

void
__glXDispSwap_MultiTexCoord3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 28);
        pc -= 4;
    }
#endif

    glMultiTexCoord3dvARB((GLenum) bswap_ENUM(pc + 24),
                          (const GLdouble *) bswap_64_array((uint64_t *) (pc + 0),
                                                            3));
}

void
__glXDispSwap_MultiTexCoord3fvARB(GLbyte * pc)
{
    glMultiTexCoord3fvARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLfloat *)
                          bswap_32_array((uint32_t *) (pc + 4), 3));
}

void
__glXDispSwap_MultiTexCoord3iv(GLbyte * pc)
{
    glMultiTexCoord3ivARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLint *) bswap_32_array((uint32_t *) (pc + 4),
                                                         3));
}

void
__glXDispSwap_MultiTexCoord3sv(GLbyte * pc)
{
    glMultiTexCoord3svARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLshort *) bswap_16_array((uint16_t *) (pc + 4),
                                                           3));
}

void
__glXDispSwap_MultiTexCoord4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 36);
        pc -= 4;
    }
#endif

    glMultiTexCoord4dvARB((GLenum) bswap_ENUM(pc + 32),
                          (const GLdouble *) bswap_64_array((uint64_t *) (pc + 0),
                                                            4));
}

void
__glXDispSwap_MultiTexCoord4fvARB(GLbyte * pc)
{
    glMultiTexCoord4fvARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLfloat *)
                          bswap_32_array((uint32_t *) (pc + 4), 4));
}

void
__glXDispSwap_MultiTexCoord4iv(GLbyte * pc)
{
    glMultiTexCoord4ivARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLint *) bswap_32_array((uint32_t *) (pc + 4),
                                                         4));
}

void
__glXDispSwap_MultiTexCoord4sv(GLbyte * pc)
{
    glMultiTexCoord4svARB((GLenum) bswap_ENUM(pc + 0),
                          (const GLshort *) bswap_16_array((uint16_t *) (pc + 4),
                                                           4));
}

void
__glXDispSwap_CompressedTexImage1D(GLbyte * pc)
{
    PFNGLCOMPRESSEDTEXIMAGE1DPROC CompressedTexImage1D =
        __glGetProcAddress("glCompressedTexImage1D");
    const GLsizei imageSize = (GLsizei) bswap_CARD32(pc + 20);

    CompressedTexImage1D((GLenum) bswap_ENUM(pc + 0),
                         (GLint) bswap_CARD32(pc + 4),
                         (GLenum) bswap_ENUM(pc + 8),
                         (GLsizei) bswap_CARD32(pc + 12),
                         (GLint) bswap_CARD32(pc + 16),
                         imageSize, (const GLvoid *) (pc + 24));
}

void
__glXDispSwap_CompressedTexImage2D(GLbyte * pc)
{
    PFNGLCOMPRESSEDTEXIMAGE2DPROC CompressedTexImage2D =
        __glGetProcAddress("glCompressedTexImage2D");
    const GLsizei imageSize = (GLsizei) bswap_CARD32(pc + 24);

    CompressedTexImage2D((GLenum) bswap_ENUM(pc + 0),
                         (GLint) bswap_CARD32(pc + 4),
                         (GLenum) bswap_ENUM(pc + 8),
                         (GLsizei) bswap_CARD32(pc + 12),
                         (GLsizei) bswap_CARD32(pc + 16),
                         (GLint) bswap_CARD32(pc + 20),
                         imageSize, (const GLvoid *) (pc + 28));
}

void
__glXDispSwap_CompressedTexImage3D(GLbyte * pc)
{
    PFNGLCOMPRESSEDTEXIMAGE3DPROC CompressedTexImage3D =
        __glGetProcAddress("glCompressedTexImage3D");
    const GLsizei imageSize = (GLsizei) bswap_CARD32(pc + 28);

    CompressedTexImage3D((GLenum) bswap_ENUM(pc + 0),
                         (GLint) bswap_CARD32(pc + 4),
                         (GLenum) bswap_ENUM(pc + 8),
                         (GLsizei) bswap_CARD32(pc + 12),
                         (GLsizei) bswap_CARD32(pc + 16),
                         (GLsizei) bswap_CARD32(pc + 20),
                         (GLint) bswap_CARD32(pc + 24),
                         imageSize, (const GLvoid *) (pc + 32));
}

void
__glXDispSwap_CompressedTexSubImage1D(GLbyte * pc)
{
    PFNGLCOMPRESSEDTEXSUBIMAGE1DPROC CompressedTexSubImage1D =
        __glGetProcAddress("glCompressedTexSubImage1D");
    const GLsizei imageSize = (GLsizei) bswap_CARD32(pc + 20);

    CompressedTexSubImage1D((GLenum) bswap_ENUM(pc + 0),
                            (GLint) bswap_CARD32(pc + 4),
                            (GLint) bswap_CARD32(pc + 8),
                            (GLsizei) bswap_CARD32(pc + 12),
                            (GLenum) bswap_ENUM(pc + 16),
                            imageSize, (const GLvoid *) (pc + 24));
}

void
__glXDispSwap_CompressedTexSubImage2D(GLbyte * pc)
{
    PFNGLCOMPRESSEDTEXSUBIMAGE2DPROC CompressedTexSubImage2D =
        __glGetProcAddress("glCompressedTexSubImage2D");
    const GLsizei imageSize = (GLsizei) bswap_CARD32(pc + 28);

    CompressedTexSubImage2D((GLenum) bswap_ENUM(pc + 0),
                            (GLint) bswap_CARD32(pc + 4),
                            (GLint) bswap_CARD32(pc + 8),
                            (GLint) bswap_CARD32(pc + 12),
                            (GLsizei) bswap_CARD32(pc + 16),
                            (GLsizei) bswap_CARD32(pc + 20),
                            (GLenum) bswap_ENUM(pc + 24),
                            imageSize, (const GLvoid *) (pc + 32));
}

void
__glXDispSwap_CompressedTexSubImage3D(GLbyte * pc)
{
    PFNGLCOMPRESSEDTEXSUBIMAGE3DPROC CompressedTexSubImage3D =
        __glGetProcAddress("glCompressedTexSubImage3D");
    const GLsizei imageSize = (GLsizei) bswap_CARD32(pc + 36);

    CompressedTexSubImage3D((GLenum) bswap_ENUM(pc + 0),
                            (GLint) bswap_CARD32(pc + 4),
                            (GLint) bswap_CARD32(pc + 8),
                            (GLint) bswap_CARD32(pc + 12),
                            (GLint) bswap_CARD32(pc + 16),
                            (GLsizei) bswap_CARD32(pc + 20),
                            (GLsizei) bswap_CARD32(pc + 24),
                            (GLsizei) bswap_CARD32(pc + 28),
                            (GLenum) bswap_ENUM(pc + 32),
                            imageSize, (const GLvoid *) (pc + 40));
}

void
__glXDispSwap_SampleCoverage(GLbyte * pc)
{
    PFNGLSAMPLECOVERAGEPROC SampleCoverage =
        __glGetProcAddress("glSampleCoverage");
    SampleCoverage((GLclampf) bswap_FLOAT32(pc + 0), *(GLboolean *) (pc + 4));
}

void
__glXDispSwap_BlendFuncSeparate(GLbyte * pc)
{
    PFNGLBLENDFUNCSEPARATEPROC BlendFuncSeparate =
        __glGetProcAddress("glBlendFuncSeparate");
    BlendFuncSeparate((GLenum) bswap_ENUM(pc + 0), (GLenum) bswap_ENUM(pc + 4),
                      (GLenum) bswap_ENUM(pc + 8),
                      (GLenum) bswap_ENUM(pc + 12));
}

void
__glXDispSwap_FogCoorddv(GLbyte * pc)
{
    PFNGLFOGCOORDDVPROC FogCoorddv = __glGetProcAddress("glFogCoorddv");

#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 8);
        pc -= 4;
    }
#endif

    FogCoorddv((const GLdouble *) bswap_64_array((uint64_t *) (pc + 0), 1));
}

void
__glXDispSwap_PointParameterf(GLbyte * pc)
{
    PFNGLPOINTPARAMETERFPROC PointParameterf =
        __glGetProcAddress("glPointParameterf");
    PointParameterf((GLenum) bswap_ENUM(pc + 0),
                    (GLfloat) bswap_FLOAT32(pc + 4));
}

void
__glXDispSwap_PointParameterfv(GLbyte * pc)
{
    PFNGLPOINTPARAMETERFVPROC PointParameterfv =
        __glGetProcAddress("glPointParameterfv");
    const GLenum pname = (GLenum) bswap_ENUM(pc + 0);
    const GLfloat *params;

    params =
        (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                         __glPointParameterfv_size(pname));

    PointParameterfv(pname, params);
}

void
__glXDispSwap_PointParameteri(GLbyte * pc)
{
    PFNGLPOINTPARAMETERIPROC PointParameteri =
        __glGetProcAddress("glPointParameteri");
    PointParameteri((GLenum) bswap_ENUM(pc + 0), (GLint) bswap_CARD32(pc + 4));
}

void
__glXDispSwap_PointParameteriv(GLbyte * pc)
{
    PFNGLPOINTPARAMETERIVPROC PointParameteriv =
        __glGetProcAddress("glPointParameteriv");
    const GLenum pname = (GLenum) bswap_ENUM(pc + 0);
    const GLint *params;

    params =
        (const GLint *) bswap_32_array((uint32_t *) (pc + 4),
                                       __glPointParameteriv_size(pname));

    PointParameteriv(pname, params);
}

void
__glXDispSwap_SecondaryColor3bv(GLbyte * pc)
{
    PFNGLSECONDARYCOLOR3BVPROC SecondaryColor3bv =
        __glGetProcAddress("glSecondaryColor3bv");
    SecondaryColor3bv((const GLbyte *) (pc + 0));
}

void
__glXDispSwap_SecondaryColor3dv(GLbyte * pc)
{
    PFNGLSECONDARYCOLOR3DVPROC SecondaryColor3dv =
        __glGetProcAddress("glSecondaryColor3dv");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 24);
        pc -= 4;
    }
#endif

    SecondaryColor3dv((const GLdouble *)
                      bswap_64_array((uint64_t *) (pc + 0), 3));
}

void
__glXDispSwap_SecondaryColor3iv(GLbyte * pc)
{
    PFNGLSECONDARYCOLOR3IVPROC SecondaryColor3iv =
        __glGetProcAddress("glSecondaryColor3iv");
    SecondaryColor3iv((const GLint *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_SecondaryColor3sv(GLbyte * pc)
{
    PFNGLSECONDARYCOLOR3SVPROC SecondaryColor3sv =
        __glGetProcAddress("glSecondaryColor3sv");
    SecondaryColor3sv((const GLshort *)
                      bswap_16_array((uint16_t *) (pc + 0), 3));
}

void
__glXDispSwap_SecondaryColor3ubv(GLbyte * pc)
{
    PFNGLSECONDARYCOLOR3UBVPROC SecondaryColor3ubv =
        __glGetProcAddress("glSecondaryColor3ubv");
    SecondaryColor3ubv((const GLubyte *) (pc + 0));
}

void
__glXDispSwap_SecondaryColor3uiv(GLbyte * pc)
{
    PFNGLSECONDARYCOLOR3UIVPROC SecondaryColor3uiv =
        __glGetProcAddress("glSecondaryColor3uiv");
    SecondaryColor3uiv((const GLuint *)
                       bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_SecondaryColor3usv(GLbyte * pc)
{
    PFNGLSECONDARYCOLOR3USVPROC SecondaryColor3usv =
        __glGetProcAddress("glSecondaryColor3usv");
    SecondaryColor3usv((const GLushort *)
                       bswap_16_array((uint16_t *) (pc + 0), 3));
}

void
__glXDispSwap_WindowPos3fv(GLbyte * pc)
{
    PFNGLWINDOWPOS3FVPROC WindowPos3fv = __glGetProcAddress("glWindowPos3fv");

    WindowPos3fv((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_BeginQuery(GLbyte * pc)
{
    PFNGLBEGINQUERYPROC BeginQuery = __glGetProcAddress("glBeginQuery");

    BeginQuery((GLenum) bswap_ENUM(pc + 0), (GLuint) bswap_CARD32(pc + 4));
}

int
__glXDispSwap_DeleteQueries(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLDELETEQUERIESPROC DeleteQueries =
        __glGetProcAddress("glDeleteQueries");
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        DeleteQueries(n,
                      (const GLuint *) bswap_32_array((uint32_t *) (pc + 4),
                                                      0));
        error = Success;
    }

    return error;
}

void
__glXDispSwap_EndQuery(GLbyte * pc)
{
    PFNGLENDQUERYPROC EndQuery = __glGetProcAddress("glEndQuery");

    EndQuery((GLenum) bswap_ENUM(pc + 0));
}

int
__glXDispSwap_GenQueries(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGENQUERIESPROC GenQueries = __glGetProcAddress("glGenQueries");
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        GLuint answerBuffer[200];
        GLuint *ids =
            __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer),
                                 4);
        if (ids == NULL)
            return BadAlloc;

        GenQueries(n, ids);
        (void) bswap_32_array((uint32_t *) ids, n);
        __glXSendReplySwap(cl->client, ids, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetQueryObjectiv(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGETQUERYOBJECTIVPROC GetQueryObjectiv =
        __glGetProcAddress("glGetQueryObjectiv");
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetQueryObjectiv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        GetQueryObjectiv((GLuint) bswap_CARD32(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetQueryObjectuiv(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGETQUERYOBJECTUIVPROC GetQueryObjectuiv =
        __glGetProcAddress("glGetQueryObjectuiv");
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetQueryObjectuiv_size(pname);
        GLuint answerBuffer[200];
        GLuint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        GetQueryObjectuiv((GLuint) bswap_CARD32(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetQueryiv(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGETQUERYIVPROC GetQueryiv = __glGetProcAddress("glGetQueryiv");
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetQueryiv_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        GetQueryiv((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_IsQuery(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLISQUERYPROC IsQuery = __glGetProcAddress("glIsQuery");
    xGLXSingleReq *const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if (cx != NULL) {
        GLboolean retval;

        retval = IsQuery((GLuint) bswap_CARD32(pc + 0));
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_BlendEquationSeparate(GLbyte * pc)
{
    PFNGLBLENDEQUATIONSEPARATEPROC BlendEquationSeparate =
        __glGetProcAddress("glBlendEquationSeparate");
    BlendEquationSeparate((GLenum) bswap_ENUM(pc + 0),
                          (GLenum) bswap_ENUM(pc + 4));
}

void
__glXDispSwap_DrawBuffers(GLbyte * pc)
{
    PFNGLDRAWBUFFERSPROC DrawBuffers = __glGetProcAddress("glDrawBuffers");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

    DrawBuffers(n, (const GLenum *) bswap_32_array((uint32_t *) (pc + 4), 0));
}

void
__glXDispSwap_VertexAttrib1dv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB1DVPROC VertexAttrib1dv =
        __glGetProcAddress("glVertexAttrib1dv");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 12);
        pc -= 4;
    }
#endif

    VertexAttrib1dv((GLuint) bswap_CARD32(pc + 0),
                    (const GLdouble *) bswap_64_array((uint64_t *) (pc + 4),
                                                      1));
}

void
__glXDispSwap_VertexAttrib1sv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB1SVPROC VertexAttrib1sv =
        __glGetProcAddress("glVertexAttrib1sv");
    VertexAttrib1sv((GLuint) bswap_CARD32(pc + 0),
                    (const GLshort *) bswap_16_array((uint16_t *) (pc + 4), 1));
}

void
__glXDispSwap_VertexAttrib2dv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB2DVPROC VertexAttrib2dv =
        __glGetProcAddress("glVertexAttrib2dv");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 20);
        pc -= 4;
    }
#endif

    VertexAttrib2dv((GLuint) bswap_CARD32(pc + 0),
                    (const GLdouble *) bswap_64_array((uint64_t *) (pc + 4),
                                                      2));
}

void
__glXDispSwap_VertexAttrib2sv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB2SVPROC VertexAttrib2sv =
        __glGetProcAddress("glVertexAttrib2sv");
    VertexAttrib2sv((GLuint) bswap_CARD32(pc + 0),
                    (const GLshort *) bswap_16_array((uint16_t *) (pc + 4), 2));
}

void
__glXDispSwap_VertexAttrib3dv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB3DVPROC VertexAttrib3dv =
        __glGetProcAddress("glVertexAttrib3dv");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 28);
        pc -= 4;
    }
#endif

    VertexAttrib3dv((GLuint) bswap_CARD32(pc + 0),
                    (const GLdouble *) bswap_64_array((uint64_t *) (pc + 4),
                                                      3));
}

void
__glXDispSwap_VertexAttrib3sv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB3SVPROC VertexAttrib3sv =
        __glGetProcAddress("glVertexAttrib3sv");
    VertexAttrib3sv((GLuint) bswap_CARD32(pc + 0),
                    (const GLshort *) bswap_16_array((uint16_t *) (pc + 4), 3));
}

void
__glXDispSwap_VertexAttrib4Nbv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4NBVPROC VertexAttrib4Nbv =
        __glGetProcAddress("glVertexAttrib4Nbv");
    VertexAttrib4Nbv((GLuint) bswap_CARD32(pc + 0), (const GLbyte *) (pc + 4));
}

void
__glXDispSwap_VertexAttrib4Niv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4NIVPROC VertexAttrib4Niv =
        __glGetProcAddress("glVertexAttrib4Niv");
    VertexAttrib4Niv((GLuint) bswap_CARD32(pc + 0),
                     (const GLint *) bswap_32_array((uint32_t *) (pc + 4), 4));
}

void
__glXDispSwap_VertexAttrib4Nsv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4NSVPROC VertexAttrib4Nsv =
        __glGetProcAddress("glVertexAttrib4Nsv");
    VertexAttrib4Nsv((GLuint) bswap_CARD32(pc + 0),
                     (const GLshort *) bswap_16_array((uint16_t *) (pc + 4),
                                                      4));
}

void
__glXDispSwap_VertexAttrib4Nubv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4NUBVPROC VertexAttrib4Nubv =
        __glGetProcAddress("glVertexAttrib4Nubv");
    VertexAttrib4Nubv((GLuint) bswap_CARD32(pc + 0),
                      (const GLubyte *) (pc + 4));
}

void
__glXDispSwap_VertexAttrib4Nuiv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4NUIVPROC VertexAttrib4Nuiv =
        __glGetProcAddress("glVertexAttrib4Nuiv");
    VertexAttrib4Nuiv((GLuint) bswap_CARD32(pc + 0),
                      (const GLuint *) bswap_32_array((uint32_t *) (pc + 4),
                                                      4));
}

void
__glXDispSwap_VertexAttrib4Nusv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4NUSVPROC VertexAttrib4Nusv =
        __glGetProcAddress("glVertexAttrib4Nusv");
    VertexAttrib4Nusv((GLuint) bswap_CARD32(pc + 0),
                      (const GLushort *) bswap_16_array((uint16_t *) (pc + 4),
                                                        4));
}

void
__glXDispSwap_VertexAttrib4bv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4BVPROC VertexAttrib4bv =
        __glGetProcAddress("glVertexAttrib4bv");
    VertexAttrib4bv((GLuint) bswap_CARD32(pc + 0), (const GLbyte *) (pc + 4));
}

void
__glXDispSwap_VertexAttrib4dv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4DVPROC VertexAttrib4dv =
        __glGetProcAddress("glVertexAttrib4dv");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 36);
        pc -= 4;
    }
#endif

    VertexAttrib4dv((GLuint) bswap_CARD32(pc + 0),
                    (const GLdouble *) bswap_64_array((uint64_t *) (pc + 4),
                                                      4));
}

void
__glXDispSwap_VertexAttrib4iv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4IVPROC VertexAttrib4iv =
        __glGetProcAddress("glVertexAttrib4iv");
    VertexAttrib4iv((GLuint) bswap_CARD32(pc + 0),
                    (const GLint *) bswap_32_array((uint32_t *) (pc + 4), 4));
}

void
__glXDispSwap_VertexAttrib4sv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4SVPROC VertexAttrib4sv =
        __glGetProcAddress("glVertexAttrib4sv");
    VertexAttrib4sv((GLuint) bswap_CARD32(pc + 0),
                    (const GLshort *) bswap_16_array((uint16_t *) (pc + 4), 4));
}

void
__glXDispSwap_VertexAttrib4ubv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4UBVPROC VertexAttrib4ubv =
        __glGetProcAddress("glVertexAttrib4ubv");
    VertexAttrib4ubv((GLuint) bswap_CARD32(pc + 0), (const GLubyte *) (pc + 4));
}

void
__glXDispSwap_VertexAttrib4uiv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4UIVPROC VertexAttrib4uiv =
        __glGetProcAddress("glVertexAttrib4uiv");
    VertexAttrib4uiv((GLuint) bswap_CARD32(pc + 0),
                     (const GLuint *) bswap_32_array((uint32_t *) (pc + 4), 4));
}

void
__glXDispSwap_VertexAttrib4usv(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4USVPROC VertexAttrib4usv =
        __glGetProcAddress("glVertexAttrib4usv");
    VertexAttrib4usv((GLuint) bswap_CARD32(pc + 0),
                     (const GLushort *) bswap_16_array((uint16_t *) (pc + 4),
                                                       4));
}

void
__glXDispSwap_ClampColor(GLbyte * pc)
{
    PFNGLCLAMPCOLORPROC ClampColor = __glGetProcAddress("glClampColor");

    ClampColor((GLenum) bswap_ENUM(pc + 0), (GLenum) bswap_ENUM(pc + 4));
}

void
__glXDispSwap_BindProgramARB(GLbyte * pc)
{
    PFNGLBINDPROGRAMARBPROC BindProgramARB =
        __glGetProcAddress("glBindProgramARB");
    BindProgramARB((GLenum) bswap_ENUM(pc + 0), (GLuint) bswap_CARD32(pc + 4));
}

int
__glXDispSwap_DeleteProgramsARB(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLDELETEPROGRAMSARBPROC DeleteProgramsARB =
        __glGetProcAddress("glDeleteProgramsARB");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        DeleteProgramsARB(n,
                          (const GLuint *) bswap_32_array((uint32_t *) (pc + 4),
                                                          0));
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GenProgramsARB(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGENPROGRAMSARBPROC GenProgramsARB =
        __glGetProcAddress("glGenProgramsARB");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        GLuint answerBuffer[200];
        GLuint *programs =
            __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer),
                                 4);
        if (programs == NULL)
            return BadAlloc;

        GenProgramsARB(n, programs);
        (void) bswap_32_array((uint32_t *) programs, n);
        __glXSendReplySwap(cl->client, programs, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetProgramEnvParameterdvARB(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGETPROGRAMENVPARAMETERDVARBPROC GetProgramEnvParameterdvARB =
        __glGetProcAddress("glGetProgramEnvParameterdvARB");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLdouble params[4];

        GetProgramEnvParameterdvARB((GLenum) bswap_ENUM(pc + 0),
                                    (GLuint) bswap_CARD32(pc + 4), params);
        (void) bswap_64_array((uint64_t *) params, 4);
        __glXSendReplySwap(cl->client, params, 4, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetProgramEnvParameterfvARB(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGETPROGRAMENVPARAMETERFVARBPROC GetProgramEnvParameterfvARB =
        __glGetProcAddress("glGetProgramEnvParameterfvARB");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLfloat params[4];

        GetProgramEnvParameterfvARB((GLenum) bswap_ENUM(pc + 0),
                                    (GLuint) bswap_CARD32(pc + 4), params);
        (void) bswap_32_array((uint32_t *) params, 4);
        __glXSendReplySwap(cl->client, params, 4, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetProgramLocalParameterdvARB(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGETPROGRAMLOCALPARAMETERDVARBPROC GetProgramLocalParameterdvARB =
        __glGetProcAddress("glGetProgramLocalParameterdvARB");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLdouble params[4];

        GetProgramLocalParameterdvARB((GLenum) bswap_ENUM(pc + 0),
                                      (GLuint) bswap_CARD32(pc + 4), params);
        (void) bswap_64_array((uint64_t *) params, 4);
        __glXSendReplySwap(cl->client, params, 4, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetProgramLocalParameterfvARB(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGETPROGRAMLOCALPARAMETERFVARBPROC GetProgramLocalParameterfvARB =
        __glGetProcAddress("glGetProgramLocalParameterfvARB");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLfloat params[4];

        GetProgramLocalParameterfvARB((GLenum) bswap_ENUM(pc + 0),
                                      (GLuint) bswap_CARD32(pc + 4), params);
        (void) bswap_32_array((uint32_t *) params, 4);
        __glXSendReplySwap(cl->client, params, 4, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetProgramivARB(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGETPROGRAMIVARBPROC GetProgramivARB =
        __glGetProcAddress("glGetProgramivARB");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLenum pname = (GLenum) bswap_ENUM(pc + 4);

        const GLuint compsize = __glGetProgramivARB_size(pname);
        GLint answerBuffer[200];
        GLint *params =
            __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer,
                                 sizeof(answerBuffer), 4);

        if (params == NULL)
            return BadAlloc;
        __glXClearErrorOccured();

        GetProgramivARB((GLenum) bswap_ENUM(pc + 0), pname, params);
        (void) bswap_32_array((uint32_t *) params, compsize);
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_IsProgramARB(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLISPROGRAMARBPROC IsProgramARB = __glGetProcAddress("glIsProgramARB");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLboolean retval;

        retval = IsProgramARB((GLuint) bswap_CARD32(pc + 0));
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_ProgramEnvParameter4dvARB(GLbyte * pc)
{
    PFNGLPROGRAMENVPARAMETER4DVARBPROC ProgramEnvParameter4dvARB =
        __glGetProcAddress("glProgramEnvParameter4dvARB");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 40);
        pc -= 4;
    }
#endif

    ProgramEnvParameter4dvARB((GLenum) bswap_ENUM(pc + 0),
                              (GLuint) bswap_CARD32(pc + 4),
                              (const GLdouble *)
                              bswap_64_array((uint64_t *) (pc + 8), 4));
}

void
__glXDispSwap_ProgramEnvParameter4fvARB(GLbyte * pc)
{
    PFNGLPROGRAMENVPARAMETER4FVARBPROC ProgramEnvParameter4fvARB =
        __glGetProcAddress("glProgramEnvParameter4fvARB");
    ProgramEnvParameter4fvARB((GLenum) bswap_ENUM(pc + 0),
                              (GLuint) bswap_CARD32(pc + 4),
                              (const GLfloat *)
                              bswap_32_array((uint32_t *) (pc + 8), 4));
}

void
__glXDispSwap_ProgramLocalParameter4dvARB(GLbyte * pc)
{
    PFNGLPROGRAMLOCALPARAMETER4DVARBPROC ProgramLocalParameter4dvARB =
        __glGetProcAddress("glProgramLocalParameter4dvARB");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 40);
        pc -= 4;
    }
#endif

    ProgramLocalParameter4dvARB((GLenum) bswap_ENUM(pc + 0),
                                (GLuint) bswap_CARD32(pc + 4),
                                (const GLdouble *)
                                bswap_64_array((uint64_t *) (pc + 8), 4));
}

void
__glXDispSwap_ProgramLocalParameter4fvARB(GLbyte * pc)
{
    PFNGLPROGRAMLOCALPARAMETER4FVARBPROC ProgramLocalParameter4fvARB =
        __glGetProcAddress("glProgramLocalParameter4fvARB");
    ProgramLocalParameter4fvARB((GLenum) bswap_ENUM(pc + 0),
                                (GLuint) bswap_CARD32(pc + 4),
                                (const GLfloat *)
                                bswap_32_array((uint32_t *) (pc + 8), 4));
}

void
__glXDispSwap_ProgramStringARB(GLbyte * pc)
{
    PFNGLPROGRAMSTRINGARBPROC ProgramStringARB =
        __glGetProcAddress("glProgramStringARB");
    const GLsizei len = (GLsizei) bswap_CARD32(pc + 8);

    ProgramStringARB((GLenum) bswap_ENUM(pc + 0),
                     (GLenum) bswap_ENUM(pc + 4),
                     len, (const GLvoid *) (pc + 12));
}

void
__glXDispSwap_VertexAttrib1fvARB(GLbyte * pc)
{
    PFNGLVERTEXATTRIB1FVARBPROC VertexAttrib1fvARB =
        __glGetProcAddress("glVertexAttrib1fvARB");
    VertexAttrib1fvARB((GLuint) bswap_CARD32(pc + 0),
                       (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                                        1));
}

void
__glXDispSwap_VertexAttrib2fvARB(GLbyte * pc)
{
    PFNGLVERTEXATTRIB2FVARBPROC VertexAttrib2fvARB =
        __glGetProcAddress("glVertexAttrib2fvARB");
    VertexAttrib2fvARB((GLuint) bswap_CARD32(pc + 0),
                       (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                                        2));
}

void
__glXDispSwap_VertexAttrib3fvARB(GLbyte * pc)
{
    PFNGLVERTEXATTRIB3FVARBPROC VertexAttrib3fvARB =
        __glGetProcAddress("glVertexAttrib3fvARB");
    VertexAttrib3fvARB((GLuint) bswap_CARD32(pc + 0),
                       (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                                        3));
}

void
__glXDispSwap_VertexAttrib4fvARB(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4FVARBPROC VertexAttrib4fvARB =
        __glGetProcAddress("glVertexAttrib4fvARB");
    VertexAttrib4fvARB((GLuint) bswap_CARD32(pc + 0),
                       (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                                        4));
}

void
__glXDispSwap_BindFramebuffer(GLbyte * pc)
{
    PFNGLBINDFRAMEBUFFERPROC BindFramebuffer =
        __glGetProcAddress("glBindFramebuffer");
    BindFramebuffer((GLenum) bswap_ENUM(pc + 0), (GLuint) bswap_CARD32(pc + 4));
}

void
__glXDispSwap_BindRenderbuffer(GLbyte * pc)
{
    PFNGLBINDRENDERBUFFERPROC BindRenderbuffer =
        __glGetProcAddress("glBindRenderbuffer");
    BindRenderbuffer((GLenum) bswap_ENUM(pc + 0),
                     (GLuint) bswap_CARD32(pc + 4));
}

void
__glXDispSwap_BlitFramebuffer(GLbyte * pc)
{
    PFNGLBLITFRAMEBUFFERPROC BlitFramebuffer =
        __glGetProcAddress("glBlitFramebuffer");
    BlitFramebuffer((GLint) bswap_CARD32(pc + 0), (GLint) bswap_CARD32(pc + 4),
                    (GLint) bswap_CARD32(pc + 8), (GLint) bswap_CARD32(pc + 12),
                    (GLint) bswap_CARD32(pc + 16),
                    (GLint) bswap_CARD32(pc + 20),
                    (GLint) bswap_CARD32(pc + 24),
                    (GLint) bswap_CARD32(pc + 28),
                    (GLbitfield) bswap_CARD32(pc + 32),
                    (GLenum) bswap_ENUM(pc + 36));
}

int
__glXDispSwap_CheckFramebufferStatus(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLCHECKFRAMEBUFFERSTATUSPROC CheckFramebufferStatus =
        __glGetProcAddress("glCheckFramebufferStatus");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLenum retval;

        retval = CheckFramebufferStatus((GLenum) bswap_ENUM(pc + 0));
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_DeleteFramebuffers(GLbyte * pc)
{
    PFNGLDELETEFRAMEBUFFERSPROC DeleteFramebuffers =
        __glGetProcAddress("glDeleteFramebuffers");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

    DeleteFramebuffers(n,
                       (const GLuint *) bswap_32_array((uint32_t *) (pc + 4),
                                                       0));
}

void
__glXDispSwap_DeleteRenderbuffers(GLbyte * pc)
{
    PFNGLDELETERENDERBUFFERSPROC DeleteRenderbuffers =
        __glGetProcAddress("glDeleteRenderbuffers");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

    DeleteRenderbuffers(n,
                        (const GLuint *) bswap_32_array((uint32_t *) (pc + 4),
                                                        0));
}

void
__glXDispSwap_FramebufferRenderbuffer(GLbyte * pc)
{
    PFNGLFRAMEBUFFERRENDERBUFFERPROC FramebufferRenderbuffer =
        __glGetProcAddress("glFramebufferRenderbuffer");
    FramebufferRenderbuffer((GLenum) bswap_ENUM(pc + 0),
                            (GLenum) bswap_ENUM(pc + 4),
                            (GLenum) bswap_ENUM(pc + 8),
                            (GLuint) bswap_CARD32(pc + 12));
}

void
__glXDispSwap_FramebufferTexture1D(GLbyte * pc)
{
    PFNGLFRAMEBUFFERTEXTURE1DPROC FramebufferTexture1D =
        __glGetProcAddress("glFramebufferTexture1D");
    FramebufferTexture1D((GLenum) bswap_ENUM(pc + 0),
                         (GLenum) bswap_ENUM(pc + 4),
                         (GLenum) bswap_ENUM(pc + 8),
                         (GLuint) bswap_CARD32(pc + 12),
                         (GLint) bswap_CARD32(pc + 16));
}

void
__glXDispSwap_FramebufferTexture2D(GLbyte * pc)
{
    PFNGLFRAMEBUFFERTEXTURE2DPROC FramebufferTexture2D =
        __glGetProcAddress("glFramebufferTexture2D");
    FramebufferTexture2D((GLenum) bswap_ENUM(pc + 0),
                         (GLenum) bswap_ENUM(pc + 4),
                         (GLenum) bswap_ENUM(pc + 8),
                         (GLuint) bswap_CARD32(pc + 12),
                         (GLint) bswap_CARD32(pc + 16));
}

void
__glXDispSwap_FramebufferTexture3D(GLbyte * pc)
{
    PFNGLFRAMEBUFFERTEXTURE3DPROC FramebufferTexture3D =
        __glGetProcAddress("glFramebufferTexture3D");
    FramebufferTexture3D((GLenum) bswap_ENUM(pc + 0),
                         (GLenum) bswap_ENUM(pc + 4),
                         (GLenum) bswap_ENUM(pc + 8),
                         (GLuint) bswap_CARD32(pc + 12),
                         (GLint) bswap_CARD32(pc + 16),
                         (GLint) bswap_CARD32(pc + 20));
}

void
__glXDispSwap_FramebufferTextureLayer(GLbyte * pc)
{
    PFNGLFRAMEBUFFERTEXTURELAYERPROC FramebufferTextureLayer =
        __glGetProcAddress("glFramebufferTextureLayer");
    FramebufferTextureLayer((GLenum) bswap_ENUM(pc + 0),
                            (GLenum) bswap_ENUM(pc + 4),
                            (GLuint) bswap_CARD32(pc + 8),
                            (GLint) bswap_CARD32(pc + 12),
                            (GLint) bswap_CARD32(pc + 16));
}

int
__glXDispSwap_GenFramebuffers(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGENFRAMEBUFFERSPROC GenFramebuffers =
        __glGetProcAddress("glGenFramebuffers");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        GLuint answerBuffer[200];
        GLuint *framebuffers =
            __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer),
                                 4);

        if (framebuffers == NULL)
            return BadAlloc;

        GenFramebuffers(n, framebuffers);
        (void) bswap_32_array((uint32_t *) framebuffers, n);
        __glXSendReplySwap(cl->client, framebuffers, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GenRenderbuffers(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGENRENDERBUFFERSPROC GenRenderbuffers =
        __glGetProcAddress("glGenRenderbuffers");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        const GLsizei n = (GLsizei) bswap_CARD32(pc + 0);

        GLuint answerBuffer[200];
        GLuint *renderbuffers =
            __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer),
                                 4);

        if (renderbuffers == NULL)
            return BadAlloc;

        GenRenderbuffers(n, renderbuffers);
        (void) bswap_32_array((uint32_t *) renderbuffers, n);
        __glXSendReplySwap(cl->client, renderbuffers, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_GenerateMipmap(GLbyte * pc)
{
    PFNGLGENERATEMIPMAPPROC GenerateMipmap =
        __glGetProcAddress("glGenerateMipmap");
    GenerateMipmap((GLenum) bswap_ENUM(pc + 0));
}

int
__glXDispSwap_GetFramebufferAttachmentParameteriv(__GLXclientState * cl,
                                                  GLbyte * pc)
{
    PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVPROC
        GetFramebufferAttachmentParameteriv =
        __glGetProcAddress("glGetFramebufferAttachmentParameteriv");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLint params[1];

        GetFramebufferAttachmentParameteriv((GLenum) bswap_ENUM(pc + 0),
                                            (GLenum) bswap_ENUM(pc + 4),
                                            (GLenum) bswap_ENUM(pc + 8),
                                            params);
        (void) bswap_32_array((uint32_t *) params, 1);
        __glXSendReplySwap(cl->client, params, 1, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_GetRenderbufferParameteriv(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLGETRENDERBUFFERPARAMETERIVPROC GetRenderbufferParameteriv =
        __glGetProcAddress("glGetRenderbufferParameteriv");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLint params[1];

        GetRenderbufferParameteriv((GLenum) bswap_ENUM(pc + 0),
                                   (GLenum) bswap_ENUM(pc + 4), params);
        (void) bswap_32_array((uint32_t *) params, 1);
        __glXSendReplySwap(cl->client, params, 1, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_IsFramebuffer(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLISFRAMEBUFFERPROC IsFramebuffer =
        __glGetProcAddress("glIsFramebuffer");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLboolean retval;

        retval = IsFramebuffer((GLuint) bswap_CARD32(pc + 0));
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int
__glXDispSwap_IsRenderbuffer(__GLXclientState * cl, GLbyte * pc)
{
    PFNGLISRENDERBUFFERPROC IsRenderbuffer =
        __glGetProcAddress("glIsRenderbuffer");
    xGLXVendorPrivateReq *const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext *const cx =
        __glXForceCurrent(cl, bswap_CARD32(&req->contextTag), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if (cx != NULL) {
        GLboolean retval;

        retval = IsRenderbuffer((GLuint) bswap_CARD32(pc + 0));
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void
__glXDispSwap_RenderbufferStorage(GLbyte * pc)
{
    PFNGLRENDERBUFFERSTORAGEPROC RenderbufferStorage =
        __glGetProcAddress("glRenderbufferStorage");
    RenderbufferStorage((GLenum) bswap_ENUM(pc + 0),
                        (GLenum) bswap_ENUM(pc + 4),
                        (GLsizei) bswap_CARD32(pc + 8),
                        (GLsizei) bswap_CARD32(pc + 12));
}

void
__glXDispSwap_RenderbufferStorageMultisample(GLbyte * pc)
{
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC RenderbufferStorageMultisample =
        __glGetProcAddress("glRenderbufferStorageMultisample");
    RenderbufferStorageMultisample((GLenum) bswap_ENUM(pc + 0),
                                   (GLsizei) bswap_CARD32(pc + 4),
                                   (GLenum) bswap_ENUM(pc + 8),
                                   (GLsizei) bswap_CARD32(pc + 12),
                                   (GLsizei) bswap_CARD32(pc + 16));
}

void
__glXDispSwap_SecondaryColor3fvEXT(GLbyte * pc)
{
    PFNGLSECONDARYCOLOR3FVEXTPROC SecondaryColor3fvEXT =
        __glGetProcAddress("glSecondaryColor3fvEXT");
    SecondaryColor3fvEXT((const GLfloat *)
                         bswap_32_array((uint32_t *) (pc + 0), 3));
}

void
__glXDispSwap_FogCoordfvEXT(GLbyte * pc)
{
    PFNGLFOGCOORDFVEXTPROC FogCoordfvEXT =
        __glGetProcAddress("glFogCoordfvEXT");
    FogCoordfvEXT((const GLfloat *) bswap_32_array((uint32_t *) (pc + 0), 1));
}

void
__glXDispSwap_VertexAttrib1dvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB1DVNVPROC VertexAttrib1dvNV =
        __glGetProcAddress("glVertexAttrib1dvNV");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 12);
        pc -= 4;
    }
#endif

    VertexAttrib1dvNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLdouble *) bswap_64_array((uint64_t *) (pc + 4),
                                                        1));
}

void
__glXDispSwap_VertexAttrib1fvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB1FVNVPROC VertexAttrib1fvNV =
        __glGetProcAddress("glVertexAttrib1fvNV");
    VertexAttrib1fvNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                                       1));
}

void
__glXDispSwap_VertexAttrib1svNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB1SVNVPROC VertexAttrib1svNV =
        __glGetProcAddress("glVertexAttrib1svNV");
    VertexAttrib1svNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLshort *) bswap_16_array((uint16_t *) (pc + 4),
                                                       1));
}

void
__glXDispSwap_VertexAttrib2dvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB2DVNVPROC VertexAttrib2dvNV =
        __glGetProcAddress("glVertexAttrib2dvNV");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 20);
        pc -= 4;
    }
#endif

    VertexAttrib2dvNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLdouble *) bswap_64_array((uint64_t *) (pc + 4),
                                                        2));
}

void
__glXDispSwap_VertexAttrib2fvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB2FVNVPROC VertexAttrib2fvNV =
        __glGetProcAddress("glVertexAttrib2fvNV");
    VertexAttrib2fvNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                                       2));
}

void
__glXDispSwap_VertexAttrib2svNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB2SVNVPROC VertexAttrib2svNV =
        __glGetProcAddress("glVertexAttrib2svNV");
    VertexAttrib2svNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLshort *) bswap_16_array((uint16_t *) (pc + 4),
                                                       2));
}

void
__glXDispSwap_VertexAttrib3dvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB3DVNVPROC VertexAttrib3dvNV =
        __glGetProcAddress("glVertexAttrib3dvNV");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 28);
        pc -= 4;
    }
#endif

    VertexAttrib3dvNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLdouble *) bswap_64_array((uint64_t *) (pc + 4),
                                                        3));
}

void
__glXDispSwap_VertexAttrib3fvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB3FVNVPROC VertexAttrib3fvNV =
        __glGetProcAddress("glVertexAttrib3fvNV");
    VertexAttrib3fvNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                                       3));
}

void
__glXDispSwap_VertexAttrib3svNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB3SVNVPROC VertexAttrib3svNV =
        __glGetProcAddress("glVertexAttrib3svNV");
    VertexAttrib3svNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLshort *) bswap_16_array((uint16_t *) (pc + 4),
                                                       3));
}

void
__glXDispSwap_VertexAttrib4dvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4DVNVPROC VertexAttrib4dvNV =
        __glGetProcAddress("glVertexAttrib4dvNV");
#ifdef __GLX_ALIGN64
    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, 36);
        pc -= 4;
    }
#endif

    VertexAttrib4dvNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLdouble *) bswap_64_array((uint64_t *) (pc + 4),
                                                        4));
}

void
__glXDispSwap_VertexAttrib4fvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4FVNVPROC VertexAttrib4fvNV =
        __glGetProcAddress("glVertexAttrib4fvNV");
    VertexAttrib4fvNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLfloat *) bswap_32_array((uint32_t *) (pc + 4),
                                                       4));
}

void
__glXDispSwap_VertexAttrib4svNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4SVNVPROC VertexAttrib4svNV =
        __glGetProcAddress("glVertexAttrib4svNV");
    VertexAttrib4svNV((GLuint) bswap_CARD32(pc + 0),
                      (const GLshort *) bswap_16_array((uint16_t *) (pc + 4),
                                                       4));
}

void
__glXDispSwap_VertexAttrib4ubvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIB4UBVNVPROC VertexAttrib4ubvNV =
        __glGetProcAddress("glVertexAttrib4ubvNV");
    VertexAttrib4ubvNV((GLuint) bswap_CARD32(pc + 0),
                       (const GLubyte *) (pc + 4));
}

void
__glXDispSwap_VertexAttribs1dvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS1DVNVPROC VertexAttribs1dvNV =
        __glGetProcAddress("glVertexAttribs1dvNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 8)) - 4;

    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, cmdlen);
        pc -= 4;
    }
#endif

    VertexAttribs1dvNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLdouble *) bswap_64_array((uint64_t *) (pc + 8),
                                                         0));
}

void
__glXDispSwap_VertexAttribs1fvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS1FVNVPROC VertexAttribs1fvNV =
        __glGetProcAddress("glVertexAttribs1fvNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

    VertexAttribs1fvNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                                        0));
}

void
__glXDispSwap_VertexAttribs1svNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS1SVNVPROC VertexAttribs1svNV =
        __glGetProcAddress("glVertexAttribs1svNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

    VertexAttribs1svNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLshort *) bswap_16_array((uint16_t *) (pc + 8),
                                                        0));
}

void
__glXDispSwap_VertexAttribs2dvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS2DVNVPROC VertexAttribs2dvNV =
        __glGetProcAddress("glVertexAttribs2dvNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 16)) - 4;

    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, cmdlen);
        pc -= 4;
    }
#endif

    VertexAttribs2dvNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLdouble *) bswap_64_array((uint64_t *) (pc + 8),
                                                         0));
}

void
__glXDispSwap_VertexAttribs2fvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS2FVNVPROC VertexAttribs2fvNV =
        __glGetProcAddress("glVertexAttribs2fvNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

    VertexAttribs2fvNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                                        0));
}

void
__glXDispSwap_VertexAttribs2svNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS2SVNVPROC VertexAttribs2svNV =
        __glGetProcAddress("glVertexAttribs2svNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

    VertexAttribs2svNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLshort *) bswap_16_array((uint16_t *) (pc + 8),
                                                        0));
}

void
__glXDispSwap_VertexAttribs3dvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS3DVNVPROC VertexAttribs3dvNV =
        __glGetProcAddress("glVertexAttribs3dvNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 24)) - 4;

    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, cmdlen);
        pc -= 4;
    }
#endif

    VertexAttribs3dvNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLdouble *) bswap_64_array((uint64_t *) (pc + 8),
                                                         0));
}

void
__glXDispSwap_VertexAttribs3fvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS3FVNVPROC VertexAttribs3fvNV =
        __glGetProcAddress("glVertexAttribs3fvNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

    VertexAttribs3fvNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                                        0));
}

void
__glXDispSwap_VertexAttribs3svNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS3SVNVPROC VertexAttribs3svNV =
        __glGetProcAddress("glVertexAttribs3svNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

    VertexAttribs3svNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLshort *) bswap_16_array((uint16_t *) (pc + 8),
                                                        0));
}

void
__glXDispSwap_VertexAttribs4dvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS4DVNVPROC VertexAttribs4dvNV =
        __glGetProcAddress("glVertexAttribs4dvNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 32)) - 4;

    if ((unsigned long) (pc) & 7) {
        (void) memmove(pc - 4, pc, cmdlen);
        pc -= 4;
    }
#endif

    VertexAttribs4dvNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLdouble *) bswap_64_array((uint64_t *) (pc + 8),
                                                         0));
}

void
__glXDispSwap_VertexAttribs4fvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS4FVNVPROC VertexAttribs4fvNV =
        __glGetProcAddress("glVertexAttribs4fvNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

    VertexAttribs4fvNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLfloat *) bswap_32_array((uint32_t *) (pc + 8),
                                                        0));
}

void
__glXDispSwap_VertexAttribs4svNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS4SVNVPROC VertexAttribs4svNV =
        __glGetProcAddress("glVertexAttribs4svNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

    VertexAttribs4svNV((GLuint) bswap_CARD32(pc + 0),
                       n,
                       (const GLshort *) bswap_16_array((uint16_t *) (pc + 8),
                                                        0));
}

void
__glXDispSwap_VertexAttribs4ubvNV(GLbyte * pc)
{
    PFNGLVERTEXATTRIBS4UBVNVPROC VertexAttribs4ubvNV =
        __glGetProcAddress("glVertexAttribs4ubvNV");
    const GLsizei n = (GLsizei) bswap_CARD32(pc + 4);

    VertexAttribs4ubvNV((GLuint) bswap_CARD32(pc + 0),
                        n, (const GLubyte *) (pc + 8));
}

void
__glXDispSwap_ActiveStencilFaceEXT(GLbyte * pc)
{
    PFNGLACTIVESTENCILFACEEXTPROC ActiveStencilFaceEXT =
        __glGetProcAddress("glActiveStencilFaceEXT");
    ActiveStencilFaceEXT((GLenum) bswap_ENUM(pc + 0));
}
