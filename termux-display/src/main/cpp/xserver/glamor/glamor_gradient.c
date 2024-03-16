/*
 * Copyright © 2009 Intel Corporation
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
 *    Junyan He <junyan.he@linux.intel.com>
 *
 */

/** @file glamor_gradient.c
 *
 * Gradient acceleration implementation
 */

#include "glamor_priv.h"

#define LINEAR_SMALL_STOPS (6 + 2)
#define LINEAR_LARGE_STOPS (16 + 2)

#define RADIAL_SMALL_STOPS (6 + 2)
#define RADIAL_LARGE_STOPS (16 + 2)

static char *
_glamor_create_getcolor_fs_source(ScreenPtr screen, int stops_count,
                                  int use_array)
{
    char *gradient_fs = NULL;

#define gradient_fs_getcolor\
	    GLAMOR_DEFAULT_PRECISION\
	    "uniform int n_stop;\n"\
	    "uniform float stops[%d];\n"\
	    "uniform vec4 stop_colors[%d];\n"\
	    "vec4 get_color(float stop_len)\n"\
	    "{\n"\
	    "    int i = 0;\n"\
	    "    vec4 stop_color_before;\n"\
	    "    vec4 gradient_color;\n"\
	    "    float stop_delta;\n"\
	    "    float percentage; \n"\
	    "    \n"\
	    "    if(stop_len < stops[0])\n"\
	    "        return vec4(0.0, 0.0, 0.0, 0.0); \n"\
	    "    for(i = 1; i < n_stop; i++) {\n"\
	    "        if(stop_len < stops[i])\n"\
	    "            break; \n"\
	    "    }\n"\
	    "    if(i == n_stop)\n"\
	    "        return vec4(0.0, 0.0, 0.0, 0.0); \n"\
	    "    \n"\
	    "    stop_color_before = stop_colors[i-1];\n"\
	    "    stop_delta = stops[i] - stops[i-1];\n"\
	    "    if(stop_delta > 2.0)\n"\
	    "        percentage = 0.0;\n" /*For comply with pixman, walker->stepper overflow.*/\
	    "    else if(stop_delta < 0.000001)\n"\
	    "        percentage = 0.0;\n"\
	    "    else \n"\
	    "        percentage = (stop_len - stops[i-1])/stop_delta;\n"\
	    "    \n"\
	    "    gradient_color = stop_color_before;\n"\
	    "    if(percentage != 0.0)\n"\
	    "        gradient_color += (stop_colors[i] - gradient_color)*percentage;\n"\
	    "    return vec4(gradient_color.rgb * gradient_color.a, gradient_color.a);\n"\
	    "}\n"

    /* Because the array access for shader is very slow, the performance is very low
       if use array. So use global uniform to replace for it if the number of n_stops is small. */
    const char *gradient_fs_getcolor_no_array =
        GLAMOR_DEFAULT_PRECISION
        "uniform int n_stop;\n"
        "uniform float stop0;\n"
        "uniform float stop1;\n"
        "uniform float stop2;\n"
        "uniform float stop3;\n"
        "uniform float stop4;\n"
        "uniform float stop5;\n"
        "uniform float stop6;\n"
        "uniform float stop7;\n"
        "uniform vec4 stop_color0;\n"
        "uniform vec4 stop_color1;\n"
        "uniform vec4 stop_color2;\n"
        "uniform vec4 stop_color3;\n"
        "uniform vec4 stop_color4;\n"
        "uniform vec4 stop_color5;\n"
        "uniform vec4 stop_color6;\n"
        "uniform vec4 stop_color7;\n"
        "\n"
        "vec4 get_color(float stop_len)\n"
        "{\n"
        "    vec4 stop_color_before;\n"
        "    vec4 stop_color_after;\n"
        "    vec4 gradient_color;\n"
        "    float stop_before;\n"
        "    float stop_delta;\n"
        "    float percentage; \n"
        "    \n"
        "    if((stop_len < stop0) && (n_stop >= 1)) {\n"
        "        stop_color_before = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "        stop_delta = 0.0;\n"
        "    } else if((stop_len < stop1) && (n_stop >= 2)) {\n"
        "        stop_color_before = stop_color0;\n"
        "        stop_color_after = stop_color1;\n"
        "        stop_before = stop0;\n"
        "        stop_delta = stop1 - stop0;\n"
        "    } else if((stop_len < stop2) && (n_stop >= 3)) {\n"
        "        stop_color_before = stop_color1;\n"
        "        stop_color_after = stop_color2;\n"
        "        stop_before = stop1;\n"
        "        stop_delta = stop2 - stop1;\n"
        "    } else if((stop_len < stop3) && (n_stop >= 4)){\n"
        "        stop_color_before = stop_color2;\n"
        "        stop_color_after = stop_color3;\n"
        "        stop_before = stop2;\n"
        "        stop_delta = stop3 - stop2;\n"
        "    } else if((stop_len < stop4) && (n_stop >= 5)){\n"
        "        stop_color_before = stop_color3;\n"
        "        stop_color_after = stop_color4;\n"
        "        stop_before = stop3;\n"
        "        stop_delta = stop4 - stop3;\n"
        "    } else if((stop_len < stop5) && (n_stop >= 6)){\n"
        "        stop_color_before = stop_color4;\n"
        "        stop_color_after = stop_color5;\n"
        "        stop_before = stop4;\n"
        "        stop_delta = stop5 - stop4;\n"
        "    } else if((stop_len < stop6) && (n_stop >= 7)){\n"
        "        stop_color_before = stop_color5;\n"
        "        stop_color_after = stop_color6;\n"
        "        stop_before = stop5;\n"
        "        stop_delta = stop6 - stop5;\n"
        "    } else if((stop_len < stop7) && (n_stop >= 8)){\n"
        "        stop_color_before = stop_color6;\n"
        "        stop_color_after = stop_color7;\n"
        "        stop_before = stop6;\n"
        "        stop_delta = stop7 - stop6;\n"
        "    } else {\n"
        "        stop_color_before = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "        stop_delta = 0.0;\n"
        "    }\n"
        "    if(stop_delta > 2.0)\n"
        "        percentage = 0.0;\n" //For comply with pixman, walker->stepper overflow.
        "    else if(stop_delta < 0.000001)\n"
        "        percentage = 0.0;\n"
        "    else\n"
        "        percentage = (stop_len - stop_before)/stop_delta;\n"
        "    \n"
        "    gradient_color = stop_color_before;\n"
        "    if(percentage != 0.0)\n"
        "        gradient_color += (stop_color_after - gradient_color)*percentage;\n"
        "    return vec4(gradient_color.rgb * gradient_color.a, gradient_color.a);\n"
        "}\n";

    if (use_array) {
        XNFasprintf(&gradient_fs,
                    gradient_fs_getcolor, stops_count, stops_count);
        return gradient_fs;
    }
    else {
        return XNFstrdup(gradient_fs_getcolor_no_array);
    }
}

static void
_glamor_create_radial_gradient_program(ScreenPtr screen, int stops_count,
                                       int dyn_gen)
{
    glamor_screen_private *glamor_priv;
    int index;

    GLint gradient_prog = 0;
    char *gradient_fs = NULL;
    GLint fs_prog, vs_prog;

    const char *gradient_vs =
        GLAMOR_DEFAULT_PRECISION
        "attribute vec4 v_position;\n"
        "attribute vec4 v_texcoord;\n"
        "varying vec2 source_texture;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = v_position;\n"
        "    source_texture = v_texcoord.xy;\n"
        "}\n";

    /*
     *     Refer to pixman radial gradient.
     *
     *     The problem is given the two circles of c1 and c2 with the radius of r1 and
     *     r1, we need to calculate the t, which is used to do interpolate with stops,
     *     using the fomula:
     *     length((1-t)*c1 + t*c2 - p) = (1-t)*r1 + t*r2
     *     expand the fomula with xy coond, get the following:
     *     sqrt(sqr((1-t)*c1.x + t*c2.x - p.x) + sqr((1-t)*c1.y + t*c2.y - p.y))
     *           = (1-t)r1 + t*r2
     *     <====> At*t- 2Bt + C = 0
     *     where A = sqr(c2.x - c1.x) + sqr(c2.y - c1.y) - sqr(r2 -r1)
     *           B = (p.x - c1.x)*(c2.x - c1.x) + (p.y - c1.y)*(c2.y - c1.y) + r1*(r2 -r1)
     *           C = sqr(p.x - c1.x) + sqr(p.y - c1.y) - r1*r1
     *
     *     solve the fomula and we get the result of
     *     t = (B + sqrt(B*B - A*C)) / A  or
     *     t = (B - sqrt(B*B - A*C)) / A  (quadratic equation have two solutions)
     *
     *     The solution we are going to prefer is the bigger one, unless the
     *     radius associated to it is negative (or it falls outside the valid t range)
     */

#define gradient_radial_fs_template\
	    GLAMOR_DEFAULT_PRECISION\
	    "uniform mat3 transform_mat;\n"\
	    "uniform int repeat_type;\n"\
	    "uniform float A_value;\n"\
	    "uniform vec2 c1;\n"\
	    "uniform float r1;\n"\
	    "uniform vec2 c2;\n"\
	    "uniform float r2;\n"\
	    "varying vec2 source_texture;\n"\
	    "\n"\
	    "vec4 get_color(float stop_len);\n"\
	    "\n"\
	    "int t_invalid;\n"\
	    "\n"\
	    "float get_stop_len()\n"\
	    "{\n"\
	    "    float t = 0.0;\n"\
	    "    float sqrt_value;\n"\
	    "    t_invalid = 0;\n"\
	    "    \n"\
	    "    vec3 tmp = vec3(source_texture.x, source_texture.y, 1.0);\n"\
	    "    vec3 source_texture_trans = transform_mat * tmp;\n"\
	    "    source_texture_trans.xy = source_texture_trans.xy/source_texture_trans.z;\n"\
	    "    float B_value = (source_texture_trans.x - c1.x) * (c2.x - c1.x)\n"\
	    "                     + (source_texture_trans.y - c1.y) * (c2.y - c1.y)\n"\
	    "                     + r1 * (r2 - r1);\n"\
	    "    float C_value = (source_texture_trans.x - c1.x) * (source_texture_trans.x - c1.x)\n"\
	    "                     + (source_texture_trans.y - c1.y) * (source_texture_trans.y - c1.y)\n"\
	    "                     - r1*r1;\n"\
	    "    if(abs(A_value) < 0.00001) {\n"\
	    "        if(B_value == 0.0) {\n"\
	    "            t_invalid = 1;\n"\
	    "            return t;\n"\
	    "        }\n"\
	    "        t = 0.5 * C_value / B_value;"\
	    "    } else {\n"\
	    "        sqrt_value = B_value * B_value - A_value * C_value;\n"\
	    "        if(sqrt_value < 0.0) {\n"\
	    "            t_invalid = 1;\n"\
	    "            return t;\n"\
	    "        }\n"\
	    "        sqrt_value = sqrt(sqrt_value);\n"\
	    "        t = (B_value + sqrt_value) / A_value;\n"\
	    "    }\n"\
	    "    if(repeat_type == %d) {\n" /* RepeatNone case. */\
	    "        if((t <= 0.0) || (t > 1.0))\n"\
	    /*           try another if first one invalid*/\
	    "            t = (B_value - sqrt_value) / A_value;\n"\
	    "        \n"\
	    "        if((t <= 0.0) || (t > 1.0)) {\n" /*still invalid, return.*/\
	    "            t_invalid = 1;\n"\
	    "            return t;\n"\
	    "        }\n"\
	    "    } else {\n"\
	    "        if(t * (r2 - r1) <= -1.0 * r1)\n"\
	    /*           try another if first one invalid*/\
	    "            t = (B_value - sqrt_value) / A_value;\n"\
	    "        \n"\
	    "        if(t * (r2 -r1) <= -1.0 * r1) {\n" /*still invalid, return.*/\
	    "            t_invalid = 1;\n"\
	    "            return t;\n"\
	    "        }\n"\
	    "    }\n"\
	    "    \n"\
	    "    if(repeat_type == %d){\n" /* repeat normal*/\
	    "        t = fract(t);\n"\
	    "    }\n"\
	    "    \n"\
	    "    if(repeat_type == %d) {\n" /* repeat reflect*/\
	    "        t = abs(fract(t * 0.5 + 0.5) * 2.0 - 1.0);\n"\
	    "    }\n"\
	    "    \n"\
	    "    return t;\n"\
	    "}\n"\
	    "\n"\
	    "void main()\n"\
	    "{\n"\
	    "    float stop_len = get_stop_len();\n"\
	    "    if(t_invalid == 1) {\n"\
	    "        gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n"\
	    "    } else {\n"\
	    "        gl_FragColor = get_color(stop_len);\n"\
	    "    }\n"\
	    "}\n"\
	    "\n"\
            "%s\n" /* fs_getcolor_source */
    char *fs_getcolor_source;

    glamor_priv = glamor_get_screen_private(screen);

    if ((glamor_priv->radial_max_nstops >= stops_count) && (dyn_gen)) {
        /* Very Good, not to generate again. */
        return;
    }

    glamor_make_current(glamor_priv);

    if (dyn_gen && glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][2]) {
        glDeleteProgram(glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][2]);
        glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][2] = 0;
    }

    gradient_prog = glCreateProgram();

    vs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER, gradient_vs);

    fs_getcolor_source =
        _glamor_create_getcolor_fs_source(screen, stops_count,
                                          (stops_count > 0));

    XNFasprintf(&gradient_fs,
                gradient_radial_fs_template,
                PIXMAN_REPEAT_NONE, PIXMAN_REPEAT_NORMAL,
                PIXMAN_REPEAT_REFLECT,
                fs_getcolor_source);

    fs_prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER, gradient_fs);

    free(gradient_fs);
    free(fs_getcolor_source);

    glAttachShader(gradient_prog, vs_prog);
    glAttachShader(gradient_prog, fs_prog);
    glDeleteShader(vs_prog);
    glDeleteShader(fs_prog);

    glBindAttribLocation(gradient_prog, GLAMOR_VERTEX_POS, "v_position");
    glBindAttribLocation(gradient_prog, GLAMOR_VERTEX_SOURCE, "v_texcoord");

    glamor_link_glsl_prog(screen, gradient_prog, "radial gradient");

    if (dyn_gen) {
        index = 2;
        glamor_priv->radial_max_nstops = stops_count;
    }
    else if (stops_count) {
        index = 1;
    }
    else {
        index = 0;
    }

    glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][index] = gradient_prog;
}

static void
_glamor_create_linear_gradient_program(ScreenPtr screen, int stops_count,
                                       int dyn_gen)
{
    glamor_screen_private *glamor_priv;

    int index = 0;
    GLint gradient_prog = 0;
    char *gradient_fs = NULL;
    GLint fs_prog, vs_prog;

    const char *gradient_vs =
        GLAMOR_DEFAULT_PRECISION
        "attribute vec4 v_position;\n"
        "attribute vec4 v_texcoord;\n"
        "varying vec2 source_texture;\n"
        "\n"
        "void main()\n"
        "{\n"
        "    gl_Position = v_position;\n"
        "    source_texture = v_texcoord.xy;\n"
        "}\n";

    /*
     *                                      |
     *                                      |\
     *                                      | \
     *                                      |  \
     *                                      |   \
     *                                      |\   \
     *                                      | \   \
     *     cos_val =                        |\ p1d \   /
     *      sqrt(1/(slope*slope+1.0))  ------>\ \   \ /
     *                                      |  \ \   \
     *                                      |   \ \ / \
     *                                      |    \ *Pt1\
     *         *p1                          |     \     \     *P
     *          \                           |    / \     \   /
     *           \                          |   /   \     \ /
     *            \                         |       pd     \
     *             \                        |         \   / \
     *            p2*                       |          \ /   \       /
     *        slope = (p2.y - p1.y) /       |           /     p2d   /
     *                    (p2.x - p1.x)     |          /       \   /
     *                                      |         /         \ /
     *                                      |        /           /
     *                                      |       /           /
     *                                      |      /           *Pt2
     *                                      |                 /
     *                                      |                /
     *                                      |               /
     *                                      |              /
     *                                      |             /
     *                               -------+---------------------------------
     *                                     O|
     *                                      |
     *                                      |
     *
     *      step 1: compute the distance of p, pt1 and pt2 in the slope direction.
     *              Calculate the distance on Y axis first and multiply cos_val to
     *              get the value on slope direction(pd, p1d and p2d represent the
     *              distance of p, pt1, and pt2 respectively).
     *
     *      step 2: calculate the percentage of (pd - p1d)/(p2d - p1d).
     *              If (pd - p1d) > (p2d - p1d) or < 0, then sub or add (p2d - p1d)
     *              to make it in the range of [0, (p2d - p1d)].
     *
     *      step 3: compare the percentage to every stop and find the stpos just
     *              before and after it. Use the interpolation fomula to compute RGBA.
     */

#define gradient_fs_template	\
	    GLAMOR_DEFAULT_PRECISION\
	    "uniform mat3 transform_mat;\n"\
	    "uniform int repeat_type;\n"\
	    "uniform int hor_ver;\n"\
	    "uniform float pt_slope;\n"\
	    "uniform float cos_val;\n"\
	    "uniform float p1_distance;\n"\
	    "uniform float pt_distance;\n"\
	    "varying vec2 source_texture;\n"\
	    "\n"\
	    "vec4 get_color(float stop_len);\n"\
	    "\n"\
	    "float get_stop_len()\n"\
	    "{\n"\
	    "    vec3 tmp = vec3(source_texture.x, source_texture.y, 1.0);\n"\
	    "    float distance;\n"\
	    "    float _p1_distance;\n"\
	    "    float _pt_distance;\n"\
	    "    float y_dist;\n"\
	    "    vec3 source_texture_trans = transform_mat * tmp;\n"\
	    "    \n"\
	    "    if(hor_ver == 0) { \n" /*Normal case.*/\
	    "        y_dist = source_texture_trans.y - source_texture_trans.x*pt_slope;\n"\
	    "        distance = y_dist * cos_val;\n"\
	    "        _p1_distance = p1_distance * source_texture_trans.z;\n"\
	    "        _pt_distance = pt_distance * source_texture_trans.z;\n"\
	    "        \n"\
	    "    } else if (hor_ver == 1) {\n"/*horizontal case.*/\
	    "        distance = source_texture_trans.x;\n"\
	    "        _p1_distance = p1_distance * source_texture_trans.z;\n"\
	    "        _pt_distance = pt_distance * source_texture_trans.z;\n"\
	    "    } \n"\
	    "    \n"\
	    "    distance = (distance - _p1_distance) / _pt_distance;\n"\
	    "    \n"\
	    "    if(repeat_type == %d){\n" /* repeat normal*/\
	    "        distance = fract(distance);\n"\
	    "    }\n"\
	    "    \n"\
	    "    if(repeat_type == %d) {\n" /* repeat reflect*/\
	    "        distance = abs(fract(distance * 0.5 + 0.5) * 2.0 - 1.0);\n"\
	    "    }\n"\
	    "    \n"\
	    "    return distance;\n"\
	    "}\n"\
	    "\n"\
	    "void main()\n"\
	    "{\n"\
	    "    float stop_len = get_stop_len();\n"\
	    "    gl_FragColor = get_color(stop_len);\n"\
	    "}\n"\
	    "\n"\
            "%s" /* fs_getcolor_source */
    char *fs_getcolor_source;

    glamor_priv = glamor_get_screen_private(screen);

    if ((glamor_priv->linear_max_nstops >= stops_count) && (dyn_gen)) {
        /* Very Good, not to generate again. */
        return;
    }

    glamor_make_current(glamor_priv);
    if (dyn_gen && glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][2]) {
        glDeleteProgram(glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][2]);
        glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][2] = 0;
    }

    gradient_prog = glCreateProgram();

    vs_prog = glamor_compile_glsl_prog(GL_VERTEX_SHADER, gradient_vs);

    fs_getcolor_source =
        _glamor_create_getcolor_fs_source(screen, stops_count, stops_count > 0);

    XNFasprintf(&gradient_fs,
                gradient_fs_template,
                PIXMAN_REPEAT_NORMAL, PIXMAN_REPEAT_REFLECT,
                fs_getcolor_source);

    fs_prog = glamor_compile_glsl_prog(GL_FRAGMENT_SHADER, gradient_fs);
    free(gradient_fs);
    free(fs_getcolor_source);

    glAttachShader(gradient_prog, vs_prog);
    glAttachShader(gradient_prog, fs_prog);
    glDeleteShader(vs_prog);
    glDeleteShader(fs_prog);

    glBindAttribLocation(gradient_prog, GLAMOR_VERTEX_POS, "v_position");
    glBindAttribLocation(gradient_prog, GLAMOR_VERTEX_SOURCE, "v_texcoord");

    glamor_link_glsl_prog(screen, gradient_prog, "linear gradient");

    if (dyn_gen) {
        index = 2;
        glamor_priv->linear_max_nstops = stops_count;
    }
    else if (stops_count) {
        index = 1;
    }
    else {
        index = 0;
    }

    glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][index] = gradient_prog;
}

void
glamor_init_gradient_shader(ScreenPtr screen)
{
    glamor_screen_private *glamor_priv;
    int i;

    glamor_priv = glamor_get_screen_private(screen);

    for (i = 0; i < 3; i++) {
        glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][i] = 0;
        glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][i] = 0;
    }
    glamor_priv->linear_max_nstops = 0;
    glamor_priv->radial_max_nstops = 0;

    _glamor_create_linear_gradient_program(screen, 0, 0);
    _glamor_create_linear_gradient_program(screen, LINEAR_LARGE_STOPS, 0);

    _glamor_create_radial_gradient_program(screen, 0, 0);
    _glamor_create_radial_gradient_program(screen, RADIAL_LARGE_STOPS, 0);
}

static void
_glamor_gradient_convert_trans_matrix(PictTransform *from, float to[3][3],
                                      int width, int height, int normalize)
{
    /*
     * Because in the shader program, we normalize all the pixel cood to [0, 1],
     * so with the transform matrix, the correct logic should be:
     * v_s = A*T*v
     * v_s: point vector in shader after normalized.
     * A: The transition matrix from   width X height --> 1.0 X 1.0
     * T: The transform matrix.
     * v: point vector in width X height space.
     *
     * result is OK if we use this fomula. But for every point in width X height space,
     * we can just use their normalized point vector in shader, namely we can just
     * use the result of A*v in shader. So we have no chance to insert T in A*v.
     * We can just convert v_s = A*T*v to v_s = A*T*inv(A)*A*v, where inv(A) is the
     * inverse matrix of A. Now, v_s = (A*T*inv(A)) * (A*v)
     * So, to get the correct v_s, we need to cacula1 the matrix: (A*T*inv(A)), and
     * we name this matrix T_s.
     *
     * Firstly, because A is for the scale conversion, we find
     *      --         --
     *      |1/w  0   0 |
     * A =  | 0  1/h  0 |
     *      | 0   0  1.0|
     *      --         --
     * so T_s = A*T*inv(a) and result
     *
     *       --                      --
     *       | t11      h*t12/w  t13/w|
     * T_s = | w*t21/h  t22      t23/h|
     *       | w*t31    h*t32    t33  |
     *       --                      --
     */

    to[0][0] = (float) pixman_fixed_to_double(from->matrix[0][0]);
    to[0][1] = (float) pixman_fixed_to_double(from->matrix[0][1])
        * (normalize ? (((float) height) / ((float) width)) : 1.0);
    to[0][2] = (float) pixman_fixed_to_double(from->matrix[0][2])
        / (normalize ? ((float) width) : 1.0);

    to[1][0] = (float) pixman_fixed_to_double(from->matrix[1][0])
        * (normalize ? (((float) width) / ((float) height)) : 1.0);
    to[1][1] = (float) pixman_fixed_to_double(from->matrix[1][1]);
    to[1][2] = (float) pixman_fixed_to_double(from->matrix[1][2])
        / (normalize ? ((float) height) : 1.0);

    to[2][0] = (float) pixman_fixed_to_double(from->matrix[2][0])
        * (normalize ? ((float) width) : 1.0);
    to[2][1] = (float) pixman_fixed_to_double(from->matrix[2][1])
        * (normalize ? ((float) height) : 1.0);
    to[2][2] = (float) pixman_fixed_to_double(from->matrix[2][2]);

    DEBUGF("the transform matrix is:\n%f\t%f\t%f\n%f\t%f\t%f\n%f\t%f\t%f\n",
           to[0][0], to[0][1], to[0][2],
           to[1][0], to[1][1], to[1][2], to[2][0], to[2][1], to[2][2]);
}

static int
_glamor_gradient_set_pixmap_destination(ScreenPtr screen,
                                        glamor_screen_private *glamor_priv,
                                        PicturePtr dst_picture,
                                        GLfloat *xscale, GLfloat *yscale,
                                        int x_source, int y_source,
                                        int tex_normalize)
{
    glamor_pixmap_private *pixmap_priv;
    PixmapPtr pixmap = NULL;
    GLfloat *v;
    char *vbo_offset;

    pixmap = glamor_get_drawable_pixmap(dst_picture->pDrawable);
    pixmap_priv = glamor_get_pixmap_private(pixmap);

    if (!GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)) {     /* should always have here. */
        return 0;
    }

    glamor_set_destination_pixmap_priv_nc(glamor_priv, pixmap, pixmap_priv);

    pixmap_priv_get_dest_scale(pixmap, pixmap_priv, xscale, yscale);

    DEBUGF("xscale = %f, yscale = %f,"
           " x_source = %d, y_source = %d, width = %d, height = %d\n",
           *xscale, *yscale, x_source, y_source,
           dst_picture->pDrawable->width, dst_picture->pDrawable->height);

    v = glamor_get_vbo_space(screen, 16 * sizeof(GLfloat), &vbo_offset);

    glamor_set_normalize_vcoords_tri_strip(*xscale, *yscale,
                                           0, 0,
                                           (INT16) (dst_picture->pDrawable->
                                                    width),
                                           (INT16) (dst_picture->pDrawable->
                                                    height),
                                           v);

    if (tex_normalize) {
        glamor_set_normalize_tcoords_tri_stripe(*xscale, *yscale,
                                                x_source, y_source,
                                                (INT16) (dst_picture->
                                                         pDrawable->width +
                                                         x_source),
                                                (INT16) (dst_picture->
                                                         pDrawable->height +
                                                         y_source),
                                                &v[8]);
    }
    else {
        glamor_set_tcoords_tri_strip(x_source, y_source,
                                     (INT16) (dst_picture->pDrawable->width) +
                                     x_source,
                                     (INT16) (dst_picture->pDrawable->height) +
                                     y_source,
                                     &v[8]);
    }

    DEBUGF("vertices --> leftup : %f X %f, rightup: %f X %f,"
           "rightbottom: %f X %f, leftbottom : %f X %f\n",
           v[0], v[1], v[2], v[3],
           v[4], v[5], v[6], v[7]);
    DEBUGF("tex_vertices --> leftup : %f X %f, rightup: %f X %f,"
           "rightbottom: %f X %f, leftbottom : %f X %f\n",
           v[8], v[9], v[10], v[11],
           v[12], v[13], v[14], v[15]);

    glamor_make_current(glamor_priv);

    glVertexAttribPointer(GLAMOR_VERTEX_POS, 2, GL_FLOAT,
                          GL_FALSE, 0, vbo_offset);
    glVertexAttribPointer(GLAMOR_VERTEX_SOURCE, 2, GL_FLOAT,
                          GL_FALSE, 0, vbo_offset + 8 * sizeof(GLfloat));

    glEnableVertexAttribArray(GLAMOR_VERTEX_POS);
    glEnableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

    glamor_put_vbo_space(screen);
    return 1;
}

static int
_glamor_gradient_set_stops(PicturePtr src_picture, PictGradient *pgradient,
                           GLfloat *stop_colors, GLfloat *n_stops)
{
    int i;
    int count = 1;

    for (i = 0; i < pgradient->nstops; i++) {
        stop_colors[count * 4] =
            pixman_fixed_to_double(pgradient->stops[i].color.red);
        stop_colors[count * 4 + 1] =
            pixman_fixed_to_double(pgradient->stops[i].color.green);
        stop_colors[count * 4 + 2] =
            pixman_fixed_to_double(pgradient->stops[i].color.blue);
        stop_colors[count * 4 + 3] =
            pixman_fixed_to_double(pgradient->stops[i].color.alpha);

        n_stops[count] =
            (GLfloat) pixman_fixed_to_double(pgradient->stops[i].x);
        count++;
    }

    /* for the end stop. */
    count++;

    switch (src_picture->repeatType) {
#define REPEAT_FILL_STOPS(m, n) \
			stop_colors[(m)*4 + 0] = stop_colors[(n)*4 + 0]; \
			stop_colors[(m)*4 + 1] = stop_colors[(n)*4 + 1]; \
			stop_colors[(m)*4 + 2] = stop_colors[(n)*4 + 2]; \
			stop_colors[(m)*4 + 3] = stop_colors[(n)*4 + 3];

    default:
    case PIXMAN_REPEAT_NONE:
        stop_colors[0] = 0.0;   //R
        stop_colors[1] = 0.0;   //G
        stop_colors[2] = 0.0;   //B
        stop_colors[3] = 0.0;   //Alpha
        n_stops[0] = n_stops[1];

        stop_colors[0 + (count - 1) * 4] = 0.0; //R
        stop_colors[1 + (count - 1) * 4] = 0.0; //G
        stop_colors[2 + (count - 1) * 4] = 0.0; //B
        stop_colors[3 + (count - 1) * 4] = 0.0; //Alpha
        n_stops[count - 1] = n_stops[count - 2];
        break;
    case PIXMAN_REPEAT_NORMAL:
        REPEAT_FILL_STOPS(0, count - 2);
        n_stops[0] = n_stops[count - 2] - 1.0;

        REPEAT_FILL_STOPS(count - 1, 1);
        n_stops[count - 1] = n_stops[1] + 1.0;
        break;
    case PIXMAN_REPEAT_REFLECT:
        REPEAT_FILL_STOPS(0, 1);
        n_stops[0] = -n_stops[1];

        REPEAT_FILL_STOPS(count - 1, count - 2);
        n_stops[count - 1] = 1.0 + 1.0 - n_stops[count - 2];
        break;
    case PIXMAN_REPEAT_PAD:
        REPEAT_FILL_STOPS(0, 1);
        n_stops[0] = -(float) INT_MAX;

        REPEAT_FILL_STOPS(count - 1, count - 2);
        n_stops[count - 1] = (float) INT_MAX;
        break;
#undef REPEAT_FILL_STOPS
    }

    for (i = 0; i < count; i++) {
        DEBUGF("n_stops[%d] = %f, color = r:%f g:%f b:%f a:%f\n",
               i, n_stops[i],
               stop_colors[i * 4], stop_colors[i * 4 + 1],
               stop_colors[i * 4 + 2], stop_colors[i * 4 + 3]);
    }

    return count;
}

PicturePtr
glamor_generate_radial_gradient_picture(ScreenPtr screen,
                                        PicturePtr src_picture,
                                        int x_source, int y_source,
                                        int width, int height,
                                        PictFormatShort format)
{
    glamor_screen_private *glamor_priv;
    PicturePtr dst_picture = NULL;
    PixmapPtr pixmap = NULL;
    GLint gradient_prog = 0;
    int error;
    int stops_count = 0;
    int count = 0;
    GLfloat *stop_colors = NULL;
    GLfloat *n_stops = NULL;
    GLfloat xscale, yscale;
    float transform_mat[3][3];
    static const float identity_mat[3][3] = { {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0}
    };
    GLfloat stop_colors_st[RADIAL_SMALL_STOPS * 4];
    GLfloat n_stops_st[RADIAL_SMALL_STOPS];
    GLfloat A_value;
    GLfloat cxy[4];
    float c1x, c1y, c2x, c2y, r1, r2;

    GLint transform_mat_uniform_location = 0;
    GLint repeat_type_uniform_location = 0;
    GLint n_stop_uniform_location = 0;
    GLint stops_uniform_location = 0;
    GLint stop_colors_uniform_location = 0;
    GLint stop0_uniform_location = 0;
    GLint stop1_uniform_location = 0;
    GLint stop2_uniform_location = 0;
    GLint stop3_uniform_location = 0;
    GLint stop4_uniform_location = 0;
    GLint stop5_uniform_location = 0;
    GLint stop6_uniform_location = 0;
    GLint stop7_uniform_location = 0;
    GLint stop_color0_uniform_location = 0;
    GLint stop_color1_uniform_location = 0;
    GLint stop_color2_uniform_location = 0;
    GLint stop_color3_uniform_location = 0;
    GLint stop_color4_uniform_location = 0;
    GLint stop_color5_uniform_location = 0;
    GLint stop_color6_uniform_location = 0;
    GLint stop_color7_uniform_location = 0;
    GLint A_value_uniform_location = 0;
    GLint c1_uniform_location = 0;
    GLint r1_uniform_location = 0;
    GLint c2_uniform_location = 0;
    GLint r2_uniform_location = 0;

    glamor_priv = glamor_get_screen_private(screen);
    glamor_make_current(glamor_priv);

    /* Create a pixmap with VBO. */
    pixmap = glamor_create_pixmap(screen,
                                  width, height,
                                  PIXMAN_FORMAT_DEPTH(format), 0);
    if (!pixmap)
        goto GRADIENT_FAIL;

    dst_picture = CreatePicture(0, &pixmap->drawable,
                                PictureMatchFormat(screen,
                                                   PIXMAN_FORMAT_DEPTH(format),
                                                   format), 0, 0, serverClient,
                                &error);

    /* Release the reference, picture will hold the last one. */
    glamor_destroy_pixmap(pixmap);

    if (!dst_picture)
        goto GRADIENT_FAIL;

    ValidatePicture(dst_picture);

    stops_count = src_picture->pSourcePict->radial.nstops + 2;

    /* Because the max value of nstops is unknown, so create a program
       when nstops > LINEAR_LARGE_STOPS. */
    if (stops_count <= RADIAL_SMALL_STOPS) {
        gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][0];
    }
    else if (stops_count <= RADIAL_LARGE_STOPS) {
        gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][1];
    }
    else {
        _glamor_create_radial_gradient_program(screen,
                                               src_picture->pSourcePict->linear.
                                               nstops + 2, 1);
        gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_RADIAL][2];
    }

    /* Bind all the uniform vars . */
    transform_mat_uniform_location = glGetUniformLocation(gradient_prog,
                                                          "transform_mat");
    repeat_type_uniform_location = glGetUniformLocation(gradient_prog,
                                                        "repeat_type");
    n_stop_uniform_location = glGetUniformLocation(gradient_prog, "n_stop");
    A_value_uniform_location = glGetUniformLocation(gradient_prog, "A_value");
    c1_uniform_location = glGetUniformLocation(gradient_prog, "c1");
    r1_uniform_location = glGetUniformLocation(gradient_prog, "r1");
    c2_uniform_location = glGetUniformLocation(gradient_prog, "c2");
    r2_uniform_location = glGetUniformLocation(gradient_prog, "r2");

    if (src_picture->pSourcePict->radial.nstops + 2 <= RADIAL_SMALL_STOPS) {
        stop0_uniform_location =
            glGetUniformLocation(gradient_prog, "stop0");
        stop1_uniform_location =
            glGetUniformLocation(gradient_prog, "stop1");
        stop2_uniform_location =
            glGetUniformLocation(gradient_prog, "stop2");
        stop3_uniform_location =
            glGetUniformLocation(gradient_prog, "stop3");
        stop4_uniform_location =
            glGetUniformLocation(gradient_prog, "stop4");
        stop5_uniform_location =
            glGetUniformLocation(gradient_prog, "stop5");
        stop6_uniform_location =
            glGetUniformLocation(gradient_prog, "stop6");
        stop7_uniform_location =
            glGetUniformLocation(gradient_prog, "stop7");

        stop_color0_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color0");
        stop_color1_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color1");
        stop_color2_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color2");
        stop_color3_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color3");
        stop_color4_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color4");
        stop_color5_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color5");
        stop_color6_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color6");
        stop_color7_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color7");
    }
    else {
        stops_uniform_location =
            glGetUniformLocation(gradient_prog, "stops");
        stop_colors_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_colors");
    }

    glUseProgram(gradient_prog);

    glUniform1i(repeat_type_uniform_location, src_picture->repeatType);

    if (src_picture->transform) {
        _glamor_gradient_convert_trans_matrix(src_picture->transform,
                                              transform_mat, width, height, 0);
        glUniformMatrix3fv(transform_mat_uniform_location,
                           1, 1, &transform_mat[0][0]);
    }
    else {
        glUniformMatrix3fv(transform_mat_uniform_location,
                           1, 1, &identity_mat[0][0]);
    }

    if (!_glamor_gradient_set_pixmap_destination
        (screen, glamor_priv, dst_picture, &xscale, &yscale, x_source, y_source,
         0))
        goto GRADIENT_FAIL;

    glamor_set_alu(screen, GXcopy);

    /* Set all the stops and colors to shader. */
    if (stops_count > RADIAL_SMALL_STOPS) {
        stop_colors = xallocarray(stops_count, 4 * sizeof(float));
        if (stop_colors == NULL) {
            ErrorF("Failed to allocate stop_colors memory.\n");
            goto GRADIENT_FAIL;
        }

        n_stops = xallocarray(stops_count, sizeof(float));
        if (n_stops == NULL) {
            ErrorF("Failed to allocate n_stops memory.\n");
            goto GRADIENT_FAIL;
        }
    }
    else {
        stop_colors = stop_colors_st;
        n_stops = n_stops_st;
    }

    count =
        _glamor_gradient_set_stops(src_picture,
                                   &src_picture->pSourcePict->gradient,
                                   stop_colors, n_stops);

    if (src_picture->pSourcePict->linear.nstops + 2 <= RADIAL_SMALL_STOPS) {
        int j = 0;

        glUniform4f(stop_color0_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color1_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color2_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color3_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color4_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color5_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color6_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color7_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);

        j = 0;
        glUniform1f(stop0_uniform_location, n_stops[j++]);
        glUniform1f(stop1_uniform_location, n_stops[j++]);
        glUniform1f(stop2_uniform_location, n_stops[j++]);
        glUniform1f(stop3_uniform_location, n_stops[j++]);
        glUniform1f(stop4_uniform_location, n_stops[j++]);
        glUniform1f(stop5_uniform_location, n_stops[j++]);
        glUniform1f(stop6_uniform_location, n_stops[j++]);
        glUniform1f(stop7_uniform_location, n_stops[j++]);
        glUniform1i(n_stop_uniform_location, count);
    }
    else {
        glUniform4fv(stop_colors_uniform_location, count, stop_colors);
        glUniform1fv(stops_uniform_location, count, n_stops);
        glUniform1i(n_stop_uniform_location, count);
    }

    c1x = (float) pixman_fixed_to_double(src_picture->pSourcePict->radial.c1.x);
    c1y = (float) pixman_fixed_to_double(src_picture->pSourcePict->radial.c1.y);
    c2x = (float) pixman_fixed_to_double(src_picture->pSourcePict->radial.c2.x);
    c2y = (float) pixman_fixed_to_double(src_picture->pSourcePict->radial.c2.y);

    r1 = (float) pixman_fixed_to_double(src_picture->pSourcePict->radial.c1.
                                        radius);
    r2 = (float) pixman_fixed_to_double(src_picture->pSourcePict->radial.c2.
                                        radius);

    glamor_set_circle_centre(width, height, c1x, c1y, cxy);
    glUniform2fv(c1_uniform_location, 1, cxy);
    glUniform1f(r1_uniform_location, r1);

    glamor_set_circle_centre(width, height, c2x, c2y, cxy);
    glUniform2fv(c2_uniform_location, 1, cxy);
    glUniform1f(r2_uniform_location, r2);

    A_value =
        (c2x - c1x) * (c2x - c1x) + (c2y - c1y) * (c2y - c1y) - (r2 -
                                                                 r1) * (r2 -
                                                                        r1);
    glUniform1f(A_value_uniform_location, A_value);

    DEBUGF("C1:(%f, %f) R1:%f\nC2:(%f, %f) R2:%f\nA = %f\n",
           c1x, c1y, r1, c2x, c2y, r2, A_value);

    /* Now rendering. */
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    /* Do the clear logic. */
    if (stops_count > RADIAL_SMALL_STOPS) {
        free(n_stops);
        free(stop_colors);
    }

    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
    glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

    return dst_picture;

 GRADIENT_FAIL:
    if (dst_picture) {
        FreePicture(dst_picture, 0);
    }

    if (stops_count > RADIAL_SMALL_STOPS) {
        if (n_stops)
            free(n_stops);
        if (stop_colors)
            free(stop_colors);
    }

    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
    glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
    return NULL;
}

PicturePtr
glamor_generate_linear_gradient_picture(ScreenPtr screen,
                                        PicturePtr src_picture,
                                        int x_source, int y_source,
                                        int width, int height,
                                        PictFormatShort format)
{
    glamor_screen_private *glamor_priv;
    PicturePtr dst_picture = NULL;
    PixmapPtr pixmap = NULL;
    GLint gradient_prog = 0;
    int error;
    float pt_distance;
    float p1_distance;
    GLfloat cos_val;
    int stops_count = 0;
    GLfloat *stop_colors = NULL;
    GLfloat *n_stops = NULL;
    int count = 0;
    float slope;
    GLfloat xscale, yscale;
    GLfloat pt1[2], pt2[2];
    float transform_mat[3][3];
    static const float identity_mat[3][3] = { {1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {0.0, 0.0, 1.0}
    };
    GLfloat stop_colors_st[LINEAR_SMALL_STOPS * 4];
    GLfloat n_stops_st[LINEAR_SMALL_STOPS];

    GLint transform_mat_uniform_location = 0;
    GLint n_stop_uniform_location = 0;
    GLint stops_uniform_location = 0;
    GLint stop0_uniform_location = 0;
    GLint stop1_uniform_location = 0;
    GLint stop2_uniform_location = 0;
    GLint stop3_uniform_location = 0;
    GLint stop4_uniform_location = 0;
    GLint stop5_uniform_location = 0;
    GLint stop6_uniform_location = 0;
    GLint stop7_uniform_location = 0;
    GLint stop_colors_uniform_location = 0;
    GLint stop_color0_uniform_location = 0;
    GLint stop_color1_uniform_location = 0;
    GLint stop_color2_uniform_location = 0;
    GLint stop_color3_uniform_location = 0;
    GLint stop_color4_uniform_location = 0;
    GLint stop_color5_uniform_location = 0;
    GLint stop_color6_uniform_location = 0;
    GLint stop_color7_uniform_location = 0;
    GLint pt_slope_uniform_location = 0;
    GLint repeat_type_uniform_location = 0;
    GLint hor_ver_uniform_location = 0;
    GLint cos_val_uniform_location = 0;
    GLint p1_distance_uniform_location = 0;
    GLint pt_distance_uniform_location = 0;

    glamor_priv = glamor_get_screen_private(screen);
    glamor_make_current(glamor_priv);

    /* Create a pixmap with VBO. */
    pixmap = glamor_create_pixmap(screen,
                                  width, height,
                                  PIXMAN_FORMAT_DEPTH(format), 0);

    if (!pixmap)
        goto GRADIENT_FAIL;

    dst_picture = CreatePicture(0, &pixmap->drawable,
                                PictureMatchFormat(screen,
                                                   PIXMAN_FORMAT_DEPTH(format),
                                                   format), 0, 0, serverClient,
                                &error);

    /* Release the reference, picture will hold the last one. */
    glamor_destroy_pixmap(pixmap);

    if (!dst_picture)
        goto GRADIENT_FAIL;

    ValidatePicture(dst_picture);

    stops_count = src_picture->pSourcePict->linear.nstops + 2;

    /* Because the max value of nstops is unknown, so create a program
       when nstops > LINEAR_LARGE_STOPS. */
    if (stops_count <= LINEAR_SMALL_STOPS) {
        gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][0];
    }
    else if (stops_count <= LINEAR_LARGE_STOPS) {
        gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][1];
    }
    else {
        _glamor_create_linear_gradient_program(screen,
                                               src_picture->pSourcePict->linear.
                                               nstops + 2, 1);
        gradient_prog = glamor_priv->gradient_prog[SHADER_GRADIENT_LINEAR][2];
    }

    /* Bind all the uniform vars . */
    n_stop_uniform_location =
        glGetUniformLocation(gradient_prog, "n_stop");
    pt_slope_uniform_location =
        glGetUniformLocation(gradient_prog, "pt_slope");
    repeat_type_uniform_location =
        glGetUniformLocation(gradient_prog, "repeat_type");
    hor_ver_uniform_location =
        glGetUniformLocation(gradient_prog, "hor_ver");
    transform_mat_uniform_location =
        glGetUniformLocation(gradient_prog, "transform_mat");
    cos_val_uniform_location =
        glGetUniformLocation(gradient_prog, "cos_val");
    p1_distance_uniform_location =
        glGetUniformLocation(gradient_prog, "p1_distance");
    pt_distance_uniform_location =
        glGetUniformLocation(gradient_prog, "pt_distance");

    if (src_picture->pSourcePict->linear.nstops + 2 <= LINEAR_SMALL_STOPS) {
        stop0_uniform_location =
            glGetUniformLocation(gradient_prog, "stop0");
        stop1_uniform_location =
            glGetUniformLocation(gradient_prog, "stop1");
        stop2_uniform_location =
            glGetUniformLocation(gradient_prog, "stop2");
        stop3_uniform_location =
            glGetUniformLocation(gradient_prog, "stop3");
        stop4_uniform_location =
            glGetUniformLocation(gradient_prog, "stop4");
        stop5_uniform_location =
            glGetUniformLocation(gradient_prog, "stop5");
        stop6_uniform_location =
            glGetUniformLocation(gradient_prog, "stop6");
        stop7_uniform_location =
            glGetUniformLocation(gradient_prog, "stop7");

        stop_color0_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color0");
        stop_color1_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color1");
        stop_color2_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color2");
        stop_color3_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color3");
        stop_color4_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color4");
        stop_color5_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color5");
        stop_color6_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color6");
        stop_color7_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_color7");
    }
    else {
        stops_uniform_location =
            glGetUniformLocation(gradient_prog, "stops");
        stop_colors_uniform_location =
            glGetUniformLocation(gradient_prog, "stop_colors");
    }

    glUseProgram(gradient_prog);

    glUniform1i(repeat_type_uniform_location, src_picture->repeatType);

    /* set the transform matrix. */
    if (src_picture->transform) {
        _glamor_gradient_convert_trans_matrix(src_picture->transform,
                                              transform_mat, width, height, 1);
        glUniformMatrix3fv(transform_mat_uniform_location,
                           1, 1, &transform_mat[0][0]);
    }
    else {
        glUniformMatrix3fv(transform_mat_uniform_location,
                           1, 1, &identity_mat[0][0]);
    }

    if (!_glamor_gradient_set_pixmap_destination
        (screen, glamor_priv, dst_picture, &xscale, &yscale, x_source, y_source,
         1))
        goto GRADIENT_FAIL;

    glamor_set_alu(screen, GXcopy);

    /* Normalize the PTs. */
    glamor_set_normalize_pt(xscale, yscale,
                            pixman_fixed_to_double(src_picture->pSourcePict->
                                                   linear.p1.x),
                            pixman_fixed_to_double(src_picture->pSourcePict->
                                                   linear.p1.y),
                            pt1);
    DEBUGF("pt1:(%f, %f) ---> (%f %f)\n",
           pixman_fixed_to_double(src_picture->pSourcePict->linear.p1.x),
           pixman_fixed_to_double(src_picture->pSourcePict->linear.p1.y),
           pt1[0], pt1[1]);

    glamor_set_normalize_pt(xscale, yscale,
                            pixman_fixed_to_double(src_picture->pSourcePict->
                                                   linear.p2.x),
                            pixman_fixed_to_double(src_picture->pSourcePict->
                                                   linear.p2.y),
                            pt2);
    DEBUGF("pt2:(%f, %f) ---> (%f %f)\n",
           pixman_fixed_to_double(src_picture->pSourcePict->linear.p2.x),
           pixman_fixed_to_double(src_picture->pSourcePict->linear.p2.y),
           pt2[0], pt2[1]);

    /* Set all the stops and colors to shader. */
    if (stops_count > LINEAR_SMALL_STOPS) {
        stop_colors = xallocarray(stops_count, 4 * sizeof(float));
        if (stop_colors == NULL) {
            ErrorF("Failed to allocate stop_colors memory.\n");
            goto GRADIENT_FAIL;
        }

        n_stops = xallocarray(stops_count, sizeof(float));
        if (n_stops == NULL) {
            ErrorF("Failed to allocate n_stops memory.\n");
            goto GRADIENT_FAIL;
        }
    }
    else {
        stop_colors = stop_colors_st;
        n_stops = n_stops_st;
    }

    count =
        _glamor_gradient_set_stops(src_picture,
                                   &src_picture->pSourcePict->gradient,
                                   stop_colors, n_stops);

    if (src_picture->pSourcePict->linear.nstops + 2 <= LINEAR_SMALL_STOPS) {
        int j = 0;

        glUniform4f(stop_color0_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color1_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color2_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color3_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color4_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color5_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color6_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);
        j++;
        glUniform4f(stop_color7_uniform_location,
                    stop_colors[4 * j + 0], stop_colors[4 * j + 1],
                    stop_colors[4 * j + 2], stop_colors[4 * j + 3]);

        j = 0;
        glUniform1f(stop0_uniform_location, n_stops[j++]);
        glUniform1f(stop1_uniform_location, n_stops[j++]);
        glUniform1f(stop2_uniform_location, n_stops[j++]);
        glUniform1f(stop3_uniform_location, n_stops[j++]);
        glUniform1f(stop4_uniform_location, n_stops[j++]);
        glUniform1f(stop5_uniform_location, n_stops[j++]);
        glUniform1f(stop6_uniform_location, n_stops[j++]);
        glUniform1f(stop7_uniform_location, n_stops[j++]);

        glUniform1i(n_stop_uniform_location, count);
    }
    else {
        glUniform4fv(stop_colors_uniform_location, count, stop_colors);
        glUniform1fv(stops_uniform_location, count, n_stops);
        glUniform1i(n_stop_uniform_location, count);
    }

    if (src_picture->pSourcePict->linear.p2.y == src_picture->pSourcePict->linear.p1.y) {       // The horizontal case.
        glUniform1i(hor_ver_uniform_location, 1);
        DEBUGF("p1.y: %f, p2.y: %f, enter the horizontal case\n",
               pt1[1], pt2[1]);

        p1_distance = pt1[0];
        pt_distance = (pt2[0] - p1_distance);
        glUniform1f(p1_distance_uniform_location, p1_distance);
        glUniform1f(pt_distance_uniform_location, pt_distance);
    }
    else {
        /* The slope need to compute here. In shader, the viewport set will change
           the original slope and the slope which is vertical to it will not be correct. */
        slope = -(float) (src_picture->pSourcePict->linear.p2.x
                          - src_picture->pSourcePict->linear.p1.x) /
            (float) (src_picture->pSourcePict->linear.p2.y
                     - src_picture->pSourcePict->linear.p1.y);
        slope = slope * yscale / xscale;
        glUniform1f(pt_slope_uniform_location, slope);
        glUniform1i(hor_ver_uniform_location, 0);

        cos_val = sqrt(1.0 / (slope * slope + 1.0));
        glUniform1f(cos_val_uniform_location, cos_val);

        p1_distance = (pt1[1] - pt1[0] * slope) * cos_val;
        pt_distance = (pt2[1] - pt2[0] * slope) * cos_val - p1_distance;
        glUniform1f(p1_distance_uniform_location, p1_distance);
        glUniform1f(pt_distance_uniform_location, pt_distance);
    }

    /* Now rendering. */
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    /* Do the clear logic. */
    if (stops_count > LINEAR_SMALL_STOPS) {
        free(n_stops);
        free(stop_colors);
    }

    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
    glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);

    return dst_picture;

 GRADIENT_FAIL:
    if (dst_picture) {
        FreePicture(dst_picture, 0);
    }

    if (stops_count > LINEAR_SMALL_STOPS) {
        if (n_stops)
            free(n_stops);
        if (stop_colors)
            free(stop_colors);
    }

    glDisableVertexAttribArray(GLAMOR_VERTEX_POS);
    glDisableVertexAttribArray(GLAMOR_VERTEX_SOURCE);
    return NULL;
}
