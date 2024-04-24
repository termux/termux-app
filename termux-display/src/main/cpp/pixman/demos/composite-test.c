#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include "pixman.h"
#include "gtk-utils.h"
#include "parrot.c"

#define WIDTH	80
#define HEIGHT	80

typedef struct {
    const char *name;
    pixman_op_t op;
} operator_t;

static const operator_t operators[] = {
    { "CLEAR",		PIXMAN_OP_CLEAR },
    { "SRC",		PIXMAN_OP_SRC },
    { "DST",		PIXMAN_OP_DST },
    { "OVER",		PIXMAN_OP_OVER },
    { "OVER_REVERSE",	PIXMAN_OP_OVER_REVERSE },
    { "IN",		PIXMAN_OP_IN },
    { "IN_REVERSE",	PIXMAN_OP_IN_REVERSE },
    { "OUT",		PIXMAN_OP_OUT },
    { "OUT_REVERSE",	PIXMAN_OP_OUT_REVERSE },
    { "ATOP",		PIXMAN_OP_ATOP },
    { "ATOP_REVERSE",	PIXMAN_OP_ATOP_REVERSE },
    { "XOR",		PIXMAN_OP_XOR },
    { "ADD",		PIXMAN_OP_ADD },
    { "SATURATE",	PIXMAN_OP_SATURATE },

    { "MULTIPLY",	PIXMAN_OP_MULTIPLY },
    { "SCREEN",		PIXMAN_OP_SCREEN },
    { "OVERLAY",	PIXMAN_OP_OVERLAY },
    { "DARKEN",		PIXMAN_OP_DARKEN },
    { "LIGHTEN",	PIXMAN_OP_LIGHTEN },
    { "COLOR_DODGE",	PIXMAN_OP_COLOR_DODGE },
    { "COLOR_BURN",	PIXMAN_OP_COLOR_BURN },
    { "HARD_LIGHT",	PIXMAN_OP_HARD_LIGHT },
    { "SOFT_LIGHT",	PIXMAN_OP_SOFT_LIGHT },
    { "DIFFERENCE",	PIXMAN_OP_DIFFERENCE },
    { "EXCLUSION",	PIXMAN_OP_EXCLUSION },
    { "HSL_HUE",	PIXMAN_OP_HSL_HUE },
    { "HSL_SATURATION",	PIXMAN_OP_HSL_SATURATION },
    { "HSL_COLOR",	PIXMAN_OP_HSL_COLOR },
    { "HSL_LUMINOSITY",	PIXMAN_OP_HSL_LUMINOSITY },
};

static uint32_t
reader (const void *src, int size)
{
    switch (size)
    {
    case 1:
	return *(uint8_t *)src;
    case 2:
	return *(uint16_t *)src;
    case 4:
	return *(uint32_t *)src;
    default:
	g_assert_not_reached();
    }
}

static void
writer (void *src, uint32_t value, int size)
{
    switch (size)
    {
    case 1:
	*(uint8_t *)src = value;
	break;

    case 2:
	*(uint16_t *)src = value;
	break;

    case 4:
	*(uint32_t *)src = value;
	break;

    default:
        break;
    }
}

int
main (int argc, char **argv)
{
#define d2f pixman_double_to_fixed

    GtkWidget *window, *swindow;
    GtkWidget *table;
    uint32_t *dest = malloc (WIDTH * HEIGHT * 4);
    uint32_t *src = malloc (WIDTH * HEIGHT * 4);
    pixman_image_t *gradient, *parrot;
    pixman_image_t *dest_img;
    pixman_point_fixed_t p1 = { -10 << 16, 10 << 16 };
    pixman_point_fixed_t p2 = { (WIDTH + 10) << 16, (HEIGHT - 10) << 16 };
    uint16_t alpha = 0xdddd;
    pixman_gradient_stop_t stops[6] =
    {
	{ d2f (0.0), { 0xf2f2, 0x8787, 0x7d7d, alpha } },
	{ d2f (0.22), { 0xf3f3, 0xeaea, 0x8383, alpha } },
	{ d2f (0.42), { 0x6b6b, 0xc0c0, 0x7777, alpha } },
	{ d2f (0.57), { 0x4b4b, 0xc9c9, 0xf5f5, alpha } },
	{ d2f (0.75), { 0x6a6a, 0x7f7f, 0xbebe, alpha } },
	{ d2f (1.0), { 0xeded, 0x8282, 0xb0b0, alpha } },
    };

    int i;

    gtk_init (&argc, &argv);

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);

    g_signal_connect (window, "delete-event",
		      G_CALLBACK (gtk_main_quit),
		      NULL);
    table = gtk_table_new (G_N_ELEMENTS (operators) / 6, 6, TRUE);

    gradient = pixman_image_create_linear_gradient (&p1, &p2, stops, G_N_ELEMENTS (stops));
    parrot = pixman_image_create_bits (PIXMAN_a8r8g8b8, WIDTH, HEIGHT, (uint32_t *)parrot_bits, WIDTH * 4);

    pixman_image_set_repeat (gradient, PIXMAN_REPEAT_PAD);

    dest_img = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					 WIDTH, HEIGHT,
					 NULL,
					 WIDTH * 4);
    pixman_image_set_accessors (dest_img, reader, writer);

    for (i = 0; i < G_N_ELEMENTS (operators); ++i)
    {
	GtkWidget *image;
	GdkPixbuf *pixbuf;
	GtkWidget *vbox;
	GtkWidget *label;

	vbox = gtk_vbox_new (FALSE, 0);

	label = gtk_label_new (operators[i].name);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 6);
	gtk_widget_show (label);

	pixman_image_composite (PIXMAN_OP_SRC, gradient, NULL, dest_img,
				0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);
	pixman_image_composite (operators[i].op, parrot, NULL, dest_img,
				0, 0, 0, 0, 0, 0, WIDTH, HEIGHT);
	pixbuf = pixbuf_from_argb32 (pixman_image_get_data (dest_img),
				     WIDTH, HEIGHT, WIDTH * 4);
	image = gtk_image_new_from_pixbuf (pixbuf);
	gtk_box_pack_start (GTK_BOX (vbox), image, FALSE, FALSE, 0);
	gtk_widget_show (image);

	gtk_table_attach_defaults (GTK_TABLE (table), vbox,
				   i % 6, (i % 6) + 1, i / 6, (i / 6) + 1);
	gtk_widget_show (vbox);

	g_object_unref (pixbuf);
    }

    pixman_image_unref (gradient);
    free (src);
    pixman_image_unref (dest_img);
    free (dest);

    swindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swindow),
				    GTK_POLICY_AUTOMATIC,
				    GTK_POLICY_AUTOMATIC);

    gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (swindow), table);
    gtk_widget_show (table);

    gtk_container_add (GTK_CONTAINER (window), swindow);
    gtk_widget_show (swindow);

    gtk_widget_show (window);

    gtk_main ();

    return 0;
}
