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
 *    Zhigang Gong <zhigang.gong@linux.intel.com>
 *
 */

#ifndef GLAMOR_PRIV_H
#error This file can only be included by glamor_priv.h
#endif

#ifndef __GLAMOR_UTILS_H__
#define __GLAMOR_UTILS_H__

#include "glamor_prepare.h"
#include "mipict.h"

#define v_from_x_coord_x(_xscale_, _x_)          ( 2 * (_x_) * (_xscale_) - 1.0)
#define v_from_x_coord_y(_yscale_, _y_)          (2 * (_y_) * (_yscale_) - 1.0)
#define t_from_x_coord_x(_xscale_, _x_)          ((_x_) * (_xscale_))
#define t_from_x_coord_y(_yscale_, _y_)          ((_y_) * (_yscale_))

#define pixmap_priv_get_dest_scale(pixmap, _pixmap_priv_, _pxscale_, _pyscale_) \
  do {                                                                   \
    int _w_,_h_;                                                         \
    PIXMAP_PRIV_GET_ACTUAL_SIZE(pixmap, _pixmap_priv_, _w_, _h_);        \
    *(_pxscale_) = 1.0 / _w_;                                            \
    *(_pyscale_) = 1.0 / _h_;                                            \
   } while(0)

#define pixmap_priv_get_scale(_pixmap_priv_, _pxscale_, _pyscale_)	\
   do {									\
    *(_pxscale_) = 1.0 / (_pixmap_priv_)->fbo->width;			\
    *(_pyscale_) = 1.0 / (_pixmap_priv_)->fbo->height;			\
  } while(0)

#define PIXMAP_PRIV_GET_ACTUAL_SIZE(pixmap, priv, w, h)          \
  do {								\
	if (_X_UNLIKELY(glamor_pixmap_priv_is_large(priv))) {	\
		w = priv->box.x2 - priv->box.x1;	\
		h = priv->box.y2 - priv->box.y1;	\
	} else {						\
		w = (pixmap)->drawable.width;		\
		h = (pixmap)->drawable.height;		\
	}							\
  } while(0)

#define glamor_pixmap_fbo_fix_wh_ratio(wh, pixmap, priv)         \
  do {								\
	int actual_w, actual_h;					\
	PIXMAP_PRIV_GET_ACTUAL_SIZE(pixmap, priv, actual_w, actual_h);	\
	wh[0] = (float)priv->fbo->width / actual_w;	\
	wh[1] = (float)priv->fbo->height / actual_h;	\
	wh[2] = 1.0 / priv->fbo->width;			\
	wh[3] = 1.0 / priv->fbo->height;			\
  } while(0)

#define pixmap_priv_get_fbo_off(_priv_, _xoff_, _yoff_)		\
   do {								\
        if (_X_UNLIKELY(_priv_ && glamor_pixmap_priv_is_large(_priv_))) { \
		*(_xoff_) = - (_priv_)->box.x1;	\
		*(_yoff_) = - (_priv_)->box.y1;	\
	} else {						\
		*(_xoff_) = 0;					\
		*(_yoff_) = 0;					\
	}							\
   } while(0)

#define xFixedToFloat(_val_) ((float)xFixedToInt(_val_)			\
			      + ((float)xFixedFrac(_val_) / 65536.0))

#define glamor_picture_get_matrixf(_picture_, _matrix_)			\
  do {									\
    int _i_;								\
    if ((_picture_)->transform)						\
      {									\
	for(_i_ = 0; _i_ < 3; _i_++)					\
	  {								\
	    (_matrix_)[_i_ * 3 + 0] =					\
	      xFixedToFloat((_picture_)->transform->matrix[_i_][0]);	\
	    (_matrix_)[_i_ * 3 + 1] =					\
	      xFixedToFloat((_picture_)->transform->matrix[_i_][1]);	\
	    (_matrix_)[_i_ * 3 + 2] = \
	      xFixedToFloat((_picture_)->transform->matrix[_i_][2]);	\
	  }								\
      }									\
  }  while(0)

#define fmod(x, w)		(x - w * floor((float)x/w))

#define fmodulus(x, w, c)	do {c = fmod(x, w);		\
				    c = c >= 0 ? c : c + w;}	\
				while(0)
/* @x: is current coord
 * @x2: is the right/bottom edge
 * @w: is current width or height
 * @odd: is output value, 0 means we are in an even region, 1 means we are in a
 * odd region.
 * @c: is output value, equal to x mod w. */
#define fodd_repeat_mod(x, x2, w, odd, c)	\
  do {						\
	float shift;				\
	fmodulus((x), w, c); 			\
	shift = fabs((x) - (c));		\
	shift = floor(fabs(round(shift)) / w);	\
	odd = (int)shift & 1;			\
	if (odd && (((x2 % w) == 0) &&		\
	    round(fabs(x)) == x2))		\
		odd = 0;			\
  } while(0)

/* @txy: output value, is the corrected coords.
 * @xy: input coords to be fixed up.
 * @cd: xy mod wh, is a input value.
 * @wh: current width or height.
 * @bxy1,bxy2: current box edge's x1/x2 or y1/y2
 *
 * case 1:
 *  ----------
 *  |  *     |
 *  |        |
 *  ----------
 *  tx = (c - x1) mod w
 *
 *  case 2:
 *     ---------
 *  *  |       |
 *     |       |
 *     ---------
 *   tx = - (c - (x1 mod w))
 *
 *   case 3:
 *
 *   ----------
 *   |        |  *
 *   |        |
 *   ----------
 *   tx = ((x2 mod x) - c) + (x2 - x1)
 **/
#define __glamor_repeat_reflect_fixup(txy, xy,		\
				cd, wh, bxy1, bxy2)	\
  do {							\
	cd = wh - cd;					\
	if ( xy >= bxy1 && xy < bxy2) {			\
		cd = cd - bxy1;				\
		fmodulus(cd, wh, txy);			\
	} else	if (xy < bxy1) {			\
		float bxy1_mod;				\
		fmodulus(bxy1, wh, bxy1_mod);		\
		txy = -(cd - bxy1_mod);			\
	}						\
	else if (xy >= bxy2)	{			\
		float bxy2_mod;				\
		fmodulus(bxy2, wh, bxy2_mod);		\
		if (bxy2_mod == 0)			\
			bxy2_mod = wh;			\
		txy = (bxy2_mod - cd) + bxy2 - bxy1;	\
	} else {assert(0); txy = 0;}			\
  } while(0)

#define _glamor_repeat_reflect_fixup(txy, xy, cd, odd,	\
				     wh, bxy1, bxy2)	\
  do {							\
	if (odd) {					\
		__glamor_repeat_reflect_fixup(txy, xy, 	\
			cd, wh, bxy1, bxy2);		\
	} else						\
		txy = xy - bxy1;			\
  } while(0)

#define _glamor_get_reflect_transform_coords(pixmap, priv, repeat_type,	\
					    tx1, ty1, 		\
				            _x1_, _y1_)		\
  do {								\
	int odd_x, odd_y;					\
	float c, d;						\
	fodd_repeat_mod(_x1_,priv->box.x2,			\
		    (pixmap)->drawable.width,		\
		    odd_x, c);					\
	fodd_repeat_mod(_y1_,	priv->box.y2,			\
		    (pixmap)->drawable.height,		\
		    odd_y, d);					\
	DEBUGF("c %f d %f oddx %d oddy %d \n",			\
		c, d, odd_x, odd_y);				\
	DEBUGF("x2 %d x1 %d fbo->width %d \n", priv->box.x2,	\
		priv->box.x1, priv->fbo->width);		\
	DEBUGF("y2 %d y1 %d fbo->height %d \n", priv->box.y2, 	\
		priv->box.y1, priv->fbo->height);		\
	_glamor_repeat_reflect_fixup(tx1, _x1_, c, odd_x,	\
		(pixmap)->drawable.width,		\
		priv->box.x1, priv->box.x2);			\
	_glamor_repeat_reflect_fixup(ty1, _y1_, d, odd_y,	\
		(pixmap)->drawable.height,		\
		priv->box.y1, priv->box.y2);			\
   } while(0)

#define _glamor_get_repeat_coords(pixmap, priv, repeat_type, tx1,	\
				  ty1, tx2, ty2,		\
				  _x1_, _y1_, _x2_,		\
				  _y2_, c, d, odd_x, odd_y)	\
  do {								\
	if (repeat_type == RepeatReflect) {			\
		DEBUGF("x1 y1 %d %d\n",				\
			_x1_, _y1_ );				\
		DEBUGF("width %d box.x1 %d \n",			\
		       (pixmap)->drawable.width,	\
		       priv->box.x1);				\
		if (odd_x) {					\
			c = (pixmap)->drawable.width	\
				- c;				\
			tx1 = c - priv->box.x1;			\
			tx2 = tx1 - ((_x2_) - (_x1_));		\
		} else {					\
			tx1 = c - priv->box.x1;			\
			tx2 = tx1 + ((_x2_) - (_x1_));		\
		}						\
		if (odd_y){					\
			d = (pixmap)->drawable.height\
			    - d;				\
			ty1 = d - priv->box.y1;			\
			ty2 = ty1 - ((_y2_) - (_y1_));		\
		} else {					\
			ty1 = d - priv->box.y1;			\
			ty2 = ty1 + ((_y2_) - (_y1_));		\
		}						\
	} else { /* RepeatNormal*/				\
		tx1 = (c - priv->box.x1);  			\
		ty1 = (d - priv->box.y1);			\
		tx2 = tx1 + ((_x2_) - (_x1_));			\
		ty2 = ty1 + ((_y2_) - (_y1_));			\
	}							\
   } while(0)

/* _x1_ ... _y2_ may has fractional. */
#define glamor_get_repeat_transform_coords(pixmap, priv, repeat_type, tx1, \
					   ty1, _x1_, _y1_)		\
  do {									\
	DEBUGF("width %d box.x1 %d x2 %d y1 %d y2 %d\n",		\
		(pixmap)->drawable.width,			\
		priv->box.x1, priv->box.x2, priv->box.y1,		\
		priv->box.y2);						\
	DEBUGF("x1 %f y1 %f \n", _x1_, _y1_);				\
	if (repeat_type != RepeatReflect) {				\
		tx1 = _x1_ - priv->box.x1;				\
		ty1 = _y1_ - priv->box.y1;				\
	} else			\
                _glamor_get_reflect_transform_coords(pixmap, priv, repeat_type, \
				  tx1, ty1, 				\
				  _x1_, _y1_);				\
	DEBUGF("tx1 %f ty1 %f \n", tx1, ty1);				\
   } while(0)

/* _x1_ ... _y2_ must be integer. */
#define glamor_get_repeat_coords(pixmap, priv, repeat_type, tx1,		\
				 ty1, tx2, ty2, _x1_, _y1_, _x2_,	\
				 _y2_) 					\
  do {									\
	int c, d;							\
	int odd_x = 0, odd_y = 0;					\
	DEBUGF("width %d box.x1 %d x2 %d y1 %d y2 %d\n",		\
		(pixmap)->drawable.width,			\
		priv->box.x1, priv->box.x2,				\
		priv->box.y1, priv->box.y2);				\
	modulus((_x1_), (pixmap)->drawable.width, c); 	\
	modulus((_y1_), (pixmap)->drawable.height, d);	\
	DEBUGF("c %d d %d \n", c, d);					\
	if (repeat_type == RepeatReflect) {				\
		odd_x = abs((_x1_ - c)					\
                            / ((pixmap)->drawable.width)) & 1;            \
		odd_y = abs((_y1_ - d)					\
                            / ((pixmap)->drawable.height)) & 1;           \
	}								\
	_glamor_get_repeat_coords(pixmap, priv, repeat_type, tx1, ty1, tx2, ty2, \
				  _x1_, _y1_, _x2_, _y2_, c, d,		\
				  odd_x, odd_y);			\
   } while(0)

#define glamor_transform_point(matrix, tx, ty, x, y)			\
  do {									\
    int _i_;								\
    float _result_[4];							\
    for (_i_ = 0; _i_ < 3; _i_++) {					\
      _result_[_i_] = (matrix)[_i_ * 3] * (x) + (matrix)[_i_ * 3 + 1] * (y)	\
	+ (matrix)[_i_ * 3 + 2];					\
    }									\
    tx = _result_[0] / _result_[2];					\
    ty = _result_[1] / _result_[2];					\
  } while(0)

#define _glamor_set_normalize_tpoint(xscale, yscale, _tx_, _ty_,	\
				     texcoord)                          \
  do {									\
	(texcoord)[0] = t_from_x_coord_x(xscale, _tx_);			\
        (texcoord)[1] = t_from_x_coord_y(yscale, _ty_);                 \
        DEBUGF("normalized point tx %f ty %f \n", (texcoord)[0],	\
		(texcoord)[1]);						\
  } while(0)

#define glamor_set_transformed_point(priv, matrix, xscale,              \
				     yscale, texcoord,			\
                                     x, y)				\
  do {									\
    float tx, ty;							\
    int fbo_x_off, fbo_y_off;						\
    pixmap_priv_get_fbo_off(priv, &fbo_x_off, &fbo_y_off);		\
    glamor_transform_point(matrix, tx, ty, x, y);			\
    DEBUGF("tx %f ty %f fbooff %d %d \n",				\
	    tx, ty, fbo_x_off, fbo_y_off);				\
									\
    tx += fbo_x_off;							\
    ty += fbo_y_off;							\
    (texcoord)[0] = t_from_x_coord_x(xscale, tx);			\
    (texcoord)[1] = t_from_x_coord_y(yscale, ty);                       \
    DEBUGF("normalized tx %f ty %f \n", (texcoord)[0], (texcoord)[1]);	\
  } while(0)

#define glamor_set_transformed_normalize_tcoords_ext( priv,		\
						  matrix,		\
						  xscale,		\
						  yscale,		\
                                                  tx1, ty1, tx2, ty2,   \
                                                  texcoords,		\
						  stride)		\
  do {									\
    glamor_set_transformed_point(priv, matrix, xscale, yscale,		\
				 texcoords, tx1, ty1);                  \
    glamor_set_transformed_point(priv, matrix, xscale, yscale,		\
				 texcoords + 1 * stride, tx2, ty1);     \
    glamor_set_transformed_point(priv, matrix, xscale, yscale,		\
				 texcoords + 2 * stride, tx2, ty2);     \
    glamor_set_transformed_point(priv, matrix, xscale, yscale,		\
				 texcoords + 3 * stride, tx1, ty2);     \
  } while (0)

#define glamor_set_repeat_transformed_normalize_tcoords_ext(pixmap, priv, \
							 repeat_type,	\
							 matrix,	\
							 xscale,	\
							 yscale,	\
							 _x1_, _y1_,	\
							 _x2_, _y2_,   	\
							 texcoords,	\
							 stride)	\
  do {									\
    if (_X_LIKELY(glamor_pixmap_priv_is_small(priv))) {		\
	glamor_set_transformed_normalize_tcoords_ext(priv, matrix, xscale,	\
						 yscale, _x1_, _y1_,	\
						 _x2_, _y2_,	\
						 texcoords, stride);	\
    } else {								\
    float tx1, ty1, tx2, ty2, tx3, ty3, tx4, ty4;			\
    float ttx1, tty1, ttx2, tty2, ttx3, tty3, ttx4, tty4;		\
    DEBUGF("original coords %d %d %d %d\n", _x1_, _y1_, _x2_, _y2_);	\
    glamor_transform_point(matrix, tx1, ty1, _x1_, _y1_);		\
    glamor_transform_point(matrix, tx2, ty2, _x2_, _y1_);		\
    glamor_transform_point(matrix, tx3, ty3, _x2_, _y2_);		\
    glamor_transform_point(matrix, tx4, ty4, _x1_, _y2_);		\
    DEBUGF("transformed %f %f %f %f %f %f %f %f\n",			\
	   tx1, ty1, tx2, ty2, tx3, ty3, tx4, ty4);			\
    glamor_get_repeat_transform_coords(pixmap, priv, repeat_type, \
				       ttx1, tty1, 			\
				       tx1, ty1);			\
    glamor_get_repeat_transform_coords(pixmap, priv, repeat_type, 	\
				       ttx2, tty2, 			\
				       tx2, ty2);			\
    glamor_get_repeat_transform_coords(pixmap, priv, repeat_type, 	\
				       ttx3, tty3, 			\
				       tx3, ty3);			\
    glamor_get_repeat_transform_coords(pixmap, priv, repeat_type, 	\
				       ttx4, tty4, 			\
				       tx4, ty4);			\
    DEBUGF("repeat transformed %f %f %f %f %f %f %f %f\n", ttx1, tty1, 	\
	    ttx2, tty2,	ttx3, tty3, ttx4, tty4);			\
    _glamor_set_normalize_tpoint(xscale, yscale, ttx1, tty1,		\
				 texcoords);			\
    _glamor_set_normalize_tpoint(xscale, yscale, ttx2, tty2,		\
				 texcoords + 1 * stride);	\
    _glamor_set_normalize_tpoint(xscale, yscale, ttx3, tty3,		\
				 texcoords + 2 * stride);	\
    _glamor_set_normalize_tpoint(xscale, yscale, ttx4, tty4,		\
				 texcoords + 3 * stride);	\
   }									\
  } while (0)

#define glamor_set_repeat_transformed_normalize_tcoords( pixmap,        \
                                                         priv,          \
							 repeat_type,	\
							 matrix,	\
							 xscale,	\
							 yscale,	\
							 _x1_, _y1_,	\
							 _x2_, _y2_,   	\
							 texcoords)	\
  do {									\
      glamor_set_repeat_transformed_normalize_tcoords_ext( pixmap,      \
                                                           priv,	\
							 repeat_type,	\
							 matrix,	\
							 xscale,	\
							 yscale,	\
							 _x1_, _y1_,	\
							 _x2_, _y2_,   	\
							 texcoords,	\
							 2);	\
  } while (0)

#define _glamor_set_normalize_tcoords(xscale, yscale, tx1,		\
				      ty1, tx2, ty2,			\
				      vertices, stride)                 \
  do {									\
    /* vertices may be write-only, so we use following			\
     * temporary variable. */ 						\
    float _t0_, _t1_, _t2_, _t5_;					\
    (vertices)[0] = _t0_ = t_from_x_coord_x(xscale, tx1);		\
    (vertices)[1 * stride] = _t2_ = t_from_x_coord_x(xscale, tx2);	\
    (vertices)[2 * stride] = _t2_;					\
    (vertices)[3 * stride] = _t0_;					\
    (vertices)[1] = _t1_ = t_from_x_coord_y(yscale, ty1);               \
    (vertices)[2 * stride + 1] = _t5_ = t_from_x_coord_y(yscale, ty2);  \
    (vertices)[1 * stride + 1] = _t1_;					\
    (vertices)[3 * stride + 1] = _t5_;					\
  } while(0)

#define glamor_set_normalize_tcoords_ext(priv, xscale, yscale,		\
				     x1, y1, x2, y2,			\
                                     vertices, stride)	\
  do {									\
     if (_X_UNLIKELY(glamor_pixmap_priv_is_large(priv))) {		\
	float tx1, tx2, ty1, ty2;					\
	int fbo_x_off, fbo_y_off;					\
	pixmap_priv_get_fbo_off(priv, &fbo_x_off, &fbo_y_off);		\
	tx1 = x1 + fbo_x_off; 						\
	tx2 = x2 + fbo_x_off;						\
	ty1 = y1 + fbo_y_off;						\
	ty2 = y2 + fbo_y_off;						\
	_glamor_set_normalize_tcoords(xscale, yscale, tx1, ty1,		\
                                      tx2, ty2, vertices,               \
				   stride);				\
     } else								\
	_glamor_set_normalize_tcoords(xscale, yscale, x1, y1,		\
                                      x2, y2, vertices, stride);        \
 } while(0)

#define glamor_set_repeat_normalize_tcoords_ext(pixmap, priv, repeat_type, \
					    xscale, yscale,		\
					    _x1_, _y1_, _x2_, _y2_,	\
	                                    vertices, stride)		\
  do {									\
     if (_X_UNLIKELY(glamor_pixmap_priv_is_large(priv))) {		\
	float tx1, tx2, ty1, ty2;					\
	if (repeat_type == RepeatPad) {					\
		tx1 = _x1_ - priv->box.x1;			        \
		ty1 = _y1_ - priv->box.y1;			        \
		tx2 = tx1 + ((_x2_) - (_x1_));				\
		ty2 = ty1 + ((_y2_) - (_y1_));				\
	} else {							\
            glamor_get_repeat_coords(pixmap, priv, repeat_type,         \
				 tx1, ty1, tx2, ty2,			\
				 _x1_, _y1_, _x2_, _y2_);		\
	}								\
	_glamor_set_normalize_tcoords(xscale, yscale, tx1, ty1,		\
                                      tx2, ty2, vertices,               \
				   stride);				\
     } else								\
	_glamor_set_normalize_tcoords(xscale, yscale, _x1_, _y1_,	\
                                      _x2_, _y2_, vertices,             \
				   stride);				\
 } while(0)

#define glamor_set_normalize_tcoords_tri_stripe(xscale, yscale,		\
						x1, y1, x2, y2,		\
						vertices)               \
    do {								\
	(vertices)[0] = t_from_x_coord_x(xscale, x1);			\
	(vertices)[2] = t_from_x_coord_x(xscale, x2);			\
	(vertices)[6] = (vertices)[2];					\
	(vertices)[4] = (vertices)[0];					\
        (vertices)[1] = t_from_x_coord_y(yscale, y1);                   \
        (vertices)[7] = t_from_x_coord_y(yscale, y2);                   \
	(vertices)[3] = (vertices)[1];					\
	(vertices)[5] = (vertices)[7];					\
    } while(0)

#define glamor_set_tcoords_tri_strip(x1, y1, x2, y2, vertices)          \
    do {								\
	(vertices)[0] = (x1);						\
	(vertices)[2] = (x2);						\
	(vertices)[6] = (vertices)[2];					\
	(vertices)[4] = (vertices)[0];					\
        (vertices)[1] = (y1);                                           \
        (vertices)[7] = (y2);                                           \
	(vertices)[3] = (vertices)[1];					\
	(vertices)[5] = (vertices)[7];					\
    } while(0)

#define glamor_set_normalize_vcoords_ext(priv, xscale, yscale,		\
				     x1, y1, x2, y2,			\
                                         vertices, stride)              \
  do {									\
    int fbo_x_off, fbo_y_off;						\
    /* vertices may be write-only, so we use following			\
     * temporary variable. */						\
    float _t0_, _t1_, _t2_, _t5_;					\
    pixmap_priv_get_fbo_off(priv, &fbo_x_off, &fbo_y_off);		\
    (vertices)[0] = _t0_ = v_from_x_coord_x(xscale, x1 + fbo_x_off);	\
    (vertices)[1 * stride] = _t2_ = v_from_x_coord_x(xscale,		\
					x2 + fbo_x_off);		\
    (vertices)[2 * stride] = _t2_;					\
    (vertices)[3 * stride] = _t0_;					\
    (vertices)[1] = _t1_ = v_from_x_coord_y(yscale, y1 + fbo_y_off);    \
    (vertices)[2 * stride + 1] = _t5_ =                                 \
        v_from_x_coord_y(yscale, y2 + fbo_y_off);                       \
    (vertices)[1 * stride + 1] = _t1_;					\
    (vertices)[3 * stride + 1] = _t5_;					\
  } while(0)

#define glamor_set_normalize_vcoords_tri_strip(xscale, yscale,		\
					       x1, y1, x2, y2,		\
					       vertices)		\
    do {								\
	(vertices)[0] = v_from_x_coord_x(xscale, x1);			\
	(vertices)[2] = v_from_x_coord_x(xscale, x2);			\
	(vertices)[6] = (vertices)[2];					\
	(vertices)[4] = (vertices)[0];					\
        (vertices)[1] = v_from_x_coord_y(yscale, y1);                   \
        (vertices)[7] = v_from_x_coord_y(yscale, y2);                   \
	(vertices)[3] = (vertices)[1];					\
	(vertices)[5] = (vertices)[7];					\
    } while(0)

#define glamor_set_normalize_pt(xscale, yscale, x, y,		\
                                pt)				\
    do {							\
        (pt)[0] = t_from_x_coord_x(xscale, x);			\
        (pt)[1] = t_from_x_coord_y(yscale, y);                  \
    } while(0)

#define glamor_set_circle_centre(width, height, x, y,	\
				 c)		\
    do {						\
        (c)[0] = (float)x;				\
        (c)[1] = (float)y;				\
    } while(0)

#define ALIGN(i,m)	(((i) + (m) - 1) & ~((m) - 1))
#define MIN(a,b)	((a) < (b) ? (a) : (b))
#define MAX(a,b)	((a) > (b) ? (a) : (b))

#define glamor_check_fbo_size(_glamor_,_w_, _h_)    ((_w_) > 0 && (_h_) > 0 \
                                                    && (_w_) <= _glamor_->max_fbo_size  \
                                                    && (_h_) <= _glamor_->max_fbo_size)

#define GLAMOR_PIXMAP_PRIV_HAS_FBO(pixmap_priv)    (pixmap_priv->gl_fbo == GLAMOR_FBO_NORMAL)

#define REVERT_NONE       		0
#define REVERT_NORMAL     		1
#define REVERT_UPLOADING_A1		3

#define SWAP_UPLOADING	  	2
#define SWAP_NONE_UPLOADING	3

/* borrowed from uxa */
static inline Bool
glamor_get_rgba_from_pixel(CARD32 pixel,
                           float *red,
                           float *green,
                           float *blue, float *alpha, CARD32 format)
{
    int rbits, bbits, gbits, abits;
    int rshift, bshift, gshift, ashift;

    rbits = PICT_FORMAT_R(format);
    gbits = PICT_FORMAT_G(format);
    bbits = PICT_FORMAT_B(format);
    abits = PICT_FORMAT_A(format);

    if (PICT_FORMAT_TYPE(format) == PICT_TYPE_A) {
        rshift = gshift = bshift = ashift = 0;
    }
    else if (PICT_FORMAT_TYPE(format) == PICT_TYPE_ARGB) {
        bshift = 0;
        gshift = bbits;
        rshift = gshift + gbits;
        ashift = rshift + rbits;
    }
    else if (PICT_FORMAT_TYPE(format) == PICT_TYPE_ABGR) {
        rshift = 0;
        gshift = rbits;
        bshift = gshift + gbits;
        ashift = bshift + bbits;
    }
    else if (PICT_FORMAT_TYPE(format) == PICT_TYPE_BGRA) {
        ashift = 0;
        rshift = abits;
        if (abits == 0)
            rshift = PICT_FORMAT_BPP(format) - (rbits + gbits + bbits);
        gshift = rshift + rbits;
        bshift = gshift + gbits;
    }
    else {
        return FALSE;
    }
#define COLOR_INT_TO_FLOAT(_fc_, _p_, _s_, _bits_)	\
  *_fc_ = (((_p_) >> (_s_)) & (( 1 << (_bits_)) - 1))	\
    / (float)((1<<(_bits_)) - 1)

    if (rbits)
        COLOR_INT_TO_FLOAT(red, pixel, rshift, rbits);
    else
        *red = 0;

    if (gbits)
        COLOR_INT_TO_FLOAT(green, pixel, gshift, gbits);
    else
        *green = 0;

    if (bbits)
        COLOR_INT_TO_FLOAT(blue, pixel, bshift, bbits);
    else
        *blue = 0;

    if (abits)
        COLOR_INT_TO_FLOAT(alpha, pixel, ashift, abits);
    else
        *alpha = 1;

    return TRUE;
}

static inline void
glamor_get_rgba_from_color(const xRenderColor *color, float rgba[4])
{
    rgba[0] = color->red   / (float)UINT16_MAX;
    rgba[1] = color->green / (float)UINT16_MAX;
    rgba[2] = color->blue  / (float)UINT16_MAX;
    rgba[3] = color->alpha / (float)UINT16_MAX;
}

inline static Bool
glamor_is_large_pixmap(PixmapPtr pixmap)
{
    glamor_pixmap_private *priv;

    priv = glamor_get_pixmap_private(pixmap);
    return (glamor_pixmap_priv_is_large(priv));
}

static inline void
glamor_make_current(glamor_screen_private *glamor_priv)
{
    if (lastGLContext != glamor_priv->ctx.ctx) {
        lastGLContext = glamor_priv->ctx.ctx;
        glamor_priv->ctx.make_current(&glamor_priv->ctx);
    }
}

static inline BoxRec
glamor_no_rendering_bounds(void)
{
    BoxRec bounds = {
        .x1 = 0,
        .y1 = 0,
        .x2 = MAXSHORT,
        .y2 = MAXSHORT,
    };

    return bounds;
}

static inline BoxRec
glamor_start_rendering_bounds(void)
{
    BoxRec bounds = {
        .x1 = MAXSHORT,
        .y1 = MAXSHORT,
        .x2 = 0,
        .y2 = 0,
    };

    return bounds;
}

static inline void
glamor_bounds_union_rect(BoxPtr bounds, xRectangle *rect)
{
    bounds->x1 = min(bounds->x1, rect->x);
    bounds->y1 = min(bounds->y1, rect->y);
    bounds->x2 = min(SHRT_MAX, max(bounds->x2, rect->x + rect->width));
    bounds->y2 = min(SHRT_MAX, max(bounds->y2, rect->y + rect->height));
}

static inline void
glamor_bounds_union_box(BoxPtr bounds, BoxPtr box)
{
    bounds->x1 = min(bounds->x1, box->x1);
    bounds->y1 = min(bounds->y1, box->y1);
    bounds->x2 = max(bounds->x2, box->x2);
    bounds->y2 = max(bounds->y2, box->y2);
}

/**
 * Helper function for implementing draws with GL_QUADS on GLES2,
 * where we don't have them.
 */
static inline void
glamor_glDrawArrays_GL_QUADS(glamor_screen_private *glamor_priv, unsigned count)
{
    if (glamor_priv->use_quads) {
        glDrawArrays(GL_QUADS, 0, count * 4);
    } else {
        glamor_gldrawarrays_quads_using_indices(glamor_priv, count);
    }
}

static inline Bool
glamor_glsl_has_ints(glamor_screen_private *glamor_priv) {
    return glamor_priv->glsl_version >= 130 || glamor_priv->use_gpu_shader4;
}

#endif
