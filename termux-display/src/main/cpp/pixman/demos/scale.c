/*
 * Copyright 2012, Red Hat, Inc.
 * Copyright 2012, Soren Sandmann
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
 *
 * Author: Soren Sandmann <soren.sandmann@gmail.com>
 */
#ifdef HAVE_CONFIG_H
#include "pixman-config.h"
#endif
#include <math.h>
#include <gtk/gtk.h>
#include <pixman.h>
#include <stdlib.h>
#include "gtk-utils.h"

typedef struct
{
    GtkBuilder *        builder;
    pixman_image_t *	original;
    GtkAdjustment *     scale_x_adjustment;
    GtkAdjustment *     scale_y_adjustment;
    GtkAdjustment *     rotate_adjustment;
    GtkAdjustment *	subsample_adjustment;
    int                 scaled_width;
    int                 scaled_height;
} app_t;

static GtkWidget *
get_widget (app_t *app, const char *name)
{
    GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (app->builder, name));

    if (!widget)
        g_error ("Widget %s not found\n", name);

    return widget;
}

/* Figure out the boundary of a diameter=1 circle transformed into an ellipse
 * by trans. Proof that this is the correct calculation:
 *
 * Transform x,y to u,v by this matrix calculation:
 *
 *  |u|   |a c| |x|
 *  |v| = |b d|*|y|
 *
 * Horizontal component:
 *
 *  u = ax+cy (1)
 *
 * For each x,y on a radius-1 circle (p is angle to the point):
 *
 *  x^2+y^2 = 1
 *  x = cos(p)
 *  y = sin(p)
 *  dx/dp = -sin(p) = -y
 *  dy/dp = cos(p) = x
 *
 * Figure out derivative of (1) relative to p:
 *
 *  du/dp = a(dx/dp) + c(dy/dp)
 *        = -ay + cx
 *
 * The min and max u are when du/dp is zero:
 *
 *  -ay + cx = 0
 *  cx = ay
 *  c = ay/x  (2)
 *  y = cx/a  (3)
 *
 * Substitute (2) into (1) and simplify:
 *
 *  u = ax + ay^2/x
 *    = a(x^2+y^2)/x
 *    = a/x (because x^2+y^2 = 1)
 *  x = a/u (4)
 *
 * Substitute (4) into (3) and simplify:
 *
 *  y = c(a/u)/a
 *  y = c/u (5)
 *
 * Square (4) and (5) and add:
 *
 *  x^2+y^2 = (a^2+c^2)/u^2
 *
 * But x^2+y^2 is 1:
 *
 *  1 = (a^2+c^2)/u^2
 *  u^2 = a^2+c^2
 *  u = hypot(a,c)
 *
 * Similarily the max/min of v is at:
 *
 *  v = hypot(b,d)
 *
 */
static void
compute_extents (pixman_f_transform_t *trans, double *sx, double *sy)
{
    *sx = hypot (trans->m[0][0], trans->m[0][1]) / trans->m[2][2];
    *sy = hypot (trans->m[1][0], trans->m[1][1]) / trans->m[2][2];
}

typedef struct
{
    char	name [20];
    int		value;
} named_int_t;

static const named_int_t filters[] =
{
    { "Box",			PIXMAN_KERNEL_BOX },
    { "Impulse",		PIXMAN_KERNEL_IMPULSE },
    { "Linear",			PIXMAN_KERNEL_LINEAR },
    { "Cubic",			PIXMAN_KERNEL_CUBIC },
    { "Lanczos2",		PIXMAN_KERNEL_LANCZOS2 },
    { "Lanczos3",		PIXMAN_KERNEL_LANCZOS3 },
    { "Lanczos3 Stretched",	PIXMAN_KERNEL_LANCZOS3_STRETCHED },
    { "Gaussian",		PIXMAN_KERNEL_GAUSSIAN },
};

static const named_int_t repeats[] =
{
    { "None",                   PIXMAN_REPEAT_NONE },
    { "Normal",                 PIXMAN_REPEAT_NORMAL },
    { "Reflect",                PIXMAN_REPEAT_REFLECT },
    { "Pad",                    PIXMAN_REPEAT_PAD },
};

static int
get_value (app_t *app, const named_int_t table[], const char *box_name)
{
    GtkComboBox *box = GTK_COMBO_BOX (get_widget (app, box_name));

    return table[gtk_combo_box_get_active (box)].value;
}

static void
copy_to_counterpart (app_t *app, GObject *object)
{
    static const char *xy_map[] =
    {
	"reconstruct_x_combo_box", "reconstruct_y_combo_box",
	"sample_x_combo_box",      "sample_y_combo_box",
	"scale_x_adjustment",      "scale_y_adjustment",
    };
    GObject *counterpart = NULL;
    int i;

    for (i = 0; i < G_N_ELEMENTS (xy_map); i += 2)
    {
	GObject *x = gtk_builder_get_object (app->builder, xy_map[i]);
	GObject *y = gtk_builder_get_object (app->builder, xy_map[i + 1]);

	if (object == x)
	    counterpart = y;
	if (object == y)
	    counterpart = x;
    }

    if (!counterpart)
	return;
    
    if (GTK_IS_COMBO_BOX (counterpart))
    {
	gtk_combo_box_set_active (
	    GTK_COMBO_BOX (counterpart),
	    gtk_combo_box_get_active (
		GTK_COMBO_BOX (object)));
    }
    else if (GTK_IS_ADJUSTMENT (counterpart))
    {
	gtk_adjustment_set_value (
	    GTK_ADJUSTMENT (counterpart),
	    gtk_adjustment_get_value (
		GTK_ADJUSTMENT (object)));
    }
}

static double
to_scale (double v)
{
    return pow (1.15, v);
}

static void
rescale (GtkWidget *may_be_null, app_t *app)
{
    pixman_f_transform_t ftransform;
    pixman_transform_t transform;
    double new_width, new_height;
    double fscale_x, fscale_y;
    double rotation;
    pixman_fixed_t *params;
    int n_params;
    double sx, sy;

    pixman_f_transform_init_identity (&ftransform);

    if (may_be_null && gtk_toggle_button_get_active (
	    GTK_TOGGLE_BUTTON (get_widget (app, "lock_checkbutton"))))
    {
	copy_to_counterpart (app, G_OBJECT (may_be_null));
    }
    
    fscale_x = gtk_adjustment_get_value (app->scale_x_adjustment);
    fscale_y = gtk_adjustment_get_value (app->scale_y_adjustment);
    rotation = gtk_adjustment_get_value (app->rotate_adjustment);

    fscale_x = to_scale (fscale_x);
    fscale_y = to_scale (fscale_y);
    
    new_width = pixman_image_get_width (app->original) * fscale_x;
    new_height = pixman_image_get_height (app->original) * fscale_y;

    pixman_f_transform_scale (&ftransform, NULL, fscale_x, fscale_y);

    pixman_f_transform_translate (&ftransform, NULL, - new_width / 2.0, - new_height / 2.0);

    rotation = (rotation / 360.0) * 2 * M_PI;
    pixman_f_transform_rotate (&ftransform, NULL, cos (rotation), sin (rotation));

    pixman_f_transform_translate (&ftransform, NULL, new_width / 2.0, new_height / 2.0);

    pixman_f_transform_invert (&ftransform, &ftransform);

    compute_extents (&ftransform, &sx, &sy);
    
    pixman_transform_from_pixman_f_transform (&transform, &ftransform);
    pixman_image_set_transform (app->original, &transform);

    params = pixman_filter_create_separable_convolution (
        &n_params,
        sx * 65536.0 + 0.5,
	sy * 65536.0 + 0.5,
	get_value (app, filters, "reconstruct_x_combo_box"),
	get_value (app, filters, "reconstruct_y_combo_box"),
	get_value (app, filters, "sample_x_combo_box"),
	get_value (app, filters, "sample_y_combo_box"),
	gtk_adjustment_get_value (app->subsample_adjustment),
	gtk_adjustment_get_value (app->subsample_adjustment));

    pixman_image_set_filter (app->original, PIXMAN_FILTER_SEPARABLE_CONVOLUTION, params, n_params);

    pixman_image_set_repeat (
        app->original, get_value (app, repeats, "repeat_combo_box"));
    
    free (params);

    app->scaled_width = ceil (new_width);
    app->scaled_height = ceil (new_height);
    
    gtk_widget_set_size_request (
        get_widget (app, "drawing_area"), new_width + 0.5, new_height + 0.5);

    gtk_widget_queue_draw (
        get_widget (app, "drawing_area"));
}

static gboolean
on_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    app_t *app = user_data;
    GdkRectangle area;
    cairo_surface_t *surface;
    pixman_image_t *tmp;
    uint32_t *pixels;

    gdk_cairo_get_clip_rectangle(cr, &area);

    pixels = calloc (1, area.width * area.height * 4);
    tmp = pixman_image_create_bits (
        PIXMAN_a8r8g8b8, area.width, area.height, pixels, area.width * 4);

    if (area.x < app->scaled_width && area.y < app->scaled_height)
    {
        pixman_image_composite (
            PIXMAN_OP_SRC,
            app->original, NULL, tmp,
            area.x, area.y, 0, 0, 0, 0,
            app->scaled_width - area.x, app->scaled_height - area.y);
    }

    surface = cairo_image_surface_create_for_data (
        (uint8_t *)pixels, CAIRO_FORMAT_ARGB32,
        area.width, area.height, area.width * 4);

    cairo_set_source_surface (cr, surface, area.x, area.y);

    cairo_paint (cr);

    cairo_surface_destroy (surface);
    free (pixels);
    pixman_image_unref (tmp);

    return TRUE;
}

static void
set_up_combo_box (app_t *app, const char *box_name,
                  int n_entries, const named_int_t table[])
{
    GtkWidget *widget = get_widget (app, box_name);
    GtkListStore *model;
    GtkCellRenderer *cell;
    int i;

    model = gtk_list_store_new (1, G_TYPE_STRING);
    
    cell = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), cell, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (widget), cell,
				    "text", 0,
				    NULL);

    gtk_combo_box_set_model (GTK_COMBO_BOX (widget), GTK_TREE_MODEL (model));
    
    for (i = 0; i < n_entries; ++i)
    {
	const named_int_t *info = &(table[i]);
	GtkTreeIter iter;

	gtk_list_store_append (model, &iter);
	gtk_list_store_set (model, &iter, 0, info->name, -1);
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (widget), 0);

    g_signal_connect (widget, "changed", G_CALLBACK (rescale), app);
}

static void
set_up_filter_box (app_t *app, const char *box_name)
{
    set_up_combo_box (app, box_name, G_N_ELEMENTS (filters), filters);
}

static char *
format_value (GtkWidget *widget, double value)
{
    return g_strdup_printf ("%.4f", to_scale (value));
}

static app_t *
app_new (pixman_image_t *original)
{
    GtkWidget *widget;
    app_t *app = g_malloc (sizeof *app);
    GError *err = NULL;

    app->builder = gtk_builder_new ();
    app->original = original;

    if (!gtk_builder_add_from_file (app->builder, "scale.ui", &err))
	g_error ("Could not read file scale.ui: %s", err->message);

    app->scale_x_adjustment =
        GTK_ADJUSTMENT (gtk_builder_get_object (app->builder, "scale_x_adjustment"));
    app->scale_y_adjustment =
        GTK_ADJUSTMENT (gtk_builder_get_object (app->builder, "scale_y_adjustment"));
    app->rotate_adjustment =
        GTK_ADJUSTMENT (gtk_builder_get_object (app->builder, "rotate_adjustment"));
    app->subsample_adjustment =
	GTK_ADJUSTMENT (gtk_builder_get_object (app->builder, "subsample_adjustment"));

    g_signal_connect (app->scale_x_adjustment, "value_changed", G_CALLBACK (rescale), app);
    g_signal_connect (app->scale_y_adjustment, "value_changed", G_CALLBACK (rescale), app);
    g_signal_connect (app->rotate_adjustment, "value_changed", G_CALLBACK (rescale), app);
    g_signal_connect (app->subsample_adjustment, "value_changed", G_CALLBACK (rescale), app);
    
    widget = get_widget (app, "scale_x_scale");
    gtk_scale_add_mark (GTK_SCALE (widget), 0.0, GTK_POS_LEFT, NULL);
    g_signal_connect (widget, "format_value", G_CALLBACK (format_value), app);
    widget = get_widget (app, "scale_y_scale");
    gtk_scale_add_mark (GTK_SCALE (widget), 0.0, GTK_POS_LEFT, NULL);
    g_signal_connect (widget, "format_value", G_CALLBACK (format_value), app);
    widget = get_widget (app, "rotate_scale");
    gtk_scale_add_mark (GTK_SCALE (widget), 0.0, GTK_POS_LEFT, NULL);

    widget = get_widget (app, "drawing_area");
    g_signal_connect (widget, "draw", G_CALLBACK (on_draw), app);

    set_up_filter_box (app, "reconstruct_x_combo_box");
    set_up_filter_box (app, "reconstruct_y_combo_box");
    set_up_filter_box (app, "sample_x_combo_box");
    set_up_filter_box (app, "sample_y_combo_box");

    set_up_combo_box (
        app, "repeat_combo_box", G_N_ELEMENTS (repeats), repeats);

    g_signal_connect (
	gtk_builder_get_object (app->builder, "lock_checkbutton"),
	"toggled", G_CALLBACK (rescale), app);
    
    rescale (NULL, app);
    
    return app;
}

int
main (int argc, char **argv)
{
    GtkWidget *window;
    pixman_image_t *image;
    app_t *app;
    
    gtk_init (&argc, &argv);

    if (argc < 2)
    {
	printf ("%s <image file>\n", argv[0]);
	return -1;
    }

    if (!(image = pixman_image_from_file (argv[1], PIXMAN_a8r8g8b8)))
    {
	printf ("Could not load image \"%s\"\n", argv[1]);
	return -1;
    }

    app = app_new (image);
    
    window = get_widget (app, "main");

    g_signal_connect (window, "delete_event", G_CALLBACK (gtk_main_quit), NULL);
    
    gtk_window_set_default_size (GTK_WINDOW (window), 1024, 768);
    
    gtk_widget_show_all (window);
    
    gtk_main ();

    return 0;
}
