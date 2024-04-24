/*
 * Copyright 2012, Red Hat, Inc.
 * Copyright 2012, Soren Sandmann
 * Copyright 2018, Basile Clement
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
#ifdef HAVE_CONFIG_H
#include "pixman-config.h"
#endif
#include <math.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include "utils.h"
#include "gtk-utils.h"

#define WIDTH 1024
#define HEIGHT 640

typedef struct
{
    GtkBuilder *         builder;
    pixman_image_t *	 original;
    pixman_format_code_t format;
    pixman_dither_t      dither;
    int                  width;
    int                  height;
} app_t;

static GtkWidget *
get_widget (app_t *app, const char *name)
{
    GtkWidget *widget = GTK_WIDGET (gtk_builder_get_object (app->builder, name));

    if (!widget)
	g_error ("Widget %s not found\n", name);

    return widget;
}

typedef struct
{
    char	name [20];
    int		value;
} named_int_t;

static const named_int_t formats[] =
{
    { "a8r8g8b8",  PIXMAN_a8r8g8b8      },
    { "rgb",       PIXMAN_rgb_float     },
    { "sRGB",      PIXMAN_a8r8g8b8_sRGB },
    { "r5g6b5",    PIXMAN_r5g6b5        },
    { "a4r4g4b4",  PIXMAN_a4r4g4b4      },
    { "a2r2g2b2",  PIXMAN_a2r2g2b2      },
    { "r3g3b2",    PIXMAN_r3g3b2        },
    { "r1g2b1",    PIXMAN_r1g2b1        },
    { "a1r1g1b1",  PIXMAN_a1r1g1b1      },
};

static const named_int_t dithers[] =
{
    { "None",                   PIXMAN_REPEAT_NONE },
    { "Bayer 8x8",              PIXMAN_DITHER_ORDERED_BAYER_8 },
    { "Blue noise 64x64",       PIXMAN_DITHER_ORDERED_BLUE_NOISE_64 },
};

static int
get_value (app_t *app, const named_int_t table[], const char *box_name)
{
    GtkComboBox *box = GTK_COMBO_BOX (get_widget (app, box_name));

    return table[gtk_combo_box_get_active (box)].value;
}

static void
rescale (GtkWidget *may_be_null, app_t *app)
{
    app->dither = get_value (app, dithers, "dithering_combo_box");
    app->format = get_value (app, formats, "target_format_combo_box");

    gtk_widget_set_size_request (
	get_widget (app, "drawing_area"), app->width + 0.5, app->height + 0.5);

    gtk_widget_queue_draw (
	get_widget (app, "drawing_area"));
}

static gboolean
on_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    app_t *app = user_data;
    GdkRectangle area;
    cairo_surface_t *surface;
    pixman_image_t *tmp, *final;
    uint32_t *pixels;

    gdk_cairo_get_clip_rectangle(cr, &area);

    tmp = pixman_image_create_bits (
	app->format, area.width, area.height, NULL, 0);
    pixman_image_set_dither (tmp, app->dither);

    pixman_image_composite (
	PIXMAN_OP_SRC,
	app->original, NULL, tmp,
	area.x, area.y, 0, 0, 0, 0,
	app->width - area.x,
	app->height - area.y);

    pixels = calloc (1, area.width * area.height * 4);
    final = pixman_image_create_bits (
	PIXMAN_a8r8g8b8, area.width, area.height, pixels, area.width * 4);

    pixman_image_composite (
	PIXMAN_OP_SRC,
	tmp, NULL, final,
	area.x, area.y, 0, 0, 0, 0,
	app->width - area.x,
	app->height - area.y);

    surface = cairo_image_surface_create_for_data (
	(uint8_t *)pixels, CAIRO_FORMAT_ARGB32,
	area.width, area.height, area.width * 4);

    cairo_set_source_surface (cr, surface, area.x, area.y);

    cairo_paint (cr);

    cairo_surface_destroy (surface);
    free (pixels);
    pixman_image_unref (final);
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

static app_t *
app_new (pixman_image_t *original)
{
    GtkWidget *widget;
    app_t *app = g_malloc (sizeof *app);
    GError *err = NULL;

    app->builder = gtk_builder_new ();
    app->original = original;

    if (original->type == BITS)
    {
	app->width = pixman_image_get_width (original);
	app->height = pixman_image_get_height (original);
    }
    else
    {
	app->width = WIDTH;
	app->height = HEIGHT;
    }

    if (!gtk_builder_add_from_file (app->builder, "dither.ui", &err))
	g_error ("Could not read file dither.ui: %s", err->message);

    widget = get_widget (app, "drawing_area");
    g_signal_connect (widget, "draw", G_CALLBACK (on_draw), app);

    set_up_combo_box (app, "target_format_combo_box",
		      G_N_ELEMENTS (formats), formats);
    set_up_combo_box (app, "dithering_combo_box",
		      G_N_ELEMENTS (dithers), dithers);

    app->dither = get_value (app, dithers, "dithering_combo_box");
    app->format = get_value (app, formats, "target_format_combo_box");

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
	pixman_gradient_stop_t stops[] = {
	    /* These colors make it very obvious that dithering
	     * is useful even for 8-bit gradients
	     */
	    { 0x00000, { 0x1b1b, 0x5d5d, 0x7c7c, 0xffff } },
	    { 0x10000, { 0x3838, 0x3232, 0x1010, 0xffff } },
	};
	pixman_point_fixed_t p1, p2;

	p1.x = p1.y = 0x0000;
	p2.x = WIDTH << 16;
	p2.y = HEIGHT << 16;

	if (!(image = pixman_image_create_linear_gradient (
		  &p1, &p2, stops, ARRAY_LENGTH (stops))))
	{
	    printf ("Could not create gradient\n");
	    return -1;
	}
    }
    else if (!(image = pixman_image_from_file (argv[1], PIXMAN_a8r8g8b8)))
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
