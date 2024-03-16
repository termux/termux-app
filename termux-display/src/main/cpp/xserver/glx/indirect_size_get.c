/* DO NOT EDIT - This file generated automatically by glX_proto_size.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2004
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


#include <X11/Xfuncproto.h>
#include <GL/gl.h>
#include "indirect_size_get.h"
#include "glxserver.h"
#include "indirect_util.h"
#include "indirect_size.h"

#if defined(__GNUC__) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590))
#define PURE __attribute__((pure))
#else
#define PURE
#endif

#if defined(__i386__) && defined(__GNUC__) && !defined(__CYGWIN__) && !defined(__MINGW32__)
#define FASTCALL __attribute__((fastcall))
#else
#define FASTCALL
#endif


#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(GLX_USE_APPLEGL)
#undef HAVE_ALIAS
#endif
#ifdef HAVE_ALIAS
#define ALIAS2(from,to) \
    _X_INTERNAL PURE FASTCALL GLint __gl ## from ## _size( GLenum e ) \
        __attribute__ ((alias( # to )));
#define ALIAS(from,to) ALIAS2( from, __gl ## to ## _size )
#else
#define ALIAS(from,to) \
    _X_INTERNAL PURE FASTCALL GLint __gl ## from ## _size( GLenum e ) \
    { return __gl ## to ## _size( e ); }
#endif


_X_INTERNAL PURE FASTCALL GLint
__glCallLists_size(GLenum e)
{
    switch (e) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
        return 1;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
    case GL_2_BYTES:
    case GL_HALF_FLOAT:
        return 2;
    case GL_3_BYTES:
        return 3;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
    case GL_4_BYTES:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glFogfv_size(GLenum e)
{
    switch (e) {
    case GL_FOG_INDEX:
    case GL_FOG_DENSITY:
    case GL_FOG_START:
    case GL_FOG_END:
    case GL_FOG_MODE:
    case GL_FOG_OFFSET_VALUE_SGIX:
    case GL_FOG_DISTANCE_MODE_NV:
        return 1;
    case GL_FOG_COLOR:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glLightfv_size(GLenum e)
{
    switch (e) {
    case GL_SPOT_EXPONENT:
    case GL_SPOT_CUTOFF:
    case GL_CONSTANT_ATTENUATION:
    case GL_LINEAR_ATTENUATION:
    case GL_QUADRATIC_ATTENUATION:
        return 1;
    case GL_SPOT_DIRECTION:
        return 3;
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_POSITION:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glLightModelfv_size(GLenum e)
{
    switch (e) {
    case GL_LIGHT_MODEL_LOCAL_VIEWER:
    case GL_LIGHT_MODEL_TWO_SIDE:
    case GL_LIGHT_MODEL_COLOR_CONTROL:
/*      case GL_LIGHT_MODEL_COLOR_CONTROL_EXT:*/
        return 1;
    case GL_LIGHT_MODEL_AMBIENT:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glMaterialfv_size(GLenum e)
{
    switch (e) {
    case GL_SHININESS:
        return 1;
    case GL_COLOR_INDEXES:
        return 3;
    case GL_AMBIENT:
    case GL_DIFFUSE:
    case GL_SPECULAR:
    case GL_EMISSION:
    case GL_AMBIENT_AND_DIFFUSE:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glTexParameterfv_size(GLenum e)
{
    switch (e) {
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_PRIORITY:
    case GL_TEXTURE_WRAP_R:
    case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
/*      case GL_SHADOW_AMBIENT_SGIX:*/
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD:
    case GL_TEXTURE_BASE_LEVEL:
    case GL_TEXTURE_MAX_LEVEL:
    case GL_TEXTURE_CLIPMAP_FRAME_SGIX:
    case GL_TEXTURE_LOD_BIAS_S_SGIX:
    case GL_TEXTURE_LOD_BIAS_T_SGIX:
    case GL_TEXTURE_LOD_BIAS_R_SGIX:
    case GL_GENERATE_MIPMAP:
/*      case GL_GENERATE_MIPMAP_SGIS:*/
    case GL_TEXTURE_COMPARE_SGIX:
    case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
    case GL_TEXTURE_MAX_CLAMP_S_SGIX:
    case GL_TEXTURE_MAX_CLAMP_T_SGIX:
    case GL_TEXTURE_MAX_CLAMP_R_SGIX:
    case GL_TEXTURE_MAX_ANISOTROPY_EXT:
    case GL_TEXTURE_LOD_BIAS:
/*      case GL_TEXTURE_LOD_BIAS_EXT:*/
    case GL_TEXTURE_STORAGE_HINT_APPLE:
    case GL_STORAGE_PRIVATE_APPLE:
    case GL_STORAGE_CACHED_APPLE:
    case GL_STORAGE_SHARED_APPLE:
    case GL_DEPTH_TEXTURE_MODE:
/*      case GL_DEPTH_TEXTURE_MODE_ARB:*/
    case GL_TEXTURE_COMPARE_MODE:
/*      case GL_TEXTURE_COMPARE_MODE_ARB:*/
    case GL_TEXTURE_COMPARE_FUNC:
/*      case GL_TEXTURE_COMPARE_FUNC_ARB:*/
    case GL_TEXTURE_UNSIGNED_REMAP_MODE_NV:
        return 1;
    case GL_TEXTURE_CLIPMAP_CENTER_SGIX:
    case GL_TEXTURE_CLIPMAP_OFFSET_SGIX:
        return 2;
    case GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX:
        return 3;
    case GL_TEXTURE_BORDER_COLOR:
    case GL_POST_TEXTURE_FILTER_BIAS_SGIX:
    case GL_POST_TEXTURE_FILTER_SCALE_SGIX:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glTexEnvfv_size(GLenum e)
{
    switch (e) {
    case GL_ALPHA_SCALE:
    case GL_TEXTURE_ENV_MODE:
    case GL_TEXTURE_LOD_BIAS:
    case GL_COMBINE_RGB:
    case GL_COMBINE_ALPHA:
    case GL_RGB_SCALE:
    case GL_SOURCE0_RGB:
    case GL_SOURCE1_RGB:
    case GL_SOURCE2_RGB:
    case GL_SOURCE3_RGB_NV:
    case GL_SOURCE0_ALPHA:
    case GL_SOURCE1_ALPHA:
    case GL_SOURCE2_ALPHA:
    case GL_SOURCE3_ALPHA_NV:
    case GL_OPERAND0_RGB:
    case GL_OPERAND1_RGB:
    case GL_OPERAND2_RGB:
    case GL_OPERAND3_RGB_NV:
    case GL_OPERAND0_ALPHA:
    case GL_OPERAND1_ALPHA:
    case GL_OPERAND2_ALPHA:
    case GL_OPERAND3_ALPHA_NV:
    case GL_BUMP_TARGET_ATI:
    case GL_COORD_REPLACE_ARB:
/*      case GL_COORD_REPLACE_NV:*/
        return 1;
    case GL_TEXTURE_ENV_COLOR:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glTexGendv_size(GLenum e)
{
    switch (e) {
    case GL_TEXTURE_GEN_MODE:
        return 1;
    case GL_OBJECT_PLANE:
    case GL_EYE_PLANE:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glMap1d_size(GLenum e)
{
    switch (e) {
    case GL_MAP1_INDEX:
    case GL_MAP1_TEXTURE_COORD_1:
        return 1;
    case GL_MAP1_TEXTURE_COORD_2:
        return 2;
    case GL_MAP1_NORMAL:
    case GL_MAP1_TEXTURE_COORD_3:
    case GL_MAP1_VERTEX_3:
        return 3;
    case GL_MAP1_COLOR_4:
    case GL_MAP1_TEXTURE_COORD_4:
    case GL_MAP1_VERTEX_4:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glMap2d_size(GLenum e)
{
    switch (e) {
    case GL_MAP2_INDEX:
    case GL_MAP2_TEXTURE_COORD_1:
        return 1;
    case GL_MAP2_TEXTURE_COORD_2:
        return 2;
    case GL_MAP2_NORMAL:
    case GL_MAP2_TEXTURE_COORD_3:
    case GL_MAP2_VERTEX_3:
        return 3;
    case GL_MAP2_COLOR_4:
    case GL_MAP2_TEXTURE_COORD_4:
    case GL_MAP2_VERTEX_4:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetBooleanv_size(GLenum e)
{
    switch (e) {
    case GL_CURRENT_INDEX:
    case GL_CURRENT_RASTER_INDEX:
    case GL_CURRENT_RASTER_POSITION_VALID:
    case GL_CURRENT_RASTER_DISTANCE:
    case GL_POINT_SMOOTH:
    case GL_POINT_SIZE:
    case GL_SMOOTH_POINT_SIZE_GRANULARITY:
    case GL_LINE_SMOOTH:
    case GL_LINE_WIDTH:
    case GL_LINE_WIDTH_GRANULARITY:
    case GL_LINE_STIPPLE:
    case GL_LINE_STIPPLE_PATTERN:
    case GL_LINE_STIPPLE_REPEAT:
    case GL_LIST_MODE:
    case GL_MAX_LIST_NESTING:
    case GL_LIST_BASE:
    case GL_LIST_INDEX:
    case GL_POLYGON_SMOOTH:
    case GL_POLYGON_STIPPLE:
    case GL_EDGE_FLAG:
    case GL_CULL_FACE:
    case GL_CULL_FACE_MODE:
    case GL_FRONT_FACE:
    case GL_LIGHTING:
    case GL_LIGHT_MODEL_LOCAL_VIEWER:
    case GL_LIGHT_MODEL_TWO_SIDE:
    case GL_SHADE_MODEL:
    case GL_COLOR_MATERIAL_FACE:
    case GL_COLOR_MATERIAL_PARAMETER:
    case GL_COLOR_MATERIAL:
    case GL_FOG:
    case GL_FOG_INDEX:
    case GL_FOG_DENSITY:
    case GL_FOG_START:
    case GL_FOG_END:
    case GL_FOG_MODE:
    case GL_DEPTH_TEST:
    case GL_DEPTH_WRITEMASK:
    case GL_DEPTH_CLEAR_VALUE:
    case GL_DEPTH_FUNC:
    case GL_STENCIL_TEST:
    case GL_STENCIL_CLEAR_VALUE:
    case GL_STENCIL_FUNC:
    case GL_STENCIL_VALUE_MASK:
    case GL_STENCIL_FAIL:
    case GL_STENCIL_PASS_DEPTH_FAIL:
    case GL_STENCIL_PASS_DEPTH_PASS:
    case GL_STENCIL_REF:
    case GL_STENCIL_WRITEMASK:
    case GL_MATRIX_MODE:
    case GL_NORMALIZE:
    case GL_MODELVIEW_STACK_DEPTH:
    case GL_PROJECTION_STACK_DEPTH:
    case GL_TEXTURE_STACK_DEPTH:
    case GL_ATTRIB_STACK_DEPTH:
    case GL_CLIENT_ATTRIB_STACK_DEPTH:
    case GL_ALPHA_TEST:
    case GL_ALPHA_TEST_FUNC:
    case GL_ALPHA_TEST_REF:
    case GL_DITHER:
    case GL_BLEND_DST:
    case GL_BLEND_SRC:
    case GL_BLEND:
    case GL_LOGIC_OP_MODE:
    case GL_LOGIC_OP:
    case GL_AUX_BUFFERS:
    case GL_DRAW_BUFFER:
    case GL_READ_BUFFER:
    case GL_SCISSOR_TEST:
    case GL_INDEX_CLEAR_VALUE:
    case GL_INDEX_WRITEMASK:
    case GL_INDEX_MODE:
    case GL_RGBA_MODE:
    case GL_DOUBLEBUFFER:
    case GL_STEREO:
    case GL_RENDER_MODE:
    case GL_PERSPECTIVE_CORRECTION_HINT:
    case GL_POINT_SMOOTH_HINT:
    case GL_LINE_SMOOTH_HINT:
    case GL_POLYGON_SMOOTH_HINT:
    case GL_FOG_HINT:
    case GL_TEXTURE_GEN_S:
    case GL_TEXTURE_GEN_T:
    case GL_TEXTURE_GEN_R:
    case GL_TEXTURE_GEN_Q:
    case GL_PIXEL_MAP_I_TO_I:
    case GL_PIXEL_MAP_I_TO_I_SIZE:
    case GL_PIXEL_MAP_S_TO_S_SIZE:
    case GL_PIXEL_MAP_I_TO_R_SIZE:
    case GL_PIXEL_MAP_I_TO_G_SIZE:
    case GL_PIXEL_MAP_I_TO_B_SIZE:
    case GL_PIXEL_MAP_I_TO_A_SIZE:
    case GL_PIXEL_MAP_R_TO_R_SIZE:
    case GL_PIXEL_MAP_G_TO_G_SIZE:
    case GL_PIXEL_MAP_B_TO_B_SIZE:
    case GL_PIXEL_MAP_A_TO_A_SIZE:
    case GL_UNPACK_SWAP_BYTES:
    case GL_UNPACK_LSB_FIRST:
    case GL_UNPACK_ROW_LENGTH:
    case GL_UNPACK_SKIP_ROWS:
    case GL_UNPACK_SKIP_PIXELS:
    case GL_UNPACK_ALIGNMENT:
    case GL_PACK_SWAP_BYTES:
    case GL_PACK_LSB_FIRST:
    case GL_PACK_ROW_LENGTH:
    case GL_PACK_SKIP_ROWS:
    case GL_PACK_SKIP_PIXELS:
    case GL_PACK_ALIGNMENT:
    case GL_MAP_COLOR:
    case GL_MAP_STENCIL:
    case GL_INDEX_SHIFT:
    case GL_INDEX_OFFSET:
    case GL_RED_SCALE:
    case GL_RED_BIAS:
    case GL_ZOOM_X:
    case GL_ZOOM_Y:
    case GL_GREEN_SCALE:
    case GL_GREEN_BIAS:
    case GL_BLUE_SCALE:
    case GL_BLUE_BIAS:
    case GL_ALPHA_SCALE:
    case GL_ALPHA_BIAS:
    case GL_DEPTH_SCALE:
    case GL_DEPTH_BIAS:
    case GL_MAX_EVAL_ORDER:
    case GL_MAX_LIGHTS:
    case GL_MAX_CLIP_PLANES:
    case GL_MAX_TEXTURE_SIZE:
    case GL_MAX_PIXEL_MAP_TABLE:
    case GL_MAX_ATTRIB_STACK_DEPTH:
    case GL_MAX_MODELVIEW_STACK_DEPTH:
    case GL_MAX_NAME_STACK_DEPTH:
    case GL_MAX_PROJECTION_STACK_DEPTH:
    case GL_MAX_TEXTURE_STACK_DEPTH:
    case GL_MAX_CLIENT_ATTRIB_STACK_DEPTH:
    case GL_SUBPIXEL_BITS:
    case GL_INDEX_BITS:
    case GL_RED_BITS:
    case GL_GREEN_BITS:
    case GL_BLUE_BITS:
    case GL_ALPHA_BITS:
    case GL_DEPTH_BITS:
    case GL_STENCIL_BITS:
    case GL_ACCUM_RED_BITS:
    case GL_ACCUM_GREEN_BITS:
    case GL_ACCUM_BLUE_BITS:
    case GL_ACCUM_ALPHA_BITS:
    case GL_NAME_STACK_DEPTH:
    case GL_AUTO_NORMAL:
    case GL_MAP1_COLOR_4:
    case GL_MAP1_INDEX:
    case GL_MAP1_NORMAL:
    case GL_MAP1_TEXTURE_COORD_1:
    case GL_MAP1_TEXTURE_COORD_2:
    case GL_MAP1_TEXTURE_COORD_3:
    case GL_MAP1_TEXTURE_COORD_4:
    case GL_MAP1_VERTEX_3:
    case GL_MAP1_VERTEX_4:
    case GL_MAP2_COLOR_4:
    case GL_MAP2_INDEX:
    case GL_MAP2_NORMAL:
    case GL_MAP2_TEXTURE_COORD_1:
    case GL_MAP2_TEXTURE_COORD_2:
    case GL_MAP2_TEXTURE_COORD_3:
    case GL_MAP2_TEXTURE_COORD_4:
    case GL_MAP2_VERTEX_3:
    case GL_MAP2_VERTEX_4:
    case GL_MAP1_GRID_SEGMENTS:
    case GL_TEXTURE_1D:
    case GL_TEXTURE_2D:
    case GL_POLYGON_OFFSET_UNITS:
    case GL_CLIP_PLANE0:
    case GL_CLIP_PLANE1:
    case GL_CLIP_PLANE2:
    case GL_CLIP_PLANE3:
    case GL_CLIP_PLANE4:
    case GL_CLIP_PLANE5:
    case GL_LIGHT0:
    case GL_LIGHT1:
    case GL_LIGHT2:
    case GL_LIGHT3:
    case GL_LIGHT4:
    case GL_LIGHT5:
    case GL_LIGHT6:
    case GL_LIGHT7:
    case GL_BLEND_EQUATION:
/*      case GL_BLEND_EQUATION_EXT:*/
    case GL_CONVOLUTION_1D:
    case GL_CONVOLUTION_2D:
    case GL_SEPARABLE_2D:
    case GL_MAX_CONVOLUTION_WIDTH:
/*      case GL_MAX_CONVOLUTION_WIDTH_EXT:*/
    case GL_MAX_CONVOLUTION_HEIGHT:
/*      case GL_MAX_CONVOLUTION_HEIGHT_EXT:*/
    case GL_POST_CONVOLUTION_RED_SCALE:
/*      case GL_POST_CONVOLUTION_RED_SCALE_EXT:*/
    case GL_POST_CONVOLUTION_GREEN_SCALE:
/*      case GL_POST_CONVOLUTION_GREEN_SCALE_EXT:*/
    case GL_POST_CONVOLUTION_BLUE_SCALE:
/*      case GL_POST_CONVOLUTION_BLUE_SCALE_EXT:*/
    case GL_POST_CONVOLUTION_ALPHA_SCALE:
/*      case GL_POST_CONVOLUTION_ALPHA_SCALE_EXT:*/
    case GL_POST_CONVOLUTION_RED_BIAS:
/*      case GL_POST_CONVOLUTION_RED_BIAS_EXT:*/
    case GL_POST_CONVOLUTION_GREEN_BIAS:
/*      case GL_POST_CONVOLUTION_GREEN_BIAS_EXT:*/
    case GL_POST_CONVOLUTION_BLUE_BIAS:
/*      case GL_POST_CONVOLUTION_BLUE_BIAS_EXT:*/
    case GL_POST_CONVOLUTION_ALPHA_BIAS:
/*      case GL_POST_CONVOLUTION_ALPHA_BIAS_EXT:*/
    case GL_HISTOGRAM:
    case GL_MINMAX:
    case GL_POLYGON_OFFSET_FACTOR:
    case GL_RESCALE_NORMAL:
/*      case GL_RESCALE_NORMAL_EXT:*/
    case GL_TEXTURE_BINDING_1D:
    case GL_TEXTURE_BINDING_2D:
    case GL_TEXTURE_BINDING_3D:
    case GL_PACK_SKIP_IMAGES:
    case GL_PACK_IMAGE_HEIGHT:
    case GL_UNPACK_SKIP_IMAGES:
    case GL_UNPACK_IMAGE_HEIGHT:
    case GL_TEXTURE_3D:
    case GL_MAX_3D_TEXTURE_SIZE:
    case GL_VERTEX_ARRAY:
    case GL_NORMAL_ARRAY:
    case GL_COLOR_ARRAY:
    case GL_INDEX_ARRAY:
    case GL_TEXTURE_COORD_ARRAY:
    case GL_EDGE_FLAG_ARRAY:
    case GL_VERTEX_ARRAY_SIZE:
    case GL_VERTEX_ARRAY_TYPE:
    case GL_VERTEX_ARRAY_STRIDE:
    case GL_NORMAL_ARRAY_TYPE:
    case GL_NORMAL_ARRAY_STRIDE:
    case GL_COLOR_ARRAY_SIZE:
    case GL_COLOR_ARRAY_TYPE:
    case GL_COLOR_ARRAY_STRIDE:
    case GL_INDEX_ARRAY_TYPE:
    case GL_INDEX_ARRAY_STRIDE:
    case GL_TEXTURE_COORD_ARRAY_SIZE:
    case GL_TEXTURE_COORD_ARRAY_TYPE:
    case GL_TEXTURE_COORD_ARRAY_STRIDE:
    case GL_EDGE_FLAG_ARRAY_STRIDE:
    case GL_MULTISAMPLE:
/*      case GL_MULTISAMPLE_ARB:*/
    case GL_SAMPLE_ALPHA_TO_COVERAGE:
/*      case GL_SAMPLE_ALPHA_TO_COVERAGE_ARB:*/
    case GL_SAMPLE_ALPHA_TO_ONE:
/*      case GL_SAMPLE_ALPHA_TO_ONE_ARB:*/
    case GL_SAMPLE_COVERAGE:
/*      case GL_SAMPLE_COVERAGE_ARB:*/
    case GL_SAMPLE_BUFFERS:
/*      case GL_SAMPLE_BUFFERS_ARB:*/
    case GL_SAMPLES:
/*      case GL_SAMPLES_ARB:*/
    case GL_SAMPLE_COVERAGE_VALUE:
/*      case GL_SAMPLE_COVERAGE_VALUE_ARB:*/
    case GL_SAMPLE_COVERAGE_INVERT:
/*      case GL_SAMPLE_COVERAGE_INVERT_ARB:*/
    case GL_COLOR_MATRIX_STACK_DEPTH:
    case GL_MAX_COLOR_MATRIX_STACK_DEPTH:
    case GL_POST_COLOR_MATRIX_RED_SCALE:
    case GL_POST_COLOR_MATRIX_GREEN_SCALE:
    case GL_POST_COLOR_MATRIX_BLUE_SCALE:
    case GL_POST_COLOR_MATRIX_ALPHA_SCALE:
    case GL_POST_COLOR_MATRIX_RED_BIAS:
    case GL_POST_COLOR_MATRIX_GREEN_BIAS:
    case GL_POST_COLOR_MATRIX_BLUE_BIAS:
    case GL_POST_COLOR_MATRIX_ALPHA_BIAS:
    case GL_BLEND_DST_RGB:
    case GL_BLEND_SRC_RGB:
    case GL_BLEND_DST_ALPHA:
    case GL_BLEND_SRC_ALPHA:
    case GL_COLOR_TABLE:
    case GL_POST_CONVOLUTION_COLOR_TABLE:
    case GL_POST_COLOR_MATRIX_COLOR_TABLE:
    case GL_MAX_ELEMENTS_VERTICES:
    case GL_MAX_ELEMENTS_INDICES:
    case GL_CLIP_VOLUME_CLIPPING_HINT_EXT:
    case GL_POINT_SIZE_MIN:
    case GL_POINT_SIZE_MAX:
    case GL_POINT_FADE_THRESHOLD_SIZE:
    case GL_OCCLUSION_TEST_HP:
    case GL_OCCLUSION_TEST_RESULT_HP:
    case GL_LIGHT_MODEL_COLOR_CONTROL:
    case GL_DEBUG_NEXT_LOGGED_MESSAGE_LENGTH_ARB:
    case GL_RESET_NOTIFICATION_STRATEGY_ARB:
    case GL_CURRENT_FOG_COORD:
    case GL_FOG_COORDINATE_ARRAY_TYPE:
    case GL_FOG_COORDINATE_ARRAY_STRIDE:
    case GL_FOG_COORD_ARRAY:
    case GL_COLOR_SUM_ARB:
    case GL_SECONDARY_COLOR_ARRAY_SIZE:
    case GL_SECONDARY_COLOR_ARRAY_TYPE:
    case GL_SECONDARY_COLOR_ARRAY_STRIDE:
    case GL_SECONDARY_COLOR_ARRAY:
    case GL_ACTIVE_TEXTURE:
/*      case GL_ACTIVE_TEXTURE_ARB:*/
    case GL_CLIENT_ACTIVE_TEXTURE:
/*      case GL_CLIENT_ACTIVE_TEXTURE_ARB:*/
    case GL_MAX_TEXTURE_UNITS:
/*      case GL_MAX_TEXTURE_UNITS_ARB:*/
    case GL_MAX_RENDERBUFFER_SIZE:
/*      case GL_MAX_RENDERBUFFER_SIZE_EXT:*/
    case GL_TEXTURE_COMPRESSION_HINT:
/*      case GL_TEXTURE_COMPRESSION_HINT_ARB:*/
    case GL_TEXTURE_RECTANGLE_ARB:
/*      case GL_TEXTURE_RECTANGLE_NV:*/
    case GL_TEXTURE_BINDING_RECTANGLE_ARB:
/*      case GL_TEXTURE_BINDING_RECTANGLE_NV:*/
    case GL_MAX_RECTANGLE_TEXTURE_SIZE_ARB:
/*      case GL_MAX_RECTANGLE_TEXTURE_SIZE_NV:*/
    case GL_MAX_TEXTURE_LOD_BIAS:
    case GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT:
    case GL_MAX_SHININESS_NV:
    case GL_MAX_SPOT_EXPONENT_NV:
    case GL_TEXTURE_CUBE_MAP:
/*      case GL_TEXTURE_CUBE_MAP_ARB:*/
    case GL_TEXTURE_BINDING_CUBE_MAP:
/*      case GL_TEXTURE_BINDING_CUBE_MAP_ARB:*/
    case GL_MAX_CUBE_MAP_TEXTURE_SIZE:
/*      case GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB:*/
    case GL_MULTISAMPLE_FILTER_HINT_NV:
    case GL_FOG_DISTANCE_MODE_NV:
    case GL_VERTEX_PROGRAM_ARB:
    case GL_MAX_PROGRAM_MATRIX_STACK_DEPTH_ARB:
    case GL_MAX_PROGRAM_MATRICES_ARB:
    case GL_CURRENT_MATRIX_STACK_DEPTH_ARB:
    case GL_VERTEX_PROGRAM_POINT_SIZE_ARB:
    case GL_VERTEX_PROGRAM_TWO_SIDE_ARB:
    case GL_PROGRAM_ERROR_POSITION_ARB:
    case GL_DEPTH_CLAMP:
/*      case GL_DEPTH_CLAMP_NV:*/
    case GL_NUM_COMPRESSED_TEXTURE_FORMATS:
/*      case GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB:*/
    case GL_MAX_VERTEX_UNITS_ARB:
    case GL_ACTIVE_VERTEX_UNITS_ARB:
    case GL_WEIGHT_SUM_UNITY_ARB:
    case GL_VERTEX_BLEND_ARB:
    case GL_CURRENT_WEIGHT_ARB:
    case GL_WEIGHT_ARRAY_TYPE_ARB:
    case GL_WEIGHT_ARRAY_STRIDE_ARB:
    case GL_WEIGHT_ARRAY_SIZE_ARB:
    case GL_WEIGHT_ARRAY_ARB:
    case GL_PACK_INVERT_MESA:
    case GL_STENCIL_BACK_FUNC_ATI:
    case GL_STENCIL_BACK_FAIL_ATI:
    case GL_STENCIL_BACK_PASS_DEPTH_FAIL_ATI:
    case GL_STENCIL_BACK_PASS_DEPTH_PASS_ATI:
    case GL_FRAGMENT_PROGRAM_ARB:
    case GL_MAX_DRAW_BUFFERS_ARB:
/*      case GL_MAX_DRAW_BUFFERS_ATI:*/
    case GL_DRAW_BUFFER0_ARB:
/*      case GL_DRAW_BUFFER0_ATI:*/
    case GL_DRAW_BUFFER1_ARB:
/*      case GL_DRAW_BUFFER1_ATI:*/
    case GL_DRAW_BUFFER2_ARB:
/*      case GL_DRAW_BUFFER2_ATI:*/
    case GL_DRAW_BUFFER3_ARB:
/*      case GL_DRAW_BUFFER3_ATI:*/
    case GL_DRAW_BUFFER4_ARB:
/*      case GL_DRAW_BUFFER4_ATI:*/
    case GL_DRAW_BUFFER5_ARB:
/*      case GL_DRAW_BUFFER5_ATI:*/
    case GL_DRAW_BUFFER6_ARB:
/*      case GL_DRAW_BUFFER6_ATI:*/
    case GL_DRAW_BUFFER7_ARB:
/*      case GL_DRAW_BUFFER7_ATI:*/
    case GL_DRAW_BUFFER8_ARB:
/*      case GL_DRAW_BUFFER8_ATI:*/
    case GL_DRAW_BUFFER9_ARB:
/*      case GL_DRAW_BUFFER9_ATI:*/
    case GL_DRAW_BUFFER10_ARB:
/*      case GL_DRAW_BUFFER10_ATI:*/
    case GL_DRAW_BUFFER11_ARB:
/*      case GL_DRAW_BUFFER11_ATI:*/
    case GL_DRAW_BUFFER12_ARB:
/*      case GL_DRAW_BUFFER12_ATI:*/
    case GL_DRAW_BUFFER13_ARB:
/*      case GL_DRAW_BUFFER13_ATI:*/
    case GL_DRAW_BUFFER14_ARB:
/*      case GL_DRAW_BUFFER14_ATI:*/
    case GL_DRAW_BUFFER15_ARB:
/*      case GL_DRAW_BUFFER15_ATI:*/
    case GL_BLEND_EQUATION_ALPHA_EXT:
    case GL_MATRIX_PALETTE_ARB:
    case GL_MAX_MATRIX_PALETTE_STACK_DEPTH_ARB:
    case GL_MAX_PALETTE_MATRICES_ARB:
    case GL_CURRENT_PALETTE_MATRIX_ARB:
    case GL_MATRIX_INDEX_ARRAY_ARB:
    case GL_CURRENT_MATRIX_INDEX_ARB:
    case GL_MATRIX_INDEX_ARRAY_SIZE_ARB:
    case GL_MATRIX_INDEX_ARRAY_TYPE_ARB:
    case GL_MATRIX_INDEX_ARRAY_STRIDE_ARB:
    case GL_COMPARE_REF_DEPTH_TO_TEXTURE_EXT:
    case GL_TEXTURE_CUBE_MAP_SEAMLESS:
    case GL_POINT_SPRITE_ARB:
/*      case GL_POINT_SPRITE_NV:*/
    case GL_POINT_SPRITE_R_MODE_NV:
    case GL_MAX_VERTEX_ATTRIBS_ARB:
    case GL_MAX_TEXTURE_COORDS_ARB:
    case GL_MAX_TEXTURE_IMAGE_UNITS_ARB:
    case GL_DEPTH_BOUNDS_TEST_EXT:
    case GL_ARRAY_BUFFER_BINDING_ARB:
    case GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB:
    case GL_VERTEX_ARRAY_BUFFER_BINDING_ARB:
    case GL_NORMAL_ARRAY_BUFFER_BINDING_ARB:
    case GL_COLOR_ARRAY_BUFFER_BINDING_ARB:
    case GL_INDEX_ARRAY_BUFFER_BINDING_ARB:
    case GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB:
    case GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB:
    case GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB:
    case GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB:
    case GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB:
    case GL_MAX_ARRAY_TEXTURE_LAYERS_EXT:
    case GL_STENCIL_TEST_TWO_SIDE_EXT:
    case GL_ACTIVE_STENCIL_FACE_EXT:
    case GL_SAMPLER_BINDING:
    case GL_TEXTURE_BINDING_1D_ARRAY_EXT:
    case GL_TEXTURE_BINDING_2D_ARRAY_EXT:
    case GL_FRAMEBUFFER_BINDING:
/*      case GL_DRAW_FRAMEBUFFER_BINDING_EXT:*/
    case GL_RENDERBUFFER_BINDING:
/*      case GL_RENDERBUFFER_BINDING_EXT:*/
    case GL_READ_FRAMEBUFFER_BINDING:
/*      case GL_READ_FRAMEBUFFER_BINDING_EXT:*/
    case GL_MAX_COLOR_ATTACHMENTS:
/*      case GL_MAX_COLOR_ATTACHMENTS_EXT:*/
    case GL_MAX_SAMPLES:
/*      case GL_MAX_SAMPLES_EXT:*/
    case GL_MAX_SERVER_WAIT_TIMEOUT:
    case GL_MAX_DEBUG_MESSAGE_LENGTH_ARB:
    case GL_MAX_DEBUG_LOGGED_MESSAGES_ARB:
    case GL_DEBUG_LOGGED_MESSAGES_ARB:
    case GL_RASTER_POSITION_UNCLIPPED_IBM:
        return 1;
    case GL_SMOOTH_POINT_SIZE_RANGE:
    case GL_LINE_WIDTH_RANGE:
    case GL_POLYGON_MODE:
    case GL_DEPTH_RANGE:
    case GL_MAX_VIEWPORT_DIMS:
    case GL_MAP1_GRID_DOMAIN:
    case GL_MAP2_GRID_SEGMENTS:
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
    case GL_DEPTH_BOUNDS_EXT:
        return 2;
    case GL_CURRENT_NORMAL:
    case GL_POINT_DISTANCE_ATTENUATION:
        return 3;
    case GL_CURRENT_COLOR:
    case GL_CURRENT_TEXTURE_COORDS:
    case GL_CURRENT_RASTER_COLOR:
    case GL_CURRENT_RASTER_TEXTURE_COORDS:
    case GL_CURRENT_RASTER_POSITION:
    case GL_LIGHT_MODEL_AMBIENT:
    case GL_FOG_COLOR:
    case GL_ACCUM_CLEAR_VALUE:
    case GL_VIEWPORT:
    case GL_SCISSOR_BOX:
    case GL_COLOR_CLEAR_VALUE:
    case GL_COLOR_WRITEMASK:
    case GL_MAP2_GRID_DOMAIN:
    case GL_BLEND_COLOR:
/*      case GL_BLEND_COLOR_EXT:*/
    case GL_CURRENT_SECONDARY_COLOR:
        return 4;
    case GL_MODELVIEW_MATRIX:
    case GL_PROJECTION_MATRIX:
    case GL_TEXTURE_MATRIX:
    case GL_MODELVIEW0_ARB:
    case GL_COLOR_MATRIX:
    case GL_MODELVIEW1_ARB:
    case GL_CURRENT_MATRIX_ARB:
    case GL_MODELVIEW2_ARB:
    case GL_MODELVIEW3_ARB:
    case GL_MODELVIEW4_ARB:
    case GL_MODELVIEW5_ARB:
    case GL_MODELVIEW6_ARB:
    case GL_MODELVIEW7_ARB:
    case GL_MODELVIEW8_ARB:
    case GL_MODELVIEW9_ARB:
    case GL_MODELVIEW10_ARB:
    case GL_MODELVIEW11_ARB:
    case GL_MODELVIEW12_ARB:
    case GL_MODELVIEW13_ARB:
    case GL_MODELVIEW14_ARB:
    case GL_MODELVIEW15_ARB:
    case GL_MODELVIEW16_ARB:
    case GL_MODELVIEW17_ARB:
    case GL_MODELVIEW18_ARB:
    case GL_MODELVIEW19_ARB:
    case GL_MODELVIEW20_ARB:
    case GL_MODELVIEW21_ARB:
    case GL_MODELVIEW22_ARB:
    case GL_MODELVIEW23_ARB:
    case GL_MODELVIEW24_ARB:
    case GL_MODELVIEW25_ARB:
    case GL_MODELVIEW26_ARB:
    case GL_MODELVIEW27_ARB:
    case GL_MODELVIEW28_ARB:
    case GL_MODELVIEW29_ARB:
    case GL_MODELVIEW30_ARB:
    case GL_MODELVIEW31_ARB:
    case GL_TRANSPOSE_CURRENT_MATRIX_ARB:
        return 16;
    case GL_FOG_COORDINATE_SOURCE:
    case GL_COMPRESSED_TEXTURE_FORMATS:
    case GL_RGBA_INTEGER_MODE_EXT:
        return __glGetBooleanv_variable_size(e);
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetTexParameterfv_size(GLenum e)
{
    switch (e) {
    case GL_TEXTURE_MAG_FILTER:
    case GL_TEXTURE_MIN_FILTER:
    case GL_TEXTURE_WRAP_S:
    case GL_TEXTURE_WRAP_T:
    case GL_TEXTURE_PRIORITY:
    case GL_TEXTURE_RESIDENT:
    case GL_TEXTURE_WRAP_R:
    case GL_TEXTURE_COMPARE_FAIL_VALUE_ARB:
/*      case GL_SHADOW_AMBIENT_SGIX:*/
    case GL_TEXTURE_MIN_LOD:
    case GL_TEXTURE_MAX_LOD:
    case GL_TEXTURE_BASE_LEVEL:
    case GL_TEXTURE_MAX_LEVEL:
    case GL_TEXTURE_CLIPMAP_FRAME_SGIX:
    case GL_TEXTURE_LOD_BIAS_S_SGIX:
    case GL_TEXTURE_LOD_BIAS_T_SGIX:
    case GL_TEXTURE_LOD_BIAS_R_SGIX:
    case GL_GENERATE_MIPMAP:
/*      case GL_GENERATE_MIPMAP_SGIS:*/
    case GL_TEXTURE_COMPARE_SGIX:
    case GL_TEXTURE_COMPARE_OPERATOR_SGIX:
    case GL_TEXTURE_MAX_CLAMP_S_SGIX:
    case GL_TEXTURE_MAX_CLAMP_T_SGIX:
    case GL_TEXTURE_MAX_CLAMP_R_SGIX:
    case GL_TEXTURE_MAX_ANISOTROPY_EXT:
    case GL_TEXTURE_LOD_BIAS:
/*      case GL_TEXTURE_LOD_BIAS_EXT:*/
    case GL_TEXTURE_RANGE_LENGTH_APPLE:
    case GL_TEXTURE_STORAGE_HINT_APPLE:
    case GL_DEPTH_TEXTURE_MODE:
/*      case GL_DEPTH_TEXTURE_MODE_ARB:*/
    case GL_TEXTURE_COMPARE_MODE:
/*      case GL_TEXTURE_COMPARE_MODE_ARB:*/
    case GL_TEXTURE_COMPARE_FUNC:
/*      case GL_TEXTURE_COMPARE_FUNC_ARB:*/
    case GL_TEXTURE_UNSIGNED_REMAP_MODE_NV:
        return 1;
    case GL_TEXTURE_CLIPMAP_CENTER_SGIX:
    case GL_TEXTURE_CLIPMAP_OFFSET_SGIX:
        return 2;
    case GL_TEXTURE_CLIPMAP_VIRTUAL_DEPTH_SGIX:
        return 3;
    case GL_TEXTURE_BORDER_COLOR:
    case GL_POST_TEXTURE_FILTER_BIAS_SGIX:
    case GL_POST_TEXTURE_FILTER_SCALE_SGIX:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetTexLevelParameterfv_size(GLenum e)
{
    switch (e) {
    case GL_TEXTURE_WIDTH:
    case GL_TEXTURE_HEIGHT:
    case GL_TEXTURE_COMPONENTS:
    case GL_TEXTURE_BORDER:
    case GL_TEXTURE_RED_SIZE:
/*      case GL_TEXTURE_RED_SIZE_EXT:*/
    case GL_TEXTURE_GREEN_SIZE:
/*      case GL_TEXTURE_GREEN_SIZE_EXT:*/
    case GL_TEXTURE_BLUE_SIZE:
/*      case GL_TEXTURE_BLUE_SIZE_EXT:*/
    case GL_TEXTURE_ALPHA_SIZE:
/*      case GL_TEXTURE_ALPHA_SIZE_EXT:*/
    case GL_TEXTURE_LUMINANCE_SIZE:
/*      case GL_TEXTURE_LUMINANCE_SIZE_EXT:*/
    case GL_TEXTURE_INTENSITY_SIZE:
/*      case GL_TEXTURE_INTENSITY_SIZE_EXT:*/
    case GL_TEXTURE_DEPTH:
    case GL_TEXTURE_INDEX_SIZE_EXT:
    case GL_TEXTURE_COMPRESSED_IMAGE_SIZE:
/*      case GL_TEXTURE_COMPRESSED_IMAGE_SIZE_ARB:*/
    case GL_TEXTURE_COMPRESSED:
/*      case GL_TEXTURE_COMPRESSED_ARB:*/
    case GL_TEXTURE_DEPTH_SIZE:
/*      case GL_TEXTURE_DEPTH_SIZE_ARB:*/
    case GL_TEXTURE_STENCIL_SIZE:
/*      case GL_TEXTURE_STENCIL_SIZE_EXT:*/
        return 1;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetPointerv_size(GLenum e)
{
    switch (e) {
    case GL_DEBUG_CALLBACK_FUNCTION_ARB:
    case GL_DEBUG_CALLBACK_USER_PARAM_ARB:
        return 1;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glColorTableParameterfv_size(GLenum e)
{
    switch (e) {
    case GL_COLOR_TABLE_SCALE:
    case GL_COLOR_TABLE_BIAS:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetColorTableParameterfv_size(GLenum e)
{
    switch (e) {
    case GL_COLOR_TABLE_FORMAT:
/*      case GL_COLOR_TABLE_FORMAT_EXT:*/
    case GL_COLOR_TABLE_WIDTH:
/*      case GL_COLOR_TABLE_WIDTH_EXT:*/
    case GL_COLOR_TABLE_RED_SIZE:
/*      case GL_COLOR_TABLE_RED_SIZE_EXT:*/
    case GL_COLOR_TABLE_GREEN_SIZE:
/*      case GL_COLOR_TABLE_GREEN_SIZE_EXT:*/
    case GL_COLOR_TABLE_BLUE_SIZE:
/*      case GL_COLOR_TABLE_BLUE_SIZE_EXT:*/
    case GL_COLOR_TABLE_ALPHA_SIZE:
/*      case GL_COLOR_TABLE_ALPHA_SIZE_EXT:*/
    case GL_COLOR_TABLE_LUMINANCE_SIZE:
/*      case GL_COLOR_TABLE_LUMINANCE_SIZE_EXT:*/
    case GL_COLOR_TABLE_INTENSITY_SIZE:
/*      case GL_COLOR_TABLE_INTENSITY_SIZE_EXT:*/
        return 1;
    case GL_COLOR_TABLE_SCALE:
    case GL_COLOR_TABLE_BIAS:
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glConvolutionParameterfv_size(GLenum e)
{
    switch (e) {
    case GL_CONVOLUTION_BORDER_MODE:
/*      case GL_CONVOLUTION_BORDER_MODE_EXT:*/
        return 1;
    case GL_CONVOLUTION_FILTER_SCALE:
/*      case GL_CONVOLUTION_FILTER_SCALE_EXT:*/
    case GL_CONVOLUTION_FILTER_BIAS:
/*      case GL_CONVOLUTION_FILTER_BIAS_EXT:*/
    case GL_CONVOLUTION_BORDER_COLOR:
/*      case GL_CONVOLUTION_BORDER_COLOR_HP:*/
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetConvolutionParameterfv_size(GLenum e)
{
    switch (e) {
    case GL_CONVOLUTION_BORDER_MODE:
/*      case GL_CONVOLUTION_BORDER_MODE_EXT:*/
    case GL_CONVOLUTION_FORMAT:
/*      case GL_CONVOLUTION_FORMAT_EXT:*/
    case GL_CONVOLUTION_WIDTH:
/*      case GL_CONVOLUTION_WIDTH_EXT:*/
    case GL_CONVOLUTION_HEIGHT:
/*      case GL_CONVOLUTION_HEIGHT_EXT:*/
    case GL_MAX_CONVOLUTION_WIDTH:
/*      case GL_MAX_CONVOLUTION_WIDTH_EXT:*/
    case GL_MAX_CONVOLUTION_HEIGHT:
/*      case GL_MAX_CONVOLUTION_HEIGHT_EXT:*/
        return 1;
    case GL_CONVOLUTION_FILTER_SCALE:
/*      case GL_CONVOLUTION_FILTER_SCALE_EXT:*/
    case GL_CONVOLUTION_FILTER_BIAS:
/*      case GL_CONVOLUTION_FILTER_BIAS_EXT:*/
    case GL_CONVOLUTION_BORDER_COLOR:
/*      case GL_CONVOLUTION_BORDER_COLOR_HP:*/
        return 4;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetHistogramParameterfv_size(GLenum e)
{
    switch (e) {
    case GL_HISTOGRAM_WIDTH:
    case GL_HISTOGRAM_FORMAT:
    case GL_HISTOGRAM_RED_SIZE:
    case GL_HISTOGRAM_GREEN_SIZE:
    case GL_HISTOGRAM_BLUE_SIZE:
    case GL_HISTOGRAM_ALPHA_SIZE:
    case GL_HISTOGRAM_LUMINANCE_SIZE:
    case GL_HISTOGRAM_SINK:
        return 1;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetMinmaxParameterfv_size(GLenum e)
{
    switch (e) {
    case GL_MINMAX_FORMAT:
    case GL_MINMAX_SINK:
        return 1;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glPointParameterfv_size(GLenum e)
{
    switch (e) {
    case GL_POINT_SIZE_MIN:
/*      case GL_POINT_SIZE_MIN_ARB:*/
/*      case GL_POINT_SIZE_MIN_SGIS:*/
    case GL_POINT_SIZE_MAX:
/*      case GL_POINT_SIZE_MAX_ARB:*/
/*      case GL_POINT_SIZE_MAX_SGIS:*/
    case GL_POINT_FADE_THRESHOLD_SIZE:
/*      case GL_POINT_FADE_THRESHOLD_SIZE_ARB:*/
/*      case GL_POINT_FADE_THRESHOLD_SIZE_SGIS:*/
    case GL_POINT_SPRITE_R_MODE_NV:
    case GL_POINT_SPRITE_COORD_ORIGIN:
        return 1;
    case GL_POINT_DISTANCE_ATTENUATION:
/*      case GL_POINT_DISTANCE_ATTENUATION_ARB:*/
/*      case GL_POINT_DISTANCE_ATTENUATION_SGIS:*/
        return 3;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetQueryObjectiv_size(GLenum e)
{
    switch (e) {
    case GL_QUERY_RESULT_ARB:
    case GL_QUERY_RESULT_AVAILABLE_ARB:
        return 1;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetQueryiv_size(GLenum e)
{
    switch (e) {
    case GL_QUERY_COUNTER_BITS_ARB:
    case GL_CURRENT_QUERY_ARB:
    case GL_ANY_SAMPLES_PASSED:
        return 1;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetProgramivARB_size(GLenum e)
{
    switch (e) {
    case GL_PROGRAM_LENGTH_ARB:
    case GL_PROGRAM_BINDING_ARB:
    case GL_PROGRAM_ALU_INSTRUCTIONS_ARB:
    case GL_PROGRAM_TEX_INSTRUCTIONS_ARB:
    case GL_PROGRAM_TEX_INDIRECTIONS_ARB:
    case GL_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
    case GL_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
    case GL_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
    case GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB:
    case GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB:
    case GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB:
    case GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
    case GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
    case GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
    case GL_PROGRAM_FORMAT_ARB:
    case GL_PROGRAM_INSTRUCTIONS_ARB:
    case GL_MAX_PROGRAM_INSTRUCTIONS_ARB:
    case GL_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
    case GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
    case GL_PROGRAM_TEMPORARIES_ARB:
    case GL_MAX_PROGRAM_TEMPORARIES_ARB:
    case GL_PROGRAM_NATIVE_TEMPORARIES_ARB:
    case GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB:
    case GL_PROGRAM_PARAMETERS_ARB:
    case GL_MAX_PROGRAM_PARAMETERS_ARB:
    case GL_PROGRAM_NATIVE_PARAMETERS_ARB:
    case GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB:
    case GL_PROGRAM_ATTRIBS_ARB:
    case GL_MAX_PROGRAM_ATTRIBS_ARB:
    case GL_PROGRAM_NATIVE_ATTRIBS_ARB:
    case GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB:
    case GL_PROGRAM_ADDRESS_REGISTERS_ARB:
    case GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB:
    case GL_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
    case GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
    case GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB:
    case GL_MAX_PROGRAM_ENV_PARAMETERS_ARB:
    case GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB:
    case GL_MAX_PROGRAM_EXEC_INSTRUCTIONS_NV:
    case GL_MAX_PROGRAM_CALL_DEPTH_NV:
    case GL_MAX_PROGRAM_IF_DEPTH_NV:
    case GL_MAX_PROGRAM_LOOP_DEPTH_NV:
    case GL_MAX_PROGRAM_LOOP_COUNT_NV:
        return 1;
    default:
        return 0;
    }
}

_X_INTERNAL PURE FASTCALL GLint
__glGetFramebufferAttachmentParameteriv_size(GLenum e)
{
    switch (e) {
    case GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING:
    case GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE:
    case GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE:
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
/*      case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT:*/
    case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
/*      case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT:*/
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
/*      case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT:*/
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
/*      case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT:*/
    case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT:
        return 1;
    default:
        return 0;
    }
}

ALIAS(Fogiv, Fogfv)
    ALIAS(Lightiv, Lightfv)
    ALIAS(LightModeliv, LightModelfv)
    ALIAS(Materialiv, Materialfv)
    ALIAS(TexParameteriv, TexParameterfv)
    ALIAS(TexEnviv, TexEnvfv)
    ALIAS(TexGenfv, TexGendv)
    ALIAS(TexGeniv, TexGendv)
    ALIAS(Map1f, Map1d)
    ALIAS(Map2f, Map2d)
    ALIAS(GetDoublev, GetBooleanv)
    ALIAS(GetFloatv, GetBooleanv)
    ALIAS(GetIntegerv, GetBooleanv)
    ALIAS(GetLightfv, Lightfv)
    ALIAS(GetLightiv, Lightfv)
    ALIAS(GetMaterialfv, Materialfv)
    ALIAS(GetMaterialiv, Materialfv)
    ALIAS(GetTexEnvfv, TexEnvfv)
    ALIAS(GetTexEnviv, TexEnvfv)
    ALIAS(GetTexGendv, TexGendv)
    ALIAS(GetTexGenfv, TexGendv)
    ALIAS(GetTexGeniv, TexGendv)
    ALIAS(GetTexParameteriv, GetTexParameterfv)
    ALIAS(GetTexLevelParameteriv, GetTexLevelParameterfv)
    ALIAS(ColorTableParameteriv, ColorTableParameterfv)
    ALIAS(GetColorTableParameteriv, GetColorTableParameterfv)
    ALIAS(ConvolutionParameteriv, ConvolutionParameterfv)
    ALIAS(GetConvolutionParameteriv, GetConvolutionParameterfv)
    ALIAS(GetHistogramParameteriv, GetHistogramParameterfv)
    ALIAS(GetMinmaxParameteriv, GetMinmaxParameterfv)
    ALIAS(PointParameteriv, PointParameterfv)
    ALIAS(GetQueryObjectuiv, GetQueryObjectiv)
#undef PURE
#undef FASTCALL
