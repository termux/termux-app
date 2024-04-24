/*
 * Copyright © 2008 Intel Corporation
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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *    Zhigang Gong <zhigang.gong@linux.intel.com>
 *
 */

#ifndef GLAMOR_H
#define GLAMOR_H

#include <scrnintstr.h>
#include <pixmapstr.h>
#include <gcstruct.h>
#include <picturestr.h>
#include <fb.h>
#include <fbpict.h>
#ifdef GLAMOR_FOR_XORG
#include <xf86xv.h>
#endif

struct glamor_context;
struct gbm_bo;
struct gbm_device;

/*
 * glamor_pixmap_type : glamor pixmap's type.
 * @MEMORY: pixmap is in memory.
 * @TEXTURE_DRM: pixmap is in a texture created from a DRM buffer.
 * @SEPARATE_TEXTURE: The texture is created from a DRM buffer, but
 * 		      the format is incompatible, so this type of pixmap
 * 		      will never fallback to DDX layer.
 * @DRM_ONLY: pixmap is in a external DRM buffer.
 * @TEXTURE_ONLY: pixmap is in an internal texture.
 */
typedef enum glamor_pixmap_type {
    GLAMOR_MEMORY = 0, /* Newly calloc()ed pixmaps are memory. */
    GLAMOR_TEXTURE_DRM,
    GLAMOR_DRM_ONLY,
    GLAMOR_TEXTURE_ONLY,
} glamor_pixmap_type_t;

typedef Bool (*GetDrawableModifiersFuncPtr) (DrawablePtr draw,
                                             uint32_t format,
                                             uint32_t *num_modifiers,
                                             uint64_t **modifiers);

#define GLAMOR_EGL_EXTERNAL_BUFFER 3
#define GLAMOR_USE_EGL_SCREEN		(1 << 0)
#define GLAMOR_NO_DRI3			(1 << 1)
#define GLAMOR_VALID_FLAGS      (GLAMOR_USE_EGL_SCREEN                \
                                 | GLAMOR_NO_DRI3)

/* until we need geometry shaders GL3.1 should suffice. */
#define GLAMOR_GL_CORE_VER_MAJOR 3
#define GLAMOR_GL_CORE_VER_MINOR 1

/* @glamor_init: Initialize glamor internal data structure.
 *
 * @screen: Current screen pointer.
 * @flags:  Please refer the flags description above.
 *
 * 	@GLAMOR_USE_EGL_SCREEN:
 * 	If you are using EGL layer, then please set this bit
 * 	on, otherwise, clear it.
 *
 *      @GLAMOR_NO_DRI3
 *      Disable the built-in DRI3 support
 *
 * This function initializes necessary internal data structure
 * for glamor. And before calling into this function, the OpenGL
 * environment should be ready. Should be called before any real
 * glamor rendering or texture allocation functions. And should
 * be called after the DDX's screen initialization or at the last
 * step of the DDX's screen initialization.
 */
extern _X_EXPORT Bool glamor_init(ScreenPtr screen, unsigned int flags);
extern _X_EXPORT void glamor_fini(ScreenPtr screen);

/* This function is used to free the glamor private screen's
 * resources. If the DDX driver is not set GLAMOR_USE_SCREEN,
 * then, DDX need to call this function at proper stage, if
 * it is the xorg DDX driver,then it should be called at free
 * screen stage not the close screen stage. The reason is after
 * call to this function, the xorg DDX may need to destroy the
 * screen pixmap which must be a glamor pixmap and requires
 * the internal data structure still exist at that time.
 * Otherwise, the glamor internal structure will not be freed.*/
extern _X_EXPORT Bool glamor_close_screen(ScreenPtr screen);

extern _X_EXPORT uint32_t glamor_get_pixmap_texture(PixmapPtr pixmap);

extern _X_EXPORT Bool glamor_set_pixmap_texture(PixmapPtr pixmap,
                                                unsigned int tex);

extern _X_EXPORT void glamor_set_pixmap_type(PixmapPtr pixmap,
                                             glamor_pixmap_type_t type);

extern _X_EXPORT void glamor_clear_pixmap(PixmapPtr pixmap);

extern _X_EXPORT void glamor_block_handler(ScreenPtr screen);

extern _X_EXPORT PixmapPtr glamor_create_pixmap(ScreenPtr screen, int w, int h,
                                                int depth, unsigned int usage);
extern _X_EXPORT Bool glamor_destroy_pixmap(PixmapPtr pixmap);

#define GLAMOR_CREATE_PIXMAP_CPU        0x100
#define GLAMOR_CREATE_PIXMAP_FIXUP      0x101
#define GLAMOR_CREATE_FBO_NO_FBO        0x103
#define GLAMOR_CREATE_NO_LARGE          0x105
#define GLAMOR_CREATE_PIXMAP_NO_TEXTURE 0x106
#define GLAMOR_CREATE_FORMAT_CBCR       0x107

/* @glamor_egl_exchange_buffers: Exchange the underlying buffers(KHR image,fbo).
 *
 * @front: front pixmap.
 * @back: back pixmap.
 *
 * Used by the DRI2 page flip. This function will exchange the KHR images and
 * fbos of the two pixmaps.
 * */
extern _X_EXPORT void glamor_egl_exchange_buffers(PixmapPtr front,
                                                  PixmapPtr back);

extern _X_EXPORT void glamor_pixmap_exchange_fbos(PixmapPtr front,
                                                  PixmapPtr back);

/* The DDX is not supposed to call these four functions */
extern _X_EXPORT void glamor_enable_dri3(ScreenPtr screen);
extern _X_EXPORT int glamor_egl_fds_from_pixmap(ScreenPtr, PixmapPtr, int *,
                                                uint32_t *, uint32_t *,
                                                uint64_t *);
extern _X_EXPORT int glamor_egl_fd_name_from_pixmap(ScreenPtr, PixmapPtr,
                                                    CARD16 *, CARD32 *);

extern _X_EXPORT struct gbm_device *glamor_egl_get_gbm_device(ScreenPtr screen);
extern _X_EXPORT int glamor_egl_fd_from_pixmap(ScreenPtr, PixmapPtr, CARD16 *, CARD32 *);

/* @glamor_supports_pixmap_import_export: Returns whether
 * glamor_fds_from_pixmap(), glamor_name_from_pixmap(), and
 * glamor_pixmap_from_fds() are supported.
 *
 * @screen: Current screen pointer.
 *
 * To have DRI3 support enabled, glamor and glamor_egl need to be
 * initialized. glamor also has to be compiled with gbm support.
 *
 * The EGL layer needs to have the following extensions working:
 *
 * .EGL_KHR_surfaceless_context
 * */
extern _X_EXPORT Bool glamor_supports_pixmap_import_export(ScreenPtr screen);

/* @glamor_fds_from_pixmap: Get a dma-buf fd from a pixmap.
 *
 * @screen: Current screen pointer.
 * @pixmap: The pixmap from which we want the fd.
 * @fds, @strides, @offsets: Pointers to fill info of each plane.
 * @modifier: Pointer to fill the modifier of the buffer.
 *
 * the pixmap and the buffer associated by the fds will share the same
 * content. The caller is responsible to close the returned file descriptors.
 * Returns the number of planes, -1 on error.
 * */
extern _X_EXPORT int glamor_fds_from_pixmap(ScreenPtr screen,
                                            PixmapPtr pixmap,
                                            int *fds,
                                            uint32_t *strides, uint32_t *offsets,
                                            uint64_t *modifier);

/* @glamor_fd_from_pixmap: Get a dma-buf fd from a pixmap.
 *
 * @screen: Current screen pointer.
 * @pixmap: The pixmap from which we want the fd.
 * @stride, @size: Pointers to fill the stride and size of the
 * 		   buffer associated to the fd.
 *
 * the pixmap and the buffer associated by the fd will share the same
 * content.
 * Returns the fd on success, -1 on error.
 * */
extern _X_EXPORT int glamor_fd_from_pixmap(ScreenPtr screen,
                                           PixmapPtr pixmap,
                                           CARD16 *stride, CARD32 *size);

/* @glamor_shareable_fd_from_pixmap: Get a dma-buf fd suitable for sharing
 *				     with other GPUs from a pixmap.
 *
 * @screen: Current screen pointer.
 * @pixmap: The pixmap from which we want the fd.
 * @stride, @size: Pointers to fill the stride and size of the
 * 		   buffer associated to the fd.
 *
 * The returned fd will point to a buffer which is suitable for sharing
 * across GPUs (not using GPU specific tiling).
 * The pixmap and the buffer associated by the fd will share the same
 * content.
 * The pixmap's stride may be modified by this function.
 * Returns the fd on success, -1 on error.
 * */
extern _X_EXPORT int glamor_shareable_fd_from_pixmap(ScreenPtr screen,
                                                     PixmapPtr pixmap,
                                                     CARD16 *stride,
                                                     CARD32 *size);

/**
 * @glamor_name_from_pixmap: Gets a gem name from a pixmap.
 *
 * @pixmap: The pixmap from which we want the gem name.
 *
 * the pixmap and the buffer associated by the gem name will share the
 * same content. This function can be used by the DDX to support DRI2,
 * and needs the same set of buffer export GL extensions as DRI3
 * support.
 *
 * Returns the name on success, -1 on error.
 * */
extern _X_EXPORT int glamor_name_from_pixmap(PixmapPtr pixmap,
                                             CARD16 *stride, CARD32 *size);

/* @glamor_gbm_bo_from_pixmap: Get a GBM bo from a pixmap.
 *
 * @screen: Current screen pointer.
 * @pixmap: The pixmap from which we want the fd.
 * @stride, @size: Pointers to fill the stride and size of the
 * 		   buffer associated to the fd.
 *
 * the pixmap and the buffer represented by the gbm_bo will share the same
 * content.
 *
 * Returns the gbm_bo on success, NULL on error.
 * */
extern _X_EXPORT struct gbm_bo *glamor_gbm_bo_from_pixmap(ScreenPtr screen,
                                                          PixmapPtr pixmap);

/* @glamor_pixmap_from_fds: Creates a pixmap to wrap a dma-buf fds.
 *
 * @screen: Current screen pointer.
 * @num_fds: Number of fds to import
 * @fds: The dma-buf fds to import.
 * @width: The width of the buffers.
 * @height: The height of the buffers.
 * @stride: The stride of the buffers.
 * @depth: The depth of the buffers.
 * @bpp: The bpp of the buffers.
 * @modifier: The modifier of the buffers.
 *
 * Returns a valid pixmap if the import succeeded, else NULL.
 * */
extern _X_EXPORT PixmapPtr glamor_pixmap_from_fds(ScreenPtr screen,
                                                  CARD8 num_fds,
                                                  const int *fds,
                                                  CARD16 width,
                                                  CARD16 height,
                                                  const CARD32 *strides,
                                                  const CARD32 *offsets,
                                                  CARD8 depth,
                                                  CARD8 bpp,
                                                  uint64_t modifier);

/* @glamor_pixmap_from_fd: Creates a pixmap to wrap a dma-buf fd.
 *
 * @screen: Current screen pointer.
 * @fd: The dma-buf fd to import.
 * @width: The width of the buffer.
 * @height: The height of the buffer.
 * @stride: The stride of the buffer.
 * @depth: The depth of the buffer.
 * @bpp: The bpp of the buffer.
 *
 * Returns a valid pixmap if the import succeeded, else NULL.
 * */
extern _X_EXPORT PixmapPtr glamor_pixmap_from_fd(ScreenPtr screen,
                                                 int fd,
                                                 CARD16 width,
                                                 CARD16 height,
                                                 CARD16 stride,
                                                 CARD8 depth,
                                                 CARD8 bpp);

/* @glamor_back_pixmap_from_fd: Backs an existing pixmap with a dma-buf fd.
 *
 * @pixmap: Pixmap to change backing for
 * @fd: The dma-buf fd to import.
 * @width: The width of the buffer.
 * @height: The height of the buffer.
 * @stride: The stride of the buffer.
 * @depth: The depth of the buffer.
 * @bpp: The number of bpp of the buffer.
 *
 * Returns TRUE if successful, FALSE on failure.
 * */
extern _X_EXPORT Bool glamor_back_pixmap_from_fd(PixmapPtr pixmap,
                                                 int fd,
                                                 CARD16 width,
                                                 CARD16 height,
                                                 CARD16 stride,
                                                 CARD8 depth,
                                                 CARD8 bpp);

extern _X_EXPORT Bool glamor_get_formats(ScreenPtr screen,
                                         CARD32 *num_formats,
                                         CARD32 **formats);

extern _X_EXPORT Bool glamor_get_modifiers(ScreenPtr screen,
                                           uint32_t format,
                                           uint32_t *num_modifiers,
                                           uint64_t **modifiers);

extern _X_EXPORT Bool glamor_get_drawable_modifiers(DrawablePtr draw,
                                                    uint32_t format,
                                                    uint32_t *num_modifiers,
                                                    uint64_t **modifiers);

extern _X_EXPORT void glamor_set_drawable_modifiers_func(ScreenPtr screen,
                                                         GetDrawableModifiersFuncPtr func);

#ifdef GLAMOR_FOR_XORG

#define GLAMOR_EGL_MODULE_NAME  "glamoregl"

/* @glamor_egl_init: Initialize EGL environment.
 *
 * @scrn: Current screen info pointer.
 * @fd:   Current drm fd.
 *
 * This function creates and initializes EGL contexts.
 * Should be called from DDX's preInit function.
 * Return TRUE if success, otherwise return FALSE.
 * */
extern _X_EXPORT Bool glamor_egl_init(ScrnInfoPtr scrn, int fd);

extern _X_EXPORT Bool glamor_egl_init_textured_pixmap(ScreenPtr screen);

/* @glamor_egl_create_textured_screen: Create textured screen pixmap.
 *
 * @screen: screen pointer to be processed.
 * @handle: screen pixmap's BO handle.
 * @stride: screen pixmap's stride in bytes.
 *
 * This function is similar with the create_textured_pixmap. As the
 * screen pixmap is a special, we handle it separately in this function.
 */
extern _X_EXPORT Bool glamor_egl_create_textured_screen(ScreenPtr screen,
                                                        int handle, int stride);

/* Obsolete entrypoint, temporarily left here for API compatibility
 * for xf86-video-ati.
 */
#define glamor_egl_create_textured_screen_ext(a, b, c, d) \
    glamor_egl_create_textured_screen(a, b, c)

/*
 * @glamor_egl_create_textured_pixmap: Try to create a textured pixmap from
 * 				       a BO handle.
 *
 * @pixmap: The pixmap need to be processed.
 * @handle: The BO's handle attached to this pixmap at DDX layer.
 * @stride: Stride in bytes for this pixmap.
 *
 * This function try to create a texture from the handle and attach
 * the texture to the pixmap , thus glamor can render to this pixmap
 * as well. Return true if successful, otherwise return FALSE.
 */
extern _X_EXPORT Bool glamor_egl_create_textured_pixmap(PixmapPtr pixmap,
                                                        int handle, int stride);

/*
 * @glamor_egl_create_textured_pixmap_from_bo: Try to create a textured pixmap
 * 					       from a gbm_bo.
 *
 * @pixmap: The pixmap need to be processed.
 * @bo: a pointer on a gbm_bo structure attached to this pixmap at DDX layer.
 *
 * This function is similar to glamor_egl_create_textured_pixmap.
 */
extern _X_EXPORT Bool
 glamor_egl_create_textured_pixmap_from_gbm_bo(PixmapPtr pixmap,
                                               struct gbm_bo *bo,
                                               Bool used_modifiers);

extern _X_EXPORT const char *glamor_egl_get_driver_name(ScreenPtr screen);

#endif

extern _X_EXPORT void glamor_egl_screen_init(ScreenPtr screen,
                                             struct glamor_context *glamor_ctx);

extern _X_EXPORT int glamor_create_gc(GCPtr gc);

extern _X_EXPORT void glamor_validate_gc(GCPtr gc, unsigned long changes,
                                         DrawablePtr drawable);

extern _X_EXPORT void glamor_destroy_gc(GCPtr gc);

#define HAS_GLAMOR_DESTROY_GC 1

extern Bool _X_EXPORT glamor_change_window_attributes(WindowPtr pWin, unsigned long mask);
extern void _X_EXPORT glamor_copy_window(WindowPtr window, DDXPointRec old_origin, RegionPtr src_region);

extern _X_EXPORT void glamor_finish(ScreenPtr screen);
#define HAS_GLAMOR_TEXT 1

#ifdef GLAMOR_FOR_XORG
extern _X_EXPORT XF86VideoAdaptorPtr glamor_xv_init(ScreenPtr pScreen,
                                                    int num_texture_ports);
#endif

#endif                          /* GLAMOR_H */
