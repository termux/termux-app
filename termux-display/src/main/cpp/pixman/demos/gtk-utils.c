#include <gtk/gtk.h>
#ifdef HAVE_CONFIG_H
#include <pixman-config.h>
#endif
#include "utils.h"
#include "gtk-utils.h"

pixman_image_t *
pixman_image_from_file (const char *filename, pixman_format_code_t format)
{
    GdkPixbuf *pixbuf;
    pixman_image_t *image;
    int width, height;
    uint32_t *data, *d;
    uint8_t *gdk_data;
    int n_channels;
    int j, i;
    int stride;

    if (!(pixbuf = gdk_pixbuf_new_from_file (filename, NULL)))
	return NULL;

    image = NULL;

    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);
    n_channels = gdk_pixbuf_get_n_channels (pixbuf);
    gdk_data = gdk_pixbuf_get_pixels (pixbuf);
    stride = gdk_pixbuf_get_rowstride (pixbuf);

    if (!(data = malloc (width * height * sizeof (uint32_t))))
	goto out;

    d = data;
    for (j = 0; j < height; ++j)
    {
	uint8_t *gdk_line = gdk_data;

	for (i = 0; i < width; ++i)
	{
	    int r, g, b, a;
	    uint32_t pixel;

	    r = gdk_line[0];
	    g = gdk_line[1];
	    b = gdk_line[2];

	    if (n_channels == 4)
		a = gdk_line[3];
	    else
		a = 0xff;

	    r = (r * a + 127) / 255;
	    g = (g * a + 127) / 255;
	    b = (b * a + 127) / 255;

	    pixel = (a << 24) | (r << 16) | (g << 8) | b;

	    *d++ = pixel;
	    gdk_line += n_channels;
	}

	gdk_data += stride;
    }

    image = pixman_image_create_bits (
	format, width, height, data, width * 4);

out:
    g_object_unref (pixbuf);
    return image;
}

GdkPixbuf *
pixbuf_from_argb32 (uint32_t *bits,
		    int width,
		    int height,
		    int stride)
{
    GdkPixbuf *pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE,
					8, width, height);
    int p_stride = gdk_pixbuf_get_rowstride (pixbuf);
    guint32 *p_bits = (guint32 *)gdk_pixbuf_get_pixels (pixbuf);
    int i;

    for (i = 0; i < height; ++i)
    {
	uint32_t *src_row = &bits[i * (stride / 4)];
	uint32_t *dst_row = p_bits + i * (p_stride / 4);

	a8r8g8b8_to_rgba_np (dst_row, src_row, width);
    }

    return pixbuf;
}

static gboolean
on_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data)
{
    pixman_image_t *pimage = user_data;
    int width = pixman_image_get_width (pimage);
    int height = pixman_image_get_height (pimage);
    int stride = pixman_image_get_stride (pimage);
    cairo_surface_t *cimage;
    cairo_format_t format;

    if (pixman_image_get_format (pimage) == PIXMAN_x8r8g8b8)
	format = CAIRO_FORMAT_RGB24;
    else
	format = CAIRO_FORMAT_ARGB32;

    cimage = cairo_image_surface_create_for_data (
	(uint8_t *)pixman_image_get_data (pimage),
	format, width, height, stride);

    cairo_rectangle (cr, 0, 0, width, height);
    cairo_set_source_surface (cr, cimage, 0, 0);
    cairo_fill (cr);

    cairo_surface_destroy (cimage);
    
    return TRUE;
}

void
show_image (pixman_image_t *image)
{
    GtkWidget *window;
    int width, height;
    int argc;
    char **argv;
    char *arg0 = g_strdup ("pixman-test-program");
    pixman_format_code_t format;
    pixman_image_t *copy;

    argc = 1;
    argv = (char **)&arg0;

    gtk_init (&argc, &argv);
    
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    width = pixman_image_get_width (image);
    height = pixman_image_get_height (image);

    gtk_window_set_default_size (GTK_WINDOW (window), width, height);

    format = pixman_image_get_format (image);

    /* We always display the image as if it contains sRGB data. That
     * means that no conversion should take place when the image
     * has the a8r8g8b8_sRGB format.
     */
    switch (format)
    {
    case PIXMAN_a8r8g8b8_sRGB:
    case PIXMAN_a8r8g8b8:
    case PIXMAN_x8r8g8b8:
	copy = pixman_image_ref (image);
	break;

    default:
	copy = pixman_image_create_bits (PIXMAN_a8r8g8b8,
					 width, height, NULL, -1);
	pixman_image_composite32 (PIXMAN_OP_SRC,
				  image, NULL, copy,
				  0, 0, 0, 0, 0, 0,
				  width, height);
	break;
    }

    g_signal_connect (window, "draw", G_CALLBACK (on_draw), copy);
    g_signal_connect (window, "delete_event", G_CALLBACK (gtk_main_quit), NULL);
    
    gtk_widget_show (window);
    
    gtk_main ();
}
